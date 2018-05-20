#ifndef GLQUAKE // WinQuake Software renderer
#if !defined(CORE_SDL) && !defined(CORE_GL) /*win thru GL*/

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
// vid_wsoft.c


#include "quakedef.h"
#include "winquake.h"
#include "resource.h" // IDI_ICON2

#pragma comment (lib, "gdi32.lib") // CreateCompatibleDC, BitBlt, etc.

static void VID_WinQuake_AdjustBuffers (vmode_t *mode);
static void VID_CreateDIB (int width, int height, byte *palette);

//
// miscelleanous init
//

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

void VID_Local_Vsync_f (cvar_t *var)
{
	// Unsupported at the moment.
}



void VID_Local_Multisample_f (cvar_t *var)
{
	// Unsupported
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

	while ( (stat = EnumDisplaySettings (NULL, hmodenum++, &devmode)) && vid.nummodes < MAX_MODE_LIST )
	{
		vmode_t test		= { MODE_FULLSCREEN, devmode.dmPelsWidth, devmode.dmPelsHeight, devmode.dmBitsPerPel };
		cbool bpp_ok		= (int)devmode.dmBitsPerPel == vid.desktop.bpp;
		cbool width_ok	= in_range (MIN_MODE_WIDTH_640, devmode.dmPelsWidth, MAX_MODE_WIDTH_10000);
		cbool height_ok	= in_range (MIN_MODE_HEIGHT_400, devmode.dmPelsHeight, MAX_MODE_HEIGHT_10000);
		cbool qualified	= (bpp_ok && width_ok && height_ok);

		devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

		if (qualified && !VID_Mode_Exists(&test, NULL) && ChangeDisplaySettings (&devmode, CDS_TEST | CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL)
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

	if (1 /*COM_CheckParm ("-resizable")*/)
	{
		vid.screen.width = vid.client_window.width;
		vid.screen.height = vid.client_window.height;
		VID_WinQuake_AdjustBuffers (&vid.screen);
	}

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

	if (ChangeDisplaySettings (&sysplat.gdevmode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		System_Error ("Couldn't set fullscreen mode %d x %d @ %d bpp", p->width, p->height, p->bpp);
}

cbool VID_Local_SetMode (int modenum)
{
	vmode_t *p 			= &vid.modelist[modenum];	// vid.c sets vid.screen, so do not use that here.
	RECT client_rect	= {0, 0, p->width, p->height};
	RECT window_rect	= client_rect;
	cbool bordered		= p->type == MODE_WINDOWED;/* &&
						  (p->width  != vid.desktop.width ||
						  p->height != vid.desktop.height); */

	DWORD ExWindowStyle = 0;
	DWORD WindowStyle	= bordered ? DW_BORDERED : DW_BORDERLESS;
	cbool restart		= (sysplat.mainwindow != NULL);

// Baker: begin resize window on the fly
	if (bordered &&  1 /* COM_CheckParm ("-resizable")*/)
		WindowStyle = WindowStyle | WS_SIZEBOX;
// End resize window on the fly

	if (restart)
		VID_Local_Window_Renderer_Teardown (TEARDOWN_NO_DELETE_GL_CONTEXT_0, true /*reset video mode*/);

	if (p->type == MODE_FULLSCREEN)
		WIN_Change_DisplaySettings (modenum);

	AdjustWindowRectEx (&window_rect, WindowStyle, FALSE, ExWindowStyle);
	WIN_AdjustRectToCenterScreen(&window_rect);

	// Oh dear.  The Windows 8 workaround isn't here?
	// What does that mean?
#if 0 // Attempt Jan 8 - It didn't help.  At least not for the problem caused by unwanted ChangeDisplaySettings
	// Windows 8 introduces chaos :(
	if (restart && p->type != vid.screen.type)
	{
		DestroyWindow (sysplat.mainwindow);
		sysplat.mainwindow = 0;
	}
#endif


	WIN_Construct_Or_Resize_Window (WindowStyle, ExWindowStyle, window_rect);

	if (p->type == MODE_WINDOWED)
		ChangeDisplaySettings (NULL, 0);

	// Get focus if we can, get foreground, finish setup, pump messages.
	// then sleep a little.

	ShowWindow (sysplat.mainwindow, SW_SHOWDEFAULT);
	UpdateWindow (sysplat.mainwindow);
	SetWindowPos (sysplat.mainwindow, HWND_TOP, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOCOPYBITS);
	SetForegroundWindow (sysplat.mainwindow);

	System_Process_Messages_Sleep_100 ();

	VID_WinQuake_AdjustBuffers (p);

	return true;
}

//
// in game
//

typedef struct dibinfo_s
{
	BITMAPINFOHEADER	header;
	RGBQUAD				acolors[256];
} dibinfo_t;


typedef struct
{
	HGDIOBJ			previously_selected_GDI_obj;
	HBITMAP			hDIBSection;
	unsigned char	*pDIBBase;
	HDC				hdcDIBSection;
	HDC				maindc;
} tdibset_t;

tdibset_t tdibset;




void VID_Local_Suspend (cbool bSuspend)
{
	if (bSuspend == false)
	{
		ChangeDisplaySettings (&sysplat.gdevmode, CDS_FULLSCREEN);
		ShowWindow(sysplat.mainwindow, SW_SHOWNORMAL);
		MoveWindow(sysplat.mainwindow, 0, 0, sysplat.gdevmode.dmPelsWidth, sysplat.gdevmode.dmPelsHeight, false); //johnfitz -- alt-tab fix via Baker
	} else  ChangeDisplaySettings (NULL, 0);
}

//
// window setup
//


// Baker: Similar to GL Initial setup after window is constructed, needs to know palette
void VID_CreateDIB (int width, int height, byte *palette)
{
	dibinfo_t   dibheader;
	BITMAPINFO *pbmiDIB = (BITMAPINFO *) &dibheader;
	int i;

	tdibset.maindc = GetDC (sysplat.mainwindow);
	memset (&dibheader, 0, sizeof (dibheader));

#if 1
	width = roundup_8 (width); // if (width & 7) width = (width - (width & 7)) + 8; // Replace with roundup_8 ?
#endif

	// fill in the bitmap info
	pbmiDIB->bmiHeader.biSize          = sizeof (BITMAPINFOHEADER);
	pbmiDIB->bmiHeader.biWidth         = width;
	pbmiDIB->bmiHeader.biHeight        = height;
	pbmiDIB->bmiHeader.biPlanes        = 1;
	pbmiDIB->bmiHeader.biBitCount      = 8;
	pbmiDIB->bmiHeader.biCompression   = BI_RGB;
	pbmiDIB->bmiHeader.biSizeImage     = 0;
	pbmiDIB->bmiHeader.biXPelsPerMeter = 0;
	pbmiDIB->bmiHeader.biYPelsPerMeter = 0;
	pbmiDIB->bmiHeader.biClrUsed       = 256;
	pbmiDIB->bmiHeader.biClrImportant  = 256;

	// fill in the palette
	for (i = 0; i < PALETTE_COLORS_256; i++)
	{
		// d_8to24table isn't filled in yet so this is just for testing
		dibheader.acolors[i].rgbRed   = palette[i * 3];
		dibheader.acolors[i].rgbGreen = palette[i * 3 + 1];
		dibheader.acolors[i].rgbBlue  = palette[i * 3 + 2];
	}

	// create the DIB section
	tdibset.hDIBSection = CreateDIBSection (tdibset.maindc,
							pbmiDIB,
							DIB_RGB_COLORS,
							(void**)&tdibset.pDIBBase,
							NULL,
							0);

	// set video buffers
	if (pbmiDIB->bmiHeader.biHeight > 0)
	{
		// bottom up
		vid.buffer = tdibset.pDIBBase + (height - 1) * width;
		vid.rowbytes = -width;
	}
	else
	{
		// top down
		vid.buffer = tdibset.pDIBBase;
		vid.rowbytes = width;
	}

	// clear the buffer
	memset (tdibset.pDIBBase, 0xff, width * height);

	if ((tdibset.hdcDIBSection = CreateCompatibleDC (tdibset.maindc)) == NULL)
		System_Error ("DIB_Init() - CreateCompatibleDC failed");

	if ((tdibset.previously_selected_GDI_obj = SelectObject (tdibset.hdcDIBSection, tdibset.hDIBSection)) == NULL)
		System_Error ("DIB_Init() - SelectObject failed");
}


//
// window teardown
//

void VID_Local_Window_Renderer_Teardown (int destroy, cbool reset_video_mode)
{
// Baker: Shuts down the DIB.   Similar to GL_Teardown
	{
		if (tdibset.hdcDIBSection)
		{
			SelectObject (tdibset.hdcDIBSection, tdibset.previously_selected_GDI_obj);
			DeleteDC (tdibset.hdcDIBSection);
			tdibset.hdcDIBSection = NULL;
		}

		if (tdibset.hDIBSection)
		{
			DeleteObject (tdibset.hDIBSection);
			tdibset.hDIBSection = NULL;
			tdibset.pDIBBase = NULL;
		}

		if (tdibset.maindc)
		{
			// if maindc exists sysplat.mainwindow must also be valid
			ReleaseDC (sysplat.mainwindow, tdibset.maindc);
			tdibset.maindc = NULL;
		}
	}

	if (destroy)
	{
		DestroyWindow (sysplat.mainwindow);
		sysplat.mainwindow = NULL;
	}

	if (reset_video_mode)
		ChangeDisplaySettings (NULL, 0);
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
		int stretch_try = CLAMP(0, vid.stretch_old_cvar_val, 2);

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

	VID_Local_Window_Renderer_Teardown (TEARDOWN_NO_DELETE_GL_CONTEXT_0, false /*do not reset video mode*/); // restart the DIB

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

void VID_Update (vrect_t *rects)
{
	// We've drawn the frame; copy it to the screen

	if (tdibset.hdcDIBSection)
	{
		int numrects = 0;

#if 0 // Baker: Unused
		while (rects)
#endif
		{
			if (vid.stretch_x !=1 || vid.stretch_y != 1)
				StretchBlt(
				  tdibset.maindc,
				  rects->x * vid.stretch_x, rects->y * vid.stretch_y,
				  rects->width * vid.stretch_x, rects->height * vid.stretch_y,
				  tdibset.hdcDIBSection,
				  rects->x, rects->y,
				  rects->width, rects->height,
				  SRCCOPY);
			else
				BitBlt (tdibset.maindc,
						rects->x, rects->y,
						rects->width,
						rects->height,
						tdibset.hdcDIBSection,
						rects->x, rects->y,
						SRCCOPY);

			numrects++;
#if 0 // Baker: multi-rect drawing unused
			rects = rects->pnext;
#endif
		}
	}
	// END FLIPPAGE

}


//
//
//
// Palette Set
//
//
//


void VID_Local_Modify_Palette (byte *palette)
{
	int		i, bestmatch, bestmatchmetric, t, dr, dg, db;
	byte	*ptmp;
	// GDI doesn't let us remap palette index 0, so we'll remap color
	// mappings from that black to another one
	bestmatchmetric = 256 * 256 * 3;

	for (i = 1; i < PALETTE_COLORS_256; i++)
	{
		dr = palette[0] - palette[i * 3 + 0];
		dg = palette[1] - palette[i * 3 + 1];
		db = palette[2] - palette[i * 3 + 2];
		t = (dr * dr) + (dg * dg) + (db * db);

		if (t < bestmatchmetric)
		{
			bestmatchmetric = t;
			bestmatch = i;

			if (t == 0)
				break;
		}
	}

	for (i = 0, ptmp = vid.colormap; i < (1 << (VID_CBITS + 8)); i++, ptmp++)
	{
		if (*ptmp == 0)
			*ptmp = bestmatch;
	}
	vid.altblack = bestmatch;

#ifdef WINQUAKE_QBISM_ALPHAMAP
	R_WinQuake_Generate_Alpha50_Map (vid.alpha50map);
#endif // WINQUAKE_QBISM_ALPHAMAP
}

void VID_Local_SetPalette (byte *palette)
{
	int			i;
	RGBQUAD		colors[PALETTE_COLORS_256];
	byte *pal = palette;

	if (tdibset.hdcDIBSection)
	{
		// incoming palette is 3 component
		for (i = 0; i < PALETTE_COLORS_256; i++, pal += RGB_3)
		{
			colors[i].rgbRed   = pal[0];
			colors[i].rgbGreen = pal[1];
			colors[i].rgbBlue  = pal[2];
			colors[i].rgbReserved = 0;
		}

		colors[0].rgbRed = 0;
		colors[0].rgbGreen = 0;
		colors[0].rgbBlue = 0;
		colors[255].rgbRed = 0xff;
		colors[255].rgbGreen = 0xff;
		colors[255].rgbBlue = 0xff;

		if (SetDIBColorTable (tdibset.hdcDIBSection, 0, 256, colors) == 0)
		{
			Con_SafePrintLinef ("DIB_SetPalette() - SetDIBColorTable failed");
		}
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
// Early
#ifdef WINQUAKE_RENDERER_SUPPORT
	VID_Palette_NewGame ();
#endif // WINQUAKE_RENDERER_SUPPORT


}

#endif // PLATFORM_WINDOWS
#endif // !CORE_SDL + !CORE_GL /*win thru GL has GLQUAKE false but CORE_GL true*/
#endif // !GLQUAKE - WinQuake Software renderer
