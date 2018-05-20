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
// view.c -- player eye positioning

#include "quakedef.h"

/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/



vec3_t	v_punchangles[2]; //johnfitz -- copied from cl.punchangle.  0 is current, 1 is previous value. never the same unless map just loaded

/*
===============
View_CalcBob

===============
*/
float View_CalcBob (void)
{
	float	bob;
	float	cycle;

	// catch division by 0 below
	if (cl_bobcycle.value < 0.001)
		return 0;

	cycle = cl.ctime - (int)(cl.ctime / cl_bobcycle.value) * cl_bobcycle.value;
	cycle /= cl_bobcycle.value;

	if (cycle < cl_bobup.value)
		cycle = M_PI * cycle / cl_bobup.value;
	else
		cycle = M_PI + M_PI * (cycle - cl_bobup.value) / (1.0 - cl_bobup.value);

// bob is proportional to velocity in the xy plane
// (don't count Z, or jumping messes it up)

	bob = sqrt(cl.velocity[0] * cl.velocity[0] + cl.velocity[1] * cl.velocity[1]) * cl_bob.value;
//Con_PrintLinef ("speed: %5.1f", VectorLength(cl.velocity));
	bob = bob * 0.3 + bob * 0.7 * sin(cycle);

	if (bob > 4)
		bob = 4;
	else if (bob < -7)
		bob = -7;

	return bob;

}

/*
===============
View_CalcBobSide

===============
*/
float View_CalcBobSide (void)
{
	float	bobside;
	float	cycle;

	// catch division by 0 below
	if (cl_bobsidecycle.value < 0.001)
		return 0;

	cycle = cl.ctime - (int)(cl.ctime / cl_bobsidecycle.value) * cl_bobsidecycle.value;
	cycle /= cl_bobsidecycle.value;
	if (cycle < cl_bobsideup.value)
		cycle = M_PI * cycle / cl_bobsideup.value;
	else
		cycle = M_PI + M_PI * (cycle - cl_bobsideup.value) / (1.0 - cl_bobsideup.value);

// bob is proportional to velocity in the xy plane
// (don't count Z, or jumping messes it up)

	bobside = sqrt(cl.velocity[0] * cl.velocity[0] + cl.velocity[1] * cl.velocity[1]) * cl_bobside.value;
//Con_PrintLinef ("speed: %5.1f", VectorLength(cl.velocity));
	bobside = bobside * 0.3 + bobside * 0.7 * sin(cycle);
	if (bobside > 4)
		bobside = 4;
	else if (bobside < -7)
		bobside = -7;

	return bobside;
}


void View_StartPitchDrift (lparse_t *unused)
{
#if 1

	if (cl.laststop == cl.time)
	{
		return;		// something else is keeping it from drifting
	}

#endif

	if (cl.nodrift || !cl.pitchvel)
	{
		cl.pitchvel = v_centerspeed.value;
		cl.nodrift = false;
		cl.driftmove = 0;
	}
}

void View_StopPitchDrift (void)
{
	cl.laststop = cl.time;
	cl.nodrift = true;
	cl.pitchvel = 0;
}

/*
===============
View_DriftPitch

Moves the client pitch angle towards cl.idealpitch sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.

Drifting is enabled when the center view key is hit, mlook is released and
lookspring is non 0, or when
===============
*/
void View_DriftPitch (void)
{
	float		delta, move;

	if (cl.noclip_anglehack || !cl.onground || cls.demoplayback )
	//FIXME: noclip_anglehack is set on the server, so in a nonlocal game this won't work.
	{
		cl.driftmove = 0;
		cl.pitchvel = 0;
		return;
	}

// don't count small mouse motion
	if (cl.nodrift)
	{
		if ( fabs(cl.cmd.forwardmove) < cl_forwardspeed.value)
			cl.driftmove = 0;
		else cl.driftmove += cl_frametime;
#pragma message ("MH says fixme but there are lots of these luring around")

		if ( cl.driftmove > v_centermove.value)
		{
			if (lookspring.value)
				View_StartPitchDrift (NULL);
		}

		return;
	}

	delta = cl.idealpitch - cl.viewangles[PITCH];

	if (!delta)
	{
		cl.pitchvel = 0;
		return;
	}

	move = cl_frametime * cl.pitchvel;
	cl.pitchvel += cl_frametime * v_centerspeed.value;

	//Con_PrintLinef ("move: %f (%f)", move, host_frametime);

	if (delta > 0)
	{
		if (move > delta)
		{
			cl.pitchvel = 0;
			move = delta;
		}

		cl.viewangles[PITCH] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			cl.pitchvel = 0;
			move = -delta;
		}

		cl.viewangles[PITCH] -= move;
	}

