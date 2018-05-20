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
// cl.input.c  -- builds an intended movement command to send to the server

// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

#include "quakedef.h"


/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition

===============================================================================
*/


kbutton_t	in_mlook, in_klook;
kbutton_t	in_left, in_right, in_forward, in_back;
kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
kbutton_t	in_strafe, in_speed, in_use, in_jump, in_attack;
kbutton_t	in_up, in_down;

int			in_impulse;


void KeyDown (lparse_t* line, kbutton_t *b)
{
	int		k;

	if (line->count >=2)
		k = atoi (line->args[1]);
	else
		k = -1;		// typed manually at the console for continuous down

// PQ_MOVEUP START
	// JPG 1.05 - if jump is pressed underwater, translate it to a moveup
	if (b == &in_jump && pq_moveup.value && cl.stats[STAT_HEALTH] > 0 && cl.inwater)
		b = &in_up;
// PQ_MOVEUP END

	if (k == b->down[0] || k == b->down[1])
		return;		// repeating key

	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		Con_PrintLinef ("Three keys down for a button!");
		return;
	}

	if (b->state & 1)
		return;		// still down

	b->state |= 1 + 2;	// down + impulse down
}

void KeyUp (lparse_t* line, kbutton_t *b)
{
	int		k = 0; // ?

#if 0
	const char	*c;

	c = line->args[1];
	if (c[0])
		k = atoi(c);
#else
	if (line->count >=2)
		k = atoi (line->args[1]);
#endif
	else
	{
		// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

// PQMOVEUP
	// JPG 1.05 - check to see if we need to translate -jump to -moveup
	if (b == &in_jump && pq_moveup.value)
	{
		if (k == in_up.down[0] || k == in_up.down[1])
			b = &in_up;
		else
		{
			// in case a -moveup got lost somewhere
			in_up.down[0] = in_up.down[1] = 0;
			in_up.state = 4;
		}
	}
// PQMOVEUP

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)

	if (b->down[0] || b->down[1])
		return;		// some other key is still holding it down

	if (!(b->state & 1))
		return;		// still up (this should not happen)

	b->state &= ~1;		// now up
	b->state |= 4; 		// impulse up
}

void IN_KLookDown (lparse_t* line)        { KeyDown (line, &in_klook);      }
void IN_KLookUp (lparse_t* line)          { KeyUp (line, &in_klook);        }
void IN_MLookDown (lparse_t* line)        { KeyDown (line, &in_mlook);      }
void IN_MLookUp (lparse_t* line)
{
    KeyUp(line, &in_mlook);

    if ( !MOUSELOOK_ACTIVE &&  lookspring.value)
        View_StartPitchDrift(NULL);
}

void IN_UpDown (lparse_t* line)           { KeyDown (line, &in_up);         }
void IN_UpUp (lparse_t* line)             { KeyUp (line, &in_up);           }
void IN_DownDown (lparse_t* line)         { KeyDown (line, &in_down);       }
void IN_DownUp (lparse_t* line)           { KeyUp (line, &in_down);         }
void IN_LeftDown (lparse_t* line)         { KeyDown (line, &in_left);       }
void IN_LeftUp (lparse_t* line)           { KeyUp (line, &in_left);         }
void IN_RightDown (lparse_t* line)        { KeyDown (line, &in_right);      }
void IN_RightUp (lparse_t* line)          { KeyUp (line, &in_right);        }
void IN_ForwardDown (lparse_t* line)      { KeyDown (line, &in_forward);    }
void IN_ForwardUp (lparse_t* line)        { KeyUp (line, &in_forward);      }
void IN_BackDown (lparse_t* line)         { KeyDown (line, &in_back);       }
void IN_BackUp (lparse_t* line)           { KeyUp (line, &in_back);         }
void IN_LookupDown (lparse_t* line)       { KeyDown (line, &in_lookup);     }
void IN_LookupUp (lparse_t* line)         { KeyUp (line, &in_lookup);       }
void IN_LookdownDown (lparse_t* line)     { KeyDown (line, &in_lookdown);   }
void IN_LookdownUp (lparse_t* line)       { KeyUp (line, &in_lookdown);     }
void IN_MoveleftDown (lparse_t* line)     { KeyDown (line, &in_moveleft);   }
void IN_MoveleftUp (lparse_t* line)       { KeyUp (line, &in_moveleft);     }
void IN_MoverightDown (lparse_t* line)    { KeyDown (line, &in_moveright);  }
void IN_MoverightUp (lparse_t* line)      { KeyUp (line, &in_moveright);    }

void IN_SpeedDown (lparse_t* line)        { KeyDown (line, &in_speed);      }
void IN_SpeedUp (lparse_t* line)          { KeyUp (line, &in_speed);        }
void IN_StrafeDown (lparse_t* line)       { KeyDown (line, &in_strafe);     }
void IN_StrafeUp (lparse_t* line)         { KeyUp (line, &in_strafe);       }

