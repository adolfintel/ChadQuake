/*
Copyright (C) 2013-2014 Baker

*/
// vidco_win.c -- vid

#include "environment.h"

#ifdef PLATFORM_GUI_WINDOWS // Not SDL build; not console build


#define CORE_LOCAL
#include "core.h"
#include "core_windows.h" // <windows.h>
#include "vidco.h" // Courtesy

#ifdef CORE_GL
	#include "core_opengl.h"

	#if defined(CORE_GL) && defined (PLATFORM_WINDOWS)
		LONG (WINAPI *eChangeDisplaySettings) (LPDEVMODE lpDevMode, DWORD dwflags);
		LONG (WINAPI *eEnumDisplaySettings) (LPCSTR lpszDeviceName, DWORD iModeNum, LPDEVMODE lpDevMode);

		HGLRC (WINAPI *ewglCreateContext) (HDC);
		BOOL  (WINAPI *ewglDeleteContext) (HGLRC);
		HGLRC (WINAPI *ewglGetCurrentContext) (VOID);
		HDC   (WINAPI *ewglGetCurrentDC) (VOID);
		PROC  (WINAPI *ewglGetProcAddress)(LPCSTR);
		BOOL  (WINAPI *ewglMakeCurrent) (HDC, HGLRC);
		BOOL  (WINAPI *eChoosePixelFormat) (HDC, CONST PIXELFORMATDESCRIPTOR *);
		BOOL  (WINAPI *eDescribePixelFormat) (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
		BOOL  (WINAPI *eSetPixelFormat) (HDC, int, CONST PIXELFORMATDESCRIPTOR *);
		BOOL  (WINAPI *eSwapBuffers) (HDC);
	#endif // CORE_GL + PLATFORM_WINDOWS
#else // ^^ CORE_GL
	#define eEnumDisplaySettings EnumDisplaySettings
	#define eChoosePixelFormat ChoosePixelFormat
	#define eDescribePixelFormat DescribePixelFormat
	#define eSetPixelFormat SetPixelFormat
#endif // !CORE_GL

static const char *WIN_STRPROP_MINW = "minw";
static const char *WIN_STRPROP_MINH = "minh";
static const char *WIN_STRPROP_MAXW = "maxw";
static const char *WIN_STRPROP_MAXH = "maxh";
static const char *WIN_STRPROP_TRASHED = "trashed";


static const char *WIN_CLASSNAME_CLASS1 = "Class1";

void Vidco_Local_InitOnce_Windows (void)
{

#if defined(CORE_GL) && defined (PLATFORM_WINDOWS) && !defined(DIRECT3DX_WRAPPER)
	// Even if a wrapper like DIRECT3D_WRAPPER is allowed to override these, we still set them here.
	ewglCreateContext       = wglCreateContext;
	ewglDeleteContext       = wglDeleteContext;
	ewglGetCurrentContext   = wglGetCurrentContext;
	ewglGetCurrentDC        = wglGetCurrentDC;
	ewglMakeCurrent         = wglMakeCurrent;
	ewglGetProcAddress		= wglGetProcAddress;

	eChoosePixelFormat		= ChoosePixelFormat;
	eDescribePixelFormat	= DescribePixelFormat;
	eSetPixelFormat         = SetPixelFormat;

	eChangeDisplaySettings  = ChangeDisplaySettings;
#endif // CORE_GL + PLATFORM_WINDOWS

}

//void Vid_InitOnce (void)
//{
//	static cbool done;
//
//	if (!done && !mainus.dispatch_fn) mainus.dispatch_fn = Session_Dispatch;
//	if (!done) {
//		HINSTANCE		hInst = GetModuleHandle(NULL); // Use hinst if supplied but really GetModuleHandle should always suffice
//		HICON			hIcon = ExtractIcon (hInst, File_Binary_URL(), 0); // Pull out of running .exe  instead of sysplat.hIcon
//		HCURSOR			hCursor = LoadCursor (NULL, IDC_ARROW);
//		// CS_HREDRAW and CS_VREDRAW force redraw of entire window when sized, avoids jerky 2 part resize on maximize.  It's awesome.
//		WNDCLASS		wc = {CS_HREDRAW | CS_VREDRAW, mainus.dispatch_fn, 0, 0, hInst, hIcon, hCursor, NULL, NULL, WIN_CLASSNAME_CLASS1};
//
//		if (!RegisterClass (&wc))
//			log_fatal ("Couldn't register window class");
//
//		done = true;
//	}
//}


static PIXELFORMATDESCRIPTOR *WIN_PFD_Fill (int colorbits, int depthbits, int stencilbits)
{
	static PIXELFORMATDESCRIPTOR pfd;
	memset (&pfd, 0, sizeof(pfd));

	pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = colorbits;  // 24-bit color depth
	pfd.cDepthBits = depthbits;  // 32-bit z-buffer
	pfd.cStencilBits = stencilbits; // 8-bit stencil buffer
	return &pfd;
}

void Vidco_WIN_SetupPixelFormat (HDC hDC, int colorbits, int depthbits, int stencilbits)
{
	PIXELFORMATDESCRIPTOR *ppfd = WIN_PFD_Fill (colorbits, depthbits, stencilbits), testpfd;
	int pixelformat = eChoosePixelFormat(hDC, ppfd);

	if (!pixelformat)
		log_fatal ("ChoosePixelFormat failed");

	eDescribePixelFormat(hDC, pixelformat, sizeof(testpfd), &testpfd);

    if (!eSetPixelFormat(hDC, pixelformat, ppfd))
        log_fatal ("SetPixelFormat failed");
}


int _Plat_Window_Style (wdostyle_e style)
{
	return _Shell_Window_Style (style); // Only SDL is this not the method.
}

int _Plat_Window_StyleEx (wdostyle_e style)
{
	return _Shell_Window_StyleEx (style); // Only SDL is this not the method.
}




cbool Vid_Display_Properties_Get (reply int *left, reply int *top, reply int *width, reply int *height, reply int *bpp)
{
	DEVMODE	devmode;

	if (!eEnumDisplaySettings (NULL, ENUM_CURRENT_SETTINGS, &devmode))
		log_fatal (SPRINTSFUNC "eEnumDisplaySettings failed", __func__);

	NOT_MISSING_ASSIGN(left, 0); // Better be zero!
	NOT_MISSING_ASSIGN(top, 0); // Better be zero!
	NOT_MISSING_ASSIGN(width, devmode.dmPelsWidth);
	NOT_MISSING_ASSIGN(height, devmode.dmPelsHeight);
	NOT_MISSING_ASSIGN(bpp, devmode.dmBitsPerPel);

	return true;
}


// Send -1 to start, it returns count.
int Vid_Display_Modes_Properties_Get (int n, reply int *width, reply int *height, reply int *bpp)
{
	int			ret;
	DEVMODE		devmode = {0};

	int modes_count = 0;	// Hardware modes start at 0

	while ( eEnumDisplaySettings (NULL /*device name*/, modes_count, &devmode)  )
		modes_count ++;

	if (n == -1)				ret = modes_count;		// -1 wants count
	else if (n >= modes_count)	ret = 0; 				// Hit last return 0
	else {						ret = 1;				// continue
		if (!eEnumDisplaySettings (NULL, n, &devmode))
			return log_fatal ("eEnumDisplaySettings failed on %d", n); // End

		NOT_MISSING_ASSIGN(width, devmode.dmPelsWidth);
		NOT_MISSING_ASSIGN(height, devmode.dmPelsHeight);
		NOT_MISSING_ASSIGN(bpp,	devmode.dmBitsPerPel);

		//alert ("Mode %d %d %d", width ? *width : -1, height ? *height : -1, bpp ? *bpp : -1);
	}
	return ret; // continue
}


void Vid_Handle_Client_RectRB_Get (sys_handle_t cw, reply int *left, reply int *top, reply int *width, reply int *height, reply int *right, reply int *bottom)
{
	WINDOWINFO windowinfo;
	windowinfo.cbSize = sizeof (WINDOWINFO);
	GetWindowInfo (cw, &windowinfo);	// client_area screen coordinates

	// Fill in top left, bottom, right, center
	NOT_MISSING_ASSIGN (left, windowinfo.rcClient.left);
	NOT_MISSING_ASSIGN (top, windowinfo.rcClient.top);
	NOT_MISSING_ASSIGN (width, windowinfo.rcClient.right - windowinfo.rcClient.left);
	NOT_MISSING_ASSIGN (height, windowinfo.rcClient.bottom - windowinfo.rcClient.top);
	NOT_MISSING_ASSIGN (right, windowinfo.rcClient.right);
	NOT_MISSING_ASSIGN (bottom, windowinfo.rcClient.bottom);
}

#ifdef CORE_GL

sys_handle_t Vid_Handle_Context_Get (void)
{
	return ewglGetCurrentContext();
}


void Vid_Handle_Context_Set (sys_handle_t cw, sys_handle_t draw_context, sys_handle_t gl_context)
{
#ifdef _DEBUG
	HGLRC hRC = ewglGetCurrentContext();
    HDC	  hDC = ewglGetCurrentDC();
//	if (hRC != gl_context)
		//alert ("Changeroo!");
#endif //
	ewglMakeCurrent (draw_context, gl_context); // Windows doesn't need the window handle
}

#endif // CORE_GL

void Vid_Handle_SwapBuffers (sys_handle_t cw, sys_handle_t draw_context, sys_handle_t gl_context)
{
	SwapBuffers (draw_context);  // Windows doesn't need the window handle
}


sys_handle_t *Vid_Handle_Destroy (sys_handle_t cw, required sys_handle_t *draw_context, required sys_handle_t *gl_context, cbool should_delete_context)
{
	// destroy = 1 = TEARDOWN_FULL else TEARDOWN_NO_DELETE_GL_CONTEXT (don't destroy the context or destroy window)


	RemoveProp (cw, WIN_STRPROP_MINW);
	RemoveProp (cw, WIN_STRPROP_MINH);
	RemoveProp (cw, WIN_STRPROP_MAXW);
	RemoveProp (cw, WIN_STRPROP_MAXH);
	RemoveProp (cw, WIN_STRPROP_TRASHED);

#ifdef CORE_GL
	if (*gl_context) {
		ewglMakeCurrent(NULL, NULL);

		if (should_delete_context) {
			ewglDeleteContext (*gl_context);
			*gl_context = NULL;
		}
	}
#endif // CORE_GL

	if (*draw_context) {
		ReleaseDC (cw, *draw_context);
		*draw_context = NULL;
	}




	DestroyWindow (cw);
	return NULL;
}


void Vid_Handle_MinSize (sys_handle_t cw, int width, int height)
{
	SetProp (cw, WIN_STRPROP_MINW, (void *) width);
	SetProp (cw, WIN_STRPROP_MINH, (void *) height);
}


void Vid_Handle_MaxSize (sys_handle_t cw, int width, int height)
{
	SetProp (cw, WIN_STRPROP_MAXW, (void *) width);
	SetProp (cw, WIN_STRPROP_MAXH, (void *) height);
}


sys_handle_t *Vid_Handle_Restore (void *obj_ptr, const char *caption, wdostyle_e style, required sys_handle_t *cw, required sys_handle_t *draw_context, required sys_handle_t *gl_context)
{
	//int plat_style		= _Plat_Window_Style   (style);
	//int plat_style_ex	= _Plat_Window_StyleEx (style);

	//sys_handle_t cw = CreateWindowEx (
	//	plat_style_ex,
	//	WIN_CLASSNAME_CLASS1,
	//	caption, plat_style,
	//	window_rect.left,
	//	window_rect.top,
	//	window_rect.width,
	//	window_rect.height,
	//	NULL /*parent*/,
	//	NULL /*menu*/,
	//	GetModuleHandle(NULL),
	//	NULL /*lparam for MDI*/
	//);

	//void * old = (void *)SetWindowLongPtr (cw, GWLP_USERDATA, (LONG_PTR)obj_ptr);  // Points to us.
	//sys_handle_t drawcontext = GetDC (cw); // Get handle
	//sys_handle_t glcontext = gl_context_out ? *gl_context_out : NULL;

	//Vidco_WIN_SetupPixelFormat (drawcontext, 24, 32, 8); // color bits, depthbits, stencil bits

	//// Re-Use context if possible
	//if (glcontext && !ewglMakeCurrent (drawcontext, glcontext) ) {
	//	// Tried to reuse existing context and it failed
	//	ewglDeleteContext (glcontext);

	//	glcontext = NULL;
	//	logd ("Context reuse failed.  Must reload textures.");
	//}

	//// No rendering context because first time or re-use failed above ...
	//if (!glcontext) {
	//	glcontext = ewglCreateContext (drawcontext);

	//	if (!glcontext)
	//		log_fatal ("ewglCreateContext failed");

	//	if (!ewglMakeCurrent (drawcontext, glcontext) )
	//		log_fatal ("ewglMakeCurrent failed");
	//}

	//NOT_MISSING_ASSIGN (draw_context_out, drawcontext);
	//NOT_MISSING_ASSIGN (gl_context_out, glcontext);

	return cw;
}


sys_handle_t *Vid_Handle_Create (void *obj_ptr, const char *caption, crect_t window_rect, wdostyle_e style, cbool havemenu, required sys_handle_t *draw_context_out, required sys_handle_t *gl_context_out)
{
	int plat_style		= _Plat_Window_Style   (style);
	int plat_style_ex	= _Plat_Window_StyleEx (style);

	sys_handle_t cw = CreateWindowEx (
		plat_style_ex,
		WIN_CLASSNAME_CLASS1,
		caption, plat_style,
		window_rect.left,
		window_rect.top,
		window_rect.width,
		window_rect.height,
		NULL /*parent*/,
		NULL /*menu*/,
		GetModuleHandle(NULL),
		NULL /*lparam for MDI*/
	);

	void * old = (void *)SetWindowLongPtr (cw, GWLP_USERDATA, (LONG_PTR)obj_ptr);  // Points to us.
	sys_handle_t drawcontext = GetDC (cw); // Get handle
	sys_handle_t glcontext = gl_context_out ? *gl_context_out : NULL;

	Vidco_WIN_SetupPixelFormat (drawcontext, BPP_24, /*depth ->*/ 24, /*stencil*/ 8); // color bits, depthbits, stencil bits

#ifdef CORE_GL
	// Re-Use context if possible
	if (glcontext && !ewglMakeCurrent (drawcontext, glcontext) ) {
		// Tried to reuse existing context and it failed
		ewglDeleteContext (glcontext);

		glcontext = NULL;
		logd ("Context reuse failed.  Must reload textures.");
	}

	// No rendering context because first time or re-use failed above ...
	if (!glcontext) {
		glcontext = ewglCreateContext (drawcontext);

		if (!glcontext)
			log_fatal ("ewglCreateContext failed");

		if (!ewglMakeCurrent (drawcontext, glcontext) )
			log_fatal ("ewglMakeCurrent failed");
	}
#endif // CORE_GL

	NOT_MISSING_ASSIGN (draw_context_out, drawcontext);
	NOT_MISSING_ASSIGN (gl_context_out, glcontext);

	return cw;
}


void Vid_Handle_Move (sys_handle_t cw, wdostyle_e style, cbool have_menu, int left, int top, int width, int height)
{
	Vid_Handle_Client_Rect_To_Window_Rect (style, have_menu, &left, &top, &width, &height);

	MoveWindow(cw, left, top, width, height, 1 /*repaint*/);
}


void Vid_Handle_Hide (sys_handle_t cw)
{
	ShowWindow (cw, SW_HIDE);
}


void Vid_Handle_Show (sys_handle_t cw)
{

	ShowWindow (cw, SW_SHOWNORMAL);// ShowWindow: (handle, cmd) ... SW_HIDE (0), SW_MAXIMIZE, SW_MINIMIZE, SW_SHOW, SW_SHOWDEFAULT, SW_SHOWNORMAL (<--- supposedly what this should be)// SW_SHOWDEFAULT); // Sets the specified window's show state. Minimized, maximized, etc.
	UpdateWindow (cw); // Sends WM_PAINT message

//	SetForegroundWindow (cw); // Bring to foreground
}


void Vid_Handle_ZOrder (sys_handle_t cw)
{
// I don't know if both of these are required.
	SetWindowPos (cw, HWND_TOP, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOCOPYBITS); // Put us on top  SWP_DEFERERASE |
	SetForegroundWindow (cw); // Bring to foreground
}


void _Vid_Handle_Caption (sys_handle_t cw, const char *text)
{
	SetWindowText (cw, text);
}


static LPCTSTR W_For_CursorEnum (mousepointer_e mousepointer)
{
	typedef struct tt  {
		int i;
		LPCTSTR name;
	} tt_t;

	static cbool init;
	static struct tt table[MAX_MOUSEPOINTERS] = {
		{ mousepointer_invalid,				IDC_ARROW		},		// Invalid
		{ mousepointer_arrow,				IDC_ARROW		},		// Default
		{ mousepointer_crosshair,			IDC_CROSS		},		// Think image editor
		{ mousepointer_hand,				IDC_HAND		},		// Button or action, in theory, in practice that's an arrow
		{ mousepointer_help,				IDC_HELP		},		// Help.  I've never seen this decently used.
		{ mousepointer_hourglass,			IDC_WAIT		},		// Hourglass
		{ mousepointer_move,				IDC_SIZEALL		},		// Move something
		{ mousepointer_prohibited,			IDC_NO			},		// Move something
		{ mousepointer_size_up,				IDC_SIZENS		},		// Size north south
		{ mousepointer_size_left,			IDC_SIZEWE		},		// Size east west
		{ mousepointer_size_both,			IDC_SIZENWSE	},		// Size diagonal
		{ mousepointer_text,				IDC_IBEAM		},		// Text
	};

	if (! in_range(0, mousepointer, MAX_MOUSEPOINTERS - 1) )
		log_fatal ("Invalid mousepointer");
	return table[mousepointer].name;
}


void Vid_Handle_MousePointer (sys_handle_t cw, sys_handle_t *hmousepointer, mousepointer_e mousepointer)
{
	*hmousepointer = LoadCursor (NULL, W_For_CursorEnum(mousepointer) );

	//SetCursor (hicon);
	SetClassLong(cw, GCL_HCURSOR, (DWORD)*hmousepointer);
}



///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: MSGBOX HELPER
///////////////////////////////////////////////////////////////////////////////

// Title cannot be NULL, System_MessageBox checks for that.
int _Platform_MessageBox (const char *title, const char *text)
{
	MessageBox (NULL, text, title , MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
	return 0;
}

/* See inputbox_alloc in msgbox_win.c */

///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: EVENTS
///////////////////////////////////////////////////////////////////////////////


void Platform_Events_SleepForInput (required sys_handle_t *phandle_tevent, double max_wait_seconds)
{
	MsgWaitForMultipleObjects (1, phandle_tevent, false, (int)(max_wait_seconds * 1000) /* converting up to milliseconds */, QS_ALLINPUT);
}


cbool Platform_Events_Do (void *dispatcher_function_unused)
{
// A command line utility doesn't use the message system
	MSG msg;

	if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE) != 0)
	{
		if (msg.message == WM_QUIT) {
			return false; // Terminate!
		}

		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	return true;
}




///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: DISPATCH.  AT LEAST THE DEFAULT ONE.
///////////////////////////////////////////////////////////////////////////////


int keymap [KEYMAP_COUNT_512][5] = {
// HOLD: 'V' - If NOT extended, translate to the next value.  Only keypad 5 has this issue.  It could be this is an always translation.
// 'E' - If extended, tranlate to the next value.
// 'N' - Do not emit a key press.  This is specific for the numpad keys and our choice for how we decided to translate the keys.
//                                 The short version is we use scancodes 128-255 for things like "HOME"/"END" since it is invalid unicode.
// TODO: Convert Microsoft UTF-16 to UTF-32 or UTF8.  TODO: Unicode!
    { 0,    0,                               0,                    0, 0,               /* vk hex = 0x0 */  },
    { 1,    VK_LBUTTON,                      K_MOUSE1,             0, 0,               /* vk hex = 0x1 */  },
    { 2,    VK_RBUTTON,                      K_MOUSE2,             0, 0,               /* vk hex = 0x2 */  },
    { 3,    VK_CANCEL,                       0,                    0, 0,               /* vk hex = 0x3 */  },
    { 4,    VK_MBUTTON,                      K_MOUSE3,             0, 0,               /* vk hex = 0x4 */  },
    { 5,    VK_XBUTTON1,                     K_MOUSE4,             0, 0,               /* vk hex = 0x5 */  },
    { 6,    VK_XBUTTON2,                     K_MOUSE5,             0, 0,               /* vk hex = 0x6 */  },
    { 7,    0,                               0,                    0, 0,               /* vk hex = 0x7 */  },
    { 8,    VK_BACK,                         K_BACKSPACE,          0, 0,               /* vk hex = 0x8 */  },
    { 9,    VK_TAB,                          K_TAB,                0, 0,               /* vk hex = 0x9 */  },
    { 10,   0,                               0,                    0, 0,               /* vk hex = 0xA */  },
    { 11,   0,                               0,                    0, 0,               /* vk hex = 0xB */  },
    { 12,   VK_CLEAR,                        K_NUMPAD_5, /* 'V' */ 0, K_NUMPAD_5,      /* vk hex = 0xC */  },
    { 13,   VK_RETURN,                       K_ENTER,            'E', K_NUMPAD_ENTER,  /* vk hex = 0xD */  },   // ENTER is not a unique keyboard key (numpad)
    { 14,   0,                               0,                    0, 0,               /* vk hex = 0xE */  },
    { 15,   0,                               0,                    0, 0,               /* vk hex = 0xF */  },
    { 16,   VK_SHIFT,                        0 /*K_SHIFT*/,      'R', 0,               /* vk hex = 0x10 */ },     // SHIFT is not a unique keyboard key.
    { 17,   VK_CONTROL,                      0 /*K_CTRL*/,       'R', 0,               /* vk hex = 0x11 */ },   // CTRL is not a unique keyboard key.
    { 18,   VK_MENU,                         0 /*K_ALT*/,        'R', 0,               /* vk hex = 0x12 */ },   // ALT is not a unique keyboard key.
    { 19,   VK_PAUSE,                        K_PAUSE,              0, 0,               /* vk hex = 0x13 */ },
    { 20,   VK_CAPITAL,                      K_CAPSLOCK,           0, 0,               /* vk hex = 0x14 */ },
    { 21,   VK_KANA,                         0,                    0, 0,               /* vk hex = 0x15 */ },
    { 22,   0,                               0,                    0, 0,               /* vk hex = 0x16 */ },
    { 23,   VK_JUNJA,                        0,                    0, 0,               /* vk hex = 0x17 */ },
    { 24,   VK_FINAL,                        0,                    0, 0,               /* vk hex = 0x18 */ },
    { 25,   VK_HANJA,                        0,                    0, 0,               /* vk hex = 0x19 */ },
    { 26,   0,                               0,                    0, 0,               /* vk hex = 0x1A */ },
    { 27,   VK_ESCAPE,                       K_ESCAPE,             0, 0,               /* vk hex = 0x1B */ },
    { 28,   VK_CONVERT,                      0,                    0, 0,               /* vk hex = 0x1C */ },
    { 29,   VK_NONCONVERT,                   0,                    0, 0,               /* vk hex = 0x1D */ },
    { 30,   VK_ACCEPT,                       0,                    0, 0,               /* vk hex = 0x1E */ },
    { 31,   VK_MODECHANGE,                   0,                    0, 0,               /* vk hex = 0x1F */ },
    { 32,   VK_SPACE,                        K_SPACE,              0, 0,               /* vk hex = 0x20 */ },
    { 33,   VK_PRIOR,                        K_PAGEUP,           'L', K_NUMPAD_9,      /* vk hex = 0x21 */ },
    { 34,   VK_NEXT,                         K_PAGEDOWN,         'L', K_NUMPAD_3,      /* vk hex = 0x22 */ },
    { 35,   VK_END,                          K_END,              'L', K_NUMPAD_1,      /* vk hex = 0x23 */ },
    { 36,   VK_HOME,                         K_HOME,             'L', K_NUMPAD_7,      /* vk hex = 0x24 */ },
    { 37,   VK_LEFT,						 K_LEFTARROW,        'L', K_NUMPAD_4,      /* vk hex = 0x25 */ },
    { 38,   VK_UP,							 K_UPARROW,          'L', K_NUMPAD_8,      /* vk hex = 0x26 */ },
    { 39,   VK_RIGHT,						 K_RIGHTARROW,       'L', K_NUMPAD_6,      /* vk hex = 0x27 */ },
    { 40,   VK_DOWN,						 K_DOWNARROW,        'L', K_NUMPAD_2,      /* vk hex = 0x28 */ },
    { 41,   VK_SELECT,                       0,                    0, 0,               /* vk hex = 0x29 */ },
    { 42,   VK_PRINT,                        0,                    0, 0,               /* vk hex = 0x2A */ },
    { 43,   VK_EXECUTE,                      0,                    0, 0,               /* vk hex = 0x2B */ },
    { 44,   VK_SNAPSHOT,                     K_PRINTSCREEN,        0, 0,               /* vk hex = 0x2C */ },
    { 45,   VK_INSERT,                       K_INSERT,           'L', K_NUMPAD_0,      /* vk hex = 0x2D */ },
    { 46,   VK_DELETE,                       K_DELETE,           'L', K_NUMPAD_PERIOD, /* vk hex = 0x2E */ },
    { 47,   VK_HELP,                         0,                    0, 0,               /* vk hex = 0x2F */ },
    { 48,   0/*VK_0*/,                       '0',                  0, 0,               /* vk hex = 0x30 */ },
    { 49,   0/*VK_1*/,                       '1',                  0, 0,               /* vk hex = 0x31 */ },
    { 50,   0/*VK_2*/,                       '2',                  0, 0,               /* vk hex = 0x32 */ },
    { 51,   0/*VK_3*/,                       '3',                  0, 0,               /* vk hex = 0x33 */ },
    { 52,   0/*VK_4*/,                       '4',                  0, 0,               /* vk hex = 0x34 */ },
    { 53,   0/*VK_5*/,                       '5',                  0, 0,               /* vk hex = 0x35 */ },
    { 54,   0/*VK_6*/,                       '6',                  0, 0,               /* vk hex = 0x36 */ },
    { 55,   0/*VK_7*/,                       '7',                  0, 0,               /* vk hex = 0x37 */ },
    { 56,   0/*VK_8*/,                       '8',                  0, 0,               /* vk hex = 0x38 */ },
    { 57,   0/*VK_9*/,                       '9',                  0, 0,               /* vk hex = 0x39 */ },
    { 58,   0,                               0,                    0, 0,               /* vk hex = 0x3A */ },
    { 59,   0,                               0,                    0, 0,               /* vk hex = 0x3B */ },
    { 60,   0,                               0,                    0, 0,               /* vk hex = 0x3C */ },
    { 61,   0,                               0,                    0, 0,               /* vk hex = 0x3D */ },
    { 62,   0,                               0,                    0, 0,               /* vk hex = 0x3E */ },
    { 63,   0,                               0,                    0, 0,               /* vk hex = 0x3F */ },
    { 64,   0,                               0,                    0, 0,               /* vk hex = 0x40 */ },
    { 65,   0/*VK_A*/,                       'A',                  0, 0,               /* vk hex = 0x41 */ },
    { 66,   0/*VK_B*/,                       'B',                  0, 0,               /* vk hex = 0x42 */ },
    { 67,   0/*VK_C*/,                       'C',                  0, 0,               /* vk hex = 0x43 */ },
    { 68,   0/*VK_D*/,                       'D',                  0, 0,               /* vk hex = 0x44 */ },
    { 69,   0/*VK_E*/,                       'E',                  0, 0,               /* vk hex = 0x45 */ },
    { 70,   0/*VK_F*/,                       'F',                  0, 0,               /* vk hex = 0x46 */ },
    { 71,   0/*VK_G*/,                       'G',                  0, 0,               /* vk hex = 0x47 */ },
    { 72,   0/*VK_H*/,                       'H',                  0, 0,               /* vk hex = 0x48 */ },
    { 73,   0/*VK_I*/,                       'I',                  0, 0,               /* vk hex = 0x49 */ },
    { 74,   0/*VK_J*/,                       'J',                  0, 0,               /* vk hex = 0x4A */ },
    { 75,   0/*VK_K*/,                       'K',                  0, 0,               /* vk hex = 0x4B */ },
    { 76,   0/*VK_L*/,                       'L',                  0, 0,               /* vk hex = 0x4C */ },
    { 77,   0/*VK_M*/,                       'M',                  0, 0,               /* vk hex = 0x4D */ },
    { 78,   0/*VK_N*/,                       'N',                  0, 0,               /* vk hex = 0x4E */ },
    { 79,   0/*VK_O*/,                       'O',                  0, 0,               /* vk hex = 0x4F */ },
    { 80,   0/*VK_P*/,                       'P',                  0, 0,               /* vk hex = 0x50 */ },
    { 81,   0/*VK_Q*/,                       'Q',                  0, 0,               /* vk hex = 0x51 */ },
    { 82,   0/*VK_R*/,                       'R',                  0, 0,               /* vk hex = 0x52 */ },
    { 83,   0/*VK_S*/,                       'S',                  0, 0,               /* vk hex = 0x53 */ },
    { 84,   0/*VK_T*/,                       'T',                  0, 0,               /* vk hex = 0x54 */ },
    { 85,   0/*VK_U*/,                       'U',                  0, 0,               /* vk hex = 0x55 */ },
    { 86,   0/*VK_V*/,                       'V',                  0, 0,               /* vk hex = 0x56 */ },
    { 87,   0/*VK_W*/,                       'W',                  0, 0,               /* vk hex = 0x57 */ },
    { 88,   0/*VK_X*/,                       'X',                  0, 0,               /* vk hex = 0x58 */ },
    { 89,   0/*VK_Y*/,                       'Y',                  0, 0,               /* vk hex = 0x59 */ },
    { 90,   0/*VK_Z*/,                       'Z',                  0, 0,               /* vk hex = 0x5A */ },
    { 91,   VK_LWIN,                         K_LWIN,               0, 0,               /* vk hex = 0x5B */ },
    { 92,   VK_RWIN,                         K_RWIN,               0, 0,               /* vk hex = 0x5C */ },
    { 93,   VK_APPS,                         K_MENU,               0, 0,               /* vk hex = 0x5D */ },
    { 94,   0,                               0,                    0, 0,               /* vk hex = 0x5E */ },
    { 95,   0,                               0,                    0, 0,               /* vk hex = 0x5F */ },
    { 96,   VK_NUMPAD0,                      K_NUMPAD_0,         'N', 0,               /* vk hex = 0x60 */ },
    { 97,   VK_NUMPAD1,                      K_NUMPAD_1,         'N', 0,               /* vk hex = 0x61 */ },
    { 98,   VK_NUMPAD2,                      K_NUMPAD_2,         'N', 0,               /* vk hex = 0x62 */ },
    { 99,   VK_NUMPAD3,                      K_NUMPAD_3,         'N', 0,               /* vk hex = 0x63 */ },
    { 100,  VK_NUMPAD4,                      K_NUMPAD_4,         'N', 0,               /* vk hex = 0x64 */ },
    { 101,  VK_NUMPAD5,                      K_NUMPAD_5,         'N', 0,               /* vk hex = 0x65 */ },
    { 102,  VK_NUMPAD6,                      K_NUMPAD_6,         'N', 0,               /* vk hex = 0x66 */ },
    { 103,  VK_NUMPAD7,                      K_NUMPAD_7,         'N', 0,               /* vk hex = 0x67 */ },
    { 104,  VK_NUMPAD8,                      K_NUMPAD_8,         'N', 0,               /* vk hex = 0x68 */ },
    { 105,  VK_NUMPAD9,                      K_NUMPAD_9,         'N', 0,               /* vk hex = 0x69 */ },
    { 106,  VK_MULTIPLY,                     K_NUMPAD_MULTIPLY,  'N', 0,               /* vk hex = 0x6A */ },
    { 107,  VK_ADD,                          K_NUMPAD_PLUS,      'N', 0,               /* vk hex = 0x6B */ },
    { 108,  VK_SEPARATOR,                    K_NUMPAD_SEPARATOR, 'N', 0,               /* vk hex = 0x6C */ },
    { 109,  VK_SUBTRACT,                     K_NUMPAD_MINUS,     'N', 0,               /* vk hex = 0x6D */ },
    { 110,  VK_DECIMAL,                      K_NUMPAD_PERIOD,    'N', 0,               /* vk hex = 0x6E */ },
    { 111,  VK_DIVIDE,                       K_NUMPAD_DIVIDE,    'N', 0,               /* vk hex = 0x6F */ },
    { 112,  VK_F1,                           K_F1,                 0, 0,               /* vk hex = 0x70 */ },
    { 113,  VK_F2,                           K_F2,                 0, 0,               /* vk hex = 0x71 */ },
    { 114,  VK_F3,                           K_F3,                 0, 0,               /* vk hex = 0x72 */ },
    { 115,  VK_F4,                           K_F4,                 0, 0,               /* vk hex = 0x73 */ },
    { 116,  VK_F5,                           K_F5,                 0, 0,               /* vk hex = 0x74 */ },
    { 117,  VK_F6,                           K_F6,                 0, 0,               /* vk hex = 0x75 */ },
    { 118,  VK_F7,                           K_F7,                 0, 0,               /* vk hex = 0x76 */ },
    { 119,  VK_F8,                           K_F8,                 0, 0,               /* vk hex = 0x77 */ },
    { 120,  VK_F9,                           K_F9,                 0, 0,               /* vk hex = 0x78 */ },
    { 121,  VK_F10,                          K_F10,                0, 0,               /* vk hex = 0x79 */ },
    { 122,  VK_F11,                          K_F11,                0, 0,               /* vk hex = 0x7A */ },
    { 123,  VK_F12,                          K_F12,                0, 0,               /* vk hex = 0x7B */ },
    { 124,  VK_F13,                          0,                    0, 0,               /* vk hex = 0x7C */ },
    { 125,  VK_F14,                          0,                    0, 0,               /* vk hex = 0x7D */ },
    { 126,  VK_F15,                          0,                    0, 0,               /* vk hex = 0x7E */ },
    { 127,  VK_F16,                          0,                    0, 0,               /* vk hex = 0x7F */ },
    { 128,  VK_F17,                          0,                    0, 0,               /* vk hex = 0x80 */ },
    { 129,  VK_F18,                          0,                    0, 0,               /* vk hex = 0x81 */ },
    { 130,  VK_F19,                          0,                    0, 0,               /* vk hex = 0x82 */ },
    { 131,  VK_F20,                          0,                    0, 0,               /* vk hex = 0x83 */ },
    { 132,  VK_F21,                          0,                    0, 0,               /* vk hex = 0x84 */ },
    { 133,  VK_F22,                          0,                    0, 0,               /* vk hex = 0x85 */ },
    { 134,  VK_F23,                          0,                    0, 0,               /* vk hex = 0x86 */ },
    { 135,  VK_F24,                          0,                    0, 0,               /* vk hex = 0x87 */ },
    { 136,  0,                               0,                    0, 0,               /* vk hex = 0x88 */ },
    { 137,  0,                               0,                    0, 0,               /* vk hex = 0x89 */ },
    { 138,  0,                               0,                    0, 0,               /* vk hex = 0x8A */ },
    { 139,  0,                               0,                    0, 0,               /* vk hex = 0x8B */ },
    { 140,  0,                               0,                    0, 0,               /* vk hex = 0x8C */ },
    { 141,  0,                               0,                    0, 0,               /* vk hex = 0x8D */ },
    { 142,  0,                               0,                    0, 0,               /* vk hex = 0x8E */ },
    { 143,  0,                               0,                    0, 0,               /* vk hex = 0x8F */ },
    { 144,  VK_NUMLOCK,                      K_NUMLOCK,            0, 0,               /* vk hex = 0x90 */ },
    { 145,  VK_SCROLL,                       K_SCROLLLOCK,         0, 0,               /* vk hex = 0x91 */ },
    { 146,  0,                               0,                    0, 0,               /* vk hex = 0x92 */ },
    { 147,  0,                               0,                    0, 0,               /* vk hex = 0x93 */ },
    { 148,  0,                               0,                    0, 0,               /* vk hex = 0x94 */ },
    { 149,  0,                               0,                    0, 0,               /* vk hex = 0x95 */ },
    { 150,  0,                               0,                    0, 0,               /* vk hex = 0x96 */ },
    { 151,  0,                               0,                    0, 0,               /* vk hex = 0x97 */ },
    { 152,  0,                               0,                    0, 0,               /* vk hex = 0x98 */ },
    { 153,  0,                               0,                    0, 0,               /* vk hex = 0x99 */ },
    { 154,  0,                               0,                    0, 0,               /* vk hex = 0x9A */ },
    { 155,  0,                               0,                    0, 0,               /* vk hex = 0x9B */ },
    { 156,  0,                               0,                    0, 0,               /* vk hex = 0x9C */ },
    { 157,  0,                               0,                    0, 0,               /* vk hex = 0x9D */ },
    { 158,  0,                               0,                    0, 0,               /* vk hex = 0x9E */ },
    { 159,  0,                               0,                    0, 0,               /* vk hex = 0x9F */ },
    { 160,  VK_LSHIFT,                       K_LSHIFT,             0, 0,               /* vk hex = 0xA0 */ },
    { 161,  VK_RSHIFT,                       K_RSHIFT,             0, 0,               /* vk hex = 0xA1 */ },
    { 162,  VK_LCONTROL,                     K_LCTRL,              0, 0,               /* vk hex = 0xA2 */ },
    { 163,  VK_RCONTROL,                     K_RCTRL,              0, 0,               /* vk hex = 0xA3 */ },
    { 164,  VK_LMENU,                        K_LALT,               0, 0,               /* vk hex = 0xA4 */ },
    { 165,  VK_RMENU,                        K_RALT,               0, 0,               /* vk hex = 0xA5 */ },
    { 166,  VK_BROWSER_BACK,                 0,                    0, 0,               /* vk hex = 0xA6 */ },
    { 167,  VK_BROWSER_FORWARD,              0,                    0, 0,               /* vk hex = 0xA7 */ },
    { 168,  VK_BROWSER_REFRESH,              0,                    0, 0,               /* vk hex = 0xA8 */ },
    { 169,  VK_BROWSER_STOP,                 0,                    0, 0,               /* vk hex = 0xA9 */ },
    { 170,  VK_BROWSER_SEARCH,               0,                    0, 0,               /* vk hex = 0xAA */ },
    { 171,  VK_BROWSER_FAVORITES,            0,                    0, 0,               /* vk hex = 0xAB */ },
    { 172,  VK_BROWSER_HOME,                 0,                    0, 0,               /* vk hex = 0xAC */ },
    { 173,  VK_VOLUME_MUTE,                  0,                    0, 0,               /* vk hex = 0xAD */ },
    { 174,  VK_VOLUME_DOWN,                  0,                    0, 0,               /* vk hex = 0xAE */ },
    { 175,  VK_VOLUME_UP,                    0,                    0, 0,               /* vk hex = 0xAF */ },
    { 176,  VK_MEDIA_NEXT_TRACK,             0,                    0, 0,               /* vk hex = 0xB0 */ },
    { 177,  VK_MEDIA_PREV_TRACK,             0,                    0, 0,               /* vk hex = 0xB1 */ },
    { 178,  VK_MEDIA_STOP,                   0,                    0, 0,               /* vk hex = 0xB2 */ },
    { 179,  VK_MEDIA_PLAY_PAUSE,             0,                    0, 0,               /* vk hex = 0xB3 */ },
    { 180,  VK_LAUNCH_MAIL,                  0,                    0, 0,               /* vk hex = 0xB4 */ },
    { 181,  VK_LAUNCH_MEDIA_SELECT,          0,                    0, 0,               /* vk hex = 0xB5 */ },
    { 182,  VK_LAUNCH_APP1,                  0,                    0, 0,               /* vk hex = 0xB6 */ },
    { 183,  VK_LAUNCH_APP2,                  0,                    0, 0,               /* vk hex = 0xB7 */ },
    { 184,  0,                               0,                    0, 0,               /* vk hex = 0xB8 */ },
    { 185,  0,                               0,                    0, 0,               /* vk hex = 0xB9 */ },
    { 186,  VK_OEM_1,                        K_SEMICOLON,          0, 0,               /* vk hex = 0xBA */ },
    { 187,  VK_OEM_PLUS,                     K_EQUALS,             0, 0,               /* vk hex = 0xBB */ },
    { 188,  VK_OEM_COMMA,                    K_COMMA,              0, 0,               /* vk hex = 0xBC */ },
    { 189,  VK_OEM_MINUS,                    K_MINUS,              0, 0,               /* vk hex = 0xBD */ },
    { 190,  VK_OEM_PERIOD,                   K_PERIOD,             0, 0,               /* vk hex = 0xBE */ },
    { 191,  VK_OEM_2,                        K_SLASH,              0, 0,               /* vk hex = 0xBF */ },
    { 192,  VK_OEM_3,                        K_GRAVE,              0, 0,               /* vk hex = 0xC0 */ },
    { 193,  0,                               0,                    0, 0,               /* vk hex = 0xC1 */ },
    { 194,  0,                               0,                    0, 0,               /* vk hex = 0xC2 */ },
    { 195,  0,                               0,                    0, 0,               /* vk hex = 0xC3 */ },
    { 196,  0,                               0,                    0, 0,               /* vk hex = 0xC4 */ },
    { 197,  0,                               0,                    0, 0,               /* vk hex = 0xC5 */ },
    { 198,  0,                               0,                    0, 0,               /* vk hex = 0xC6 */ },
    { 199,  0,                               0,                    0, 0,               /* vk hex = 0xC7 */ },
    { 200,  0,                               0,                    0, 0,               /* vk hex = 0xC8 */ },
    { 201,  0,                               0,                    0, 0,               /* vk hex = 0xC9 */ },
    { 202,  0,                               0,                    0, 0,               /* vk hex = 0xCA */ },
    { 203,  0,                               0,                    0, 0,               /* vk hex = 0xCB */ },
    { 204,  0,                               0,                    0, 0,               /* vk hex = 0xCC */ },
    { 205,  0,                               0,                    0, 0,               /* vk hex = 0xCD */ },
    { 206,  0,                               0,                    0, 0,               /* vk hex = 0xCE */ },
    { 207,  0,                               0,                    0, 0,               /* vk hex = 0xCF */ },
    { 208,  0,                               0,                    0, 0,               /* vk hex = 0xD0 */ },
    { 209,  0,                               0,                    0, 0,               /* vk hex = 0xD1 */ },
    { 210,  0,                               0,                    0, 0,               /* vk hex = 0xD2 */ },
    { 211,  0,                               0,                    0, 0,               /* vk hex = 0xD3 */ },
    { 212,  0,                               0,                    0, 0,               /* vk hex = 0xD4 */ },
    { 213,  0,                               0,                    0, 0,               /* vk hex = 0xD5 */ },
    { 214,  0,                               0,                    0, 0,               /* vk hex = 0xD6 */ },
    { 215,  0,                               0,                    0, 0,               /* vk hex = 0xD7 */ },
    { 216,  0,                               0,                    0, 0,               /* vk hex = 0xD8 */ },
    { 217,  0,                               0,                    0, 0,               /* vk hex = 0xD9 */ },
    { 218,  0,                               0,                    0, 0,               /* vk hex = 0xDA */ },
    { 219,  VK_OEM_4,                        K_LEFTBRACKET,        0, 0,               /* vk hex = 0xDB */ },
    { 220,  VK_OEM_5,                        K_BACKSLASH,          0, 0,               /* vk hex = 0xDC */ },
    { 221,  VK_OEM_6,                        K_RIGHTBRACKET,       0, 0,               /* vk hex = 0xDD */ },
    { 222,  VK_OEM_7,                        K_APOSTROPHE,         0, 0,               /* vk hex = 0xDE */ },
    { 223,  VK_OEM_8,                        0,                    0, 0,               /* vk hex = 0xDF */ },
    { 224,  0,                               0,                    0, 0,               /* vk hex = 0xE0 */ },
    { 225,  0,                               0,                    0, 0,               /* vk hex = 0xE1 */ },
    { 226,  VK_OEM_102,                      0,                    0, 0,               /* vk hex = 0xE2 */ },
    { 227,  0,                               0,                    0, 0,               /* vk hex = 0xE3 */ },
    { 228,  0,                               0,                    0, 0,               /* vk hex = 0xE4 */ },
    { 229,  VK_PROCESSKEY,                   0,                    0, 0,               /* vk hex = 0xE5 */ },
    { 230,  0,                               0,                    0, 0,               /* vk hex = 0xE6 */ },
    { 231,  VK_PACKET,                       0,                    0, 0,               /* vk hex = 0xE7 */ },
    { 232,  0,                               0,                    0, 0,               /* vk hex = 0xE8 */ },
    { 233,  0,                               0,                    0, 0,               /* vk hex = 0xE9 */ },
    { 234,  0,                               0,                    0, 0,               /* vk hex = 0xEA */ },
    { 235,  0,                               0,                    0, 0,               /* vk hex = 0xEB */ },
    { 236,  0,                               0,                    0, 0,               /* vk hex = 0xEC */ },
    { 237,  0,                               0,                    0, 0,               /* vk hex = 0xED */ },
    { 238,  0,                               0,                    0, 0,               /* vk hex = 0xEE */ },
    { 239,  0,                               0,                    0, 0,               /* vk hex = 0xEF */ },
    { 240,  0,                               0,                    0, 0,               /* vk hex = 0xF0 */ },
    { 241,  0,                               0,                    0, 0,               /* vk hex = 0xF1 */ },
    { 242,  0,                               0,                    0, 0,               /* vk hex = 0xF2 */ },
    { 243,  0,                               0,                    0, 0,               /* vk hex = 0xF3 */ },
    { 244,  0,                               0,                    0, 0,               /* vk hex = 0xF4 */ },
    { 245,  0,                               0,                    0, 0,               /* vk hex = 0xF5 */ },
    { 246,  VK_ATTN,                         0,                    0, 0,               /* vk hex = 0xF6 */ },
    { 247,  VK_CRSEL,                        0,                    0, 0,               /* vk hex = 0xF7 */ },
    { 248,  VK_EXSEL,                        0,                    0, 0,               /* vk hex = 0xF8 */ },
    { 249,  VK_EREOF,                        0,                    0, 0,               /* vk hex = 0xF9 */ },
    { 250,  VK_PLAY,                         0,                    0, 0,               /* vk hex = 0xFA */ },
    { 251,  VK_ZOOM,                         0,                    0, 0,               /* vk hex = 0xFB */ },
    { 252,  VK_NONAME,                       0,                    0, 0,               /* vk hex = 0xFC */ },
    { 253,  VK_PA1,                          0,                    0, 0,               /* vk hex = 0xFD */ },
    { 254,  VK_OEM_CLEAR,                    0,                    0, 0,               /* vk hex = 0xFE */ },
};



// getshiftbits
int Platform_Windows_Input_GetShiftBits (void)
{
	int shifted = Flag_Check (GetKeyState(VK_LSHIFT),    0x8000) || Flag_Check (GetKeyState(VK_RSHIFT),   0x8000);
	int ctrled	= Flag_Check (GetKeyState(VK_LCONTROL),  0x8000) || Flag_Check (GetKeyState(VK_RCONTROL), 0x8000);
	int alted	= Flag_Check (GetKeyState(VK_LMENU),     0x8000) || Flag_Check (GetKeyState(VK_RMENU),    0x8000);

	return shifted + ctrled * 2 + alted * 4;
}


// getmousebits
void Platform_Windows_Input_GetMouseBits (WPARAM wparam, LPARAM lparam, required int *button_bits, required int *shift_bits, required int *x, required int *y)
{
	int m1 = Flag_Check (wparam, MK_LBUTTON);
	int m2 = Flag_Check (wparam, MK_RBUTTON);
	int m3 = Flag_Check (wparam, MK_MBUTTON);
	int m4 = Flag_Check (wparam, MK_XBUTTON1);
	int m5 = Flag_Check (wparam, MK_XBUTTON2);
	*shift_bits = Platform_Windows_Input_GetShiftBits();
	*button_bits =	m1 * 1 + m2 * 2 + m3 * 4 + m4 * 8 + m5 * 16;
	*x = GET_X_LPARAM(lparam);
	*y = GET_Y_LPARAM(lparam);
}


//
// Dispatch
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


#endif // PLATFORM_GUI_WINDOWS



