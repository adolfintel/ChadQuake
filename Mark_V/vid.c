/*
Copyright (C) 2009-2014 Baker

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
// vid.c -- common video


#include "quakedef.h"

viddef_t	vid;				// global video state

#pragma message ("Are we reseting hardware gamma on start?  Verify  We shoud")

#ifdef SUPPORTS_RESIZABLE_WINDOW
// Baker: resize on the fly
static void VID_BeginRendering_Resize_Think (void); // Internal to vid.c
#endif // SUPPORTS_RESIZABLE_WINDOW


//
// set mode and restart
//

/*
================
VID_SetMode
================
*/

static void VID_Activate (cbool active);
int VID_SetMode (int modenum)
{
// so Con_Printfs don't mess us up by forcing vid and snd updates
    int temp = scr_disabled_for_loading;
    cbool re_setupgl;

    scr_disabled_for_loading = true;  // temped
    vid.canalttab = false;

    VID_Activate (0); // Deactivate, stops sounds, releases key

    // Platform specific stuff
    re_setupgl = (VID_Local_SetMode (modenum) == false);

    // Assignment
    {
        vid.modenum_screen = modenum;
        vid.screen = vid.modelist[vid.modenum_screen];

        // keep cvars in line with actual mode
        VID_Cvars_Sync_To_Mode (&vid.modelist[vid.modenum_screen]);

        // Refresh console
#ifdef GLQUAKE_RENDERER_SUPPORT
        vid.consize_stale = true;
        vid.numpages = 2;
#else // WinQuake ..

#pragma message ("Baker: Blitting in winquake will break these calcs, should be clwidth")
        // Moved conwidth, conheight, aspect to CreateDIB in WinQuake
        // Mac must mirror or adapt it
        VID_Local_SetPalette (vid.curpal);
        vid.numpages = 1;
#endif // !GLQUAKE_RENDERER_SUPPORT

        Sbar_Changed ();
        vid.recalc_refdef = 1;
    }

#ifdef CORE_GL //  GLQUAKE_RENDERER_SUPPORT
    // GL break-in
    GL_VID_SetMode_GL_Evaluate_Renderer ();
    GL_VID_SetMode_GL_SetupState ();
#endif // CORE_GL

#ifdef GLQUAKE_RENDERER_SUPPORT // SDL guarantees context reuse?  Hmmm.  I thought it might?
#if 1
    if (re_setupgl)
    {
        if (host_initialized)
            Con_DPrintLinef ("VID_SetMode: Couldn't reuse context.  Reuploading images ...");

        TexMgr_ReloadImages (1); // SAFE?
        if (host_initialized) Con_DPrintLinef (" completed.");

    }
    else Con_DPrintLinef ("Reused context no reupload images");
#endif // GLQUAKE_RENDERER_SUPPORT

    // Resolution dependent settings need refreshed
    vid.warp_stale = true;
    vid.consize_stale = true;
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef CORE_GL
    // ensure swap settings right
    VID_Local_Vsync ();
#endif // CORE_GL

#ifdef GLQUAKE_HARDWARE_GAMMA
    VID_Gamma_Think ();
    VID_Gamma_Clock_Set (); // Baker: This sets a clock to restore the gamma
#endif // GLQUAKE_HARDWARE_GAMMA




#ifdef WINQUAKE_VSYNC_SUPPORT
    VID_Local_Vsync ();
#endif // WINQUAKE_VSYNC_SUPPORT

    // Restore sound

    VID_Activate (1);  // Activate, restores sound

    vid.canalttab = true;
    scr_disabled_for_loading = temp;

// MH knocked this out!
//#ifdef DIRECT3D9_WRAPPER
//    // ensure swap settings right
//    if (vid.screen.type == MODE_WINDOWED && vid_vsync.value)
//    {
//        Cbuf_AddTextLine ("vid_vsync 0; wait; vid_vsync 1 \\ Shouldn't need, but running vsync right now doesn't keep"); // Works!
//    }
//#endif // DIRECT3D9_WRAPPER

    return true;
}

cbool VID_Restart (int flags /* favorite vs. temp*/)
{

    vmode_t		newmode = VID_Cvars_To_Mode ();
    vmode_t		oldmode = vid.screen;
    int			newmodenum;


    // No change scenario
    if ( memcmp (&newmode, &oldmode, sizeof(vmode_t)) == 0
#ifdef WINQUAKE_RENDERER_SUPPORT
            && vid.stretch_old_cvar_val == (int)vid_sw_stretch.value
#endif //WINQUAKE_RENDERER_SUPPORT
       ) // End paren
    {
        Con_SafePrintLinef ("Video mode request is same as current mode.");
        return false;
    }


    // Fullscreen must check existing modes, window must set it instead.
    switch (newmode.type)
    {
    case MODE_WINDOWED:
        memcpy (&vid.modelist[MODE_WINDOWED], &newmode, sizeof (vmode_t) );
        newmodenum = 0;
        break;

    case MODE_FULLSCREEN:
//#if /*defined(PLATFORM_LINUX) &&*/ defined(CORE_SDL) && !defined(GLQUAKE) && defined (CORE_GL) // WinQuake GL
#ifdef CORE_SDL
        newmode = vid.desktop;

#endif // CORE_SDL
        if (VID_Mode_Exists (&newmode, &newmodenum) == false)
        {
            Con_SafePrintLinef ("%d x %d (%d) is not a valid fullscreen mode",
                                (int)vid_width.value,
                                (int)vid_height.value,
                                (int)vid_fullscreen.value);
            return false;
        }
        break;
    }

    // Determine if the mode is invalid.

    VID_SetMode (newmodenum);

    if (flags == USER_SETTING_FAVORITE_MODE)
        vid.modenum_user_selected = vid.modenum_screen;

    return true;
}

