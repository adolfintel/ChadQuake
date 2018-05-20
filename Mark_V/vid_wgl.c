#ifdef GLQUAKE // GLQUAKE specific
#ifndef CORE_SDL // NOT SDL

#include "environment.h"
#ifdef PLATFORM_WINDOWS // Has to be here, set by a header


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
#include "winquake.h"
#include "resource.h" // IDI_ICON2


//
// miscelleanous init
//

void VID_Local_Window_PreSetup (void)
{
	WIN_Vid_Init_Once_CreateClass (); // Class, icon, etc.

	 // Baker: Multisample support.  In principal, at least.
	if (sysplat.hwnd_dialog)
	{
		if (vid_multisample.value)
		{
			// Poke into it for the PFD
			HDC	hdc			= GetDC(sysplat.hwnd_dialog);
			int unused		= WIN_SetupPixelFormat (hdc);
			HGLRC wglHRC	= ewglCreateContext( hdc );
			HDC wglHDC 		= ewglGetCurrentDC();
			int unused2		= ewglMakeCurrent( hdc, wglHRC);
			int ask_samples	= (int)vid_multisample.value;

			if (ask_samples != 2 && ask_samples != 4 && ask_samples != 8)
			{
				Con_WarningLinef ("Multisamples requested " QUOTED_D " is invalid, trying 4", ask_samples);
				ask_samples = 4;
			}

			// Do it.  We already have desktop properties
			sysplat.multisamples = WIN_InitMultisample (sysplat.hInstance, sysplat.hwnd_dialog, sysplat.pfd, ask_samples, &sysplat.forcePixelFormat);

			// Your mission is complete.  You may leave now ...
			ewglMakeCurrent(NULL, NULL);
			ewglDeleteContext(wglHRC);
			ReleaseDC(sysplat.hwnd_dialog, wglHDC);
			ReleaseDC(sysplat.hwnd_dialog, hdc);

			if (sysplat.multisamples)
			{
				Con_SafePrintLinef ("Multisample x %d Enabled (Requested %d, Received %d).", sysplat.multisamples, ask_samples, sysplat.multisamples);
				vid.multisamples = sysplat.multisamples;
			}
			else Con_WarningLinef ("Multisample: Requested but not available.");
		} else Con_DPrintLinef ("Note: Multisample not requested");

		// Post teardown
		DestroyWindow (sysplat.hwnd_dialog);
		sysplat.hwnd_dialog = NULL;
	}
	// Baker: End multisample support
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
#pragma message ("Baker: What if it isn't supported?  Like if we don't bother on a Mac?")
	if (host_initialized) {
		Con_PrintLinef ("%s set to " QUOTED_S ".  requires engine restart.", var->name, var->string);
		Con_PrintLinef ("Note settings are: 2, 4, 8 and 0");
	}
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
#ifdef SUPPORTS_REFRESHRATE
		vmode_t test		= { MODE_FULLSCREEN, devmode.dmPelsWidth, devmode.dmPelsHeight, devmode.dmBitsPerPel, devmode.dmDisplayFrequency };
#else
		vmode_t test		= { MODE_FULLSCREEN, devmode.dmPelsWidth, devmode.dmPelsHeight, devmode.dmBitsPerPel };
#endif // ! SUPPORTS_REFRESHRATE
		cbool bpp_ok		= (int)devmode.dmBitsPerPel == vid.desktop.bpp;
		cbool width_ok	= in_range (MIN_MODE_WIDTH_640, devmode.dmPelsWidth, MAX_MODE_WIDTH_10000);
		cbool height_ok	= in_range (MIN_MODE_HEIGHT_400, devmode.dmPelsHeight, MAX_MODE_HEIGHT_10000);
		cbool qualified	= (bpp_ok && width_ok && height_ok);

#ifdef SUPPORTS_REFRESHRATE
		devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;
#else
		devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
#endif // ! SUPPORTS_REFRESHRATE

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

	// Baker: Not if 0
	if (windowinfo.rcClient.right - windowinfo.rcClient.left <= 0) {
		// Make sure the window is set as minimized here if it isn't?
		return;
	}

	// Need to catch minimized scenario
	// Fill in top left, bottom, right, center
	vid.client_window.left = windowinfo.rcClient.left;
	vid.client_window.right = windowinfo.rcClient.right;
	vid.client_window.bottom = windowinfo.rcClient.bottom;
	vid.client_window.top = windowinfo.rcClient.top;
	vid.client_window.width = vid.client_window.right - vid.client_window.left;
	vid.client_window.height = vid.client_window.bottom - vid.client_window.top;

#ifndef DIRECT3D8_WRAPPER // dx8 - Not for Direct3D 8! (-resizable)  Keep in mind we are in a windows source file! vid_wgl.c
	// Added MH extra conditions.
	if (1 /*COM_CheckParm ("-resizable")*/ && (vid.screen.width != vid.client_window.width || vid.screen.height != vid.client_window.height))
	{
		vid.screen.width = vid.client_window.width;
		vid.screen.height = vid.client_window.height;
		vid.consize_stale = true; // This triggers a cascade of recalculations in SCR_UpdateScreen
		vid.warp_stale = true; // Means warp needs recalculated.


		// Since the WINAPI is blocking the way we do Quake, a Quake frame cannot occur until after resize. 
#ifdef DIRECT3D9_WRAPPER 
		if (!vid.resized) {
			Con_DPrintLinef ("Window resized without event!"); // Baker: For mh/gunter
		}


		Direct3D9_ResetMode (vid.screen.width, vid.screen.height, true /*windowed*/, vid.client_window.left, vid.client_window.top, -1 /*width*/, -1 /*height*/, true, &vid.border_width, &vid.border_height);
#endif // DIRECT3D9_WRAPPER
		//vid.mouse_resized = true;  // We don't really have a way of knowing this easily.

		eglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);
		
	}
