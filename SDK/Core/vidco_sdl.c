/*
Copyright (C) 2013-2014 Baker

*/
// vidco_sdl.c -- vid

#include "environment.h"

#ifdef CORE_SDL


#define CORE_LOCAL
#include "core.h"
#include "core_sdl.h" // <windows.h>
#include "vidco.h" // Courtesy



//void Vid_InitOnce (void)
//{
//	static cbool done;
//
//	if (!done && !mainus.dispatch_fn) mainus.dispatch_fn = Session_Dispatch;
//	if (!done) {
//		SDL_SetHint(SDL_HINT_ORIENTATIONS, "Portrait");
//
//#if 1 //def PLATFORM_IOS // Something about wanting to initialize Haptic for IOS?
//	    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
//			log_fatal ("Could not initialize SDL Video: %s", SDL_GetError());
//#else
//		if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
//			log_fatal ("Could not initialize SDL: %s", SDL_GetError());
//#endif
//	  			//SDL_Log("Unable to initialize SDL");
//
//		// Attributes -- stencil, multisample, anything else?
//		// SDL_GL_SetAttributes must be done BEFORE SDL_SetVideoMode
//#ifdef PLATFORM_ANDROID
//		SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 16);				// Hmmmm.  Just android?
//#endif //PLATFORM_ANDROID
//		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
////
//	    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);	  // Set depth 24 bits
//	    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);  // Set depth 24 bits
//		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1 ); // Double buffer
//		done = true;
//	}
//}






int _Plat_Window_Style (wdostyle_e style)
{
// SDL_WINDOW_FULLSCREEN_DESKTOP, SDL_WINDOW_MAXIMIZED, SDL_WINDOW_MINIMIZED
// SDL_WINDOW_SHOWN, SDL_WINDOW_FULLSCREEN, SDL_WINDOW_HIDDEN are available.
// We have SDL_WINDOW_OPENGL hard-coded in create window, since our hard code requires it.
    SDL_WindowFlags dw_bordered   = SDL_WINDOW_RESIZABLE;
	SDL_WindowFlags dw_borderless = SDL_WINDOW_BORDERLESS;
	int ret = Flag_Check (style, wdostyle_borderless) ? dw_borderless : dw_bordered;

	if (!Flag_Check (style, wdostyle_resizable))
		ret = Flag_Remove (style, SDL_WINDOW_RESIZABLE);

	return ret;
}

int _Plat_Window_StyleEx (wdostyle_e style)
{
	return 0;
}




cbool Vid_Display_Properties_Get (reply int *left, reply int *top, reply int *width, reply int *height, reply int *bpp)
{
    SDL_DisplayMode mode;
	float diag, vert, horz;

    if (SDL_GetDesktopDisplayMode(0, &mode) != 0)
        log_fatal (SPRINTSFUNC "Failed", __func__);

	// Evolving.
	//SDL_GetDisplayDPI (0, &diag, &horz, &vert);
	//alert ("diag %g horz %g vert %g", diag, vert * mode.w,

	NOT_MISSING_ASSIGN(left, 0); // Better be zero!
	NOT_MISSING_ASSIGN(top, 0); // Better be zero!
	NOT_MISSING_ASSIGN(width, mode.w);
	NOT_MISSING_ASSIGN(height, mode.h);
	NOT_MISSING_ASSIGN(bpp, SDL_BITSPERPIXEL(mode.format));

	return true;
}


// Send -1 to start, it returns count.
int Vid_Display_Modes_Properties_Get (int n, reply int *width, reply int *height, reply int *bpp)
{
	int ret;
    SDL_DisplayMode mode;

    int modes_count = SDL_GetNumDisplayModes(0);

	if (n == -1)				ret = modes_count;		// -1 wants count
	else if (n >= modes_count)	ret = 0; 				// Hit last return 0
	else {						ret = 1;				// continue
		if (SDL_GetDisplayMode(0, n, &mode))
			return log_fatal ("EnumDisplaySettings failed on %d", n); // End

		NOT_MISSING_ASSIGN(width, mode.w);
		NOT_MISSING_ASSIGN(height, mode.h);
		NOT_MISSING_ASSIGN(bpp,	SDL_BITSPERPIXEL(mode.format));

		//alert ("Mode %d %d %d", width ? *width : -1, height ? *height : -1, bpp ? *bpp : -1);
	}
	return ret; // continue
}


