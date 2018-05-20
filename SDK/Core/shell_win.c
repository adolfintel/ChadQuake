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
// shell_win.c -- Windows graphical interaction layer.
// Platform unavoidable and SDL doesn't provide.
// Bundle - Storage Paths - Images for clipboard - Dialogs
// There are dialogs in here and folder explorers.  A console app wouldn't normal use them, but no reason to prevent them.

#include "environment.h"
#ifdef PLATFORM_WINDOWS

#define CORE_LOCAL
#include "core.h"
#include "core_windows.h" // <windows.h> // GetCurrentDirectory, etc.


#pragma comment (lib, "shell32.lib")  	// ShellExecute
#pragma comment (lib, "gdi32.lib")	  	// CreateBitmap (clipboard image ops)
#pragma comment (lib, "kernel32.lib")	// CreateBitmap (clipboard image ops)
#pragma comment (lib, "comdlg32.lib")	// File dialog stuff?
#pragma comment (lib, "winmm.lib")		// timeBeginPeriod

///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: URL QUERY
///////////////////////////////////////////////////////////////////////////////

// Baker: Function is unused currently (not any more!!!)

const char *File_Binary_URL (void)
{
	static char binary_url[MAX_OSPATH];
	if (!binary_url[0]) {
		int length = GetModuleFileNameA (NULL, binary_url, sizeof(binary_url) - 1 );
		if (!length)
			log_fatal ("Couldn't determine executable directory");

		//MSDN: Windows XP:  The string is truncated to nSize characters and is not null-terminated.
		binary_url[length] = 0;
		File_URL_Edit_SlashesForward_Like_Unix (binary_url);
	}
	return binary_url;
}


const char *Shell_Windows_GetEnv_Path_URL (const char *varname)
{
	static char enviro_path[MAX_OSPATH];

	char *_s = getenv(varname); // "APPDATA");

	if (!_s) {
		return NULL; // Found found.
	}
	c_strlcpy (enviro_path, _s); File_URL_Edit_SlashesForward_Like_Unix (enviro_path);

	return enviro_path;
}


const char *Shell_Windows_Folder_System32 (void)
{
	static char folder_system32_path[MAX_OSPATH];
	if (!folder_system32_path[0]) {
		const char *_appdata = Shell_Windows_GetEnv_Path_URL ("COMSPEC");
		c_strlcpy (folder_system32_path, _appdata); File_URL_Edit_SlashesForward_Like_Unix (folder_system32_path);
		File_URL_Edit_Reduce_To_Parent_Path (folder_system32_path);
	}
	return folder_system32_path;
}


static const char *sShell_URL_AppData (void)
{
	static char appdatapath[MAX_OSPATH];
	if (!appdatapath[0]) {
		const char *_appdata = Shell_Windows_GetEnv_Path_URL ("APPDATA");
		c_strlcpy (appdatapath, _appdata); File_URL_Edit_SlashesForward_Like_Unix (appdatapath);
	}
	return appdatapath;
}


const char *_Shell_Folder_Caches_By_AppName (const char *appname)
{
#define CACHES_DIR_SUBPATH_OF_APPDATA_WINDOWS "caches"
	static char caches_folder_for_appname [MAX_OSPATH];
	c_snprintf3 (caches_folder_for_appname, "%s/%s/%s", sShell_URL_AppData(), appname, CACHES_DIR_SUBPATH_OF_APPDATA_WINDOWS);

	return caches_folder_for_appname;
}




///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: CLIPBOARD IMAGE OPERATIONS
///////////////////////////////////////////////////////////////////////////////


static cbool sShell_Clipboard_Set_Image_BGRA (const unsigned *bgra, int width, int height)
{
	HBITMAP hBitmap = CreateBitmap (width, height, 1, 32 /* bits per pixel is 32 */, bgra);

	OpenClipboard (NULL);

	if (EmptyClipboard()) {
		if ((SetClipboardData (CF_BITMAP, hBitmap)) == NULL) {
			logd ("SetClipboardData failed"); // Was fatal.  But for clipboard?  Seriously?
			return false;
		}
	}

	CloseClipboard ();
	return true;
}


