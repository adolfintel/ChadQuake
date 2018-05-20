/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
Copyright (C) 2010-2011 O. Sezer <sezero@users.sourceforge.net>
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

// snd_dma.c -- main control for any streaming sound output device

#include "quakedef.h"

#ifdef DIRECT_SOUND_QUAKE
#include "winquake.h"
#endif // DIRECT_SOUND_QUAKE

static void S_Update_(void);
void S_StopAllSounds(cbool clear);

// =======================================================================
// Internal sound data & structures
// =======================================================================

channel_t   snd_channels[MAX_CHANNELS];
int			total_channels;

int snd_blocked = 0; // Extern!
static cbool	snd_ambient = 1; // Baker to shut them off.
static cbool	snd_initialized = false;

// pointer should go away
volatile dma_t *shm = NULL;
volatile dma_t sn;

vec3_t		listener_origin;
vec3_t		listener_forward;
vec3_t		listener_right;
vec3_t		listener_up;

//#define		sound_nominal_clip_dist 1000.0			Moved to q_sound.h
//#define		deathmatch_sound_nominal_clip_dist 1500.0  Moved q_sound.h
#pragma message ("Get deathmatch sound_nominal_clip_dist working so on parity for online play.  Your multimap tourney mod idea has merit.  Place tourney maps 1500 apart to avoid sound conflicts?")

int			soundtime;		// sample PAIRS
int   		paintedtime; 	// sample PAIRS



static sfx_t *known_sfx = NULL; // hunk allocated [MAX_SFX]
static int num_sfx;

static sfx_t *ambient_sfx[NUM_AMBIENTS];

int desired_speed = 11025;
int desired_bits = 16;

static cbool sound_started = false;



// ====================================================================
// User-setable variables
// ====================================================================


//
// Fake dma is a synchronous faking of the DMA progress used for
// isolating performance in the renderer.  The fakedma_updates is
// number of times S_Update() is called per second.
//

cbool fakedma = false;
int fakedma_updates = 15;


void S_AmbientOff (void)
{
	snd_ambient = false;
}


void S_AmbientOn (void)
{
	snd_ambient = true;
}


void S_SoundInfo_f (void)
{
	if (!sound_started || !shm)
	{
		Con_PrintLinef ("sound system not started");
		return;
	}

	Con_SafePrintLinef ("%d bit, %s, %d Hz", shm->samplebits,
				(shm->channels == 2) ? "stereo" : "mono", shm->speed);
    Con_SafePrintLinef ("%5d samples", shm->samples);
    Con_SafePrintLinef ("%5d samplepos", shm->samplepos);
    Con_SafePrintLinef ("%5d samplebits", shm->samplebits);
    Con_SafePrintLinef ("%5d submission_chunk", shm->submission_chunk);
    Con_SafePrintLinef ("%5d speed", shm->speed);
    Con_SafePrintLinef ("%p dma buffer", shm->buffer);
	Con_SafePrintLinef ("%5d total_channels", total_channels);
}


/*
================
S_Startup
================
*/

void S_Startup (void)
{
	int		rc;

	if (!snd_initialized)
		return;

	if (!fakedma)
	{
		rc = SNDDMA_Init();

		if (!rc)
		{
#ifndef	DIRECT_SOUND_QUAKE
			Con_PrintLinef ("S_Startup: SNDDMA_Init failed.");
#endif // !DIRECT_SOUND_QUAKE
			sound_started = 0;
			return;
		}
	}

	Con_SafePrintLinef ("Audio: %d bit, %s, %d Hz",
				shm->samplebits,
				(shm->channels == 2) ? "stereo" : "mono",
				shm->speed);

	sound_started = 1;
}

cbool SND_Read_Early_Cvars (void)
{
	// Any of these found and we bail
	char *sound_override_commandline_params[] = {"-sndspeed", NULL };

	const cvar_t *cvarslist[] = {&sndspeed, NULL};
	cbool found_in_config, found_in_autoexec;
	int i;

	for (i = 0; sound_override_commandline_params[i]; i++)
		if (COM_CheckParm (sound_override_commandline_params[i]))
			return false;

	found_in_config = Read_Early_Cvars_For_File (CONFIG_CFG, cvarslist);
	found_in_autoexec = Read_Early_Cvars_For_File (AUTOEXEC_CFG, cvarslist);

	return (found_in_config || found_in_autoexec);
}