void Vid_Handle_Client_RectRB_Get (sys_handle_t cw, reply int *left, reply int *top, reply int *width, reply int *height, reply int *right, reply int *bottom)
{
// Not implemented at this time.
	crect_t clrect;
	SDL_GetWindowPosition (cw, &clrect.left, &clrect.top);
	SDL_GetWindowSize (cw, &clrect.width, &clrect.height);

	NOT_MISSING_ASSIGN (left, clrect.left);
	NOT_MISSING_ASSIGN (top, clrect.top);
	NOT_MISSING_ASSIGN (width, clrect.width);
	NOT_MISSING_ASSIGN (height, clrect.height);
	NOT_MISSING_ASSIGN (right, clrect.left + clrect.width); // Windows style
	NOT_MISSING_ASSIGN (bottom, clrect.top + clrect.height);// Where bottom right overshoots +1 like floating point would
}


#include "core_opengl.h"

sys_handle_t Vid_Handle_Context_Get (void)
{
	return SDL_GL_GetCurrentContext ();
}


void Vid_Handle_Context_Set (sys_handle_t cw, sys_handle_t draw_context, sys_handle_t gl_context)
{
	if (SDL_GL_MakeCurrent(cw, gl_context) < 0)
		log_fatal ("SDL_GL_MakeCurrent failed");

//	alert ("Vendor %s", glGetString (GL_VENDOR));
}

void Vid_Handle_SwapBuffers (sys_handle_t cw, sys_handle_t draw_context, sys_handle_t gl_context)
{
	SDL_GL_SwapWindow (cw);
}


sys_handle_t *Vid_Handle_Destroy (sys_handle_t cw, required sys_handle_t *draw_context, required sys_handle_t *gl_context, cbool should_delete_context)
{
	// destroy = 1 = TEARDOWN_FULL else TEARDOWN_NO_DELETE_GL_CONTEXT (don't destroy the context or destroy window)

	*draw_context = NULL;	// SDL_FreeSurface (*draw_context);  But I think destroywindow does this.
	SDL_GL_DeleteContext(*gl_context); *gl_context = NULL; // Think we need to do this.

	SDL_DestroyWindow (cw);

	return NULL;
}


void Vid_Handle_MinSize (sys_handle_t cw, int width, int height)
{
	SDL_SetWindowMinimumSize (cw, width, height); // Make this a function or something Mac is setContentMinSize or something
}


void Vid_Handle_MaxSize (sys_handle_t cw, int width, int height)
{
	SDL_SetWindowMinimumSize (cw, width, height); // Make this a function or something Mac is setContentMinSize or something
}


sys_handle_t *Vid_Handle_Create (void *obj_ptr, const char *caption, crect_t window_rect, wdostyle_e style, cbool have_menu, required sys_handle_t *draw_context_out, required sys_handle_t *gl_context_out)
{
	SDL_WindowFlags plat_style		= _Plat_Window_Style   (style);
	SDL_WindowFlags plat_style_ex	= _Plat_Window_StyleEx (style);

	// SDL creates a window based on the client height ... as a result, we have extra work to do.
	crectrb_t border;
	Vid_Handle_Borders_Get (style, have_menu, RECTRB_REPLY(border));

	{
		// In order to do this perfect, I guess SDL places client x and y too so we'd need grandular left vs. right border for each dimension.
		sys_handle_t cw = SDL_CreateWindow(
			caption,
			window_rect.left + border.left, window_rect.top + border.top,			// x, y SDL_WINDOWPOS_CENTERED
			window_rect.width - border.width, window_rect.height - border.height,					// width, height
			plat_style | SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN /*SDL_WINDOW_HIDDEN  SDL_WINDOW_SHOWN */
		);

		void *old = (void *)SDL_SetWindowData (cw, "GWLP_USERDATA", obj_ptr); // Points to us.
		sys_handle_t drawcontext = NULL; // SDL_GetWindowSurface(cw); // TOXIC to android for some reason and we don't need it very badly
		sys_handle_t glcontext 	 = SDL_GL_CreateContext(cw);

		SDL_GL_MakeCurrent(cw, glcontext);

		NOT_MISSING_ASSIGN (draw_context_out, NULL); // NULL for now, SDL_GetWindowSurface is toxic to android.
		NOT_MISSING_ASSIGN (gl_context_out, glcontext);

		return cw;  //SDL_GL_SetSwapInterval(1); // Really?
	}
}


void Vid_Handle_Move (sys_handle_t cw, wdostyle_e style, cbool have_menu, int left, int top, int width, int height)
{
	//Vid_Handle_Client_Rect_To_Window_Rect (style, have_menu, &left, &top, &width, &height);

	SDL_SetWindowPosition (cw, left, top);
	SDL_SetWindowSize (cw, width, height);
}


