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
// chase.c -- chase camera code

#include "quakedef.h"


typedef struct
{
	char		*description;
	float 		back;
	float 		up;
	float 		right;
	float 		pitch;
	float 		yaw;
	float 		roll;
	cbool	step_to_position;
	cbool	clip_against_objects;		// Camera doesn't clip against map and such
	cbool	locate_target_lookat;	// Targets a lookat spot
	cbool	accepts_pitch;	// Use provided pitch
	cbool	accepts_yaw;	// Use provided yaw
	cbool	accepts_roll;	// Use provided roll
} chase_cam_t;

chase_cam_t chase_classic_plus 	 = {"Classic",		  100.0,   16.0,    0.0,  0.0,  0.0, 0.0, 1, 1, 1, 1, 1, 1};
chase_cam_t chase_classic_noclip = {"Classic noclip", 100.0,   16.0,    0.0,  0.0,  0.0, 0.0, 1, 0, 1, 1, 1, 1};
chase_cam_t chase_overhead 		 = {"Overhead",		    0.0,  200.0,    0.0, 90.0,  0.0, 0.0, 0, 0, 0, 0, 0, 0};
chase_cam_t chase_overhead_yaw 	 = {"Overhead yaw",   125.0,  200.0,    0.0, 45.0,  0.0, 0.0, 1, 0, 0, 0, 1, 0};
chase_cam_t chase_overhead_yaw_c = {"Overhead yaw",   125.0,  200.0,    0.0, 45.0,  0.0, 0.0, 1, 1, 0, 0, 1, 0};
chase_cam_t chase_overhead_clip  = {"Overhead clip",    0.0,  200.0,    0.0, 90.0,  0.0, 0.0, 0, 1, 0, 0, 0, 0};
chase_cam_t chase_rpg 			 = {"RPG",		     -160.0,  200.0, -160.0, 45.0, 45.0, 0.0, 0, 0, 0, 0, 0, 0};
chase_cam_t chase_side 			 = {"Side",          -200.0,   16.0,    0.0,  0.0,  0.0, 0.0, 0, 0, 0, 0, 0, 0};

int chase_mode;
chase_cam_t *chase_type = NULL;

void Chase_Mode_f (lparse_t *line) // overhead, rpg, side, classic, classic
{
//	int 	n;

	if (line->count >= 2)
	{
		int newmode = atoi(line->args[1]);
		chase_mode = newmode;
	}
	else
		chase_mode = chase_mode + 1;

	if (chase_mode > 8 || chase_mode < 0)
		chase_mode = 0;

	if (chase_mode == 0)
		chase_type = NULL;
	else if (chase_mode == 1)
		chase_type = &chase_classic_plus;
	else if (chase_mode == 2)
		chase_type = &chase_classic_noclip;
	else if (chase_mode == 3)
		chase_type = &chase_overhead;
	else if (chase_mode == 4)
		chase_type = &chase_overhead_yaw;
	else if (chase_mode == 5)
		chase_type = &chase_overhead_yaw_c;
	else if (chase_mode == 6)
		chase_type = &chase_overhead_clip;
	else if (chase_mode == 7)
		chase_type = &chase_rpg;
	else if (chase_mode == 8)
		chase_type = &chase_side;

	if (chase_type)
		Con_DPrintLinef ("chase_mode %s", chase_type->description);
	else
		Con_DPrintLinef ("chase_mode toggle");
}


/*
==============
Chase_Init
==============
*/
void Chase_Init (void)
{

	Cmd_AddCommands (Chase_Init);
}

/*
==============
TraceLine

TODO: impact on bmodels, monsters
==============
*/
cbool TraceLine (vec3_t start, vec3_t end, vec3_t impact)
{
	trace_t	trace;

	memset (&trace, 0, sizeof(trace));
	SV_RecursiveHullCheck (cl.worldmodel->hulls, 0, 0, 1, start, end, &trace);

	VectorCopy (trace.endpos, impact);

	if (impact[0] || impact[1] || impact[2])
		return false; // Traceline hit something

	return true; // Traceline hit unimpaired
}

/*
==============
Chase_UpdateForClient -- johnfitz -- orient client based on camera. called after input
==============
*/
void Chase_UpdateForClient (void)
{
	//place camera

	//assign client angles to camera

	//see where camera points

	//adjust client angles to point at the same place
}

/*
==============
Chase_UpdateForDrawing -- johnfitz -- orient camera based on client. called before drawing

TODO: stay at least 8 units away from all walls in this leaf
==============
*/
void LerpVector (const vec3_t from, const vec3_t to, float frac, vec3_t out)
{
	out[0] = from[0] + frac * (to[0] - from[0]);
	out[1] = from[1] + frac * (to[1] - from[1]);
	out[2] = from[2] + frac * (to[2] - from[2]);
}

