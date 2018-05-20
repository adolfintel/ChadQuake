#ifndef CORE_SDL
#include "environment.h"
#ifdef PLATFORM_WINDOWS // Has to be here, set by a header


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

#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h> // Baker: Removes a warning
#include <time.h>

#include "quakedef.h"
#include "winquake.h"
#include "resource.h"
#include "dedicated_win.h"

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
void System_MakeCodeWriteable (unsigned long startaddr, unsigned long len)
{
	DWORD  flOldProtect;

	if (!VirtualProtect((LPVOID)startaddr, len, PAGE_READWRITE, &flOldProtect))
   		System_Error("Protection change failed");
}
#endif // id386




///////////////////////////////////////////////////////////////////////////////
//  SYSTEM ERROR: Baker
///////////////////////////////////////////////////////////////////////////////


int System_Error (const char *fmt, ...)
{
	static int	in_sys_error0 = 0;
	static int	in_sys_error1 = 0;
	static int	in_sys_error2 = 0;
	static int	in_sys_error3 = 0;

	VA_EXPAND (text, SYSTEM_STRING_SIZE_1024, fmt);

	if (!in_sys_error3) in_sys_error3 = 1;

	switch (isDedicated)
	{
	case true:
		ConProc_Error (text);
		break;

	case false:

#ifdef DIRECT3DX_WRAPPER // dx8 + dx9 - Temp maybe for Direct3D 9.  Possibly.  Relates to TOPMOST.  Must determine if we can detect scenario
		// Baker: Direct3D hides popups, although this assumes window is setup which might be bad assumption
		if (vid.initialized && vid.screen.type == MODE_FULLSCREEN)
		{
			VID_Shutdown ();
			Input_Shutdown ();
			System_Process_Messages_Sleep_100 ();
		}
#endif // DIRECT3DX_WRAPPER ... TopMost interferes with msgbox

		switch (in_sys_error0)
		{
		case true: // Recursive error, like occurred during shutdown
			MessageBox(NULL, text, "Double Quake Error", MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
			break;

		case false:
			in_sys_error0 = 1;
			Input_Shutdown ();
			MessageBox(NULL, text, "Quake Error", MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
			break;
		}
		break;
	} // End of dedicated vs. non-dedicated switch statement

	if (!in_sys_error1)
	{
		in_sys_error1 = 1;
		Host_Shutdown ();
	}

// shut down QHOST hooks if necessary
	if (!in_sys_error2)
	{
		in_sys_error2 = 1;
		DeinitConProc ();
	}

	exit (1);
#ifndef __GNUC__ // Return silence
	return 1; // No return as an attribute isn't universally available.
#endif // __GNUC__	// Make GCC not complain about return
}



///////////////////////////////////////////////////////////////////////////////
//  SYSTEM EVENTS
///////////////////////////////////////////////////////////////////////////////

// Baker: Used for Windows video mode switch
void System_Process_Messages_Sleep_100 (void)
{
	MSG				msg;
	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
	{
	    TranslateMessage (&msg);
	    DispatchMessage (&msg);
	}

	Sleep (100);
}

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

	if (sysplat.tevent)
	{
		CloseHandle (sysplat.tevent);
		sysplat.tevent = NULL;
	}

	if (isDedicated)
		FreeConsole ();

// shut down QHOST hooks if necessary
	DeinitConProc ();

	exit (0);
}



// Main_Central calls us
void System_Init (void)
{
	OSVERSIONINFO	vinfo;

	Shell_Platform_Init_DPI ();
	Shell_Platform_Init_Timer_Sleep ();

#if id386
	MaskExceptions ();
	Sys_SetFPCW ();
#endif // id386

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	if (!GetVersionEx (&vinfo))
		System_Error ("Couldn't get OS info");

	if ((vinfo.dwMajorVersion < 4) ||
		(vinfo.dwPlatformId == VER_PLATFORM_WIN32s))
	{
		System_Error ("WinQuake requires at least Win95 or NT 4.0");
	}
#pragma message ("Baker: I'm not sure we can run on Windows 98 any more or even Windows 2000")

#if defined(GLQUAKE_RENDERER_SUPPORT) && !defined (DIRECT3D_WRAPPER)
	// This is the "starting Quake" dialog which we abuse for multisample
	if (!isDedicated)
		VID_Local_Startup_Dialog ();
#endif // GLQUAKE_RENDERER_SUPPORT + ! DIRECT3D_WRAPPER

	if (!(sysplat.tevent = CreateEvent(NULL, FALSE, FALSE, NULL)))
		System_Error ("Couldn't create event");

	if (isDedicated)
		Dedicated_Init ();
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


int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	sysplat.hInstance = hInstance;

    return Main_Central (lpCmdLine, &sysplat.mainwindow, true /* perform loop */);
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

#if defined(_DEBUG) && defined(_MSC_VER)
static keyvalue_t wm_msgs_text [] = {
	KEYVALUE (WM_NULL                         ),
	KEYVALUE (WM_CREATE                       ),
	KEYVALUE (WM_DESTROY                      ),
	KEYVALUE (WM_MOVE                         ),
	KEYVALUE (WM_SIZE                         ),
	KEYVALUE (WM_ACTIVATE                     ),
	KEYVALUE (WM_SETFOCUS                     ),
	KEYVALUE (WM_KILLFOCUS                    ),
	KEYVALUE (WM_ENABLE                       ),
	KEYVALUE (WM_SETREDRAW                    ),
	KEYVALUE (WM_SETTEXT                      ),
	KEYVALUE (WM_GETTEXT                      ),
	KEYVALUE (WM_GETTEXTLENGTH                ),
	KEYVALUE (WM_PAINT                        ),
	KEYVALUE (WM_CLOSE                        ),
	KEYVALUE (WM_QUERYENDSESSION              ),
	KEYVALUE (WM_QUERYOPEN                    ),
	KEYVALUE (WM_ENDSESSION                   ),
	KEYVALUE (WM_QUIT                         ),
	KEYVALUE (WM_ERASEBKGND                   ),
	KEYVALUE (WM_SYSCOLORCHANGE               ),
	KEYVALUE (WM_SHOWWINDOW                   ),
	KEYVALUE (WM_WININICHANGE                 ),
	KEYVALUE (WM_SETTINGCHANGE                ),
	KEYVALUE (WM_DEVMODECHANGE                ),
	KEYVALUE (WM_ACTIVATEAPP                  ),
	KEYVALUE (WM_FONTCHANGE                   ),
	KEYVALUE (WM_TIMECHANGE                   ),
	KEYVALUE (WM_CANCELMODE                   ),
	KEYVALUE (WM_SETCURSOR                    ),
	KEYVALUE (WM_MOUSEACTIVATE                ),
	KEYVALUE (WM_CHILDACTIVATE                ),
	KEYVALUE (WM_QUEUESYNC                    ),
	KEYVALUE (WM_GETMINMAXINFO                ),
	KEYVALUE (WM_PAINTICON                    ),
	KEYVALUE (WM_ICONERASEBKGND               ),
	KEYVALUE (WM_NEXTDLGCTL                   ),
	KEYVALUE (WM_SPOOLERSTATUS                ),
	KEYVALUE (WM_DRAWITEM                     ),
	KEYVALUE (WM_MEASUREITEM                  ),
	KEYVALUE (WM_DELETEITEM                   ),
	KEYVALUE (WM_VKEYTOITEM                   ),
	KEYVALUE (WM_CHARTOITEM                   ),
	KEYVALUE (WM_SETFONT                      ),
	KEYVALUE (WM_GETFONT                      ),
	KEYVALUE (WM_SETHOTKEY                    ),
	KEYVALUE (WM_GETHOTKEY                    ),
	KEYVALUE (WM_QUERYDRAGICON                ),
	KEYVALUE (WM_COMPAREITEM                  ),
	KEYVALUE (WM_GETOBJECT                    ),
	KEYVALUE (WM_COMPACTING                   ),
	KEYVALUE (WM_COMMNOTIFY                   ),
	KEYVALUE (WM_WINDOWPOSCHANGING            ),
	KEYVALUE (WM_WINDOWPOSCHANGED             ),
	KEYVALUE (WM_POWER                        ),
	KEYVALUE (WM_COPYDATA                     ),
	KEYVALUE (WM_CANCELJOURNAL                ),
	KEYVALUE (WM_NOTIFY                       ),
	KEYVALUE (WM_INPUTLANGCHANGEREQUEST       ),
	KEYVALUE (WM_INPUTLANGCHANGE              ),
	KEYVALUE (WM_TCARD                        ),
	KEYVALUE (WM_HELP                         ),
	KEYVALUE (WM_USERCHANGED                  ),
	KEYVALUE (WM_NOTIFYFORMAT                 ),
	KEYVALUE (WM_CONTEXTMENU                  ),
	KEYVALUE (WM_STYLECHANGING                ),
	KEYVALUE (WM_STYLECHANGED                 ),
	KEYVALUE (WM_DISPLAYCHANGE                ),
	KEYVALUE (WM_GETICON                      ),
	KEYVALUE (WM_SETICON                      ),
	KEYVALUE (WM_NCCREATE                     ),
	KEYVALUE (WM_NCDESTROY                    ),
	KEYVALUE (WM_NCCALCSIZE                   ),
	KEYVALUE (WM_NCHITTEST                    ),
	KEYVALUE (WM_NCPAINT                      ),
	KEYVALUE (WM_NCACTIVATE                   ),
	KEYVALUE (WM_GETDLGCODE                   ),
	KEYVALUE (WM_SYNCPAINT                    ),
	KEYVALUE (WM_NCMOUSEMOVE                  ),
	KEYVALUE (WM_NCLBUTTONDOWN                ),
	KEYVALUE (WM_NCLBUTTONUP                  ),
	KEYVALUE (WM_NCLBUTTONDBLCLK              ),
	KEYVALUE (WM_NCRBUTTONDOWN                ),
	KEYVALUE (WM_NCRBUTTONUP                  ),
	KEYVALUE (WM_NCRBUTTONDBLCLK              ),
	KEYVALUE (WM_NCMBUTTONDOWN                ),
	KEYVALUE (WM_NCMBUTTONUP                  ),
	KEYVALUE (WM_NCMBUTTONDBLCLK              ),
#ifndef __VISUAL_STUDIO_6__
	KEYVALUE (WM_NCXBUTTONDOWN                ),//
	KEYVALUE (WM_NCXBUTTONUP                  ),//
	KEYVALUE (WM_NCXBUTTONDBLCLK              ),//
	KEYVALUE (WM_INPUT_DEVICE_CHANGE          ),//
	KEYVALUE (WM_INPUT                        ),//
#endif // __VISUAL_STUDIO_6__
	KEYVALUE (WM_KEYFIRST                     ),
	KEYVALUE (WM_KEYDOWN                      ),
	KEYVALUE (WM_KEYUP                        ),
	KEYVALUE (WM_CHAR                         ),
	KEYVALUE (WM_DEADCHAR                     ),
	KEYVALUE (WM_SYSKEYDOWN                   ),
	KEYVALUE (WM_SYSKEYUP                     ),
	KEYVALUE (WM_SYSCHAR                      ),
	KEYVALUE (WM_SYSDEADCHAR                  ),
#ifndef __VISUAL_STUDIO_6__
	KEYVALUE (WM_UNICHAR                      ),//
#endif // __VISUAL_STUDIO_6__
	KEYVALUE (WM_KEYLAST                      ),
	KEYVALUE (WM_KEYLAST                      ),
	KEYVALUE (WM_IME_STARTCOMPOSITION         ),
	KEYVALUE (WM_IME_ENDCOMPOSITION           ),
	KEYVALUE (WM_IME_COMPOSITION              ),
	KEYVALUE (WM_IME_KEYLAST                  ),
	KEYVALUE (WM_INITDIALOG                   ),
	KEYVALUE (WM_COMMAND                      ),
	KEYVALUE (WM_SYSCOMMAND                   ),
	KEYVALUE (WM_TIMER                        ),
	KEYVALUE (WM_HSCROLL                      ),
	KEYVALUE (WM_VSCROLL                      ),
	KEYVALUE (WM_INITMENU                     ),
	KEYVALUE (WM_INITMENUPOPUP                ),
	KEYVALUE (WM_MENUSELECT                   ),
	KEYVALUE (WM_MENUCHAR                     ),
	KEYVALUE (WM_ENTERIDLE                    ),
	KEYVALUE (WM_MENURBUTTONUP                ),
	KEYVALUE (WM_MENUDRAG                     ),
	KEYVALUE (WM_MENUGETOBJECT                ),
	KEYVALUE (WM_UNINITMENUPOPUP              ),
	KEYVALUE (WM_MENUCOMMAND                  ),
#ifndef __VISUAL_STUDIO_6__
	KEYVALUE (WM_CHANGEUISTATE                ),//
	KEYVALUE (WM_UPDATEUISTATE                ),//
	KEYVALUE (WM_QUERYUISTATE                 ),//
#endif // __VISUAL_STUDIO_6__
	KEYVALUE (WM_CTLCOLORMSGBOX               ),
	KEYVALUE (WM_CTLCOLOREDIT                 ),
	KEYVALUE (WM_CTLCOLORLISTBOX              ),
	KEYVALUE (WM_CTLCOLORBTN                  ),
	KEYVALUE (WM_CTLCOLORDLG                  ),
	KEYVALUE (WM_CTLCOLORSCROLLBAR            ),
	KEYVALUE (WM_CTLCOLORSTATIC               ),
	KEYVALUE (WM_MOUSEFIRST                   ),
	KEYVALUE (WM_MOUSEMOVE                    ),
	KEYVALUE (WM_LBUTTONDOWN                  ),
	KEYVALUE (WM_LBUTTONUP                    ),
	KEYVALUE (WM_LBUTTONDBLCLK                ),
	KEYVALUE (WM_RBUTTONDOWN                  ),
	KEYVALUE (WM_RBUTTONUP                    ),
	KEYVALUE (WM_RBUTTONDBLCLK                ),
	KEYVALUE (WM_MBUTTONDOWN                  ),
	KEYVALUE (WM_MBUTTONUP                    ),
	KEYVALUE (WM_MBUTTONDBLCLK                ),
	KEYVALUE (WM_MOUSEWHEEL                   ),
	KEYVALUE (WM_XBUTTONDOWN                  ),
	KEYVALUE (WM_XBUTTONUP                    ),
#ifndef __VISUAL_STUDIO_6__
	KEYVALUE (WM_XBUTTONDBLCLK                ),//
	KEYVALUE (WM_MOUSEHWHEEL                  ),// Horizontal?
#endif // __VISUAL_STUDIO_6__
	KEYVALUE (WM_MOUSELAST                    ),
	KEYVALUE (WM_MOUSELAST                    ),
	KEYVALUE (WM_MOUSELAST                    ),
	KEYVALUE (WM_MOUSELAST                    ),
	KEYVALUE (WM_PARENTNOTIFY                 ),
	KEYVALUE (WM_ENTERMENULOOP                ),
	KEYVALUE (WM_EXITMENULOOP                 ),
	KEYVALUE (WM_NEXTMENU                     ),
	KEYVALUE (WM_SIZING                       ),
	KEYVALUE (WM_CAPTURECHANGED               ),
	KEYVALUE (WM_MOVING                       ),
	KEYVALUE (WM_POWERBROADCAST               ),
	KEYVALUE (WM_DEVICECHANGE                 ),
	KEYVALUE (WM_MDICREATE                    ),
	KEYVALUE (WM_MDIDESTROY                   ),
	KEYVALUE (WM_MDIACTIVATE                  ),
	KEYVALUE (WM_MDIRESTORE                   ),
	KEYVALUE (WM_MDINEXT                      ),
	KEYVALUE (WM_MDIMAXIMIZE                  ),
	KEYVALUE (WM_MDITILE                      ),
	KEYVALUE (WM_MDICASCADE                   ),
	KEYVALUE (WM_MDIICONARRANGE               ),
	KEYVALUE (WM_MDIGETACTIVE                 ),
	KEYVALUE (WM_MDISETMENU                   ),
	KEYVALUE (WM_ENTERSIZEMOVE                ),
	KEYVALUE (WM_EXITSIZEMOVE                 ),
	KEYVALUE (WM_DROPFILES                    ),
	KEYVALUE (WM_MDIREFRESHMENU               ),
	KEYVALUE (WM_IME_SETCONTEXT               ),
	KEYVALUE (WM_IME_NOTIFY                   ),
	KEYVALUE (WM_IME_CONTROL                  ),
	KEYVALUE (WM_IME_COMPOSITIONFULL          ),
	KEYVALUE (WM_IME_SELECT                   ),
	KEYVALUE (WM_IME_CHAR                     ),
	KEYVALUE (WM_IME_REQUEST                  ),
	KEYVALUE (WM_IME_KEYDOWN                  ),
	KEYVALUE (WM_IME_KEYUP                    ),
	KEYVALUE (WM_MOUSEHOVER                   ),
	KEYVALUE (WM_MOUSELEAVE                   ),
	KEYVALUE (WM_NCMOUSEHOVER                 ),
	KEYVALUE (WM_NCMOUSELEAVE                 ),
#ifndef __VISUAL_STUDIO_6__
	KEYVALUE (WM_WTSSESSION_CHANGE            ),//
	KEYVALUE (WM_TABLET_FIRST                 ),//
	KEYVALUE (WM_TABLET_LAST                  ),//
#endif // __VISUAL_STUDIO_6__
	KEYVALUE (WM_CUT                          ),
	KEYVALUE (WM_COPY                         ),
	KEYVALUE (WM_PASTE                        ),
	KEYVALUE (WM_CLEAR                        ),
	KEYVALUE (WM_UNDO                         ),
	KEYVALUE (WM_RENDERFORMAT                 ),
	KEYVALUE (WM_RENDERALLFORMATS             ),
	KEYVALUE (WM_DESTROYCLIPBOARD             ),
	KEYVALUE (WM_DRAWCLIPBOARD                ),
	KEYVALUE (WM_PAINTCLIPBOARD               ),
	KEYVALUE (WM_VSCROLLCLIPBOARD             ),
	KEYVALUE (WM_SIZECLIPBOARD                ),
	KEYVALUE (WM_ASKCBFORMATNAME              ),
	KEYVALUE (WM_CHANGECBCHAIN                ),
	KEYVALUE (WM_HSCROLLCLIPBOARD             ),
	KEYVALUE (WM_QUERYNEWPALETTE              ),
	KEYVALUE (WM_PALETTEISCHANGING            ),
	KEYVALUE (WM_PALETTECHANGED               ),
	KEYVALUE (WM_HOTKEY                       ),
	KEYVALUE (WM_PRINT                        ),
	KEYVALUE (WM_PRINTCLIENT                  ),
#ifndef __VISUAL_STUDIO_6__
	KEYVALUE (WM_APPCOMMAND                   ),//
	KEYVALUE (WM_THEMECHANGED                 ),//
	KEYVALUE (WM_CLIPBOARDUPDATE              ),//
	KEYVALUE (WM_DWMCOMPOSITIONCHANGED        ),//
	KEYVALUE (WM_DWMNCRENDERINGCHANGED        ),//
	KEYVALUE (WM_DWMCOLORIZATIONCOLORCHANGED  ),//
	KEYVALUE (WM_DWMWINDOWMAXIMIZEDCHANGE     ),//
	KEYVALUE (WM_GETTITLEBARINFOEX            ),//
#endif
	KEYVALUE (WM_HANDHELDFIRST                ),
	KEYVALUE (WM_HANDHELDLAST                 ),
	KEYVALUE (WM_AFXFIRST                     ),
	KEYVALUE (WM_AFXLAST                      ),
	KEYVALUE (WM_PENWINFIRST                  ),
	KEYVALUE (WM_PENWINLAST                   ),
	KEYVALUE (WM_APP                          ),
	KEYVALUE (WM_USER                         ),
NULL, 0}; // Null term
#endif

LRESULT CALLBACK Session_Windows_Dispatch (
	HWND    hWnd,
	UINT    Msg,
	WPARAM  wParam,
	LPARAM  lParam)
{
	int fActive, fMinimized;

	// check for input messages
	if (WIN_IN_ReadInputMessages (hWnd, Msg, wParam, lParam)) return 0;

#if 0 // Baker: Message dump
	{
		const char *msgtext_ = KeyValue_GetKeyString (wm_msgs_text, Msg);
		const char *msgtext  = msgtext_ ? msgtext_ : va("Unknown message: %x", Msg);
		logd ("%s wparam: %x lparam %x", msgtext, wParam, lParam);
	}
#endif

    switch (Msg)
    {
	// events we want to discard
	case WM_CREATE:		return 0;
	case WM_ERASEBKGND: return 1; // MH: treachery!!! see your MSDN!
	case WM_SYSCHAR:	return 0;

	case WM_KILLFOCUS:
		// Baker: Plus this makes it survive a Windows firewall warning "better"
		if (vid.screen.type == MODE_FULLSCREEN)
			ShowWindow(sysplat.mainwindow, SW_SHOWMINNOACTIVE);
		break;

	case WM_SYSCOMMAND:
		switch (wParam & ~0x0F)
		{
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			// prevent from happening
			return 0;
		}
		break;

   	case WM_CLOSE:
		//if (MessageBox (sysplat.mainwindow, "Are you sure you want to quit?", "Confirm Exit", MB_YESNO | MB_SETFOREGROUND | MB_ICONQUESTION) == IDYES)
		System_Quit ();

	    return 0;

	case WM_ACTIVATE:
		fActive = LOWORD(wParam);
		fMinimized = (BOOL) HIWORD(wParam);
		VID_AppActivate(!(fActive == WA_INACTIVE), fMinimized, false);
#pragma message ("Baker: If we lost the context due a Windows firewall warming we might be able to rebuild it here")

		return 0;

   	case WM_DESTROY:
        PostQuitMessage (0);
        return 0;

#ifdef SUPPORTS_RESIZABLE_WINDOW  // Baker: Optional resizeable GL window start
	case WM_GETMINMAXINFO:	// Sent before size change; can be used to override default mins/maxs
		//if (host.isAltTabCapable) // If we aren't ALT-TAB capable, we cannot resize.

		if (sysplat.mainwindow)
		{
			MINMAXINFO *mmi = (MINMAXINFO *) lParam;

			mmi->ptMinTrackSize.x = QWIDTH_MINIMUM_320  + (int)vid.border_width;
			mmi->ptMinTrackSize.y = QHEIGHT_MINIMUM_2XX + (int)vid.border_height;
		}
		return 0;

	case WM_SIZE:

		logd ("WM_SIZE occurred");
		if (vid.Minimized || !vid.ActiveApp)
			return 0;

		if (wParam == SIZE_MINIMIZED)
			return 0;

		VID_Resize_Check (1);
		return 0;
#endif  // SUPPORTS_RESIZABLE_WINDOW // Baker: Optional resizable window


#ifdef SUPPORTS_MP3_MUSIC // Baker change
	case WM_GRAPHNOTIFY:
		return WIN_MediaPlayer_MessageHandler (hWnd, Msg, wParam, lParam);
#endif // Baker change +

	case MM_MCINOTIFY:
        return WIN_CDAudio_MessageHandler (hWnd, Msg, wParam, lParam);

#ifdef WINQUAKE_RENDERER_SUPPORT
	case WM_PAINT:
		// Baker: GL does not seem to need.  Makes me wonder if we are
		// constantly drawing the sbar or something in GL
		// In WinQuake, I need this if I keep moving a window over the
		// top, it won't paint certain areas if I am tricky about it
		// without this scr_fullupdate. GLQuake won't do it.
		winquake_scr_fullupdate = 0;
		break;
#endif // WINQUAKE_RENDERER_SUPPORT

    default:
		break;
    }

	// pass all unhandled messages to DefWindowProc
	return DefWindowProc (hWnd, Msg, wParam, lParam);
}


// Main_Central calls us.  But SDL does not currently use that
void System_SleepUntilInput (int time)
{
	MsgWaitForMultipleObjects(1, &sysplat.tevent, FALSE, time, QS_ALLINPUT);
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

//const char *Dedicated_ConsoleInput (void)
// But on windows we use a dedicated file (dedicated_win.c)
// Since Quake is not a console app on Windows


///////////////////////////////////////////////////////////////////////////////
//  VIDEO ROGUES
///////////////////////////////////////////////////////////////////////////////

void WIN_Vid_Init_Once_CreateClass (void)
{
	WNDCLASS		wc;
	sysplat.hIcon = LoadIcon (sysplat.hInstance, MAKEINTRESOURCE (IDI_ICON1));


	// Register the frame class
    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC) Session_Windows_Dispatch;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = sysplat.hInstance;
//  wc.hIcon         = sysplat.hIcon;
	wc.hIcon		 = ExtractIcon (sysplat.hInstance, File_Binary_URL(), 0);
    wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wc.hbrBackground = NULL;
    wc.lpszMenuName  = 0;  // We can change this later
    wc.lpszClassName = ENGINE_NAME;

    if (!RegisterClass (&wc) )
		System_Error ("Couldn't register window class");
}

#ifndef CORE_GL
	#define eEnumDisplaySettings EnumDisplaySettings // WinQuake
#endif // !CORE_GL ... WinQuake

vmode_t WIN_Vid_GetDesktopProperties (void)
{
// Baker: If vid isn't initialized, we haven't hooked up eEnumDisplaySettings yet.  Must use pure.
	DEVMODE	devmode;
	vmode_t desktop = {0};
	cbool result = vid.initialized ? eEnumDisplaySettings (NULL, ENUM_CURRENT_SETTINGS, &devmode) :
                                    EnumDisplaySettings (NULL, ENUM_CURRENT_SETTINGS, &devmode);

	if (!result) {
		System_Error ("VID_UpdateDesktopProperties: eEnumDisplaySettings failed");
		return desktop;
	}

	desktop.type		=	MODE_FULLSCREEN;
	desktop.width		=	devmode.dmPelsWidth;
	desktop.height		=	devmode.dmPelsHeight;
	desktop.bpp			=	devmode.dmBitsPerPel;

	return desktop;
}

void WIN_AdjustRectToCenterScreen (RECT *in_windowrect)
{
	vmode_t desktop = VID_Local_GetDesktopProperties ();		// This call is not affected by DPI.  It would want a DPI affected call.
	int nwidth  = in_windowrect->right - in_windowrect->left;	// But the window positioning is apparently affected by DPI
	int nheight = in_windowrect->bottom - in_windowrect->top;

	in_windowrect->left = 0 + (desktop.width - nwidth) / 2;
	in_windowrect->top =  0 + (desktop.height - nheight) / 2;
	in_windowrect->right = in_windowrect->left + nwidth;
	in_windowrect->bottom = in_windowrect->top + nheight;
}


#endif // PLATFORM_WINDOWS

#endif // ! CORE_SDL


