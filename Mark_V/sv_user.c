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
// sv_user.c -- server code for moving users

#include "quakedef.h"

edict_t	*sv_player;


static	vec3_t		forward, right, up;

vec3_t	wishdir;
float	wishspeed;

// world
float	*angles;
float	*origin;
float	*velocity;

cbool	onground;

usercmd_t	cmd;


/*
===============
SV_SetIdealPitch
===============
*/
#define	MAX_FORWARD	6
void SV_SetIdealPitch (void)
{
	float	angleval, sinval, cosval;
	trace_t	tr;
	vec3_t	top, bottom;
	float	z[MAX_FORWARD];
	int		i, j;
	int		step, dir, steps;

	if (!((int)sv_player->v.flags & FL_ONGROUND))
		return;

	angleval = sv_player->v.angles[YAW] * M_PI*2 / 360;
	sinval = sin(angleval);
	cosval = cos(angleval);

	for (i=0 ; i<MAX_FORWARD ; i++)
	{
		top[0] = sv_player->v.origin[0] + cosval*(i+3)*12;
		top[1] = sv_player->v.origin[1] + sinval*(i+3)*12;
		top[2] = sv_player->v.origin[2] + sv_player->v.view_ofs[2];

		bottom[0] = top[0];
		bottom[1] = top[1];
		bottom[2] = top[2] - 160;

		tr = SV_Move (top, vec3_origin, vec3_origin, bottom, 1, sv_player);

		if (tr.allsolid)
			return;	// looking at a wall, leave ideal the way is was

		if (tr.fraction == 1)
			return;	// near a dropoff

		z[i] = top[2] + tr.fraction*(bottom[2]-top[2]);
	}

	dir = 0;
	steps = 0;

	for (j=1 ; j<i ; j++)
	{
		step = z[j] - z[j-1];

		if (step > -ON_EPSILON && step < ON_EPSILON)
			continue;

		if (dir && ( step-dir > ON_EPSILON || step-dir < -ON_EPSILON ) )
			return;		// mixed changes

		steps++;
		dir = step;
	}

	if (!dir)
	{
		sv_player->v.idealpitch = 0;
		return;
	}

	if (steps < 2)
		return;

	sv_player->v.idealpitch = -dir * sv_idealpitchscale.value;
}


/*
==================
SV_UserFriction

==================
*/
void SV_UserFriction (double frametime)
{
	float	*vel;
	float	speed, newspeed, control;
	vec3_t	start, stop;
	float	friction;
	trace_t	trace;

	vel = velocity;

	speed = sqrt(vel[0]*vel[0] +vel[1]*vel[1]);

	if (!speed)
		return;

// if the leading edge is over a dropoff, increase friction
	start[0] = stop[0] = origin[0] + vel[0]/speed*16;
	start[1] = stop[1] = origin[1] + vel[1]/speed*16;
	start[2] = origin[2] + sv_player->v.mins[2];
	stop[2] = start[2] - 34;

	trace = SV_Move (start, vec3_origin, vec3_origin, stop, true, sv_player);

	if (trace.fraction == 1.0)
		friction = sv_friction.value*sv_edgefriction.value;
	else friction = sv_friction.value;

// apply friction
	control = speed < sv_stopspeed.value ? sv_stopspeed.value : speed;
	newspeed = speed - frametime * control * friction;

	if (newspeed < 0)
		newspeed = 0;

	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}

/*
==============
SV_Accelerate
==============
*/
void SV_Accelerate (double frametime)
{
	int			i;
	float		addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct (velocity, wishdir);
	addspeed = wishspeed - currentspeed;

	if (addspeed <= 0)
		return;

	accelspeed = sv_accelerate.value * frametime * wishspeed;

	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i=0 ; i<3 ; i++)
		velocity[i] += accelspeed*wishdir[i];
}

