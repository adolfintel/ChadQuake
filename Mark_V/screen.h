/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2009-2014 Baker and others

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
// screen.h

#ifndef __SCREEN_H___
#define __SCREEN_H___

// screen.h

void SCR_Init (void);
void SCR_LoadPics (void);

void SCR_UpdateScreen (void);

void SCR_SizeUp (void);
void SCR_SizeDown (void);
void SCR_CenterPrint (const char *str);

void SCR_BeginLoadingPlaque (void);
void SCR_EndLoadingPlaque (void);
void SCR_BeginLoadingPlaque_Force_NoTransition (void);
void SCR_BeginLoadingPlaque_Force_Transition (void);

int SCR_ModalMessage (const char *text, float timeout, cbool enter_out); //johnfitz -- added timeout

void SCR_ScreenShot_Clipboard_f (void);

extern int			sb_lines;
extern int			clearnotify;	// set to 0 whenever notify text is drawn

extern cbool		scr_disabled_for_loading;
extern float		scr_disabled_time;
extern cbool		scr_skipupdate;
extern vrect_t		scr_vrect;


#ifdef GLQUAKE_DRAWING_METHODS
void SCR_Conwidth_f (cvar_t *var);
extern int glquake_scr_tileclear_updates; //johnfitz
#endif // GLQUAKE_DRAWING_METHODS

#ifdef WINQUAKE_RENDERER_SUPPORT
// only the refresh window will be updated unless these variables are flagged
extern	int			winquake_scr_copytop;
extern	int			winquake_scr_copyeverything;
extern	int			winquake_scr_fullupdate;	// set to 0 to force full redraw
#endif // WINQUAKE_RENDERER_SUPPORT

#endif // ! __SCREEN_H___

