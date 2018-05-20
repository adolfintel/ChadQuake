/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
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

#ifndef __VID_H__
#define __VID_H__

// vid.h -- video driver defs

// moved here for global use -- kristian
typedef enum { MODE_UNINIT = -1, MODE_WINDOWED = 0, MODE_FULLSCREEN = 1 } modestate_t;

#ifdef WINQUAKE_RENDERER_SUPPORT

#define VID_CBITS	6
#define VID_GRADES	(1 << VID_CBITS)
typedef byte pixel_t; // a pixel can be one, two, or four bytes

#define WINQUAKE_MAX_WIDTH_3000  3000	// MAXWIDTH in r_shared.h is 3000.  Does not cascade into other definitions as much as MAXHEIGHT
#define WINQUAKE_MAX_HEIGHT_1080 1080	// Must also change MAXHEIGHT 1080 in r_shared.h, d_ifacea.h.  This affects asm.
#endif // WINQUAKE_RENDERER_SUPPORT

#define QWIDTH_MINIMUM_320 320

#ifdef GLQUAKE
#define QHEIGHT_MINIMUM_2XX 200		// 200, supported mostly for testing
#else
#define QHEIGHT_MINIMUM_2XX 240		// 240, mostly so double size is 640x480 // I guess.
#endif

typedef struct vrect_s
{
	int			x,y,width,height;
	struct vrect_s	*pnext;	// Baker: ASM expects this in struct
} vrect_t;

enum {TEARDOWN_NO_DELETE_GL_CONTEXT_0 = 0, TEARDOWN_FULL_1 = 1};

enum {USER_SETTING_FAVORITE_MODE = 0, ALT_ENTER_TEMPMODE = 1};

#define MAX_MODE_LIST				600
#define MAX_MODE_WIDTH_10000		1280
#define MAX_MODE_HEIGHT_10000		1024
#define MIN_MODE_WIDTH_640			640
#define MIN_MODE_HEIGHT_400			400

#define MIN_WINDOWED_MODE_WIDTH		320
#define MIN_WINDOWED_MODE_HEIGHT	200

typedef struct
{
	modestate_t	type;
#ifdef PLATFORM_OSX
	void*		ptr;	// Baker: I use this for OS X
#endif // PLATFORM_OSX
	int			width;
	int			height;
	int			bpp;
#ifdef SUPPORTS_REFRESHRATE
	int			refreshrate;
#endif // SUPPORTS_REFRESHRATE
} vmode_t;


#define VID_MIN_CONTRAST 1.0
#define VID_MAX_CONTRAST 2.0

#define VID_MIN_POSSIBLE_GAMMA 0.5
#define VID_MAX_POSSIBLE_GAMMA 4.0

#define VID_MIN_MENU_GAMMA 0.5
#define VID_MAX_MENU_GAMMA 1.0


typedef struct mrect_s
{
	int				left, right, bottom, top;
	int				center_x, center_y;
	int				width, height;
} mrect_t;

// Baker: Similar to GL Initial setup after window is constructed, needs to know palette
#ifdef WINQUAKE_GL // !defined(GLQUAKE) && defined(CORE_GL) // WinQuake GL
typedef struct {
	unsigned		rgbapal[PALETTE_COLORS_256];
	unsigned int	texslot;
	int				w, h;
	int				numpels;
	int				width_pow2, height_pow2;
	float			s1, t1;
	unsigned		*rgbabuffer;
	pixel_t			*pixelbytes;	// Link vid.buffer to us.
} wingl_t;
#endif // WINQUAKE_GL // !GLQUAKE but CORE_GL - WinQuake GL