// Baker: ProQuake doesn't bound here, but we are for consistency
	cl.viewangles[PITCH] = CLAMP (cl_minpitch.value, cl.viewangles[PITCH], cl_maxpitch.value);
}

/*
==============================================================================

	VIEW BLENDING

==============================================================================
*/


cshift_t	cshift_water = {{0,0,0}, 0}; // Baker: start empty, cvar registration sets
cshift_t	cshift_slime = {{0,0,0}, 0}; // Baker: start empty, cvar registration sets
cshift_t	cshift_lava  = {{0,0,0}, 0}; // Baker: start empty, cvar registration sets

#ifdef WINQUAKE_VIEW_BLENDS
cbool blend_dirty;

// Baker: Tell software renderer palette is out of date
void View_Blend_Stale (void)
{
	blend_dirty = true;
}
#endif // WINQUAKE_VIEW_BLENDS


void View_Custom_CShift (cshift_t *cshift, cshift_t *default_cshift, const char *string)
{
	if (string[0] && string[1])
	{
		float values[4];
		int size = sizeof(values)/sizeof(values[0]);

		ParseFloats(string, values, &size);

		cshift->destcolor[0] = values[0];
		cshift->destcolor[1] = values[1];
		cshift->destcolor[2] = values[2];
		cshift->percent = values[3];
		return;
	}

	memcpy (cshift, default_cshift, sizeof(*cshift));  // Empty string.  Default it
}

void View_WaterCshift_f (cvar_t *var)
{
	cshift_t cshift_water_default = { {130,80,50}, 50 };
	View_Custom_CShift (&cshift_water, &cshift_water_default, r_watercshift.string);
#ifdef WINQUAKE_VIEW_BLENDS
	View_Blend_Stale ();
#endif // WINQUAKE_VIEW_BLENDS
}

void View_LavaCshift_f (cvar_t *var)
{
	cshift_t	cshift_lava_default = { {255,80,0}, 150 };
	View_Custom_CShift (&cshift_lava, &cshift_lava_default, r_lavacshift.string);
#ifdef WINQUAKE_VIEW_BLENDS
	View_Blend_Stale ();
#endif // WINQUAKE_VIEW_BLENDS
}

void View_SlimeCshift_f (cvar_t *var)
{
	cshift_t	cshift_slime_default = { {0,25,5}, 150 };
	View_Custom_CShift (&cshift_slime, &cshift_slime_default, r_slimecshift.string);
#ifdef WINQUAKE_VIEW_BLENDS
	View_Blend_Stale ();
#endif // WINQUAKE_VIEW_BLENDS
}

#ifdef GLQUAKE_VIEW_BLENDS
float		v_blend[4];		// rgba 0.0 - 1.0
#endif // GLQUAKE_VIEW_BLENDS


void R_PolyBlendChanged_f (cvar_t *var)
{
#ifdef WINQUAKE_VIEW_BLENDS
	View_Blend_Stale ();
#endif // WINQUAKE_VIEW_BLENDS
}


/*
===============
View_ParseDamage
===============
*/
void View_ParseDamage (void)
{
	int		armor, blood;
	vec3_t	from;
	int		i;
	vec3_t	forward, right, up;
	entity_t	*ent;
	float	side;
	float	count;

	armor = MSG_ReadByte ();
	blood = MSG_ReadByte ();

	for (i=0 ; i<3 ; i++)
		from[i] = MSG_ReadCoord ();

	count = blood*0.5 + armor*0.5;

	if (count < 10)
		count = 10;

	cl.faceanimtime = cl.time + 0.2;		// but sbar face into pain frame

	cl.cshifts[CSHIFT_DAMAGE].percent += 3*count;

	if (cl.cshifts[CSHIFT_DAMAGE].percent < 0)
		cl.cshifts[CSHIFT_DAMAGE].percent = 0;

	if (cl.cshifts[CSHIFT_DAMAGE].percent > 150)
		cl.cshifts[CSHIFT_DAMAGE].percent = 150;

	if (armor > blood)
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 200;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 100;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 100;
	}
	else if (armor)
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 220;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 50;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 50;
	}
	else
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 255;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 0;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 0;
	}

