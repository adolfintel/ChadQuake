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
// sv_phys.c

#include "quakedef.h"

/*


pushmove objects do not obey gravity, and do not interact with each other or trigger fields, but block normal movement and push normal objects when they move.

onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects

doors, plats, etc are SOLID_BSP, and MOVETYPE_PUSH
bonus items are SOLID_TRIGGER touch, and MOVETYPE_TOSS
corpses are SOLID_NOT and MOVETYPE_TOSS
crates are SOLID_BBOX and MOVETYPE_TOSS
walking monsters are SOLID_SLIDEBOX and MOVETYPE_STEP
flying/floating monsters are SOLID_SLIDEBOX and MOVETYPE_FLY

solid_edge items only clip against bsp models.

*/


#define	MOVE_EPSILON	0.01

void SV_Physics_Toss (edict_t *ent, double frametime);
void SV_Physics_Follow (edict_t *ent, double frametime);

/*
================
SV_CheckAllEnts
================
*/
void SV_CheckAllEnts (void)
{
	int			e;
	edict_t		*check;

// see if any solid entities are inside the final position
	check = NEXT_EDICT (sv.edicts);
	for (e = 1 ; e < sv.num_edicts ; e++, check = NEXT_EDICT(check))
	{
		if (check->free)
			continue;

		switch ((int)check->v.movetype)
		{
		case MOVETYPE_PUSH:
		case MOVETYPE_NONE:
	    case MOVETYPE_FOLLOW: // Nehahra
	    case MOVETYPE_NOCLIP:
	         continue;
		default:
			break;
		}

		if (SV_TestEntityPosition (check))
			Con_PrintLinef ("entity in invalid position");
	}
}


/*
================
SV_CheckVelocity
================
*/
void SV_CheckVelocity (edict_t *ent)
{
	int		i;

// bound velocity
	for (i = 0 ; i < 3 ; i++)
	{
		if (IS_NAN(ent->v.velocity[i]))
		{
#ifdef SUPPORTS_NEHAHRA
			if (!nehahra_active)
#endif // SUPPORTS_NEHAHRA
				Con_PrintLinef ("Got a NaN velocity on %s", PR_GetString( ent->v.classname) );
			ent->v.velocity[i] = 0;
		}

		if (IS_NAN(ent->v.origin[i]))
		{
#ifdef SUPPORTS_NEHAHRA
			if (!nehahra_active)
#endif // SUPPORTS_NEHAHRA
				Con_PrintLinef ("Got a NaN origin on %s", PR_GetString( ent->v.classname) );
			ent->v.origin[i] = 0;
		}

		// original velocity bounding
		if (ent->v.velocity[i] > sv_maxvelocity.value) ent->v.velocity[i] = sv_maxvelocity.value;
		if (ent->v.velocity[i] < -sv_maxvelocity.value) ent->v.velocity[i] = -sv_maxvelocity.value;
	} // End of for

	#pragma message ("Baker: MH has a vector length change here to do calc right, he defaults oldvelocity")
}

/*
=============
SV_RunThink

Runs thinking code if time.  There is some play in the exact time the think
function will be called, because it is called before any movement is done
in a frame.  Not used for pushmove objects, because they must be exact.
Returns false if the entity removed itself.
=============
*/
cbool SV_RunThink (edict_t *ent, double frametime)
{
	float 	thinktime = ent->v.nextthink;
	float	oldframe; //johnfitz
	int 	oldcount;

	if (thinktime <= 0 || thinktime > sv.time + frametime)
		return true;

	// don't let things stay in the past. it is possible to start that 
	// way by a trigger with a local time.
	if (thinktime < sv.time) thinktime = sv.time;	

	// cache the frame from before the think function was run
	oldframe = ent->v.frame; //johnfitz

	ent->v.nextthink = 0;
	pr_global_struct->time = thinktime;
	pr_global_struct->self = EDICT_TO_PROG(ent);
	pr_global_struct->other = EDICT_TO_PROG(sv.edicts);

#pragma message ("Baker: Make sure fix fish still works after we finish the time thing")
// JDH: the extra 1 added to total_monsters by monster_fish happens
//      in swimmonster_start_go, during the first frame (sv.time = 1).
//      But fix it only if total_monsters was increased when fish were
//      spawned (ie. if sv.fish_counted is true)

	if ((sv.time == 1.0) && vm_fishfix.value && sv.fish_counted &&
		!strcmp(PR_GetString(ent->v.classname), "monster_fish") &&
		!strcmp(PR_GetString(pr_functions[ent->v.think].s_name), "swimmonster_start_go"))
	{
		oldcount = pr_global_struct->total_monsters;
	}
	else oldcount = -1;

	PR_ExecuteProgram (ent->v.think);

	if (oldcount != -1)
	{
		if ((int)pr_global_struct->total_monsters - oldcount == 1)
		{
			pr_global_struct->total_monsters -= 1;
			if (sv.fish_counted == 1)
			{
				Con_WarningLinef ("Override progs.dat fish-count bug");
				sv.fish_counted++;
			}
		}
	}


//johnfitz -- PROTOCOL_FITZQUAKE
//capture interval to nextthink here and send it to client for better
//lerp timing, but only if interval is not 0.1 (which client assumes)
	

	// default is not to do it
	ent->sendinterval = false;

	// check if we need to send the interval
	if (!ent->free && ent->v.nextthink && (ent->v.movetype == MOVETYPE_STEP || ent->v.frame != oldframe))
	{
		int i = c_rint((ent->v.nextthink-thinktime)*255);

		//25 and 26 are close enough to 0.1 to not send
		if (i >= 0 && i < 256 && i != 25 && i != 26) 
			ent->sendinterval = true;
	}

//johnfitz

	return !ent->free;
}

