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
// cl_main.c  -- client main loop

#include "quakedef.h"

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

// these two are not intended to be set directly

client_static_t	cls;
client_state_t	cl;

entity_t		*cl_entities; //johnfitz -- was a static array, now on hunk
int				cl_max_edicts; //johnfitz -- only changes when new map loads


/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState (unsigned int protocol_num)
{
	int			i;

	Con_DPrintLinef ("Host_ClearMemory");

	if (!sv.active)
		Host_ClearMemory ();

// wipe the entire cl structure
	memset (&cl, 0, sizeof(cl)); // Technically redundant as Host_ClearMemory already did this.

	SZ_Clear (&cls.message);

	//johnfitz -- cl_entities is now dynamically allocated
	cl.protocol = protocol_num;

	cl_max_edicts = MAX_SANE_EDICTS_8192; // Client always uses this size now.


#ifdef SUPPORTS_SERVER_PROTOCOL_15
// Baker: If not running a server, restore datagram cap for client
	if (!sv.active)
		host_protocol_datagram_maxsize = MAX_MARK_V_DATAGRAM;
#endif // SUPPORTS_SERVER_PROTOCOL_15
	
	cl_entities = Hunk_AllocName (cl_max_edicts*sizeof(entity_t), "cl_entities");

// allocate the efrags and chain together into a free list
	cl.free_efrags = cl.efrags;

	for (i=0 ; i< MAX_MARK_V_EFRAGS - 1 ; i++)
		cl.free_efrags[i].entnext = &cl.free_efrags[i+1];

	cl.free_efrags[i].entnext = NULL;

#ifdef SUPPORTS_NEHAHRA
	if (nehahra_active)
		SHOWLMP_clear ();
#endif // SUPPORTS_NEHAHRA

}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (void)
{
// stop sounds (especially looping!)
	S_StopAllSounds (true);
	CDAudio_Stop();

// HTTP Depot download shutdown in CL_Disconnect_f instead of here.

#ifdef SUPPORTS_NEHAHRA
	if (nehahra_active)
		FMOD_Stop ();
#endif // SUPPORTS_NEHAHRA

// if running a local server, shut it down
	if (cls.demoplayback)
		CL_StopPlayback ();

// Baker: moved up, this only occurred before if state was connected.
// but there is no requirement to be connected to record a demo.
	if (cls.demorecording)
		CL_Stop_f (NULL);

	if (cls.state == ca_connected)
	{
		Con_DPrintLinef ("Sending clc_disconnect");
		SZ_Clear (&cls.message);
		MSG_WriteByte (&cls.message, clc_disconnect);
		NET_SendUnreliableMessage (cls.netcon, &cls.message);
		SZ_Clear (&cls.message);
		NET_Close (cls.netcon);

		cls.state = ca_disconnected;

		if (sv.active)
			Host_ShutdownServer(false);
	}

	cls.demoplayback = cls.timedemo = false;
	cls.signon = 0;
	cl.intermission = 0;
	cl.worldmodel = NULL; // Spike

#pragma message ("What are the things we need to wipe to get to a clean state.  Should we wipe cl and sv too?")
#pragma message ("Nuke viewblends")
#pragma message ("If keydest is game, set it to console.  Except this occurs at the end of a demo in a start demos chain")
#ifdef SUPPORTS_NEHAHRA
	if (nehahra_active)
		Neh_ResetSFX ();
#endif // SUPPORTS_NEHAHRA
	//SCR_EndLoadingPlaque (); // Baker: any disconnect state should end the loading plague, right?
	// No because demo to demo transition or starting up a new game

// SUPPORTS_KEYBIND_FLUSH
	// Possible weakness is a "lost server connection"
	// i.e. disconnect without explicit disconnect
	// But server keybinds don't save anyway and
	// they still remain temp so they'd disappear on next disconnect
	if ( !sv_fix_flush_alias_exceptions.string[0] || String_Does_Not_Start_With_Caseless( gamedir_shortname(), sv_fix_flush_alias_exceptions.string)) {
		Keys_Flush_ServerBinds (); // Flush keybinds altered by the server on disconnect
		Cmd_Unalias_ServerAliases ();
	}

}

