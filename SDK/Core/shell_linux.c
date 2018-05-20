/*
Copyright (C) 2012-2016 Baker

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
// shell_linux.c -- Linux graphical interaction layer
// Platform unavoidable and SDL doesn't provide.
// Bundle - Storage Paths - Images for clipboard - Dialogs
// There are dialogs in here and folder explorers.  A console app wouldn't normal use them, but no reason to prevent them.

// NOTE: If you get during startup program exited with code 126 - Check permissions!!!!
// Make sure chmod 777 --- remember can't build on flash drive :(

#include "environment.h"
#ifdef PLATFORM_LINUX

#define CORE_LOCAL
#include "core.h"




///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: URL QUERY
///////////////////////////////////////////////////////////////////////////////

// Baker: Function is unused currently
const char *File_Binary_URL (void)
{
    static char binary_url[MAX_OSPATH];
	if (!binary_url[0]) {
		pid_t pid = getpid();
		int length;

		char linkname[MAX_OSPATH];
		c_snprintf1 (linkname, "/proc/%d/exe", pid);

		length = readlink (linkname, binary_url, sizeof(binary_url)-1);

    	// In case of an error, leave the handling up to the caller
    	if (length == -1 || length >= (int)sizeof(binary_url) )
			log_fatal ("Couldn't determine executable directory");

    	binary_url[length] = 0;
	}
	return binary_url;
}





const char *_Shell_Folder_Caches_By_AppName (const char* appname)
{
    static char cachesfolder[MAX_OSPATH];
	if (!cachesfolder[0])
    	c_strlcpy (cachesfolder, "./_caches"); // I hope this works ok.
    return cachesfolder;
}






///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: CLIPBOARD IMAGE OPERATIONS
///////////////////////////////////////////////////////////////////////////////




cbool _Shell_Clipboard_Set_Image_RGBA (const unsigned *rgba, int width, int height)
{
    return false;
}



unsigned *_Shell_Clipboard_Get_Image_RGBA_Alloc (int *outwidth, int *outheight)
{
    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: VIDEO LIMITED
///////////////////////////////////////////////////////////////////////////////


cbool Vid_Desktop_Properties_Get (reply int *left, reply int *top, reply int *width, reply int *height)
{
#pragma message ("LINUX: I don't know if we can get the desktop usable area nor how")
	int x0, y0, w0, h0;
	Vid_Display_Properties_Get (&x0, &y0, &w0, &h0, NULL);

	NOT_MISSING_ASSIGN (left, 0);
	NOT_MISSING_ASSIGN (top, 0);
	NOT_MISSING_ASSIGN (width,  w0 - 20);
	NOT_MISSING_ASSIGN (height, h0 - 80);

	return true;
}


int _Shell_Window_Style (wdostyle_e style)
{
	return 0;
}

int _Shell_Window_StyleEx (wdostyle_e style)
{
	return 0;
}


void Vid_Handle_Borders_Get (wdostyle_e style, cbool have_menu, reply int *left, reply int *top, reply int *width, reply int *height, reply int *right, reply int *bottom)
{
	int plat_style		= _Shell_Window_Style   (style);
	int plat_style_ex	= _Shell_Window_StyleEx (style);

	const int width_320 = 320, height_240 = 240;
	crectrb_t bordered = {0, 22, 0, 22, 0, 0 }; // NFI for Linux how to get this
	crectrb_t borderless = {0};
	crectrb_t border = Flag_Check(style, wdostyle_borderless) ? bordered : borderless;

	NOT_MISSING_ASSIGN (left, border.left);
	NOT_MISSING_ASSIGN (top, border.top);
	NOT_MISSING_ASSIGN (width, border.width);
	NOT_MISSING_ASSIGN (height, border.height);
	NOT_MISSING_ASSIGN (right,  border.right);
	NOT_MISSING_ASSIGN (bottom, border.bottom);
}


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: FILE DIALOGS
///////////////////////////////////////////////////////////////////////////////

// See https://github.com/mlabbe/nativefiledialog for GTK shit




const char *_Shell_Dialog_Open_Directory (const char *title, const char *starting_folder_url)
{

	return NULL;
}


const char *_Shell_Dialog_Open_Type (const char *title, const char * starting_folder_url, const char *extensions_comma_delimited)
{
	return NULL;
}



const char * _Shell_Dialog_Save_Type (const char *title, const char * starting_folder_url, const char *extensions_comma_delimited)
{

	return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: FILE MANAGER INTERACTION
///////////////////////////////////////////////////////////////////////////////


// Folder must exist.  It must be a folder.
cbool _Shell_Folder_Open_Folder_Must_Exist (const char *path_to_file)
{
//  xdg-open is a desktop-independent tool for configuring the default applications of a user
	if ( fork() == 0) {
		execl ("/usr/bin/xdg-open", "xdg-open", path_to_file, (char *)0);
		//cleanup_on_exit();  /* clean up before exiting */
        exit(3);
	}

    return true;
}


