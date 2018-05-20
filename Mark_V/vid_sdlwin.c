#ifdef CORE_GL // WinQuake on GL
#ifndef GLQUAKE // NOT GLQUAKE
#ifdef CORE_SDL // SDL specific

/*
Copyright (C) 2009-2013 Baker

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
// vid_wgl.c -- server code for moving users


#include "quakedef.h"
#include "sdlquake.h"
static void VID_WinQuake_AdjustBuffers (vmode_t *mode);
static cbool VID_CreateDIB (int width, int height, byte *palette);

//
// miscelleanous init
//

// Runs once.
void VID_Local_Window_PreSetup (void)
{
	#define IDI_ICON1 1 // It's always just 1.
	Shell_Platform_Icon_Load ( (void *)IDI_ICON1); // Windows

    // I don't think we need anything here
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
        System_Error("Could not initialize SDL Video");


}


vmode_t VID_Local_GetDesktopProperties (void)
{
    SDL_DisplayMode mode;
    vmode_t desktop = {0};

    if (SDL_GetDesktopDisplayMode(0, &mode) != 0)
        System_Error("Could not get desktop display mode");

	desktop.type		=	MODE_FULLSCREEN;
	desktop.width		=	mode.w;
	desktop.height		=	mode.h;
	desktop.bpp			=	SDL_BITSPERPIXEL(mode.format);

	return desktop;
}

//
// vsync
//

// Sole caller is GL_CheckExtensions.  No fucking kidding.  It's true.
//GLint (APIENTRY *qglXSwapIntervalSGI)(GLint interval);

cbool VID_Local_Vsync_Init (const char *gl_extensions_str)
{
#if 0
// Linux ...
// Look for the Linux hints and the Windows hints
// WGL_EXT_swap_control
//

	if ( strstr( glConfig.extensions_string, "GLX_SGI_swap_control" ) )
    qglXSwapIntervalSGI = GL_GetProcAddress("glXSwapIntervalSGI");

	if (strstr(gl_extensions_str, "GL_EXT_swap_control") ||
		strstr(gl_extensions_str, "GL_WIN_swap_hint") 	 ||
		strstr(gl_extensions_str, "GLX_SGI_swap_control") ||
		strstr(gl_extensions_str, "GLX_SGI_swap_control") ||
		) {

		sysplat.wglSwapIntervalEXT = (SETSWAPFUNC) glXGetProcAddress("wglSwapIntervalEXT");
		//sysplat.wglGetSwapIntervalEXT = (GETSWAPFUNC) glXGetProcAddress("wglGetSwapIntervalEXT");

	}
glXGetProcAddress
// glXGetProcAddress("wglSwapIntervalEXT") // glSwapIntervalEXT?
// glXGetProcAddress("wglGetSwapIntervalEXT")
// glXSwapIntervalSGI
// GLX_SGI_swap_control
// glXSwapIntervalSGI
// WGL_EXT_swap_control, GLX_SGI_swap_control, GLX_SGI_swap_control, WGL_EXT_swap_control, GL_WIN_swap_hint


	return SDL_GL_GetSwapInterval() == -1 ? false : true;
#endif // 0 vsync
	return false;
}

void VID_Local_Vsync (void)
{
#if 0 // I see no evidence this actually works.  On the plus side, reuse context appears to work fine.  Platform it, I suppose.
	if (renderer.gl_swap_control) {
		cbool b_set_swap = vid_vsync.value ? 1 : 0;
		int result = -1;

		//else
		if (vid.canalttab /* if we can't, we are in window restart*/) {
			int reuseok = -1; // 0 is success
			SDL_GLContext hRC = SDL_GL_GetCurrentContext(); // HGLRC hRC = ewglGetCurrentContext();
			SDL_Surface *hDC = SDL_GetWindowSurface (sysplat.mainwindow);
			SDL_GL_MakeCurrent (0,0); // Really? ewglMakeCurrent(NULL, NULL);
			SDL_FreeSurface (hDC); hDC = NULL; // ReleaseDC(sysplat.mainwindow, hDC);
			if (hRC && (reuseok = SDL_GL_MakeCurrent (sysplat.mainwindow, hRC)) != 0)
			{
				// Tried to reuse context and it failed
				//ewglDeleteContext (wglHRC);
				//wglHRC = NULL;
				System_Error ("Context reuse failed.  Must reload textures.");
			}
			result = SDL_GL_SetSwapInterval (b_set_swap);

			if (result == -1)
				Con_PrintLinef ("VID_Vsync_f: set failed on %d", b_set_swap);

		}

	}