#endif // DIRECT3D8_WRAPPER
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
#endif // DIRECT3D8_WRAPPER // DX8 - no resize

// End resize window on the fly


#ifdef DIRECT3D9_WRAPPER // dx9 - an alternate resize that may not be friendly to Windows 8 or Windows 10 but I don't know for sure.  At one point in time, Windows 8 was very stupid about changing window attributes without destroying the window, did SP1 change that?  Is Windows 10 affected?
	if (restart) {
		// &window_rect ?  We still need this set right?  Yes.  Mouse cursor.  I think.  No.  It's declared here.
		vid.canalttab = false; // Necessary?  Are we handling any messages between now and then?  Does not look like it.
//		if (p->type == MODE_WINDOWED)
//			eChangeDisplaySettings (NULL, 0);

#pragma message ("TODO: Give it the style and the EX style.  We may or may have different ideas in mind for borderstyle via cvar or other settings.")
		Direct3D9_ResetMode (p->width, p->height, /* kild vid.desktop.bpp,*/ (p->type == MODE_WINDOWED), -1, -1, vid.desktop.width, vid.desktop.height, false, &vid.border_width, &vid.border_height); //, WindowStyle, ExWindowStyle);

		vid.canalttab = true; // Necessary?  Are we handling any messages between now and then?
		return true; // Reuseok!
	}


#endif // DIRECT3D9_WRAPPER

	if (restart)
		VID_Local_Window_Renderer_Teardown (TEARDOWN_NO_DELETE_GL_CONTEXT_0, true /*reset video mode*/);

	if (p->type == MODE_FULLSCREEN)
		WIN_Change_DisplaySettings (modenum);

// Baker: begin resize window on the fly
	AdjustWindowRectEx (&window_rect, WindowStyle, FALSE, ExWindowStyle);  // Adds the borders
	// Window width - Client width - WM_GETMINMAXINFO needs the border size.
	vid.border_width = (window_rect.right - window_rect.left) - client_rect.right;
	vid.border_height = (window_rect.bottom - window_rect.top) - client_rect.bottom;
// End resize window on the fly
	WIN_AdjustRectToCenterScreen(&window_rect);

#if 1
	// Windows 8 introduces chaos :(
	if (restart && p->type != vid.screen.type)
	{
		DestroyWindow (sysplat.mainwindow);
		sysplat.mainwindow = 0;
	}
#endif


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

	WIN_SetupPixelFormat (sysplat.draw_context);

#ifdef DIRECT3D8_WRAPPER // dx8 - vid_vsync work around that does not apply to dx9
	Direct3D8_SetVsync (vid_vsync.value); // Baker
	Direct3D8_SetFullscreen (p->type == MODE_FULLSCREEN); // Baker
	Direct3D8_SetBPP (vid.desktop.bpp);
#endif // DIRECT3D8_WRAPPER - extra information on restart

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
	if (!restart)
	{
		eglClear (GL_COLOR_BUFFER_BIT);
		VID_SwapBuffers ();
	}
#endif
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
#ifdef GLQUAKE_HARDWARE_GAMMA
		VID_Gamma_Clock_Set (); // Baker: Don't trust windows to do the right thing.
