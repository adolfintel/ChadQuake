#ifndef CORE_SDL
#include "environment.h"
#ifdef PLATFORM_WINDOWS // Has to be here, set by a header


/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2012 John Fitzgibbons and others
Copyright (C) 2009-2014 Baker and others

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
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
#include "winquake.h"

#define iDirectSoundCreate(a,b,c)	pDirectSoundCreate(a,b,c)

HRESULT (WINAPI *pDirectSoundCreate)(GUID FAR *lpGUID, LPDIRECTSOUND FAR *lplpDS, IUnknown FAR *pUnkOuter);

// 64K is > 1 second at 16-bit, 22050 Hz
#define	WAV_BUFFERS				64
#define	WAV_MASK				0x3F
#define	WAV_BUFFER_SIZE			0x0400
#define SECONDARY_BUFFER_SIZE	0x10000

typedef enum {SIS_SUCCESS, SIS_FAILURE, SIS_NOTAVAIL} sndinitstat;

static cbool	wavonly;
static cbool	dsound_init;
static cbool	wav_init;
static cbool	snd_firsttime = true, snd_isdirect, snd_iswave;
static cbool	primary_format_set;

static int	sample16;
static int	snd_sent, snd_completed;

/*
 * Global variables. Must be visible to window-procedure function
 *  so it can unlock and free the data block after it has been played.
 */

HANDLE		hData;
HPSTR		lpData, lpData2;

HGLOBAL		hWaveHdr;
LPWAVEHDR	lpWaveHdr;

HWAVEOUT    hWaveOut;

WAVEOUTCAPS	wavecaps;

DWORD	gSndBufSize;

MMTIME		mmstarttime;

LPDIRECTSOUND pDS;
LPDIRECTSOUNDBUFFER pDSBuf, pDSPBuf;

HINSTANCE hInstDS;

sndinitstat SNDDMA_InitDirect (void);
cbool SNDDMA_InitWav (void);


/*
==================
S_BlockSound
==================
*/
void S_BlockSound (void)
{

// DirectSound takes care of blocking itself
	if (snd_iswave)
	{
		snd_blocked++;

		if (snd_blocked == 1)
		{
			waveOutReset (hWaveOut);
		}
	}
}


/*
==================
S_UnblockSound
==================
*/
void S_UnblockSound (void)
{

// DirectSound takes care of blocking itself
	if (snd_iswave)
	{
		snd_blocked--;
	}
}


/*
==================
FreeSound
==================
*/
void FreeSound (void)
{
	int		i;

	if (pDSBuf)
	{
		IDirectSoundBuffer_Stop (pDSBuf);
		IDirectSound_Release (pDSBuf);
	}

// only release primary buffer if it's not also the mixing buffer we just released
	if (pDSPBuf && (pDSBuf != pDSPBuf))
	{
		IDirectSound_Release (pDSPBuf);
	}

	if (pDS)
	{
		IDirectSound_SetCooperativeLevel (pDS, sysplat.mainwindow, DSSCL_NORMAL);
		IDirectSound_Release (pDS);
	}

	if (hWaveOut)
	{
		waveOutReset (hWaveOut);

		if (lpWaveHdr)
		{
			for (i=0 ; i< WAV_BUFFERS ; i++)
				waveOutUnprepareHeader (hWaveOut, lpWaveHdr+i, sizeof(WAVEHDR));
		}

		waveOutClose (hWaveOut);

		if (hWaveHdr)
		{
			GlobalUnlock(hWaveHdr);
			GlobalFree(hWaveHdr);
		}

		if (hData)
		{
			GlobalUnlock(hData);
			GlobalFree(hData);
		}

	}

	pDS = NULL;
	pDSBuf = NULL;
	pDSPBuf = NULL;
	hWaveOut = 0;
	hData = 0;
	hWaveHdr = 0;
	lpData = NULL;
	lpWaveHdr = NULL;
	dsound_init = false;
	wav_init = false;

#ifdef SUPPORTS_NEHAHRA
	if (nehahra_active)
		FMOD_Close ();
#endif // SUPPORTS_NEHAHRA
}