#endif // 0 vsync
	return;	// Obvious, haha
}

void VID_Local_Vsync_f (cvar_t *var)
{
	VID_Local_Vsync ();
}


void VID_Local_Multisample_f (cvar_t *var)
{
	// Unsupported - Wouldn't do anything in WinQuake GL anyway
}


//
// vid modes
//


void VID_Local_AddFullscreenModes (void)
{
    const int num_sdl_modes = SDL_GetNumDisplayModes(0);
	int	i;

	for (i = 0;  i < num_sdl_modes && vid.nummodes < MAX_MODE_LIST; i++)
	{
	    SDL_DisplayMode mode;

		if (SDL_GetDisplayMode(0, i, &mode) == 0)
		{
            vmode_t test	= { MODE_FULLSCREEN, mode.w, mode.h, SDL_BITSPERPIXEL(mode.format)};
            cbool width_ok	= in_range (MIN_MODE_WIDTH_640,  test.width, MAX_MODE_WIDTH_10000 );
            cbool height_ok	= in_range (MIN_MODE_HEIGHT_400, test.height, MAX_MODE_HEIGHT_10000);
            cbool bpp_ok	= (test.bpp == vid.desktop.bpp);
            cbool qualified	= (bpp_ok && width_ok && height_ok);

            if (qualified && !VID_Mode_Exists(&test, NULL))
            {
                memcpy (&vid.modelist[vid.nummodes++], &test, sizeof(vmode_t) );
//                alert ("Added %d x %d %d", vid.modelist[vid.nummodes-1].width, vid.modelist[vid.nummodes-1].height, vid.modelist[vid.nummodes-1].bpp);
            }

		}
		else System_Error ("Couldn't get display mode");

	}
}


// Baker: begin resize window on the fly
void VID_BeginRendering_Resize_Think_Resize_Act (void)
{
	mrect_t windowinfo;

	SDL_GetWindowPosition (sysplat.mainwindow, &windowinfo.left, &windowinfo.top);
	SDL_GetWindowSize (sysplat.mainwindow, &windowinfo.width, &windowinfo.height);

	// Need to catch minimized scenario
	// Fill in top left, bottom, right, center
	vid.client_window.left = windowinfo.left;
	vid.client_window.right = windowinfo.left + windowinfo.width;
	vid.client_window.bottom = windowinfo.top + windowinfo.height;
	vid.client_window.top = windowinfo.top;
	vid.client_window.width = windowinfo.width;
	vid.client_window.height = windowinfo.height;


#ifndef DIRECT3D8_WRAPPER // dx8 - Not for Direct3D 8! (-resizable)  Keep in mind we are in a windows source file! vid_wgl.c
	if (1 /*COM_CheckParm ("-resizable")*/)
	{
		vid.screen.width = vid.client_window.width;
		vid.screen.height = vid.client_window.height;
		VID_WinQuake_AdjustBuffers (&vid.screen);
		eglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);
	}
#endif // DIRECT3D8_WRAPPER (oldwater)
}
// End resize window on the fly

/*
================
VID_SDL2_GetDisplayMode

Returns a pointer to a statically allocated SDL_DisplayMode structure
if there is one with the requested params on the default display.
Otherwise returns NULL.

This is passed to SDL_SetWindowDisplayMode to specify a pixel format
with the requested bpp. If we didn't care about bpp we could just pass NULL.
================
*/
static SDL_DisplayMode *VID_SDL2_GetDisplayMode(int width, int height, int bpp)
{
	static SDL_DisplayMode mode;
	const int sdlmodes = SDL_GetNumDisplayModes(0);
	int i;

	for (i = 0; i < sdlmodes; i++)
	{
		if (SDL_GetDisplayMode(0, i, &mode) == 0
			&& mode.w == width && mode.h == height
			&& (int)SDL_BITSPERPIXEL(mode.format) == bpp)
		{
			return &mode;
		}
	}
	return NULL;
}