void CL_Disconnect_f (lparse_t *unused)
{
// Baker --- this handles client shutdown

#ifdef SUPPORTS_PQ_CL_HTTP_DOWNLOAD 
	// If an HTTP download is in progress, don't disconnect because we have the loop nested.
	// Instead set a disconnect to true and we'll disconnect at the very next moment it is possible.  Ensures proper cleanup.
	if (cls.download.name[0]) {
		cls.download.disconnect = true;
		return;
	}
#endif // SUPPORTS_PQ_CL_HTTP_DOWNLOAD 

	CL_Clear_Demos_Queue (); // timedemo is a very intentional action

	CL_Disconnect ();
// Baker --- this handles dedicated server shutdown
	if (sv.active)
		Host_ShutdownServer (false);
}


/*
=====================
CL_EstablishConnection

Host should be either "local" or a net address to be passed on
=====================
*/
void CL_EstablishConnection (const char *host)
{
// We do not hit here on reconnect/changelevel
	static char lasthost[NET_NAMELEN_64];
	if (isDedicated)
		return;

#ifdef SUPPORTS_NEHAHRA
	Neh_Reset_Sfx_Count ();
#endif // SUPPORTS_NEHAHRA

	if (cls.demoplayback)
		return;

	if (!host) {
		if (!lasthost[0]) {
			Con_PrintLinef ("No server to reconnect!");
			return;
		}
		host = lasthost;
	}
	else c_strlcpy(lasthost, host);

#ifdef BUGFIX_DEMO_RECORD_BEFORE_CONNECTED_FIX
	// Baker this prevents shutdown of demo recording
	// before connecting.
	if (cls.state == ca_connected)
#endif // BUGFIX_DEMO_RECORD_BEFORE_CONNECTED_FIX
		CL_Disconnect ();

	cls.netcon = NET_Connect (host);

	if (!cls.netcon)
	{
		const char *extra = "";
		if (net_hostport != DEFAULT_QPORT_26000)
			extra = va ("\nTry using port %d\n", DEFAULT_QPORT_26000); //r00k added
		Host_Error ("connect failed" NEWLINE "%s", extra);
	}
	Con_DPrintLinef ("CL_EstablishConnection: connected to %s", host);

	if (NET_QSocketIsProQuakeServer(cls.netcon))
		Con_PrintLinef ("%c%cConnected to %s server%c", 2, 29, "ProQuake", 31); // char 2 means bronze it.

	cls.demonum = -1;			// not in the demo loop now
	cls.state = ca_connected;
	cls.signon = 0;				// need all the signon messages before playing
	MSG_WriteByte (&cls.message, clc_nop);	// NAT Fix from ProQuake
}

/*
=====================
CL_SignonReply

An svc_signonnum has been received, perform a client side setup
=====================
*/
void CL_SignonReply (void)
{
//	char str[8192]; // 8192 is somewhat bigly.

	Con_DPrintLinef ("CL_SignonReply: %d", cls.signon);

	switch (cls.signon)
	{
	case 1:
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, "prespawn");
		break;

	case 2:
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteStringf (&cls.message, "name " QUOTED_S NEWLINE, cl_name.string);

		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteStringf (&cls.message, "color %d %d" NEWLINE, ((int)cl_color.value) >> 4, ((int)cl_color.value) & 15);

		MSG_WriteByte (&cls.message, clc_stringcmd);
#if 0
		c_snprintf1 (str, "spawn %s", cls.spawnparms); // Note the lack of newline here.  Interesting
		MSG_WriteString (&cls.message, str);
#else
		// Baker - Dec 30 2016
		MSG_WriteStringf (&cls.message, "spawn %s", cls.spawnparms);
#endif
		break;

	case 3:
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, "begin");
		Cache_Report ();		// print remaining memory
		break;

	case 4:
		SCR_EndLoadingPlaque ();		// allow normal screen updates
		if (cl_autodemo.value && !cls.demoplayback && !cls.demorecording && (cl_autodemo.value >= 2 || host_maxfps.value == 72))
		{
			// Baker: host_maxfps > 72 will lead to really big demos.
			Cmd_ExecuteString ("record " AUTO_DEMO_NAME NEWLINE, src_command); // AFAIK newline is superfluous
		}
		break;
	}
}

/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo (void)
{
	char	str[1024];

	if (cls.demonum == -1)
		return;		// don't play demos

	if (!cls.demos[cls.demonum][0] || cls.demonum == MAX_DEMOS_32)
	{
		cls.demonum = 0;

		if (!cls.demos[cls.demonum][0])
		{
			Con_PrintLinef ("No demos listed with startdemos");
			cls.demonum = -1;
			CL_Disconnect ();
			return;
		}
	}

	SCR_BeginLoadingPlaque_Force_NoTransition ();

	c_snprintf1 (str, "nextstartdemo %s" NEWLINE, cls.demos[cls.demonum]);
	Cbuf_InsertText (str);
	
	cls.demonum++;
}