/*
==================
SV_Impact

Two entities have touched, so run their touch functions
==================
*/
void SV_Impact (edict_t *e1, edict_t *e2)
{
	int		old_self, old_other;

	old_self = pr_global_struct->self;
	old_other = pr_global_struct->other;

	pr_global_struct->time = sv.time;

	if (e1->v.touch && e1->v.solid != SOLID_NOT)
	{
		pr_global_struct->self = EDICT_TO_PROG(e1);
		pr_global_struct->other = EDICT_TO_PROG(e2);
		PR_ExecuteProgram (e1->v.touch);
	}

	if (e2->v.touch && e2->v.solid != SOLID_NOT)
	{
		pr_global_struct->self = EDICT_TO_PROG(e2);
		pr_global_struct->other = EDICT_TO_PROG(e1);
		PR_ExecuteProgram (e2->v.touch);
	}

	pr_global_struct->self = old_self;
	pr_global_struct->other = old_other;
}


/*
==================
ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
#define	STOP_EPSILON	0.1

int ClipVelocity (vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float	backoff;
	float	change;
	int		i, blocked;

	blocked = 0;

	if (normal[2] > 0)
		blocked |= 1;		// floor

	if (!normal[2])
		blocked |= 2;		// step

	backoff = DotProduct (in, normal) * overbounce;

	for (i=0 ; i<3 ; i++)
	{
		change = normal[i]*backoff;
		out[i] = in[i] - change;

		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}

	return blocked;
}


/*
============
SV_FlyMove

The basic solid body movement clip that slides along multiple planes
Returns the clipflags if the velocity was modified (hit something solid)
1 = floor
2 = wall / step
4 = dead stop
If steptrace is not NULL, the trace of any vertical wall hit will be stored
============
*/
#pragma message ("Baker: MH has MAX_CLIP_PLANES 20, DarkPlaces doesn't.  FTE no sure.  What is that for?")
#define	MAX_CLIP_PLANES	5
int SV_FlyMove (edict_t *ent, float time, trace_t *steptrace)
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity, original_velocity, new_velocity;
	int			i, j;
	trace_t		trace;
	vec3_t		end;
	float		time_left;
	int			blocked;

	numbumps = 4;

	blocked = 0;
	VectorCopy (ent->v.velocity, original_velocity);
	VectorCopy (ent->v.velocity, primal_velocity);
	numplanes = 0;

	time_left = time;

	for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++)
	{
		if (!ent->v.velocity[0] && !ent->v.velocity[1] && !ent->v.velocity[2])
			break;

		for (i=0 ; i<3 ; i++)
			end[i] = ent->v.origin[i] + time_left * ent->v.velocity[i];

		trace = SV_Move (ent->v.origin, ent->v.mins, ent->v.maxs, end, false, ent);

		if (trace.allsolid)
		{
			// entity is trapped in another solid
			VectorCopy (vec3_origin, ent->v.velocity);
			return 3;
		}

		if (trace.fraction > 0)
		{
			// actually covered some distance
			VectorCopy (trace.endpos, ent->v.origin);
			VectorCopy (ent->v.velocity, original_velocity);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			 break;		// moved the entire distance

		if (!trace.ent)
			Host_Error ("SV_FlyMove: !trace.ent");

		if (trace.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor

			if (trace.ent->v.solid == SOLID_BSP)
			{
				ent->v.flags =	(int)ent->v.flags | FL_ONGROUND;
				ent->v.groundentity = EDICT_TO_PROG(trace.ent);
			}
		}

		if (!trace.plane.normal[2])
		{
			blocked |= 2;		// step

			if (steptrace)
				*steptrace = trace;	// save for player extrafriction
		}

// run the impact function
		SV_Impact (ent, trace.ent);

		if (ent->free)
			break;		// removed by the impact function

		time_left -= time_left * trace.fraction;

	// clipped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{
			// this shouldn't really happen
			VectorCopy (vec3_origin, ent->v.velocity);
			return 3;
		}

		VectorCopy (trace.plane.normal, planes[numplanes]);
		numplanes++;

// modify original_velocity so it parallels all of the clip planes
		for (i = 0 ; i < numplanes ; i++)
		{
			ClipVelocity (original_velocity, planes[i], new_velocity, 1);

			for (j = 0 ; j < numplanes ; j++)
			{
				if (j != i)
				{
					if (DotProduct (new_velocity, planes[j]) < 0)
						break;	// not ok
				}
			}

			if (j == numplanes)
				break;
		}

		if (i != numplanes)
		{
			// go along this plane
			VectorCopy (new_velocity, ent->v.velocity);
		}
		else
		{
			// go along the crease
			if (numplanes != 2)
			{
//				Con_PrintLinef ("clip velocity, numplanes == %d",numplanes);
				VectorCopy (vec3_origin, ent->v.velocity);
				return 7;
			}

			VectorCrossProduct (planes[0], planes[1], dir);
			d = DotProduct (dir, ent->v.velocity);
			VectorScale (dir, d, ent->v.velocity);
		}

// if original velocity is against the original velocity, stop dead
// to avoid tiny occilations in sloping corners
		if (DotProduct (ent->v.velocity, primal_velocity) <= 0)
		{
			VectorCopy (vec3_origin, ent->v.velocity);
			return blocked;
		}
	}

	return blocked;
}


/*
============
SV_AddGravity

============
*/
void SV_AddGravity (edict_t *ent, double frametime)
{
	float	ent_gravity;

	eval_t	*val = GETEDICTFIELDVALUE (ent, eval_gravity);

	if (val && val->_float)
		ent_gravity = val->_float;
	else ent_gravity = 1.0;

	ent->v.velocity[2] -= (ent_gravity * sv_gravity.value * frametime);
}


/*
===============================================================================

PUSHMOVE

===============================================================================
*/

