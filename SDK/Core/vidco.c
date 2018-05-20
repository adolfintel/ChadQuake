/*
Copyright (C) 2013-2016 Baker

*/
// vidco.c -- common
#include "environment.h"



#include "core.h"
#include "vidco.h" // Courtesy

#ifdef CORE_GL
	#include "core_opengl.h"
#endif // CORE_GL

#ifndef PLATFORM_OSX // Temp

// We might end up being a rather lonely file ?  Oct 17 2015 - didn't happen though.

sys_handle_t *Vid_Handle_Create_Solo_Client (const char *caption, int client_width, int client_height, required sys_handle_t *dc, required sys_handle_t *rc)
{
	sys_handle_t cw = NULL;
	sys_handle_t drawcontext;
	sys_handle_t glcontext;

	wdostyle_e request_style = wdostyle_normal; // WDO_CENTERED | WDO_RESIZABLE;
	crect_t desktop_rect = {0}, window_rect = {0};
	crectrb_t border;

	Vid_Desktop_Properties_Get (RECT_REPLY(desktop_rect));
	Vid_Handle_Borders_Get (request_style, false /* no have menu*/, RECTRB_REPLY(border) );

	window_rect = crect_t_centered(client_width + border.width, client_height + border.height, desktop_rect);

	cw = Vid_Handle_Create (NULL, caption, window_rect, request_style, false /*no menu for solo*/, &drawcontext, &glcontext);

	REQUIRED_ASSIGN (dc, drawcontext);
	REQUIRED_ASSIGN (rc, glcontext);

	return cw;
}



void Vid_Handle_Caption (sys_handle_t cw, const char *fmt, ...)
{
	void _Vid_Handle_Caption (sys_handle_t cw, const char *text);
	if (cw) {
		VA_EXPAND_ALLOC (text, length, bufsiz, fmt);
		_Vid_Handle_Caption (cw, text);
		free (text); // VA_EXPAND_ALLOC free
	}
}

void Vid_Handle_Client_Rect_To_Window_Rect (wdostyle_e style,  cbool have_menu, modify int *left, modify int *top, modify int *width, modify int *height)
{
	crect_t border;

	Vid_Handle_Borders_Get (style, have_menu, RECT_REPLY (border), NULL, NULL);

	REQUIRED_ASSIGN (left,   *left   - border.left);
	REQUIRED_ASSIGN (top,    *top    - border.top);
	REQUIRED_ASSIGN (width,  *width  + border.width);
	REQUIRED_ASSIGN (height, *height + border.height);
}







void Window_Borders_Get (vid_t *vid, reply int *left, reply int *top, reply int *width, reply int *height, reply int *right, reply int *bottom)
{
#pragma message ("Surprisingly I am empty")
}


void Window_Caption (vid_t *vid, const char *fmt, ...) //__core_attribute__((__format__(__printf__,2,3)))
{
	void _Vid_Handle_Caption (sys_handle_t cw, const char *text);
	VA_EXPAND_ALLOC (text, length, bufsiz, fmt);
	_Vid_Handle_Caption (vid->wdo, text);
	free (text); // VA_EXPAND_ALLOC free
}


void Window_Client_Rect_Get (vid_t *vid, reply int *left, reply int *top, reply int *width, reply int *height)
{
	Vid_Handle_Client_RectRB_Get (vid->wdo, left, top, width, height, NULL, NULL);
}


void Window_Client_Rect_To_Window_Rect (vid_t *vid, modify int *left, modify int *top, modify int *width, modify int *height)
{
	Vid_Handle_Client_Rect_To_Window_Rect (vid->style, vid->menu != NULL, left, top, width, height);
}


void Window_Client_RectRB_Get (vid_t *vid, reply int *left, reply int *top, reply int *width, reply int *height, reply int *right, reply int *bottom)
{
	Vid_Handle_Client_RectRB_Get (vid->wdo, left, top, width, height, right, bottom);
}






void Window_Hide (vid_t *vid)
{
	Vid_Handle_Hide (vid->wdo);
}



void Window_MousePointer (vid_t *vid, mousepointer_e mousepointer)
{
	Vid_Handle_MousePointer (vid->wdo, &vid->hmousepointer, mousepointer);
}


void Window_Move (vid_t *vid, int left, int top, int width, int height)
{
	Vid_Handle_Move (vid->wdo, vid->style, vid->menu != NULL, left, top, width, height);
}


void Window_MinSize (vid_t *vid, int width, int height)
{
	Vid_Handle_MinSize (vid->wdo, width, height);
}


void Window_MaxSize (vid_t *vid, int width, int height)
{
	Vid_Handle_MaxSize (vid->wdo, width, height);
}


void Window_Show (vid_t *vid)
{
	Vid_Handle_Show (vid->wdo);
}


void Window_SwapBuffers (vid_t *vid)
{
	Vid_Handle_SwapBuffers (vid->wdo, vid->dc, vid->glrc);
}


void Window_ZOrder (vid_t *vid)
{
	Vid_Handle_ZOrder (vid->wdo);
}


#ifdef CORE_GL
	void Window_MakeCurrent (vid_t *vid)
	{
		if (Vid_Handle_Context_Get() != vid->glrc)
			Vid_Handle_Context_Set (vid->wdo, vid->dc, vid->glrc);
	}
#endif // CORE_GL


#endif // #ifndef PLATFORM_OSX  not ready yet

// Special case of needed to be available for platform osx even now

#ifdef CORE_GL
	void *Vid_GL_GetProcAddress(const char *pfunction_name)
	{
		#if defined(CORE_SDL)
			return SDL_GL_GetProcAddress (pfunction_name);
		#elif defined(PLATFORM_WINDOWS)
			return (void *)ewglGetProcAddress (pfunction_name);  // Could be OpenGL or could be a wrapper
		#else
			void *_Platform_GetProcAddress (const char *pfunction_name);
			return _Platform_GetProcAddress (pfunction_name);
		#endif
	}


#endif // CORE_GL
