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
// cl_tent.c -- client side temporary entities

#include "quakedef.h"


static	vec3_t	playerbeam_end; // True lightning
static sfx_t			*cl_sfx_wizhit;
static sfx_t			*cl_sfx_knighthit;
static sfx_t			*cl_sfx_tink1;
static sfx_t			*cl_sfx_ric1;
static sfx_t			*cl_sfx_ric2;
static sfx_t			*cl_sfx_ric3;
static sfx_t			*cl_sfx_r_exp3;

/*
=================
CL_InitTEnts
=================
*/
void CL_InitTEnts (void) // Should be done at gamedir change time?  Technically.
{
	cl_sfx_wizhit = S_PrecacheSound ("wizard/hit.wav", NULL);
	cl_sfx_knighthit = S_PrecacheSound ("hknight/hit.wav", NULL);
	cl_sfx_tink1 = S_PrecacheSound ("weapons/tink1.wav", NULL);
	cl_sfx_ric1 = S_PrecacheSound ("weapons/ric1.wav", NULL);
	cl_sfx_ric2 = S_PrecacheSound ("weapons/ric2.wav", NULL);
	cl_sfx_ric3 = S_PrecacheSound ("weapons/ric3.wav", NULL);
	cl_sfx_r_exp3 = S_PrecacheSound ("weapons/r_exp3.wav", NULL);
}

/*
=================
CL_ParseBeam
=================
*/
void CL_ParseBeam (qmodel_t *m)
{
	int		i, ent;
	vec3_t	start, end;
	beam_t	*b;

	ent = MSG_ReadShort ();

	start[0] = MSG_ReadCoord ();
	start[1] = MSG_ReadCoord ();
	start[2] = MSG_ReadCoord ();

	end[0] = MSG_ReadCoord ();
	end[1] = MSG_ReadCoord ();
	end[2] = MSG_ReadCoord ();
	if (ent == cl.viewentity_player)
		VectorCopy (end, playerbeam_end);	// for cl_truelightning

	Stain_AddStain(end, 13, 22); // Intense blackening, medium radius (beam LG)

// override any beam with the same entity
	for (i = 0, b = cl.beams; i < MAX_FITZQUAKE_BEAMS; i++, b++)
	{
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = m;
			b->endtime = cl.time + 0.2;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			return;
		}
	}

// find a free beam
	for (i = 0, b = cl.beams; i < MAX_FITZQUAKE_BEAMS; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
		{
			b->entity = ent;
			b->model = m;
			b->endtime = cl.time + 0.2;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			return;
		}
	}

	//johnfitz -- less spammy overflow message
	if (!dev_overflows.beams || dev_overflows.beams + CONSOLE_RESPAM_TIME < realtime )
	{
		Con_PrintLinef ("Beam list overflow!");
		dev_overflows.beams = realtime;
	}
	//johnfitz
}

#define QMB_EXPLOSION_LIGHT_LEVEL_1 1

#define	SetCommonExploStuff												\
	dl = CL_AllocDlight (0);											\
	VectorCopy (pos, dl->origin);										\
	dl->radius = 150 + 200 * CLAMP (0, QMB_EXPLOSION_LIGHT_LEVEL_1, 1);	\
	dl->die = cl.time + 0.5;											\
	dl->decay = 300