void IN_AttackDown (lparse_t* line)       { KeyDown (line, &in_attack);     }
void IN_AttackUp (lparse_t* line)         { KeyUp (line, &in_attack);       }

void IN_UseDown (lparse_t* line)          { KeyDown (line, &in_use);        }
void IN_UseUp (lparse_t* line)            { KeyUp (line, &in_use);          }
void IN_JumpDown (lparse_t* line)         { KeyDown (line, &in_jump);       }
void IN_JumpUp (lparse_t* line)           { KeyUp (line, &in_jump);         }

void IN_Impulse (lparse_t* line)			{ in_impulse = atoi(line->args[1]); }

static int weaponstat[7] = {STAT_SHELLS, STAT_SHELLS, STAT_NAILS, STAT_NAILS, STAT_ROCKETS, STAT_ROCKETS, STAT_CELLS};

/*
===============
IN_BestWeapon
===============
*/
void IN_BestWeapon (lparse_t *line)
{
	int i, impulse;

	for (i = 1 ; i < line->count ; i++)
	{
		impulse = atoi(line->args[i]);
		if (impulse > 0 && impulse < 9 && (impulse == 1 ||
			((cl.items & (IT_SHOTGUN << (impulse - 2))) && cl.stats[weaponstat[impulse-2]])))
		{
			in_impulse = impulse;
			break;
		}
	}
}


/*
===============
CL_KeyState

Returns 0.25 if a key was pressed and released during the frame,
0.5 if it was pressed and held
0 if held then released, and
1.0 if held for the entire time
===============
*/
float CL_KeyState (kbutton_t *key)
{
	float		val;
	cbool	impulsedown, impulseup, down;

	impulsedown = key->state & 2;
	impulseup = key->state & 4;
	down = key->state & 1;
	val = 0;

	if (impulsedown && !impulseup)
	{
		if (down)
			val = 0.5;	// pressed and held this frame
		else
			val = 0;	//	I_Error ();
	}

	if (impulseup && !impulsedown)
	{
		if (down)
			val = 0;	//	I_Error ();
		else
			val = 0;	// released this frame
	}

	if (!impulsedown && !impulseup)
	{
		if (down)
			val = 1.0;	// held the entire frame
		else
			val = 0;	// up the entire frame
	}

	if (impulsedown && impulseup)
	{
		if (down)
			val = 0.75;	// released and re-pressed this frame
		else
			val = 0.25;	// pressed and released this frame
	}

	key->state &= 1;		// clear impulses

	return val;
}


/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles (void)
{
	float	speed;
	float	up, down;

	if (cl.paused) return;

#ifdef SUPPORTS_SHIFT_SLOW_ALWAYS_RUN // Baker: Which is disabled for several reasons
	if ((cl_forwardspeed.value > 200) ^ (in_speed.state & 1))
#else
	if (in_speed.state & 1)
#endif
		speed = cl_frametime * cl_anglespeedkey.value;

	else
		speed = cl_frametime;

	if (!(in_strafe.state & 1))
	{
		cl.viewangles[YAW] -= speed*cl_yawspeed.value*CL_KeyState (&in_right);
		cl.viewangles[YAW] += speed*cl_yawspeed.value*CL_KeyState (&in_left);
		cl.viewangles[YAW] = anglemod (cl.viewangles[YAW]);
	}

	if (in_klook.state & 1)
	{
		View_StopPitchDrift ();
		cl.viewangles[PITCH] -= speed*cl_pitchspeed.value * CL_KeyState (&in_forward);
		cl.viewangles[PITCH] += speed*cl_pitchspeed.value * CL_KeyState (&in_back);
	}

	up = CL_KeyState (&in_lookup);
	down = CL_KeyState(&in_lookdown);

	cl.viewangles[PITCH] -= speed*cl_pitchspeed.value * up;
	cl.viewangles[PITCH] += speed*cl_pitchspeed.value * down;

	if (up || down)
		View_StopPitchDrift ();

	CL_BoundViewPitch (cl.viewangles); // Keyboard lock

	if (cl.viewangles[ROLL] > 50)
		cl.viewangles[ROLL] = 50;

	if (cl.viewangles[ROLL] < -50)
		cl.viewangles[ROLL] = -50;

}

