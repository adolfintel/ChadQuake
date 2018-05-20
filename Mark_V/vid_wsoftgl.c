#if !defined(CORE_SDL) && !defined(GLQUAKE) && defined(CORE_GL)
#include "environment.h"
#ifdef PLATFORM_WINDOWS // Header level define, must be here


/*
Copyright (C) 2001-2012 Axel 'awe' Wefers (Fruitz Of Dojo)
Copyright (C) 2010-2011 MH
Copyright (C) 2009-2014 Baker

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
// vid_wsoftgl.c


#include "quakedef.h"
#include "winquake.h"
#include "resource.h" // IDI_ICON2

//#pragma comment (lib, "gdi32.lib") // CreateCompatibleDC, BitBlt, etc.

static void VID_WinQuake_AdjustBuffers (vmode_t *mode);
static void VID_CreateDIB (int width, int height, byte *palette);

//
// miscelleanous init
//

// Runs once.
void VID_Local_Window_PreSetup (void)
{
	WIN_Vid_Init_Once_CreateClass (); // Class, icon, etc.

}


vmode_t VID_Local_GetDesktopProperties (void)
{

	return WIN_Vid_GetDesktopProperties ();  // Vid_Display_Properties_Get	
}


//
// vsync
//

// Sole caller is GL_CheckExtensions.  No fucking kidding.  It's true.
cbool VID_Local_Vsync_Init (const char *gl_extensions_str)
{
	if (vid.direct3d == 8) // dx8 - vsync handled specially, automatically available, but not used through functions and requires vid_restart
		return true; 

	if (strstr(gl_extensions_str, "GL_EXT_swap_control") || strstr(gl_extensions_str, "GL_WIN_swap_hint"))
	{
		sysplat.wglSwapIntervalEXT = (SETSWAPFUNC) ewglGetProcAddress("wglSwapIntervalEXT");
		sysplat.wglGetSwapIntervalEXT = (GETSWAPFUNC) ewglGetProcAddress("wglGetSwapIntervalEXT");

		if (sysplat.wglSwapIntervalEXT && sysplat.wglGetSwapIntervalEXT && sysplat.wglSwapIntervalEXT(0) &&
			sysplat.wglGetSwapIntervalEXT() != -1)
				return true;
	}
	return false;
}

void VID_Local_Vsync (void)
{
	if (vid.direct3d == 8) // dx8 - vsync only through mode switch
		return; // Can only be performed on mode switch

	if (renderer.gl_swap_control)
	{
		if (vid_vsync.value)
		{
			if (!sysplat.wglSwapIntervalEXT(1))
				Con_PrintLinef ("VID_Vsync_f: failed on wglSwapIntervalEXT");
		}
		else
		{
			if (!sysplat.wglSwapIntervalEXT(0))
				Con_PrintLinef ("VID_Vsync_f: failed on wglSwapIntervalEXT");
		}
	}
}

void VID_Local_Vsync_f (cvar_t *var)
{
	if (vid.direct3d == 8) { // dx8 - vsync switch handled specially
		if (host_post_initialized) {
			Con_PrintLinef ("Direct3D: vid_vsync takes effect after mode change");
			Con_PrintLinef ("          vsync only works for fullscreen");
		}
	}

	VID_Local_Vsync ();
}


void VID_Local_Multisample_f (cvar_t *var) 
{
	// Unsupported - Wouldn't do anything in WinQuake GL anyway
}


//
// vid modes
//

// Sys_win?  NO.
void VID_Local_AddFullscreenModes (void)
{

	BOOL		stat;						// Used to test mode validity
	DEVMODE		devmode = {0};
	int			hmodenum = 0;				// Hardware modes start at 0

	// Baker: Run through every display mode and get information

	while ( (stat = eEnumDisplaySettings (NULL, hmodenum++, &devmode)) && vid.nummodes < MAX_MODE_LIST )
	{
		vmode_t test		= { MODE_FULLSCREEN, devmode.dmPelsWidth, devmode.dmPelsHeight, devmode.dmBitsPerPel };
		cbool bpp_ok		= (int)devmode.dmBitsPerPel == vid.desktop.bpp;
		cbool width_ok	= in_range (MIN_MODE_WIDTH_640, devmode.dmPelsWidth, MAX_MODE_WIDTH_10000);
		cbool height_ok	= in_range (MIN_MODE_HEIGHT_400, devmode.dmPelsHeight, MAX_MODE_HEIGHT_10000);
		cbool qualified	= (bpp_ok && width_ok && height_ok);

		devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

		if (qualified && !VID_Mode_Exists(&test, NULL) && eChangeDisplaySettings (&devmode, CDS_TEST | CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL)
		{
			// Not a dup and test = ok ---> add it
			memcpy (&vid.modelist[vid.nummodes++], &test, sizeof(vmode_t) );
			logd ("Added %d: %d x %d %d", vid.nummodes -1, vid.modelist[vid.nummodes-1].width, vid.modelist[vid.nummodes-1].height, vid.modelist[vid.nummodes-1].bpp);
		}
	}
}


// Baker: begin resize window on the fly
void VID_BeginRendering_Resize_Think_Resize_Act (void)
{
	WINDOWINFO windowinfo;
	windowinfo.cbSize = sizeof (WINDOWINFO);
	GetWindowInfo (sysplat.mainwindow, &windowinfo); // Client screen

	// Need to catch minimized scenario
	// Fill in top left, bottom, right, center
	vid.client_window.left = windowinfo.rcClient.left;
	vid.client_window.right = windowinfo.rcClient.right;
	vid.client_window.bottom = windowinfo.rcClient.bottom;
	vid.client_window.top = windowinfo.rcClient.top;
	vid.client_window.width = vid.client_window.right - vid.client_window.left;
	vid.client_window.height = vid.client_window.bottom - vid.client_window.top;

#ifndef DIRECT3D8_WRAPPER // dx8 - Not for Direct3D 8! (-resizable)  Keep in mind we are in a windows source file! vid_wgl.c
	if (1 /*COM_CheckParm ("-resizable")*/)
	{
		vid.screen.width = vid.client_window.width;
		vid.screen.height = vid.client_window.height;
		VID_WinQuake_AdjustBuffers (&vid.screen);
		eglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);
	}
