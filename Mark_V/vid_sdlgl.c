#ifdef GLQUAKE // GLQUAKE specific
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

//#ifdef PLATFORM_WINDOWS // Right?
//#include "resource.h" // IDI_ICON2
//#endif

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
#pragma message ("Baker: What if it isn't supported?  Like if we don't bother on a Mac?")
	if (host_initialized) {
		Con_PrintLinef ("%s set to " QUOTED_S ".  requires engine restart.", var->name, var->string);
		Con_PrintLinef ("Note settings are: 2, 4, 8 and 0");
	}
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
		vid.consize_stale = true; // This triggers a cascade of recalculations in SCR_UpdateScreen
		vid.warp_stale = true; // Means warp needs recalculated.
#ifdef DIRECT3D9_WRAPPER
#endif // DIRECT3D9_WRAPPER
		//vid.mouse_resized = true;  // We don't really have a way of knowing this easily.
	}
#endif // DIRECT3D8_WRAPPER
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
	cbool do_bordered	= p->type == MODE_WINDOWED; // && (p->width != vid.desktop.width || p->height != vid.desktop.height);
	cbool first			= !sysplat.mainwindow;

#if 1
    // Attributes -- stencil, multisample, anything else?
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);	// Set depth 24 bits
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);	// Set depth 24 bits
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetSwapInterval (!!vid_vsync.value);
	//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, fsaa > 0 ? 1 : 0);
	///SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, fsaa);

	// Create or resize?
	if (!sysplat.mainwindow) {
		// Create
		int vid_extra_flags	= (p->type == MODE_FULLSCREEN) ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_RESIZABLE; // Window resize on fly
		int vid_sdl_flags 	= SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE; // If I am understanding this right, fullscreen won't get the border.
		if (p->type == MODE_FULLSCREEN) vid_sdl_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

		// Let's go with hidden window.

		sysplat.mainwindow = SDL_CreateWindow(ENGINE_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, p->width, p->height, vid_sdl_flags);

		if (!sysplat.mainwindow)
			System_Error ("Could not create window");

		//Shell_Platform_Icon_Window_Set (sysplat.mainwindow);
		//SDL_SetWindowMinimumSize (sysplat.mainwindow, QWIDTH_MINIMUM_320, QHEIGHT_MINIMUM_2XX);

	}

	// Quakespasm makes sure the window isn't fullscreen here.  Perhaps because it cannot alter the size if so.
	if (SDL_GetWindowFlags(sysplat.mainwindow) & SDL_WINDOW_FULLSCREEN_DESKTOP) {
		#define SDL_WINDOWED_MODE_0 0
		if (SDL_SetWindowFullscreen (sysplat.mainwindow, SDL_WINDOWED_MODE_0) != 0)
			System_Error ("Couldn't set fullscreen");
	}

	// Quakespasm now sizes the window
	SDL_SetWindowSize (sysplat.mainwindow, p->width, p->height);
	SDL_SetWindowPosition (sysplat.mainwindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_SetWindowDisplayMode (sysplat.mainwindow, /*seriously?? */ VID_SDL2_GetDisplayMode(p->width, p->height, vid.desktop.bpp));
	SDL_SetWindowBordered (sysplat.mainwindow, do_bordered ? SDL_TRUE : SDL_FALSE); // SDL docs says fullscreen will ignore.

	if (p->type == MODE_FULLSCREEN) {
		if (SDL_SetWindowFullscreen (sysplat.mainwindow, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0)
			System_Error ("Couldn't set fullscreen state mode");
	}

	Shell_Platform_Icon_Window_Set (sysplat.mainwindow);
	SDL_SetWindowMinimumSize (sysplat.mainwindow, QWIDTH_MINIMUM_320, QHEIGHT_MINIMUM_2XX);
	SDL_ShowWindow (sysplat.mainwindow);
	SDL_SetWindowBordered (sysplat.mainwindow, do_bordered ? SDL_TRUE : SDL_FALSE); // Hit it again :(
	Shell_Platform_Icon_Window_Set (sysplat.mainwindow);

	// Create GL context if needed
	if (!sysplat.draw_gl_context) {
		sysplat.draw_gl_context = SDL_GL_CreateContext(sysplat.mainwindow);
		if (!sysplat.draw_gl_context)
			System_Error("Couldn't create GL context");
	}

	SDL_GL_SetSwapInterval (!!vid_vsync.value);

	if (!first)
		VID_Resize_Check (2);

//	vid.ActiveApp = 1; // Crutch because we never activate.  Unneeded now?

	return true;

#else






	cbool reuseok = false;
//	cbool bordered	= p->type   == MODE_WINDOWED &&
	//					  (p->width  != vid.desktop.width ||
		//				  p->height != vid.desktop.height);
	cbool restart	= (sysplat.mainwindow != NULL);

	// Preserve these for hopeful reuse.
	SDL_GLContext wglHRC 	= restart ? SDL_GL_GetCurrentContext() : 0; // HGLRC hRC = ewglGetCurrentContext();
	SDL_Surface *hDC 	= restart ? SDL_GetWindowSurface (sysplat.mainwindow) : 0; // hdc = ewglGetCurrentContext()

	vmode_t *p = &vid.modelist[modenum];
	int vid_sdl_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN;
	if (p->type == MODE_FULLSCREEN) vid_sdl_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;


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
	SDL_DestroyWindow (sysplat.mainwindow); sysplat.mainwindow = 0;
#endif

	// Note that we are not constructing or resizing the existing window
	// It's ok because even on Windows I think we destroy it due to windows 8?

/*
	WIN_Construct_Or_Resize_Window (WindowStyle, ExWindowStyle, window_rect);

	// I think we do not need this?
	if (p->type == MODE_WINDOWED)
		SDL_SetWindowDisplayMode (NULL, 0) ; //sysplat.mainwindow,  (NULL, 0);

*/

	// Note that we are not constructing or resizing the existing window
	// It's ok because even on Windows I think we destroy it due to windows 8?
	//		SDL_Surface *hDC = SDL_GetWindowSurface (sysplat.mainwindow);
	//		SDL_GL_MakeCurrent (0,0); // Really? ewglMakeCurrent(NULL, NULL);
	//		SDL_FreeSurface (hDC); hDC = NULL; // ReleaseDC(sysplat.mainwindow, hDC);


		//VID_Local_Window_Renderer_Teardown (TEARDOWN_FULL_1, true /*reset video mode*/); // For now.

    sysplat.mainwindow = SDL_CreateWindow(ENGINE_NAME,
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			p->width, p->height,
			vid_sdl_flags);

#if 1
	// Apparently SDL goes out of its way to preserve the context!  Yay!
	if (!sysplat.draw_context) {
		sysplat.draw_context = SDL_GL_CreateContext(sysplat.mainwindow);
		if (!sysplat.draw_context)
			System_Error ("Couldn't create context %d x %d @ %d bpp", p->width, p->height, p->bpp);
	}

	// Do we need to attach it?  Apparently it automatically attaches to current window
#else
	if (  !(sysplat.draw_context = SDL_GL_CreateContext(sysplat.mainwindow))  )
        System_Error ("Couldn't create context %d x %d @ %d bpp", p->width, p->height, p->bpp);
#endif

	Shell_Platform_Icon_Window_Set (sysplat.mainwindow);

//    SDL_WM_SetCaption("hi", "lo");

	// Is this including or excluding the border?
	// I suppose we'd have to enable resizable to find out.

	SDL_SetWindowMinimumSize (sysplat.mainwindow, 320, 200);

// Get focus if we can, get foreground, finish setup, pump messages.
// then sleep a little.

	SDL_ShowWindow (sysplat.mainwindow);
	// UpdateWindow is part of ShowWindow
	// SetWindowPos unnecessary I think
	SDL_RaiseWindow (sysplat.mainwindow);	// SetForegroundWindow equivalent, ZOrder it.


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

	if (wglHRC && (reuseok = SDL_GL_MakeCurrent (sysplat.mainwindow, wglHRC)) == 0)
	{
		// Tried to reuse context and it failed
		SDL_GL_DeleteContext (wglHRC); // ewglDeleteContext (wglHRC);
		wglHRC = NULL;
		log_debug /*Con_DPrintLinef*/ ("Context reuse failed.  Must reload textures.");
	}


	if (!wglHRC) {
		wglHRC = SDL_GL_CreateContext (sysplat.mainwindow);


		if (!wglHRC)
			System_Error ("Could not initialize GL (SDL_GL_CreateContext failed)." NEWLINE NEWLINE "Make sure you in are 65535 color mode, and try running -window.");
		if (SDL_GL_MakeCurrent( sysplat.mainwindow, wglHRC ) != 0)
			System_Error ("VID_Init: SDL_GL_MakeCurrent failed");
	}

#if 1
	if (!restart)
	{
		eglClear (GL_COLOR_BUFFER_BIT);
		VID_SwapBuffers ();
	}
#endif



// Are we able to re-use the context?
	vid.ActiveApp = 1; // Crutch because we never activate.
	//return false;  //reuseok;
	return reuseok;
#endif
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
	if (bSuspend == false)
	{
//		eChangeDisplaySettings (&sysplat.gdevmode, CDS_FULLSCREEN);
//		ShowWindow(sysplat.mainwindow, SW_SHOWNORMAL);
//		MoveWindow(sysplat.mainwindow, 0, 0, sysplat.gdevmode.dmPelsWidth, sysplat.gdevmode.dmPelsHeight, false); //johnfitz -- alt-tab fix via Baker
#ifdef GLQUAKE_HARDWARE_GAMMA
		VID_Gamma_Clock_Set (); // Baker: Don't trust windows to do the right thing.
#endif // GLQUAKE_HARDWARE_GAMMA
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

//
// window teardown
//

void VID_Local_Window_Renderer_Teardown (int destroy, cbool reset_video_mode)
{
#if 0
	// destroy = 1 = TEARDOWN_FULL else TEARDOWN_NO_DELETE_GL_CONTEXT (don't destroy the context or destroy window)
	SDL_GLContext hRC = SDL_GL_GetCurrentContext(); // HGLRC hRC = ewglGetCurrentContext();
	SDL_Surface *hDC = SDL_GetWindowSurface (sysplat.mainwindow);

	SDL_GL_MakeCurrent (NULL, NULL); // Really? ewglMakeCurrent(NULL, NULL);

	if (hRC && destroy)		{ SDL_GL_DeleteContext(sysplat.draw_context); sysplat.draw_context = NULL; }
	if (hDC)				{ SDL_FreeSurface (hDC); hDC = NULL; } // ReleaseDC(sysplat.mainwindow, hDC);

	// Draw context for SDL is hGLRC not an hDC
	//*draw_context = NULL;	// SDL_FreeSurface (*draw_context);  But I think destroywindow does this.
#endif
	if (destroy) {


		SDL_DestroyWindow (sysplat.mainwindow);
		sysplat.mainwindow = NULL;
	}

	// Change display settings?
	SDL_SetWindowDisplayMode (NULL, 0) ;
}

#if 0
// Baker: Multisample support ...

#include "vid_wglext.h"		//WGL extensions

int	arbMultisampleSupported	= false;
int	arbMultisampleFormat	= 0;

cbool WGLisExtensionSupported(const char *extension)
{
#ifndef DIRECT3D_WRAPPER
	const size_t extlen = strlen(extension);
	const char *supported = NULL;
	const char *p;

	// Try To Use wglGetExtensionStringARB On Current DC, If Possible
	PROC wglGetExtString = ewglGetProcAddress("wglGetExtensionsStringARB");

	if (wglGetExtString)
		supported = ((char*(__stdcall*)(HDC))wglGetExtString)(wglGetCurrentDC());

	// If That Failed, Try Standard Opengl Extensions String
	if (supported == NULL)
		supported = (char*)eglGetString(GL_EXTENSIONS);

	// If That Failed Too, Must Be No Extensions Supported
	if (supported == NULL)
		return false;

	// Begin Examination At Start Of String, Increment By 1 On False Match
	for (p = supported; ; p++)
	{
		// Advance p Up To The Next Possible Match
		p = strstr(p, extension);

		if (p == NULL)
			return false;															// No Match

		// Make Sure That Match Is At The Start Of The String Or That
		// The Previous Char Is A Space, Or Else We Could Accidentally
		// Match "wglFunkywglExtension" With "wglExtension"

		// Also, Make Sure That The Following Character Is Space Or NULL
		// Or Else "wglExtensionTwo" Might Match "wglExtension"
		if ((p==supported || p[-1]==' ') && (p[extlen]=='\0' || p[extlen]==' '))
			return true;															// Match
	}
#else
	return false;

#endif // DIRECT3D_WRAPPER
}

// InitMultisample: Used To Query The Multisample Frequencies
int WIN_InitMultisample (HINSTANCE hInstance, HWND hWnd,PIXELFORMATDESCRIPTOR pfd, int ask_samples, int* pixelForceFormat)
{
	 // See If The String Exists In WGL!
	if (!WGLisExtensionSupported("WGL_ARB_multisample"))
	{
		return (arbMultisampleSupported = 0);
	}

	{
		// Get Our Pixel Format
		PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)ewglGetProcAddress("wglChoosePixelFormatARB");
		if (!wglChoosePixelFormatARB)
		{
			arbMultisampleSupported=false;
			return false;
		}


		{
			// Get Our Current Device Context
			HDC hDC = GetDC(hWnd);

			int		pixelFormat;
			int		valid;
			UINT	numFormats;
			float	fAttributes[] = {0,0};

			// These Attributes Are The Bits We Want To Test For In Our Sample
			// Everything Is Pretty Standard, The Only One We Want To
			// Really Focus On Is The SAMPLE BUFFERS ARB And WGL SAMPLES
			// These Two Are Going To Do The Main Testing For Whether Or Not
			// We Support Multisampling On This Hardware.
			int iAttributes[] =
			{
				WGL_DRAW_TO_WINDOW_ARB,GL_TRUE,
				WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
				WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
				WGL_COLOR_BITS_ARB, 24 /*currentbpp? Nah */, // Baker: Mirror current bpp color depth?
				WGL_ALPHA_BITS_ARB,8,
				WGL_DEPTH_BITS_ARB,24, // Baker: Changed ... didn't help.  Wrong place.
				WGL_STENCIL_BITS_ARB,8, // Baker: Stencil bits
				WGL_DOUBLE_BUFFER_ARB,GL_TRUE,
				WGL_SAMPLE_BUFFERS_ARB,GL_TRUE,
				WGL_SAMPLES_ARB, ask_samples /*multisample bits*/,
				0,0
			};


			while (ask_samples == 8 || ask_samples == 4 || ask_samples == 2)
			{
				iAttributes[19] = ask_samples;

				// First We Check To See If We Can Get A Pixel Format For 4 Samples
				valid = wglChoosePixelFormatARB(hDC,iAttributes,fAttributes,1,&pixelFormat,&numFormats);

				// If We Returned True, And Our Format Count Is Greater Than 1
				if (valid && numFormats >= 1)
				{
					*pixelForceFormat = arbMultisampleFormat = pixelFormat;
					return (arbMultisampleSupported = ask_samples);
				}

				ask_samples >>= 1; // Divide by 2
			}

			// Return Fail
			return  (arbMultisampleSupported = 0);
		}
	}
}

#endif



cbool VID_Local_IsGammaAvailable (unsigned short* ramps)
{
	// Returns 0 on success or a negative error code

	if (0 == SDL_GetWindowGammaRamp (sysplat.mainwindow, &ramps[0], &ramps[256], &ramps[512]) )
		return true;

	return false;
}


void VID_Local_Gamma_Set (unsigned short *ramps)
{
	if (!vid.ever_set_gamma)
		vid.ever_set_gamma = true;

	SDL_SetWindowGammaRamp (sysplat.mainwindow, &ramps[0], &ramps[256], &ramps[512]);
}

cbool VID_Local_Gamma_Reset (void)
{
    // Does SDL need?  I don't think so.
	unsigned short gammaramps[3][256]; // Doesn't pertain to 8 bit palette
    int i;
	int result;

	if (!vid.ever_set_gamma)
		vid.ever_set_gamma = true;

	for (i = 0;i < 256; i++)
		gammaramps[0][i] = gammaramps[1][i] = gammaramps[2][i] = (i * 65535) / 255;

	// Returns 0 on success or a negative error code on failure
	result = SDL_SetWindowGammaRamp (sysplat.mainwindow, gammaramps[0], gammaramps[1], gammaramps[2]);
	return (result == 0);
}

// Baker: Starting Quake Dialog
void VID_Local_Startup_Dialog (void)
{
    // No multisample support for SDL at the moment.
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

#ifdef GLQUAKE_HARDWARE_GAMMA
	VID_Gamma_Shutdown ();
#endif // GLQUAKE_HARDWARE_GAMMA

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
// Early
#ifdef GLQUAKE_RENDERER_SUPPORT
	VID_Renderer_Setup (); // Hooks up our GL functions
#endif // GLQUAKE_RENDERER_SUPPORT

}

#endif // CORE_SDL
#endif // GLQUAKE specific