void S_Snd_Speed_Notify_f (cvar_t *var)
{
	if (host_post_initialized) // Too late, remember this reads early in SND_Read_Early_Cvars
	{
		Con_PrintLinef ("sndspeed changed. takes effect on engine restart.");
		Con_PrintLinef ("values: 11025, 22050, 44100");
	}
}

/*
================
S_Init
================
*/
int sound_rate_hz; // Hello global
void S_Init (void)
{
	if (COM_CheckParm("-nosound"))
		return;

	Cmd_AddCommands (S_Init);
	SND_Read_Early_Cvars ();

	do
	{
		int argnum = COM_CheckParm("-sndspeed");
		int sound_requested_hz;

		if (!argnum || argnum + 1 >= com_argc)
			break; // Command line param not found

		sound_requested_hz = atoi (com_argv[ argnum + 1]);

		if (sound_requested_hz == 44100 || sound_requested_hz == 22050 || sound_requested_hz == 11025)
			Cvar_SetValueQuick (&sndspeed, (float)sound_requested_hz);

	} while (0);

	if (!isin3 (sndspeed.value, 44100, 22050, 11025))
		Cvar_SetValueQuick (&sndspeed, 11025);

	sound_rate_hz = sndspeed.value;

#ifdef CORE_SDL_STATIC // SDL static linkage workaround
	sound_rate_hz = 44100;
	Con_SafePrintLinef ("Sound forced to 44100 Hz due to SDL + static linkage Windows sound issue");
#endif // CORE_SDL + !_DEBUG // SDL static linkage workaround

	if (COM_CheckParm("-simsound")) {
		fakedma = true;
		sound_rate_hz = 22050; // Baker Jan 2017 - Is good enough?
	}

	Con_SafePrintLine ();
	Con_SafePrintLinef ("Sound Initialization");

	SND_InitScaletable ();

	known_sfx = (sfx_t *)Hunk_AllocName (MAX_SFX*sizeof(sfx_t), "sfx_t");
	num_sfx = 0;

	snd_initialized = true;

	S_Startup ();

// create a piece of DMA memory

	if (fakedma)
	{
		shm = (void *) Hunk_AllocName(sizeof(*shm), "shm");
		shm->splitbuffer = 0;
		shm->samplebits = 16;
		shm->speed = sound_rate_hz; // 22050;
		shm->channels = 2;
		shm->samples = 32768;
		shm->samplepos = 0;
		shm->soundalive = true;
		shm->gamealive = true;
		shm->submission_chunk = 1;
		shm->buffer = Hunk_AllocName(1<<16, "shmbuf");
	}

    if (shm)
    {
		Con_SafePrintLinef ("Sound sampling rate: %d", shm->speed);
    }

	// provides a tick sound until washed clean
	//	if (shm->buffer)
	//		shm->buffer[4] = shm->buffer[5] = 0x7f;	// force a pop for debugging

	ambient_sfx[AMBIENT_WATER] = S_PrecacheSound ("ambience/water1.wav", NULL); // Technically this should occur at gamedir change?
	ambient_sfx[AMBIENT_SKY] = S_PrecacheSound ("ambience/wind2.wav", NULL);

	S_StopAllSounds (true);
}


// =======================================================================
// Shutdown sound engine
// =======================================================================

void S_Shutdown(void)
{
	if (!sound_started)
		return;

	if (shm)
		shm->gamealive = 0;

	shm = 0;
	sound_started = 0;

	if (!fakedma)
	{
		SNDDMA_Shutdown();
	}
}


// =======================================================================
// Load a sound
// =======================================================================

/*
==================
S_FindName

==================
*/
static sfx_t *S_FindName (const char *name)
{
	int		i;
	sfx_t	*sfx;

	if (!name)
		System_Error ("S_FindName: NULL");

	if (strlen(name) >= MAX_QPATH_64)
		System_Error ("Sound name too long: %s", name);

// see if already loaded
	for (i=0 ; i < num_sfx ; i++)
	{
		if (!strcmp(known_sfx[i].name, name))
		{
			return &known_sfx[i];
		}
	}

	if (num_sfx == MAX_SFX)
		System_Error ("S_FindName: out of sfx_t");

	sfx = &known_sfx[i];
	c_strlcpy (sfx->name, name);

	num_sfx++;

	return sfx;
}