void SV_AirAccelerate (vec3_t wishveloc, double frametime)
{
	int			i;
	float		addspeed, wishspd, accelspeed, currentspeed;

	wishspd = VectorNormalize (wishveloc);

	if (wishspd > 30)
		wishspd = 30;

	currentspeed = DotProduct (velocity, wishveloc);
	addspeed = wishspd - currentspeed;

	if (addspeed <= 0)
		return;

//	accelspeed = sv_accelerate.value * frametime;
	accelspeed = sv_accelerate.value * wishspeed * frametime;

	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i=0 ; i<3 ; i++)
		velocity[i] += accelspeed*wishveloc[i];
}


void DropPunchAngle (double frametime)
{
	float	len;

	len = VectorNormalize (sv_player->v.punchangle);
	len -= 10* frametime;

	if (len < 0)
		len = 0;

	VectorScale (sv_player->v.punchangle, len, sv_player->v.punchangle);
}

/*
===================
SV_WaterMove

===================
*/
void SV_WaterMove (double frametime)
{
	int		i;
	vec3_t	wishvel;
	float	speed, newspeed, addspeed, accelspeed;
	//float	wishspeed;

// user intentions
	AngleVectors (sv_player->v.v_angle, forward, right, up);

	for (i=0 ; i<3 ; i++)
		wishvel[i] = forward[i]*cmd.forwardmove + right[i]*cmd.sidemove;

	if (!cmd.forwardmove && !cmd.sidemove && !cmd.upmove)
		wishvel[2] -= 60;		// drift towards bottom
	else wishvel[2] += cmd.upmove;

	wishspeed = VectorLength(wishvel);

	if (wishspeed > sv_maxspeed.value)
	{
		VectorScale (wishvel, sv_maxspeed.value/wishspeed, wishvel);
		wishspeed = sv_maxspeed.value;
	}

	wishspeed *= 0.7;

// water friction
	speed = VectorLength (velocity);

	if (speed)
	{
		newspeed = speed - frametime * speed * sv_friction.value;

		if (newspeed < 0)
			newspeed = 0;

		VectorScale (velocity, newspeed/speed, velocity);
	}
	else newspeed = 0;

// water acceleration
	if (!wishspeed)
		return;

	addspeed = wishspeed - newspeed;

	if (addspeed <= 0)
		return;

	VectorNormalize (wishvel);
	accelspeed = sv_accelerate.value * wishspeed * frametime;

	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i=0 ; i<3 ; i++)
		velocity[i] += accelspeed * wishvel[i];
}

void SV_WaterJump (void)
{
	if (sv.time > sv_player->v.teleport_time || !sv_player->v.waterlevel)
	{
		sv_player->v.flags = (int)sv_player->v.flags & ~FL_WATERJUMP;
		sv_player->v.teleport_time = 0;
	}

	sv_player->v.velocity[0] = sv_player->v.movedir[0];
	sv_player->v.velocity[1] = sv_player->v.movedir[1];
}



/*
===================
SV_AirMove
===================
*/
void SV_AirMove (double frametime)
{
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;

	AngleVectors (sv_player->v.angles, forward, right, up);

	fmove = cmd.forwardmove;
	smove = cmd.sidemove;

// hack to not let you back into teleporter
	if (sv.time < sv_player->v.teleport_time && fmove < 0)
		fmove = 0;

	for (i=0 ; i<3 ; i++)
		wishvel[i] = forward[i]*fmove + right[i]*smove;

	if ( (int)sv_player->v.movetype != MOVETYPE_WALK)
		wishvel[2] = cmd.upmove;
	else
		wishvel[2] = 0;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (wishspeed > sv_maxspeed.value)
	{
		VectorScale (wishvel, sv_maxspeed.value/wishspeed, wishvel);
		wishspeed = sv_maxspeed.value;
	}

	if ( sv_player->v.movetype == MOVETYPE_NOCLIP)
	{
		// noclip
		VectorCopy (wishvel, velocity);
	}
	else if ( onground )
	{
		SV_UserFriction (frametime);
		SV_Accelerate (frametime);
	}
	else
	{
		// not on ground, so little effect on velocity
		SV_AirAccelerate (wishvel, frametime);
	}
}