typedef struct
{
	vmode_t			modelist[MAX_MODE_LIST];
	int				nummodes; // The number of them filled in

	vmode_t			screen;

#ifdef WINQUAKE_GL // !defined(GLQUAKE) && defined(CORE_GL) // WinQuake GL
	wingl_t			wingl;
#endif // WINQUAKE_GL -- !GLQUAKE but CORE_GL - WinQuake GL

#ifdef SUPPORTS_RESIZABLE_WINDOW // Windows resize on the fly
	int				border_width;
	int				border_height;
	mrect_t			client_window;
	int				resized; // Baker: resize on fly, if resized this flag gets cleared after resize actions occur.  Set 2 for setmode.
#endif // SUPPORTS_RESIZABLE_WINDOW
	int				modenum_screen;		// mode # on-screen now
	int				modenum_user_selected;	// mode # user intentionally selected (i.e. not an ALT-ENTER toggle)

	int				conwidth;		// Largely used by software renderer, rarely used by GL?
	int				conheight;		// Largely used by software renderer, rarely used by GL?

#ifdef GLQUAKE_RENDERER_SUPPORT // The irony
	int				maxwarpwidth;
	int				maxwarpheight;
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
// These need to be set when screen changes
	unsigned		rowbytes;		// may be > width if displayed in a window
	float			stretch_x;		// If we scaled for large resolution  WINDOWS StretchBlt usage
	float			stretch_y;		// If we scaled for large resolution  WINDOWS StretchBlt usage
	float			aspect;			// width / height -- < 0 is taller than wide

	pixel_t			*buffer;		// invisible buffer, the main vid.buffer!
	byte			*basepal;		// host_basepal
	pixel_t			*colormap;		// 256 * VID_GRADES size
	byte			altblack;
	byte			alpha50map[PALETTE_COLORS_256 * PALETTE_COLORS_256]; // Best in front and best behind

	byte			*surfcache;
	int				surfcachesize;
//	int				highhunkmark;  extincted by malloc instead

	int				stretch_old_cvar_val;	// The cvar value at time of set.
#endif // WINQUAKE_RENDERER_SUPPORT
#if defined(WINQUAKE_RENDERER_SUPPORT) && defined(PLATFORM_OSX)
	short			*pzbuffer;

	unsigned int	texture;
	unsigned int	texture_actual_width, texture_actual_height; //POW2 upsized
	float			texture_s1, texture_t1;

	unsigned int    rgbapal[PALETTE_COLORS_256];
	unsigned int	*bitmap;

	cbool			texture_initialized;
#endif // WINQUAKE_RENDERER_SUPPORT + PLATFORM_OSX

// Color
#ifdef GLQUAKE_RENDERER_SUPPORT
	unsigned int	d_8to24table[PALETTE_COLORS_256];  // Palette representation in RGB color
	cbool			ever_set_gamma;
#endif // GLQUAKE_RENDERER_SUPPORT

	byte			gammatable[GAMMA_UNITS_256];		// Palette gamma ramp (gamma, contrast)
	
#ifdef WINQUAKE_RENDERER_SUPPORT
	byte			curpal[PALETTE_SIZE_768];	// Palette RGB with gamma ramp
#endif // WINQUAKE_RENDERER_SUPPORT

	int				numpages;
	int				recalc_refdef;		// if true, recalc vid-based stuff

	vmode_t			desktop;
	cbool			canalttab;
	cbool			wassuspended;
	cbool			ActiveApp;
	cbool			Hidden;
	cbool			Minimized;
	cbool			sound_suspended;
	cbool			initialized;
	cbool			system_enhanced_keys; // A bit dumb, but we need this somewhere global and it relates to video directly because initial video setup triggers this.
	cbool			nomouse;

#ifdef GLQUAKE_RENDERER_SUPPORT // Windows resize on the fly

	cbool			warp_stale;
	cbool			consize_stale;

	cbool			scale_dirty; // Happens if vid.conwidth changes, scr_scaleauto changes, scr_menuscale changes, scr_sbarscalechanges, vid_conscale
	float			menu_scale;
	float			sbar_scale;	

#endif  // GLQUAKE_RENDERER_SUPPORT

	int				direct3d;
	int				multisamples;
} viddef_t;

extern	viddef_t	vid;				// global video state

extern	int clx, cly, clwidth, clheight;


//cmd void VID_Test (void);
//cmd void VID_Restart_f (void);
void VID_Alt_Enter_f (void);


