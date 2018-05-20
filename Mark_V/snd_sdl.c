#ifdef CORE_SDL

/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2005 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske

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
#include "quakedef.h"
#include "sdlquake.h"
// stolen from http://www.libsdl.org/projects/quake/ -- apologies, I don't know the author's name


static dma_t the_shm;
static int snd_inited;

#define BUF_SIZE 512

static void paint_audio(void *unused, byte *stream, int len)
{
	if ( shm ) {
		shm->buffer = stream;
		shm->samplepos += len/(shm->samplebits/8)/2;
		// Check for samplepos overflow?
		S_PaintChannels (shm->samplepos);
	}
}

int SNDDMA_Init(void)
{
	SDL_AudioSpec desired, obtained;

	snd_inited = 0;

	/* Set up the desired format */
	desired.freq = sound_rate_hz; // sndspeed.value;

    if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        desired.format = AUDIO_S16MSB;
    else desired.format = AUDIO_S16LSB;

	desired.channels = 2;
	desired.samples = BUF_SIZE;
	desired.callback = paint_audio;

	/* Open the audio device */
	if ( SDL_OpenAudio(&desired, &obtained) < 0 ) {
        	Con_PrintLinef ("Couldn't open SDL audio: %s", SDL_GetError());
		return 0;
	}

	/* Make sure we can support the audio format */
	switch (obtained.format)
	{
		case AUDIO_U8:
			/* Supported */
			break;
		case AUDIO_S16LSB:
		case AUDIO_S16MSB:
			if ( ((obtained.format == AUDIO_S16LSB) &&
			     (SDL_BYTEORDER == SDL_LIL_ENDIAN)) ||
			     ((obtained.format == AUDIO_S16MSB) &&
			     (SDL_BYTEORDER == SDL_BIG_ENDIAN)) ) {
				/* Supported */
				break;
			}
			/* Unsupported, fall through */;
		default:
			/* Not supported -- force SDL to do our bidding */
			SDL_CloseAudio();
			if ( SDL_OpenAudio(&desired, NULL) < 0 ) {
        			Con_PrintLinef ("Couldn't open SDL audio: %s",
							SDL_GetError());
				return 0;
			}
			memcpy(&obtained, &desired, sizeof(desired));
			break;
	}
	SDL_PauseAudio(0);

	/* Fill the audio DMA information block */
	shm = &the_shm;
	shm->splitbuffer = 0;
	shm->samplebits = (obtained.format & 0xFF);
	shm->speed = obtained.freq;
	shm->channels = obtained.channels;
	shm->samples = obtained.samples*shm->channels;
	shm->samplepos = 0;
	shm->submission_chunk = 1;
	shm->buffer = NULL;

    sound_rate_hz = obtained.freq; // Right?

	snd_inited = 1;
	Con_SafePrintLinef ("SDL Sound Initialized");
	return 1;
}

int SNDDMA_GetDMAPos(void)
{
	return shm->samplepos;
}

void SNDDMA_Shutdown(void)
{
	if (snd_inited)
	{
		SDL_CloseAudio();
		snd_inited = 0;
	}
}

void    SNDDMA_Submit (void)
{
}

#endif // CORE_SDL
