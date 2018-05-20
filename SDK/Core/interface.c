/*
Copyright (C) 2012-2014 Baker

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
// interface.c -- platform interface

#define CORE_LOCAL

#include "core.h"
#include "environment.h"
#include "interface.h"
#include <stdio.h> // fopen, etc.
#include "file.h"
#include <time.h> // time_t, etc.

///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: URL QUERY
///////////////////////////////////////////////////////////////////////////////


//const char *Folder_Binary_URL (void)
//{
//	return System_URL_Binary ();
//}

const char * Folder_Binary_Folder_URL (void)
{
	static char binary_folder_url[MAX_OSPATH];
	if (!binary_folder_url[0]) {
		c_strlcpy (binary_folder_url, File_Binary_URL () );
		File_URL_Edit_Reduce_To_Parent_Path (binary_folder_url);
	}
	return binary_folder_url;
}


const char * Folder_Caches_URL (void)
{
	static char caches_folder_url [MAX_OSPATH];
	if (!caches_folder_url[0]) {
		if (!gCore_Appname[0])
			log_fatal ("App name not set");
		c_strlcpy (caches_folder_url, _Shell_Folder_Caches_By_AppName(gCore_Appname));
	}
	return caches_folder_url;
}


///////////////////////////////////////////////////////////////////////////////
//  OS AND FOLDER INTERACTION: Baker
///////////////////////////////////////////////////////////////////////////////

cbool Folder_Open (const char *path_to_file)
{
	// Check if folder?
	if (!File_Exists (path_to_file)) {
		logd ("Folder " QUOTED_S " does not exist to show", path_to_file);
		return false;
	}

	return _Shell_Folder_Open_Folder_Must_Exist (path_to_file);
}

cbool Folder_Open_Highlight (const char *path_to_file)
{
	if (!File_Exists (path_to_file))
	{
		logd ("File " QUOTED_S " does not exist to show", path_to_file);
		return false;
	}

	return _Shell_Folder_Open_Highlight_URL_Must_Exist (path_to_file);
}

cbool Folder_Open_Highlight_Binary (void)
{
	const char *binary_url = File_Binary_URL ();

	return Folder_Open_Highlight(binary_url);
}


///////////////////////////////////////////////////////////////////////////////
//  CLIPBOARD -- Baker
///////////////////////////////////////////////////////////////////////////////

// gets image off clipboard
unsigned *Clipboard_Get_Image_Alloc (int *outwidth, int *outheight)
{
	return _Shell_Clipboard_Get_Image_RGBA_Alloc (outwidth, outheight);
}

// gets text from clipboard, spaces out whitespace, terms at newline

char *Clipboard_Get_Text_Line (void)
{
	static char out[SYSTEM_STRING_SIZE_1024];
	char *cliptext_a = _Platform_Clipboard_Get_Text_Alloc ();

	out[0] = 0; // In case cliptext_a is NULL
	if (cliptext_a) {
#if 1
		c_strlcpy (out, cliptext_a);
		cliptext_a = core_free (cliptext_a);
		String_Edit_To_Single_Line (out); // spaces < 32 except for newline, cr, backspace which it kills.
#else
		const char *src = cliptext;
		char *dst = out;
		int remaining = sizeof out - 1;
		/*
		\e	Write an <escape> character.
		\a	Write a <bell> character.
		\b	Write a <backspace> character.
		\f	Write a <form-feed> character.
		\n	Write a <new-line> character.
		\r	Write a <carriage return> character.
		\t	Write a <tab> character.
		\v	Write a <vertical tab> character.
		\'	Write a <single quote> character.
		\\	Write a backslash character.
		*/

		// Truncate at any new line or carriage return or backspace character
		// BUT convert any whitespace characters that are not actual spaces into spaces.
		//while (*src && dst - cliptext < sizeof out - 1 && *src != '\n' && *src != '\r' && *src != '\b')
		while (*src && remaining > 0 && *src != '\n' && *src != '\r' && *src != '\b')
		{
			if (*src < ' ')
				*dst++ = ' ';
			else *dst++ = *src;
			src++;
			remaining --;
		}
		*dst = 0;
		core_free (cliptext);
#endif
		
	}

	return out;
}


char *Clipboard_Get_Text_Alloc (void)
{
	return _Platform_Clipboard_Get_Text_Alloc ();
}


// copies given image to clipbard
cbool Clipboard_Set_Image (unsigned *rgba, int width, int height)
{
	return _Shell_Clipboard_Set_Image_RGBA (rgba, width, height);
}

// copies given text to clipboard
cbool Clipboard_Set_Text (const char * text_to_clipboard)
{
	if (!text_to_clipboard)
		return false;
	return _Platform_Clipboard_Set_Text (text_to_clipboard);
}

