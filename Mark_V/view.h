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
// view.h

#ifndef __VIEW_H__
#define __VIEW_H__


#ifdef GLQUAKE_VIEW_BLENDS
extern float v_blend[4];
#endif // GLQUAKE_VIEW_BLENDS

extern vec3_t v_punchangles[2]; // Baker: Used by cl_parse.c (johnfitz)

void View_Init (void);
void View_NewMap (void);
void View_CalcBlend (void);

//
// view.c
//
void View_StartPitchDrift (lparse_t *unused);
void View_StopPitchDrift (void);

void View_RenderView (void);
void View_ParseDamage (void);
void View_SetContentsColor (int contents);

void View_UpdateBlend (void);
void View_Blend_Stale (void); // Baker: To tell software renderer palette is out-of-date

#define WEAPON_INVISIBILITY_ALPHA 0.4

// Baker: Could these be used to improve chasecam??
// as in detecting if the chase_cam has a novis leaf?
extern vec3_t nonchase_origin;
extern vec3_t nonchase_angles;

#endif	// ! __VIEW_H__

