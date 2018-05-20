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
// shell_ios.m -- IOS graphical interaction layer
// Platform unavoidable and SDL doesn't provide.
// Bundle - Storage Paths - Images for clipboard - Dialogs
// There are dialogs in here and folder explorers.  A console app wouldn't normal use them, but no reason to prevent them.

#include "environment.h"
#ifdef PLATFORM_IOS

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
	    NSString *_basePath = [[[NSBundle mainBundle] bundlePath] stringByStandardizingPath];
	    c_strlcpy (binary_url, TO_CSTRING(_basePath)  );
	}
	return binary_url;
}





const char *_Shell_Folder_Caches_By_AppName (const char *appname)
{
	// UNUSED: appname   (appname isn't necessary for Mac)
    static char caches_folder_for_appname[MAX_OSPATH];

	if (!caches_folder_for_appname[0]) {
	    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
	    NSString* mydir = [paths objectAtIndex:0];
	    NSString *myAppID = [[NSBundle mainBundle] bundleIdentifier];
	    NSString *cachesDirectory = [NSString stringWithFormat:@"%@/%@", mydir, myAppID];
	    c_strlcpy (caches_folder_for_appname, TO_CSTRING(cachesDirectory)  );
	}    



    return caches_folder_for_appname;    
}


///////////////////////////////////////////////////////////////////////////////
//  SHELL: Bundle
///////////////////////////////////////////////////////////////////////////////

const char *sBundleFolder (void)
{
    static char bundlefolder[MAX_OSPATH];

	if (!bundlefolder[0]) {
    	NSString *mydir = [[NSBundle mainBundle] resourcePath];
    	c_strlcpy (bundlefolder, TO_CSTRING(mydir)  );
	}

    return bundlefolder;
}

const char *sBundlePack (void)
{
    static char buf[MAX_OSPATH];

	if (!buf[0])
    	c_snprintf2 (buf, "%s/%s", sBundleFolder(), "bundle.pak");

    return buf;
}

const void *Shell_Data_From_Resource (size_t *mem_length, cbool *must_free)
{
// Bundling it up!
// For Apple we will load from bundle data.

	const char *bundle_url = sBundlePack();

	size_t num_bytes;
	void *mem			= File_To_Memory_Alloc (bundle_url, &num_bytes);

	NOT_MISSING_ASSIGN (mem_length, num_bytes);
	NOT_MISSING_ASSIGN (must_free, true); // Yes because we loaded from app bundle file.
	return num_bytes ? mem : NULL;
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
	CGRect rect = [[UIScreen mainScreen] applicationFrame];

	int display_height; Vid_Display_Properties_Get (NULL, NULL, NULL, &display_height, NULL);
	int display_top = display_height - rect.origin.y - 1; // shell_osx.m, I am matching SDL.  Here I'm not.

	NOT_MISSING_ASSIGN(left, rect.origin.x);
	NOT_MISSING_ASSIGN(top, display_top);
	NOT_MISSING_ASSIGN(width, rect.size.width);
	NOT_MISSING_ASSIGN(height, rect.size.height);
	
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

	CGRect window_rect = CGRectMake (0, 0, 320, 240);
	CGRect client_rect = window_rect; //[NSWindow contentRectForFrameRect:window_rect styleMask:plat_style];


	NOT_MISSING_ASSIGN (left, 0);
	NOT_MISSING_ASSIGN (top, 0);
	NOT_MISSING_ASSIGN (width, 0);
	NOT_MISSING_ASSIGN (height, 0);
	NOT_MISSING_ASSIGN (right, 0); 
	NOT_MISSING_ASSIGN (bottom, 0);
}


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: FILE DIALOGS
///////////////////////////////////////////////////////////////////////////////




// iPhone doesn't support dialogs
const char * _Shell_Dialog_Open_Directory (const char *title, const char *starting_folder_url)
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
    return false; // unimplemented
}

// File must exist
cbool _Shell_Folder_Open_Highlight_URL_Must_Exist (const char *path_to_file)
{
    return false; // unimplemented
}


#ifndef CORE_SDL // MachineTime and Clipboard - we defer to SDL if CORE_SDL build

	///////////////////////////////////////////////////////////////////////////////
	//  PLATFORM: MACHINE TIME
	///////////////////////////////////////////////////////////////////////////////
	
	// Double self-initializes now
	
	double Platform_MachineTime (void) // no set metric
	{
	    static uint64_t start_time   = 0;
	    static double   scale       = 0.0;
	    const uint64_t  time        = mach_absolute_time();
	    
	    if (start_time == 0)
	    {
	        mach_timebase_info_data_t   info = { 0 };
	        
	        if (mach_timebase_info (&info) != 0)
	        {
	            log_fatal ("Failed to read timebase!");
	        }
	        
	        scale       = 1e-9 * ((double) info.numer) / ((double) info.denom);
	        start_time   = time;
	    }
		
	    return (double) (time - start_time) * scale;
	}
	

	
	///////////////////////////////////////////////////////////////////////////////
	//  PLATFORM: CLIPBOARD HELPER FUNCTIONS FOR INTERFACE.C
	///////////////////////////////////////////////////////////////////////////////
	
	
	char *_Platform_Clipboard_Get_Text_Alloc (void)
	{	
		char	*text_out = NULL;
	
		UIPasteboard *myPasteboard = [UIPasteboard generalPasteboard];
		
		NSString *clipboardString = [myPasteboard string];
		if (clipboardString && [clipboardString length])
	    	text_out =  core_strdup (TO_CSTRING (clipboardString ));
		return text_out;
	}
	
	
	// copies given text to clipboard.  Text can't be NULL
	cbool _Platform_Clipboard_Set_Text (const char *text_to_clipboard)
	{
		UIPasteboard *myPasteboard = [UIPasteboard generalPasteboard];
	
		[myPasteboard setString:CSTRING(text_to_clipboard)];
		return true;
	}

#endif // !CORE_SDL -- MachineTime and Clipboard - we defer to SDL if CORE_SDL build

#endif // PLATFORM_IOS