/*
============
SV_PushEntity

Does not change the entities velocity at all
============
*/
trace_t SV_PushEntity (edict_t *ent, vec3_t push)
{
	trace_t	trace;
	vec3_t	end;

	VectorAdd (ent->v.origin, push, end);

	if (ent->v.movetype == MOVETYPE_FLYMISSILE)
		trace = SV_Move (ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_MISSILE, ent);
	else if (ent->v.solid == SOLID_TRIGGER || ent->v.solid == SOLID_NOT)
	{
		// only clip against bmodels
		trace = SV_Move (ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_NOMONSTERS, ent);
	}
	else trace = SV_Move (ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_NORMAL, ent);

	VectorCopy (trace.endpos, ent->v.origin);
	SV_LinkEdict (ent, true);

	if (trace.ent)
		SV_Impact (ent, trace.ent);

	return trace;
}


/*
============
SV_PushMove

============
*/
edict_t		*moved_edict[MAX_EDICTS_PROTOCOL_666];
vec3_t		moved_from[MAX_EDICTS_PROTOCOL_666];

void SV_PushMove (edict_t *pusher, float movetime)
{
	int			i, e;
	edict_t		*check, *block;
	vec3_t		mins, maxs, move;
	vec3_t		entorig, pushorig;
	int			num_moved;
	float		oldsolid;

	if (!pusher->v.velocity[0] && !pusher->v.velocity[1] && !pusher->v.velocity[2])
	{
		pusher->v.ltime += movetime;
		return;
	}

	for (i=0 ; i<3 ; i++)
	{
		move[i] = pusher->v.velocity[i] * movetime;
		mins[i] = pusher->v.absmin[i] + move[i];
		maxs[i] = pusher->v.absmax[i] + move[i];
	}

	VectorCopy (pusher->v.origin, pushorig);

// move the pusher to it's final position
	VectorAdd (pusher->v.origin, move, pusher->v.origin);
	pusher->v.ltime += movetime;
	SV_LinkEdict (pusher, false);

// see if any solid entities are inside the final position
	num_moved = 0;
	check = NEXT_EDICT(sv.edicts);

	for (e = 1 ; e < sv.num_edicts ; e++, check = NEXT_EDICT(check))
	{
		if (check->free) continue;

		switch ((int)check->v.movetype)
		{
		case MOVETYPE_PUSH:
		case MOVETYPE_NONE:
	    case MOVETYPE_FOLLOW:
	    case MOVETYPE_NOCLIP:
	         continue;
		default:
			break;
		}


	// if the entity is standing on the pusher, it will definitely be moved
		if ( ! ( ((int)check->v.flags & FL_ONGROUND)
		&& PROG_TO_EDICT(check->v.groundentity) == pusher) )
		{
			if ( check->v.absmin[0] >= maxs[0]
			|| check->v.absmin[1] >= maxs[1]
			|| check->v.absmin[2] >= maxs[2]
			|| check->v.absmax[0] <= mins[0]
			|| check->v.absmax[1] <= mins[1]
			|| check->v.absmax[2] <= mins[2] )
				continue;

		// see if the ent's bbox is inside the pusher's final position
			if (!SV_TestEntityPosition (check)) continue;
		}

	// remove the onground flag for non-players
		if (check->v.movetype != MOVETYPE_WALK)
			check->v.flags = (int)check->v.flags & ~FL_ONGROUND;

		VectorCopy (check->v.origin, entorig);
		VectorCopy (check->v.origin, moved_from[num_moved]);
		moved_edict[num_moved] = check;
		num_moved++;

#pragma message ("Baker: DirectQ has some different physics, does DirectQ work with the rotator map when standing on the top?")
		// try moving the contacted entity
		oldsolid = pusher->v.solid;
		pusher->v.solid = SOLID_NOT;

		SV_PushEntity (check, move);
#if 0
		pusher->v.solid = SOLID_BSP;
#else
		pusher->v.solid = oldsolid;
#endif 
#pragma message ("Baker: Techically this is a behavior change")

	// if it is still inside the pusher, block
		block = SV_TestEntityPosition (check);

		if (block)
		{
			// fail the move
			if (check->v.mins[0] == check->v.maxs[0]) continue;

			if (check->v.solid == SOLID_NOT || check->v.solid == SOLID_TRIGGER)
			{
				// corpse
				check->v.mins[0] = check->v.mins[1] = 0;
				VectorCopy (check->v.mins, check->v.maxs);
				continue;
			}

			VectorCopy (entorig, check->v.origin);
			SV_LinkEdict (check, true);

			VectorCopy (pushorig, pusher->v.origin);
			SV_LinkEdict (pusher, false);
			pusher->v.ltime -= movetime;

			// if the pusher has a "blocked" function, call it
			// otherwise, just stay in place until the obstacle is gone
			if (pusher->v.blocked)
			{
				pr_global_struct->self = EDICT_TO_PROG(pusher);
				pr_global_struct->other = EDICT_TO_PROG(check);
				PR_ExecuteProgram (pusher->v.blocked);
			}

		// move back any entities we already moved
			for (i=0 ; i<num_moved ; i++)
			{
				VectorCopy (moved_from[i], moved_edict[i]->v.origin);
				SV_LinkEdict (moved_edict[i], false);
			}

			return;
		}
	}
}


