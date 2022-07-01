/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2012 John Fitzgibbons and others
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
// sys.c -- system

#include "quakedef.h"



int Main_Central_Loop ()
{
#ifndef CORE_SDL
	double oldtime = System_DoubleTime ();

    /* main window message loop */
	while (1)
	{
		double time, newtime;

		switch (isDedicated)
		{
		case true: // Dedicated server
			newtime = System_DoubleTime ();
			time = newtime - oldtime;

			while (time < sys_ticrate.value )
			{
				System_Sleep_Milliseconds (QUAKE_DEDICATED_SLEEP_TIME_MILLISECONDS_1 /* 1 */);
				newtime = System_DoubleTime ();
				time = newtime - oldtime;
			}
			break;

		case false:

			newtime = System_DoubleTime ();
			time = newtime - oldtime;
			break;
		}

		Host_Frame (time);
		oldtime = newtime;
	}
#endif // !CORE_SDL

    /* return success of application */
    return 0; // Baker: unreachable
}


int Main_Central (char *cmdline, void *main_window_holder_addr, cbool do_loop)
{
	const char	*executable_directory = Folder_Binary_Folder_URL ();
	char		*argv[MAX_NUM_Q_ARGVS_50 + 30] = { "" }; // Set first one to empty string

	Core_Init (ENGINE_FAMILY_NAME, &qfunction_set, main_window_holder_addr);

	//
	// Set Up Parameters
	//

	c_strlcpy (host_parms._basedir, File_Getcwd()); // Uses current working directory.

	// Baker: On Windows if a user makes a shortcut and doesn't set the "Start In" directory, it won't find the pak files
	// we will help by silently checking for situation and correcting the directory
	//	alert ("Current basedir is " QUOTED_S ".", host_parms._basedir);
	
	if (!File_Exists (va ("%s/id1/pak0.pak", host_parms._basedir)) && File_Exists (va ("%s/id1/pak0.pak", executable_directory)) ) {
		// Copy exe_dir to cwd}
		c_strlcpy (host_parms._basedir, executable_directory); 
	}  // Right?


	host_parms.basedir = host_parms._basedir;
	host_parms.argc = 1;
	host_parms.argv = argv; // Null out of the executable name

	// Reconstruct the argc/argv from the cmdline
	String_Command_String_To_Argv (cmdline, &host_parms.argc, argv, MAX_NUM_Q_ARGVS_50);

	COM_InitArgv (host_parms.argc, host_parms.argv);

	//	alert ("Current final is " QUOTED_S ".", host_parms.basedir);


	Memory_Init ();

	//
	// Parameters and memory initialized
	//

	System_Init (); // Initializes time, sets some things for WinQuake asm id386 and checks Windows version.  Floating point exceptions, ..

// because sound is off until we become active
	S_BlockSound ();

	Host_Init ();

	if (!do_loop)
		return 0; // Mac uses a frame timer

	return Main_Central_Loop();
}
