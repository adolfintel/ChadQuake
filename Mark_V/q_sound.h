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
// sound.h -- client sound i/o functions

#ifndef __Q_SOUND_H__
#define __Q_SOUND_H__

/* !!! if this is changed, it must be changed in asm_i386.h too !!! */
typedef struct
{
	int left;
	int right;
} portable_samplepair_t;

typedef struct sfx_s
{
	char 	name[MAX_QPATH_64];
	cache_user_t	cache;
} sfx_t;

/* !!! if this is changed, it must be changed in asm_i386.h too !!! */
typedef struct
{
	int 	length;
	int 	loopstart;
	int 	speed;
	int 	width;
	int 	stereo;
	byte	data[1];	/* variable sized	*/
} sfxcache_t;

typedef struct
{
	cbool		gamealive;
	cbool		soundalive;
	cbool		splitbuffer;
	int				channels;
	int	samples;		/* mono samples in buffer			*/
	int	submission_chunk;	/* don't mix less than this #			*/
	int	samplepos;		/* in mono samples				*/
	int				samplebits;
	int				speed;
	unsigned char	*buffer;
} dma_t;

/* !!! if this is changed, it must be changed in asm_i386.h too !!! */
typedef struct
{
	sfx_t	*sfx;			/* sfx number					*/
	int	leftvol;		/* 0-255 volume					*/
	int	rightvol;		/* 0-255 volume					*/
	int	end;			/* end time in global paintsamples		*/
	int	pos;			/* sample position in sfx			*/
	int	looping;		/* where to loop, -1 = no looping		*/
	int	entnum;			/* to allow overriding a specific sound		*/
	int	entchannel;
	vec3_t	origin;			/* origin of sound effect			*/
	vec_t	dist_mult;		/* distance multiplier (attenuation/clipK)	*/
	int	master_vol;		/* 0-255 master volume				*/
} channel_t;

#define WAV_FORMAT_PCM	1

typedef struct
{
	int		rate;
	int		width;
	int		channels;
	int		loopstart;
	int		samples;
	int		dataofs;		/* chunk starts this many bytes from file start	*/
} wavinfo_t;

void S_Init (void);
void S_Startup (void);
void S_Shutdown (void);
void S_StartSound (int entnum, int entchannel, sfx_t *sfx, const vec3_t origin, float fvol,  float attenuation);
void S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation);
void S_StopSound (int entnum, int entchannel);
void S_StopAllSounds(cbool clear);
void S_ClearBuffer (void);
void S_Update (const vec3_t origin, const vec3_t forward, const vec3_t right, const vec3_t up);
void S_ExtraUpdate (void);

void S_BlockSound (void);
sfx_t *S_PrecacheSound (const char *name, cbool *precached_ok);
cbool S_PrecacheSound_Again (sfx_t *sfx);
void S_UnblockSound (void);


void S_TouchSound (const char *sample);
void S_ClearPrecache (void);
void S_BeginPrecaching (void);
void S_EndPrecaching (void);
void S_PaintChannels(int endtime);
void S_InitPaintChannels (void);

/* picks a channel based on priorities, empty slots, number of channels */
channel_t *SND_PickChannel(int entnum, int entchannel);

/* spatializes a channel */
void SND_Spatialize(channel_t *ch);

/* initializes cycling through a DMA buffer and returns information on it */
int SNDDMA_Init(void);

/* gets the current DMA position */
int SNDDMA_GetDMAPos(void);

/* shutdown the DMA xfer. */
void SNDDMA_Shutdown(void);

/* unlocks the dma buffer / sends sound to the device */
void SNDDMA_Submit(void);

/* ====================================================================
 * User-setable variables
 * ====================================================================
*/

#define	MAX_CHANNELS			1024 /* johnfitz -- was 128, was 512 in Fitz now 1024 for Rubicon Rumble*/
#define	MAX_DYNAMIC_CHANNELS	128 /* johnfitz -- was 8   */
#define	MAX_SFX					1024

extern	channel_t   snd_channels[MAX_CHANNELS];
/* 0 to MAX_DYNAMIC_CHANNELS-1	= normal entity sounds
 * MAX_DYNAMIC_CHANNELS to MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS -1 = water, etc
 * MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS to total_channels = static sounds
 */

extern volatile dma_t *shm;

extern	int			total_channels;
extern int		paintedtime;

//
// Fake dma is a synchronous faking of the DMA progress used for
// isolating performance in the renderer.  The fakedma_updates is
// number of times S_Update() is called per second.
//

extern cbool 		fakedma;
extern int 			fakedma_updates;

extern vec3_t listener_origin;
extern vec3_t listener_forward;
extern vec3_t listener_right;
extern vec3_t listener_up;

extern volatile dma_t sn;
//extern float sound_nominal_clip_dist;

extern int		snd_blocked;

#define		SOUND_NOMINAL_CLIP_DIST_DEFAULT_1000			1000.0
//#define		deathmatch_sound_nominal_clip_dist				1500.0  


void S_LocalSound (const char *name);
sfxcache_t *S_LoadSound (sfx_t *s);

wavinfo_t GetWavinfo (const char *name, byte *wav, int wavlength);

void SND_InitScaletable (void);


void S_AmbientOff (void);
void S_AmbientOn (void);

void Sound_Toggle_Mute_f (void);

const char *S_Sound_ListExport (void); // Baker

extern int sound_rate_hz;

#endif // ! __Q_SOUND_H__