/*
==================
SNDDMA_InitDirect

Direct-Sound support
==================
*/
sndinitstat SNDDMA_InitDirect (void)
{
	DSBUFFERDESC	dsbuf;
	DSBCAPS			dsbcaps;
	DWORD			dwSize, dwWrite;
	DSCAPS			dscaps;
	WAVEFORMATEX	format, pformat;
	HRESULT			hresult;
	int				reps;

	memset ((void *)&sn, 0, sizeof (sn));

	shm = &sn;

	shm->channels = 2;
	shm->samplebits = 16;
	shm->speed = sound_rate_hz; //(int)sndspeed.value ? (int)sndspeed.value : 11025;  // SOUND_RATE

	memset (&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = shm->channels;
    format.wBitsPerSample = shm->samplebits;
    format.nSamplesPerSec = shm->speed;
    format.nBlockAlign = format.nChannels *format.wBitsPerSample / 8;
    format.cbSize = 0;
    format.nAvgBytesPerSec = format.nSamplesPerSec *format.nBlockAlign;

	if (!hInstDS)
	{
		hInstDS = LoadLibrary("dsound.dll");

		if (hInstDS == NULL)
		{
			Con_SafePrintLinef ("Couldn't load dsound.dll");
			return SIS_FAILURE;
		}

		pDirectSoundCreate = (void *)GetProcAddress(hInstDS,"DirectSoundCreate");

		if (!pDirectSoundCreate)
		{
			Con_SafePrintLinef ("Couldn't get DS proc addr");
			return SIS_FAILURE;
		}
	}

	while ((hresult = iDirectSoundCreate(NULL, &pDS, NULL)) != DS_OK)
	{
		if (hresult != DSERR_ALLOCATED)
		{
			Con_SafePrintLinef ("DirectSound create failed");
			return SIS_FAILURE;
		}

		if (MessageBox (NULL,
						"The sound hardware is in use by another app." NEWLINE NEWLINE
					    "Select Retry to try to start sound again or Cancel to run Quake with no sound.",
						"Sound not available",
						MB_RETRYCANCEL | MB_SETFOREGROUND | MB_ICONEXCLAMATION) != IDRETRY)
		{
			Con_SafePrintLinef ("DirectSoundCreate failure");
			Con_SafePrintLinef ("  hardware already in use");
			return SIS_NOTAVAIL;
		}
	}

	dscaps.dwSize = sizeof(dscaps);

	if (DS_OK != IDirectSound_GetCaps (pDS, &dscaps))
	{
		Con_SafePrintLinef ("Couldn't get DS caps");
	}

	if (dscaps.dwFlags & DSCAPS_EMULDRIVER)
	{
		Con_SafePrintLinef ("No DirectSound driver installed");
		FreeSound ();
		return SIS_FAILURE;
	}

	if (DS_OK != IDirectSound_SetCooperativeLevel (pDS, sysplat.mainwindow, DSSCL_EXCLUSIVE))
	{
		Con_SafePrintLinef ("Set coop level failed");
		FreeSound ();
		return SIS_FAILURE;
	}

// get access to the primary buffer, if possible, so we can set the
// sound hardware format
	memset (&dsbuf, 0, sizeof(dsbuf));
	dsbuf.dwSize = sizeof(DSBUFFERDESC);
	dsbuf.dwFlags = DSBCAPS_PRIMARYBUFFER;
	dsbuf.dwBufferBytes = 0;
	dsbuf.lpwfxFormat = NULL;

	memset(&dsbcaps, 0, sizeof(dsbcaps));
	dsbcaps.dwSize = sizeof(dsbcaps);
	primary_format_set = false;

	if (!COM_CheckParm ("-snoforceformat"))
	{
		if (DS_OK == IDirectSound_CreateSoundBuffer (pDS, &dsbuf, &pDSPBuf, NULL))
		{
			pformat = format;

			if (DS_OK != IDirectSoundBuffer_SetFormat (pDSPBuf, &pformat))
			{
				if (snd_firsttime)
					Con_SafePrintLinef ("Set primary sound buffer format: no");
			}
			else
			{
				if (snd_firsttime)
					Con_SafePrintLinef ("Set primary sound buffer format: yes");

				primary_format_set = true;
			}
		}
	}

	if (!primary_format_set || !COM_CheckParm ("-primarysound"))
	{
	// create the secondary buffer we'll actually work with
		memset (&dsbuf, 0, sizeof(dsbuf));
		dsbuf.dwSize = sizeof(DSBUFFERDESC);
		dsbuf.dwFlags = DSBCAPS_CTRLFREQUENCY | DSBCAPS_LOCSOFTWARE;
		dsbuf.dwBufferBytes = SECONDARY_BUFFER_SIZE;
		dsbuf.lpwfxFormat = &format;

		memset(&dsbcaps, 0, sizeof(dsbcaps));
		dsbcaps.dwSize = sizeof(dsbcaps);

		if (DS_OK != IDirectSound_CreateSoundBuffer (pDS, &dsbuf, &pDSBuf, NULL))
		{
			Con_SafePrintLinef ("DS:CreateSoundBuffer Failed");
			FreeSound ();
			return SIS_FAILURE;
		}

		shm->channels = format.nChannels;
		shm->samplebits = format.wBitsPerSample;
		shm->speed = format.nSamplesPerSec;

		if (DS_OK != IDirectSound_GetCaps (pDSBuf, &dsbcaps))
		{
			Con_SafePrintLinef ("DS:GetCaps failed");
			FreeSound ();
			return SIS_FAILURE;
		}

		if (snd_firsttime)
			Con_SafePrintLinef ("Using secondary sound buffer");
	}
	else
	{
		if (DS_OK != IDirectSound_SetCooperativeLevel (pDS, sysplat.mainwindow, DSSCL_WRITEPRIMARY))
		{
			Con_SafePrintLinef ("Set coop level failed");
			FreeSound ();
			return SIS_FAILURE;
		}

		if (DS_OK != IDirectSound_GetCaps (pDSPBuf, &dsbcaps))
		{
			Con_PrintLinef ("DS:GetCaps failed");
			return SIS_FAILURE;
		}

		pDSBuf = pDSPBuf;
		Con_SafePrintLinef ("Using primary sound buffer");
	}

	// Make sure mixer is active
	IDirectSoundBuffer_Play (pDSBuf, 0, 0, DSBPLAY_LOOPING);

	if (snd_firsttime) {
		Con_SafePrintLinef ("   %d channel(s)", shm->channels);
		Con_SafePrintLinef ("   %d bits/sample", shm->samplebits);
		Con_SafePrintLinef ("   %d bytes/sec", shm->speed);
	}

	gSndBufSize = dsbcaps.dwBufferBytes;

// initialize the buffer
	reps = 0;

	while ((hresult = IDirectSoundBuffer_Lock (pDSBuf, 0, gSndBufSize, &lpData, &dwSize, NULL, NULL, 0)) != DS_OK)
	{
		if (hresult != DSERR_BUFFERLOST)
		{
			Con_SafePrintLinef ("SNDDMA_InitDirect: DS::Lock Sound Buffer Failed");
			FreeSound ();
			return SIS_FAILURE;
		}

		if (++reps > 10000)
		{
			Con_SafePrintLinef ("SNDDMA_InitDirect: DS: couldn't restore buffer");
			FreeSound ();
			return SIS_FAILURE;
		}

	}

	memset(lpData, 0, dwSize);
//		lpData[4] = lpData[5] = 0x7f;	// force a pop for debugging

	IDirectSoundBuffer_Unlock (pDSBuf, lpData, dwSize, NULL, 0);

	/* we don't want anyone to access the buffer directly w/o locking it first. */
	lpData = NULL;

	IDirectSoundBuffer_Stop (pDSBuf);
	IDirectSoundBuffer_GetCurrentPosition (pDSBuf, &mmstarttime.u.sample, &dwWrite);
	IDirectSoundBuffer_Play (pDSBuf, 0, 0, DSBPLAY_LOOPING);

	shm->soundalive = true;
	shm->splitbuffer = false;
	shm->samples = gSndBufSize/(shm->samplebits/8);
	shm->samplepos = 0;
	shm->submission_chunk = 1;
	shm->buffer = (unsigned char *) lpData;
	sample16 = (shm->samplebits/8) - 1;

	dsound_init = true;

	return SIS_SUCCESS;
}


/*
==================
SNDDM_InitWav

Crappy windows multimedia base
==================
*/
cbool SNDDMA_InitWav (void)
{
	WAVEFORMATEX  format;
	int				i;
	HRESULT			hr;

	snd_sent = 0;
	snd_completed = 0;

	shm = &sn;

	shm->channels = 2;
	shm->samplebits = 16;
	shm->speed = sound_rate_hz;  // SOUND_RATE (int)sndspeed.value;

	memset (&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = shm->channels;
	format.wBitsPerSample = shm->samplebits;
	format.nSamplesPerSec = shm->speed;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.cbSize = 0;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	/* Open a waveform device for output using window callback. */
	while ((hr = waveOutOpen((LPHWAVEOUT)&hWaveOut, WAVE_MAPPER, &format, 0, 0L, CALLBACK_NULL)) != MMSYSERR_NOERROR)
	{
		if (hr != MMSYSERR_ALLOCATED)
		{
			Con_SafePrintLinef ("waveOutOpen failed");
			return false;
		}

		if (MessageBox (NULL,
						"The sound hardware is in use by another app." NEWLINE NEWLINE
					    "Select Retry to try to start sound again or Cancel to run Quake with no sound.",
						"Sound not available",
						MB_RETRYCANCEL | MB_SETFOREGROUND | MB_ICONEXCLAMATION) != IDRETRY)
		{
			Con_SafePrintLinef ("waveOutOpen failure;");
			Con_SafePrintLinef ("  hardware already in use");
			return false;
		}
	}

	/*
	 * Allocate and lock memory for the waveform data. The memory
	 * for waveform data must be globally allocated with
	 * GMEM_MOVEABLE and GMEM_SHARE flags.

	*/
	gSndBufSize = WAV_BUFFERS*WAV_BUFFER_SIZE;
	hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, gSndBufSize);
	if (!hData)
	{
		Con_SafePrintLinef ("Sound: Out of memory.");
		FreeSound ();
		return false;
	}
	lpData = GlobalLock(hData);
	if (!lpData)
	{
		Con_SafePrintLinef ("Sound: Failed to lock.");
		FreeSound ();
		return false;
	}
	memset (lpData, 0, gSndBufSize);

	/*
	 * Allocate and lock memory for the header. This memory must
	 * also be globally allocated with GMEM_MOVEABLE and
	 * GMEM_SHARE flags.
	 */
	hWaveHdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, (DWORD) sizeof(WAVEHDR) * WAV_BUFFERS);

	if (hWaveHdr == NULL)
	{
		Con_SafePrintLinef ("Sound: Failed to Alloc header.");
		FreeSound ();
		return false;
	}

	lpWaveHdr = (LPWAVEHDR) GlobalLock(hWaveHdr);

	if (lpWaveHdr == NULL)
	{
		Con_SafePrintLinef ("Sound: Failed to lock header.");
		FreeSound ();
		return false;
	}

	memset (lpWaveHdr, 0, sizeof(WAVEHDR) * WAV_BUFFERS);

	/* After allocation, set up and prepare headers. */
	for (i=0 ; i<WAV_BUFFERS ; i++)
	{
		lpWaveHdr[i].dwBufferLength = WAV_BUFFER_SIZE;
		lpWaveHdr[i].lpData = lpData + i*WAV_BUFFER_SIZE;

		if (waveOutPrepareHeader(hWaveOut, lpWaveHdr+i, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
		{
			Con_SafePrintLinef ("Sound: failed to prepare wave headers");
			FreeSound ();
			return false;
		}
	}

	shm->soundalive = true;
	shm->splitbuffer = false;
	shm->samples = gSndBufSize/(shm->samplebits/8);
	shm->samplepos = 0;
	shm->submission_chunk = 1;
	shm->buffer = (unsigned char *) lpData;
	sample16 = (shm->samplebits/8) - 1;

	wav_init = true;

	return true;
}

/*
==================
SNDDMA_Init

Try to find a sound device to mix for.
Returns false if nothing is found.
==================
*/

int SNDDMA_Init(void)
{
	sndinitstat	stat;

	if (COM_CheckParm ("-wavonly"))
		wavonly = true;

	// Read early cvars


	dsound_init = wav_init = 0;

	stat = SIS_FAILURE;	// assume DirectSound won't initialize

	/* Init DirectSound */
	if (!wavonly)
	{
		if (snd_firsttime || snd_isdirect)
		{
			stat = SNDDMA_InitDirect ();

			if (stat == SIS_SUCCESS)
			{
				snd_isdirect = true;

				if (snd_firsttime)
					Con_SafePrintLinef ("DirectSound initialized");
			}
			else
			{
				snd_isdirect = false;
				Con_SafePrintLinef ("DirectSound failed to init");
			}
		}
	}

// if DirectSound didn't succeed in initializing, try to initialize
// waveOut sound, unless DirectSound failed because the hardware is
// already allocated (in which case the user has already chosen not
// to have sound)
	if (!dsound_init && (stat != SIS_NOTAVAIL))
	{
		if (snd_firsttime || snd_iswave)
		{

			snd_iswave = SNDDMA_InitWav ();

			if (snd_iswave)
			{
				if (snd_firsttime)
					Con_SafePrintLinef ("Wave sound initialized");
			}
			else
			{
				Con_SafePrintLinef ("Wave sound failed to init");
			}
		}
	}

	snd_firsttime = false;

	if (!dsound_init && !wav_init)
	{
		if (snd_firsttime)
			Con_SafePrintLinef ("No sound device initialized");

		return 0;
	}

	return 1;
}

/*
==============
SNDDMA_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
int SNDDMA_GetDMAPos(void)
{
	MMTIME	mmtime;
	int		s;
	DWORD	dwWrite;

	if (dsound_init)
	{
		mmtime.wType = TIME_SAMPLES;
		IDirectSoundBuffer_GetCurrentPosition (pDSBuf, &mmtime.u.sample, &dwWrite);
		s = mmtime.u.sample - mmstarttime.u.sample;
	}
	else if (wav_init)
	{
		s = snd_sent * WAV_BUFFER_SIZE;
	}


	s >>= sample16;

	s &= (shm->samples-1);

	return s;
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{
	LPWAVEHDR	h;
	int			wResult;

	if (!wav_init)
		return;

	//
	// find which sound blocks have completed
	//
	while (1)
	{
		if ( snd_completed == snd_sent )
		{
			Con_DPrintLinef ("Sound overrun");
			break;
		}

		if ( ! (lpWaveHdr[ snd_completed & WAV_MASK].dwFlags & WHDR_DONE) )
		{
			break;
		}

		snd_completed++;	// this buffer has been played
	}

	//
	// submit two new sound blocks
	//
	while (((snd_sent - snd_completed) >> sample16) < 4)
	{
		h = lpWaveHdr + ( snd_sent&WAV_MASK );

		snd_sent++;
		/*
		 * Now the data block can be sent to the output device. The
		 * waveOutWrite function returns immediately and waveform
		 * data is sent to the output device in the background.
		 */
		wResult = waveOutWrite(hWaveOut, h, sizeof(WAVEHDR));

		if (wResult != MMSYSERR_NOERROR)
		{
			Con_SafePrintLinef ("Failed to write block to device");
			FreeSound ();
			return;
		}
	}
}

/*
==============
SNDDMA_Shutdown

Reset the sound device for exiting
===============
*/
void SNDDMA_Shutdown(void)
{
	FreeSound ();
}

#endif // PLATFORM_WINDOWS

#endif // !CORE_SDL
