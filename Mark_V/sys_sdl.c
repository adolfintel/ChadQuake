#ifdef CORE_SDL

/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2012 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
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
// sys.c -- system

#include "quakedef.h"
#include "sdlquake.h"
sysplat_t sysplat;



///////////////////////////////////////////////////////////////////////////////
//  FILE IO: Baker ... I'd love to kill these, but it can wait - 2016 Dec
///////////////////////////////////////////////////////////////////////////////


#define	MAX_HANDLES		100 //johnfitz -- was 10
FILE	*sys_handles[MAX_HANDLES];

int findhandle (void)
{
	int		i;

	for (i = 1 ; i < MAX_HANDLES ; i++)
		if (!sys_handles[i])
			return i;

	System_Error ("out of handles");
	return -1;
}


int System_FileOpenRead (const char *path_to_file, int *pHandle)
{
	FILE	*f;
	int		i, retval;

	i = findhandle ();

	f = FS_fopen_read(path_to_file, "rb");

	if (!f)
	{
		*pHandle = -1;
		retval = -1;
	}
	else
	{
		sys_handles[i] = f;
		*pHandle = i;
		//retval = (int) FileHandle_GetLength (f);
		retval = (int) File_Length (path_to_file);
	}

	return retval;
}



int System_FileOpenWrite (const char *path_to_file)
{
	FILE	*f;
	int		i;

	i = findhandle ();

	f = FS_fopen_write(path_to_file, "wb");
	if (!f)
		System_Error ("Error opening %s: %s", path_to_file, strerror(errno));
	sys_handles[i] = f;

	return i;
}

void System_FileClose (int handle)
{
	FS_fclose (sys_handles[handle]);
	sys_handles[handle] = NULL;
}


void System_FileSeek (int handle, int position)
{
	fseek (sys_handles[handle], position, SEEK_SET);
}


int System_FileRead (int handle, void *dest, int count)
{
	return fread (dest, 1, count, sys_handles[handle]);
}

int System_FileWrite (int handle, const void *pdata, int numbytes)
{
	return fwrite (pdata, 1, numbytes, sys_handles[handle]);
}




///////////////////////////////////////////////////////////////////////////////
//  SYSTEM IO
///////////////////////////////////////////////////////////////////////////////


#if id386

/*
================
System_MakeCodeWriteable
================
*/
#ifdef __GNUC__
#include <signal.h>
#endif // __GNUC__
void System_MakeCodeWriteable (unsigned long startaddr, unsigned long len)
{
#ifdef PLATFORM_WINDOWS // Software floating point exceptions
	DWORD  flOldProtect;
	if (!VirtualProtect((LPVOID)startaddr, len, PAGE_READWRITE, &flOldProtect))
   		System_Error("Protection change failed");
#ifdef __GNUC__
    signal (SIGFPE, SIG_IGN); // Baker: Ignore floating point exceptions.  WinQuake.
#endif // __GNUC__

#else // Not PLATFORM_WINDOWS .. Mac and Linux (right?)
	signal (SIGFPE, SIG_IGN); // Baker: Ignore floating point exceptions.  WinQuake.
#endif // !PLATFORM_WINDOWS // WinQuake floating point exceptions
}
#endif // id386




///////////////////////////////////////////////////////////////////////////////
//  SYSTEM ERROR: Baker
///////////////////////////////////////////////////////////////////////////////


int System_Error (const char *fmt, ...)
{
	static int	in_sys_error = 0;
	int was_in_error = in_sys_error;

	VA_EXPAND (text, SYSTEM_STRING_SIZE_1024, fmt);

	Con_DebugLogLine (NEWLINE /*<-- for log legibility*/ SPRINTSFUNC "%s", __func__, text); // Get it in the damned log.

	in_sys_error = 1;

	if (was_in_error == false)
		Input_Shutdown ();

	if (sysplat.mainwindow) {
		SDL_DestroyWindow (sysplat.mainwindow);
		sysplat.mainwindow = 0;
	}

	msgbox (was_in_error ? "Recursive Double Quake Error" : "Quake Error", "%s", text);

	Host_Shutdown ();

	exit (1);
#ifndef __GNUC__ // Return silence
	return 1; // No return as an attribute isn't universally available.
#endif // __GNUC__	// Make GCC not complain about return
}