#endif // GLQUAKE_HARDWARE_GAMMA
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

	if (!sysplat.multisamples)
	{
		if ( (pixelformat = eChoosePixelFormat(hDC, &pfd)) == 0 )
		{
			System_Error ("Video: ChoosePixelFormat failed");
			return FALSE;
		}
	} else pixelformat = sysplat.forcePixelFormat; // Multisample overrride

	eDescribePixelFormat(hDC, pixelformat, sizeof(PIXELFORMATDESCRIPTOR), &test);

    if (eSetPixelFormat(hDC, pixelformat, &pfd) == FALSE)
    {
        System_Error ("SetPixelFormat failed");
        return FALSE;
    }

	memcpy (&sysplat.pfd, &pfd, sizeof(pfd) );

    return TRUE;
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
		ReleaseDC (sysplat.mainwindow, sysplat.draw_context);
		sysplat.draw_context = NULL;
	}

	if (destroy)
	{
		DestroyWindow (sysplat.mainwindow);
		sysplat.mainwindow = NULL;
	}

	eChangeDisplaySettings (NULL, 0);
}

// Baker: Multisample support ...

#include "vid_wglext.h"		//WGL extensions

int	arbMultisampleSupported	= false;
int	arbMultisampleFormat	= 0;

cbool WGLisExtensionSupported(const char *extension)
{
//#if !defined(DIRECT3D_WRAPPER) && !defined(DIRECT3D9_WRAPPER)
	const size_t extlen = strlen(extension);
	const char *supported = NULL;
	const char *p;

	// Try To Use wglGetExtensionStringARB On Current DC, If Possible
	PROC PVT_wglGetExtString = ewglGetProcAddress("wglGetExtensionsStringARB");

	if (PVT_wglGetExtString)
		supported = ((char *(__stdcall*)(HDC))PVT_wglGetExtString)(ewglGetCurrentDC());

	// If That Failed, Try Standard Opengl Extensions String
	if (supported == NULL)
		supported = (char *)PVT_wglGetExtString(GL_EXTENSIONS);

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
//#else
//	return false;

//#endif // !DIRECT3D_WRAPPER && !DIRECT3D9_WRAPPER
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

cbool VID_Local_IsGammaAvailable (unsigned short* ramps)
{
	if (!GetDeviceGammaRamp (sysplat.draw_context, ramps))
		return false;

	return true;
}


void VID_Local_Gamma_Set (unsigned short* ramps)
{
	if (!vid.ever_set_gamma)
		vid.ever_set_gamma = true;

	SetDeviceGammaRamp (sysplat.draw_context, ramps);
}

int VID_Local_Gamma_Reset (void)
{
	int i;
	HDC hdc = GetDC (NULL);
	WORD gammaramps[3][256];

	if (!vid.ever_set_gamma)
		vid.ever_set_gamma = true;

	for (i = 0;i < 256; i++)
		gammaramps[0][i] = gammaramps[1][i] = gammaramps[2][i] = (i * 65535) / 255;

	i = SetDeviceGammaRamp(hdc, &gammaramps[0][0]);
	ReleaseDC (NULL, hdc);

	return !!i;
}

// Baker: Starting Quake Dialog
void VID_Local_Startup_Dialog (void)
{
	if (DIRECT3D_WRAPPER_VERSION != 0) {
		// Multisample not supported in Direct 3D version.
		Con_DebugLogLine ("Direct 3D version does not support multisample");
		return;
	}

	// Baker: I think this is broke somehow.  Although doesn't matter that much.  Few situations that it would matter.

	if (COM_CheckParm ("-nomultisample")) {
		//Con_DPrintLinef ("Multisample disabled at command line"); // This is WAY TOO early.
		return;
	}

	sysplat.hwnd_dialog = CreateDialog(sysplat.hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, NULL);

	if (sysplat.hwnd_dialog)
	{
		RECT rect;
		GetWindowRect (sysplat.hwnd_dialog, &rect);
		WIN_AdjustRectToCenterScreen (&rect);
		SetWindowPos
		(
			sysplat.hwnd_dialog,
			NULL,
			rect.left,
			rect.top,
			0,
			0,
			SWP_NOZORDER | SWP_NOSIZE
		);
		ShowWindow (sysplat.hwnd_dialog, SW_SHOWDEFAULT);
		UpdateWindow (sysplat.hwnd_dialog);
		SetForegroundWindow (sysplat.hwnd_dialog);
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

#ifdef GLQUAKE_HARDWARE_GAMMA
	VID_Gamma_Shutdown ();
#endif // GLQUAKE_HARDWARE_GAMMA
}


// This function gets called before anything happens
void VID_Local_Init (void)
{
// Early
#ifdef GLQUAKE_RENDERER_SUPPORT
	VID_Renderer_Setup (); // Hooks up our GL functions
#endif // GLQUAKE_RENDERER_SUPPORT

}

#endif // PLATFORM_WINDOWS
#endif // !CORE_SDL
#endif // GLQUAKE specific