/*
============
SV_PushRotate

============
*/
void SV_PushRotate (edict_t *pusher, float movetime)
{
	int         i, e;
	edict_t      *check, *block;
	vec3_t      move, a, amove;
	vec3_t      entorig, pushorig;
	int         num_moved;
	vec3_t      org, org2;
	vec3_t      forward, right, up;
	
	if (!pusher->v.avelocity[0] && !pusher->v.avelocity[1] && !pusher->v.avelocity[2])
	{
		pusher->v.ltime += movetime;
		return;
	}
	
	for (i=0 ; i<3 ; i++)
		amove[i] = pusher->v.avelocity[i] * movetime;
	
	VectorSubtract (vec3_origin, amove, a);
	AngleVectors (a, forward, right, up);
	
	VectorCopy (pusher->v.angles, pushorig);
	
	// move the pusher to it's final position
	VectorAdd (pusher->v.angles, amove, pusher->v.angles);
	pusher->v.ltime += movetime;
	SV_LinkEdict (pusher, false);
	
	// see if any solid entities are inside the final position
	num_moved = 0;
	check = NEXT_EDICT(sv.edicts);
	
	for (e = 1 ; e < sv.num_edicts ; e++, check = NEXT_EDICT(check))
	{
		float oldsolid;

		if (check->free)
		 continue;
		
		switch ((int)check->v.movetype)		
		{
		case MOVETYPE_PUSH:
		case MOVETYPE_NONE:
		case MOVETYPE_FOLLOW:
		case MOVETYPE_NOCLIP:
		     continue;
		default:
			break;
		}
		
		// if the entity is standing on the pusher, it will definitely be moved
		if ( ! ( ((int)check->v.flags & FL_ONGROUND)
		&& PROG_TO_EDICT(check->v.groundentity) == pusher) )
		{
			if ( check->v.absmin[0] >= pusher->v.absmax[0]
				|| check->v.absmin[1] >= pusher->v.absmax[1]
				|| check->v.absmin[2] >= pusher->v.absmax[2]
				|| check->v.absmax[0] <= pusher->v.absmin[0]
				|| check->v.absmax[1] <= pusher->v.absmin[1]
				|| check->v.absmax[2] <= pusher->v.absmin[2] )
				continue;
			
			// see if the ent's bbox is inside the pusher's final position
			if (!SV_TestEntityPosition (check))
				continue;
		}
		
		// remove the onground flag for non-players
		if (check->v.movetype != MOVETYPE_WALK)
		 check->v.flags = (int)check->v.flags & ~FL_ONGROUND;
		
		VectorCopy (check->v.origin, entorig);
		VectorCopy (check->v.origin, moved_from[num_moved]);
		moved_edict[num_moved] = check;
		num_moved++;
		
		// calculate destination position
		VectorSubtract (check->v.origin, pusher->v.origin, org);
		org2[0] = DotProduct (org, forward);
		org2[1] = -DotProduct (org, right);
		org2[2] = DotProduct (org, up);
		VectorSubtract (org2, org, move);
		
		// try moving the contacted entity
		oldsolid = pusher->v.solid;
		pusher->v.solid = SOLID_NOT;
		SV_PushEntity (check, move);
		pusher->v.solid = oldsolid;
		
		// if it is still inside the pusher, block
		block = SV_TestEntityPosition (check);
		
		if (block)
		{
			// fail the move
			if (check->v.mins[0] == check->v.maxs[0])
				continue;
			
			if (check->v.solid == SOLID_NOT || check->v.solid == SOLID_TRIGGER)
			{
				// corpse
				check->v.mins[0] = check->v.mins[1] = 0;
				VectorCopy (check->v.mins, check->v.maxs);
				continue;
			}
			
			VectorCopy (entorig, check->v.origin);
			SV_LinkEdict (check, true);
			
			VectorCopy (pushorig, pusher->v.origin /*pusher->v.angles*/); // Formerly 
			SV_LinkEdict (pusher, false);
			pusher->v.ltime -= movetime;
		
			// if the pusher has a "blocked" function, call it
			// otherwise, just stay in place until the obstacle is gone
			if (pusher->v.blocked)
			{
				pr_global_struct->self = EDICT_TO_PROG(pusher);
				pr_global_struct->other = EDICT_TO_PROG(check);
				PR_ExecuteProgram (pusher->v.blocked);
			}
			
			// move back any entities we already moved
			for (i = 0 ; i < num_moved ; i++)
			{
				VectorCopy (moved_from[i], moved_edict[i]->v.origin);
				VectorSubtract (moved_edict[i]->v.angles, amove, moved_edict[i]->v.angles);
				SV_LinkEdict (moved_edict[i], false);
			}
			
			return;
		}
		else // Not blocked ...
		{
			VectorAdd (check->v.angles, amove, check->v.angles);
		}
	} // End of for
}


/*
================
SV_Physics_Pusher

================
*/
void SV_Physics_Pusher (edict_t *ent, double frametime)
{
	float	thinktime;
	float	oldltime;
	float	movetime;

	oldltime = ent->v.ltime;
	thinktime = ent->v.nextthink;

	if (thinktime < ent->v.ltime + frametime)
	{
		movetime = thinktime - ent->v.ltime;

		if (movetime < 0)
			movetime = 0;
	}
	else movetime = frametime;

	// MH: advances ent->v.ltime if not blocked
	// MH: this gives bad results with the end.bsp spiked ball
#pragma message ("Baker: MH says this gives bad results for end.bsp spiked ball")
	if (movetime)
	{
//ROTATE START
		if (SUPPORTS_ROTATION(sv.protocol) && (ent->v.avelocity[0] || ent->v.avelocity[1] || ent->v.avelocity[2]) && ent->v.solid == SOLID_BSP)
		{
			#pragma message ("Baker: Why are we having trouble standing on top of a rotating entity?  Does only Havoc's compiler work for that?")
			SV_PushRotate (ent, frametime);
		}
		else
//ROTATE END
		SV_PushMove (ent, movetime);	// advances ent->v.ltime if not blocked
	}

	if (thinktime > oldltime && thinktime <= ent->v.ltime)
	{
		ent->v.nextthink = 0;
		pr_global_struct->time = sv.time;
		pr_global_struct->self = EDICT_TO_PROG(ent);
		pr_global_struct->other = EDICT_TO_PROG(sv.edicts);
		PR_ExecuteProgram (ent->v.think);

		if (ent->free) return;
	}
}


/*
===============================================================================

CLIENT MOVEMENT

===============================================================================
*/