/*
==================
S_TouchSound

==================
*/
void S_TouchSound (const char *name)
{
	sfx_t	*sfx;

	if (!sound_started)
		return;

	sfx = S_FindName (name);
	Cache_Check (&sfx->cache);
}

/*
==================
S_PrecacheSound

==================
*/
sfx_t *S_PrecacheSound (const char *name, cbool *precached_ok)
{
	sfx_t	*sfx;

	if (!sound_started || nosound.value)
		return NULL;

	sfx = S_FindName (name);

// cache it in
	if (precache.value)
	{
		if (S_LoadSound (sfx) == NULL)
			if (precached_ok)
				*precached_ok = false;
	}
	return sfx;
}

cbool S_PrecacheSound_Again (sfx_t *sfx)
{
	if (S_LoadSound (sfx))
		return true;

	return false;

}


//=============================================================================

/*
=================
SND_PickChannel

picks a channel based on priorities, empty slots, number of channels
=================
*/
channel_t *SND_PickChannel(int entnum, int entchannel)
{
    int ch_idx;
    int first_to_die;
    int life_left;

// Check for replacement sound, or find the best one to replace
    first_to_die = -1;
    life_left = 0x7fffffff;

    for (ch_idx=NUM_AMBIENTS ; ch_idx < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS ; ch_idx++)
    {
		if (entchannel != 0		// channel 0 never overrides
		&& snd_channels[ch_idx].entnum == entnum
		&& (snd_channels[ch_idx].entchannel == entchannel || entchannel == -1) )
		{	// always override sound from same entity
			first_to_die = ch_idx;
			break;
		}

		// don't let monster sounds override player sounds
		if (snd_channels[ch_idx].entnum == cl.viewentity_player && entnum != cl.viewentity_player && snd_channels[ch_idx].sfx)
			continue;

		if (snd_channels[ch_idx].end - paintedtime < life_left)
		{
			life_left = snd_channels[ch_idx].end - paintedtime;
			first_to_die = ch_idx;
		}
   }

	if (first_to_die == -1)
		return NULL;

	if (snd_channels[first_to_die].sfx)
		snd_channels[first_to_die].sfx = NULL;

    return &snd_channels[first_to_die];
}

/*
=================
SND_Spatialize

spatializes a channel
=================
*/
void SND_Spatialize(channel_t *ch)
{
    vec_t dot;
	vec_t	dist;
    vec_t lscale, rscale, scale;
    vec3_t source_vec;

// anything coming from the view entity will always be full volume
	if (ch->entnum == cl.viewentity_player)
	{
		ch->leftvol = ch->master_vol;
		ch->rightvol = ch->master_vol;
		return;
	}

// calculate stereo separation and distance attenuation
	VectorSubtract(ch->origin, listener_origin, source_vec);
	dist = VectorNormalize(source_vec) * ch->dist_mult;
	dot = DotProduct(listener_right, source_vec);

	if (shm->channels == 1)
	{
		rscale = 1.0;
		lscale = 1.0;
	}
	else
	{
		rscale = 1.0 + dot;
		lscale = 1.0 - dot;
	}

// add in distance effect
	scale = (1.0 - dist) * rscale;
	ch->rightvol = (int) (ch->master_vol * scale);

	if (ch->rightvol < 0)
		ch->rightvol = 0;

	scale = (1.0 - dist) * lscale;
	ch->leftvol = (int) (ch->master_vol * scale);

	if (ch->leftvol < 0)
		ch->leftvol = 0;
}


// =======================================================================
// Start a sound effect
// =======================================================================