/*
================
VID_Test -- johnfitz -- like vid_restart, but asks for confirmation after switching modes
================
*/
void VID_Test (lparse_t *unused)
{
    vmode_t		newmode = VID_Cvars_To_Mode ();
    vmode_t		oldmode = vid.screen;
    cbool	mode_changed = memcmp (&newmode, &vid.screen, sizeof(vmode_t) );

    if (!mode_changed)
        return;
//
// now try the switch
//
    VID_Restart (USER_SETTING_FAVORITE_MODE);

    //pop up confirmation dialog
    if (!SCR_ModalMessage("Would you like to keep this" NEWLINE "video mode? (y/n)", 5, false))
    {
        // User rejected new mode: revert cvars and mode
        VID_Cvars_Sync_To_Mode (&oldmode);
        VID_Restart (USER_SETTING_FAVORITE_MODE);
    }
}

void VID_Alt_Enter_f (void)
{
#ifdef PLATFORM_LINUX
    static double last_time;
    double new_time = System_DoubleTime ();

    if (last_time && new_time  < last_time + 0.75)
    {
        // Linux the key is repeating too much too fast?
        return;
    }
    last_time = new_time;
#endif // PLATFORM_LINUX

    if (vid.modenum_screen != vid.modenum_user_selected)
    {
        // Go to favorite mode
        VID_Cvars_Sync_To_Mode ( &vid.modelist[vid.modenum_user_selected] );
        VID_Restart (USER_SETTING_FAVORITE_MODE);
        return;
    }

// ALT-ENTER to a temp mode
    if (vid.screen.type == MODE_WINDOWED)
    {
#if defined(CORE_SDL) && defined(GLQUAKE) && defined(PLATFORM_LINUX)
        int old_sel = vid.modenum_user_selected;
        Cvar_SetValueQuick (&vid_width, vid.desktop.width);
        Cvar_SetValueQuick (&vid_height, vid.desktop.height);
        Cvar_SetValueQuick (&vid_fullscreen, 1);
        Cbuf_AddTextLine ("vid_restart");
        Cbuf_Execute ();
        vid.modenum_user_selected = old_sel;
        return; // Fuck it
#else
        VID_Cvars_Set_Autoselect_Temp_Fullscreen_Mode (vid.modenum_screen);
#endif
    }
    else  VID_Cvars_Set_Autoselect_Temp_Windowed_Mode (vid.modenum_screen);

    VID_Restart (ALT_ENTER_TEMPMODE);
}


void VID_Restart_f (void)
{
    VID_Restart (USER_SETTING_FAVORITE_MODE);
}

//
// in-game
//

#ifdef SUPPORTS_RESIZABLE_WINDOW
// Baker: Optional resize on-the-fly start ...
void VID_Resize_Check (int resize_level) // 1 = dynamic resize, 2 = setmode
{
    // Perform resize check
    vid.resized = resize_level; // 1 or 2 - Causes VID_BeginRendering to call Resize_Act
    vid.recalc_refdef = 1;
}

// Called by VID_BeginRendering
static void VID_BeginRendering_Resize_Think (void)
{
    if (vid.direct3d == 9 /* MH is want this every frame as a test*/ || vid.resized || vid.client_window.width == 0)
	//if (/*vid.direct3d == 9 /* MH is want this every frame as a test ||*/ vid.resized || vid.client_window.width == 0)
    {
        VID_BeginRendering_Resize_Think_Resize_Act ();

        // Future: Let's center the screen?
        vid.resized = false;
    }
}
// Baker: Optional resize on-the-fly-finished

#endif // SUPPORTS_RESIZABLE_WINDOW


static void VID_Activate (cbool active)
{

    if (active)
    {
        if (vid.sound_suspended)
        {
            S_UnblockSound ();
            CDAudio_Resume ();
#ifdef SUPPORTS_NEHAHRA
            FMOD_Resume ();
#endif // SUPPORTS_NEHAHRA
            vid.sound_suspended = false;
            logd ("VID_Activate Sound Unsuspended");
        }
#pragma message ("Baker: This needs to return to control the mouse a little")
        Input_Think ();
    }

    if (!active)
    {
        if (!vid.sound_suspended)
        {
            S_BlockSound ();
#if defined(PLATFORM_OSX) || defined(CORE_SDL)
            S_StopAllSounds(true); // On a Mac, S_BlockSound alone doesn't suffice.  Same with SDL.
#endif // PLATFORM_OSX
            CDAudio_Pause ();
#ifdef SUPPORTS_NEHAHRA
            FMOD_Pause ();
#endif // SUPPORTS_NEHAHRA
            vid.sound_suspended = true;
            logd ("VID_Activate Sound Suspended");
        }
        Input_Local_Deactivate (); // Stops drag flag
        Key_Release_Keys (NULL); // We might not get all key messages during this process.

    }
}


void VID_AppActivate(cbool fActive, cbool minimize, cbool hide)
{
	//if (minimize != vid.Minimized) {
	//	logd ("Minimized set to %d", minimize);
	//}

    vid.ActiveApp = fActive;
    vid.Minimized = minimize;
	
    vid.Hidden = hide;

#if 0
    Con_PrintLinef ("App activate occurred %d", vid.ActiveApp);
#endif

    if (vid.ActiveApp)
    {
        VID_Activate (1); // Baker: This activates sound, gives input a think

        if (vid.screen.type == MODE_FULLSCREEN && vid.canalttab && vid.wassuspended)
        {
            VID_Local_Suspend (false);
            vid.wassuspended = false;

            if (vid.direct3d) // I can't recall why we need special treatment for Direct3D here ... but not messing with it now.
                Sbar_Changed ();
        }
    }

    if (!vid.ActiveApp)
    {
        if (vid.screen.type == MODE_FULLSCREEN && vid.canalttab)
        {
            VID_Local_Suspend (true);
            vid.wassuspended = true;
        }

        VID_Activate (0); // Baker: This deactivates sound, releases keys
    }
}