// calculate view angle kicks
	ent = &cl_entities[cl.viewentity_player];

	VectorSubtract (from, ent->origin, from);
	VectorNormalize (from);

	AngleVectors (ent->angles, forward, right, up);

	side = DotProduct (from, right);
	cl.v_dmg_roll = count*side*v_kickroll.value;

	side = DotProduct (from, forward);
	cl.v_dmg_pitch = count*side*v_kickpitch.value;

	cl.v_dmg_time = v_kicktime.value;
}


void View_NewMap (void)
{
#ifdef WINQUAKE_VIEW_BLENDS
	View_Blend_Stale (); // To clear a damage shift
#endif // WINQUAKE_VIEW_BLENDS
}


/*
==================
View_cshift_f
==================
*/
void View_cshift_f (lparse_t *line)
{
	cl.cshift_empty.destcolor[0] = atoi(line->args[1]);
	cl.cshift_empty.destcolor[1] = atoi(line->args[2]);
	cl.cshift_empty.destcolor[2] = atoi(line->args[3]);
	cl.cshift_empty.percent = atoi(line->args[4]);
}


/*
==================
View_BonusFlash_f

When you run over an item, the server sends this command
==================
*/
void View_BonusFlash_f (lparse_t *unused)
{
	cl.cshifts[CSHIFT_BONUS].destcolor[0] = 215;
	cl.cshifts[CSHIFT_BONUS].destcolor[1] = 186;
	cl.cshifts[CSHIFT_BONUS].destcolor[2] = 69;
	cl.cshifts[CSHIFT_BONUS].percent = 50;
}

/*
=============
View_SetContentsColor

Underwater, lava, etc each has a color shift
=============
*/
void View_SetContentsColor (int contents)
{
	switch (contents)
	{
	case CONTENTS_EMPTY:
	case CONTENTS_SOLID:
	case CONTENTS_SKY: //johnfitz -- no blend in sky
		cl.cshifts[CSHIFT_CONTENTS] = cl.cshift_empty;
		break;
	case CONTENTS_LAVA:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_lava;
		break;
	case CONTENTS_SLIME:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_slime;
		break;
	default:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_water;
	}
}

/*
=============
View_CalcPowerupCshift
=============
*/
void View_CalcPowerupCshift (void)
{
	if (cl.items & IT_QUAD)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 255;
		cl.cshifts[CSHIFT_POWERUP].percent = v_polyblend_lite.value ? 20: 30;
	}
	else if (cl.items & IT_SUIT)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		cl.cshifts[CSHIFT_POWERUP].percent = v_polyblend_lite.value ? 10: 20;
	}
	else if (cl.items & IT_INVISIBILITY)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 100;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 100;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 100;
		cl.cshifts[CSHIFT_POWERUP].percent = v_polyblend_lite.value ? 30: 100;
	}
	else if (cl.items & IT_INVULNERABILITY)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		cl.cshifts[CSHIFT_POWERUP].percent = v_polyblend_lite.value ? 10: 30;
	}
	else
		cl.cshifts[CSHIFT_POWERUP].percent = 0;
}

#ifdef GLQUAKE_VIEW_BLENDS
/*
=============
View_CalcBlend
=============
*/
void View_CalcBlend (void)
{
	float	r, g, b, a, a2;
	int		j;

	r = 0;
	g = 0;
	b = 0;
	a = 0;

	for (j=0 ; j<NUM_CSHIFTS ; j++)
	{
		if (!v_cshiftpercent.value)
			continue;

		//johnfitz -- only apply leaf contents color shifts during intermission
		if (cl.intermission && j != CSHIFT_CONTENTS)
			continue;

		//johnfitz

		a2 = ((cl.cshifts[j].percent * v_cshiftpercent.value * (v_polyblend_lite.value && j == CSHIFT_CONTENTS ? 0.20 : 1.0)   ) / 100.0) / 255.0;

		if (!a2)
			continue;

		a = a + a2*(1-a);
		a2 = a2/a;
		r = r*(1-a2) + cl.cshifts[j].destcolor[0]*a2;
		g = g*(1-a2) + cl.cshifts[j].destcolor[1]*a2;
		b = b*(1-a2) + cl.cshifts[j].destcolor[2]*a2;
	}

	v_blend[0] = r/255.0;
	v_blend[1] = g/255.0;
	v_blend[2] = b/255.0;
	v_blend[3] = a;

	if (v_blend[3] > 1)
		v_blend[3] = 1;

	if (v_blend[3] < 0)
		v_blend[3] = 0;
}