/*
===================
SV_NoclipMove -- johnfitz

new, alternate noclip. old noclip is still handled in SV_AirMove
===================
*/
void SV_NoclipMove (void)
{
	AngleVectors (sv_player->v.v_angle, forward, right, up);

	velocity[0] = forward[0]*cmd.forwardmove + right[0]*cmd.sidemove;
	velocity[1] = forward[1]*cmd.forwardmove + right[1]*cmd.sidemove;
	velocity[2] = forward[2]*cmd.forwardmove + right[2]*cmd.sidemove;

	//doubled to match running speed
	velocity[2] += cmd.upmove*2; 

	if (VectorLength (velocity) > sv_maxspeed.value)
	{
		VectorNormalize (velocity);
		VectorScale (velocity, sv_maxspeed.value, velocity);
	}
}
/*
===================
SV_ClientThink

the move fields specify an intended velocity in pix/sec
the angle fields specify an exact angular motion in degrees
===================
*/
// Exclusively called by RunClients
static void SV_Host_Frame_RunClients_ClientThink (double frametime)
{
	vec3_t		v_angle;

	if (sv_player->v.movetype == MOVETYPE_NONE)
		return;

	onground = (int)sv_player->v.flags & FL_ONGROUND;

	origin = sv_player->v.origin;
	velocity = sv_player->v.velocity;

	DropPunchAngle (frametime);

// if dead, behave differently
	if (sv_player->v.health <= 0)
		return;

// angles
// show 1/3 the pitch angle and all the roll angle
	cmd = host_client->cmd;
	angles = sv_player->v.angles;

	VectorAdd (sv_player->v.v_angle, sv_player->v.punchangle, v_angle);
	angles[ROLL] = Math_CalcRoll (sv_player->v.angles, sv_player->v.velocity, cl_rollangle.value, cl_rollspeed.value)*4;

	if (!sv_player->v.fixangle)
	{
		angles[PITCH] = -v_angle[PITCH]/3;
		angles[YAW] = v_angle[YAW];
	}

	if ( (int)sv_player->v.flags & FL_WATERJUMP )
	{
		SV_WaterJump ();
		return;
	}

// walk
	if (sv_player->v.movetype == MOVETYPE_NOCLIP && sv_altnoclip.value)
		SV_NoclipMove ();
	else if (sv_player->v.waterlevel >= 2 && sv_player->v.movetype != MOVETYPE_NOCLIP)
		SV_WaterMove (frametime);
	else SV_AirMove (frametime);
}


/*
===================
SV_ReadClientMove
===================
*/
void SV_ReadClientMove (usercmd_t *move)
{
	int		i;
	vec3_t	angle;
	int		bits;

// read ping time
	host_client->ping_times[host_client->num_pings%NUM_PING_TIMES]
		= sv.time - MSG_ReadFloat ();
	host_client->num_pings++;

// read current angles
	for (i=0 ; i<3 ; i++)
	{
		//johnfitz -- 16-bit angles for PROTOCOL_FITZQUAKE
		if (sv.protocol == PROTOCOL_NETQUAKE && !NET_QSocketIsProQuakeServer(host_client->netconnection))
			angle[i] = MSG_ReadAngle ();
		else
			angle[i] = MSG_ReadAngle16 ();
		//johnfitz
	}

	VectorCopy (angle, host_client->edict->v.v_angle);

// read movement
	move->forwardmove = MSG_ReadShort ();
	move->sidemove = MSG_ReadShort ();
	move->upmove = MSG_ReadShort ();

// read buttons
	bits = MSG_ReadByte ();
	host_client->edict->v.button0 = bits & 1;
	host_client->edict->v.button2 = (bits & 2)>>1;

	// read impulse
	i = MSG_ReadByte ();

	if (i) host_client->edict->v.impulse = i;
}

