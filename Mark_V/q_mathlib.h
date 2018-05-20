/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
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

#ifndef __Q_MATHLIB_H__
#define __Q_MATHLIB_H__

// mathlib.h

#include <math.h>

///////////////////////////////////////////////////////////////////////////////
//  BoxOnPlaneSide
///////////////////////////////////////////////////////////////////////////////

void BOPS_Error (void) __core_attribute__((__noreturn__)); // asms calls in case of error

struct mplane_s;
int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, struct mplane_s *plane); // efrag and world.c->SV_FindTouchedLeafs via macro

// Macro called by efrag.c->R_SplitEntityOnNode/R_SplitEntityOnNode2 and world.c->SV_FindTouchedLeafs
#define BOX_ON_PLANE_SIDE(emins, emaxs, p)	\
	(((p)->type < 3)?						\
	(										\
		((p)->dist <= (emins)[(p)->type])?	\
			1								\
		:									\
		(									\
			((p)->dist >= (emaxs)[(p)->type])?\
				2							\
			:								\
				3							\
		)									\
	)										\
	:										\
		BoxOnPlaneSide( (emins), (emaxs), (p)))


///////////////////////////////////////////////////////////////////////////////
//  Software Render exclusives
///////////////////////////////////////////////////////////////////////////////

int GreatestCommonDivisor (int i1, int i2); // Used by r_sky.c->R_SetSkyFrame
void FloorDivMod (double numer, double denom, int *quotient, int *rem); // used by d_polyse.c->D_PolysetSetUpForLineScan
void R_ConcatRotations (float in1[3][3], float in2[3][3], float out[3][3]); // r_bsp.c->R_RotateBmodel
void R_ConcatTransforms (float in1[3][4], float in2[3][4], float out[3][4]); // r_alias.c->R_AliasSetUpTransformc

// fixed16_t Invert24To16(fixed16_t val); // Not called, but asm WinQuake has an asm function used within ASM.

///////////////////////////////////////////////////////////////////////////////
//  NaN, origin
///////////////////////////////////////////////////////////////////////////////

extern const vec3_t vec3_origin; // called many places
extern int nanmask;
#define	IS_NAN(x) (((*(int *)&x)&nanmask)==nanmask) // used by sv_phys.c: SV_CheckVelocity


///////////////////////////////////////////////////////////////////////////////
//  Misc
///////////////////////////////////////////////////////////////////////////////

// called several places (input, sv_move, pr_cmds, ...)
float anglemod (float a); 

// Used by view.c->View_Custom_CShift
int ParseFloats (const char *s, float *f, int *f_size) ;

// sv_user.c->SV_ClientThink and view.c->View_CalcViewRoll
float Math_CalcRoll (vec3_t angles, vec3_t velocity, float rollangle, float rollspeed); 

// tool_inspector uses this to set box colors
#define VectorSetColor4f(v, r, g, b, a)	 ((v)[0] = (r), (v)[1] = (g), (v)[2] = (b), (v)[3] = (a))

#define DoublePrecisionDotProduct(x,y) ((double)x[0]*y[0]+(double)x[1]*y[1]+(double)x[2]*y[2])

#endif // ! __Q_MATHLIB_H__