/*
============
View_PolyBlend -- johnfitz -- moved here from gl_rmain.c, and rewritten to use glOrtho
============
*/
void View_PolyBlend (void)
{
	if (!v_polyblend.value || !v_blend[3])
		return;

	GL_DisableMultitexture();

	eglDisable (GL_ALPHA_TEST);
	eglDisable (GL_TEXTURE_2D);
	eglDisable (GL_DEPTH_TEST);
	eglEnable (GL_BLEND);

	eglMatrixMode(GL_PROJECTION);
    eglLoadIdentity ();
	eglOrtho (0, 1, 1, 0, -99999, 99999);
	eglMatrixMode(GL_MODELVIEW);
    eglLoadIdentity ();

	eglColor4f (v_blend[0], v_blend[1], v_blend[2], v_blend[3] * v_polyblend.value);

	eglBegin (GL_QUADS);
	eglVertex2f (0,0);
	eglVertex2f (1, 0);
	eglVertex2f (1, 1);
	eglVertex2f (0, 1);
	eglEnd ();

	eglDisable (GL_BLEND);
	eglEnable (GL_DEPTH_TEST);
	eglEnable (GL_TEXTURE_2D);
	eglEnable (GL_ALPHA_TEST);
}
#endif // GLQUAKE_VIEW_BLENDS

void View_UpdateBlend (void)
{
	int		i, j;
	cbool	blend_changed = false;

#ifdef WINQUAKE_RENDERER_SUPPORT
	byte	*basepal, *newpal;
	byte	pal[768];
	int		r,g,b;
	cbool force;
#endif // WINQUAKE_RENDERER_SUPPORT

	if (cls.state != ca_connected || cl.intermission) //	Baker: eliminate shifts when disconnected
	{
		cl.cshifts[CSHIFT_CONTENTS] = cl.cshift_empty;
		cl.cshifts[CSHIFT_POWERUP].percent = 0;
	}
	else
	{
		View_CalcPowerupCshift ();
	}

#ifdef WINQUAKE_RENDERER_SUPPORT
	if (blend_dirty)
		blend_changed = true;
#endif // WINQUAKE_RENDERER_SUPPORT


	for (i=0 ; i<NUM_CSHIFTS ; i++)
	{
		if (cl.cshifts[i].percent != cl.prev_cshifts[i].percent)
		{
			blend_changed = true;
			cl.prev_cshifts[i].percent = cl.cshifts[i].percent;
		}

		for (j=0 ; j<3 ; j++)
		{
			if (cl.cshifts[i].destcolor[j] != cl.prev_cshifts[i].destcolor[j])
			{
				blend_changed = true;
				cl.prev_cshifts[i].destcolor[j] = cl.cshifts[i].destcolor[j];
			}
		}
	}

	if (!cl.paused) // Baker: Pause bonus flashes
	{
	// drop the damage value
		cl.cshifts[CSHIFT_DAMAGE].percent -= host_frametime_ * 150; // 28 Apr 2015

		if (cl.cshifts[CSHIFT_DAMAGE].percent <= 0)
			cl.cshifts[CSHIFT_DAMAGE].percent = 0;

	// drop the bonus value
		cl.cshifts[CSHIFT_BONUS].percent -= host_frametime_ * 100;  // 28 Apr 2015

		if (cl.cshifts[CSHIFT_BONUS].percent <= 0)
			cl.cshifts[CSHIFT_BONUS].percent = 0;
	}

#ifdef GLQUAKE_RENDERER_SUPPORT
	if (blend_changed)
		View_CalcBlend ();
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
	force = VID_CheckGamma ();
	if (!blend_changed && !force)
		return;

	basepal = vid.basepal;
	newpal = pal;

	for (i = 0 ; i < PALETTE_COLORS_256 ; i++)
	{
		r = basepal[0];
		g = basepal[1];
		b = basepal[2];
		basepal += 3;
		if (v_polyblend.value)		// JPG 3.30 - added r_polyblend
		{
			for (j=0 ; j<NUM_CSHIFTS ; j++)
			{
				float factor = j == CSHIFT_CONTENTS  ? (v_polyblend_lite.value ? 0.333: 1) : (v_polyblend_lite.value ? 0.9: 1) ;
				int shift_amount = (int)(cl.cshifts[j].percent * factor); // (0-255)
				r += (shift_amount*(cl.cshifts[j].destcolor[0]-r)) >> 8;
				g += (shift_amount*(cl.cshifts[j].destcolor[1]-g)) >> 8;
				b += (shift_amount*(cl.cshifts[j].destcolor[2]-b)) >> 8;				
			}
		}

		newpal[0] = vid.gammatable[r];
		newpal[1] = vid.gammatable[g];
		newpal[2] = vid.gammatable[b];
		newpal += 3;
	}

	VID_ShiftPalette (pal);
	Sbar_Changed (); // Baker: Or should do the everything one?

	blend_dirty = false;
#endif // WINQUAKE_RENDERER_SUPPORT
}