void S_StartSound (int entnum, int entchannel, sfx_t *sfx, const vec3_t origin, float fvol, float attenuation)
{
	channel_t *target_chan, *check;
	sfxcache_t	*sc;
	int		ch_idx;
	int		skip;

	if (!sound_started)
		return;

	if (!sfx)
		return;

	if (nosound.value)
		return;

// pick a channel to play on
	target_chan = SND_PickChannel(entnum, entchannel);
	if (!target_chan)
		return;

// spatialize
	memset (target_chan, 0, sizeof(*target_chan));
	VectorCopy(origin, target_chan->origin);
	target_chan->dist_mult = attenuation / level.sound_nominal_clip_dist /*sound_nominal_clip_dist*/;
	target_chan->master_vol = (int) (fvol * 255);
	target_chan->entnum = entnum;
	target_chan->entchannel = entchannel;
	SND_Spatialize(target_chan);

	if (!target_chan->leftvol && !target_chan->rightvol)
		return;		// not audible at all

// new channel
	sc = S_LoadSound (sfx);

	if (!sc)
	{
		target_chan->sfx = NULL;
		return;		// couldn't load the sound's data
	}

	target_chan->sfx = sfx;
	target_chan->pos = 0.0;
    target_chan->end = paintedtime + sc->length;

// if an identical sound has also been started this frame, offset the pos
// a bit to keep it from just making the first one louder
	check = &snd_channels[NUM_AMBIENTS];
    for (ch_idx=NUM_AMBIENTS ; ch_idx < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS ; ch_idx++, check++)
    {
		if (check == target_chan)
			continue;

		if (check->sfx == sfx && !check->pos)
		{
			/*
			skip = rand () % (int)(0.1*shm->speed);

			if (skip >= target_chan->end)
				skip = target_chan->end - 1;
			*/
			/* LordHavoc: fixed skip calculations */
			skip = 0.1 * shm->speed; /* 0.1 * sc->speed */
			if (skip > sc->length)
				skip = sc->length;
			if (skip > 0)
				skip = rand() % skip;
			target_chan->pos += skip;
			target_chan->end -= skip;
			break;
		}
	}
}

void S_StopSound(int entnum, int entchannel)
{
	int i;

	for (i=0 ; i<MAX_DYNAMIC_CHANNELS ; i++)
	{
		if (snd_channels[i].entnum == entnum
			&& snd_channels[i].entchannel == entchannel)
		{
			snd_channels[i].end = 0;
			snd_channels[i].sfx = NULL;
			return;
		}
	}
}

void S_StopAllSounds(cbool clear)
{
	int		i;

	if (!sound_started)
		return;

	total_channels = MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS;	// no statics

	for (i=0 ; i<MAX_CHANNELS ; i++)
	{
		if (snd_channels[i].sfx)
			snd_channels[i].sfx = NULL;
	}

	memset(snd_channels, 0, MAX_CHANNELS * sizeof(channel_t));

	if (clear)
		S_ClearBuffer ();
}

void S_StopAllSoundsC (void)
{
	S_StopAllSounds (true);
}

void S_ClearBuffer (void)
{
	int		clear;

#ifdef DIRECT_SOUND_QUAKE
	if (!sound_started || !shm || (!shm->buffer && !pDSBuf))
#else
	if (!sound_started || !shm || !shm->buffer)
#endif
		return;

	if (shm->samplebits == 8)
		clear = 0x80;
	else
		clear = 0;

#ifdef DIRECT_SOUND_QUAKE
	if (pDSBuf)
	{
		DWORD	dwSize;
		DWORD	*pData;
		int		reps;
		HRESULT	hresult;

		reps = 0;

		while ((hresult = IDirectSoundBuffer_Lock (pDSBuf, 0, gSndBufSize, &pData, &dwSize, NULL, NULL, 0)) != DS_OK)
		{
			if (hresult != DSERR_BUFFERLOST)
			{
				Con_PrintLinef ("S_ClearBuffer: DS::Lock Sound Buffer Failed");
				S_Shutdown ();
				return;
			}

			if (++reps > 10000)
			{
				Con_PrintLinef ("S_ClearBuffer: DS: couldn't restore buffer");
				S_Shutdown ();
				return;
			}
		}

		memset(pData, clear, shm->samples * shm->samplebits/8);

		IDirectSoundBuffer_Unlock (pDSBuf, pData, dwSize, NULL, 0);

	}
	else
#endif
	{
		memset(shm->buffer, clear, shm->samples * shm->samplebits/8);
	}
}