static void sSystem_Clipboard_Set_Image_RGBA_Maybe_Flip (const unsigned *rgba, int width, int height, cbool flip)
{
	int		pelscount = width * height;
	int		buffersize = pelscount * RGBA_4;
	byte    *bgra_data_a = core_malloc (buffersize); // Clipboard From RGBA work
//	int		i;
//	byte	temp;

	memcpy (bgra_data_a, rgba, buffersize);

	// If flip ....
	if (flip)
		Image_Flip_Buffer (bgra_data_a, width, height, RGBA_4);

	// RGBA to BGRA so clipboard will take it
#if 1
	Image_Flip_RedGreen (bgra_data_a, width * height * RGBA_4);
#else
	for (i = 0 ; i < buffersize ; i += RGBA_4)
	{
		temp = bgra_data[i];

		bgra_data[i] = bgra_data[i + 2];
		bgra_data[i + 2] = temp;
	}
#endif

	sShell_Clipboard_Set_Image_BGRA ((unsigned *)bgra_data_a, width, height);
	core_free (bgra_data_a);
}

cbool _Shell_Clipboard_Set_Image_RGBA (const unsigned *rgba, int width, int height)
{
	sSystem_Clipboard_Set_Image_RGBA_Maybe_Flip (rgba, width, height, false);
	return true;
}



unsigned *_Shell_Clipboard_Get_Image_RGBA_Alloc (int *outwidth, int *outheight)
{
// Baker: Was extremely helpful info ... https://sites.google.com/site/michaelleesimons/clipboard
	byte *ptr = NULL;

	if (OpenClipboard(NULL))
	{
		HBITMAP hBitmap = GetClipboardData (CF_BITMAP);
		BITMAP csBitmap;
		if (hBitmap && GetObject(hBitmap, sizeof(csBitmap), &csBitmap) && csBitmap.bmBitsPixel == BPP_32)
		{
			// allocate buffer
			int i, bufsize = csBitmap.bmWidth * csBitmap.bmHeight * (csBitmap.bmBitsPixel / 8);

			csBitmap.bmBits = ptr = core_malloc (bufsize); // "bmbits buffer"
			GetBitmapBits((HBITMAP)hBitmap, bufsize, csBitmap.bmBits );

			// Convert BGRA --> RGBA, set alpha full since clipboard loses it somehow
			for (i = 0; i < bufsize; i += RGBA_4)
			{
				byte temp = ptr[i + 0];
				ptr[i + 0] = ptr[i + 2];
				ptr[i + 2] = temp;
				ptr[i + 3] = 255; // Full alpha
			}
			*outwidth = csBitmap.bmWidth;
			*outheight = csBitmap.bmHeight;
		}
		CloseClipboard ();
	}
	return (unsigned *)ptr;
}


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: VIDEO LIMITED
///////////////////////////////////////////////////////////////////////////////


cbool Vid_Desktop_Properties_Get (reply int *left, reply int *top, reply int *width, reply int *height)
{
	RECT rect;

	SystemParametersInfo (SPI_GETWORKAREA, 0, &rect, 0);

	NOT_MISSING_ASSIGN(left, rect.left);
	NOT_MISSING_ASSIGN(top, rect.top);
	NOT_MISSING_ASSIGN(width, rect.right - rect.left);
	NOT_MISSING_ASSIGN(height, rect.bottom - rect.top);

	return true;
}


int _Shell_Window_Style (wdostyle_e style)
{
	int dw_bordered		= (WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX /* resize */);
	int dw_borderless 	= (WS_POPUP); // Window covers entire screen; no caption, borders etc
	int ret = Flag_Check (style, wdostyle_borderless) ? dw_borderless : dw_bordered;

	if (!Flag_Check (style, wdostyle_resizable))
		ret = Flag_Remove (style, WS_MAXIMIZEBOX | WS_SIZEBOX);

	return ret;
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

	RECT client_rect 	= {0, 0, width_320, height_240};
	RECT window_rect	= client_rect;

	AdjustWindowRectEx (&window_rect, plat_style, have_menu /*has menu? */, plat_style_ex); // Expands adding borders

	NOT_MISSING_ASSIGN (left,  -window_rect.left);
	NOT_MISSING_ASSIGN (top, -window_rect.top);
	NOT_MISSING_ASSIGN (width, -window_rect.left + (window_rect.right - width_320));
	NOT_MISSING_ASSIGN (height, -window_rect.top + (window_rect.bottom - height_240));
	NOT_MISSING_ASSIGN (right,  window_rect.right - width_320);
	NOT_MISSING_ASSIGN (bottom, window_rect.bottom - height_240);
}


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: FILE DIALOGS
///////////////////////////////////////////////////////////////////////////////