/*
==============
CL_PrintEntities_f
==============
*/
void CL_PrintEntities_f (lparse_t *unused)
{
	entity_t	*ent;
	int			i;

	for (i = 0, ent = cl_entities ; i < cl.num_entities ; i++, ent++)
	{
		Con_PrintContf ("%3d:", i);

		if (!ent->model)
		{
			Con_PrintLinef ("EMPTY");
			continue;
		}

		Con_PrintLinef
		(
			"%s:%2d  (%5.1f,%5.1f,%5.1f) [%5.1f %5.1f %5.1f]",
			ent->model->name,ent->frame,
			ent->origin[0], ent->origin[1], ent->origin[2],
			ent->angles[0], ent->angles[1], ent->angles[2]
		);
	}
}

//johnfitz -- deleted SetPal()

/*
===============
CL_AllocDlight

===============
*/
dlight_t *CL_AllocDlight (int key)
{
	int		i;
	dlight_t	*dl;

// first look for an exact key match
	if (key)
	{
		dl = cl.dlights;
		for (i = 0 ; i<MAX_FITZQUAKE_DLIGHTS ; i++, dl++)
		{
			if (dl->key == key)
			{
				memset (dl, 0, sizeof(*dl));
				dl->key = key;
#ifdef GLQUAKE_COLORED_LIGHTS
// Why did we comment this out?  I don't think we had a reason?
				dl->color.vec3[0] = dl->color.vec3[1] = dl->color.vec3[2] = 1; //johnfitz -- lit support via lordhavoc
#endif // GLQUAKE_COLORED_LIGHTS
				return dl;
			}
		}
	}

// then look for anything else
	dl = cl.dlights;

	for (i=0 ; i < MAX_FITZQUAKE_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time)
		{
			memset (dl, 0, sizeof(*dl));
			dl->key = key;
#ifdef GLQUAKE_COLORED_LIGHTS
			dl->color.vec3[0] = dl->color.vec3[1] = dl->color.vec3[2] = 1; //johnfitz -- lit support via lordhavoc
#endif // GLQUAKE_COLORED_LIGHTS
			return dl;
		}
	}

	dl = &cl.dlights[0];
	memset (dl, 0, sizeof(*dl));
	dl->key = key;
#ifdef GLQUAKE_COLORED_LIGHTS
	dl->color.vec3[0] = dl->color.vec3[1] = dl->color.vec3[2] = 1; //johnfitz -- lit support via lordhavoc
#endif // GLQUAKE_COLORED_LIGHTS
	return dl;
}


/*
===============
CL_DecayLights

===============
*/
void CL_DecayLights (void)
{
	int			i;
	dlight_t	*dl;
	float		time;

// Baker: forward oriented time even if demo rewinding

	time = fabs(cl.time - cl.oldtime);

	dl = cl.dlights;

	for (i = 0 ; i < MAX_FITZQUAKE_DLIGHTS /*128*/ ; i++, dl++) // MAX_WINQUAKE_DLIGHTS
	{
		if (dl->die < cl.time || !dl->radius)
			continue;

		dl->radius -= time*dl->decay;

		if (dl->radius < 0)
			dl->radius = 0;
	}
}


/*
===============
CL_LerpPoint

Determines the fraction between the last two messages that the objects
should be put at.
===============
*/
static float CL_LerpPoint (void)
{
	extern cbool bumper_on;
	float	f, frac;

	f = cl.mtime[0] - cl.mtime[1];

	if (!f || cls.timedemo || sv.active) // Baker: MH removed sv.active
	{
		cl.time = cl.ctime = cl.mtime[0];
		return 1;
	}

	if (f > 0.1) // dropped packet, or start of demo
	{
		cl.mtime[1] = cl.mtime[0] - 0.1;
		f = 0.1;
	}

	frac = (cl.ctime - cl.mtime[1]) / f;

	if (frac < 0)
	{
		if (frac < -0.01)
		{
			if (bumper_on)
			{
				cl.time = cl.ctime = cl.mtime[1];
			}
			else
			{
				cl.time = cl.ctime = cl.mtime[1];
			}
		}
		frac = 0;
	}
	else if (frac > 1)
	{
		if (frac > 1.01)
		{
			if (bumper_on)
			{
				cl.time = cl.ctime = cl.mtime[0];
			}
			else
			{
				cl.time = cl.ctime = cl.mtime[0]; // Here is where we get foobar'd
			}
		}
		frac = 1;
	}

	//johnfitz -- better nolerp behavior
	if (cl_nolerp.value)
		return 1;

	//johnfitz

	return frac;
}