//static	vec3_t	chase_pos;
//static	vec3_t	chase_angles;

static	vec3_t	chase_dest;
//static	vec3_t	chase_dest_angles;

void Chase_Active_1 (void)
{
	int		i;
	float	dist;
	vec3_t	forward, up, right, dest, stop;

	// if can't see player, reset
	AngleVectors (cl.lerpangles, forward, right, up);
	// calc exact destination
	for (i = 0 ; i < 3 ; i++)
		chase_dest[i] = r_refdef.vieworg[i] - forward[i]*chase_back.value - right[i]*chase_right.value;
	chase_dest[2] = r_refdef.vieworg[2] + chase_up.value;

	// find the spot the player is looking at
	VectorMA (r_refdef.vieworg, 4096, forward, dest);
	TraceLine (r_refdef.vieworg, dest, stop);

	// calculate pitch to look at the same spot from camera
	VectorSubtract (stop, r_refdef.vieworg, stop);

	dist = c_max(1, DotProduct(stop, forward));

	if (dist < 1)
	{
		dist = 1;	// should never happen
	}

	r_refdef.viewangles[PITCH] = -180 / M_PI * atan2( stop[2], dist );
	r_refdef.viewangles[YAW] = cl.viewangles[YAW];

	TraceLine (r_refdef.vieworg, chase_dest, stop);
	if (stop[0] != 0 || stop[1] != 0 || stop[2] != 0)
	{
		VectorCopy (stop, chase_dest);//update the camera destination to where we hit the wall

		//R00k, this prevents the camera from poking into the wall by rounding off the traceline...
		LerpVector (r_refdef.vieworg, chase_dest, 0.8f, chase_dest);
	}
	// move towards destination
	VectorCopy (chase_dest, r_refdef.vieworg);

}

void Chase_Mode (void)
{
	int		i;
	vec3_t	forward, up, right;
	vec3_t	ideal, crosshair, temp;

	AngleVectors (cl.viewangles, forward, right, up);

	if (chase_mode && !chase_type->step_to_position)
	{
		ideal[0] = cl.viewent_gun.origin[0] + chase_type->back;
		ideal[1] = cl.viewent_gun.origin[1] + chase_type->right;
		ideal[2] = cl.viewent_gun.origin[2] + chase_type->up;
	}
	else if (chase_mode && chase_type->step_to_position)
	{
		for (i=0 ; i<3 ; i++)
			ideal[i] = cl.viewent_gun.origin[i] - forward[i]*chase_type->back + chase_type->right;  //+ up[i]*chase_up.value;
		ideal[2] = cl.viewent_gun.origin[2] + chase_type->up;
	}
	else
	{
		// calc ideal camera location before checking for walls
		for (i=0 ; i<3 ; i++)
		ideal[i] = cl.viewent_gun.origin[i]
		- forward[i]*chase_back.value
		+ right[i]*chase_right.value;
		//+ up[i]*chase_up.value;
		ideal[2] = cl.viewent_gun.origin[2] + chase_up.value;
	}

	if (chase_mode && !chase_type->clip_against_objects)
	{
		// camera doesn't clip.  It is where it is.
	}
	else
	{
		// make sure camera is not in or behind a wall
		TraceLine(r_refdef.vieworg, ideal, temp);
		if (VectorLength(temp) != 0)
			VectorCopy(temp, ideal);
	}
	// place camera
	VectorCopy (ideal, r_refdef.vieworg);

	if (chase_mode && !chase_type->locate_target_lookat)
	{
		// Don't need to find where player is looking
	}
	else
	{
		// find the spot the player is looking at
		VectorMA (cl.viewent_gun.origin, 4096, forward, temp);
		TraceLine (cl.viewent_gun.origin, temp, crosshair);
	}


	if (chase_mode && !chase_type->locate_target_lookat)
	{
		// Don't need to calculate the angles
		r_refdef.viewangles[PITCH] = chase_type->pitch;
		if (chase_type->accepts_yaw)
			r_refdef.viewangles[YAW] = cl.viewangles[YAW];
		else
		{
			r_refdef.viewangles[YAW] = chase_type->yaw;
		}
		r_refdef.viewangles[ROLL] = chase_type->roll;
	}
	else
	{
		// calculate camera angles to look at the same spot
		VectorSubtract (crosshair, r_refdef.vieworg, temp);
		VectorAngles (temp, r_refdef.viewangles);
		if (r_refdef.viewangles[PITCH] == 90 || r_refdef.viewangles[PITCH] == -90)
			r_refdef.viewangles[YAW] = cl.viewangles[YAW];
	}
}

// Baker: This function is only called if chase_active 1
void Chase_UpdateForDrawing (void)
{
	if (chase_mode)
		Chase_Mode ();
	else Chase_Active_1 ();

}