/*
==============================================================================

	VIEW RENDERING

==============================================================================
*/

float angledelta (float a)
{
	a = anglemod(a);

	if (a > 180)
		a -= 360;

	return a;
}

/*
==================
CalcGunAngle
==================
*/
void CalcGunAngle (void)
{
	float	yaw, pitch, move;
	static float oldyaw = 0;
	static float oldpitch = 0;

	yaw = r_refdef.viewangles[YAW];
	pitch = -r_refdef.viewangles[PITCH];

	yaw = angledelta(yaw - r_refdef.viewangles[YAW]) * 0.4;

	if (yaw > 10)
		yaw = 10;

	if (yaw < -10)
		yaw = -10;

	pitch = angledelta(-pitch - r_refdef.viewangles[PITCH]) * 0.4;

	if (pitch > 10)
		pitch = 10;

	if (pitch < -10)
		pitch = -10;

	move = host_frametime_ * 20;

	if (yaw > oldyaw)
	{
		if (oldyaw + move < yaw)
			yaw = oldyaw + move;
	}
	else
	{
		if (oldyaw - move > yaw)
			yaw = oldyaw - move;
	}

	if (pitch > oldpitch)
	{
		if (oldpitch + move < pitch)
			pitch = oldpitch + move;
	}
	else
	{
		if (oldpitch - move > pitch)
			pitch = oldpitch - move;
	}

	oldyaw = yaw;
	oldpitch = pitch;

	cl.viewent_gun.angles[YAW] = r_refdef.viewangles[YAW] + yaw;
	cl.viewent_gun.angles[PITCH] = - (r_refdef.viewangles[PITCH] + pitch);

	cl.viewent_gun.angles[ROLL] -= v_idlescale.value * sin(host_frametime_ * v_iroll_cycle.value) * v_iroll_level.value;
	cl.viewent_gun.angles[PITCH] -= v_idlescale.value * sin(host_frametime_ * v_ipitch_cycle.value) * v_ipitch_level.value;
	cl.viewent_gun.angles[YAW] -= v_idlescale.value * sin(host_frametime_ * v_iyaw_cycle.value) * v_iyaw_level.value;
}

/*
==============
View_BoundOffsets
==============
*/
void View_BoundOffsets (void)
{
	entity_t	*ent;

	ent = &cl_entities[cl.viewentity_player];

// absolutely bound refresh relative to entity clipping hull
// so the view can never be inside a solid wall
	r_refdef.vieworg[0] = c_max(r_refdef.vieworg[0], ent->origin[0] - 14);
	r_refdef.vieworg[0] = c_min(r_refdef.vieworg[0], ent->origin[0] + 14);
	r_refdef.vieworg[1] = c_max(r_refdef.vieworg[1], ent->origin[1] - 14);
	r_refdef.vieworg[1] = c_min(r_refdef.vieworg[1], ent->origin[1] + 14);
	r_refdef.vieworg[2] = c_max(r_refdef.vieworg[2], ent->origin[2] - 22);
	r_refdef.vieworg[2] = c_min(r_refdef.vieworg[2], ent->origin[2] + 30);
}

