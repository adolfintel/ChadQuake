/*
Copyright (C) 2013-2014 Baker

*/
// vidco.h -- vid

#ifndef __VIDCO_H__
#define __VIDCO_H__

// Dispatch wiring
#ifdef CORE_SDL
	int Session_Dispatch (void *_sdl_event); // why int?
#endif

	// This causes several billion problems in header include order.  Don't do it!
//#ifdef PLATFORM_GUI_WINDOWS
//	#include "core_windows.h" // Puke city ...
//	LRESULT CALLBACK Session_Dispatch (HWND hwnd, UINT Msg, WPARAM wparam, LPARAM lparam);
//#endif

#ifdef PLATFORM_GUI_OSX
#include "vid_osx_keys.h" // I ain't including an ancient carbon framework to get the keys.
#endif // PLATFORM_GUI_OSX

void Vid_InitOnce (void);

// Structs

typedef struct
{
	int		width;
	int		height;
	int		bpp;
	cbool	is_fullscreen;
} videomode_item_t;

int Vid_Display_Modes_Properties_Get (int n, reply int *width, reply int *height, reply int *bpp); // for main display


// Shell video.  These aren't properties of the video, but of the platform shell.
cbool Vid_Display_Properties_Get (reply int *left, reply int *top, reply int *width, reply int *height, reply int *bpp); // As in main display.
cbool Vid_Desktop_Properties_Get (reply int *left, reply int *top, reply int *width, reply int *height);

// Not thrilled about this being here
typedef enum  {
	wdostyle_minimized		= (1 << 0),		// honor width and height, but minimize it.  not implemented.
	wdostyle_maximized		= (1 << 1),		// ignore width and height and x and y and give us the max

	wdostyle_display_capture= (1 << 2),		// not implemented.  beyond maximized, a locked captured topmoster with hopefully no border.

	wdostyle_borderless		= (1 << 3),		// without a border, very hard to resize.
	wdostyle_resizable		= (1 << 4),		// i feel like this is and borderless are only 2 real properties

	wdostyle_hidden			= (1 << 5),		// i feel like this is and borderless are only 2 real properties

	wdostyle_dont_center	= (1 << 6),		// Normally we are going to center the window, this says honor X, Y instead.

	wdostyle_normal			= wdostyle_resizable,
} wdostyle_e;


// These ugly bastards are buried for a reason
int _Shell_Window_Style (wdostyle_e style);		// These are the platform specific window styles, ignoring whether we use SDL.
int _Shell_Window_StyleEx (wdostyle_e style);	// We need them even with SDL to determine border size.
int _Plat_Window_Style (wdostyle_e style);		// These are the video handler window styles, which SDL uses its own obviously.
int _Plat_Window_StyleEx (wdostyle_e style);	// So when we create a window with the video handler, we have to use this.  (Confused yet?)


// If you want no cursor, that's hidecursor
typedef enum
{
    _mousepointer__fake = -1, // Force GCC to used signed enum
	mousepointer_invalid = 0,

	mousepointer_arrow,		// Default
	mousepointer_crosshair,	// Think image editor
	mousepointer_hand,		// Button or action, in theory, in practice that's an arrow
	mousepointer_help,		// Help.  I've never seen this decently used.
	mousepointer_hourglass,	// Hourglass
	mousepointer_move,		// Move something
	mousepointer_prohibited,	// Button or action, in theory, in practice that's an arrow
	mousepointer_size_up,		// Size north south
	mousepointer_size_left,	// Size east west
	mousepointer_size_both,	// Size diagonal
	mousepointer_text,		// Text

	MAX_MOUSEPOINTERS,
} mousepointer_e;


// Handle methods
void Vid_Handle_Borders_Get (wdostyle_e style, cbool have_menu, reply int *left, reply int *top, reply int *width, reply int *height, reply int *right, reply int *bottom);
void Vid_Handle_Caption (sys_handle_t cw, const char *fmt, ...) __core_attribute__((__format__(__printf__,2,3))) ;

#ifdef CORE_GL
	sys_handle_t Vid_Handle_Context_Get (void);
	void Vid_Handle_Context_Set (sys_handle_t cw, sys_handle_t draw_context, sys_handle_t gl_context);
#endif