/*
=================
S_StaticSound
=================
*/
void S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation)
{
	channel_t	*ss;
	sfxcache_t		*sc;

	if (!sfx)
		return;

	if (total_channels == MAX_CHANNELS)
	{
		Con_PrintLinef ("total_channels == MAX_CHANNELS");
		return;
	}

	ss = &snd_channels[total_channels];
	total_channels++;

	sc = S_LoadSound (sfx);

	if (!sc)
		return;

	if (sc->loopstart == -1)
	{
		Con_PrintLinef ("Sound %s not looped", sfx->name);
		return;
	}

	ss->sfx = sfx;
	VectorCopy (origin, ss->origin);
	ss->master_vol = (int)vol;
	ss->dist_mult = (attenuation/64) / level.sound_nominal_clip_dist; //sound_nominal_clip_dist;
    ss->end = paintedtime + sc->length;

	SND_Spatialize (ss);
}


//=============================================================================

/*
===================
S_UpdateAmbientSounds
===================
*/
static void S_UpdateAmbientSounds (void)
{
	mleaf_t		*l;
	int		vol, ambient_channel;
	channel_t	*chan;

	if (!snd_ambient)
		return;

// no ambients when disconnected
	if (cls.state != ca_connected || cls.signon != SIGNONS) // Spike added: || cls.signon != SIGNONS
		return;

// calc ambient sound levels
	if (!cl.worldmodel || cl.worldmodel->needload) // Spike added: || cl.worldmodel->needload
		return;

	l = Mod_PointInLeaf (listener_origin, cl.worldmodel);

	if (!l || !ambient_level.value)
	{
		for (ambient_channel = 0 ; ambient_channel< NUM_AMBIENTS ; ambient_channel++)
			snd_channels[ambient_channel].sfx = NULL;

		return;
	}

	for (ambient_channel = 0 ; ambient_channel< NUM_AMBIENTS ; ambient_channel++)
	{
		chan = &snd_channels[ambient_channel];
		chan->sfx = ambient_sfx[ambient_channel];

		vol = (int) (ambient_level.value * l->ambient_sound_level[ambient_channel]);

		if (vol < 8)
			vol = 0;

	// don't adjust volume too fast
		if (chan->master_vol < vol)
		{
			chan->master_vol +=  (s_frametime * ambient_fade.value); // Baker: Removed (int), tends to round down to zero.
																		// I think MH's solve had something to do with doing sound time differently.
			if (chan->master_vol > vol)
				chan->master_vol = vol;
		}
		else if (chan->master_vol > vol)
		{
			chan->master_vol -= (s_frametime * ambient_fade.value); // Baker: Removed (int), tends to round down to 0.

			if (chan->master_vol < vol)
				chan->master_vol = vol;
		}

		chan->leftvol = chan->rightvol = chan->master_vol;
	}
}


/*
============
S_Update

Called once each time through the main loop
============
*/
void S_Update (const vec3_t origin, const vec3_t forward, const vec3_t right, const vec3_t up)
{
	int			i, j;
	int			total;
	channel_t	*ch;
	channel_t	*combine;

	if (!sound_started || (snd_blocked > 0))
		return;

	VectorCopy(origin, listener_origin);
	VectorCopy(forward, listener_forward);
	VectorCopy(right, listener_right);
	VectorCopy(up, listener_up);

// update general area ambient sound sources
	S_UpdateAmbientSounds ();

	combine = NULL;

// update spatialization for static and dynamic sounds
	ch = snd_channels + NUM_AMBIENTS;

	for (i=NUM_AMBIENTS ; i<total_channels; i++, ch++)
	{
		if (!ch->sfx)
			continue;

		SND_Spatialize(ch);         // respatialize channel

		if (!ch->leftvol && !ch->rightvol)
			continue;

	// try to combine static sounds with a previous channel of the same
	// sound effect so we don't mix five torches every frame

		if (i >= MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS)
		{
		// see if it can just use the last one
			if (combine && combine->sfx == ch->sfx)
			{
				combine->leftvol += ch->leftvol;
				combine->rightvol += ch->rightvol;
				ch->leftvol = ch->rightvol = 0;
				continue;
			}

		// search for one
			combine = snd_channels + MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS;
			for (j=MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS ; j<i; j++, combine++)
			{
				if (combine->sfx == ch->sfx)
					break;
			}

			if (j == total_channels)
			{
				combine = NULL;
			}
			else
			{
				if (combine != ch)
				{
					combine->leftvol += ch->leftvol;
					combine->rightvol += ch->rightvol;
					ch->leftvol = ch->rightvol = 0;
				}

				continue;
			}
		}
	}

//
// debugging output
//
	if (snd_show.value)
	{
		total = 0;
		ch = snd_channels;

		for (i=0 ; i<total_channels; i++, ch++)
		{
			if (ch->sfx && (ch->leftvol || ch->rightvol) )
			{
				//Con_PrintLinef ("%3d %3d %s", ch->leftvol, ch->rightvol, ch->sfx->name);
				total++;
			}
		}

		Con_PrintLinef ("----(%d)----", total);
	}

// mix some sound
	S_Update_();
}