vec3_t	player_origin[MAX_SCOREBOARD_16];
int	numplayers;

/*
===============
CL_RelinkEntities
===============
*/

static float	_mathlib_temp_float1, _mathlib_temp_float2;
static vec3_t	_mathlib_temp_vec1;

#define VectorL2Compare(v, w, m)				\
	(_mathlib_temp_float1 = (m) * (m),			\
	_mathlib_temp_vec1[0] = (v)[0] - (w)[0], _mathlib_temp_vec1[1] = (v)[1] - (w)[1], _mathlib_temp_vec1[2] = (v)[2] - (w)[2],\
	_mathlib_temp_vec1[0] * _mathlib_temp_vec1[0] +		\
	_mathlib_temp_vec1[1] * _mathlib_temp_vec1[1] +		\
	_mathlib_temp_vec1[2] * _mathlib_temp_vec1[2] < _mathlib_temp_float1)


static void CL_RelinkEntities (void)
{
	entity_t		*ent;
	int				i, j;
	float			frac, f, d;
	vec3_t			delta;
	float			bobjrotate;
	vec3_t			oldorg;
	
#ifdef GLQUAKE_SUPPORTS_QMB
	dlight_t		*dl;
	dlighttype_e	color_type;
#endif // GLQUAKE_SUPPORTS_QMB

// determine partial update time
	frac = CL_LerpPoint ();
#pragma message ("Baker: CL_Lerp point can change cl.time")

#if 1 // MH has this 
	// update frametime after CL_LerpPoint as it can change the value of cl.time
	cl_frametime = cl.time - cl.oldtime;
#endif

	cl.numvisedicts = 0;

// interpolate player info
	for (i = 0 ; i < 3 ; i++)
		cl.velocity[i] = cl.mvelocity[1][i] + frac * (cl.mvelocity[0][i] - cl.mvelocity[1][i]);

	if (cls.demoplayback || cl.last_angle_time > cl.time)
	{
	// interpolate the angles
		for (j=0 ; j<3 ; j++)
		{
			d = cl.mviewangles[0][j] - cl.mviewangles[1][j];

			if (d > 180)
				d -= 360;
			else if (d < -180)
				d += 360;

			cl.lerpangles[j] = cl.mviewangles[1][j] + frac*d;
			if (cls.demoplayback)
				cl.viewangles[j] = cl.mviewangles[1][j] + frac*d;
		}
	}
	else
		VectorCopy(cl.viewangles, cl.lerpangles);

	bobjrotate = anglemod (100 * cl.ctime);

// start on the entity after the world
	for (i = 1, numplayers = 0, ent = cl_entities + 1 ; i < cl.num_entities ; i++, ent++)
	{
		cbool evaluated = false;

		if (!ent->model)
		{
			// empty slot
			// MH: no efrags on these entities in GL...
			// Baker: We support WinQuake too so keep ...
			if (ent->forcelink)
				R_RemoveEfrags (ent);	// just became empty

			continue;
		}

		

// if the object wasn't included in the last packet, remove it
		if (ent->msgtime != cl.mtime[0])
		{
			ent->model = NULL;
			ent->lerpflags |= LERP_RESETMOVE|LERP_RESETANIM; //johnfitz -- next time this entity slot is reused, the lerp will need to be reset
			ent->traildrawn = false; // Hmmmm
			continue;
		}

		VectorCopy (ent->origin, oldorg);

		if (ent->forcelink)
		{
			// the entity was not updated in the last message
			// so move to the final spot
			VectorCopy (ent->msg_origins[0], ent->origin);
			VectorCopy (ent->msg_angles[0], ent->angles);
		}
		else
		{
			// if the delta is large, assume a teleport and don't lerp
			f = frac;

			for (j=0 ; j<3 ; j++)
			{
				delta[j] = ent->msg_origins[0][j] - ent->msg_origins[1][j];

				if (delta[j] > 100 || delta[j] < -100)
				{
					f = 1;		// assume a teleportation, not a motion
					ent->lerpflags |= LERP_RESETMOVE; //johnfitz -- don't lerp teleports
				}
			}

			//johnfitz -- don't cl_lerp entities that will be r_lerped
			if (r_lerpmove.value && (ent->lerpflags & LERP_MOVESTEP))
					f = 1;
			//johnfitz

		// interpolate the origin and angles
			for (j = 0; j < 3; j++)
			{
				ent->origin[j] = ent->msg_origins[1][j] + f* delta[j];

				d = ent->msg_angles[0][j] - ent->msg_angles[1][j];

				if (d > 180)
					d -= 360;
				else if (d < -180)
					d += 360;

				ent->angles[j] = ent->msg_angles[1][j] + f*d;
			}

		}  // End interpolation, etc.

#ifdef GLQUAKE_SUPPORTS_QMB
		{
			int			client_no		= ent - cl_entities;
			cbool		isPlayer		= (client_no >= 1 && client_no<=cl.maxclients);

			if (isPlayer)	// gamehack
			{
				VectorCopy (ent->origin, player_origin[numplayers]);
				numplayers++;
			}
		}

		// Explosion boxes override
		if (frame.qmb && isin2(ent->modelindex, cl_modelindex[mi_explo1], cl_modelindex[mi_explo2]) /*isin*/  ) {
			if (qmb_explosions.value && isin2(qmb_explosiontype.value, 2 /* this*/, 3))
				continue;  // software removal of sprites
		}
#endif // GLQUAKE_SUPPORTS_QMB

// rotate binary objects locally
		if (ent->model->modelflags & EF_ROTATE) {
			ent->angles[1] = bobjrotate;
			if (cl_item_bobbing.value)
				ent->origin[2] += sin(bobjrotate / 90 * M_PI) * 5 + 5;
		}

		// EF_BRIGHTFIELD is not used by original progs
		if (ent->effects & EF_BRIGHTFIELD)
			R_EntityParticles (ent);

		// EF_BRIGHTLIGHT is not used by original progs
		if (ent->effects & EF_BRIGHTLIGHT) {
			vec3_t org = {ent->origin[0], ent->origin[1], ent->origin[2] + 16};
			DLight_Add (i, org, 400 + (rand() & 31), 0, cl.time + 0.001, /*rgb: */ 1,1 /*baker1*/,1);
		}

		// EF_DIMLIGHT is for powerup glows and enforcer's laser
		if (ent->effects & EF_DIMLIGHT) {
			DLight_Add (i, ent->origin, 200 + (rand() & 31), 0, cl.time + 0.001, /*rgb: */ 1,1,1 /*baker1*/);		
		}

		if (ent->effects & EF_MUZZLEFLASH)
		{
			vec3_t		fv, rv, uv;
			vec3_t		muzzle_flash_origin;

			VectorCopy (ent->origin,  muzzle_flash_origin);
			muzzle_flash_origin[2] += 16;
			AngleVectors (ent->angles, fv, rv, uv);
			VectorMA (muzzle_flash_origin, 18, fv, muzzle_flash_origin);

			// Light Add: key......origin ........radius ............minlight .......die ..... color
			DLight_Add (i, muzzle_flash_origin, 200 + (rand() & 31), 32, cl.time + 0.1, /*rgb: */ 1 /*baker1*/,1,1);

			//johnfitz -- assume muzzle flash accompanied by muzzle flare, which looks bad when lerped
			if (r_lerpmodels.value != 2)
			{
			if (ent == &cl_entities[cl.viewentity_player])
				cl.viewent_gun.lerpflags |= LERP_RESETANIM | LERP_RESETANIM2; //no lerping for two frames
			else
				ent->lerpflags |= LERP_RESETANIM | LERP_RESETANIM2; //no lerping for two frames
			}
			//johnfitz
		}


		//if (ent->model->modelflags)
		//{
		//	if (!ent->traildrawn || !VectorL2Compare(ent->trail_origin, ent->origin, 140))
		//	{
		//		VectorCopy (ent->origin, oldorg);	//not present last frame or too far away
		//		ent->traildrawn = true;
		//	}
		//	else
		//	{
		//		VectorCopy (ent->trail_origin, oldorg);
		//	}
		//}

#ifdef GLQUAKE_SUPPORTS_QMB
		if (frame.qmb) evaluated = QMB_Effects_Evaluate (i, ent, oldorg);
#endif // GLQUAKE_SUPPORTS_QMB
		
		if (!evaluated)	evaluated = Clasic_Effects_Evaluate (i, ent, oldorg);  // What's this do, I forget.  Emits classic trails



		// Here, we are insert effects from nothing.
		// A spikes make bubbles, shambler charge effect.  Bubble inserted, what happens to the sprite?

#ifdef GLQUAKE_SUPPORTS_QMB
		if (frame.qmb)   
			if (QMB_MaybeInsertEffect (ent, oldorg, i)) // Bubble, Shambler, Spikes, Laser.  Although Bubble removed.
				continue; // We inserted an effect.  Bubble or something.
#endif // GLQUAKE_SUPPORTS_QMB

		// This should be after all the effects right?
		// In JoeQuake things after this ... chase active skip, no draw, 
		ent->forcelink = false; 

		if (ent->effects & EF_NODRAW) // Baker: Seriously.  This late in the process?  Yes.  Could be carrying effects!
			continue;

		if (i == cl.viewentity_player) {
			// If we are the player and chase_active 0, skip us
			if (!chase_active.value)
				continue;
		}

		if (cl.numvisedicts < MAX_MARK_V_VISEDICTS)
		{
			cl.visedicts[cl.numvisedicts] = ent;
			cl.numvisedicts++;
// SKIN CHECK MOVED ABOVE FOR MIRRORS
#ifdef GLQUAKE_COLORMAP_TEXTURES // Baker: We manually build the skins for GLQuake
			if (ent->coloredskin == NULL || R_SkinTextureChanged (ent))
				ent->coloredskin = R_TranslateNewModelSkinColormap (ent);
#endif // GLQUAKE_COLORMAP_TEXTURES

		}
	}
}