/*
==============
View_AddIdle

Idle swaying
==============
*/
void View_AddIdle (void)
{
	r_refdef.viewangles[ROLL] += v_idlescale.value * sin(cl.ctime * v_iroll_cycle.value) * v_iroll_level.value;
	r_refdef.viewangles[PITCH] += v_idlescale.value * sin(cl.ctime * v_ipitch_cycle.value) * v_ipitch_level.value;
	r_refdef.viewangles[YAW] += v_idlescale.value * sin(cl.ctime * v_iyaw_cycle.value) * v_iyaw_level.value;
}


/*
==============
View_CalcViewRoll

Roll is induced by movement and damage
==============
*/
void View_CalcViewRoll (void)
{
	float		side;

	side = Math_CalcRoll (cl_entities[cl.viewentity_player].angles, cl.velocity, cl_rollangle.value, cl_rollspeed.value);
	r_refdef.viewangles[ROLL] += side;

	if (cl.v_dmg_time > 0 && !cl.paused) // Baker: Pause view blends
	{
		r_refdef.viewangles[ROLL] += cl.v_dmg_time/v_kicktime.value * cl.v_dmg_roll;
		r_refdef.viewangles[PITCH] += cl.v_dmg_time/v_kicktime.value * cl.v_dmg_pitch;
		cl.v_dmg_time -= host_frametime_;
	}

	if (cl.stats[STAT_HEALTH] <= 0)
	{
		r_refdef.viewangles[ROLL] = 80;	// dead view angle
		return;
	}

}

/*
==================
View_CalcIntermissionRefdef

==================
*/
void View_CalcIntermissionRefdef (void)
{
	entity_t	*ent, *view;
	float		old;

// ent is the player model (visible when out of body)
	ent = &cl_entities[cl.viewentity_player];

// view is the weapon model (only visible from inside body)
	view = &cl.viewent_gun;

	VectorCopy (ent->origin, r_refdef.vieworg);
	VectorCopy (ent->angles, r_refdef.viewangles);
	view->model = NULL;

// always idle in intermission
	old = v_idlescale.value;
	v_idlescale.value = 1;
	View_AddIdle ();
	v_idlescale.value = old;
}