static void GetSoundtime(void)
{
	int		samplepos;
	static	int		buffers;
	static	int		oldsamplepos;
	int		fullsamples;

#ifdef SUPPORTS_AVI_CAPTURE
	if (Movie_GetSoundtime())
		return;
#endif
	fullsamples = shm->samples / shm->channels;

// it is possible to miscount buffers if it has wrapped twice between
// calls to S_Update.  Oh well.
	samplepos = SNDDMA_GetDMAPos();


	if (samplepos < oldsamplepos)
	{
		buffers++;					// buffer wrapped

		if (paintedtime > 0x40000000)
		{
			// time to chop things off to avoid 32 bit limits
			buffers = 0;
			paintedtime = fullsamples;
			S_StopAllSounds (true);
		}
	}

	oldsamplepos = samplepos;

	soundtime = buffers*fullsamples + samplepos/shm->channels;
}

void S_ExtraUpdate (void)
{
#ifdef SUPPORTS_AVI_CAPTURE // Baker change
	if (Movie_IsActive())
		return;
#endif // Baker change -

// Baker: If a frame is really slow, this will recenter the mouse again to allow for more movement
// Versus a low frame rate potentially allowing a user to hit the mouse clip region boundary lock
// So this alleviates that.

#ifdef _WIN32
	Input_Mouse_Accumulate ();  // Can SDL deal with this?  We'll find out.  I think not.
#endif // Baker: OS X everything occurs once per frame

	if (snd_noextraupdate.value)
		return;		// don't pollute timings

	S_Update_();
}

static void S_Update_(void)
{
	unsigned int    endtime;
	int				samps;

	if (!sound_started || (snd_blocked > 0))
		return;

// Updates DMA time
	GetSoundtime();

// check to make sure that we haven't overshot
	if (paintedtime < soundtime)
	{
		//Con_PrintLinef ("S_Update_ : overflow");
		paintedtime = soundtime;
	}

// mix ahead of current position
	endtime = soundtime + (unsigned int)(_snd_mixahead.value * shm->speed);
	samps = shm->samples >> (shm->channels-1);
	endtime = c_min(endtime, (unsigned int)(soundtime + samps));

#ifdef DIRECT_SOUND_QUAKE
// if the buffer was lost or stopped, restore it and/or restart it
	{
		DWORD	dwStatus;

		if (pDSBuf)
		{
			if (IDirectSoundBuffer_GetStatus (pDSBuf, &dwStatus) != S_OK)
				Con_PrintLinef ("Couldn't get sound buffer status");

			if (dwStatus & DSBSTATUS_BUFFERLOST)
				IDirectSoundBuffer_Restore (pDSBuf);

			if (!(dwStatus & DSBSTATUS_PLAYING))
				IDirectSoundBuffer_Play (pDSBuf, 0, 0, DSBPLAY_LOOPING);
		}
	}
#endif

	S_PaintChannels (endtime);

	SNDDMA_Submit ();
}

/*
===============================================================================

console functions

===============================================================================
*/

static void S_Play (lparse_t *line, float att)
{
	static int hash=345;
	int 	i;
	char name[256];
	sfx_t	*sfx;

	for (i = 1 ; i < line->count ; i++)
	{
		c_strlcpy(name, line->args[i]);
		if (!strrchr(line->args[i], '.'))
		{
			c_strlcat(name, ".wav");
		}
		sfx = S_PrecacheSound(name, NULL);
		S_StartSound(hash++, 0, sfx, listener_origin, 1.0, att);
	}
}