void Vid_Handle_Hide (sys_handle_t cw)
{
	SDL_HideWindow (cw);
}


void Vid_Handle_Show (sys_handle_t cw)
{
	SDL_ShowWindow (cw);	// Show it
//	SDL_RaiseWindow (cw);	// Give it focus
}




void Vid_Handle_ZOrder (sys_handle_t cw)
{
	SDL_RaiseWindow (cw);	// Give it focus
}


void _Vid_Handle_Caption (sys_handle_t cw, const char *text)
{
	SDL_SetWindowTitle (cw, text);
}

/*
static LPCTSTR W_For_CursorEnum (mousepointer_e mousepointer)
{
	typedef struct tt  {
		int i;
		LPCTSTR name;
	};

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
*/

void Vid_Handle_MousePointer (sys_handle_t cw, sys_handle_t *hmousepointer, mousepointer_e mousepointer)
{
#if 0
	*hmousepointer = LoadCursor (NULL, W_For_CursorEnum(mousepointer) );

	//SetCursor (hicon);
	SetClassLong(cw, GCL_HCURSOR, (DWORD)*hmousepointer);
	// SDL_SetCursor ?
#endif
}



///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: MSGBOX HELPER
///////////////////////////////////////////////////////////////////////////////

// Title cannot be NULL, System_MessageBox checks for that.
// windows equivalent: MessageBox (NULL, text, title , MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
int _Platform_MessageBox (const char *title, const char *text)
{
	//SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_INFORMATION, title, text, NULL); // Not urgent enough
	// SDL_MESSAGEBOX_ERROR does nothing differently :(
	SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_ERROR, title, text, NULL /*no parent window*/);

	return 0;
}

char *_Platform_Text_Dialog_Popup_Alloc (sys_handle_t parent_window, const char *prompt, const char *text, cbool _is_multiline)
{
	msgbox (prompt, "Paste text to clipboard and click ok.  Default will be " QUOTED_S, text);
	return Clipboard_Get_Text_Alloc ();
}


///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: EVENTS
///////////////////////////////////////////////////////////////////////////////


void Platform_Events_SleepForInput (required sys_handle_t *phandle_tevent, double max_wait_seconds)
{
	// By sending NULL, we don't pull item off the queue.
	SDL_WaitEventTimeout (NULL, (int)(max_wait_seconds * 1000) /* converting up to milliseconds */);
}