/*
===============
CL_ReadFromServer

Read all incoming data from the server
===============
*/
void CL_UpdateClient (double frametime, cbool readfromserver)
{
	int			ret;
	cbool	forwardtime = (!cls.demorewind || !cls.demoplayback);	// by joe

#ifdef SUPPORTS_CUTSCENE_PROTECTION
	// CL_Parse writes into 'found_server_command' if we are executing svc_stufftext
	cbool	found_server_command = false;

	cmd_from_server = false; // Reset to "not from server" until we know
#endif // SUPPORTS_CUTSCENE_PROTECTION

#if 1
	if ((cl.paused & 2) && cls.demoplayback) // Demo playback pause.  We still read new messages though ...
	{
		// Don't advance time, don't read new messages
		/*host_timeslice = */
		cl_frametime  = 0; // Don't let particles move, blends to fade, etc.
//		return;
	}
#endif


	// Advance time
	// cl.time is forward only time, cl.ctime runs backwards during demo rewinding
	cl.oldtime = cl.time;
	cl.time += frametime;
	cl.ctime = cl.ctime + (forwardtime ? frametime : - frametime) ;

		do
		{
			ret = CL_GetMessage ();

			if (ret == -1)
				Host_Error ("CL_ReadFromServer: lost server connection");

			if (!ret)
				break;

			cl.last_received_message = realtime;
	#ifdef SUPPORTS_CUTSCENE_PROTECTION
			CL_ParseServerMessage (&found_server_command);
	#endif // SUPPORTS_CUTSCENE_PROTECTION
		} while (ret && cls.state == ca_connected);

	#ifdef SUPPORTS_CUTSCENE_PROTECTION
		if (found_server_command)
			Cbuf_AddTextLinef (NEWLINE "%c", CUTSCENE_CHAR_END_6);
	#endif // SUPPORTS_CUTSCENE_PROTECTION

		if (cl_shownet.value)
			Con_PrintLine ();

#pragma message ("This stuff has to be moved out")
#pragma message ("This stuff has to be moved out")
#pragma message ("If we aren't reading from server every frame, this stuff will break ...")
	CL_RelinkEntities ();
	CL_UpdateTEnts ();

//johnfitz -- devstats
	{
		int			num_beams = 0; //johnfitz
		int			num_dlights = 0; //johnfitz
		beam_t		*b; //johnfitz
		dlight_t	*l; //johnfitz
		int			i; //johnfitz

		//visedicts
		if (cl.numvisedicts > MAX_WINQUAKE_VISEDICTS && dev_peakstats.visedicts <= MAX_WINQUAKE_VISEDICTS)
			Con_DWarningLine ("%d visedicts exceeds standard limit of %d.", cl.numvisedicts, MAX_WINQUAKE_VISEDICTS); // 256
		dev_stats.visedicts = cl.numvisedicts;
		dev_peakstats.visedicts = c_max(cl.numvisedicts, dev_peakstats.visedicts);

		//temp entities
		if (cl.num_temp_entities > 64 && dev_peakstats.tempents <= 64)
			Con_DWarningLine ("%d tempentities exceeds standard limit of 64.", cl.num_temp_entities);
		dev_stats.tempents = cl.num_temp_entities;
		dev_peakstats.tempents = c_max(cl.num_temp_entities, dev_peakstats.tempents);

		//beams
		for (i=0, b=cl.beams ; i< MAX_FITZQUAKE_BEAMS ; i++, b++)
			if (b->model && b->endtime >= cl.time)
				num_beams++;
		if (num_beams > MAX_WINQUAKE_BEAMS && dev_peakstats.beams <= MAX_WINQUAKE_BEAMS)
			Con_DWarningLine ("%d beams exceeded standard limit of %d.", num_beams, MAX_WINQUAKE_BEAMS);
		dev_stats.beams = num_beams;
		dev_peakstats.beams = c_max(num_beams, dev_peakstats.beams);

		//dlights
		for (i=0, l=cl.dlights ; i<MAX_FITZQUAKE_DLIGHTS ; i++, l++)
			if (l->die >= cl.time && l->radius)
				num_dlights++;
		if (num_dlights > MAX_WINQUAKE_DLIGHTS && dev_peakstats.dlights <= MAX_WINQUAKE_DLIGHTS)
			Con_DWarningLine ("%d dlights exceeded standard limit of %d.", num_dlights, MAX_WINQUAKE_DLIGHTS); // 32
		dev_stats.dlights = num_dlights;
		dev_peakstats.dlights = c_max(num_dlights, dev_peakstats.dlights);

	//johnfitz
	}

#ifdef SUPPORTS_AVI_CAPTURE // Baker change
	if (cls.demoplayback && cls.capturedemo /*cls.demonum == -1 && !cls.timedemo && !cls.titledemo*/)
	{
		static float olddrealtime; // Yay.  Another timer.  Sheesh.
		float timeslice = realtime - olddrealtime;
		olddrealtime = realtime;

		if (cl.paused & 2)
			timeslice = 0;

		// If we have no start cltime, fill it in now
		if (!cls.demo_cltime_start)
		{
			cls.demo_cltime_start = cl.time;
			cls.demo_cltime_elapsed = 0;
		}
		else cls.demo_cltime_elapsed += cl_frametime;

		// If we have no start hosttime, fill it in now
		if (!cls.demo_hosttime_start)
		{
			cls.demo_hosttime_start = realtime;
			cls.demo_hosttime_elapsed = 0;
		}
		else cls.demo_hosttime_elapsed += timeslice; // Advance time only if we are not paused

		while (1)
		{
			// This is the "percentage" (0 to 1) of the demoplay that has been completed.
			float completed_amount = (cls.demo_offset_current - cls.demo_offset_start)/(float)cls.demo_file_length;
			float remaining_time = 0;
			int minutes, seconds;
			char tempstring[256];


			if (vid.screen.type == MODE_FULLSCREEN)
				break; // Don't bother, we are in fullscreen mode.

			if (timeslice = 0)
				break; // Don't bother updating the caption if we are paused

			if (cls.demo_hosttime_elapsed)
				remaining_time = (cls.demo_hosttime_elapsed / completed_amount) - cls.demo_hosttime_elapsed;

			minutes = Time_Minutes((int)remaining_time);
			seconds = Time_Seconds((int)remaining_time);

			c_snprintf6 (tempstring, "Demo: %s (%3.1f%% elapsed: %4.1f secs) - Estimated Remaining %d:%02d (Capturing: %s)", cls.demo_url, completed_amount * 100, cls.demo_hosttime_elapsed, (int)minutes, (int)seconds, movie_codec);

			VID_Local_Set_Window_Caption (tempstring);
			break;
		}

	}
#endif // SUPPORTS_AVI_CAPTURE
}

