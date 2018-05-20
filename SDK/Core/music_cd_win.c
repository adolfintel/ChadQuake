/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2009-2014 Baker and others

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
// music_cd_win.c - cd player

#include "environment.h"
#ifdef PLATFORM_WINDOWS
#ifndef __GNUC__ // No MinGW at this time

#include "core.h"
#include "core_windows.h"

#pragma comment (lib, "winmm.lib")





///////////////////////////////////////////////////////////////////////////////
//  LOCAL SETUP
///////////////////////////////////////////////////////////////////////////////

#define MLOCAL(_m) struct mlocal *_m = (struct mlocal *) me->_local

// Begin ...

static const char * const _tag = "cd";  // TAG
#define mobj_t musicplayer_t			// object type
#define mlocal _cd_local				// private data struct

struct mlocal							// private data
{
	UINT	wDeviceID;
	cbool	cd_valid;
	byte	maxtrack;
};


///////////////////////////////////////////////////////////////////////////////
//  INTERNAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

static void Pause (mobj_t *me);
static void Resume (mobj_t *me);
static void sCDError (mobj_t *me, const char *fmt, ...);

#define FAILED_SETUP(_s) { logd ("%s: %s", _tag, _s); return false; }
static cbool sGetAudioDiskInfo (mobj_t *me)
{
	MLOCAL(m);
	MCI_STATUS_PARMS	mciStatusParms = {0,0, MCI_STATUS_READY}; // 3rd = dwItem
	DWORD				dwReturn;

	if ((dwReturn = mciSendCommand(m->wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms)))
		FAILED_SETUP ("drive ready test - get status failed");

	if (!mciStatusParms.dwReturn)
		FAILED_SETUP ("drive not ready");

	mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
	if ((dwReturn = mciSendCommand(m->wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms)))
		FAILED_SETUP ("get tracks - status failed");

	if (mciStatusParms.dwReturn < 1)
		FAILED_SETUP ("no music tracks");

	m->cd_valid = true;
	m->maxtrack = mciStatusParms.dwReturn;

	return true;
}
#undef FAILED_SETUP


// Tells the system the current volume level, do not check against previous volume as we might need to re-tell the system
// unconditionally on new playback, etc.
// Called when volume changes in a frame or on new playback
static void sRefreshVolume (mobj_t *me)
{
	float volumepct = me->getvolumepct ? *me->getvolumepct : 1;
	if (me->playing)
	{
		MLOCAL(m);
		cbool bvolume = !(volumepct <=0);
		switch (bvolume)
		{
		case 0:
			Pause (me);
			break;
		default:
			Resume (me);
			break;
		}
		me->volumepct = volumepct;
		logd ("%s: refresh volume", _tag);
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

		DWORD dwReturn;

		me->playing = false;
		me->paused = false;

		if ((dwReturn = mciSendCommand(m->wDeviceID, MCI_STOP, 0, (DWORD)NULL)))
			sCDError (me, "MCI_STOP failed (%d)", dwReturn);

		logd ("%s: stop", _tag);
	}
}


static void AttachVolume (mobj_t *me, float *pvolume)
{
	me->getvolumepct = pvolume;
}


#pragma message ("Make sure FMOD pause works, that was in here")
// CD volume is either on or off
// Intended to be called once per frame
// Effectively the main purpose is to update the volume, although it could do other things
static void Update (mobj_t *me)
{
	if (me->getvolumepct && *(me->getvolumepct) != me->volumepct)
	{
		sRefreshVolume (me);
		logd ("%s: volume is %g", _tag, me->volumepct);
	}
}





static void sCDError (mobj_t *me, const char *fmt, ...)
{
	VA_EXPAND (msg, SYSTEM_STRING_SIZE_1024, fmt);

	log_debug ("%s", msg);
	// More error handling?  Shutdown?
}