///////////////////////////////////////////////////////////////////////////////
//  SYSTEM EVENTS
///////////////////////////////////////////////////////////////////////////////

//
//
//
//
//
//
// Called by Modal Message, Download, Install, NotifyBox
void System_SendKeyEvents (void)
{
	Input_Local_SendKeyEvents ();
}


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM QUIT, INIT
///////////////////////////////////////////////////////////////////////////////


void System_Quit (void)
{
	Host_Shutdown();

#ifdef SUPPORTS_NEHAHRA
	Nehahra_Shutdown ();
#endif // SUPPORTS_NEHAHRA


//	if (isDedicated)
//		FreeConsole ();

// shut down QHOST hooks if necessary
//	DeinitConProc ();

	exit (0);
}



// Main_Central calls us
void System_Init (void)
{
    // Nothing here hits console log.
#if id386
	MaskExceptions ();
	Sys_SetFPCW ();
#endif // id386

}

//
//
//
//
//
//
//
//

//
//
//
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////
//  SYSTEM MAIN LOOP
///////////////////////////////////////////////////////////////////////////////


int main (int argc, char *argv[])
{
	char		cmdline[SYSTEM_STRING_SIZE_1024];
	uintptr_t 	fakemainwindow = 0;
	int			done = 0;
	double 		oldtime;

	String_Command_Argv_To_String (cmdline, argc - 1, &argv[1], sizeof(cmdline));


	//SDL_AudioInit ("winmm"); // ericw!  No help.  Static linked version sound is terrible for 11025 for sure.
	Main_Central (cmdline, &fakemainwindow, false /* we perform loop ourselves */);

	oldtime = System_DoubleTime ();

	SDL_InitSubSystem (SDL_INIT_EVENTS );

	while (!done)
	{
		double time, newtime;
		SDL_Event	my_event;

		while (!done && SDL_PollEvent (&my_event)) {
			done = Session_Dispatch (&my_event);
		}
        newtime = System_DoubleTime();
        time = newtime - oldtime;

        Host_Frame(time);

        // throttle the game loop just a little bit - noone needs more than 1000fps, I think
        if (newtime - oldtime < 1)
            SDL_Delay(1);

        oldtime = newtime;
    }

    System_Quit();
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM DISPATCH
///////////////////////////////////////////////////////////////////////////////
//
//
//
//

//
//
//
//



// Return 0 normally.  Return 1 if received quit.
int Session_Dispatch (void *_sdl_event)
{
	cbool is_hidden = false, is_activate = false;
	SDL_Event  *e    = _sdl_event;

	// check for input messages
	if (SDLQ_IN_ReadInputMessages (e))
		return 0; // Input message was handled

	if (e->type == SDL_WINDOWEVENT) {
		switch (e->window.event) {
		case SDL_WINDOWEVENT_HIDDEN:
			// Will we ever do this in Quake.  Should we ever do this?
			is_hidden = true; // Fall through ..
			VID_AppActivate (false /* can't be active*/, false /*minimize*/, is_hidden /*hidden*/);
			return 0;

		case SDL_WINDOWEVENT_SHOWN:
			// WM_SHOWWINDOW equivalent.  We don't do anything with this in Quake.  Windows would do cbool is_hidden = wparam != 0;
			// Window shown.  Does this happen?
			return 0;

		// case WM_GETMINMAXINFO:  No equivalent needed, use SDL_SetWindowMinimumSize, etc.
		case SDL_WINDOWEVENT_CLOSE:
			// WM_CLOSE equivalent.  Asking that the window be closed.  Not same as quit, but since Quake is single window it is same.
			// Note that we are not obligated to close.  But for Quake we will anyway. Plus SDL automatically closes app when last window closed and apparently CTRL-Q on Mac.
			{ SDL_Event sdlevent = {0}; SDL_PushEvent(&sdlevent); }
			return 0;

		// SDL has a mouse focus version of these for Linux
		// Note that I think the Mac basically works like that too
		// Especially for mouse wheel.

		case SDL_WINDOWEVENT_FOCUS_GAINED:
			is_activate = true;  logd ("SDL_WINDOWEVENT_FOCUS_GAINED");
			// Fall through ..

		case SDL_WINDOWEVENT_FOCUS_LOST:
			// WM_ACTIVATE equivalent.
			if (!is_activate) logd ("SDL_WINDOWEVENT_FOCUS_LOST");

			VID_AppActivate (is_activate, false /*minimize*/, false /*hidden*/);
			return 0;

		case SDL_WINDOWEVENT_MINIMIZED:
			// WM_SIZE equivalent.
			VID_AppActivate (false /*can't be active*/, true /*minimize*/, false /*hidden*/);
			logd ("SDL_WINDOWEVENT_MINIMIZED");
			return 0;

		case SDL_WINDOWEVENT_RESTORED:
			// WM_SIZE equivalent.
			// If unhidden do we throw a paint event? SDL IS NOT THROWING A PAINT EVENT ON RESTORE
			// Fall through ... since we are about the same as a resize
			VID_AppActivate (true /*can't be active*/, false /*minimize*/, false /*hidden*/);
			logd ("SDL_WINDOWEVENT_RESTORED");
			return 0;

		case SDL_WINDOWEVENT_RESIZED: // <--- this doesn't always happen.  User changed size, I think
			logd ("SDL_WINDOWEVENT_RESIZED");

			if (vid.Minimized || !vid.ActiveApp || vid.Hidden)
				return 0;

			logd ("SDL_WINDOWEVENT_RESIZED + THINK");
			VID_Resize_Check (1);
			return 0;

		case SDL_WINDOWEVENT_MAXIMIZED:
			logd ("SDL_WINDOWEVENT_MAXIMIZED");
			logd ("SDL_WINDOWEVENT_RESIZED + THINK");
#if !defined(PLATFORM_LINUX)
			if (!vid.ActiveApp || vid.Hidden)
				return 0;
#endif // PLATFORM_LINUX
			VID_Resize_Check (1);
			return 0;

		case SDL_WINDOWEVENT_SIZE_CHANGED:
#ifdef PLATFORM_LINUX
			VID_Resize_Check (1);
#endif // PLATFORM_LINUX
			logd ("SDL_WINDOWEVENT_SIZE_CHANGED");
			return 0;


			// Fall through ... since this are about the same as window move

		case SDL_WINDOWEVENT_MOVED:
			// Could do a moved event here
			logd ("SDL_WINDOWEVENT_MOVED");
			return 0;

		case SDL_WINDOWEVENT_EXPOSED:
			// Window requires some repainting
			//logd ("SDL_WINDOWEVENT_EXPOSED");  // We don't care.
			return 0;

		default:
			return 0; // Whatever it was, we didn't handle it.  But it was a WindowEvent
		} // End of switch

	} // End if windowevent

//
// It wasn't a window event
//

	// Unhandled ...
	switch (e->type) {
	case SDL_MOUSEMOTION:
		// Accumulate?  Nah.
		// IN_MouseMove(event.motion.xrel, event.motion.yrel);
		return 0;

	case SDL_QUIT:
		return 1; // This means quit!

	default:
		return 0;
	}
}


// Main_Central calls us.  But SDL does not currently use that
void System_SleepUntilInput (int time)
{
	// Used to sleep us until input - does SDL have an equivalent?
	// Nothing?
}

//
//
//
//
//
//
//
//
//
//
//

//
//
//
//
//
//
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////
//  CONSOLE:
///////////////////////////////////////////////////////////////////////////////


const char *Dedicated_ConsoleInput (void)
{
    char *pText = NULL;

//#ifdef SERVER_ONLY
//
//    if (stdin_ready != 0)
//    {
//        static char     text[256];
//        const int		length = read (0, text, FD_SIZE_OF_ARRAY (text));
//
//        stdin_ready = 0;
//
//        if (length > 0)
//        {
//			Con_PrintLinef ("%d",(int)length);
//            text[length - 1]    = '\0';
//            pText               = &(text[0]);
//        }
//    }
//
//#endif // SERVER_ONLY

    return pText;
}

#endif // CORE_SDL
