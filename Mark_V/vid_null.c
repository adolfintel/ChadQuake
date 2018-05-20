/*
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
// vid.c -- common video


#include "quakedef.h"

viddef_t	vid;				// global video state

//
// set mode and restart
//

/*
 ================
 VID_SetMode
 ================
 */

static void VID_Activate (cbool active);
int VID_SetMode (int modenum)
{
	
	return true;
}

cbool VID_Restart (int flags /* favorite vs. temp*/)
{
	
	return true;
}

/*
 ================
 VID_Test -- johnfitz -- like vid_restart, but asks for confirmation after switching modes
 ================
 */
void VID_Test (void)
{
}

void VID_Alt_Enter_f (void)
{
}


void VID_Restart_f (void)
{
}

//
// in-game
//



static void VID_Activate (cbool active)
{
}


void VID_AppActivate(cbool fActive, cbool minimize, cbool hide)
{
}

void VID_SwapBuffers (void)
{
}


//
// functions to match modes to cvars or reverse
//

void VID_Cvars_Sync_To_Mode (vmode_t* mymode)
{
}

vmode_t VID_Cvars_To_Mode (void)
{
	vmode_t none;
	return none;
}




cbool VID_Read_Early_Cvars (void)
{
	return true;
}


cbool VID_Mode_Exists (vmode_t* test, int *outmodenum)
{
	return true;
}

void VID_MakeMode (modestate_t mode_type, vmode_t *new_mode)
{
}

void VID_Cvars_Set_Autoselect_Temp_Windowed_Mode (int favoritemode)
{
}

int VID_Find_Best_Fullscreen_Modenum (int request_width, int request_height)
{
	return 0;
}

int VID_Cvars_To_Best_Fullscreen_Modenum (void)
{
	return 0;
}


void VID_Cvars_Set_Autoselect_Temp_Fullscreen_Mode (int favoritemode)
{
}


//
// startup / shutdown
//

void	VID_Init (void)
{
}


void VID_Shutdown (void)
{
}




/*
 =================
 VID_BeginRendering -- sets values of clx, cly, clwidth, clheight
 =================
 */
void VID_BeginRendering (int *x, int *y, int *width, int *height)
{
	
}

/*
 =================
 VID_EndRendering
 =================
 */

void VID_EndRendering (void)
{
}


unsigned *VID_GetBuffer_RGBA_Malloc (int *width, int *height, cbool bgra)
{
	
	return NULL;
}

#ifdef WINQUAKE_RENDERER_SUPPORT
void VID_ShiftPalette (unsigned char *palette)
{
}

void VID_Palette_NewGame (void)
{
}
#endif // WINQUAKE_RENDERER_SUPPORT

void VID_Local_Window_PreSetup (void)
{
}


vmode_t VID_Local_GetDesktopProperties (void)
{
    vmode_t desktop = {0};
	
	
    return desktop;
}





//
// vid modes
//


void VID_Local_AddFullscreenModes (void)
{
}

static void sVID_GetBuffers_InitializeTexture (int newwidth, int newheight)
{
}


static void sVID_FlushBuffers_ShutdownTexture (void)
{
}


// Baker: Done by resize handler and VID_Update
void VID_RenderTexture (void)
{
}






static void sVID_Local_SetMode_GetBuffers (int newwidth, int newheight)
{
}



// Baker: Called by setmode and shutdown
void VID_FlushBuffers (void)
{
}


// Baker: Supposed to be called by VID_Activate only but we have WillHide an WillUnhide calling this
// On the Mac.  For now ...
void VID_Local_Suspend (cbool doSuspend)
{
}




void VID_SetOriginalMode (void);
void VID_Local_Window_Renderer_Teardown (int destroy)
{
}


// Baker: Called by setmode and shutdown
// Baker: Note that this destroys the window.  Mac setmode will reinitialize one.
void VID_SetOriginalMode (void)
{
}

void VID_Local_Vsync (void)
{
}

void VID_Local_Vsync_f (cvar_t *var)
{
}

cbool VID_Local_SetMode (int mode)
{
    return true;
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
}

void VID_Local_SwapBuffers (void) // GLQuake uses
{
}

//
//
//
// Palette Set
//
//
//


void VID_Local_Modify_Palette (unsigned char *palette)
{
	// The OS X style implementation doesn't need to do anything here
}

void VID_Local_SetPalette (unsigned char *palette)
{
}

void VID_Local_Set_Window_Caption (const char *text)
{
}


void VID_Local_Multisample_f (cvar_t *var)
{
}


cbool VID_CheckGamma (void) {return 0;}