static void sSetPause (mobj_t *me, cbool setpause)
{
	if (me->paused != setpause)
	{
		MLOCAL(m);

		MCI_GENERIC_PARMS	mciGenericParms = { (DWORD)gCore_Window };
		MCI_PLAY_PARMS		mciPlayParms =
		{
			(DWORD)gCore_Window,						// dwCallback
			MCI_MAKE_TMSF (me->filename[0], 0, 0, 0),			// dwFrom
			MCI_MAKE_TMSF (me->filename[0] + 1, 0, 0, 0),		// dwTo
		};

		DWORD				dwReturn;

		// don't wait for the filter states here
		switch (setpause)
		{
		case true:
			dwReturn = mciSendCommand(m->wDeviceID, MCI_PAUSE, 0, (DWORD)(LPVOID) &mciGenericParms);

			if (!dwReturn /* success = 0 */) break;

			// Error
			sCDError (me, "MCI_PAUSE failed (%d)", dwReturn);
			return;

		case false:
			dwReturn = mciSendCommand(m->wDeviceID, MCI_PLAY, MCI_TO | MCI_NOTIFY, (DWORD)(LPVOID) &mciPlayParms);

			if (!dwReturn /* success = 0 */) break;

			// Error
			sCDError (me, "%s: MCI_PLAY failed (%d)", _tag, dwReturn);
			return;
		}

		me->paused = setpause;

		log_debug ("%s: %s", _tag, me->paused ? "paused" : "resumed");
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

	byte				tracknum = path_to_file ? path_to_file[0] : 0;
	DWORD				dwReturn;
    MCI_PLAY_PARMS		mciPlayParms;
	MCI_STATUS_PARMS	mciStatusParms;

	// Stop music
	Stop (me);

	if (!m->cd_valid)
	{
		// try one more time
		sGetAudioDiskInfo(me);

		// didn't work
		if (!m->cd_valid)
			return false;
	}

#pragma message ("I was always under the impression track 0 with first one?")
	if (tracknum < 1 || tracknum > m->maxtrack)
	{
		log_debug ("CDAudio: Bad track number %d.", (int)tracknum);
		return false;
	}

	// don't try to play a non-audio track
	mciStatusParms.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
	mciStatusParms.dwTrack = tracknum;
    dwReturn = mciSendCommand (m->wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);

	if (dwReturn)
	{
		log_debug ("MCI_STATUS failed (%d)", dwReturn);
		return false;
	}

	if (mciStatusParms.dwReturn != MCI_CDA_TRACK_AUDIO)
	{
		log_debug ("%s: track %d is not audio", _tag, tracknum);
//		return false;
	}

	// get the length of the track to be played
	mciStatusParms.dwItem = MCI_STATUS_LENGTH;
	mciStatusParms.dwTrack = tracknum;
    dwReturn = mciSendCommand (m->wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);

	if (dwReturn)
	{
		log_debug ("%s: MCI_STATUS failed (%d)", _tag, dwReturn);
		return false;
	}

//	if (gCore_Window)
	{
		mciPlayParms.dwCallback = gCore_Window ? (DWORD)gCore_Window : (DWORD)NULL;
		mciPlayParms.dwFrom = MCI_MAKE_TMSF (tracknum, 0, 0, 0);
		mciPlayParms.dwTo = (mciStatusParms.dwReturn << 8) | tracknum;

		dwReturn = mciSendCommand(m->wDeviceID, MCI_PLAY, MCI_NOTIFY | MCI_FROM | MCI_TO, (DWORD)(LPVOID) &mciPlayParms);

		if (dwReturn)
		{
			log_debug ("%s: MCI_PLAY failed (%d)", _tag, dwReturn);
			return false;
		}
	}

	// If we made it this far everything is fine.
	c_strlcpy (me->filename, path_to_file);
	me->playing = true;
	me->paused = false;
	me->looping = looping;
	sRefreshVolume (me);

	log_debug ("%s: playing", _tag);
	return true;
}


// MM_MCINOTIFY message handler.
int WIN_CD_Message (mobj_t *me, WPARAM wParam, LPARAM lParam)
{
	MLOCAL(m);

	if (lParam != (int)m->wDeviceID)
		return 1;

	switch (wParam)
	{
	case MCI_NOTIFY_SUCCESSFUL:

		if (!me->playing) break;

		me->playing = me->paused = false;
		if (me->looping)
		{
			 Play (me, me->filename, true);
			log_debug ("%s: looped", _tag);
		}
		break;

	case MCI_NOTIFY_ABORTED:
	case MCI_NOTIFY_SUPERSEDED:
		break;

	case MCI_NOTIFY_FAILURE:
		log_debug ("MCI_NOTIFY_FAILURE");
		Stop (me);
		m->cd_valid = false;
		break;

	default:
		log_debug ("Unexpected MM_MCINOTIFY type (%d)", wParam);
		return 1;
	}
	return 0;
}


#pragma message ("What all needs to be setup for regular Con_Printf and not Safeprintf")
static cbool Initialize (mobj_t *me)
{
	MLOCAL(m);

	DWORD			dwReturn;
	MCI_OPEN_PARMS	mciOpenParms = { 0,0, "cdaudio"};
    MCI_SET_PARMS	mciSetParms = {0, MCI_FORMAT_TMSF }; // Set the time format to track/minute/second/frame (TMSF).

	if ((dwReturn = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE, (DWORD) (LPVOID) &mciOpenParms)))
	{
		logd ("CDAudio_Init: MCI_OPEN failed (%d)", dwReturn);
		return false;
	}

	// Set the device id
	m->wDeviceID = mciOpenParms.wDeviceID;

    if ((dwReturn = mciSendCommand(m->wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)(LPVOID) &mciSetParms)))
    {
		logd ("MCI_SET_TIME_FORMAT failed (%d)", dwReturn);
        mciSendCommand(m->wDeviceID, MCI_CLOSE, 0, (DWORD)NULL);
		return false;
    }

	if (sGetAudioDiskInfo(me))
		log_debug ("%s: init - No CD in player.", _tag);

	log_debug ("CD Audio Initialized");
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//  PUBLIC GLOBAL FUNCTIONS:  Creation and freeing of object
///////////////////////////////////////////////////////////////////////////////


// Shutdown and Free
static void *Shutdown (mobj_t *me)
{
	MLOCAL(m);

	Stop (me);

	if (mciSendCommand(m->wDeviceID, MCI_CLOSE, MCI_WAIT, (DWORD)NULL))
		log_debug ("%s: MCI_CLOSE failed", _tag);

	core_free (me->_local);
	core_free (me);

	return NULL;
}


// Short: Creates an instance of a cd object, which calls Init
mobj_t *CD_Instance (void)
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

#endif // !__GNUC__ // No MinGW at this time
#endif // PLATFORM_WINDOWS