// Returns false if need to do GL setup again.
cbool VID_Local_SetMode (int modenum)
{
// Ok, my platform neutral way does not appear to be optimal for SDL so am going to try it more like the "Quakespasm" way, which is more compatible with the SDL philosophy.
	vmode_t *p 			= &vid.modelist[modenum];
	cbool do_bordered	= p->type == MODE_WINDOWED && (p->width != vid.desktop.width || p->height != vid.desktop.height);
	cbool first			= !sysplat.mainwindow;







	cbool reuseok = false;
//	cbool bordered	= p->type   == MODE_WINDOWED &&
	//					  (p->width  != vid.desktop.width ||
		//				  p->height != vid.desktop.height);
	cbool restart	= (sysplat.mainwindow != NULL);

	// Preserve these for hopeful reuse.
	SDL_GLContext wglHRC 	= restart ? SDL_GL_GetCurrentContext() : 0; // HGLRC hRC = ewglGetCurrentContext();
	SDL_Surface *hDC 	= restart ? SDL_GetWindowSurface (sysplat.mainwindow) : 0; // hdc = ewglGetCurrentContext()

//	vmode_t *p = &vid.modelist[modenum];
	int vid_sdl_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN;
//	if (p->type == MODE_FULLSCREEN) vid_sdl_flags |= SDL_WINDOW_FULLSCREEN;
	if (p->type == MODE_FULLSCREEN) {
            vid_sdl_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        p = &vid.desktop;
	}


// Baker: begin resize window on the fly
	if (p->type == MODE_WINDOWED)   vid_sdl_flags |= SDL_WINDOW_RESIZABLE;

// End resize window on the fly

	if (restart)
		VID_Local_Window_Renderer_Teardown (TEARDOWN_FULL_1, true /*reset video mode*/);  // TEARDOWN_NO_DELETE_GL_CONTEXT_0
#pragma message ("^^ that is egregious ... TEARDOWN_FULL_1 ... OOF!  But yeah, if no delete context isn't working .. :(")

	// This is intended to set the fullscreen display but SDL should do that for us.
//	if (p->type == MODE_FULLSCREEN)
//		WIN_Change_DisplaySettings (modenum);
// SDL_SetWindowDisplayMode (window, p) // If we do need to set it?

// Baker: begin resize window on the fly
/*
	AdjustWindowRectEx (&window_rect, WindowStyle, FALSE, ExWindowStyle);  // Adds the borders
	// Window width - Client width
	vid.border_width = (window_rect.right - window_rect.left) - client_rect.right;
	vid.border_height = (window_rect.bottom - window_rect.top) - client_rect.bottom;
*/
// End resize window on the fly

/*
	WIN_AdjustRectToCenterScreen(&window_rect);

#if 1
	// Windows 8 introduces chaos :(
	if (restart && p->type != vid.screen.type)
	{
		DestroyWindow (sysplat.mainwindow);
		sysplat.mainwindow = 0;
	}
#endif
*/

	// ugh ... for SDL we need to reuse the window and we just size it :(
	// I prefer that method but I'll have to rewrite stuff now :(
#if 1 // For now.  Focus on other stuff.
	if (sysplat.mainwindow) {
		SDL_DestroyWindow (sysplat.mainwindow); sysplat.mainwindow = 0;
		sysplat.draw_gl_context = 0; // Because this action kills the context with SDL too.
	}
#endif

	// Note that we are not constructing or resizing the existing window
	// It's ok because even on Windows I think we destroy it due to windows 8?

/*
	WIN_Construct_Or_Resize_Window (WindowStyle, ExWindowStyle, window_rect);

	// I think we do not need this?
	if (p->type == MODE_WINDOWED)
		SDL_SetWindowDisplayMode (NULL, 0) ; //sysplat.mainwindow,  (NULL, 0);

*/
//	if (p->type == MODE_WINDOWED) {
//		SDL_SetWindowDisplayMode (NULL, 0) ; //sysplat.mainwindow,  (NULL, 0);
//	}

	// Note that we are not constructing or resizing the existing window
	// It's ok because even on Windows I think we destroy it due to windows 8?
	//		SDL_Surface *hDC = SDL_GetWindowSurface (sysplat.mainwindow);
	//		SDL_GL_MakeCurrent (0,0); // Really? ewglMakeCurrent(NULL, NULL);
	//		SDL_FreeSurface (hDC); hDC = NULL; // ReleaseDC(sysplat.mainwindow, hDC);


		//VID_Local_Window_Renderer_Teardown (TEARDOWN_FULL_1, true /*reset video mode*/); // For now.

if (p->type == MODE_WINDOWED) {
    sysplat.mainwindow = SDL_CreateWindow(ENGINE_NAME,
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			p->width, p->height,
			vid_sdl_flags);
} else {
    sysplat.mainwindow = SDL_CreateWindow(ENGINE_NAME,
			0, 0,
			p->width, p->height,
			vid_sdl_flags | SDL_WINDOW_FULLSCREEN_DESKTOP);

}

#if 1
	// Apparently SDL goes out of its way to preserve the context!  Yay!
#if 1 // I ain't buying this
	if (!sysplat.draw_gl_context) {
		sysplat.draw_gl_context = SDL_GL_CreateContext(sysplat.mainwindow);
		if (!sysplat.draw_gl_context) {
            SDL_DestroyWindow (sysplat.mainwindow);
			System_Error ("Couldn't create context %d x %d @ %d bpp", p->width, p->height, p->bpp);
		}
	}
#endif

	// Do we need to attach it?  Apparently it automatically attaches to current window
#else
	if (  !(sysplat.draw_context = SDL_GL_CreateContext(sysplat.mainwindow))  )
        System_Error ("Couldn't create context %d x %d @ %d bpp", p->width, p->height, p->bpp);
#endif


//    SDL_WM_SetCaption("hi", "lo");

	// Is this including or excluding the border?
	// I suppose we'd have to enable resizable to find out.
	Shell_Platform_Icon_Window_Set (sysplat.mainwindow);
	SDL_SetWindowMinimumSize (sysplat.mainwindow, QWIDTH_MINIMUM_320, QHEIGHT_MINIMUM_2XX);



// Get focus if we can, get foreground, finish setup, pump messages.
// then sleep a little.
#if 0
	SDL_SetWindowDisplayMode (sysplat.mainwindow, /*seriously?? */ VID_SDL2_GetDisplayMode(p->width, p->height, vid.desktop.bpp));
	if (p->type == MODE_FULLSCREEN) {
		if (SDL_SetWindowFullscreen (sysplat.mainwindow, SDL_WINDOW_FULLSCREEN) != 0)
			System_Error ("Couldn't set fullscreen state mode");
	}
#endif

	SDL_ShowWindow (sysplat.mainwindow);
	// UpdateWindow is part of ShowWindow
	// SetWindowPos unnecessary I think
	SDL_RaiseWindow (sysplat.mainwindow);	// SetForegroundWindow equivalent, ZOrder it.
	Shell_Platform_Icon_Window_Set (sysplat.mainwindow);

#if 1
	if (p->type == MODE_WINDOWED) {
        SDL_SetWindowPosition (sysplat.mainwindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}
	else {
        SDL_SetWindowPosition (sysplat.mainwindow, 0, 0);
        SDL_SetWindowSize (sysplat.mainwindow, p->width, p->height);
	}
#endif


	{
		int j;
		for (j = 0; j < 3; j ++) {
			//Platform_Events_Do (); Not quite
			Input_Local_SendKeyEvents ();
			SDL_Delay (1);
		}
	}

	// Context reuse goes here.  We need to store the "HGLRC"
	// Then try to set it.

/*

	WIN_SetupPixelFormat (sysplat.draw_context); // Probably needs???
*/
	sysplat.draw_gl_context = SDL_GL_GetCurrentContext();
	if (!sysplat.draw_gl_context) {
		SDL_DestroyWindow (sysplat.mainwindow);
		System_Error ("There was no context created");
	}

#if 0
	if (wglHRC && (reuseok = SDL_GL_MakeCurrent (sysplat.mainwindow, wglHRC)) == 0)
	{
		// Tried to reuse context and it failed
		SDL_GL_DeleteContext (wglHRC); // ewglDeleteContext (wglHRC);
		wglHRC = NULL;
		log_debug /*Con_DPrintLinef*/ ("Context reuse failed.  Must reload textures.");
	}
#endif

	if (!wglHRC) {
		wglHRC = SDL_GL_CreateContext (sysplat.mainwindow);


		if (!wglHRC)
			System_Error ("Could not initialize GL (SDL_GL_CreateContext failed)." NEWLINE NEWLINE "Make sure you in are 65535 color mode, and try running -window.");
		if (SDL_GL_MakeCurrent( sysplat.mainwindow, wglHRC ) != 0)
			System_Error ("VID_Init: SDL_GL_MakeCurrent failed");
	}

	//SDL_GL_MakeCurrent (sysplat.mainwindow, draw_gl_context

#if 1
	if (!restart)
	{
		eglClear (GL_COLOR_BUFFER_BIT);
		VID_SwapBuffers ();
	}
#endif



// Are we able to re-use the context?
#if 0 // I'm think we do not need
	vid.ActiveApp = 1; // Crutch because we never activate.
#endif
	//return false;  //reuseok;

	vid.wingl.texslot = 0; // We killed it.

	Shell_Platform_Icon_Window_Set (sysplat.mainwindow);
	VID_WinQuake_AdjustBuffers (p);

	return reuseok;

}

//
// in game
//

void VID_Local_SwapBuffers (void)
{
    SDL_GL_SwapWindow (sysplat.mainwindow);
}


void VID_Local_Suspend (cbool bSuspend)
{
// SDL does nothing?  Interesting.
	if (bSuspend == false)
	{
//		eChangeDisplaySettings (&sysplat.gdevmode, CDS_FULLSCREEN);
//		ShowWindow(sysplat.mainwindow, SW_SHOWNORMAL);
//		MoveWindow(sysplat.mainwindow, 0, 0, sysplat.gdevmode.dmPelsWidth, sysplat.gdevmode.dmPelsHeight, false); //johnfitz -- alt-tab fix via Baker
	} //else  eChangeDisplaySettings (NULL, 0);
	// case SDL_ACTIVEEVENT: SDL handles the gamma
}

//
// window setup
//


cbool SDL_SetupPixelFormat (void)
{
//	24,						// 24-bit color depth
//	32,						// 32-bit z-buffer
//	8,						// 8-bit stencil buffer
//	if (!sysplat.multisamples)
/*	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, fsaa > 0 ? 1 : 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, fsaa);

	draw_context = SDL_SetVideoMode(width, height, bpp, flags);
	if (!draw_context) { // scale back fsaa
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
		draw_context = SDL_SetVideoMode(width, height, bpp, flags);
	}
	if (!draw_context) { // scale back SDL_GL_DEPTH_SIZE
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
		draw_context = SDL_SetVideoMode(width, height, bpp, flags);
		if (!draw_context)
			Sys_Error ("Couldn't set video mode");
	}
// read the obtained z-buffer depth
	if (SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depthbits) == -1)
		depthbits = 0;

// read obtained fsaa samples
	if (SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &fsaa_obtained) == -1)
		fsaa_obtained = 0;

// GLMaxSize?*/
    return true;
}

#define MSVC_IS_BEING_DUMB // and giving me heap corruption but GCC isn't.  And I think if I tack on "CORE_GL" to WinQuake debug it is ok too ... dumb
							// Will we eventually discover it's something else?  Project setting or something?  Who knows!

#ifdef MSVC_IS_BEING_DUMB
static pixel_t /*byte*/ my_static_buffero[1024][512]; // Legal?
#endif // MSVC_IS_BEING_DUMB

cbool VID_CreateDIB (int newwidth, int newheight, byte *palette)
{
	// vid.conwidth, vid.conheight = newwidth and newheight
	GLenum glerrorcode = 0;
#if 0
	if (!sysplat.draw_context)
		System_Error ("No GL context for WinQuake GL");
#endif
	//ewglMakeCurrent (sysplat.draw_context);

	if (0 && vid.wingl.texslot) {
		// THIS CAUSES A HANG DELAY.
		// free and zero related resources.  glDeleteTextures does not zero it out, we must do that.
        eglDeleteTextures (1, &vid.wingl.texslot); vid.wingl.texslot = 0;
	}

	if (vid.wingl.rgbabuffer) {
		free (vid.wingl.rgbabuffer);  vid.wingl.rgbabuffer = NULL;
	}

	if (vid.wingl.pixelbytes) {
#ifdef MSVC_IS_BEING_DUMB
		// Static buffer if we can
#else
		free (vid.wingl.pixelbytes);
#endif
			vid.wingl.pixelbytes = vid.buffer = NULL;

		vid.wingl.s1 = vid.wingl.t1 = vid.wingl.width_pow2 = vid.wingl.height_pow2 = 0;
	}
	else {
		// No texture
		glerrorcode = glerrorcode;
	}


	vid.wingl.w				= vid.rowbytes = newwidth;															// Must set vid.rowbytes
	vid.wingl.h				= newheight;
    vid.wingl.width_pow2	= newwidth; // Image_Power_Of_Two_Size (newwidth);
    vid.wingl.height_pow2	= newheight; //Image_Power_Of_Two_Size (newheight);

	//vid.wingl.numpels		= vid.wingl.w * vid.wingl.h;
	vid.wingl.numpels		= vid.wingl.width_pow2 * vid.wingl.height_pow2; // 1024 x 512

	vid.wingl.rgbabuffer	= calloc (vid.wingl.numpels, sizeof(unsigned));
#ifdef MSVC_IS_BEING_DUMB // Static buffer
	vid.wingl.pixelbytes	= vid.buffer = my_static_buffero;
#else
	vid.wingl.pixelbytes	= vid.buffer = calloc (vid.wingl.numpels, sizeof(pixel_t) /*which is byte*/ );		// Must set vid.buffer
#endif

    vid.wingl.s1			= newwidth  / (float)vid.wingl.width_pow2;
    vid.wingl.t1			= newheight / (float)vid.wingl.height_pow2;

    eglGenTextures (1, &vid.wingl.texslot);

	if (!vid.wingl.texslot) {
#if 1 // PLATFORM_LINUX ... errors during video setup don't present well with SDL on Linux
		// The window is in the way :(
        static unsigned int planb = 1000;
        vid.wingl.texslot = planb ++;
        if (0) {
		SDL_DestroyWindow (sysplat.mainwindow);
		System_Error ("No texture slot for WinQuake GL, window is %p and context is %p", sysplat.mainwindow, sysplat.draw_gl_context);
        }
        #else

		System_Error ("No texture slot for WinQuake GL");
		#endif

	}

    eglBindTexture	(GL_TEXTURE_2D, vid.wingl.texslot);
    eglEnable		(GL_TEXTURE_2D);
    eglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    eglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    eglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    eglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    //glGetError();
#if 0
	eglTexImage2D (GL_PROXY_TEXTURE_2D, /* mip level */ 0, GL_COLOR_INDEX8_EXT, vid.wingl.width_pow2, vid.wingl.height_pow2, /* border */ 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
#else
    eglTexImage2D (GL_PROXY_TEXTURE_2D, /* mip level */ 0, GL_RGBA, vid.wingl.width_pow2, vid.wingl.height_pow2, /* border */ 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
#endif

    //error = glGetError();
	{	int actualTexWidth = -1, actualTexHeight = -1;
		glGetTexLevelParameteriv (GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &actualTexWidth);
		glGetTexLevelParameteriv (GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &actualTexHeight);
		actualTexHeight = actualTexHeight;
	}

	// We specify a power of 2 size.  But later when we upload, we specify a different size.  Will Direct3D wrapper be ok with that?
	// If not, demand NPOT for Direct3D (which means we need the D3D9 wrapper).  Or rewrite updater to row copy transfer.
#if 0
	eglTexImage2D (GL_TEXTURE_2D, /* mip level */ 0, GL_COLOR_INDEX8_EXT, vid.wingl.width_pow2, vid.wingl.height_pow2, /* border */0, GL_RGBA, GL_UNSIGNED_BYTE, /* pixels */ vid.wingl.rgbabuffer);
#else
    eglTexImage2D (GL_TEXTURE_2D, /* mip level */ 0, GL_RGBA, vid.wingl.width_pow2, vid.wingl.height_pow2, /* border */0, GL_RGBA, GL_UNSIGNED_BYTE, /* pixels */ vid.wingl.rgbabuffer);
#endif
	return true;
}


//
// window teardown
//

void VID_Local_Window_Renderer_Teardown (int destroy, cbool reset_video_mode)
{
#if 1
	// destroy = 1 = TEARDOWN_FULL else TEARDOWN_NO_DELETE_GL_CONTEXT (don't destroy the context or destroy window)
	SDL_GLContext hRC = SDL_GL_GetCurrentContext(); // HGLRC hRC = ewglGetCurrentContext();
	SDL_Surface *hDC = SDL_GetWindowSurface (sysplat.mainwindow);

	SDL_GL_MakeCurrent (NULL, NULL); // Really? ewglMakeCurrent(NULL, NULL);

	if (hRC && destroy)		{ SDL_GL_DeleteContext(sysplat.draw_gl_context); sysplat.draw_gl_context = NULL; }
	if (hDC)				{ SDL_FreeSurface (hDC); hDC = NULL; } // ReleaseDC(sysplat.mainwindow, hDC);

	// Draw context for SDL is hGLRC not an hDC
	sysplat.draw_gl_context = NULL;	// SDL_FreeSurface (*draw_context);  But I think destroywindow does this.
#endif
	if (destroy) {


		SDL_DestroyWindow (sysplat.mainwindow);
		sysplat.mainwindow = NULL;
	}

	// Change display settings?
	SDL_SetWindowDisplayMode (NULL, 0) ;
}






static void VID_WinQuake_AdjustBuffers (vmode_t *p)
{
	// Uses screen.

	// If the following doesn't scream r_misc.c I don't know what does?

	// Find best integral factor, set both the x and the y.  This wants to be 1.  But a giant mode like 6000 x 2000 would generate 2.

	for (vid.stretch_x = 1; p->width  / vid.stretch_x > WINQUAKE_MAX_WIDTH_3000 ; vid.stretch_x ++);
	for (vid.stretch_y = 1; p->height / vid.stretch_y > WINQUAKE_MAX_HEIGHT_1080; vid.stretch_y ++);

	vid.stretch_old_cvar_val = (int)vid_sw_stretch.value; // This isn't the actual stretch, but the cvar value attempted.
	// Ok we need to validate this.
	// Let's say I want 4.  I can't have 4 in 640x480.  /320  /240  highx = (int)(p->width / 320);

	vid.stretch_x = vid.stretch_y = c_max (vid.stretch_x, vid.stretch_y); // Take the larger of the 2.  Lowest it can be.
	{
		int high_x   = (int)(p->width  / 320);
		int high_y   = (int)(p->height / 240);
		int high_any = c_min (high_x, high_y);

		//int stretch_try = vid.stretch_old_cvar_val;
#if 1 // WINQUAKE-GL EXCEPTION since it is so fucking slow ...
		int stretch_try = CLAMP(1, vid.stretch_old_cvar_val, 2);
#else
		int stretch_try = CLAMP(0, vid.stretch_old_cvar_val, 2);
#endif

		switch (stretch_try) {
		case 0:	stretch_try = 1; break;
		case 2:	stretch_try = 9999; break;
		case 1:	stretch_try = (int)(high_any / 2.0 + 0.5); break;
		}

		if (stretch_try > high_any)
			stretch_try = high_any;

		if (stretch_try < vid.stretch_x)
			stretch_try = vid.stretch_x;

		vid.stretch_x = vid.stretch_y = stretch_try;
	}

	vid.conwidth  = p->width  / vid.stretch_x;
	vid.conheight  = p->height  / vid.stretch_y;

	vid.aspect = ((float) vid.conwidth / (float) vid.conheight) * (320.0 / 240.0); // toxic

// UH?  NO!! 	VID_Local_Window_Renderer_Teardown (TEARDOWN_NO_DELETE_GL_CONTEXT_0, false /*do not reset video mode*/); // restart the DIB

	VID_CreateDIB (vid.conwidth, vid.conheight, vid.curpal);

	VID_WinQuake_AllocBuffers_D_InitCaches (vid.conwidth, vid.conheight); // It never returns false.


}

//
//
//
// Equivalent of swap buffers
//
//
//

// Called exclusively by vid.c VID_SwapBuffers
void VID_Update (vrect_t *rects)
{
	// For now, we are going to lazily update the whole thing.
    if (!vid.wingl.rgbabuffer)
        return; // Don't have one.

	if (1) {
		// copy the rendered scene to the texture buffer:
		int n; for (n = 0; n < vid.wingl.numpels; n ++) {
			//vid.wingl.rgbabuffer[n] = -1 ;
			vid.wingl.rgbabuffer[n] = vid.wingl.rgbapal[vid.wingl.pixelbytes[n]];
//			if (vid.wingl.rgbabuffer[n])
//				n=n;
		}

		{
			eglBindTexture		(GL_TEXTURE_2D, vid.wingl.texslot);
			eglTexSubImage2D	(GL_TEXTURE_2D, 0 /*level*/, /*offset x, y */ 0, 0, vid.wingl.w, vid.wingl.h, GL_RGBA, GL_UNSIGNED_BYTE, vid.wingl.rgbabuffer);


			eglViewport			(0, 0, vid.screen.width, vid.screen.height);

			eglMatrixMode		(GL_PROJECTION);
			eglLoadIdentity		();
			eglOrtho			(0, 1, 1, 0, -1, 1);
			eglMatrixMode		(GL_MODELVIEW);
			eglLoadIdentity		();
			eglClearColor		(0, 0, 0, 0);
			eglClear			(GL_COLOR_BUFFER_BIT);

			eglEnable			(GL_TEXTURE_2D);
			eglBindTexture		(GL_TEXTURE_2D, vid.wingl.texslot);
			eglColor3f			(1, 1, 1);

			eglBegin			(GL_TRIANGLE_STRIP);
			eglTexCoord2f		(vid.wingl.s1, 0);
			eglVertex2f			(1, 0);

			eglTexCoord2f		(0, 0);
			eglVertex2f			(0, 0);

			eglTexCoord2f		(vid.wingl.s1, vid.wingl.t1);
			eglVertex2f			(1, 1);

			eglTexCoord2f		(0, vid.wingl.t1);
			eglVertex2f			(0, 1);
			eglEnd();
		}
	}
	// Done.  Still need to flip
}


//
//
//
// Palette Set
//
//
//

// Startup/Gamedir change initiates this.  MH Windows WinQuake has to play with the palette here.  All WinQuake implementations must generate alpha50 map.
void VID_Local_Modify_Palette (byte *palette)
{
	// Although everyone needs to generate an alphamap table.
#ifdef WINQUAKE_QBISM_ALPHAMAP
	R_WinQuake_Generate_Alpha50_Map (vid.alpha50map);
#endif // WINQUAKE_QBISM_ALPHAMAP
}

// This gets called by vid.c - SetMode and ShiftPalette(View_UpdateBlend)
void VID_Local_SetPalette (byte *palette)
{
	int c;
	for (c = 0; c < PALETTE_COLORS_256; c ++) {
        byte red				= palette[c * 3 + 0];
        byte green				= palette[c * 3 + 1];
        byte blue				= palette[c * 3 + 2];
        byte alpha				= 0xFF;
        unsigned color			= (red <<  0) + (green <<  8) + (blue << 16) + (alpha << 24);
        vid.wingl.rgbapal[c]	= color;
    }
}


//
//  Window
//


void VID_Local_Set_Window_Caption (const char *text)
{
    const char *new_caption = text ? text : ENGINE_NAME;

    if (!sysplat.mainwindow)
        return;

    SDL_SetWindowTitle (sysplat.mainwindow, new_caption);
}


void VID_Local_Shutdown (void)
{
    VID_Local_Window_Renderer_Teardown (TEARDOWN_FULL_1, true /*reset video mode*/);

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}


// This function gets called before anything happens
void VID_Local_Init (void)
{
    // Because we want the console log available!
#ifndef CORE_SDL_STATIC // (test it without first)
	{
#define SDL_REQUIRED_MAJOR_2 2
#define SDL_REQUIRED_MINOR_0 0
#define SDL_REQUIRED_PATCH_4 4
		SDL_version compiled, linked;

		SDL_VERSION (&compiled);
		SDL_GetVersion (&linked);

		//Con_DPrintLinef ("SDL: We compiled against SDL version %d.%d.%d ...", compiled.major, compiled.minor, compiled.patch);
		//alert ("SDL: But we are linking against SDL version %d.%d.%d.", linked.major, linked.minor, linked.patch);
		// Print SDL version to log, if doesn't match error out.
		// No reason to run with SDL version mismatch.
		// Question: What if we are statically linked
		Con_DebugLogLine ("SDL version: %d.%d.%d", linked.major, linked.minor, linked.patch);
		if (linked.major < SDL_REQUIRED_MAJOR_2 || (linked.major == SDL_REQUIRED_MAJOR_2 && linked.minor < SDL_REQUIRED_MINOR_0) || (linked.major == SDL_REQUIRED_MAJOR_2 && linked.minor == SDL_REQUIRED_MINOR_0 && linked.patch < SDL_REQUIRED_PATCH_4))
			System_Error ("SDL %d.%d.%d is minimum required version but SDL version is %d.%d.%d.  Please update your SDL2.", SDL_REQUIRED_MAJOR_2, SDL_REQUIRED_MINOR_0, SDL_REQUIRED_PATCH_4, linked.major, linked.minor, linked.patch);
	}
#endif // CORE_SDL_STATIC

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
        System_Error("Could not initialize SDL Video");

//    SDL_putenv("SDL_VIDEO_CENTERED=center"); // Is this SDL 1.2 or something?  Yes.  We do not need.


	VID_Renderer_Setup (); // Hooks up our GL functions
// Early
#ifdef WINQUAKE_RENDERER_SUPPORT
	VID_Palette_NewGame ();
#endif // WINQUAKE_RENDERER_SUPPORT


}

#endif // CORE_SDL
#endif // NOT GLQUAKE
#endif // WinQuake on GL