// During run ...
void VID_AppActivate(cbool fActive, cbool minimize, cbool hidden);
void VID_Local_Suspend (cbool bSuspend);
void VID_BeginRendering (int *x, int *y, int *width, int *height);
void VID_EndRendering (void);
void VID_SwapBuffers (void);
void VID_Local_SwapBuffers (void);

// Platform localized video setup ...
vmode_t VID_Local_GetDesktopProperties (void);
void VID_Local_Window_PreSetup (void);

// Main
void VID_Init (void);
void VID_Local_Init (void);
int VID_SetMode (int modenum);
cbool VID_Local_SetMode (int modenum);
void VID_Shutdown (void);
void VID_Local_Shutdown (void);
void VID_Local_Window_Renderer_Teardown (int destroy, cbool reset_video_mode); // Many versions don't care about the resize, but WinQuake does.
void VID_Local_Set_Window_Caption (const char *text);

// Video modes
cbool VID_Mode_Exists (vmode_t *test, int *outmodenum);
void VID_Local_AddFullscreenModes (void);


// Cvars and modes
vmode_t VID_Cvars_To_Mode (void);
void VID_Cvars_Sync_To_Mode (vmode_t *mymode);
void VID_Cvars_Set_Autoselect_Temp_Fullscreen_Mode (int favoritemode);
void VID_Cvars_Set_Autoselect_Temp_Windowed_Mode (int favoritemode);

#ifdef CORE_GL // Applies to either GLQUAKE or WinQuake GL
void VID_Renderer_Setup (void);
#endif // CORE_GL

#ifdef GLQUAKE_RENDERER_SUPPORT
void VID_Local_Startup_Dialog (void);
void VID_Local_Multisample_f (cvar_t *var);
void VID_BrightenScreen (void); // Non-hardware gamma

// Gamma Table
void VID_Gamma_Init (void);
void VID_Gamma_Think (void);
void VID_Gamma_Shutdown (void);
cbool VID_Local_IsGammaAvailable (unsigned short* ramps);
void VID_Local_Gamma_Set (unsigned short* ramps);
cbool VID_Local_Gamma_Reset (void);
void VID_Gamma_Clock_Set (void); // Initiates a "timer" to ensure gamma is good in fullscreen

void Vid_Gamma_TextureGamma_f (lparse_t *line);
#endif // GLQUAKE_RENDERER_SUPPORT


#ifdef SUPPORTS_RESIZABLE_WINDOW
// Baker: resize on the fly
void VID_Resize_Check (int resize_level); // System messages calls this
void VID_BeginRendering_Resize_Think_Resize_Act (void); // Exclusively called by vid.c but we'll declare it here.
// static void VID_BeginRendering_Resize_Think (void); // Internal to vid.c
// Baker: end resize on the fly
#endif // SUPPORTS_RESIZABLE_WINDOW

#ifdef WINQUAKE_RENDERER_SUPPORT
void VID_Local_SetPalette (byte *palette);
// called after any gamma correction

void VID_ShiftPalette (byte *palette);
// called for bonus and pain flashes, and for underwater color changes

void VID_Update (vrect_t *rects); // Equivalent of swap buffers for WinQuake


void VID_Local_Modify_Palette (byte *palette); // Only MH Windows WinQuake needs this.  On Mac won't do anything.  On WinQuake GL, won't do anything.
cbool VID_CheckGamma (void);  // Equivalent of VID_Gamma_Think
void VID_Palette_NewGame (void); // New game needs to reload palette (a few rare mods use custom palette / colormap)

#endif // WINQUAKE_RENDERER_SUPPORT


// Vsync on Windows doesn't work for software renderer
// But could probably be made to work
void VID_Local_Vsync (void);
void VID_Local_Vsync_f (cvar_t *var);

// Baker: Doesn't apply on a Mac
cbool VID_Local_Vsync_Init (const char *gl_extensions_str);

unsigned *VID_GetBuffer_RGBA_Malloc (int *width, int *height, cbool bgra);



#endif	// ! __VID_H__