void S_Play_f (lparse_t *line)
{
	if (line->count != 2)
	{
		Con_PrintLinef ("Usage: play <filename>");
		return;
	}

	S_Play (line, 1.0f);
}

void S_Play2_f (lparse_t *line)
{
	if (line->count != 2)
	{
		Con_PrintLinef ("Usage: play2 <filename>");
		return;
	}

	S_Play (line, 0.0f);
}

void S_PlayVol (lparse_t *line)
{
	static int hash=543;
	int i;
	float vol;
	char name[256];
	sfx_t	*sfx;

	i = 1;

	while (i < line->count)
	{
		c_strlcpy (name, line->args[i]);
		if (!strrchr(line->args[i], '.'))
		{
			c_strlcat(name, ".wav");
		}
		sfx = S_PrecacheSound(name, NULL);
		vol = atof(line->args[i + 1]);
		S_StartSound(hash++, 0, sfx, listener_origin, vol, 1.0);
		i+=2;
	}
}

void S_SoundList (lparse_t *unused)
{
	int		i;
	sfx_t	*sfx;
	sfxcache_t	*sc;
	int		size, total, found = 0;

	total = 0;

	for (sfx=known_sfx, i=0 ; i<num_sfx ; i++, sfx++)
	{
		sc = (sfxcache_t *)Cache_Check (&sfx->cache);

		if (!sc)
			continue;

		size = sc->length*sc->width*(sc->stereo+1);
		found ++;
		total += size;

		if (sc->loopstart >= 0)
			Con_SafePrintContf ("L"); //johnfitz -- was Con_Printf
		else
			Con_SafePrintContf (" "); //johnfitz -- was Con_Printf
		Con_SafePrintLinef ("(%2db) %6d : %s", sc->width*8,  size, sfx->name); //johnfitz -- was Con_Printf
	}
	Con_PrintLinef ("%d sounds, %d bytes", /*num_sfx*/ found, total); //johnfitz -- added count (Baker: Made count ones loaded instead of named ones.)
}


const char *S_Sound_ListExport (void)
{
	static int last = -1; // Top of list.

	sfx_t *sfx;
	int i;

	// We want the first entry greater than or equal to this (i.e.)
	int	wanted = last + 1;

	for (sfx = known_sfx, i = 0 ; i < num_sfx ; i ++, sfx ++)
	{
		if (Cache_Check (&sfx->cache) && i >= wanted)
		{
			last = i;
			return sfx->name;
		}
	}

	// Return NULL and reset
	last = -1;
	return NULL;
}


void S_LocalSound (const char *name)
{
	sfx_t *sfx;

	if (nosound.value)
		return;

	if (!sound_started)
		return;

	sfx = S_PrecacheSound (name, NULL);

	if (!sfx)
	{
		Con_PrintLinef ("S_LocalSound: can't cache %s", name);
		return;
	}

	S_StartSound (cl.viewentity_player, -1, sfx, vec3_origin, 1, 1);
}


void S_ClearPrecache (void)
{
}


void S_BeginPrecaching (void)
{
}


void S_EndPrecaching (void)
{
}


#ifdef SUPPORTS_NEHAHRA
// nehahra supported
void Neh_ResetSFX (void)
{
	int	i;

	if (num_sfxorig == 0)
		num_sfxorig = num_sfx;

	num_sfx = num_sfxorig;
	Con_DPrintLinef ("Current SFX: %d", num_sfx);

	for (i=num_sfx+1 ; i< MAX_SFX ; i++)
	{
		c_strlcpy (known_sfx[i].name, "dfw3t23EWG#@T#@");
		if (known_sfx[i].cache.data)
			Cache_Free (&known_sfx[i].cache, false);
	}
}

void Neh_Reset_Sfx_Count (void)
{
	if (nehahra_active)
		num_sfxorig = num_sfx;

}
#endif // SUPPORTS_NEHAHRA

#ifndef DIRECT_SOUND_QUAKE // Baker: To allow blocking sound on non-Windows platforms
/*
==================
S_UnblockSound
==================
*/

void S_UnblockSound (void)
{
	snd_blocked = 0;
	logd ("Sound blocked = 0");
}

void S_BlockSound (void)
{
	snd_blocked = 1;
	logd ("Sound blocked = 1");
}

#endif // ! DIRECT_SOUND_QUAKE