// File must exist
cbool _Shell_Folder_Open_Highlight_URL_Must_Exist (const char *path_to_file)
{
	char folder_url[MAX_OSPATH];
	if (!File_Exists (path_to_file))
	{
		log_debug ("File " QUOTED_S " does not exist to show", path_to_file);
		log_debug ("File does not exist to show");
		return false;
	}

#if 0
	// If we do it this way, it does the application associated with the file instead of showing it in the folder and highlighting it :(
	// Which is a fail to me.
	if ( fork() == 0) {
		execl ("/usr/bin/xdg-open", "xdg-open", path_to_file, (char *)0);
		//cleanup_on_exit();  /* clean up before exiting */
        exit(3);
	}

#else
	c_strlcpy (folder_url, path_to_file);
	File_URL_Edit_Reduce_To_Parent_Path (folder_url);
	return Folder_Open (folder_url); // Doesn't highlight at this time

#endif

    //http://stackoverflow.com/questions/7652928/launch-osx-finder-window-with-specific-files-selected
    return true;
}


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: STARTUP/ICON
///////////////////////////////////////////////////////////////////////////////

// Considerations: Windows sticky keys, Windows key (full-screen only), Mac mouse acceleration (mouse, not keyboard though)
cbool Shell_Input_KeyBoard_Capture (cbool bDoCapture, cbool ActOnStickeyKeys, cbool bSuppressWindowskey)
{
// Nothing?  For now, I guess.
	return bDoCapture;
}



#ifndef CORE_SDL // MachineTime and Clipboard - we defer to SDL if CORE_SDL build

	///////////////////////////////////////////////////////////////////////////////
	//  PLATFORM: MACHINE TIME
	///////////////////////////////////////////////////////////////////////////////

	// Double self-initializes now
	double Platform_MachineTime (void) // no set metric except result is in seconds
	{
		#pragma error ("Only support Linux via SDL")
	}



	///////////////////////////////////////////////////////////////////////////////
	//  PLATFORM: CLIPBOARD HELPER FUNCTIONS FOR INTERFACE.C
	///////////////////////////////////////////////////////////////////////////////


	char *_Platform_Clipboard_Get_Text_Alloc (void)
	{
		char	*text_out = NULL;
		#pragma error ("Only support Linux via SDL")
		return text_out;
	}


	// copies given text to clipboard.  Text can't be NULL
	cbool _Platform_Clipboard_Set_Text (const char *text_to_clipboard)
	{
		#pragma error ("Only support Linux via SDL")
	}

#endif // !CORE_SDL -- MachineTime and Clipboard - we defer to SDL if CORE_SDL build




#ifdef CORE_SDL

	///////////////////////////////////////////////////////////////////////////////
	//  SHELL: PLATFORM
	///////////////////////////////////////////////////////////////////////////////

	cbool Shell_Platform_Icon_Load (void *key /*wildcard*/)
	{
		// TODO
		return true;
	}

	cbool Shell_Platform_Icon_Window_Set (sys_handle_t cw)
	{
		// TODO

		return true;
	}

#endif // CORE_SDL

#endif // PLATFORM_LINUX