/*
=============
SV_CheckStuck

This is a big hack to try and fix the rare case of getting stuck in the world
clipping hull.
=============
*/
void SV_CheckStuck (edict_t *ent)
{
	int		i, j;
	int		z;
	vec3_t	org;

	if (!SV_TestEntityPosition(ent))
	{
		VectorCopy (ent->v.origin, ent->v.oldorigin);
		return;
	}

	VectorCopy (ent->v.origin, org);
	VectorCopy (ent->v.oldorigin, ent->v.origin);

	if (!SV_TestEntityPosition(ent))
	{
		Con_DPrintLinef ("Unstuck.");
		SV_LinkEdict (ent, true);
		return;
	}

	for (z = 0 ; z < 18 ; z++)
	{
		for (i = -1 ; i <= 1 ; i++)
		{
			for (j = -1 ; j <= 1 ; j++)
			{
				ent->v.origin[0] = org[0] + i;
				ent->v.origin[1] = org[1] + j;
				ent->v.origin[2] = org[2] + z;

				if (!SV_TestEntityPosition(ent))
				{
					Con_DPrintLinef ("Unstuck.");
					SV_LinkEdict (ent, true);
					return;
				}
			}
		}
	}

	VectorCopy (org, ent->v.origin);
	Con_DPrintLinef ("player is stuck.");
}


/*
=============
SV_CheckWater
=============
*/
cbool SV_CheckWater (edict_t *ent)
{
	vec3_t	point;
	int		cont;

	point[0] = ent->v.origin[0];
	point[1] = ent->v.origin[1];
	point[2] = ent->v.origin[2] + ent->v.mins[2] + 1;

	ent->v.waterlevel = 0;
	ent->v.watertype = CONTENTS_EMPTY;
	cont = SV_PointContents (point);

	if (cont <= CONTENTS_WATER)
	{
		ent->v.watertype = cont;
		ent->v.waterlevel = 1;
		point[2] = ent->v.origin[2] + (ent->v.mins[2] + ent->v.maxs[2])*0.5;
		cont = SV_PointContents (point);

		if (cont <= CONTENTS_WATER)
		{
			ent->v.waterlevel = 2;
			point[2] = ent->v.origin[2] + ent->v.view_ofs[2];
			cont = SV_PointContents (point);

			if (cont <= CONTENTS_WATER)
				ent->v.waterlevel = 3;
		}
	}

	return ent->v.waterlevel > 1;
}

/*
============
SV_WallFriction

============
*/
void SV_WallFriction (edict_t *ent, trace_t *trace)
{
	vec3_t		forward, right, up;
	float		d, i;
	vec3_t		into, side;

	AngleVectors (ent->v.v_angle, forward, right, up);
	d = DotProduct (trace->plane.normal, forward);

	d += 0.5;

	if (d >= 0)
		return;

// cut the tangential velocity
	i = DotProduct (trace->plane.normal, ent->v.velocity);
	VectorScale (trace->plane.normal, i, into);
	VectorSubtract (ent->v.velocity, into, side);

	ent->v.velocity[0] = side[0] * (1 + d);
	ent->v.velocity[1] = side[1] * (1 + d);
}

/*
=====================
SV_TryUnstick

Player has come to a dead stop, possibly due to the problem with limited
float precision at some angle joins in the BSP hull.

Try fixing by pushing one pixel in each direction.

This is a hack, but in the interest of good gameplay...
======================
*/
int SV_TryUnstick (edict_t *ent, vec3_t oldvel)
{
	int		i;
	vec3_t	oldorg;
	vec3_t	dir;
	int		clip;
	trace_t	steptrace;

	VectorCopy (ent->v.origin, oldorg);
	VectorCopy (vec3_origin, dir);

	for (i=0 ; i<8 ; i++)
	{
// try pushing a little in an axial direction
		switch (i)
		{
			case 0:	dir[0] = 2; dir[1] = 0; break;
			case 1:	dir[0] = 0; dir[1] = 2; break;
			case 2:	dir[0] = -2; dir[1] = 0; break;
			case 3:	dir[0] = 0; dir[1] = -2; break;
			case 4:	dir[0] = 2; dir[1] = 2; break;
			case 5:	dir[0] = -2; dir[1] = 2; break;
			case 6:	dir[0] = 2; dir[1] = -2; break;
			case 7:	dir[0] = -2; dir[1] = -2; break;
		}

		SV_PushEntity (ent, dir);

// retry the original move
		ent->v.velocity[0] = oldvel[0];
		ent->v. velocity[1] = oldvel[1];
		ent->v. velocity[2] = 0;
		clip = SV_FlyMove (ent, 0.1, &steptrace);

		if ( fabs(oldorg[1] - ent->v.origin[1]) > 4 || fabs(oldorg[0] - ent->v.origin[0]) > 4 )
		{
//Con_DPrintLinef ("unstuck!");
			return clip;
		}

// go back to the original pos and try again
		VectorCopy (oldorg, ent->v.origin);
	}

	VectorCopy (vec3_origin, ent->v.velocity);
	return 7;		// still not moving
}