cbool Platform_Events_Do (plat_dispatch_fn_t dispatcher_function)
{
	// I have a little bit of concern the Windows version of this loop may be inadequate
	// It looks like it doesn't clear the queue but does just 1 message?
	SDL_Event e;

	//Handle events on queue
	while(SDL_PollEvent (&e))
	{
		//User requests quit
		if (e.type == SDL_QUIT )
			return false;

		if (dispatcher_function)
			dispatcher_function (&e);
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//  SDL OVERRIDES:  SPECIAL CASES BECAUSE THEY REALLY ARE "SHELL" FUNCTIONS
//                  "SHELL" FUNCTIONS DON'T VARY FOR A CONSOLE APP.
//                  AND THEY DON'T HERE EITHER ... BUT SDL HAS A REPLACEMENT
//                  AND WE USE IT DESPITE HAVING A BETTER NATIVE METHOD
//                  AVAILABLE TO US.  FOR THE SAKE OF NOT HAVING TO CUSTOMIZE
//                  FOR ANDROID OR IPHONE OR LINUX, WHICH WOULD BE A MESS
///////////////////////////////////////////////////////////////////////////////
// Platform_Sleep_Milliseconds is not member of this.
// It is in file_system.c and we use the SDL func if SDL specified.
// _Platform_GetProcAddress isn't settled yet.  Probably stays in file_system.c
// because file_system.c is an all operating systems file.

double Platform_MachineTime (void) // no set metric except result is in seconds
{
	double _ticks = SDL_GetTicks(); /* milliseconds since library loaded */
	double d = _ticks / 1000.0;
	return d;
}

// copies given text to clipboard.  Text can't be NULL
cbool _Platform_Clipboard_Set_Text (const char *text_to_clipboard)
{
	// SDL_SetClipboardText returns 0 on success
	return !SDL_SetClipboardText(text_to_clipboard);
}

char *_Platform_Clipboard_Get_Text_Alloc (void)
{
	char *_text = SDL_GetClipboardText();  //(!SDL_HasClipboardText())
	char *text_out  = _text ? core_strdup(_text) : NULL;
	return text_out;
}




///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: DISPATCH.  AT LEAST THE DEFAULT ONE.
///////////////////////////////////////////////////////////////////////////////


int keymap [KEYMAP_COUNT_512][5] = {


	{ 0,    0,                               0,                  /*  */ },
	{ 1,    0,                               0,                  /*  */ },
	{ 2,    0,                               0,                  /*  */ },
	{ 3,    0,                               0,                  /*  */ },
	{ 4,    SDL_SCANCODE_A,                  'A',                /*  */ },
	{ 5,    SDL_SCANCODE_B,                  'B',                /*  */ },
	{ 6,    SDL_SCANCODE_C,                  'C',                /*  */ },
	{ 7,    SDL_SCANCODE_D,                  'D',                /*  */ },
	{ 8,    SDL_SCANCODE_E,                  'E',                /*  */ },
	{ 9,    SDL_SCANCODE_F,                  'F',                /*  */ },
	{ 10,   SDL_SCANCODE_G,                  'G',                /*  */ },
	{ 11,   SDL_SCANCODE_H,                  'H',                /*  */ },
	{ 12,   SDL_SCANCODE_I,                  'I',                /*  */ },
	{ 13,   SDL_SCANCODE_J,                  'J',                /*  */ },
	{ 14,   SDL_SCANCODE_K,                  'K',                /*  */ },
	{ 15,   SDL_SCANCODE_L,                  'L',                /*  */ },
	{ 16,   SDL_SCANCODE_M,                  'M',                /*  */ },
	{ 17,   SDL_SCANCODE_N,                  'N',                /*  */ },
	{ 18,   SDL_SCANCODE_O,                  'O',                /*  */ },
	{ 19,   SDL_SCANCODE_P,                  'P',                /*  */ },
	{ 20,   SDL_SCANCODE_Q,                  'Q',                /*  */ },
	{ 21,   SDL_SCANCODE_R,                  'R',                /*  */ },
	{ 22,   SDL_SCANCODE_S,                  'S',                /*  */ },
	{ 23,   SDL_SCANCODE_T,                  'T',                /*  */ },
	{ 24,   SDL_SCANCODE_U,                  'U',                /*  */ },
	{ 25,   SDL_SCANCODE_V,                  'V',                /*  */ },
	{ 26,   SDL_SCANCODE_W,                  'W',                /*  */ },
	{ 27,   SDL_SCANCODE_X,                  'X',                /*  */ },
	{ 28,   SDL_SCANCODE_Y,                  'Y',                /*  */ },
	{ 29,   SDL_SCANCODE_Z,                  'Z',                /*  */ },
	{ 30,   SDL_SCANCODE_1,                  '1',                /*  */ },
	{ 31,   SDL_SCANCODE_2,                  '2',                /*  */ },
	{ 32,   SDL_SCANCODE_3,                  '3',                /*  */ },
	{ 33,   SDL_SCANCODE_4,                  '4',                /*  */ },
	{ 34,   SDL_SCANCODE_5,                  '5',                /*  */ },
	{ 35,   SDL_SCANCODE_6,                  '6',                /*  */ },
	{ 36,   SDL_SCANCODE_7,                  '7',                /*  */ },
	{ 37,   SDL_SCANCODE_8,                  '8',                /*  */ },
	{ 38,   SDL_SCANCODE_9,                  '9',                /*  */ },
	{ 39,   SDL_SCANCODE_0,                  '0',                /*  */ },
	{ 40,   SDL_SCANCODE_RETURN,             K_ENTER,            /*  */ },
	{ 41,   SDL_SCANCODE_ESCAPE,             K_ESCAPE,           /*  */ },
	{ 42,   SDL_SCANCODE_BACKSPACE,          K_BACKSPACE,        /*  */ },
	{ 43,   SDL_SCANCODE_TAB,                K_TAB,              /*  */ },
	{ 44,   SDL_SCANCODE_SPACE,              K_SPACE,            /*  */ },
	{ 45,   SDL_SCANCODE_MINUS,              K_MINUS,            /*  */ },
	{ 46,   SDL_SCANCODE_EQUALS,             K_EQUALS,           /*  */ },
	{ 47,   SDL_SCANCODE_LEFTBRACKET,        K_LEFTBRACKET,      /*  */ },
	{ 48,   SDL_SCANCODE_RIGHTBRACKET,       K_RIGHTBRACKET,     /*  */ },
	{ 49,   SDL_SCANCODE_BACKSLASH,          K_BACKSLASH,        /*  */ },
	{ 50,   SDL_SCANCODE_NONUSHASH,          0,                  /*  */ },
	{ 51,   SDL_SCANCODE_SEMICOLON,          K_SEMICOLON,        /*  */ },
	{ 52,   SDL_SCANCODE_APOSTROPHE,         K_APOSTROPHE,       /*  */ },
	{ 53,   SDL_SCANCODE_GRAVE,              K_GRAVE,            /*  */ },
	{ 54,   SDL_SCANCODE_COMMA,              K_COMMA,            /*  */ },
	{ 55,   SDL_SCANCODE_PERIOD,             K_PERIOD,           /*  */ },
	{ 56,   SDL_SCANCODE_SLASH,              K_SLASH,            /*  */ },
	{ 57,   SDL_SCANCODE_CAPSLOCK,           K_CAPSLOCK,         /*  */ },
	{ 58,   SDL_SCANCODE_F1,                 K_F1,               /*  */ },
	{ 59,   SDL_SCANCODE_F2,                 K_F2,               /*  */ },
	{ 60,   SDL_SCANCODE_F3,                 K_F3,               /*  */ },
	{ 61,   SDL_SCANCODE_F4,                 K_F4,               /*  */ },
	{ 62,   SDL_SCANCODE_F5,                 K_F5,               /*  */ },
	{ 63,   SDL_SCANCODE_F6,                 K_F6,               /*  */ },
	{ 64,   SDL_SCANCODE_F7,                 K_F7,               /*  */ },
	{ 65,   SDL_SCANCODE_F8,                 K_F8,               /*  */ },
	{ 66,   SDL_SCANCODE_F9,                 K_F9,               /*  */ },
	{ 67,   SDL_SCANCODE_F10,                K_F10,              /*  */ },
	{ 68,   SDL_SCANCODE_F11,                K_F11,              /*  */ },
	{ 69,   SDL_SCANCODE_F12,                K_F12,              /*  */ },
	{ 70,   SDL_SCANCODE_PRINTSCREEN,        K_PRINTSCREEN,      /*  */ },
	{ 71,   SDL_SCANCODE_SCROLLLOCK,         K_SCROLLLOCK,       /*  */ },
	{ 72,   SDL_SCANCODE_PAUSE,              K_PAUSE,            /*  */ },
	{ 73,   SDL_SCANCODE_INSERT,             K_INSERT,           /*  */ },
	{ 74,   SDL_SCANCODE_HOME,               K_HOME,             /*  */ },
	{ 75,   SDL_SCANCODE_PAGEUP,             K_PAGEUP,           /*  */ },
	{ 76,   SDL_SCANCODE_DELETE,             K_DELETE,           /*  */ },
	{ 77,   SDL_SCANCODE_END,                K_END,              /*  */ },
	{ 78,   SDL_SCANCODE_PAGEDOWN,           K_PAGEDOWN,         /*  */ },
	{ 79,   SDL_SCANCODE_RIGHT,              K_RIGHTARROW,       /*  */ },
	{ 80,   SDL_SCANCODE_LEFT,               K_LEFTARROW,        /*  */ },
	{ 81,   SDL_SCANCODE_DOWN,               K_DOWNARROW,        /*  */ },
	{ 82,   SDL_SCANCODE_UP,                 K_UPARROW,          /*  */ },
	{ 83,   SDL_SCANCODE_NUMLOCKCLEAR,       K_NUMLOCK,          /*  */ },
	{ 84,   SDL_SCANCODE_KP_DIVIDE,          K_NUMPAD_DIVIDE,    /*  */ },
	{ 85,   SDL_SCANCODE_KP_MULTIPLY,        K_NUMPAD_MULTIPLY,  /*  */ },
	{ 86,   SDL_SCANCODE_KP_MINUS,           K_NUMPAD_MINUS,     /*  */ },
	{ 87,   SDL_SCANCODE_KP_PLUS,            K_NUMPAD_PLUS,      /*  */ },
	{ 88,   SDL_SCANCODE_KP_ENTER,           K_NUMPAD_ENTER,     /*  */ }, // numpad
	{ 89,   SDL_SCANCODE_KP_1,               K_NUMPAD_1,         /*  */ },
	{ 90,   SDL_SCANCODE_KP_2,               K_NUMPAD_2,         /*  */ },
	{ 91,   SDL_SCANCODE_KP_3,               K_NUMPAD_3,         /*  */ },
	{ 92,   SDL_SCANCODE_KP_4,               K_NUMPAD_4,         /*  */ },
	{ 93,   SDL_SCANCODE_KP_5,               K_NUMPAD_5,         /*  */ },
	{ 94,   SDL_SCANCODE_KP_6,               K_NUMPAD_6,         /*  */ },
	{ 95,   SDL_SCANCODE_KP_7,               K_NUMPAD_7,         /*  */ },
	{ 96,   SDL_SCANCODE_KP_8,               K_NUMPAD_8,         /*  */ },
	{ 97,   SDL_SCANCODE_KP_9,               K_NUMPAD_9,         /*  */ },
	{ 98,   SDL_SCANCODE_KP_0,               K_NUMPAD_0,         /*  */ },
	{ 99,   SDL_SCANCODE_KP_PERIOD,          K_NUMPAD_PERIOD,    /*  */ },
	{ 100,  SDL_SCANCODE_NONUSBACKSLASH,     0,                  /*  */ },
	{ 101,  SDL_SCANCODE_APPLICATION,        K_MENU,             /*  */ },
	{ 102,  SDL_SCANCODE_POWER,              0,                  /*  */ },
	{ 103,  SDL_SCANCODE_KP_EQUALS,          0,                  /*  */ },
	{ 104,  SDL_SCANCODE_F13,                0,                  /*  */ },
	{ 105,  SDL_SCANCODE_F14,                0,                  /*  */ },
	{ 106,  SDL_SCANCODE_F15,                0,                  /*  */ },
	{ 107,  SDL_SCANCODE_F16,                0,                  /*  */ },
	{ 108,  SDL_SCANCODE_F17,                0,                  /*  */ },
	{ 109,  SDL_SCANCODE_F18,                0,                  /*  */ },
	{ 110,  SDL_SCANCODE_F19,                0,                  /*  */ },
	{ 111,  SDL_SCANCODE_F20,                0,                  /*  */ },
	{ 112,  SDL_SCANCODE_F21,                0,                  /*  */ },
	{ 113,  SDL_SCANCODE_F22,                0,                  /*  */ },
	{ 114,  SDL_SCANCODE_F23,                0,                  /*  */ },
	{ 115,  SDL_SCANCODE_F24,                0,                  /*  */ },
	{ 116,  SDL_SCANCODE_EXECUTE,            0,                  /*  */ },
	{ 117,  SDL_SCANCODE_HELP,               0,                  /*  */ },
	{ 118,  SDL_SCANCODE_MENU,               0,                  /*  */ },
	{ 119,  SDL_SCANCODE_SELECT,             0,                  /*  */ },
	{ 120,  SDL_SCANCODE_STOP,               0,                  /*  */ },
	{ 121,  SDL_SCANCODE_AGAIN,              0,                  /*  */ },
	{ 122,  SDL_SCANCODE_UNDO,               0,                  /*  */ },
	{ 123,  SDL_SCANCODE_CUT,                0,                  /*  */ },
	{ 124,  SDL_SCANCODE_COPY,               0,                  /*  */ },
	{ 125,  SDL_SCANCODE_PASTE,              0,                  /*  */ },
	{ 126,  SDL_SCANCODE_FIND,               0,                  /*  */ },
	{ 127,  SDL_SCANCODE_MUTE,               0,                  /*  */ },
	{ 128,  SDL_SCANCODE_VOLUMEUP,           0,                  /*  */ },
	{ 129,  SDL_SCANCODE_VOLUMEDOWN,         0,                  /*  */ },
	{ 130,  0,                               0,                  /*  */ },
	{ 131,  0,                               0,                  /*  */ },
	{ 132,  0,                               0,                  /*  */ },
	{ 133,  SDL_SCANCODE_KP_COMMA,           0,                  /*  */ },
	{ 134,  SDL_SCANCODE_KP_EQUALSAS400,     0,                  /*  */ },
	{ 135,  SDL_SCANCODE_INTERNATIONAL1,     0,                  /*  */ },
	{ 136,  SDL_SCANCODE_INTERNATIONAL2,     0,                  /*  */ },
	{ 137,  SDL_SCANCODE_INTERNATIONAL3,     0,                  /*  */ },
	{ 138,  SDL_SCANCODE_INTERNATIONAL4,     0,                  /*  */ },
	{ 139,  SDL_SCANCODE_INTERNATIONAL5,     0,                  /*  */ },
	{ 140,  SDL_SCANCODE_INTERNATIONAL6,     0,                  /*  */ },
	{ 141,  SDL_SCANCODE_INTERNATIONAL7,     0,                  /*  */ },
	{ 142,  SDL_SCANCODE_INTERNATIONAL8,     0,                  /*  */ },
	{ 143,  SDL_SCANCODE_INTERNATIONAL9,     0,                  /*  */ },
	{ 144,  SDL_SCANCODE_LANG1,              0,                  /*  */ },
	{ 145,  SDL_SCANCODE_LANG2,              0,                  /*  */ },
	{ 146,  SDL_SCANCODE_LANG3,              0,                  /*  */ },
	{ 147,  SDL_SCANCODE_LANG4,              0,                  /*  */ },
	{ 148,  SDL_SCANCODE_LANG5,              0,                  /*  */ },
	{ 149,  SDL_SCANCODE_LANG6,              0,                  /*  */ },
	{ 150,  SDL_SCANCODE_LANG7,              0,                  /*  */ },
	{ 151,  SDL_SCANCODE_LANG8,              0,                  /*  */ },
	{ 152,  SDL_SCANCODE_LANG9,              0,                  /*  */ },
	{ 153,  SDL_SCANCODE_ALTERASE,           0,                  /*  */ },
	{ 154,  SDL_SCANCODE_SYSREQ,             0,                  /*  */ },
	{ 155,  SDL_SCANCODE_CANCEL,             0,                  /*  */ },
	{ 156,  SDL_SCANCODE_CLEAR,              0,                  /*  */ },
	{ 157,  SDL_SCANCODE_PRIOR,              0,                  /*  */ },
	{ 158,  SDL_SCANCODE_RETURN2,            0,                  /*  */ },
	{ 159,  SDL_SCANCODE_SEPARATOR,          0,                  /*  */ },
	{ 160,  SDL_SCANCODE_OUT,                0,                  /*  */ },
	{ 161,  SDL_SCANCODE_OPER,               0,                  /*  */ },
	{ 162,  SDL_SCANCODE_CLEARAGAIN,         0,                  /*  */ },
	{ 163,  SDL_SCANCODE_CRSEL,              0,                  /*  */ },
	{ 164,  SDL_SCANCODE_EXSEL,              0,                  /*  */ },
	{ 165,  0,                               0,                  /*  */ },
	{ 166,  0,                               0,                  /*  */ },
	{ 167,  0,                               0,                  /*  */ },
	{ 168,  0,                               0,                  /*  */ },
	{ 169,  0,                               0,                  /*  */ },
	{ 170,  0,                               0,                  /*  */ },
	{ 171,  0,                               0,                  /*  */ },
	{ 172,  0,                               0,                  /*  */ },
	{ 173,  0,                               0,                  /*  */ },
	{ 174,  0,                               0,                  /*  */ },
	{ 175,  0,                               0,                  /*  */ },
	{ 176,  SDL_SCANCODE_KP_00,              0,                  /*  */ },
	{ 177,  SDL_SCANCODE_KP_000,             0,                  /*  */ },
	{ 178,  SDL_SCANCODE_THOUSANDSSEPARATOR, 0,                  /*  */ },
	{ 179,  SDL_SCANCODE_DECIMALSEPARATOR,   K_NUMPAD_SEPARATOR, /*  */ },
	{ 180,  SDL_SCANCODE_CURRENCYUNIT,       0,                  /*  */ },
	{ 181,  SDL_SCANCODE_CURRENCYSUBUNIT,    0,                  /*  */ },
	{ 182,  SDL_SCANCODE_KP_LEFTPAREN,       0,                  /*  */ },
	{ 183,  SDL_SCANCODE_KP_RIGHTPAREN,      0,                  /*  */ },
	{ 184,  SDL_SCANCODE_KP_LEFTBRACE,       0,                  /*  */ },
	{ 185,  SDL_SCANCODE_KP_RIGHTBRACE,      0,                  /*  */ },
	{ 186,  SDL_SCANCODE_KP_TAB,             0,                  /*  */ },
	{ 187,  SDL_SCANCODE_KP_BACKSPACE,       0,                  /*  */ },
	{ 188,  SDL_SCANCODE_KP_A,               0,                  /*  */ },
	{ 189,  SDL_SCANCODE_KP_B,               0,                  /*  */ },
	{ 190,  SDL_SCANCODE_KP_C,               0,                  /*  */ },
	{ 191,  SDL_SCANCODE_KP_D,               0,                  /*  */ },
	{ 192,  SDL_SCANCODE_KP_E,               0,                  /*  */ },
	{ 193,  SDL_SCANCODE_KP_F,               0,                  /*  */ },
	{ 194,  SDL_SCANCODE_KP_XOR,             0,                  /*  */ },
	{ 195,  SDL_SCANCODE_KP_POWER,           0,                  /*  */ },
	{ 196,  SDL_SCANCODE_KP_PERCENT,         0,                  /*  */ },
	{ 197,  SDL_SCANCODE_KP_LESS,            0,                  /*  */ },
	{ 198,  SDL_SCANCODE_KP_GREATER,         0,                  /*  */ },
	{ 199,  SDL_SCANCODE_KP_AMPERSAND,       0,                  /*  */ },
	{ 200,  SDL_SCANCODE_KP_DBLAMPERSAND,    0,                  /*  */ },
	{ 201,  SDL_SCANCODE_KP_VERTICALBAR,     0,                  /*  */ },
	{ 202,  SDL_SCANCODE_KP_DBLVERTICALBAR,  0,                  /*  */ },
	{ 203,  SDL_SCANCODE_KP_COLON,           0,                  /*  */ },
	{ 204,  SDL_SCANCODE_KP_HASH,            0,                  /*  */ },
	{ 205,  SDL_SCANCODE_KP_SPACE,           0,                  /*  */ },
	{ 206,  SDL_SCANCODE_KP_AT,              0,                  /*  */ },
	{ 207,  SDL_SCANCODE_KP_EXCLAM,          0,                  /*  */ },
	{ 208,  SDL_SCANCODE_KP_MEMSTORE,        0,                  /*  */ },
	{ 209,  SDL_SCANCODE_KP_MEMRECALL,       0,                  /*  */ },
	{ 210,  SDL_SCANCODE_KP_MEMCLEAR,        0,                  /*  */ },
	{ 211,  SDL_SCANCODE_KP_MEMADD,          0,                  /*  */ },
	{ 212,  SDL_SCANCODE_KP_MEMSUBTRACT,     0,                  /*  */ },
	{ 213,  SDL_SCANCODE_KP_MEMMULTIPLY,     0,                  /*  */ },
	{ 214,  SDL_SCANCODE_KP_MEMDIVIDE,       0,                  /*  */ },
	{ 215,  SDL_SCANCODE_KP_PLUSMINUS,       0,                  /*  */ },
	{ 216,  SDL_SCANCODE_KP_CLEAR,           0,                  /*  */ },
	{ 217,  SDL_SCANCODE_KP_CLEARENTRY,      0,                  /*  */ },
	{ 218,  SDL_SCANCODE_KP_BINARY,          0,                  /*  */ },
	{ 219,  SDL_SCANCODE_KP_OCTAL,           0,                  /*  */ },
	{ 220,  SDL_SCANCODE_KP_DECIMAL,         0,                  /*  */ },
	{ 221,  SDL_SCANCODE_KP_HEXADECIMAL,     0,                  /*  */ },
	{ 222,  0,                               0,                  /*  */ },
	{ 223,  0,                               0,                  /*  */ },
	{ 224,  SDL_SCANCODE_LCTRL,              K_LCTRL,            /*  */ },
	{ 225,  SDL_SCANCODE_LSHIFT,             K_LSHIFT,           /*  */ },
	{ 226,  SDL_SCANCODE_LALT,               K_LALT,             /*  */ },
	{ 227,  SDL_SCANCODE_LGUI,               K_LWIN,             /*  */ },
	{ 228,  SDL_SCANCODE_RCTRL,              K_RCTRL,            /*  */ },
	{ 229,  SDL_SCANCODE_RSHIFT,             K_RSHIFT,           /*  */ },
	{ 230,  SDL_SCANCODE_RALT,               K_RALT,             /*  */ },
	{ 231,  SDL_SCANCODE_RGUI,               K_RWIN,             /*  */ },
	{ 232,  0,                               0,                  /*  */ },
};

// getshiftbits

int Platform_SDL_Input_GetShiftBits (SDL_Event *e)
{
	const byte *state = SDL_GetKeyboardState(NULL);

	int shifted = state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT];
	int ctrled	= state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL];
	int alted	= state[SDL_SCANCODE_LALT] || state[SDL_SCANCODE_RALT];

	return shifted + ctrled * 2 + alted * 4;
}


// getmousebits
void Platform_SDL_Input_GetMouseBits (SDL_Event *e, required int *button_bits, required int *shift_bits, required int *x, required int *y)
{
	const unsigned state = SDL_GetMouseState (NULL, NULL);
	int m1 = Flag_Check (state, SDL_BUTTON(1));
	int m2 = Flag_Check (state, SDL_BUTTON(3)); // SDL flips middle and right
	int m3 = Flag_Check (state, SDL_BUTTON(2)); // SDL must use middle button as #2.  Confirmed
	int m4 = Flag_Check (state, SDL_BUTTON(4));
	int m5 = Flag_Check (state, SDL_BUTTON(5));
	*shift_bits = Platform_SDL_Input_GetShiftBits(e);
	*button_bits =	m1 * 1 + m2 * 2 + m3*4 + m4*8 + m5 *16;
	*x = e->button.x;
	*y = e->button.y;
}

#endif // CORE_SDL