void VID_SwapBuffers (void)
{
    if (scr_skipupdate)
        return;

#ifdef CORE_GL // Both GLQUAKE and WinQuake through software need this
    VID_Local_SwapBuffers ();
#endif // CORE_GL

#ifdef WINQUAKE_RENDERER_SUPPORT
    {
        // update one of three areas
        vrect_t		vrect;

        if (winquake_scr_copyeverything)
        {
            // Copies entire screen
            vrect.x = 0;
            vrect.y = 0;
            vrect.width = clwidth;
            vrect.height = clheight;
            vrect.pnext = 0;

            VID_Update (&vrect);
        }
        else if (winquake_scr_copytop)
        {
            // Copies entire screen except sbar
            vrect.x = 0;
            vrect.y = 0;
            vrect.width = clwidth;
            vrect.height = clheight - sb_lines;
            vrect.pnext = 0;

            VID_Update (&vrect);
        }
        else
        {
            // Copies only view area
            vrect.x = scr_vrect.x;
            vrect.y = scr_vrect.y;
            vrect.width = scr_vrect.width;
            vrect.height = scr_vrect.height;
            vrect.pnext = 0;

            VID_Update (&vrect);
        }
    }
#endif // WINQUAKE_RENDERER_SUPPORT
}


//
// functions to match modes to cvars or reverse
//

void VID_Cvars_Sync_To_Mode (vmode_t* mymode)
{
    // Don't allow anything exiting to call this.  I think we are "ok"
    Cvar_SetValueQuick (&vid_width, (float)mymode->width);
    Cvar_SetValueQuick (&vid_height, (float)mymode->height);
    Cvar_SetValueQuick (&vid_fullscreen, mymode->type == MODE_FULLSCREEN ? 1 : 0);
}

vmode_t VID_Cvars_To_Mode (void)
{
    vmode_t retmode;

    retmode.type	= vid_fullscreen.value ? MODE_FULLSCREEN : MODE_WINDOWED;
    retmode.width	= (int)vid_width.value;
    retmode.height	= (int)vid_height.value;
    retmode.bpp		= vid.desktop.bpp;

    if (retmode.type == MODE_WINDOWED)
    {
        retmode.width	= CLAMP (MIN_WINDOWED_MODE_WIDTH, retmode.width, vid.desktop.width);
        retmode.height	= CLAMP (MIN_WINDOWED_MODE_HEIGHT, retmode.height, vid.desktop.height);
        retmode.bpp		= vid.desktop.bpp;
    }

    return retmode;
}



cbool VID_Read_Early_Non_Mode_Cvars (void)
{
// Main difference is that we don't have to bail like we do with video mode.
// These must all be read.

    const cvar_t* cvarslist[] =
    {
#ifdef GLQUAKE_SUPPORTS_VSYNC
        &vid_vsync, // Baker: Important to read here for Direct3D
#endif // GLQUAKE_SUPPORTS_VSYNC
        &vid_gamma,
        &vid_contrast,
        &in_system_enhanced_keys,
#ifdef GLQUAKE_RENDERER_SUPPORT
        &vid_hardwaregamma,
#endif // GLQUAKE_RENDERER_SUPPORT
#ifdef WINQUAKE_RENDERER_SUPPORT
        &vid_sw_stretch,
#endif // WINQUAKE_RENDERER_SUPPORT
        &hd_folder,
        NULL
    };
    cbool found_in_config, found_in_autoexec;

    found_in_config = Read_Early_Cvars_For_File (CONFIG_CFG, cvarslist);
    found_in_autoexec = Read_Early_Cvars_For_File (AUTOEXEC_CFG, cvarslist);

    return (found_in_config || found_in_autoexec);
}


cbool VID_Read_Early_Cvars (void)
{

    // Any of these found and we bail
    char *video_override_commandline_params[] = {"-window", "-width", "-height", "-current", NULL } ;
    const cvar_t *cvarslist[] =
    {
        &vid_fullscreen,
        &vid_width,
        &vid_height,
#ifdef GLQUAKE_RENDERER_SUPPORT
        &vid_multisample,  // Baker: multisample support
#endif // GLQUAKE_RENDERER_SUPPORT
        NULL
    };
    cbool found_in_config, found_in_autoexec;
    int i;

    for (i = 0; video_override_commandline_params[i]; i++)
        if (COM_CheckParm (video_override_commandline_params[i]))
        {
#ifdef GLQUAKE_RENDERER_SUPPORT
            // Baker: We have to read multisample early
            found_in_config = Read_Early_Cvars_For_File (CONFIG_CFG, &cvarslist[3]);
            found_in_autoexec = Read_Early_Cvars_For_File (AUTOEXEC_CFG, &cvarslist[3]);
#endif // GLQUAKE_RENDERER_SUPPORT
            return false;
        }

    found_in_config = Read_Early_Cvars_For_File (CONFIG_CFG, cvarslist);
    found_in_autoexec = Read_Early_Cvars_For_File (AUTOEXEC_CFG, cvarslist);

    return (found_in_config || found_in_autoexec);
}


cbool VID_Mode_Exists (vmode_t* test, int *outmodenum)
{
    int i;

    for (i = 0; i < vid.nummodes; i++)
    {
//		if (memcmp (&vid.modelist[i], test, sizeof(vmode_t)))
        if (test->width != vid.modelist[i].width)
            continue; // No match

        if (test->height != vid.modelist[i].height)
            continue; // No match

        if (test->type != vid.modelist[i].type)
            continue; // No match

        // dup
        if (outmodenum) *outmodenum = i;
        return true; // Duplicate
    }
    return false;
}

void VID_MakeMode (modestate_t mode_type, vmode_t *new_mode)
{
    new_mode->type	= mode_type;
    new_mode->width	= 640;
    new_mode->height= 480;
    new_mode->bpp	= vid.desktop.bpp;

    // !!(int)vid_fullscreen.value --> turn into an int and "NOT" it twice
    // so must have 0 or 1 value.  In case vid_fullscreen is 2 or something weird
    if ( (!!(int)vid_fullscreen.value) == mode_type)
    {
        new_mode->width = (int)vid_width.value;
        new_mode->height= (int)vid_height.value;
    }
    vid.nummodes ++;
}

void VID_Cvars_Set_Autoselect_Temp_Windowed_Mode (int favoritemode)
{
    // Pencil in last windowed mode, but set the bpp to the desktop bpp
    VID_Cvars_Sync_To_Mode (&vid.modelist[MODE_WINDOWED]);
}