/*
=====================
SV_WalkMove

Only used by players
======================
*/
#define	STEPSIZE	18
void SV_WalkMove (edict_t *ent, double frametime)
{
	vec3_t		upmove, downmove;
	vec3_t		oldorg, oldvel;
	vec3_t		nosteporg, nostepvel;
	int			clip;
	int			oldonground;
	trace_t		steptrace, downtrace;

// do a regular slide move unless it looks like you ran into a step
	oldonground = (int)ent->v.flags & FL_ONGROUND;
	ent->v.flags = (int)ent->v.flags & ~FL_ONGROUND;

	VectorCopy (ent->v.origin, oldorg);
	VectorCopy (ent->v.velocity, oldvel);

	clip = SV_FlyMove (ent, frametime, &steptrace);

	if ( !(clip & 2) ) return; // move didn't block on a step
	if (!oldonground && ent->v.waterlevel == 0) return;		// don't stair up while jumping
	if (ent->v.movetype != MOVETYPE_WALK) return; // gibbed by a trigger
	if (sv_nostep.value) return;
	if ( (int)sv_player->v.flags & FL_WATERJUMP ) return;

	VectorCopy (ent->v.origin, nosteporg);
	VectorCopy (ent->v.velocity, nostepvel);

// try moving up and forward to go up a step
	VectorCopy (oldorg, ent->v.origin);	// back to start pos

	VectorCopy (vec3_origin, upmove);
	VectorCopy (vec3_origin, downmove);
	upmove[2] = STEPSIZE;
	downmove[2] = -STEPSIZE + oldvel[2] * frametime;

// move up
	SV_PushEntity (ent, upmove);	// FIXME: don't link?

// move forward
	ent->v.velocity[0] = oldvel[0];
	ent->v. velocity[1] = oldvel[1];
	ent->v. velocity[2] = 0;
	clip = SV_FlyMove (ent, frametime, &steptrace);

// check for stuckness, possibly due to the limited precision of floats
// in the clipping hulls
	if (clip)
	{
		if ( fabs(oldorg[1] - ent->v.origin[1]) < 0.03125 && fabs(oldorg[0] - ent->v.origin[0]) < 0.03125 )
		{
			// stepping up didn't make any progress
			clip = SV_TryUnstick (ent, oldvel);
		}
	}

// extra friction based on view angle
	if ( clip & 2 ) SV_WallFriction (ent, &steptrace);

// move down
	downtrace = SV_PushEntity (ent, downmove);	// FIXME: don't link?

	if (downtrace.plane.normal[2] > 0.7)
	{
		if (ent->v.solid == SOLID_BSP)
		{
			ent->v.flags =	(int)ent->v.flags | FL_ONGROUND;
			ent->v.groundentity = EDICT_TO_PROG(downtrace.ent);
		}
	}
	else
	{
// if the push down didn't end up on good ground, use the move without
// the step up.  This happens near wall / slope combinations, and can
// cause the player to hop up higher on a slope too steep to climb
		VectorCopy (nosteporg, ent->v.origin);
		VectorCopy (nostepvel, ent->v.velocity);
	}
}


/*
============
SV_CycleWeaponReverse

(JDH: copy of weapons.qc function)
============
*/
void SV_CycleWeaponReverse (edict_t *ent)
{
	int			it, weapon;
	cbool	has_ammo;
	dfunction_t	*func;

	it = (int) ent->v.items;
	weapon = (int) ent->v.weapon;
	ent->v.impulse = 0;

//	Con_PrintLinef ("SV_CycleWeaponReverse fired");

	while (1)
	{
		has_ammo = true;

		switch (weapon)
		{
		case IT_LIGHTNING:
			weapon = IT_ROCKET_LAUNCHER;
			if (ent->v.ammo_rockets < 1)
				has_ammo = false;
			break;
		case IT_ROCKET_LAUNCHER:
			weapon = IT_GRENADE_LAUNCHER;
			if (ent->v.ammo_rockets < 1)
				has_ammo = false;
			break;
		case IT_GRENADE_LAUNCHER:
			weapon = IT_SUPER_NAILGUN;
			if (ent->v.ammo_nails < 2)
				has_ammo = false;
			break;
		case IT_SUPER_NAILGUN:
			weapon = IT_NAILGUN;
			if (ent->v.ammo_nails < 1)
				has_ammo = false;
			break;
		case IT_NAILGUN:
			weapon = IT_SUPER_SHOTGUN;
			if (ent->v.ammo_shells < 2)
				has_ammo = false;
			break;
		case IT_SUPER_SHOTGUN:
			weapon = IT_SHOTGUN;
			if (ent->v.ammo_shells < 1)
				has_ammo = false;
			break;
		case IT_SHOTGUN:
			weapon = IT_AXE;
			break;
		case IT_AXE:
			weapon = IT_LIGHTNING;
			if (ent->v.ammo_cells < 1)
				has_ammo = false;
			break;
		}

		if ((it & weapon) && has_ammo)
		{
			func = PR_FindFunction ("W_SetCurrentAmmo", PRFF_NOBUILTINS | PRFF_NOPARTIALS);
			if (func)
			{
			// W_SetCurrentAmmo usually has no params, but for lthsp2-lthsp5
			// it expects "self" as an argument
				if (func->numparms == 1)
					((int *)pr_globals)[OFS_PARM0] = pr_global_struct->self; //PR_GLOBAL(self);
				ent->v.weapon = weapon;
				PR_ExecuteProgram (func - pr_functions);
			}
			return;
		}
	}
}