/*
================
CL_BaseMove

Send the intended movement message to the server
================
*/
void CL_BaseMove (usercmd_t *cmd)
{
	if (cls.signon != SIGNONS)
		return;

	CL_AdjustAngles ();

	memset (cmd, 0, sizeof(*cmd));

	if (in_strafe.state & 1)
	{
		cmd->sidemove += cl_sidespeed.value * CL_KeyState (&in_right);
		cmd->sidemove -= cl_sidespeed.value * CL_KeyState (&in_left);
	}

	cmd->sidemove += cl_sidespeed.value * CL_KeyState (&in_moveright);
	cmd->sidemove -= cl_sidespeed.value * CL_KeyState (&in_moveleft);

	cmd->upmove += cl_upspeed.value * CL_KeyState (&in_up);
	cmd->upmove -= cl_upspeed.value * CL_KeyState (&in_down);

	if (! (in_klook.state & 1) )
	{
		cmd->forwardmove += cl_forwardspeed.value * CL_KeyState (&in_forward);
		cmd->forwardmove -= cl_backspeed.value * CL_KeyState (&in_back);
	}

// adjust for speed key
#ifdef SUPPORTS_SHIFT_SLOW_ALWAYS_RUN
	if (cl_forwardspeed.value > 200 && cl_movespeedkey.value)
		cmd->forwardmove /= cl_movespeedkey.value;
	if ((cl_forwardspeed.value > 200) ^ (in_speed.state & 1))
#else
	if (in_speed.state & 1)
#endif
	{
		cmd->forwardmove *= cl_movespeedkey.value;
		cmd->sidemove *= cl_movespeedkey.value;
		cmd->upmove *= cl_movespeedkey.value;
	}
}



/*
==============
CL_SendMove
==============
*/
void CL_SendMove (const usercmd_t *cmd)
{
	int		i;
	int		bits;
	sizebuf_t	buf;
	byte	data[128];

	buf.maxsize = 128;
	buf.cursize = 0;
	buf.data = data;

	cl.cmd = *cmd;

// send the movement message
    MSG_WriteByte (&buf, clc_move);

	MSG_WriteFloat (&buf, cl.mtime[0]);	// so server can get ping times

	//johnfitz -- 16-bit angles for PROTOCOL_FITZQUAKE
	for (i=0 ; i<3 ; i++)
	{
#if 1
		if (cl.protocol == PROTOCOL_NETQUAKE  && (cls.demoplayback || !NET_QSocketIsProQuakeServer(cls.netcon)) )
#else
		if (cl.protocol == PROTOCOL_NETQUAKE)
#endif
			MSG_WriteAngle (&buf, cl.viewangles[i]);
		else
			MSG_WriteAngle16 (&buf, cl.viewangles[i]);
	}
		//johnfitz

    MSG_WriteShort (&buf, cmd->forwardmove);
    MSG_WriteShort (&buf, cmd->sidemove);
    MSG_WriteShort (&buf, cmd->upmove);

// send button bits
	bits = 0;

	if ( in_attack.state & 3 )
		bits |= 1;

	in_attack.state &= ~2;

	if (in_jump.state & 3)
		bits |= 2;

	in_jump.state &= ~2;

    MSG_WriteByte (&buf, bits);

    MSG_WriteByte (&buf, in_impulse);
	in_impulse = 0;

//
// deliver the message
//
	if (cls.demoplayback)
		return;

//
// always dump the first two message, because it may contain leftover inputs
// from the last level
//
	if (++cl.movemessages <= 2)
		return;

	if (NET_SendUnreliableMessage (cls.netcon, &buf) == -1)
	{
		Con_PrintLinef ("CL_SendMove: lost server connection");
		CL_Disconnect ();
	}
}

void CL_BoundViewPitch (float *viewangles)
{
	cl.viewangles[PITCH] = CLAMP (cl_minpitch.value, cl.viewangles[PITCH], cl_maxpitch.value);
}

void IN_PQ_Full_Pitch_f (lparse_t *line)
{
	if (line->count !=2)
		return;

	if (!strcmp(line->args[1], "0"))
	{
		// Baker: This is writing a short and then re-reading it.  It is supposed to be 80,
		// but FitzQuake rounds angles ends and due to floating point imprecision after
		// being round and converted to a short int, ends up being 80.0024 which a
		// ProQuake server rejects as invalid being above 80 tries to correct.  The result
		// is "bounce back" when looking real far down, and it is annoying as hell.
		static const float proquake_maxpitch = ((c_rint(80.0 * 65536.0 / 360.0) & 65535)-1) * (360.0 / 65536); // Equals 79.9969

		Cvar_Set_Untrusted (cl_minpitch.name, "-70");
		Cvar_Set_Untrusted (cl_maxpitch.name, va("%f", proquake_maxpitch)); // Baker: 80 doesn't work right when connect to a ProQuake server for some reason, gets slight jitter?
		Con_DPrintLinef ("ProQuake fullpitch temporarily set for duration of map");
	}
}

/*
============
CL_InitInput
============
*/
void CL_InitInput (void)
{
	Cmd_AddCommands (CL_InitInput);
}