int VID_Find_Best_Fullscreen_Modenum (int request_width, int request_height)
{
    int best = -1;
    int	bestscore = -1;
    int i;

    // Look through the video modes.

    // If an exact matching fullscreen mode of same resolution
    // exists, pick that.  Try to go for same bpp.

    // Attempt 1: Match resolution

    // Baker: Locate matching mode
    for (i = 1; i < vid.nummodes; i ++)
    {
        vmode_t *mode = &vid.modelist[i];
        int size_match		= (mode->width == request_width && mode->height == request_height);

        int score = size_match * 20;

        if (score <= bestscore)
            continue;  // Didn't beat best

        // New best
        best = i;
        bestscore = score;
    }

    if (bestscore < 20)
    {
        // No size match ... try again
        // If fails, pick something with width/height both divisble by 8
        // so charset doesn't look stretched.  Go for largest mode with
        // desktop bpp and desktop refreshrate and desktop width/height
        // Unless those are stupid.

        best = -1;
        bestscore = -1;

        for (i = 1; i < vid.nummodes; i ++)
        {
            vmode_t *mode = &vid.modelist[i];

#if 0 // Baker: There are now laptops with 1440 x 900 (not divisible by 8) and 1366 x 768 (1366 not divisible by 8) resolutions.
            if (mode->width & 7)
                continue; // Skip stupid resolutions
            if (mode->height & 7)
                continue; // Skip stupid resolutions
#endif

            if (mode->width >= vid.desktop.width - 7)
                if (mode->height >= vid.desktop.height - 7)
                {
                    // Take it
                    best = i;
                    break;
                }

            // Not an automatic winner.  If largest ...
            if (mode->width >= vid.modelist[best].width || mode->height >= vid.modelist[best].height)
            {
                best = i;
            }
        }

        if (best == -1)
            System_Error ("Couldn't find suitable video mode");

    }

    return best;
}

int VID_Cvars_To_Best_Fullscreen_Modenum (void)
{
    vmode_t preference = VID_Cvars_To_Mode ();
    return VID_Find_Best_Fullscreen_Modenum (preference.width, preference.height);
}


void VID_Cvars_Set_Autoselect_Temp_Fullscreen_Mode (int favoritemode)
{
    vmode_t *fave = &vid.modelist[favoritemode];
    int best = VID_Find_Best_Fullscreen_Modenum (fave->width, fave->height);

    // Set cvars
    VID_Cvars_Sync_To_Mode (&vid.modelist[best]);
}


//
// startup / shutdown
//

void	VID_Init (void)
{
    int		i;

    cbool videos_cvars_read;

    // We need to set this super early.
    if (COM_CheckParm ("-nomouse"))
        vid.nomouse = true;

    VID_Local_Init ();

    vid.desktop = VID_Local_GetDesktopProperties (); // Good time to get them.


    Cmd_AddCommands (VID_Init);




    // Now, if we have -window or anything we don't bother to read the cvars early
    // But we will still read them later.

    VID_Read_Early_Non_Mode_Cvars ();
    videos_cvars_read = VID_Read_Early_Cvars ();

    // The horror!!!
    com_hdfolder_count = 0;
    if (!isDedicated && hd_folder.string[0] && HD_Folder_Ok (/* editing it maybe !*/(char *) hd_folder.string))
    {

        char		this_qpath[MAX_QPATH_64];
        const char	*cursor = hd_folder.string;

        while (  (cursor = String_Get_Word (cursor, ",", this_qpath, sizeof(this_qpath)))  )
        {
            char folder_url[MAX_OSPATH];
            // Construct the full url
            FS_FullPath_From_Basedir (folder_url, this_qpath);
            if (File_Exists (folder_url) && File_Is_Folder(folder_url))
            {
                // It's going to put screenshots and configs and demos in there.
                // Needs to be a read only folder, though.  I think we are fine.  We write to com_gamedir.

                com_modified = true;
                COM_AddGameDirectory (this_qpath, true /*hd only*/);
            }
            // No verbose print here.  Too early to print.
            // else Con_VerbosePrintLinef ("Folder " QUOTED_S " does not exist.", folder_url);
        }
    }



    vid.system_enhanced_keys = !!in_system_enhanced_keys.value;

    if ((i = COM_CheckParm("-width")) && i + 1 < com_argc)
        Cvar_SetValueQuick (&vid_width, (float)atoi(com_argv[i+1]) );

    if ((i = COM_CheckParm("-height")) && i + 1 < com_argc)
        Cvar_SetValueQuick (&vid_height, (float)atoi(com_argv[i+1]) );

    if (COM_CheckParm("-current"))
    {
        VID_Cvars_Sync_To_Mode (&vid.desktop); // Use desktop sizes.
    }

    if (COM_CheckParm("-window"))
        Cvar_SetValueQuick (&vid_fullscreen, 0);

    VID_Local_Window_PreSetup (); // Window: Registers frame class, Mac: Gets display

// Add the windowed mode
    VID_MakeMode (MODE_WINDOWED, &vid.modelist[MODE_WINDOWED]);

// Add the fullscreen modes

    VID_Local_AddFullscreenModes ();

//    alert ("Modes done");

    vid.initialized = true;

// Now we set the video mode
//#if defined(CORE_SDL) && !defined(GLQUAKE) && defined(CORE_GL)
#ifdef CORE_SDL
    if (vid_fullscreen.value)
    {
        // Fucking crazy.
        Cvar_SetValueQuick (&vid_width, vid.desktop.width);
        Cvar_SetValueQuick (&vid_height, vid.desktop.height);
        //VID_SetMode (vid_fullscreen.value ? VID_Cvars_To_Best_Fullscreen_Modenum() : MODE_WINDOWED);
    }
#endif // #CORE_SDL

    VID_SetMode (vid_fullscreen.value ? VID_Cvars_To_Best_Fullscreen_Modenum() : MODE_WINDOWED);

    clwidth = vid.screen.width; // vid_width.value // Err?  Why vid.screen.width and vid.screen.height?
    clheight = vid.screen.height;

    vid.modenum_user_selected = vid.modenum_screen; // Default choice
    VID_Cvars_Sync_To_Mode (&vid.modelist[vid.modenum_user_selected]);

#ifdef GLQUAKE_HARDWARE_GAMMA
    VID_Gamma_Init ();
#endif // GLQUAKE_HARDWARE_GAMMA

    VID_Menu_Init(); //johnfitz
}