/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void)
{
	usercmd_t		cmd;

	if (cls.state != ca_connected)
		return;

	if (cls.signon == SIGNONS)
	{
	// get basic movement from keyboard
		CL_BaseMove (&cmd);

	// allow mice or other external controllers to add to the move
		Input_Move (&cmd);

		// send the unreliable message
		CL_SendMove (&cmd);

	}

	if (cls.demoplayback)
	{
		SZ_Clear (&cls.message);
		return;
	}

// send the reliable message
	if (!cls.message.cursize)
		return;		// no message at all

	if (!NET_CanSendMessage (cls.netcon))
	{
		Con_DPrintLinef ("CL_SendCmd: can't send");
		return;
	}

	if (NET_SendMessage (cls.netcon, &cls.message) == -1)
		Host_Error ("CL_SendCmd: lost server connection");

	SZ_Clear (&cls.message);
}

/*
=============
CL_Tracepos_f -- johnfitz

display impact point of trace along VPN
=============
*/
void CL_Tracepos_f (void)
{
	vec3_t	v, w;

	//VectorScale(vpn, 8192.0, v);
	VectorMA(r_refdef.vieworg, 8192.0, vpn, v); // According to recent Quakespasm change
	TraceLine(r_refdef.vieworg, v, w);

	if (VectorLength (w) == 0)
		Con_PrintLinef ("Tracepos: trace didn't hit anything");
	else
		Con_PrintLinef ("Tracepos: (%d %d %d)", (int)w[0], (int)w[1], (int)w[2]);
}

