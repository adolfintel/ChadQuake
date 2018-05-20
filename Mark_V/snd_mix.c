/*
Copyright (C) 1996-2001 Id Software, Inc.
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
// snd_mix.c -- portable code to mix sounds for snd_dma.c

#include "quakedef.h"

#ifdef DIRECT_SOUND_QUAKE
    #include "winquake.h" // Change!
#endif

#define	PAINTBUFFER_SIZE	2048
portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE] /* qasm */;
int		snd_scaletable[32][256] /* qasm */;
int 	*snd_p /* qasm */, snd_linear_count /* qasm */;
short	*snd_out /* qasm */;

static int	snd_vol /* qasm */;

void Snd_WriteLinearBlastStereo16 (void)
{
	int		i;
	int		val;

	for (i=0 ; i<snd_linear_count ; i+=2)
	{
		val = (snd_p[i]*snd_vol)>>8;
		if (val > 0x7fff)
			snd_out[i] = 0x7fff;
		else if (val < (short)0x8000)
			snd_out[i] = (short)0x8000;
		else
			snd_out[i] = val;

		val = (snd_p[i+1]*snd_vol)>>8;
		if (val > 0x7fff)
			snd_out[i+1] = 0x7fff;
		else if (val < (short)0x8000)
			snd_out[i+1] = (short)0x8000;
		else
			snd_out[i+1] = val;
	}
}

cbool muted = false;

void Sound_Toggle_Mute_f (void)
{
	muted = !muted;

	if (muted)
		Con_SafePrintLinef ("Mute: ON  --- ALT-M to Toggle");
	else
		Con_SafePrintLinef ("Mute: OFF --- ALT-M to Toggle");


}



void S_TransferStereo16 (int endtime)
{
	int		lpos;
	int		lpaintedtime;

#ifdef DIRECT_SOUND_QUAKE
	DWORD	*pbuf;
	int		reps;
	DWORD	dwSize,dwSize2;
	DWORD	*pbuf2;
	HRESULT	hresult;
#else
	void *pbuf;
#endif // DIRECT_SOUND_QUAKE

	if (muted)
		snd_vol = 0;
	else
		snd_vol = sfxvolume.value*256;

	snd_p = (int *) paintbuffer;
	lpaintedtime = paintedtime;

#ifdef DIRECT_SOUND_QUAKE
	if (pDSBuf)
	{
		reps = 0;

		while ((hresult = IDirectSoundBuffer_Lock (pDSBuf, 0, gSndBufSize, &pbuf, &dwSize, &pbuf2, &dwSize2, 0)) != DS_OK)
		{
			if (hresult != DSERR_BUFFERLOST)
			{
				Con_PrintLinef ("S_TransferStereo16: DS::Lock Sound Buffer Failed");
				S_Shutdown ();
				S_Startup ();
				return;
			}

			if (++reps > 10000)
			{
				Con_PrintLinef ("S_TransferStereo16: DS: couldn't restore buffer");
				S_Shutdown ();
				S_Startup ();
				return;
			}
		}
	}
	else
	{
		pbuf = (DWORD *)shm->buffer;
	}
#else
    pbuf = (void *)shm->buffer;
#endif // DIRECT_SOUND_QUAKE

	while (lpaintedtime < endtime)
	{
	// handle recirculating buffer issues
		lpos = lpaintedtime & ((shm->samples>>1)-1);

		snd_out = (short *) pbuf + (lpos<<1);

		snd_linear_count = (shm->samples>>1) - lpos;
		if (lpaintedtime + snd_linear_count > endtime)
			snd_linear_count = endtime - lpaintedtime;

		snd_linear_count <<= 1;

	// write a linear blast of samples
		Snd_WriteLinearBlastStereo16 ();

		snd_p += snd_linear_count;
		lpaintedtime += (snd_linear_count>>1);

#ifdef SUPPORTS_AVI_CAPTURE // Baker change
		Movie_TransferStereo16 ();
#endif // Baker change +
	}

#ifdef DIRECT_SOUND_QUAKE
	if (pDSBuf)
		IDirectSoundBuffer_Unlock (pDSBuf, pbuf, dwSize, NULL, 0);
#endif // DIRECT_SOUND_QUAKE
}