/*
================
SV_Physics_Client

Player character actions
================
*/
// Exclusively called by SV_Physics -- SV_Physics is called by SV_SpawnServer and "SV_Host_Frame_UpdateServer"
static void SV_Physics_Client (edict_t *ent, int num, double frametime)
{
	if ( ! svs.clients[num - 1].active ) {
		// First couple of frames, the player isn't active.   (SV_SpawnServer first 2 frames.)
		// This also happens for any empty player slot.
		return;		// unconnected slot
	}

// call standard client pre-think

	pr_global_struct->time = sv.time;
	pr_global_struct->self = EDICT_TO_PROG(ent);

	PR_ExecuteProgram (pr_global_struct->PlayerPreThink);

// do a move
	SV_CheckVelocity (ent);

// decide which move function to call
	switch ((int)ent->v.movetype)
	{
	case MOVETYPE_NONE:
		if (!SV_RunThink (ent, frametime))
			return;
		break;

// Baker: MH doesn't have this because I guess physics follow
// should be impossible for a player?
	case MOVETYPE_FOLLOW:
		SV_Physics_Follow (ent, frametime);
		break;

	case MOVETYPE_WALK:
		if (!SV_RunThink (ent, frametime))
			return;

		if (!SV_CheckWater (ent) && ! ((int)ent->v.flags & FL_WATERJUMP) )
			SV_AddGravity (ent, frametime);

		SV_CheckStuck (ent);
		SV_WalkMove (ent, frametime);

		break;

	case MOVETYPE_TOSS:
	case MOVETYPE_BOUNCE:
		SV_Physics_Toss (ent, frametime);
		break;

	case MOVETYPE_FLY:
		if (!SV_RunThink (ent, frametime))
			return;

		SV_FlyMove (ent, frametime, NULL);
		break;

	case MOVETYPE_NOCLIP:
		if (!SV_RunThink (ent, frametime))
			return;

		VectorMA (ent->v.origin, frametime, ent->v.velocity, ent->v.origin);
		break;

	default:
		Host_Error ("SV_Physics_client: bad movetype %d", (int)ent->v.movetype);
	}

// call standard player post-think
	// Baker: MH's noclip doesn't fire triggers enhancement start
	if (ent->v.movetype == MOVETYPE_NOCLIP)
	   SV_LinkEdict (ent, false);

	else SV_LinkEdict (ent, true);

	pr_global_struct->time = sv.time;
	pr_global_struct->self = EDICT_TO_PROG(ent);

	// JDH: another hack, this time for progs that lack CycleWeaponReverse
	if (ent->v.impulse == 12.0)
	{
		// Baker: Server received impulse 12 ... checking conditions
		if (sv.pr_imp12_override && !ent->v.deadflag && (ent->v.view_ofs[0] || ent->v.view_ofs[1] || ent->v.view_ofs[2]) )
		{
			eval_t  *val = GETEDICTFIELDVALUE(ent, eval_attack_finished);

			if (val && (sv.time >= val->_float))
			{
				SV_CycleWeaponReverse (ent);
			}
		}
	}

	PR_ExecuteProgram (pr_global_struct->PlayerPostThink);
}

//============================================================================

/*
=============
SV_Physics_None

Non moving objects can only think
=============
*/
void SV_Physics_None (edict_t *ent, double frametime)
{
// regular thinking
	SV_RunThink (ent, frametime);
}


// Nehahra
/*
=============
SV_Physics_Follow

Entities that are "stuck" to another entity
=============
*/
void SV_Physics_Follow (edict_t *ent, double frametime)
{
#pragma message ("DirectQ has this function different for rotation")
// regular thinking
	SV_RunThink (ent, frametime);
	VectorAdd (PROG_TO_EDICT(ent->v.aiment)->v.origin, ent->v.v_angle, ent->v.origin);
	SV_LinkEdict (ent, true);
}


/*
=============
SV_Physics_Noclip

A moving object that doesn't obey physics
=============
*/
void SV_Physics_Noclip (edict_t *ent, double frametime)
{
// regular thinking
	if (!SV_RunThink (ent, frametime))
		return;

	VectorMA (ent->v.angles, frametime, ent->v.avelocity, ent->v.angles);
	VectorMA (ent->v.origin, frametime, ent->v.velocity, ent->v.origin);

	SV_LinkEdict (ent, false);
}

/*
==============================================================================

TOSS / BOUNCE

==============================================================================
*/

/*
=============
SV_CheckWaterTransition

=============
*/
void SV_CheckWaterTransition (edict_t *ent)
{
	int		cont;

	cont = SV_PointContents (ent->v.origin);

	if (!ent->v.watertype)
	{
		// just spawned here
		ent->v.watertype = cont;
		ent->v.waterlevel = 1;
		return;
	}

	if (cont <= CONTENTS_WATER)
	{
		if (ent->v.watertype == CONTENTS_EMPTY)
		{
			// just crossed into water
			SV_StartSound (ent, 0, "misc/h2ohit1.wav", 255, 1);
		}

		ent->v.watertype = cont;
		ent->v.waterlevel = 1;
	}
	else
	{
		if (ent->v.watertype != CONTENTS_EMPTY)
		{
			// just crossed into water
			SV_StartSound (ent, 0, "misc/h2ohit1.wav", 255, 1);
		}

		ent->v.watertype = CONTENTS_EMPTY;
		ent->v.waterlevel = cont;
	}
}

/*
=============
SV_Physics_Toss

Toss, bounce, and fly movement.  When onground, do nothing.
=============
*/
void SV_Physics_Toss (edict_t *ent, double frametime)
{
	trace_t	trace;
	vec3_t	move;
	float	backoff;

	// regular thinking
	if (!SV_RunThink (ent, frametime)) return;

// if onground, return without moving
	if ( ((int)ent->v.flags & FL_ONGROUND) ) return;

	SV_CheckVelocity (ent);

// add gravity
	if (ent->v.movetype != MOVETYPE_FLY && ent->v.movetype != MOVETYPE_FLYMISSILE)
		SV_AddGravity (ent, frametime);

// move angles
	VectorMA (ent->v.angles, frametime, ent->v.avelocity, ent->v.angles);

// move origin
	VectorScale (ent->v.velocity, frametime, move);
	trace = SV_PushEntity (ent, move);

	if (trace.fraction == 1) return;
	if (ent->free) return;

	if (ent->v.movetype == MOVETYPE_BOUNCE)
		backoff = 1.5;
	else backoff = 1;

	ClipVelocity (ent->v.velocity, trace.plane.normal, ent->v.velocity, backoff);

// stop if on ground
	if (trace.plane.normal[2] > 0.7)
	{
		int stop_moving = false;
		if (sv_bouncedownslopes.value)
		{
			if (DotProduct(trace.plane.normal, ent->v.velocity) < 60)
				stop_moving = true;
		}
		else
		{
			if (ent->v.velocity[2] < 60 || ent->v.movetype != MOVETYPE_BOUNCE)
				stop_moving = true;
		}

		if (stop_moving)
		{
			ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
			ent->v.groundentity = EDICT_TO_PROG(trace.ent);
			VectorCopy (vec3_origin, ent->v.velocity);
			VectorCopy (vec3_origin, ent->v.avelocity);
		}
	}

// check for in water
	SV_CheckWaterTransition (ent);
}