void VID_Shutdown (void)
{
    if (!vid.initialized)
        return;

    VID_Local_Shutdown ();

    vid.canalttab = false;
    vid.initialized = false;
}

#ifdef GLQUAKE_TEXTUREGAMMA_SUPPORT
static float vid_texgamma_user_req = 0; // Set back to 0 if changed.  Solely belongs to vid
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef GLQUAKE_HARDWARE_GAMMA
static void VID_Gamma_Table_Make (float gamma, float contrast, unsigned short* ramps)
{
    int	i, c;
    for (i = 0 ; i < 256 ; i++)		// For each color intensity 0-255
    {
        // apply contrast; limit contrast effect to 255 since this is a byte percent
        c = i * contrast;
        if (c > 255)	c = 255;

        // apply gamma
        c = 255 * pow((c + 0.5)/255.5, gamma) + 0.5;
        c = CLAMP (0, c, 255);

        ramps[i + 0] = ramps[i + 256] = ramps[i + 512] = c << 8;
    }
#pragma message ("Baker: Refresh gamma local table here?")
}


cbool gamma_available = false;
unsigned short	systemgammaramp[768]; // restore gamma on exit and/or alt-tab

void VID_Gamma_Shutdown (void)
{
    if (vid.ever_set_gamma)
        VID_Local_Gamma_Reset ();

}

void VID_Gamma_Init (void)
{
    gamma_available = VID_Local_IsGammaAvailable (systemgammaramp);
    if (!gamma_available)
    {
        Con_WarningLinef ("Hardware gamma not available (GetDeviceGammaRamp failed)");
        return;
    }


    if (vid_hardwaregamma.value)
    {
        VID_Local_Gamma_Reset (); // Sets to system user specified value.
        Con_SafePrintLinef ("Hardware gamma enabled");
    }
    else
    {
        Con_SafePrintLinef ("Hardware gamma available");
    }


//	Cvar_RegisterVariable (&vid_hardware_contrast);


}



static double hardware_active_enforcer_time_clock = 0;
static int hardware_enforcer_count = 0;



//#pragma message ("We need to make it so a think here can turn it off if !vid_hardware.gamma && hardware_is_active or vid_hardware,gamma && !hardware_is_active")
// Effectively, we will hit the gamma for 4 seconds after a video mode change
#define GAMMA_CLOCK_COUNT_48		(12 * 4) // Hit it hard and heavy for a few seconds after a video mode change
#define GAMMA_CLOCK_INTERVAL_0_125	(0.125)	 // To keep stupid drivers from rolling it back.

void VID_Gamma_Clock_Set (void)
{
    Con_DPrintLinef ("Gamma protector set");
    hardware_enforcer_count = GAMMA_CLOCK_COUNT_48;	// We really only need to do this after a mode change, specifically a windowed to fullscreen switch.
    hardware_active_enforcer_time_clock = realtime + GAMMA_CLOCK_INTERVAL_0_125;
}
#endif // GLQUAKE_HARDWARE_GAMMA

#ifdef GLQUAKE_RENDERER_SUPPORT

void VID_Gamma_Contrast_Clamp_Cvars (void)
{
    // bound cvars to menu range
    if (vid_contrast.value < VID_MIN_CONTRAST) Cvar_SetValueQuick (&vid_contrast, VID_MIN_CONTRAST);
    if (vid_contrast.value > VID_MAX_CONTRAST) Cvar_SetValueQuick (&vid_contrast, VID_MAX_CONTRAST);
    if (vid_gamma.value < VID_MIN_POSSIBLE_GAMMA) Cvar_SetValueQuick (&vid_gamma, VID_MIN_POSSIBLE_GAMMA);
    if (vid_gamma.value > VID_MAX_POSSIBLE_GAMMA) Cvar_SetValueQuick (&vid_gamma, VID_MAX_POSSIBLE_GAMMA);


}