static void S_TransferPaintBuffer(int endtime)
{
	int	out_idx, out_mask;
	int	count, step, val;
	int 	*p;

	int		snd_vol;
#ifdef DIRECT_SOUND_QUAKE
	DWORD	*pbuf;
	int		reps;
	DWORD	dwSize,dwSize2;
	DWORD	*pbuf2;
	HRESULT	hresult;
#else
    void *pbuf; // Right?
#endif // DIRECT_SOUND_QUAKE

	if (shm->samplebits == 16 && shm->channels == 2)
	{
		S_TransferStereo16 (endtime);
		return;
	}

	p = (int *) paintbuffer;
	count = (endtime - paintedtime) * shm->channels;
	out_mask = shm->samples - 1;
	out_idx = paintedtime * shm->channels & out_mask;
	step = 3 - shm->channels;

	if (muted)
		snd_vol = 0;
	else
		snd_vol = sfxvolume.value*256;

#ifdef DIRECT_SOUND_QUAKE
	if (pDSBuf)
	{
		reps = 0;

		while ((hresult = IDirectSoundBuffer_Lock (pDSBuf, 0, gSndBufSize, &pbuf, &dwSize, &pbuf2,&dwSize2, 0)) != DS_OK)
		{
			if (hresult != DSERR_BUFFERLOST)
			{
				Con_PrintLinef ("S_TransferPaintBuffer: DS::Lock Sound Buffer Failed");
				S_Shutdown ();
				S_Startup ();
				return;
			}

			if (++reps > 10000)
			{
				Con_PrintLinef ("S_TransferPaintBuffer: DS: couldn't restore buffer");
				S_Shutdown ();
				S_Startup ();
				return;
			}
		}
	}
	else
	{
		pbuf = (DWORD *)shm->buffer;
	}
#else
    pbuf = (short *)shm->buffer;
#endif // DIRECT_SOUND_QUAKE


	if (shm->samplebits == 16)
	{
		short *out = (short *) pbuf;
		while (count--)
		{
			val = (*p * snd_vol) >> 8;
			p+= step;
			if (val > 0x7fff)
				val = 0x7fff;
			else if (val < (short)0x8000)
				val = (short)0x8000;
			out[out_idx] = val;
			out_idx = (out_idx + 1) & out_mask;
		}
	}
	else if (shm->samplebits == 8) /* S8 format, e.g. with Amiga AHI */
	{
		signed char *out = (signed char *) pbuf;
		while (count--)
		{
			val = (*p * snd_vol) >> 8;
			p+= step;
			if (val > 0x7fff)
				val = 0x7fff;
			else if (val < (short)0x8000)
				val = (short)0x8000;
			out[out_idx] = (val>>8) + 128;
			out_idx = (out_idx + 1) & out_mask;
		}
	}

#ifdef DIRECT_SOUND_QUAKE
	if (pDSBuf) {
		DWORD dwNewpos, dwWrite;
		int il = paintedtime;
		int ir = endtime - paintedtime;

		ir += il;

		IDirectSoundBuffer_Unlock (pDSBuf, pbuf, dwSize, NULL, 0);

		IDirectSoundBuffer_GetCurrentPosition (pDSBuf, &dwNewpos, &dwWrite);

//		if ((dwNewpos >= il) && (dwNewpos <= ir))
//			Con_PrintLinef ("%d-%d p %d c", il, ir, dwNewpos);
	}
#endif // DIRECT_SOUND_QUAKE
}


/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/

static void SND_PaintChannelFrom8 (channel_t *ch, sfxcache_t *sc, int endtime);
static void SND_PaintChannelFrom16 (channel_t *ch, sfxcache_t *sc, int endtime);

void S_PaintChannels(int endtime)
{
	int 	i;
	int		end, ltime, count;
	channel_t *ch;
	sfxcache_t	*sc;


	while (paintedtime < endtime)
	{
	// if paintbuffer is smaller than DMA buffer
		end = endtime;
		if (endtime - paintedtime > PAINTBUFFER_SIZE)
			end = paintedtime + PAINTBUFFER_SIZE;

	// clear the paint buffer
		memset(paintbuffer, 0, (end - paintedtime) * sizeof(portable_samplepair_t));

	// paint in the channels.
		ch = snd_channels;
		for (i=0; i<total_channels ; i++, ch++)
		{
			if (!ch->sfx)
				continue;
			if (!ch->leftvol && !ch->rightvol)
				continue;
			sc = S_LoadSound (ch->sfx);
			if (!sc)
				continue;

			ltime = paintedtime;

			while (ltime < end)
			{	// paint up to end
				if (ch->end < end)
					count = ch->end - ltime;
				else
					count = end - ltime;

				if (count > 0)
				{
					if (sc->width == 1)
						SND_PaintChannelFrom8(ch, sc, count);
					else
						SND_PaintChannelFrom16(ch, sc, count);

					ltime += count;
				}

			// if at end of loop, restart
				if (ltime >= ch->end)
				{
					if (sc->loopstart >= 0)
					{
						ch->pos = sc->loopstart;
						ch->end = ltime + sc->length - ch->pos;
					}
					else
					{	// channel just stopped
						ch->sfx = NULL;
						break;
					}
				}
			}
		}

	// transfer out according to DMA format
		S_TransferPaintBuffer(end);
		paintedtime = end;
	}
}

void SND_InitScaletable (void)
{
	int		i, j;

	for (i = 0; i < 32 /*dimension 1 size*/; i++)
		for (j = 0; j < 256 /*dimension 2 size*/; j++)
			snd_scaletable[i][j] = ((signed char)j) * i * 8; // Baker: signed char evil lives here.  Watch out!
}




static void SND_PaintChannelFrom8 (channel_t *ch, sfxcache_t *sc, int count)
{
	int 	data;
	int		*lscale, *rscale;
	unsigned char *sfx;
	int		i;

	if (ch->leftvol > 255)
		ch->leftvol = 255;
	if (ch->rightvol > 255)
		ch->rightvol = 255;

	lscale = snd_scaletable[ch->leftvol >> 3];
	rscale = snd_scaletable[ch->rightvol >> 3];
	sfx = (unsigned char *)sc->data + ch->pos;

	for (i=0 ; i<count ; i++)
	{
		data = sfx[i];
		paintbuffer[i].left += lscale[data];
		paintbuffer[i].right += rscale[data];
	}

	ch->pos += count;
}



static void SND_PaintChannelFrom16 (channel_t *ch, sfxcache_t *sc, int count)
{
	int data;
	int left, right;
	int leftvol, rightvol;
	signed short *sfx;
	int	i;

	leftvol = ch->leftvol;
	rightvol = ch->rightvol;
	sfx = (signed short *)sc->data + ch->pos;

	for (i=0 ; i<count ; i++)
	{
		data = sfx[i];
		left = (data * leftvol) >> 8;
		right = (data * rightvol) >> 8;
		paintbuffer[i].left += left;
		paintbuffer[i].right += right;
	}

	ch->pos += count;
}