/*
===================
SV_ReadClientMessage

Returns false if the client should be killed
===================
*/
cbool SV_ReadClientMessage (void)
{
	int ret;
	int		ccmd;
	const char	*s;

		MSG_BeginReading ();

		while (1)
		{
			if (!host_client->active)
				return false;	// a command caused an error

			if (msg_badread)
			{
				Dedicated_PrintLinef ("SV_ReadClientMessage: badread");
				return false;
			}

			ccmd = MSG_ReadChar ();

//			Dedicated_PrintLinef ("SV_ReadClientMessage: Command %d", ccmd);

			switch (ccmd)
			{
			case -1:
			return true;	//msg_badread, meaning we just hit eof.

			default:
				Dedicated_PrintLinef ("SV_ReadClientMessage: unknown command char %d", ccmd);
				return false;

			case clc_nop:
//				Dedicated_PrintLinef ("clc_nop");
				break;

			case clc_stringcmd:
				s = MSG_ReadString ();
//				Con_PrintLinef ("Message: " QUOTED_S, s);

				if (host_client->privileged)
					ret = 2;
				else ret = 0;

				{
					static const char * valid_cmds[] = {
						"ban", "begin", "color", "fly", "freezeall", "give", "god", "kick", 
						"kill", "name", "noclip", "notarget", "pause", "ping", "prespawn", 
						"qcexec", "say", "say_team", "setpos", "spawn", "status", "tell",
						NULL
					};
					const char **cur;
					for (cur = valid_cmds; *cur; cur ++)
					{
						if (!strncasecmp (s, *cur, strlen(*cur)))
						{
							ret = 1;
							break;
						}
					} // End of for
				}

				switch (ret)
				{
				case 2: // client priviledged
					Cbuf_InsertText (s);
					break;
				case 1: // common command any client can do
					Cmd_ExecuteString (s, src_client);
					break;
				default: // 
					 Con_DPrintLinef("%s tried to %s", host_client->name, s);
					break;
				}

				break; // Get out of here
			case clc_disconnect:
				Dedicated_PrintLinef ("SV_ReadClientMessage: client disconnected");
				return false;

			case clc_move:
				SV_ReadClientMove (&host_client->cmd);
				break;
			}
		}

	return true;
}


/*
==================
SV_ClientThink_RunClients
==================
*/
// Called exclusively by Host_Frame
// We exclusively call SV_ClientThink
void SV_Host_Frame_RunClients (double frametime) // MH had this as a float, not double?
{
	int				i;
//	struct qsocket_s *sock;

	//receive from clients first
	//Spike -- reworked this to query the network code for an active connection.
	//this allows the network code to serve multiple clients with the same listening port.
	//this solves server-side nats, which is important for coop etc.
	while(1)
	{
		struct qsocket_s *sock = NET_GetServerMessage();
		if (!sock)
			break;	//no more this frame

		for (i = 0, host_client = svs.clients ; i < svs.maxclients_internal ; i++, host_client++) // Because the cap can change at any time now.
		{
			if (host_client->netconnection == sock)
			{
				sv_player = host_client->edict;
				if (!SV_ReadClientMessage ())
				{
					SV_DropClient (false);	// client misbehaved...
					break;
				}
				//else Con_PrintLinef ("Read a message from socket for %d", i);
			}
		}
	}

	//then do the per-frame stuff
	for (i = 0, host_client = svs.clients ; i < svs.maxclients_internal; i ++, host_client ++)  // Because the cap can change at any time now.
	{
		if (!host_client->active)
			continue;

		sv_player = host_client->edict;

		if (!host_client->spawned)
		{
		// clear client movement until a new packet is received
			memset (&host_client->cmd, 0, sizeof(host_client->cmd));
			continue;
		}

		//Con_Printf ("Ran a frame");

		if (!host_client->netconnection)
		{
				i =i;  //breakpoint me
		}

// always pause in single player if in console or menus
		if (!sv.paused && (svs.maxclients_internal > 1 || key_dest == key_game) ) // Because 1 is still special for cap, because it means single player.
			SV_Host_Frame_RunClients_ClientThink (frametime); // Called here
	}
}