void VID_Gamma_Think (void)
{
    static cbool	hardware_is_active = false;
    static int		vid_hardwaregamma_old = -1;
    cbool clock_fire = false;
    static unsigned short ramps[768]; // restore gamma on exit and/or alt-tab
    static	float	previous_gamma, previous_contrast;

#if 1
    VID_Gamma_Contrast_Clamp_Cvars ();
#else
    // bound cvars to menu range
    if (vid_contrast.value < VID_MIN_CONTRAST) Cvar_SetValueQuick (&vid_contrast, VID_MIN_CONTRAST);
    if (vid_contrast.value > VID_MAX_CONTRAST) Cvar_SetValueQuick (&vid_contrast, VID_MAX_CONTRAST);
    if (vid_gamma.value < VID_MIN_POSSIBLE_GAMMA) Cvar_SetValueQuick (&vid_gamma, VID_MIN_POSSIBLE_GAMMA);
    if (vid_gamma.value > VID_MAX_POSSIBLE_GAMMA) Cvar_SetValueQuick (&vid_gamma, VID_MAX_POSSIBLE_GAMMA);
#endif

    {
#ifdef GLQUAKE_HARDWARE_GAMMA
        cbool hardware_should_be_active = gamma_available && vid.ActiveApp && !vid.Minimized && !vid.Hidden && vid_hardwaregamma.value; // && !shutdown;
#else // Not ...
        cbool hardware_should_be_active = false;
#endif
        cbool hardware_change = (hardware_is_active != hardware_should_be_active);

        // Because quake.rc is going to be read and it will falsely default away from user preference
        cbool ignore_cvar_change = (host_initialized == true  && host_post_initialized == false);

        // First update the gamma table
        cbool table_change = (vid_gamma.value != previous_gamma || vid_contrast.value  != previous_contrast) && !ignore_cvar_change;
        cbool actionable_change = hardware_change || (table_change && hardware_should_be_active);

        // Baker: We will set the gamma IF
        // 1) There is a hardware change
        // 2) There is a color change *AND* hardware is active
        // 3) If hardware is active and no color change but timer fires.

#ifdef GLQUAKE_TEXTUREGAMMA_SUPPORT
        if (vid_hardwaregamma_old != -1 && vid_hardwaregamma_old != !!vid_hardwaregamma.value)
        {
            if (vid_hardwaregamma.value)
            {
                // Hardware gamma is becoming active.
                TexMgr_Gamma_Execute (1.0);
            }
            else
            {
                // Texture gamma is becoming active
                TexMgr_Gamma_Execute (vid_gamma.value);
            }
        }
        else if (vid_hardwaregamma_old == 0 && vid_texgamma_user_req)
        {
            // We were "texture gamma" last frame.  And the user has type "txgamma something" in the console.
            Cvar_SetValueQuick (&vid_gamma, vid_texgamma_user_req); // Match up the gamma cvar
            TexMgr_Gamma_Execute (vid_texgamma_user_req);
            vid_texgamma_user_req = 0;
        }
#endif // GLQUAKE_TEXTUREGAMMA_SUPPORT

        vid_hardwaregamma_old = !!vid_hardwaregamma.value; // We use this for the texture based control.

#ifdef GLQUAKE_TEXTUREGAMMA_SUPPORT
        //if (vid_hardwaregamma_old != -1 && vid_hardwaregamma_old &&  vid_texgamma_user_req) {
        //	TexMgr_Gamma_Execute (vid_texgamma_user_req);
        //	Cvar_SetValueQuick (&vid_gamma, vid_texgamma_user_req);
        //	vid_texgamma_user_req = 0; // Unconditionally should have been handled
        //}

        //if (hardware_change && hardware_should_be_active) {
        //	TexMgr_Gamma_Execute (1.0); // Turn off texture gamma by setting to 1
        //}

        //if (hardware_change && !hardware_should_be_active) {
        //	TexMgr_Gamma_Execute (vid_gamma.value); // Turn on texture gamma by setting it.
        //}

        if (vid_texgamma_user_req)
            vid_texgamma_user_req = 0; // breakpoint me.  Should be hard to happen.
#endif // GLQUAKE_TEXTUREGAMMA_SUPPORT

#ifdef GLQUAKE_HARDWARE_GAMMA
        if (!hardware_change && hardware_should_be_active && hardware_active_enforcer_time_clock)
        {
            if (realtime > hardware_active_enforcer_time_clock) // !host_post_initialized hit heavy if we aren't in main loop yet.
            {
                clock_fire = true;
                hardware_enforcer_count --;
                if (hardware_enforcer_count)
                    hardware_active_enforcer_time_clock = realtime + GAMMA_CLOCK_INTERVAL_0_125;
                else hardware_active_enforcer_time_clock = 0; // Disable
                logd ("Gamma protector clock fired  ... ");
            }
            //if (!host_initialized) {
            //	logd ("Gamma fire because not in main loop yet and clock not active");
            //	clock_fire = true; // No clock because not in main loop.  Fire away!
            //}
        }
#endif
        if (hardware_change)
            Con_DPrintLinef ("Hardware change detected ... ");

        if (!actionable_change && !clock_fire)
            return;

        if (!hardware_change && table_change)
            Con_DPrintLinef ("Color change with hardware active ... ");

#ifdef GLQUAKE_HARDWARE_GAMMA
        // If we hit here, a change occurred.
        switch (hardware_should_be_active)
        {
        case true:
        {
            previous_gamma = vid_gamma.value;
            previous_contrast = vid_contrast.value;

            if (clock_fire) // Nudge table a little
            {
                static unsigned short tempramps[768]; // restore gamma on exit and/or alt-tab
                VID_Gamma_Table_Make (vid_gamma.value + 0.01, vid_contrast.value - 0.01, tempramps);
                VID_Local_Gamma_Set (tempramps); // Table nudge
            }
            VID_Gamma_Table_Make (vid_gamma.value, vid_contrast.value, ramps);
            Image_Build_Gamma_Table (vid_gamma.value, vid_contrast.value, vid.gammatable);
            VID_Local_Gamma_Set (ramps);
        }
        logd ("Custom Gamma set: contrast = %g", vid_contrast.value);
        hardware_is_active = true;
        break;

        case false:
            VID_Local_Gamma_Set (systemgammaramp);
            Con_DPrintLinef ("Gamma system set");
            hardware_is_active = false;
            hardware_enforcer_count = 0;
            hardware_active_enforcer_time_clock = 0;
            break;
        }
#endif
    }

}



void VID_BrightenScreen (void)
{
    float f;
    float current_contrast = CLAMP (VID_MIN_CONTRAST, vid_contrast.value, VID_MAX_CONTRAST);

    if (vid.direct3d == 9)
        return;

    if (current_contrast <=1)
        return; // Because it won't do anything

    f = current_contrast;
    f = pow	(f, 1); // Baker: change to vid_hwgamma

    eglMatrixMode(GL_PROJECTION);
    eglLoadIdentity ();

#pragma message ("Baker: I expect this to act up if we ever center the window use less than fullscreen, would need to do it to test it")
//eglDisable (GL_SCISSOR_TEST);
    eglDisable (GL_DEPTH_TEST); // It was this!
    if (sbar_drawn || console1.visible_pct == 1 || cls.titledemo || cl.intermission)
        eglViewport (clx, cly, clwidth, clheight); // Full screen
    else
    {
#pragma message ("Baker: I expect this to act up if we ever center the window use less than fullscreen, would need to do it to test it")
        eglViewport (clx + r_refdef.vrect.x,
                     cly + vid.screen.height - r_refdef.vrect.y - r_refdef.vrect.height,
                     r_refdef.vrect.width,
                     r_refdef.vrect.height);
    }

    eglOrtho (0, 1, 1, 0, -99999, 99999);
    eglMatrixMode(GL_MODELVIEW);
    eglLoadIdentity ();


    eglDisable (GL_TEXTURE_2D);
    eglEnable (GL_BLEND);
    eglBlendFunc (GL_DST_COLOR, GL_ONE);


    eglBegin (GL_QUADS);
    while (f > 1)
    {
        if (f >= 2)
            eglColor3f (1,1,1);
        else
            eglColor3f (f - 1, f - 1, f - 1);

        eglVertex2f (0, 0);
        eglVertex2f (clwidth, 0);
        eglVertex2f (clwidth, clheight);
        eglVertex2f (0, clheight);
        f *= 0.5;
    }
    eglEnd ();

    eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    eglEnable (GL_TEXTURE_2D);
    eglDisable (GL_BLEND);
    eglColor4f (1,1,1,1);
}