/*
==================
View_CalcRefdef
==================
*/
void View_CalcRefdef (void)
{
	entity_t	*ent, *view;
	int			i;
	vec3_t		forward, right, up;
	vec3_t		angles;
	float		bob, bobside;
	static float oldz = 0;
	static vec3_t punch = {0,0,0}; //johnfitz -- v_gunkick
	float delta; //johnfitz -- v_gunkick

	View_DriftPitch ();

// ent is the player model (visible when out of body)
	ent = &cl_entities[cl.viewentity_player];

// view is the weapon model (only visible from inside body)
	view = &cl.viewent_gun;

// transform the view offset by the model's matrix to get the offset from
// model origin for the view
	ent->angles[YAW] = cl.lerpangles[YAW];	// the model should face the view dir
	ent->angles[PITCH] = -cl.lerpangles[PITCH];	// the model should face

	bobside = cl_sidebobbing.value ? View_CalcBobSide () : 0;
	bob = View_CalcBob ();

// refresh position
	VectorCopy (ent->origin, r_refdef.vieworg);
	r_refdef.vieworg[2] += cl.viewheight + bob;

// never let it sit exactly on a node line, because a water plane can
// dissapear when viewed with the eye exactly on it.
// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
	r_refdef.vieworg[0] += 1.0/32;
	r_refdef.vieworg[1] += 1.0/32;
	r_refdef.vieworg[2] += 1.0/32;

	VectorCopy (cl.lerpangles, r_refdef.viewangles);

	View_CalcViewRoll ();
	View_AddIdle ();

// offsets
	angles[PITCH] = -ent->angles[PITCH]; // because entity pitches are actually backward
	angles[YAW] = ent->angles[YAW];
	angles[ROLL] = ent->angles[ROLL];

	AngleVectors (angles, forward, right, up);

	View_BoundOffsets ();

// set up gun position
	VectorCopy (cl.viewangles, view->angles);

	CalcGunAngle ();

	VectorCopy (ent->origin, view->origin);
	view->origin[2] += cl.viewheight;

	for (i=0 ; i<3 ; i++)
		if (cl_sidebobbing.value)
		{
			view->origin[i] += right[i] * bobside * 0.2;
			view->origin[i] += up[i] * bob * 0.2;
		}
		else
			view->origin[i] += forward[i] * bob * 0.4;

	view->origin[2] += bob;

	//johnfitz -- removed all gun position fudging code (was used to keep gun from getting covered by sbar)
	// fudge position around to keep amount of weapon visible roughly equal with different viewsize
	if (r_viewmodel_offset.value) {
      VectorMA (view->origin,  r_viewmodel_offset.value, right,   view->origin);
	}

	if (r_viewmodel_quake.value)
	{
		if (scr_viewsize.value == 110)
			view->origin[2] += 1;
		else if (scr_viewsize.value == 100)
			view->origin[2] += 2;
		else if (scr_viewsize.value == 90)
			view->origin[2] += 1;
		else if (scr_viewsize.value == 80)
			view->origin[2] += 0.5;
	}

	view->model = cl.model_precache[cl.stats[STAT_WEAPON]];
	view->frame = cl.stats[STAT_WEAPONFRAME];
#ifdef GLQUAKE_COLORMAP_TEXTURES
	view->colormap = 0;
#endif // GLQUAKE_COLORMAP_TEXTURES

#ifdef WINQUAKE_COLORMAP_TRANSLATION
	view->colormap = vid.colormap;
#endif // WINQUAKE_COLORMAP_TRANSLATION


//johnfitz -- v_gunkick
	if (v_gunkick.value == 1) //original quake kick
		VectorAdd (r_refdef.viewangles, cl.punchangle, r_refdef.viewangles);

	if (v_gunkick.value == 2) //lerped kick
	{
		for (i=0; i<3; i++)
		{
			if (punch[i] != v_punchangles[0][i])
			{
				//speed determined by how far we need to lerp in 1/10th of a second
				delta = (v_punchangles[0][i]-v_punchangles[1][i]) * host_frametime_ * 10;

				if (delta > 0)
					punch[i] = c_min(punch[i]+delta, v_punchangles[0][i]);
				else if (delta < 0)
					punch[i] = c_max(punch[i]+delta, v_punchangles[0][i]);
			}
		}

		VectorAdd (r_refdef.viewangles, punch, r_refdef.viewangles);
	}
//johnfitz

// smooth out stair step ups
	//FIXME: noclip_anglehack is set on the server, so in a nonlocal game this won't work.
	if (v_smoothstairs.value && !cl.noclip_anglehack && cl.onground && ent->origin[2] - oldz > 0) //johnfitz -- added exception for noclip
	{
		float steptime;

		steptime = cl.ctime - cl.oldtime;

		if (steptime < 0)
			//FIXME	I_Error ("steptime < 0");
			steptime = 0;

		oldz += steptime * 80;

		if (oldz > ent->origin[2])
			oldz = ent->origin[2];

		if (ent->origin[2] - oldz > 12)
			oldz = ent->origin[2] - 12;

		r_refdef.vieworg[2] += oldz - ent->origin[2];
		view->origin[2] += oldz - ent->origin[2];
	}
	else
		oldz = ent->origin[2];

	if (chase_active.value)
	{
		VectorCopy (r_refdef.vieworg,  nonchase_origin);
		VectorCopy (r_refdef.viewangles,  nonchase_angles);
		Chase_UpdateForDrawing (); //johnfitz
	}
}

// Attempt to get chase camera to use visibility for
// its actual position, not the player's position.
// This fails for a number of different reasons.
// The chase camera might be in a different visibility leaf than the player
// and the server only sends entities based on actual player position
vec3_t nonchase_origin;
vec3_t nonchase_angles;

/*
==================
View_RenderView

The player's clipping box goes from (-16 -16 -24) to (16 16 32) from
the entity origin, so any view position inside that will be valid
==================
*/

void View_RenderView (void)
{
	if (console1.forcedup)
		return;

	if (cl.intermission)
		View_CalcIntermissionRefdef ();
	else // Baker: commented out conditions
		View_CalcRefdef ();
	
	R_RenderView ();

#ifdef GLQUAKE_ENTITY_INSPECTOR // GLQuake only
	Entity_Inspector_Think ();
#endif // GLQUAKE_ENTITY_INSPECTOR

#ifdef GLQUAKE_VIEW_BLENDS
	View_PolyBlend (); //johnfitz -- moved here from R_Renderview ();
#endif // GLQUAKE_VIEW_BLENDS

}

/*
==============================================================================

	INIT

==============================================================================
*/

/*
=============
View_Init
=============
*/
void View_Init (void)
{
	Cmd_AddCommands (View_Init);


}