/*
=================
CL_ParseTEnt
=================
*/
void CL_ParseTEnt (void)
{
	te_effect_e		type;
	vec3_t	pos;
	dlight_t	*dl;
	int		rnd;
	int		colorStart, colorLength;
#ifdef GLQUAKE_SUPPORTS_QMB
	byte		*colorByte;
#endif // GLQUAKE_SUPPORTS_QMB

	type = MSG_ReadByte ();

	switch (type)
	{
	case TE_WIZSPIKE:			// spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_RunParticleEffect (pos, vec3_origin, COLOR_WIZSPIKE_20, WIZSPIKE_COUNT_30, TE_EF_WIZSPIKE);
		Stain_AddStain(pos, 2, 20); // Minimal darkening, normal radius
		S_StartSound (-1, 0, cl_sfx_wizhit, pos, 1, 1);
		break;

	case TE_KNIGHTSPIKE:			// spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_RunParticleEffect (pos, vec3_origin, COLOR_KNIGHTSPIKE_226, KNIGHTSPIKE_COUNT_20, TE_EF_KNIGHTSPIKE);
		Stain_AddStain(pos, 2, 20); // Minimal darkening, normal radius
		S_StartSound (-1, 0, cl_sfx_knighthit, pos, 1, 1);
		break;

	case TE_SPIKE:			// spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

// joe: they put the ventillator's wind effect to "10" in Nehahra. sigh.
#ifdef SUPPORTS_NEHAHRA
		if (nehahra_active)
			R_RunParticleEffect (pos, vec3_origin, 0, VENTILLIATION_WIND_COUNT_10, TE_EF_VENTILLIATION);
		else
#endif // SUPPORTS_NEHAHRA
			R_RunParticleEffect (pos, vec3_origin, 0, NAIL_SPIKE_COUNT_9, TE_EF_SPIKE);
		Stain_AddStain(pos, 2, 20); // Minimal darkening, normal radius

		if ( rand() % 5 )
			S_StartSound (-1, 0, cl_sfx_tink1, pos, 1, 1);
		else
		{
			rnd = rand() & 3;

			if (rnd == 1)
				S_StartSound (-1, 0, cl_sfx_ric1, pos, 1, 1);
			else if (rnd == 2)
				S_StartSound (-1, 0, cl_sfx_ric2, pos, 1, 1);
			else
				S_StartSound (-1, 0, cl_sfx_ric3, pos, 1, 1);
		}

		break;

	case TE_SUPERSPIKE:			// super spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_RunParticleEffect (pos, vec3_origin, 0, SUPER_SPIKE_AND_BULLETS_COUNT_20, TE_EF_SUPERSPIKE);
		Stain_AddStain(pos, 3, 30); // Moderate darkening, moderate radius

		if ( rand() % 5 )
			S_StartSound (-1, 0, cl_sfx_tink1, pos, 1, 1);
		else
		{
			rnd = rand() & 3;

			if (rnd == 1)
				S_StartSound (-1, 0, cl_sfx_ric1, pos, 1, 1);
			else if (rnd == 2)
				S_StartSound (-1, 0, cl_sfx_ric2, pos, 1, 1);
			else
				S_StartSound (-1, 0, cl_sfx_ric3, pos, 1, 1);
		}
		break;

	case TE_GUNSHOT:			// bullet hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_RunParticleEffect (pos, vec3_origin, 0, GUNSHOT_COUNT_21, TE_EF_GUNSHOT); // was 20
		Stain_AddStain(pos, 2, 20); // Minimal darkening, normal radius
		break;

	case TE_EXPLOSION:			// rocket explosion
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_ParticleExplosion (pos); // I guess we hit this twice for QMB??
		

		if (frame.qmb && qmb_explosiontype.value == 3)
			R_RunParticleEffect (pos, vec3_origin, COLOR_EXPLOSION_BLOOD_225, 50, TE_EF_EXPLOSION_1_3); // I am QMB, explosion type 3
//			RunParticleEffect(blood, org, dir, color, count);
		else
			R_ParticleExplosion (pos); // Classic or QMB except 3

		SetCommonExploStuff;
#ifdef GLQUAKE_RENDERER_SUPPORT	
		if (frame.qmb) {
			//dl->color = QMB_GetDlightColor (qmb_explosionlightcolor.value, lt_explosion, true /*random!*/);
			dl->color = QMB_GetDlightColor (qmb_explosionlightcolor.value, lt_explosion, false /*random!*/);
			dl->radius /= 2; // Why????
		}
		else
			dl->color.vec3[0] = dl->color.vec3[1] = dl->color.vec3[2] = 1; //johnfitz -- lit support via lordhavoc
#endif // GLQUAKE_RENDERER_SUPPORT	

		Stain_AddStain(pos, 5, 50); // Fair darkening, large radius
		S_StartSound (-1, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;

	case TE_TAREXPLOSION:			// tarbaby explosion
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_BlobExplosion (pos);
		Stain_AddStain(pos, 3, 30); // Moderate darkening, moderate radius
		S_StartSound (-1, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;

	case TE_LIGHTNING1:				// lightning bolts
		CL_ParseBeam (Mod_ForName("progs/bolt.mdl", true));
		break;

	case TE_LIGHTNING2:				// lightning bolts
		CL_ParseBeam (Mod_ForName("progs/bolt2.mdl", true));
		break;

	case TE_LIGHTNING3:				// lightning bolts
		CL_ParseBeam (Mod_ForName("progs/bolt3.mdl", true));
		break;

#ifdef SUPPORTS_NEHAHRA
	// nehahra support
	case TE_LIGHTNING4:				// lightning bolts
        CL_ParseBeam (Mod_ForName(MSG_ReadString(), true));
		break;
#endif // SUPPORTS_NEHAHRA

// PGM 01/21/97
	case TE_BEAM:				// grappling hook beam
		CL_ParseBeam (Mod_ForName("progs/beam.mdl", true));
		break;
// PGM 01/21/97

	case TE_LAVASPLASH:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_LavaSplash (pos);
		break;

	case TE_TELEPORT:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_TeleportSplash (pos);
		break;

	case TE_EXPLOSION2:				// color mapped explosion
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		colorStart = MSG_ReadByte ();
		colorLength = MSG_ReadByte ();

		if (frame.qmb && qmb_explosiontype.value == 3)
			//R_RunParticleEffect (pos, vec3_origin, NEHAHRA_SPECIAL_MSGCOUNT_MAYBE_255, 50, TE_EF_EXPLOSION2);
			R_ParticleExplosion (pos); // Seriously?  Can this even happen in software?
		else
			R_ColorMappedExplosion (pos, colorStart, colorLength); // R_ParticleExplosion2

		// If we are using QMB then check if we are using explosions, otherwise we are.
		if (!frame.qmb || !qmb_explosions.value) {
			SetCommonExploStuff;
			/*dl = CL_AllocDlight (0);
			VectorCopy (pos, dl->origin);
			dl->radius = 350;
			dl->die = cl.time + 0.5;
			dl->decay = 300;*/
		}	

		Stain_AddStain(pos, 5, 50); // Intense darkening, intense radius
		S_StartSound (-1, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;

#ifdef SUPPORTS_NEHAHRA
	// nehahra support
	case TE_EXPLOSION3:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_ParticleExplosion (pos);

		SetCommonExploStuff;
/*
		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = 350;
		dl->die = cl.time + 0.5;
		dl->decay = 300;*/

#ifdef GLQUAKE_COLORED_LIGHTS
        dl->color.vec3[0] = MSG_ReadCoord () / 2.0;
		dl->color.vec3[1] = MSG_ReadCoord () / 2.0;
		dl->color.vec3[2] = MSG_ReadCoord () / 2.0;
#else
		// Ignore the colors
		MSG_ReadCoord ();
		MSG_ReadCoord ();
		MSG_ReadCoord ();
#endif // !GLQUAKE_COLORED_LIGHTS

		Stain_AddStain(pos, 5, 50); // Intense darkening, intense radius
		S_StartSound (-1, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;
#endif // SUPPORTS_NEHAHRA

	default:
		Host_Error ("CL_ParseTEnt: bad type");
	}
}


/*
=================
CL_NewTempEntity
=================
*/
static entity_t *CL_NewTempEntity (void)
{
	entity_t	*ent;

	if (cl.numvisedicts == MAX_MARK_V_VISEDICTS)
		return NULL;

	if (cl.num_temp_entities == MAX_FITZQUAKE_TEMP_ENTITIES)
		return NULL;

	ent = &cl.temp_entities[cl.num_temp_entities];
	memset (ent, 0, sizeof(*ent));
	cl.num_temp_entities++;
	cl.visedicts[cl.numvisedicts] = ent;
	cl.numvisedicts++;

#ifdef WINQUAKE_COLORMAP_TRANSLATION
// Baker: WinQuake only ... our default is 0 so no need for GL colored bodies
	ent->colormap = vid.colormap;
#endif // WINQUAKE_COLORMAP_TRANSLATION
	return ent;
}


/*
=================
CL_UpdateTEnts
=================
*/
void CL_UpdateTEnts (void)
{
	vec3_t		dist, org;
	float		d;
	entity_t	*ent;
	float		yaw, pitch;
	float		forward;
	int			beamnum;

#ifdef GLQUAKE_SUPPORTS_QMB
	vec3_t		beamstart, beamend;
	cbool		sparks = false;
	int			j;
#endif  // GLQUAKE_SUPPORTS_QMB

	cl.num_temp_entities = 0;

	srand ((int) (cl.ctime * 1000)); //johnfitz -- freeze beams when paused

// update lightning
	for (beamnum = 0; beamnum < MAX_FITZQUAKE_BEAMS ; beamnum ++)
	{
		beam_t *b = &cl.beams[beamnum];
		if (!b->model || b->endtime < cl.time) continue;

	// if coming from the player, update the start position
		if (b->entity == cl.viewentity_player) {
			VectorCopy (cl_entities[cl.viewentity_player].origin, b->start);

			if (cl_truelightning.value)
			{
				vec3_t	forward, v, org, ang;
				float	f, delta;
				trace_t	trace;

				// joe: using koval's [sons]Quake code
				//b->start[2] += 16; // Baker //cl.crouch;  16 is bad!

				f = c_max(0, c_min(1, cl_truelightning.value));

				VectorSubtract (playerbeam_end, cl_entities[cl.viewentity_player].origin, v);
				v[2] -= 22;		// adjust for view height
				VectorToAngles (v, ang); // vectoangles

				// lerp pitch
				ang[0] = -ang[0];
				if (ang[0] < -180)
					ang[0] += 360;
				ang[0] += (cl.viewangles[0] - ang[0]) * f;

				// lerp yaw
				delta = cl.viewangles[1] - ang[1];
				if (delta > 180)
					delta -= 360;
				if (delta < -180)
					delta += 360;
				ang[1] += delta * f;
				ang[2] = 0;

				AngleVectors (ang, forward, NULL, NULL);
				VectorScale (forward, 600, forward);
				VectorCopy (cl_entities[cl.viewentity_player].origin, org);
				org[2] += 16;
				VectorAdd(org, forward, b->end);

				memset (&trace, 0, sizeof(trace_t));
				if (!SV_RecursiveHullCheck(cl.worldmodel->hulls, 0, 0, 1, org, b->end, &trace))
					VectorCopy (trace.endpos, b->end);
			}
			// End of if view entity is player
		}

	// calculate pitch and yaw
		VectorSubtract (b->end, b->start, dist);

		if (dist[1] == 0 && dist[0] == 0)
		{
			// linear so pythagoras can have his coffee break
			yaw = 0;

			if (dist[2] > 0)
				pitch = 90;
			else pitch = 270;
		}
		else
		{
			yaw = (int) (atan2 (dist[1], dist[0]) * 180 / M_PI);

			if (yaw < 0) yaw += 360;

			forward = sqrt (dist[0]*dist[0] + dist[1]*dist[1]);
			pitch = (int) (atan2 (dist[2], forward) * 180 / M_PI);

			if (pitch < 0) pitch += 360;
		}

	// add new entities for the lightning
		VectorCopy (b->start, org);

#ifdef GLQUAKE_SUPPORTS_QMB
		VectorCopy (b->start, beamstart);
#endif // !GLQUAKE_SUPPORTS_QMB

		d = VectorNormalize(dist);
	VectorScale (dist, 30, dist);

		// Baker: Apparently for every 30 units we do a new beam segment
		for ( ; d > 0 ; d -= 30)
		{
#ifdef GLQUAKE_SUPPORTS_QMB
			
			if (frame.qmb && qmb_lightning.value)
			{
				VectorAdd(org, dist, beamend);
				for (j=0 ; j<3 ; j++)
					beamend[j] += (b->entity != cl.viewentity_player) ? (rand() % 40) - 20 : (rand() % 10) - 5; // Better.
					//beamend[j] += (b->entity != cl.viewentity_player) ? (rand() % 40) - 20 : (rand() % 16) - 8;
//					beamend[j] += (b->entity != cl.viewentity_player) ? (rand() % 40) - 20 : 0;
//				for (j = 0 ; j < 3 ; j++)
//					beamend[j] += (rand() % 10) - 5;

				QMB_LightningBeam (beamstart, beamend);
				VectorCopy (beamend, beamstart);
			}
			else
#endif // GLQUAKE_SUPPORTS_QMB
			{
				ent = CL_NewTempEntity ();
				if (!ent)
					return;
	
				VectorCopy (org, ent->origin);
				ent->model = b->model;
				ent->angles[0] = pitch;
				ent->angles[1] = yaw;
				ent->angles[2] = rand()%360;
			}

			// SPARKS GO HERE

			// Baker: VectorMA replaces for loop
			//VectorMA (org, 30, dist, org);
			VectorAdd (org, dist, org);
//			VectorCopy (org, ent->origin); // You would think, but the origin Quake loop isn't written like that

		} // End of while
	}
}