#endif // GLQUAKE_RENDERER_SUPPORT


#ifdef WINQUAKE_RENDERER_SUPPORT

/*
=================
V_CheckGamma
=================
*/


cbool VID_CheckGamma (void)
{
    static float previous_gamma, previous_contrast;

    if (vid_gamma.value == previous_gamma && vid_contrast.value == previous_contrast)
        return false;

    // bound cvars to menu range
    if (vid_contrast.value < VID_MIN_CONTRAST) Cvar_SetValueQuick (&vid_contrast, VID_MIN_CONTRAST);
    if (vid_contrast.value > VID_MAX_CONTRAST) Cvar_SetValueQuick (&vid_contrast, VID_MAX_CONTRAST);
    if (vid_gamma.value < VID_MIN_POSSIBLE_GAMMA) Cvar_SetValueQuick (&vid_gamma, VID_MIN_POSSIBLE_GAMMA);
    if (vid_gamma.value > VID_MAX_POSSIBLE_GAMMA) Cvar_SetValueQuick (&vid_gamma, VID_MAX_POSSIBLE_GAMMA);

    previous_gamma = vid_gamma.value;
    previous_contrast = vid_contrast.value;

    Image_Build_Gamma_Table (vid_gamma.value, vid_contrast.value, vid.gammatable);
    vid.recalc_refdef = 1; // force a surface cache flush

    return true;
}
#endif // WINQUAKE_RENDERER_SUPPORT


#ifdef GLQUAKE_RENDERER_SUPPORT
void Vid_Gamma_TextureGamma_f (lparse_t *line)
{
#ifdef GLQUAKE_TEXTUREGAMMA_SUPPORT
    float val = atof(line->args[1]), clamped_value = CLAMP(VID_MIN_POSSIBLE_GAMMA, val, VID_MAX_POSSIBLE_GAMMA);
    extern float texmgr_texturegamma_current;

#ifdef GLQUAKE_HARDWARE_GAMMA
    if (vid_hardwaregamma.value)
    {
        Con_PrintLinef ("Not applicable while vid_hardwaregamma is on.  Current level = %g", texmgr_texturegamma_current);
        return;
    }
#endif // GLQUAKE_HARDWARE_GAMMA

    switch (line->count)
    {
    default:
        Con_PrintLinef ("Usage: %s <gamma %0.2f to %0.2f> - set gamma level when vid_hardwaregamma is off by baking gamma into textures." NEWLINE
                        "Current texture gamma is %g", line->args[0], (float)VID_MIN_POSSIBLE_GAMMA, (float)VID_MAX_POSSIBLE_GAMMA, texmgr_texturegamma_current);
        return;

    case 2:
        if (vid_texgamma_user_req)
        {
            Con_PrintLinef ("Last requested texture gamma set never acknowledged");
            return;
        }

        if (val != clamped_value)
        {
            Con_PrintLinef ("gamma %g is out of range (%0.2f to %0.2f), clamping to range = %g",
                            val, (float)VID_MIN_POSSIBLE_GAMMA, (float)VID_MAX_POSSIBLE_GAMMA, clamped_value);
            val = clamped_value;
        }

        vid_texgamma_user_req = val;
        // Now we wait for end of frame for it to take effect.
    }
    // Switch num parms texture gamma request end

#endif // GLQUAKE_TEXTUREGAMMA_SUPPORT
}
#endif // GLQUAKE_RENDERER_SUPPORT


/*
=================
VID_BeginRendering -- sets values of clx, cly, clwidth, clheight
=================
*/
void VID_BeginRendering (int *x, int *y, int *width, int *height)
{


#pragma message ("Make sure VID_Resize_Think does NOT do anything essential ever")
#ifdef SUPPORTS_RESIZABLE_WINDOW
    if (!vid.Minimized && vid.ActiveApp && vid.screen.type == MODE_WINDOWED /* <--- that */) // Fixes D3D fullscreenstart
        VID_BeginRendering_Resize_Think (); // Optional resize window on-the-fly
#endif // ! SUPPORTS_RESIZABLE_WINDOW


#ifdef GLQUAKE_RENDERER_SUPPORT
    // Baker: For Direct3D = 4 ... I would like to remove this.  I use it for ALT-TAB and the Direct3D to refresh the status bar
    vid.numpages = vid.direct3d ? 4  /*d3d*/ : /*opengl -> */ (gl_triplebuffer.value) ? 3 : 2;

    sbar_drawn = 0;

    // Baker: Commented out client canvas centering for GL.
    // Center these on the client_canvas
    //	*x = (client_window.width - vid.screen.width) / 2;
    //	*y = (client_window.height - vid.screen.height) / 2;

    //	*width = vid.screen.width;
    //	*height = vid.screen.height;
    *x = *y = 0;
#ifdef SUPPORTS_RESIZABLE_WINDOW
    if (vid.screen.type == MODE_FULLSCREEN)
    {
        *width = vid.screen.width;
        *height = vid.screen.height;
    }
    else
    {
        *width = vid.client_window.width;
        *height = vid.client_window.height;
    }
#else
    *width = vid.screen.width;
    *height = vid.screen.height;
#endif // SUPPORTS_RESIZABLE_WINDOW

#ifdef DIRECT3D9_WRAPPER // Shader gamma.
    {
        static cbool old_shadergamma = 0; // If this is turned from on to off, we need to disable it.
        cbool new_shadergamma = (vid_hardwaregamma.value == 0); // && vid_shadergamma.value;

        VID_Gamma_Contrast_Clamp_Cvars ();

        if (!new_shadergamma)
        {
            //if (new_shadergamma == 0 && old_shadergamma !=0) { // It hates this!
            // Shut it down.
            Direct3D9_SetupGammaAndContrast (1.0, 1.0);
        }
        else if (new_shadergamma)
        {
            void VID_Gamma_Think (void); // Quick access

            if (!Direct3D9_SetupGammaAndContrast (vid_gamma.value, vid_contrast.value))
            {
                // TO DO: set up your fallback mode here
            }
        }
    }
#endif // DIRECT3D9_WRAPPER

#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
    *x = *y = 0;
    *width = vid.conwidth;   // WinQuake now uses vid.conwidth as buffer width
    *height = vid.conheight; // WinQuake now uses vid.conheight as buffer height
    winquake_scr_copytop = 0;
    winquake_scr_copyeverything = 0;
#endif // WINQUAKE_RENDERER_SUPPORT

}