#endif // DIRECT3D8_WRAPPER - not resizable
}
// End resize window on the fly

// Move to sys_win?  Probably not.
void WIN_Construct_Or_Resize_Window (DWORD style, DWORD exstyle, RECT window_rect)
{
	const char *nm = ENGINE_NAME;

	int x = window_rect.left, y = window_rect.top;
	int w = RECT_WIDTH(window_rect), h = RECT_HEIGHT(window_rect);

// Baker: begin resize window on the fly
	VID_Resize_Check (2);
// End resize window on the fly

	if (sysplat.mainwindow)
	{
		SetWindowLong (sysplat.mainwindow, GWL_EXSTYLE, exstyle);
		SetWindowLong (sysplat.mainwindow, GWL_STYLE, style);
		SetWindowPos  (sysplat.mainwindow, NULL, x, y, w, h, SWP_DRAWFRAME);
		return;
	}

	sysplat.mainwindow = CreateWindowEx (exstyle, nm, nm, style, x, y, w, h, NULL, NULL, sysplat.hInstance, NULL);

	if (!sysplat.mainwindow) System_Error ("Couldn't create DIB window");
}


// Move to sys_win?
void WIN_Change_DisplaySettings (int modenum)
{
	// Change display settings
	vmode_t *p = &vid.modelist[modenum];
	sysplat.gdevmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	sysplat.gdevmode.dmPelsWidth = p->width;
	sysplat.gdevmode.dmPelsHeight = p->height;
	sysplat.gdevmode.dmBitsPerPel = p->bpp;
	sysplat.gdevmode.dmSize = sizeof (DEVMODE);

	if (eChangeDisplaySettings (&sysplat.gdevmode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		System_Error ("Couldn't set fullscreen mode %d x %d @ %d bpp", p->width, p->height, p->bpp);
}


// Returns false if need to do GL setup again.
cbool VID_Local_SetMode (int modenum)
{
//	static int first_pass = true;
	vmode_t *p 			= &vid.modelist[modenum];	// vid.c sets vid.screen, so do not use that here.
	cbool reuseok 		= false;
	RECT client_rect	= {0, 0, p->width, p->height};
	RECT window_rect	= client_rect;
	cbool bordered		= p->type   == MODE_WINDOWED &&
						  (p->width  != vid.desktop.width ||
						  p->height != vid.desktop.height);

	DWORD ExWindowStyle = 0;
	DWORD WindowStyle	= bordered ? DW_BORDERED : DW_BORDERLESS;
	cbool restart	= (sysplat.mainwindow != NULL);

	// Preserve these for hopeful reuse.
	HDC wglHDC 		= restart ? ewglGetCurrentDC() : 0; // Not used?
	HGLRC wglHRC 	= restart ? ewglGetCurrentContext() : 0;

// Baker: begin resize window on the fly
#ifndef DIRECT3D8_WRAPPER // dx8 - Not for DirectX 8 (-resizable) - Keep in mind we are in a windows source file.
	if (bordered &&  1 /* COM_CheckParm ("-resizable")*/)
		WindowStyle = WindowStyle | WS_SIZEBOX;
#endif // DIRECT3D8_WRAPPER // DX8 no resize

// End resize window on the fly


#ifdef DIRECT3D9_WRAPPER // dx9 - an alternate resize that may not be friendly to Windows 8 or Windows 10 but I don't know for sure.  At one point in time, Windows 8 was very stupid about changing window attributes without destroying the window, did SP1 change that?  Is Windows 10 affected?
	if (restart) {
		// &window_rect ?  We still need this set right?  Yes.  Mouse cursor.  I think.  No.  It's declared here.
		vid.canalttab = false; // Necessary?  Are we handling any messages between now and then?  Does not look like it.
		if (p->type == MODE_WINDOWED)
			eChangeDisplaySettings (NULL, 0);

#pragma message ("TODO: Give it the style and the EX style.  We may or may have different ideas in mind for borderstyle via cvar or other settings.")
		Direct3D9_ResetMode (p->width, p->height, vid.desktop.bpp, (p->type == MODE_WINDOWED), WindowStyle, ExWindowStyle);
		vid.canalttab = true; // Necessary?  Are we handling any messages between now and then?
		return true; // Reuseok!
	}


#endif // DIRECT3D9_WRAPPER

#if 0
	if (restart)
		VID_Local_Window_Renderer_Teardown (TEARDOWN_NO_DELETE_GL_CONTEXT_0, true /*reset video mode*/);
#endif

	if (p->type == MODE_FULLSCREEN)
		WIN_Change_DisplaySettings (modenum);

// Baker: begin resize window on the fly
	AdjustWindowRectEx (&window_rect, WindowStyle, FALSE, ExWindowStyle);  // Adds the borders
	// Window width - Client width - WM_GETMINMAXINFO needs the border size.
	vid.border_width = (window_rect.right - window_rect.left) - client_rect.right;
	vid.border_height = (window_rect.bottom - window_rect.top) - client_rect.bottom;
// End resize window on the fly
	WIN_AdjustRectToCenterScreen(&window_rect);

	// Oh dear.  The Windows 8 workaround isn't here?
	// What does that mean?

	WIN_Construct_Or_Resize_Window (WindowStyle, ExWindowStyle, window_rect);

	if (p->type == MODE_WINDOWED)
		eChangeDisplaySettings (NULL, 0);

	// clear to black so it isn't empty
	sysplat.draw_context = GetDC(sysplat.mainwindow);
	#pragma message ("Baker: Oddly PaintBlackness does not seem to be doing anything now that I have multisample")
	PatBlt (sysplat.draw_context, 0, 0, p->width,p->height, BLACKNESS);

	// Get focus if we can, get foreground, finish setup, pump messages.
	// then sleep a little.

	ShowWindow (sysplat.mainwindow, SW_SHOWDEFAULT);
	UpdateWindow (sysplat.mainwindow);
	SetWindowPos (sysplat.mainwindow, HWND_TOP, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOCOPYBITS);
	SetForegroundWindow (sysplat.mainwindow);

	System_Process_Messages_Sleep_100 ();

//	if (first_pass == true) {
		WIN_SetupPixelFormat (sysplat.draw_context); // WinQuake GL difference
//		first_pass = false;
//	}

#ifdef DIRECT3D8_WRAPPER // dx8 - vid_vsync work around that does not apply to dx9
	Direct3D8_SetVsync (vid_vsync.value); // Baker
	Direct3D8_SetFullscreen (p->type == MODE_FULLSCREEN); // Baker
	Direct3D8_SetBPP (vid.desktop.bpp);
#endif // DIRECT3D8_WRAPPER - dx8 extra restart information

	if (wglHRC && (reuseok = ewglMakeCurrent (sysplat.draw_context, wglHRC)) == 0)
	{
		// Tried to reuse context and it failed
		ewglDeleteContext (wglHRC);
		wglHRC = NULL;
		Con_DPrintLinef ("Context reuse failed.  Must reload textures.");
	}


	if (!wglHRC)
	{
		// Must create a context.
		wglHRC = ewglCreateContext( sysplat.draw_context );

#ifdef DIRECT3D8_WRAPPER // dx8 - vid_vsync work around that does not apply to dx9
		Direct3D8_SetVsync (vid_vsync.value); // Baker
		Direct3D8_SetFullscreen (p->type == MODE_FULLSCREEN); // Baker
		Direct3D8_SetBPP (vid.desktop.bpp);
#endif // DIRECT3DX_WRAPPER

		if (!wglHRC)
			System_Error ("Could not initialize GL (wglCreateContext failed)." NEWLINE NEWLINE "Make sure you in are 65535 color mode, and try running -window.");
		if (!ewglMakeCurrent( sysplat.draw_context, wglHRC ))
			System_Error ("VID_Init: wglMakeCurrent failed");
	}

#if 1
	if (!ewglMakeCurrent( sysplat.draw_context, wglHRC ))
		System_Error ("VID_Init: wglMakeCurrent failed");
#endif

#if 1
	if (!restart)
	{
		eglClear (GL_COLOR_BUFFER_BIT);
		VID_SwapBuffers ();
	}
#endif

	VID_WinQuake_AdjustBuffers (p);

	return reuseok;
}

//
// in game
//

void VID_Local_SwapBuffers (void)
{	
	if (eSwapBuffers (sysplat.draw_context) == 0) {
		if (vid.ActiveApp) // I'm getting this inappropriately after changing input for some reason.  Quit does a disconnect, which causes a screen update?
			msgbox ("Quake", "Swapbuffers failed"); // I've not seen this happen in at least a month.  But since it doesn't happen, little harm in leaving it here, right?
	}
}


void VID_Local_Suspend (cbool bSuspend)
{
	if (bSuspend == false)
	{
		eChangeDisplaySettings (&sysplat.gdevmode, CDS_FULLSCREEN);
		ShowWindow(sysplat.mainwindow, SW_SHOWNORMAL);
		MoveWindow(sysplat.mainwindow, 0, 0, sysplat.gdevmode.dmPelsWidth, sysplat.gdevmode.dmPelsHeight, false); //johnfitz -- alt-tab fix via Baker
	} else  eChangeDisplaySettings (NULL, 0);
}

//
// window setup
//

BOOL WIN_SetupPixelFormat (HDC hDC)
{
    static PIXELFORMATDESCRIPTOR pfd = {
	sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
	1,						// version number
	PFD_DRAW_TO_WINDOW |	// support window
	PFD_SUPPORT_OPENGL |	// support OpenGL
	PFD_DOUBLEBUFFER,		// double buffered
	PFD_TYPE_RGBA,			// RGBA type
	24,						// 24-bit color depth
	0, 0, 0, 0, 0, 0,		// color bits ignored
	0,						// no alpha buffer
	0,						// shift bit ignored
	0,						// no accumulation buffer
	0, 0, 0, 0, 			// accum bits ignored
	BPP_24,					// 32-bit z-buffer
	8,						// 8-bit stencil buffer
	0,						// no auxiliary buffer
	PFD_MAIN_PLANE,			// main layer
	0,						// reserved
	0, 0, 0					// layer masks ignored
    };
    int pixelformat;
	PIXELFORMATDESCRIPTOR test; //johnfitz

	if (1)
	{
		if ( (pixelformat = eChoosePixelFormat(hDC, &pfd)) == 0 )
		{
			System_Error ("Video: ChoosePixelFormat failed");
			return FALSE;
		}
	} //else pixelformat = sysplat.forcePixelFormat; // Multisample overrride

	eDescribePixelFormat(hDC, pixelformat, sizeof(PIXELFORMATDESCRIPTOR), &test);

    if (eSetPixelFormat(hDC, pixelformat, &pfd) == FALSE)
    {
        System_Error ("SetPixelFormat failed");
        return FALSE;
    }

	memcpy (&sysplat.pfd, &pfd, sizeof(pfd) );

    return TRUE;
}

#define MSVC_IS_BEING_DUMB // and giving me heap corruption but GCC isn't.  And I think if I tack on "CORE_GL" to WinQuake debug it is ok too ... dumb
							// Will we eventually discover it's something else?  Project setting or something?  Who knows!

#ifdef MSVC_IS_BEING_DUMB
static pixel_t /*byte*/ my_static_buffero[1024][512]; // Legal?
#endif // MSVC_IS_BEING_DUMB

void VID_CreateDIB (int newwidth, int newheight, byte *palette)
{
	// vid.conwidth, vid.conheight = newwidth and newheight
	GLenum glerrorcode = 0;

	if (!sysplat.draw_context)
		System_Error ("No GL context for WinQuake GL");

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

	if (!vid.wingl.texslot)
		System_Error ("No texture slot for WinQuake GL");

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
}


//
// window teardown
//

void VID_Local_Window_Renderer_Teardown (int destroy, cbool reset_video_mode)
{
	// destroy = 1 = TEARDOWN_FULL else TEARDOWN_NO_DELETE_GL_CONTEXT (don't destroy the context or destroy window)
	HGLRC hRC = ewglGetCurrentContext();
    HDC	  hDC = ewglGetCurrentDC();

    ewglMakeCurrent(NULL, NULL);

    if (hRC && destroy)		ewglDeleteContext(hRC);
	if (hDC)				ReleaseDC(sysplat.mainwindow, hDC);

	if (sysplat.draw_context)
	{
		// Can the draw context ever be different?
		//ReleaseDC (sysplat.mainwindow, sysplat.draw_context);
		sysplat.draw_context = NULL;
	}

	if (destroy)
	{
		if (sysplat.mainwindow) {
			DestroyWindow (sysplat.mainwindow);
			sysplat.mainwindow = NULL;
		}

		// 
	}



	if (reset_video_mode)
		eChangeDisplaySettings (NULL, 0);
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

	#pragma message ("Let's slam this into vid.c and call vidco set window caption or something.  Please!")
	SetWindowText (sysplat.mainwindow, new_caption);

}


void VID_Local_Shutdown (void)
{
	VID_Local_Window_Renderer_Teardown (TEARDOWN_FULL_1, true /*reset video mode*/);
}


// This function gets called before anything happens
void VID_Local_Init (void)
{
	VID_Renderer_Setup (); // Hooks up our GL functions
// Early
#ifdef WINQUAKE_RENDERER_SUPPORT
	VID_Palette_NewGame ();
#endif // WINQUAKE_RENDERER_SUPPORT


}

#endif // PLATFORM_WINDOWS
#endif // !CORE_SDL && !GLQUAKE && CORE_GL /*win thru gl*/