/*
===============================================================================

STEPPING MOVEMENT

===============================================================================
*/

/*
=============
SV_Physics_Step

Monsters freefall when they don't have a ground entity, otherwise
all movement is done with discrete steps.

This is also used for objects that have become still on the ground, but
will fall if the floor is pulled out from under them.
=============
*/
void SV_Physics_Step (edict_t *ent, double frametime)
{
	// freefall if not onground
	if ( ! ((int)ent->v.flags & (FL_ONGROUND | FL_FLY | FL_SWIM) ) )
	{
		cbool hitsound;

		if (ent->v.velocity[2] < sv_gravity.value*-0.1)
			hitsound = true;
		else hitsound = false;

		SV_AddGravity (ent, frametime);
		SV_CheckVelocity (ent);
		SV_FlyMove (ent, frametime, NULL);
		SV_LinkEdict (ent, true);

		if ( (int)ent->v.flags & FL_ONGROUND )	// just hit ground
		{
			if (hitsound)
				SV_StartSound (ent, 0, "demon/dland2.wav", 255, 1);
		}
	}

// regular thinking
	SV_RunThink (ent, frametime);

	SV_CheckWaterTransition (ent);
}

//============================================================================


/*
================
SV_Physics

================
*/
// SV_SpawnServer and "SV_Host_Frame_UpdateServer"
void SV_Physics (double frametime)
{
	int		i;
	int		entity_cap; // For sv_freezenonclients
	edict_t	*ent;

// let the progs know that a new frame has started
	pr_global_struct->self = EDICT_TO_PROG(sv.edicts);
	pr_global_struct->other = EDICT_TO_PROG(sv.edicts);
	pr_global_struct->time = sv.time;
	PR_ExecuteProgram (pr_global_struct->StartFrame);

//SV_CheckAllEnts ();

// treat each object in turn
	ent = sv.edicts;

	if (sv.frozen)
		entity_cap = svs.maxclients_internal + 1; // Only run physics on clients and the world.  The cap is right.
	else entity_cap = sv.num_edicts;

	//for (i=0 ; i<sv.num_edicts ; i++, ent = NEXT_EDICT(ent))
	for (i = 0 ; i < entity_cap ; i++, ent = NEXT_EDICT(ent))
	{
		if (ent->free)
			continue;

		if (in_range (1, i, svs.maxclients_internal)) {  // Because the cap can change at any time now.
			client_t *svr_player = &svs.clients[i - 1];
			// Check for end of coop protection
			if (svr_player->coop_protect_end_time) {
				if (svr_player->coop_protect_end_time < sv.time) {
					// END THE COOP PROTECTION
					svr_player->coop_protect_end_time = 0; // Protection is gone
					//Con_PrintLinef ("Spawn protection is gone for " QUOTED_S " at %g", svr_player->name, sv.time);
				}
			}
			// Set the global
			sv.coop_physics_clientnum = svr_player->coop_protect_end_time ? i : 0;
		}

		if (pr_global_struct->force_retouch)
		{
			SV_LinkEdict (ent, true);	// force retouch even for stationary
		}
		
		// Baker:  Client physics
		if (in_range (1, i, svs.maxclients_internal)) { // Because the cap can change at any time now.
			SV_Physics_Client (ent, i, frametime);
			// Unset the global
			sv.coop_physics_clientnum = 0;
			continue;
		}	

		// Baker:  Not a player ...
		switch ((int)ent->v.movetype)
		{
		case MOVETYPE_PUSH:  
			SV_Physics_Pusher (ent, frametime); 
			break;
		case MOVETYPE_NONE:  
			SV_Physics_None (ent, frametime); 
			break;
		case MOVETYPE_FOLLOW:  // Nehahra
			SV_Physics_Follow (ent, frametime); 
			break; 
		case MOVETYPE_NOCLIP: 
			SV_Physics_Noclip (ent, frametime); 
			break;
		case MOVETYPE_STEP:  
			SV_Physics_Step (ent, frametime); 
			break;
		case MOVETYPE_TOSS:
		case MOVETYPE_BOUNCE:
		case MOVETYPE_FLY:
		case MOVETYPE_FLYMISSILE:
			SV_Physics_Toss (ent, frametime);
			break;
		default:
			Host_Error ("SV_Physics: bad movetype %d", (int)ent->v.movetype);
		}
	}

	if (pr_global_struct->force_retouch)
		pr_global_struct->force_retouch--;

	// accumulate the time as a double
	if (!sv.frozen)
		sv.time += frametime;
}


trace_t SV_Trace_Toss (edict_t *ent, edict_t *ignore)
{
	edict_t	tempent, *tent;
	trace_t	trace;
	vec3_t	move;
	vec3_t	end;
	int		i;

	// MH: needs to simulate different FPS because it comes from progs
	// MH: (note - this is probably incorrect - should it be 0.1???)
	double frametime = 0.05;

	memcpy (&tempent, ent, sizeof(edict_t));
	tent = &tempent;

#pragma message ("Is there a way to solve Trinca Forgotten Tomb physics problem?  Ticrate indepedent physics???  See DarkPlaces??")

#if 0 // Baker:  Original is a while(1) loop, using LH fix
	while (1)
#else
	for (i = 0; i < 200; i++) // LordHavoc: sanity check; never trace more than 10 seconds
#endif
	{
		SV_CheckVelocity (tent);
		SV_AddGravity (tent, frametime);
		VectorMA (tent->v.angles, frametime, tent->v.avelocity, tent->v.angles);
		VectorScale (tent->v.velocity, frametime, move);
		VectorAdd (tent->v.origin, move, end);
		trace = SV_Move (tent->v.origin, tent->v.mins, tent->v.maxs, end, MOVE_NORMAL, tent);
		VectorCopy (trace.endpos, tent->v.origin);

		if (trace.ent)
			if (trace.ent != ignore)
				break;
	}

	return trace;
}