/*
=================
VID_EndRendering
=================
*/

void VID_EndRendering (void)
{
#pragma message ("Can the following prevent gamma restore on minimize?")
    if (scr_skipupdate )
        return;

#ifdef GLQUAKE_RENDERER_SUPPORT

#ifdef PLATFORM_OSX // Baker:  Sigh ... cleanup is something we need.
    // TODO: frame.hardware_gamma or something.  Or more likely ... cbool vid.hardware_gamma;  float vid.hardware_gamma.
    if (vid_hardwaregamma.value) vid_hardwaregamma.value = 0; // Notice we aren't changing the string.  We aren't actually interfering with what writes to config.

#endif // PLATFORM_OSX

    VID_Gamma_Think (); // Baker: Hardware gamma is smart, will self-help and even turn itself off (or on), only when needed
#ifdef GLQUAKE_HARDWARE_GAMMA
    if (!vid_hardwaregamma.value)
#endif // GLQUAKE_HARDWARE_GAMMA
        VID_BrightenScreen (); // Doesn't depend on hardware gamma, but rather GL renderer or not.
#endif // GLQUAKE_RENDERER_SUPPORT

    VID_SwapBuffers ();

#ifdef SUPPORTS_AVI_CAPTURE
// WinQuake captures the frame after swap buffers because vid.buffer must be updated first.
#pragma message ("I moved GL AVI capture after swapbuffers.  See if still good.  Would need to test movie making.")
    Movie_UpdateScreen ();
#endif // SUPPORTS_AVI_CAPTURE // WINQUAKE_RENDERER_SUPPORT
}


unsigned *VID_GetBuffer_RGBA_Malloc (int *width, int *height, cbool bgra)
{

#ifdef GLQUAKE_RENDERER_SUPPORT
    int pixels = clwidth * clheight;

    byte *src_buf = (byte *)malloc (clwidth * clheight * 3);
    byte *dst_buf = (byte *)malloc (clwidth * clheight * 4);

    byte *src;
    byte *dst;
    int i;

    unsigned *buffer = (unsigned *)dst_buf;

    eglReadPixels (clx, cly, clwidth, clheight, GL_RGB, GL_UNSIGNED_BYTE, src_buf);

    if (vid_hardwaregamma.value && (vid_contrast.value != 1 || vid_gamma.value != 1))
    {
        for (i = 0, dst = dst_buf, src = src_buf; i < pixels; i++, dst += 4, src += 3)
        {
            if (bgra)
            {
                dst[0] = vid.gammatable[src[2]];
                dst[1] = vid.gammatable[src[1]];
                dst[2] = vid.gammatable[src[0]];
            }
            else
            {
                dst[0] = vid.gammatable[src[0]];
                dst[1] = vid.gammatable[src[1]];
                dst[2] = vid.gammatable[src[2]];
            }
            dst[3] = 255;
        }
    }
    else
    {
        for (i = 0, dst = dst_buf, src = src_buf; i < pixels; i++, dst += 4, src += 3)
        {
            if (bgra)
            {
                dst[0] = src[2];
                dst[1] = src[1];
                dst[2] = src[0];
            }
            else
            {
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
            }

            dst[3] = 255;
        }
    }

    free (src_buf);
#endif // GLQUAKE_RENDERER_SUPPORT


#ifdef WINQUAKE_RENDERER_SUPPORT
    byte *buffer_ = (byte *)malloc (clwidth * clheight * 4);
    unsigned *buffer = (unsigned *)buffer_;

    {
        int	i, j, rowp;
        byte *p = (byte *)buffer;
        for (i = clheight - 1 ; i >= 0 ; i--)
        {
            rowp = i * vid.rowbytes;
            for (j = 0 ; j < clwidth ; j++)
            {
                if (bgra)
                {
                    *p++ = vid.curpal[vid.buffer[rowp]*3+2];
                    *p++ = vid.curpal[vid.buffer[rowp]*3+1];
                    *p++ = vid.curpal[vid.buffer[rowp]*3+0];
                }
                else
                {
                    *p++ = vid.curpal[vid.buffer[rowp]*3+0];
                    *p++ = vid.curpal[vid.buffer[rowp]*3+1];
                    *p++ = vid.curpal[vid.buffer[rowp]*3+2];
                }
                *p++ = 255;

                rowp++;
            }
        }
    }

#endif // WINQUAKE_RENDERER_SUPPORT

    // We are upside down flip it.
    Image_Flip_Buffer (buffer, clwidth, clheight, 4);

    *width = clwidth;
    *height = clheight;

    return buffer;
}

#ifdef WINQUAKE_RENDERER_SUPPORT
void VID_ShiftPalette (byte *palette)
{
    VID_Local_SetPalette (palette);
    memcpy (vid.curpal, palette, sizeof (vid.curpal));
}

void VID_Palette_NewGame (void)
{
    // Free the old palette
    if (vid.basepal)
        free (vid.basepal);

    vid.basepal = (byte *)COM_LoadMallocFile ("gfx/palette.lmp");
    if (!vid.basepal)
        System_Error ("Couldn't load gfx/palette.lmp");

    if (vid.colormap)
        free (vid.colormap);

    vid.colormap = (byte *)COM_LoadMallocFile ("gfx/colormap.lmp");
    if (!vid.colormap)
        System_Error ("Couldn't load gfx/colormap.lmp");

    VID_Local_Modify_Palette (vid.basepal); // Only MH Windows WinQuake needs this.  On Mac won't do anything.  On WinQuake GL, won't do anything.

    View_Blend_Stale ();
}
#endif // WINQUAKE_RENDERER_SUPPORT