/*
=============
CL_Setpos_f -- Baker

Set a position.  Previously called warppos, renamed to be same as Quakespasm's new setpos command.
Add velocity nuking like Quakespasm.
=============
*/


void CL_Setpos_f (lparse_t *line)
{
	if (!sv.active) 
		Con_PrintLinef ("Server not active");
	if (!cl.worldmodel || !sv.worldmodel) 
		Con_PrintLinef ("No map active");
	else
	{
		edict_t *player = EDICT_NUM (1);
		vec3_t origin, angles;

		if (line->count == 1)
		{
			if (!cl.stored_set)
			{
				Con_PrintLinef ("No stored point set.  Type viewpos to store a location.");
				return;
			}
			VectorCopy (cl.stored_origin, origin);
			VectorCopy (cl.stored_angles, angles);
		} 
		else if (line->count == 7)
		{
			origin[0] = atof(line->args[1]);
			origin[1] = atof(line->args[2]);
			origin[2] = atof(line->args[3]);
			angles[0] = atof(line->args[4]);
			angles[1] = atof(line->args[5]);
			angles[2] = atof(line->args[6]);
		}
		else if (line->count == 4)
		{
			origin[0] = atof(line->args[1]);
			origin[1] = atof(line->args[2]);
			origin[2] = atof(line->args[3]);
			VectorCopy (cl.viewangles, angles);
		}
		else
		{
			Con_PrintLinef ("usage:");
			Con_PrintLinef ("   setpos <x> <y> <z>");
			Con_PrintLinef ("   setpos <x> <y> <z> <pitch> <yaw> <roll>");
			return;
		}

		#if 1
			if (player->v.movetype != MOVETYPE_NOCLIP) {
				int cont = SV_PointContents (origin);
				if (cont == CONTENTS_SOLID) {
					cl.noclip_anglehack = true;
					player->v.movetype = MOVETYPE_NOCLIP;
					Con_PrintLinef ("noclip ON");
					
				}
			}
		#endif

		// Edict 1
		VectorClear  (player->v.velocity); // Quakespasm erases velocity
		VectorCopy (origin, player->v.origin);
		VectorCopy (angles, cl.viewangles);
	} 
	

}


