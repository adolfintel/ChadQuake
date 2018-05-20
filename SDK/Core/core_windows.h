/*
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
// core_windows.h - <windows.h> plus fix up defines


#ifndef __CORE_WINDOWS_H__
#define __CORE_WINDOWS_H__

#include <windows.h>
#include <windowsx.h> // What's this?  2 goofy macros GET_X_LPARAM and Y
// is windosx.h a poisonous file?  No, quite harmless.  It was something else.  Maybe opengl.  Dunno.
//#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
//#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))


#if !defined(__WINDOWS_VC6_BANDAGES__) && defined(_MSC_VER) && _MSC_VER < 1400
    #define __WINDOWS_VC6_BANDAGES__
    // Bandages to cover things that must require a service pack for Visual Studio 6 ..
    // Except Microsoft doesn't make the service packs available any more so we work around

	#ifdef IS_INTRESOURCE
		#define __WINDOWS_VC6_HAVE_PSDK_2003__
	#endif

    // GetFileAttributes ...
    #define INVALID_FILE_ATTRIBUTES ((DWORD)-1)

	#ifndef __WINDOWS_VC6_HAVE_PSDK_2003__
		// Baker: I am using whether or not this is defined to detect
		// Windows February 2003 PSDK installation.


		// Defined in 2003 February SDK winuser.h
		typedef struct tagWINDOWINFO
		{
		  DWORD cbSize;
		  RECT  rcWindow;
		  RECT  rcClient;
		  DWORD dwStyle;
		  DWORD dwExStyle;
		  DWORD dwWindowStatus;
		  UINT  cxWindowBorders;
		  UINT  cyWindowBorders;
		  ATOM  atomWindowType;
		  WORD  wCreatorVersion;
		} WINDOWINFO, *PWINDOWINFO, *LPWINDOWINFO;

		BOOL WINAPI GetWindowInfo(HWND hwnd, PWINDOWINFO pwi);

		//
		// Input ...
		//

		typedef ULONG ULONG_PTR;

		#define WH_KEYBOARD_LL 13
		#define LLKHF_UP			(KF_UP >> 8)

		#define MAPVK_VSC_TO_VK 1
		#define MAPVK_VK_TO_CHAR 2
		#define MAPVK_VSC_TO_VK_EX 3

//		#define WM_INITMENUPOPUP                0x0117  // defined in winuser.h  it should be defined

	#endif // ! __WINDOWS_VC6_HAVE_PSDK_2003__

	// MSVC6 ONLY -- Do not do for CodeBlocks/MinGW/GCC
	typedef struct
	{
		DWORD		vkCode;
		DWORD		scanCode;
		DWORD		flags;
		DWORD		time;
		ULONG_PTR dwExtraInfo;
	} *PKBDLLHOOKSTRUCT;

	#define WM_MOUSEWHEEL                   0x020A // winuser.h, unsure how not defined
#endif // ! __WINDOWS_VC6_BANDAGES__

// Vsync
typedef BOOL (APIENTRY * SETSWAPFUNC) (int);
typedef int (APIENTRY * GETSWAPFUNC) (void);

#ifndef MAPVK_VK_TO_VSC
    #define MAPVK_VK_TO_VSC 0
#endif // !MAPVK_VK_TO_VSC


#if !defined(__GCC_VC6_BANDAGES__) && defined(__GNUC__)
    #define __GCC_BANDAGES__

	// Empty at moment

#endif // ! __GCC_VC6_BANDAGES__


#if !defined(__WINDOWS_VC2008_BANDAGES__) && _MSC_VER && _MSC_VER == 1500 // Visual C++ 2008
	#define __WINDOWS_VC2008_BANDAGES__

	#define WM_MOUSEWHEEL                   0x020A // winuser.h, unsure how not defined

#endif // ! __WINDOWS_VC2008_BANDAGES__

// Conditionalish.





#ifndef MK_XBUTTON1 // Litmus test.  If you don't have this one, probably don't have the others.
	#define MK_XBUTTON1 					0x0020 // winuser.h, unsure how not defined
	#define MK_XBUTTON2 					0x0040 // winuser.h, unsure how not defined
	#define WM_XBUTTONDOWN                  0x020B // winuser.h, Needed for mouse4/mouse5 events
	#define WM_XBUTTONUP                    0x020C // winuser.h, Needed for mouse4/mouse5 events
#endif


#define WM_GRAPHNOTIFY  			WM_USER + 13

#define VK_XBUTTON1                 0x05	// Not "really" needed.  It's a missing VK but I can't see us ever using it.
#define VK_XBUTTON2                 0x06	// Because who needs a "virtual key" for a mouse button, we don't receive events that way for mouse.

#define VK_BROWSER_BACK             0xA6
#define VK_BROWSER_FORWARD			0xA7
#define VK_BROWSER_REFRESH			0xA8
#define VK_BROWSER_STOP				0xA9
#define VK_BROWSER_SEARCH			0xAA
#define VK_BROWSER_FAVORITES		0xAB
#define VK_BROWSER_HOME				0xAC
#define VK_VOLUME_MUTE				0xAD
#define VK_VOLUME_DOWN				0xAE
#define VK_VOLUME_UP				0xAF
#define VK_MEDIA_NEXT_TRACK			0xB0
#define VK_MEDIA_PREV_TRACK			0xB1
#define VK_MEDIA_STOP				0xB2
#define VK_MEDIA_PLAY_PAUSE			0xB3
#define VK_LAUNCH_MAIL				0xB4
#define VK_LAUNCH_MEDIA_SELECT		0xB5
#define VK_LAUNCH_APP1				0xB6
#define VK_LAUNCH_APP2				0xB7

#define VK_OEM_1					0xBA
#define VK_OEM_PLUS					0xBB
#define VK_OEM_COMMA				0xBC
#define VK_OEM_MINUS				0xBD
#define VK_OEM_PERIOD				0xBE
#define VK_OEM_2					0xBF
#define VK_OEM_3					0xC0

#define VK_OEM_4					0xDB
#define VK_OEM_5					0xDC
#define VK_OEM_6					0xDD
#define VK_OEM_7					0xDE
#define VK_OEM_8					0xDF

#define VK_OEM_102					0xE2

#define VK_PROCESSKEY				0xE5

#define VK_PACKET					0xE7

#define VK_ATTN						0xF6
#define VK_CRSEL					0xF7
#define VK_EXSEL					0xF8
#define VK_EREOF					0xF9
#define VK_PLAY						0xFA
#define VK_ZOOM						0xFB
#define VK_NONAME					0xFC
#define VK_PA1						0xFD
#define VK_OEM_CLEAR				0xFE


/* Determine if run from command line.
#if       _WIN32_WINNT < 0x0500
#pragma message ("Redef")
  #undef  _WIN32_WINNT
  #define _WIN32_WINNT   0x0500
#endif
#include <stdio.h>
#include <windows.h>
//#include "Wincon.h"

//int main(int argc, char *argv[])
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) // wWinMain is unicode, this is ANSI
{
    HWND consoleWnd = GetConsoleWindow();
    DWORD dwProcessId;
    int result = GetWindowThreadProcessId(consoleWnd, &dwProcessId);
	int is_own_console = result && GetCurrentProcessId()== dwProcessId ? 1 : 0;
    const char *text = is_own_console ? "I have my own console, press enter to exit." : "This console is not mine";
#ifdef _CONSOLE
	printf (text);
	getchar ();
#else
	// If it is a SUBSYSTEM:WINDOWS (i.e. not a console terminal app) it always say "This console is not mine" -- so it's useless.
	MessageBoxA (NULL, text, "Alert!", MB_OK);
#endif

    return 0;
}
*/

#endif // !__CORE_WINDOWS_H__