static void WIN_Fill_Extensions_Request (char *s, const char *extension_comma_list, size_t len)
{
	const char *allfiles0 = "All Files (*.*)";
	const char *allfiles1 = "*.*";

	clist_t * extensions = List_String_Split_Alloc (extension_comma_list, ',');

	clist_t *cur;
	int count;

	for (cur = extensions, count = 0; cur; cur = cur->next)
	{
		const char *_description0 = (cur->name[0] && cur->name[1] ) ? &cur->name[1] : "";
		const char *_description1 = va ("%s", _description0);
		const char *description;

		String_Edit_To_Upper_Case ((char *)_description1); // Evile ...

		description = va("%s files (*%s)", _description1, cur->name);

		strcpy (&s[count], description); count += strlen(description); // Add, increment count
		s[count] = 0; count ++; // Add, increment count
		strcpy (&s[count], "*"); count ++; // Add, increment count
		strcpy (&s[count], cur->name); count += strlen(cur->name); // Add, increment count
		s[count] = 0; count ++; // Add, increment count
	}
	strcpy (&s[count], allfiles0); count += strlen(allfiles0); // Add, increment count
	s[count] = 0; count ++; // Add, increment count
	strcpy (&s[count], allfiles1); count += strlen(allfiles1); // Add, increment count
	s[count] = 0; count ++; // Add, increment count

	// Double NULL termination
	s[count] = 0;
	List_Free (&extensions);
}

static char m_last_open_directory[MAX_OSPATH];