void CL_RPos_Legacy_f (lparse_t *line)
{
	Con_PrintLinef ("Use 'scr_showpos' instead.");
}

void CL_Viewpos_f (lparse_t *line)
{
	//camera position
	Con_PrintLinef ("Camera: (%d %d %d) %d %d %d",
		(int)r_refdef.vieworg[0],
		(int)r_refdef.vieworg[1],
		(int)r_refdef.vieworg[2],
		(int)r_refdef.viewangles[PITCH],
		(int)r_refdef.viewangles[YAW],
		(int)r_refdef.viewangles[ROLL]);
	//player position
	Con_PrintLinef ("Viewpos: (%d %d %d) %d %d %d",
		(int)cl_entities[cl.viewentity_player].origin[0],
		(int)cl_entities[cl.viewentity_player].origin[1],
		(int)cl_entities[cl.viewentity_player].origin[2],
		(int)cl.viewangles[PITCH],
		(int)cl.viewangles[YAW],
		(int)cl.viewangles[ROLL]);

	VectorCopy (cl_entities[cl.viewentity_player].origin, cl.stored_origin);
	VectorCopy (cl.viewangles, cl.stored_angles);
	cl.stored_set = true;

//	if (line->count == 2 && !strcmp(line->args[1], "set"))
//	{
//#pragma message ("Write it to file somehow in a certain place that can be easily parsed")
//
//
//
//	}
}

/*
=================
CL_Init
=================
*/
void CL_Init (void)
{
	SZ_Alloc (&cls.message, 1024);

	CL_InitInput ();
#ifdef GLQUAKE_SUPPORTS_QMB
	GameHacks_InitModelnames (); // QMB
#endif // GLQUAKE_SUPPORTS_QMB
	CL_InitTEnts (); // Technically should occur at game dir change time. 

	Cmd_AddCommands (CL_Init);
}