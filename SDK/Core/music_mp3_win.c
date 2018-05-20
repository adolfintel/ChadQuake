/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2007-2008 MH, Reckless
Copyright (C) 2009-2014 Baker and others

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
// music_mp3_win.c - mp3 player


#include "environment.h"

#ifdef PLATFORM_WINDOWS


#ifdef _MSC_VER // Temp .. should read environment.h

    #ifndef __VISUAL_STUDIO_6__
        // Workaround for DirectX 9.0 SDK overriding include paths causing this to be undefined
        // connect.microsoft.com/VisualStudio/feedback/details/508204/vc10-0-errors-while-compiling-winnt-h
        #define POINTER_64 __ptr64
    #endif // ! __VISUAL_STUDIO_6__

    #include "core.h"
    #include "core_windows.h"
    #include <objbase.h>

    #define COBJMACROS
    #include <dshow.h>
    #pragma comment (lib, "strmiids.lib") // dxsdk/sdk8/lib/


    ///////////////////////////////////////////////////////////////////////////////
    //  LOCAL SETUP
    ///////////////////////////////////////////////////////////////////////////////

    #define MLOCAL(_m) struct mlocal *_m = (struct mlocal *) me->_local

    // Begin ...

    static const char * const _tag = "mp3"; // TAG
    #define mobj_t musicplayer_t			// object type
    #define mlocal _mp3_local				// private data struct

    struct mlocal							// private data
    {
    // private platform variables
        IGraphBuilder *pGraph;
        IMediaControl *pControl;
        IMediaEventEx *pEvent;
        IBasicAudio	  *pAudio;
        IMediaSeeking *pSeek;
    };


    ///////////////////////////////////////////////////////////////////////////////
    //  INTERNAL FUNCTIONS
    ///////////////////////////////////////////////////////////////////////////////

    static void sShowPos (mobj_t *me)
    {
    #if 0
        MLOCAL(m);

        LONGLONG pos1, pos2;
        int k;

        IMediaSeeking_GetPositions (m->pSeek, &pos1, &pos2);
        VID_Local_Set_Window_Caption (   va("%i - CD track position: pos1 %i pos2 %i", k, (int)pos1, (int)pos2 ) );
    #endif
    }

    #pragma message ("Need to verify looping.  And lack of looping.  Behaviors ...")

    // Tells the system the current volume level, do not check against previous volume as we might need to re-tell the system
    // unconditionally on new playback, etc.
    // Called when volume changes in a frame or on new playback
    static void sRefreshVolume (mobj_t *me)
    {
        float volumepct = me->getvolumepct ? *me->getvolumepct : 1;
        if (me->playing)
        {
            MLOCAL(m);
            // put_Volume uses an exponential decibel-based scale going from -10000 (no sound) to 0 (full volume)
            // each 100 represents 1 db.

            int db = 0;

            loghushed (HUSHLEVEL_MP3, "%s: refresh volumepct is %g", _tag, volumepct);
            if (volumepct <= 0)
                db = -10000;
            else if (volumepct >= 1)
                db = 0;
            else
                db = log10 (volumepct) * 2000;

            IBasicAudio_put_Volume	(m->pAudio, db);
            me->volumepct = volumepct;
            loghushed (HUSHLEVEL_MP3, "%s: refresh volume %d", _tag, db);
        }
    }

    static void sWaitForFilterState (IMediaControl *pControl, OAFilterState DesiredState)
    {
        OAFilterState MP3FS;
        HRESULT hr;

        while (1)
        {
            hr = IMediaControl_GetState(pControl, 1, &MP3FS);

            if (FAILED (hr)) continue;
            if (MP3FS == DesiredState) return;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    //  PUBLIC OBJECT FUNCTIONS:
    ///////////////////////////////////////////////////////////////////////////////

    static void Stop (mobj_t *me)
    {
        if (me->playing)
        {
            MLOCAL(m);

            IMediaEventEx_SetNotifyWindow	(m->pEvent, (OAHWND) NULL, 0, 0);
            IMediaControl_Stop				(m->pControl);
            sWaitForFilterState				(m->pControl, State_Stopped);
            IMediaControl_Release			(m->pControl);
            IMediaEventEx_Release			(m->pEvent);
            IBasicAudio_Release				(m->pAudio);
            IMediaSeeking_Release			(m->pSeek);
            IGraphBuilder_Release			(m->pGraph);
            CoUninitialize ();

            // Reset stuff
            m->pGraph = NULL;
            m->pControl = NULL;
            m->pEvent = NULL;
            m->pAudio = NULL;
            m->pSeek = NULL;

            me->playing = false;
            me->paused = false;

            loghushed (HUSHLEVEL_MP3, "%s: stop", _tag);
        }
    }


    static void AttachVolume (mobj_t *me, float *pvolume)
    {
        me->getvolumepct = pvolume;
    }


    // Intended to be called once per frame
    // Effectively the main purpose is to update the volume, although it could do other things
    static void Update (mobj_t *me)
    {
        // if (me->playing) ShowPos (me);
        if (me->getvolumepct && *(me->getvolumepct) != me->volumepct)
        {
            sRefreshVolume (me);
            loghushed (HUSHLEVEL_MP3, "%s: volume is %g", _tag, me->volumepct);
        }
    }




    static void sSetPause (mobj_t *me, cbool setpause)
    {
        if (me->paused != setpause)
        {
            MLOCAL(m);

            // don't wait for the filter states here
            switch (setpause)
            {
            case true:	IMediaControl_Pause(m->pControl); break;
            case false:	IMediaControl_Run (m->pControl);  break;
            }

            me->paused = setpause;
            loghushed (HUSHLEVEL_MP3, "%s: %s", _tag, me->paused ? "paused" : "resumed");
        }
    }

    static void Pause (mobj_t *me)
    {
        if (me->playing) sSetPause (me, true);
    }

    static void Resume (mobj_t *me)
    {
        if (me->playing) sSetPause (me, false);
    }


    static cbool Play (mobj_t *me, const char *path_to_file, cbool looping)
    {
        MLOCAL(m);

        // Stop music
        Stop(me);

        do
        {
            #define FAILED_SETUP(_s) { log_fatal ("%s", _s); return false; }

            HRESULT hr = CoInitialize (NULL);

            wchar_t W_absolute_filename[MAX_OSPATH];
            mbstowcs (W_absolute_filename, path_to_file, MAX_OSPATH);

            // Create the filter graph manager and query for interfaces.
            if (FAILED (hr = CoCreateInstance (&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (void **) &m->pGraph)))
                FAILED_SETUP ("CoCreateInstance Failed");
            if (FAILED (hr = IGraphBuilder_QueryInterface(m->pGraph, &IID_IMediaControl, (void **) &m->pControl)))
                FAILED_SETUP ("IID_IMediaControl Failed");
            if (FAILED (hr = IGraphBuilder_QueryInterface(m->pGraph, &IID_IMediaEventEx, (void **) &m->pEvent)))
                FAILED_SETUP ("IID_IMediaEventEx Failed");
            if (FAILED (hr = IGraphBuilder_QueryInterface(m->pGraph, &IID_IBasicAudio, (void **) &m->pAudio)))
                FAILED_SETUP ("IID_IBasicAudio Failed");
            if (FAILED (hr = IGraphBuilder_QueryInterface(m->pGraph, &IID_IMediaSeeking, (void **) &m->pSeek)))
                FAILED_SETUP ("IID_IMediaSeeking Failed");

            // Baker: This generates an exception but according to docs it looks right.
            if (FAILED (hr =IGraphBuilder_RenderFile(m->pGraph, W_absolute_filename, NULL))) // Baker _Render can point into a pak?
                FAILED_SETUP ("IGraphBuilder_RenderFile Failed");

            // send events through the standard window event handler
            if (gCore_Window)
            {
                if (FAILED (hr = IMediaEventEx_SetNotifyWindow(m->pEvent, (OAHWND) gCore_Window, WM_GRAPHNOTIFY, 0)))
                    FAILED_SETUP ("IMediaEventEx_SetNotifyWindow Failed");
            }

            // Run the graph.
            if (FAILED (hr = IMediaControl_Run(m->pControl) ))
                FAILED_SETUP ("IMediaControl_Run Failed");

            #undef FAILED_SETUP
        } while (0);

        // If we made it this far everything is fine.
        c_strlcpy (me->filename, path_to_file);
        me->playing = true;
        me->paused = false;
        me->looping = looping;

        // wait until it reports playing
        sWaitForFilterState (m->pControl, State_Running);

        // examples in the SDK will wait for the event to complete here, but this is totally inappropriate for a game engine.
        sRefreshVolume (me);

        loghushed (HUSHLEVEL_MP3, "%s: playing", _tag);
        return true;
    }



    // WM_GRAPHNOTIFY message handler.  Only interested in stop message, engine code handles everything else
    int WIN_MP3_Message (mobj_t *me)
    {
        MLOCAL(m);

        LONG		evCode;
        LONG_PTR	evParam1, evParam2;
        HRESULT     hr = S_OK;
        LONGLONG	pos = 0;

        // if (!me->playing) ShowPos (me);

        // Process all queued events
        while (me->playing && SUCCEEDED (IMediaEventEx_GetEvent (m->pEvent, &evCode, &evParam1, &evParam2, 0)))
        {
            // Free memory associated with callback, since we're not using it
            hr = IMediaEventEx_FreeEventParams(m->pEvent, evCode, evParam1, evParam2);

            if (evCode != EC_COMPLETE)
                continue;

            switch (me->looping)
            {
            case true:
                // If this is the end of the clip, reset to beginning (first frame)
                hr = IMediaSeeking_SetPositions(m->pSeek, &pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
                loghushed (HUSHLEVEL_MP3, "%s: looped", _tag);
                break;

            case false:
                // Not looping:  need to explicitly stop it otherwise interfaces will remain open when next mp3 is played
                Stop (me);
                break;
            }

        } // End of while loop

        return 0;
    }

    static cbool Initialize (mobj_t *me)
    {
        cbool result = false;
        HRESULT hr;

        hr = CoInitialize (NULL);

        if (!FAILED (hr))
        {
            logd ("CoInitialize initialized");
            result = true;
        } else log_fatal ("CoInitialize failed"); // Too strong?  mingw out of the box has this weakness still, right?

        CoUninitialize ();

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////
    //  PUBLIC GLOBAL FUNCTIONS:  Creation and freeing of object
    ///////////////////////////////////////////////////////////////////////////////


    // Shutdown and Free
    static void * Shutdown (mobj_t *me)
    {
        Stop (me);

        core_free (me->_local);
        core_free (me);

        return NULL;
    }


    // Short: Creates an instance of an mp3 object, which calls Init
    mobj_t *MP3_Instance (void)
    {
        cbool result;

        mobj_t *nobj = core_calloc (sizeof(mobj_t), 1);
        nobj->_local = core_calloc (sizeof(struct mlocal), 1);

        // Hook up functions
        nobj->Play			= Play;
        nobj->Update		= Update;
        nobj->Stop			= Stop;
        nobj->Pause			= Pause;
        nobj->Resume		= Resume;
        nobj->Update		= Update;

        nobj->AttachVolume  = AttachVolume;

        // Run initializer
        result = Initialize (nobj);

        if (!result)
        {
            nobj = Shutdown (nobj);
        }

        // Required information
        OBJ_REQUIRED_HOOKUP(nobj) // Sets parent, _tag, Shutdown

        return nobj;
    }

    #undef mobj_t
    #undef mlocal




#endif // _MSC_VER


#endif // PLATFORM_WINDOWS ONLY