const char *_Shell_Dialog_Open_Directory (const char *title, const char *starting_folder_url)
{
	static char directorySelected[MAX_OSPATH];

	directorySelected[0] = 0;
#if 0

	// Determine starting directory
	NSString *startingDirectory = nil;

	if (starting_folder_url)
		startingDirectory = [NSString stringWithUTF8String:starting_folder_url];
	else if ([lastOpenDirectory length] == 0)
		startingDirectory = [NSString stringWithUTF8String:Folder_Binary_Folder_URL()];
	else startingDirectory = [lastOpenDirectory copy];

	NSURL *startingDirectoryURL = [NSURL URLWithString:[startingDirectory
														stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];

	// Determine title string
	NSString *titleString = @"Select Folder";

	if (title)
		titleString = [NSString stringWithUTF8String:title];

	NSAutoreleasePool*  pool    = [[NSAutoreleasePool alloc] init];
	NSOpenPanel*        panel   = [[[NSOpenPanel alloc] init] autorelease];


	[panel setDirectoryURL:startingDirectoryURL];
	[panel setAllowsMultipleSelection: NO];
	[panel setCanChooseFiles: NO];
	[panel setCanChooseDirectories: YES];
	[panel setTitle: titleString];

	if ([panel runModal])
	{
		// Ok button was selected
		NSString *_directorySelected = [[panel directoryURL] path]; // Directory result
		c_strlcpy (directorySelected, [_directorySelected cStringUsingEncoding: NSASCIIStringEncoding]);
		lastOpenDirectory = [_directorySelected copy];
	}

	[pool release];
#endif

	return directorySelected;
}

#include <Commdlg.h> // Never needed this before?
const char *_Shell_Dialog_Open_Type (const char *title, const char * starting_folder_url, const char *extensions_comma_delimited)
{
	static char file_selected[MAX_OSPATH];

	char win_filter[SYSTEM_STRING_SIZE_1024] = {0};// FilterSpec ="Object Files(.obj)\0*.obj\0Text Files(.txt)\0.txt\0All Files(.)\0*.*\0";
	char startingfolder[MAX_OSPATH];

    OPENFILENAME ofn = {0};

	// Fill in the extensions list
	WIN_Fill_Extensions_Request (win_filter, extensions_comma_delimited, sizeof(win_filter));

	// Determine starting folder
	if (starting_folder_url)					c_strlcpy (startingfolder, starting_folder_url);
	else if (strlen (m_last_open_directory))	c_strlcpy (startingfolder, m_last_open_directory);
	else 										c_strlcpy (startingfolder, Folder_Binary_Folder_URL ());

	File_URL_Edit_SlashesBack_Like_Windows (startingfolder);
	file_selected[0] = 0;

	// Fill in ofn struct

    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= GetFocus(); // Useful, avoids passing the HWND
	#pragma message ("varning: Used GetFocus.  HWMD with keyboard focus .. might not be us")
    ofn.lpstrFilter			= win_filter;
    ofn.lpstrCustomFilter	= NULL;
    ofn.nMaxCustFilter		= 0;
    ofn.nFilterIndex		= 0;
    ofn.lpstrFile			= file_selected;
    ofn.nMaxFile			= sizeof(file_selected);
    ofn.lpstrInitialDir		= startingfolder;
    ofn.lpstrFileTitle		= startingfolder; // Returned bare file without title according to what I see in API docs
    ofn.nMaxFileTitle		= sizeof(startingfolder);
    ofn.lpstrTitle			= title;
    ofn.lpstrDefExt			= NULL;  // This might apply to save files

    ofn.Flags				= OFN_FILEMUSTEXIST; // flags like OFN_HIDEREADONLY, etc.

    if (!GetOpenFileName ((LPOPENFILENAME)&ofn))
        return NULL;

    File_URL_Edit_SlashesForward_Like_Unix (file_selected);

	// Store last directory
	c_strlcpy (m_last_open_directory, file_selected);
	File_URL_Edit_Reduce_To_Parent_Path (m_last_open_directory);

	// Set last directory
	 return file_selected;
}



const char *_Shell_Dialog_Save_Type (const char *title, const char * starting_folder_url, const char *extensions_comma_delimited)
{

	return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: FILE MANAGER INTERACTION
///////////////////////////////////////////////////////////////////////////////

char *windows_style_url_alloc (const char *path_to_file)
{
	char *windows_style_url_o = strdup (path_to_file);
	File_URL_Edit_SlashesBack_Like_Windows (windows_style_url_o);
	return windows_style_url_o;
}
#include "Shellapi.h" // Never needed this before?
// Folder must exist.  It must be a folder.
cbool _Shell_Folder_Open_Folder_Must_Exist (const char *path_to_file)
{
	char *windows_style_url_a = windows_style_url_alloc (path_to_file);
	cbool ret = ShellExecute(0, "Open", "explorer.exe", windows_style_url_a, NULL, SW_NORMAL) != 0;
	free (windows_style_url_a);
	return ret;
}

// File must exist
cbool _Shell_Folder_Open_Highlight_URL_Must_Exist (const char *path_to_file)
{
	// Copy it, Windows format the slashes
	char *windows_style_url_a = windows_style_url_alloc (path_to_file);
	char *command_line_a = c_strdupf ("/select,%s", windows_style_url_a); // Construct the command line discarding first param "length"
	cbool ret =  ShellExecute(0, "Open", "explorer.exe", command_line_a, NULL, SW_NORMAL) != 0;

	logd ("Folder highlight: explorer.exe with " QUOTED_S, command_line_a);

	free (windows_style_url_a);
	free (command_line_a);
	return ret;
}


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: STARTUP/ICON
///////////////////////////////////////////////////////////////////////////////

#if 1 // DPI Awareness.

// Baker: From Quakespasm
typedef enum { dpi_unaware = 0, dpi_system_aware = 1, dpi_monitor_aware = 2 } dpi_awareness;
typedef BOOL (WINAPI *SetProcessDPIAwareFunc)();
typedef HRESULT (WINAPI *SetProcessDPIAwarenessFunc)(dpi_awareness value);

//static void System_SetDPIAware (void)
void Shell_Platform_Init_DPI (void) // Windows DPI awareness
{
	HMODULE hUser32, hShcore;
	SetProcessDPIAwarenessFunc setDPIAwareness;
	SetProcessDPIAwareFunc setDPIAware;

	/* Neither SDL 1.2 nor SDL 2.0.3 can handle the OS scaling our window.
	  (e.g. https://bugzilla.libsdl.org/show_bug.cgi?id=2713)
	  Call SetProcessDpiAwareness/SetProcessDPIAware to opt out of scaling.
	*/

	hShcore = LoadLibraryA ("Shcore.dll");
	hUser32 = LoadLibraryA ("user32.dll");
	setDPIAwareness = (SetProcessDPIAwarenessFunc) (hShcore ? GetProcAddress (hShcore, "SetProcessDpiAwareness") : NULL);
	setDPIAware = (SetProcessDPIAwareFunc) (hUser32 ? GetProcAddress (hUser32, "SetProcessDPIAware") : NULL);

	if (setDPIAwareness) /* Windows 8.1+ */
		setDPIAwareness (dpi_monitor_aware);
	else if (setDPIAware) /* Windows Vista-8.0 */
		setDPIAware ();

	if (hShcore)
		FreeLibrary (hShcore);
	if (hUser32)
		FreeLibrary (hUser32);
}

#include "Mmsystem.h" // Never needed this before
void Shell_Platform_Init_Timer_Sleep (void)
//void Sys_SetTimerResolution (void)
{
	/* Set OS timer resolution to 1ms.
	   Works around buffer underruns with directsound and SDL2, but also
	   will make Sleep()/SDL_Dleay() accurate to 1ms which should help framerate
	   stability.
	*/
	timeBeginPeriod (1);
}
#endif //


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: INPUT INTERACTIONS
///////////////////////////////////////////////////////////////////////////////


// ericw
void Shell_Input_ResetDeadKeys (void) // Windows
{
    /*
    if a deadkey has been typed, but not the next character (which the deadkey might modify),
    this tries to undo the effect pressing the deadkey.
    see: http://archives.miloush.net/michkap/archive/2006/09/10/748775.html
    */

    BYTE keyboardState[256];
    WCHAR buffer[16];
    int keycode, scancode, result, i;

    GetKeyboardState(keyboardState);

    keycode = VK_SPACE;
    scancode = MapVirtualKey(keycode, MAPVK_VK_TO_VSC);
    if (scancode == 0)
    {
        /* the keyboard doesn't have this key */
        return;
    }

    for (i = 0; i < 5; i++)
    {
        result = ToUnicode(keycode, scancode, keyboardState, (LPWSTR)buffer, 16, 0);
        if (result > 0)
        {
            /* success */
            return;
        }
    }
}


STICKYKEYS StartupStickyKeys = {sizeof (STICKYKEYS), 0};
TOGGLEKEYS StartupToggleKeys = {sizeof (TOGGLEKEYS), 0};
FILTERKEYS StartupFilterKeys = {sizeof (FILTERKEYS), 0};

// In the beginning, do it fast and loose with the idea of 1 Window.
// In practice we need this to be an option.
// And I can't quite remember the procedure to detect an application level loss of focus.
static void sWin_DisAllowAccessibilityShortcutKeys (cbool bDisAllowKeys)
{
	static cbool initialized = false;

	if (!initialized)
	{	// Save the current sticky/toggle/filter key settings so they can be restored them later
		SystemParametersInfo (SPI_GETSTICKYKEYS, sizeof (STICKYKEYS), &StartupStickyKeys, 0);
		SystemParametersInfo (SPI_GETTOGGLEKEYS, sizeof (TOGGLEKEYS), &StartupToggleKeys, 0);
		SystemParametersInfo (SPI_GETFILTERKEYS, sizeof (FILTERKEYS), &StartupFilterKeys, 0);
		log_debug ("Accessibility key startup settings saved");
		initialized = true;
	}

	if (!bDisAllowKeys)
	{
		// Restore StickyKeys/etc to original state
		// (note that this function is called "allow", not "enable"; if they were previously
		// disabled it will put them back that way too, it doesn't force them to be enabled.)
		SystemParametersInfo (SPI_SETSTICKYKEYS, sizeof (STICKYKEYS), &StartupStickyKeys, 0);
		SystemParametersInfo (SPI_SETTOGGLEKEYS, sizeof (TOGGLEKEYS), &StartupToggleKeys, 0);
		SystemParametersInfo (SPI_SETFILTERKEYS, sizeof (FILTERKEYS), &StartupFilterKeys, 0);

		log_debug ("Accessibility keys enabled");
	}
	else
	{
		// Disable StickyKeys/etc shortcuts but if the accessibility feature is on,
		// then leave the settings alone as its probably being usefully used
		STICKYKEYS skOff = StartupStickyKeys;
		TOGGLEKEYS tkOff = StartupToggleKeys;
		FILTERKEYS fkOff = StartupFilterKeys;

		if ((skOff.dwFlags & SKF_STICKYKEYSON) == 0)
		{
			// Disable the hotkey and the confirmation
			skOff.dwFlags &= ~SKF_HOTKEYACTIVE;
			skOff.dwFlags &= ~SKF_CONFIRMHOTKEY;

			SystemParametersInfo (SPI_SETSTICKYKEYS, sizeof (STICKYKEYS), &skOff, 0);
		}

		if ((tkOff.dwFlags & TKF_TOGGLEKEYSON) == 0)
		{
			// Disable the hotkey and the confirmation
			tkOff.dwFlags &= ~TKF_HOTKEYACTIVE;
			tkOff.dwFlags &= ~TKF_CONFIRMHOTKEY;

			SystemParametersInfo (SPI_SETTOGGLEKEYS, sizeof (TOGGLEKEYS), &tkOff, 0);
		}

		if ((fkOff.dwFlags & FKF_FILTERKEYSON) == 0)
		{
			// Disable the hotkey and the confirmation
			fkOff.dwFlags &= ~FKF_HOTKEYACTIVE;
			fkOff.dwFlags &= ~FKF_CONFIRMHOTKEY;

			SystemParametersInfo (SPI_SETFILTERKEYS, sizeof (FILTERKEYS), &fkOff, 0);
		}

		log_debug ("Accessibility keys disabled");
	}
}





static LRESULT CALLBACK LLWinKeyHook(int Code, WPARAM wParam, LPARAM lParam)
{
	PKBDLLHOOKSTRUCT p;
	p = (PKBDLLHOOKSTRUCT) lParam;

//	if (vid.ActiveApp) // In theory, this must always be true.  And if it isn't, it is an application problem.
	{
		switch(p->vkCode)
		{
		case VK_LWIN:	// Left Windows Key
		case VK_RWIN:	// Right Windows key
		case VK_APPS: 	// Context Menu key

			return 1; // Ignore these keys
		}
	}

//	p = p;
	return CallNextHookEx(NULL, Code, wParam, lParam);
}



static void sWin_Keyboard_Disable_Windows_Key (cbool bDisAllowKeys)
{
	static cbool WinKeyHook_isActive = false;
	static HHOOK WinKeyHook;

	if (bDisAllowKeys)
	{
		// Disable if not already disabled
		if (!WinKeyHook_isActive)
		{
			if (!(WinKeyHook = SetWindowsHookEx(13, LLWinKeyHook, GetModuleHandle(NULL), 0))) // GetModuleHandle(NULL) gets hinstance instead of sysplat.hInstance
			//if (!(WinKeyHook = SetWindowsHookEx(13, LLWinKeyHook, sysplat.hInstance, 0)))
			{
				log_debug ("Failed to install winkey hook.");
				log_debug ("Microsoft Windows NT 4.0, 2000 or XP is required.");
				return;
			}

			WinKeyHook_isActive = true;
			log_debug ("Windows and context menu key disabled");
		}
	}

	if (!bDisAllowKeys)
	{	// Keys allowed .. stop hook
		if (WinKeyHook_isActive)
		{
			UnhookWindowsHookEx(WinKeyHook);
			WinKeyHook_isActive = false;
			log_debug ("Windows and context menu key enabled");
		}
	}
}



// Considerations: Windows sticky keys, Windows key (full-screen only), Mac mouse acceleration (mouse, not keyboard though)
cbool Shell_Input_KeyBoard_Capture (cbool bDoCapture, cbool ActOnStickeyKeys, cbool bSuppressWindowskey)
{
	sWin_Keyboard_Disable_Windows_Key			(bDoCapture && bSuppressWindowskey);

	// Some situations we only consider the windows key.
	if (ActOnStickeyKeys)
		sWin_DisAllowAccessibilityShortcutKeys	(bDoCapture);

	return bDoCapture;
}



#ifndef CORE_SDL // MachineTime and Clipboard - we defer to SDL if CORE_SDL build

	///////////////////////////////////////////////////////////////////////////////
	//  PLATFORM: MACHINE TIME
	///////////////////////////////////////////////////////////////////////////////

	// Double self-initializes now
	double Platform_MachineTime (void) // no set metric except result is in seconds
	{
		static	__int64	startcount;
		static double	pfreq;
		static cbool	first = true;


		__int64		pcount;


		QueryPerformanceCounter ((LARGE_INTEGER *)&pcount);
		if (first)
		{

			__int64	freq;
			QueryPerformanceFrequency ((LARGE_INTEGER *)&freq);
			if (freq <= 0)
				log_fatal ("Hardware timer not available");

			pfreq = (double)freq;
			first = false;
			startcount = pcount;
			return 0.0;
		}

		// TODO: check for wrapping
		return (double)(pcount - startcount) / pfreq;
	}



	///////////////////////////////////////////////////////////////////////////////
	//  PLATFORM: CLIPBOARD HELPER FUNCTIONS FOR INTERFACE.C
	///////////////////////////////////////////////////////////////////////////////


	char *_Platform_Clipboard_Get_Text_Alloc (void)
	{
		char *text_out = NULL;

		if (OpenClipboard(NULL)) {
			HANDLE th = GetClipboardData(CF_TEXT);
			const char	*clipboard_text;

			if (th && (clipboard_text = GlobalLock(th))  ) {
				text_out = core_strdup (clipboard_text);
				GlobalUnlock (th);
			}

			CloseClipboard ();

		}
		return text_out;
	}


	// copies given text to clipboard.  Text can't be NULL
	cbool _Platform_Clipboard_Set_Text (const char *text_to_clipboard)
	{
		char *clipboard_text;
		HGLOBAL hglbCopy;
		size_t len = strlen(text_to_clipboard) + 1;

		if (!OpenClipboard(NULL))
			return false;

		if (!EmptyClipboard()) {
			CloseClipboard();
			return false;
		}

		if ((hglbCopy = GlobalAlloc(GMEM_DDESHARE, len + 1)) == 0) {
			CloseClipboard();
			return false;
		}

		if ((clipboard_text = GlobalLock(hglbCopy)) == 0) {
			CloseClipboard();
			return false;
		}

		strlcpy ((char *) clipboard_text, text_to_clipboard, len);
		GlobalUnlock(hglbCopy);
		SetClipboardData(CF_TEXT, hglbCopy);

		CloseClipboard();
		return true;
	}
#endif // !CORE_SDL -- MachineTime and Clipboard - we defer to SDL if CORE_SDL build

#ifdef CORE_SDL

	///////////////////////////////////////////////////////////////////////////////
	//  SHELL: PLATFORM
	///////////////////////////////////////////////////////////////////////////////

	//#include "core_sdl.h"
	#include <SDL2/SDL_syswm.h>
	static HICON gAppIcon;
	cbool Shell_Platform_Icon_Load (void *key /*wildcard*/)
	{
		if (!gAppIcon) {
			int key_int = (int)key; // Numeric interpretation for us!  101, 102, etc.
			HINSTANCE test = GetModuleHandle(NULL);
			gAppIcon = LoadIcon (GetModuleHandle(NULL), MAKEINTRESOURCE (key_int /*IDI_ICON1*/));
			// Don't destroy icon.  Right?
		}
		return !!gAppIcon;
	}

	cbool Shell_Platform_Icon_Window_Set (sys_handle_t cw)
	{
		SDL_SysWMinfo wminfo = {0};

		if (!gAppIcon)
			return false;

		if (SDL_GetWindowWMInfo((SDL_Window*) cw, &wminfo) != SDL_TRUE) {
			logd (SPRINTSFUNC "Couldn't get hwnd", __func__);
			return false;
		}
		else {
			HWND hWnd = wminfo.info.win.window;
			

			SetClassLong (hWnd /*hwnd*/, GCL_HICON, (LONG) gAppIcon); // This is the class icon, not the window icon. No effect.
			//SendMessage(hWnd, WM_SETICON, ICON_BIG,  (LPARAM) gAppIcon);
			//SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) gAppIcon);

#if 1 // Remove maximize button if found
			{	DWORD dwStyle = (DWORD)GetWindowLong(hWnd, GWL_STYLE);
				logd ("Checking for maximize button");
				if (dwStyle & WS_MAXIMIZEBOX) {
					logd ("Found maximize button");
					dwStyle &= (~WS_MAXIMIZEBOX);
					SetWindowLong (hWnd, GWL_STYLE, dwStyle);
					SetWindowPos (hWnd, 0, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOCOPYBITS);
					logd ("Removed maximize button");
				}
			}
#endif
		}
		return true;
	}

#endif // CORE_SDL


#endif // PLATFORM_WINDOWS