void Vid_Handle_Client_RectRB_Get (sys_handle_t cw, reply int *left, reply int *top, reply int *width, reply int *height, reply int *right, reply int *bottom);
void Vid_Handle_Client_Rect_To_Window_Rect (wdostyle_e style, cbool have_menu, modify int *left, modify int *top, modify int *width, modify int *height);
sys_handle_t *Vid_Handle_Create (void *obj_ptr, const char *caption, crect_t window_rect, wdostyle_e style, cbool have_menu, required sys_handle_t *draw_context_out, required sys_handle_t *gl_context_out);
sys_handle_t *Vid_Handle_Create_Solo_Client (const char *caption, int client_width, int client_height, required sys_handle_t *dc, required sys_handle_t *rc);
sys_handle_t *Vid_Handle_Destroy (sys_handle_t cw, required sys_handle_t *draw_context, required sys_handle_t *gl_context, cbool should_delete_context);
void Vid_Handle_Hide (sys_handle_t cw);
void Vid_Handle_MousePointer (sys_handle_t cw, sys_handle_t *hmouseicon, mousepointer_e mousepointer);
void Vid_Handle_Move (sys_handle_t cw, wdostyle_e style, cbool have_menu, int left, int top, int width, int height);
void Vid_Handle_MinSize (sys_handle_t cw, int width, int height);
void Vid_Handle_MaxSize (sys_handle_t cw, int width, int height);
void Vid_Handle_Show (sys_handle_t cw);
void Vid_Handle_SwapBuffers (sys_handle_t cw, sys_handle_t draw_context, sys_handle_t gl_context);
void Vid_Handle_ZOrder (sys_handle_t cw);

typedef enum {
	window_state_minimized,
	window_state_normal,
	window_state_maximized,
	window_state_fullscreen,
	window_state_fullscreen_exclusive,
} window_state_e;

typedef enum {
	window_life_creating = -1,
	window_life_live = 0,
	window_life_destroying = 1,
} window_life_e;

typedef struct {
// Bring your own
	void				*ptr;
	wdostyle_e			style;
	void				*menu;
	void				**allocs; // Allocs!

// System Makes
	sys_handle_t		wdo, dc, glrc;
	struct _gl_info_t	*gl_info;
	struct _rstate_t	*hw; // base
	struct _texture_array_t_s		*textures;

// System dictates.  At least indirectly.  Through resize
	glmatrix			*ortho;						// CONVENIENCE
	glmatrix			*ortho3d;					// CONVENIENCE
	glmatrix			*projection;				// CONVENIENCE
	glmatrix			*modelview;					// CONVENIENCE
	int					viewport[4];				// CONVENIENCE
	int                 viewport_mouse[4];          // Rendering and mouse viewport are not always same (ios content scale)
	struct _rstate_t	*rstate;					// Desired default, convenience.
	crectrb_t			border;						// CONVENIENCE
	crect_t				frame;						// CONVENIENCE
	crect_t				bounds;						// Similar, but left and right are 0.  Makes a return for viewport
	cbool				in_resize;

	mousepointer_e		mousepointer_style;			// It's available.
	sys_handle_t		hmousepointer;

	cbool				active;
	cbool				hidden;
	window_state_e		window_state; // Minimized, normal, maximized, fullscreen, fullscreen exclusive.
	int					buffer_update;	// Set to -1 on erase background (frame draw prohibited until painting).  When painting increase.
	cbool				locked_awaiting_paint;
	cbool				in_live_resize;  //#pragma message ("todo for mac in particular")
	ticktime_t			cursor_blink;
	cbool				cursor_drawn;
	int					cursor_update;
//	cbool				draw_until_activated;
	window_life_e		window_life;
} vid_t;

// Window methods
void Window_Alloc (vid_t **pvid, void *ptr, const char *caption, int client_width, int client_height, wdostyle_e style, void *menu);
void Window_Borders_Get (vid_t *vid, reply int *left, reply int *top, reply int *width, reply int *height, reply int *right, reply int *bottom);
void Window_Caption (vid_t *vid, const char *fmt, ...) __core_attribute__((__format__(__printf__,2,3))) ;
void Window_Client_Rect_Get (vid_t *vid, reply int *left, reply int *top, reply int *width, reply int *height);
void Window_Client_RectRB_Get (vid_t *vid, reply int *left, reply int *top, reply int *width, reply int *height, reply int *right, reply int *bottom);
void Window_Client_Rect_To_Window_Rect (vid_t *vid, modify int *left, modify int *top, modify int *width, modify int *height);
void Window_Free (vid_t **pvid);
void Window_Hide (vid_t *vid);

void Window_MousePointer (vid_t *vid, mousepointer_e mousepointer);
void Window_Move (vid_t *vid, int left, int top, int width, int height);
void Window_MinSize (vid_t *vid, int width, int height);
void Window_MaxSize (vid_t *vid, int width, int height);
void Window_Show (vid_t *vid);
void Window_SwapBuffers (vid_t *vid);
void Window_ZOrder (vid_t *vid);

#ifdef CORE_GL
	void Window_MakeCurrent (vid_t *vid); // See if the context is current, if not change it.

	void *Vid_GL_GetProcAddress(const char *pfunction_name); // Argument to be had for putting in core_opengl.h but not going to at the moment.
#endif // CORE_GL


#endif // ! __VIDCO_H__
