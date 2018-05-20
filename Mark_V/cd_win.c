// Here's something crazy.  If I don't exclude this from the project, it will explode compile for SDL.

#ifndef CORE_SDL
#include "environment.h"
#ifdef PLATFORM_WINDOWS // Header level define, must be here

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
// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

// cd_win.c

#include "quakedef.h"

#ifdef WANTED_MP3_MUSIC // Baker change
#pragma message ("Note: MP3 Music disabled; I've never had any luck with DX8 and MinGW + CodeBlocks / GCC --- Baker :(")
#endif

#include "winquake.h"

static cbool cdValid = false;
static cbool	playing = false;
static cbool	wasPlaying = false;
static cbool	initialized = false;
static cbool	enabled = false;
static cbool playLooping = false;
static float	cdvolume;
static byte 	remap[MAX_MUSIC_MAPS_100];
static byte		cdrom;
static byte		playTrack;
static byte		maxTrack;
#ifdef SUPPORTS_MP3_MUSIC // Baker change
static cbool		using_directshow = false;
#endif // Baker change +

UINT	wDeviceID;



static void CDAudio_Eject(void)
{
	DWORD	dwReturn;

    if (dwReturn = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_DOOR_OPEN, (DWORD)NULL))
		Con_DPrintLinef ("MCI_SET_DOOR_OPEN failed (%d)", dwReturn);
}


static void CDAudio_CloseDoor(void)
{
	DWORD	dwReturn;

    if (dwReturn = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_DOOR_CLOSED, (DWORD)NULL))
		Con_DPrintLinef ("MCI_SET_DOOR_CLOSED failed (%d)", dwReturn);
}


static int CDAudio_GetAudioDiskInfo(void)
{
	DWORD				dwReturn;
	MCI_STATUS_PARMS	mciStatusParms;

	cdValid = false;

	mciStatusParms.dwItem = MCI_STATUS_READY;
    dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);

	if (dwReturn)
	{
		Con_PrintLinef ("CDAudio: drive ready test - get status failed");
		return -1;
	}

	if (!mciStatusParms.dwReturn)
	{
		Con_DPrintLinef ("CDAudio: drive not ready");
		return -1;
	}

	mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);

	if (dwReturn)
	{
		Con_PrintLinef ("CDAudio: get tracks - status failed");
		return -1;
	}

	if (mciStatusParms.dwReturn < 1)
	{
		Con_PrintLinef ("CDAudio: no music tracks");
		return -1;
	}

	cdValid = true;
	maxTrack = mciStatusParms.dwReturn;

	return 0;
}

//
//
//
//
//
//
//
//
//
//
//
//

void CDAudio_Play(byte track, cbool looping)
{
	DWORD				dwReturn;
    MCI_PLAY_PARMS		mciPlayParms;
	MCI_STATUS_PARMS	mciStatusParms;

	CDAudio_Stop();

#ifdef SUPPORTS_MP3_MUSIC // Baker change
	using_directshow = false;
#endif // Baker change +

	if (!external_music.value)
		return; // User has it off

	if (!enabled)
	{
#ifdef SUPPORTS_MP3_MUSIC // Baker change
		// try directshow
		using_directshow = MediaPlayer_Play (track, looping);
		if (using_directshow)
#endif // Baker change +
		return;
	}

	if (!cdValid)
	{
		// try one more time
		CDAudio_GetAudioDiskInfo();

		// didn't work
		if (!cdValid)
		{
#ifdef SUPPORTS_MP3_MUSIC // Baker change
			// play with directshow instead
			using_directshow = MediaPlayer_Play (track, looping);
#endif // Baker change +
			return;
		}
	}

	track = remap[track];

	if (track < 1 || track > maxTrack)
	{
		Con_DPrintLinef ("CDAudio: Bad track number %u.", track);
		return;
	}

	// don't try to play a non-audio track
	mciStatusParms.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
	mciStatusParms.dwTrack = track;
    dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);

	if (dwReturn)
	{
		Con_DPrintLinef ("MCI_STATUS failed (%d)", dwReturn);
		return;
	}

	if (mciStatusParms.dwReturn != MCI_CDA_TRACK_AUDIO)
	{
		Con_PrintLinef ("CDAudio: track %d is not audio", track);
		return;
	}

	// get the length of the track to be played
	mciStatusParms.dwItem = MCI_STATUS_LENGTH;
	mciStatusParms.dwTrack = track;
    dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);

	if (dwReturn)
	{
		Con_DPrintLinef ("MCI_STATUS failed (%d)", dwReturn);
		return;
	}

	if (playing)
	{
		if (playTrack == track)
			return;

		CDAudio_Stop();
	}

    mciPlayParms.dwFrom = MCI_MAKE_TMSF(track, 0, 0, 0);
	mciPlayParms.dwTo = (mciStatusParms.dwReturn << 8) | track;
    mciPlayParms.dwCallback = (DWORD)sysplat.mainwindow;
    dwReturn = mciSendCommand(wDeviceID, MCI_PLAY, MCI_NOTIFY | MCI_FROM | MCI_TO, (DWORD)(LPVOID) &mciPlayParms);

	if (dwReturn)
	{
		Con_DPrintLinef ("CDAudio: MCI_PLAY failed (%d)", dwReturn);
		return;
	}

	playLooping = looping;
	playTrack = track;
	playing = true;

	if (cdvolume == 0.0)
		CDAudio_Pause ();
}

//
//
//
//

void CDAudio_Stop(void)
{
	DWORD	dwReturn;

#ifdef SUPPORTS_MP3_MUSIC // Baker change
	if (using_directshow)
	{
//		MediaPlayer_Stop ();
		MediaPlayer_Shutdown ();
		using_directshow = false;
		return;
	}
#endif // Baker change +

	if (!enabled)
		return;

	if (!playing)
		return;

    if (dwReturn = mciSendCommand(wDeviceID, MCI_STOP, 0, (DWORD)NULL))
		Con_DPrintLinef ("MCI_STOP failed (%d)", dwReturn);

	wasPlaying = false;
	playing = false;
}


void CDAudio_Pause(void)
{
	DWORD				dwReturn;
	MCI_GENERIC_PARMS	mciGenericParms;

#ifdef SUPPORTS_MP3_MUSIC // Baker change
	if (using_directshow)
	{
		MediaPlayer_Pause ();
		return;
	}
#endif // Baker change +

	if (!enabled)
		return;

	if (!playing)
		return;

	mciGenericParms.dwCallback = (DWORD)sysplat.mainwindow;
    if (dwReturn = mciSendCommand(wDeviceID, MCI_PAUSE, 0, (DWORD)(LPVOID) &mciGenericParms))
		Con_DPrintLinef ("MCI_PAUSE failed (%d)", dwReturn);

	wasPlaying = playing;
	playing = false;
}


void CDAudio_Resume(void)
{
	DWORD			dwReturn;
    MCI_PLAY_PARMS	mciPlayParms;

#ifdef SUPPORTS_MP3_MUSIC // Baker change
	if (using_directshow)
	{
		MediaPlayer_Resume ();
		return;
	}
#endif // Baker change +

	if (!enabled)
		return;

	if (!cdValid)
		return;

	if (!wasPlaying)
		return;

    mciPlayParms.dwFrom = MCI_MAKE_TMSF(playTrack, 0, 0, 0);
    mciPlayParms.dwTo = MCI_MAKE_TMSF(playTrack + 1, 0, 0, 0);
    mciPlayParms.dwCallback = (DWORD)sysplat.mainwindow;
    dwReturn = mciSendCommand(wDeviceID, MCI_PLAY, MCI_TO | MCI_NOTIFY, (DWORD)(LPVOID) &mciPlayParms);

	if (dwReturn)
	{
		Con_DPrintLinef ("CDAudio: MCI_PLAY failed (%d)", dwReturn);
		return;
	}

	playing = true;
}


void CD_f (lparse_t *line)
{
	const char *cmds[] = {"info", "loop", "off", "on", "pause", "play", "remap", "resume", "reset", "stop", NULL};
	enum arg_e {arg_info, arg_loop, arg_off, arg_on, arg_pause, arg_play, arg_remap, arg_resume, arg_reset, arg_stop, max_args};

	if (line->count >= 2)
	{
		const char *parm2 = line->args[1];
		int cmd_num = String_To_Array_Index (parm2, cmds);

		int		n, ret;

		switch (cmd_num)
		{
		case arg_info: // information

#if !defined(SUPPORTS_MP3_MUSIC) // Baker change
	if (!cdValid)
	{
		CDAudio_GetAudioDiskInfo();
		if (!cdValid)
		{
			Con_PrintLinef ("No CD in player.");
			return;
		}
	}
#endif // Baker change -
			Con_PrintLinef ("%u tracks", maxTrack);

			if (playing)
				Con_PrintLinef ("Currently %s track %u", playLooping ? "looping" : "playing", playTrack);
			else if (wasPlaying)
				Con_PrintLinef ("Paused %s track %u", playLooping ? "looping" : "playing", playTrack);

			Con_PrintLinef ("Volume is %f", cdvolume);
			return;

		case arg_loop: // loop the selected track:

			CDAudio_Play((byte)atoi(line->args[2]), true);
			return;

		case arg_on: // turn CD playback on:

#ifdef SUPPORTS_MP3_MUSIC // Baker change
			if (MediaPlayer_Force_On () )
			{
				if (host_post_initialized)
					Con_PrintLinef ("Music track play will start on map load");
			}
			else
			{
				if (host_post_initialized)
					Con_PrintLinef ("Disabled due to command line param -nosound or -nocdaudio");
			}
#endif // Baker change +

			return;

		case arg_off: // turn CD playback off:

	#ifdef SUPPORTS_MP3_MUSIC // Baker change
			MediaPlayer_Shutdown ();
			MediaPlayer_Force_Off ();
			using_directshow = false;
	#endif // Baker change +

			if (playing)
				CDAudio_Stop();

			enabled = false;
			return;

		case arg_pause: // pause the current track:

			CDAudio_Pause();
			return;

		case arg_play: // play the selected track:

			CDAudio_Play((byte)atoi(line->args[2]), false);
			return;

		case arg_stop: // stop the current track

			CDAudio_Stop();
			return;

		case arg_remap:

			ret = line->count - 2;

			if (ret <= 0)
			{
				for (n = 1; n < MAX_MUSIC_MAPS_100; n++)
					if (remap[n] != n)
						Con_PrintLinef ("  %u -> %u", n, remap[n]);

				return;
			}

			for (n = 1; n <= ret; n++)
				remap[n] = atoi(line->args[n+1]);

			return;

		case arg_resume: // resume the current track:

			CDAudio_Resume();
			return;

		case arg_reset: // reset the current CD:

			enabled = true;

			if (playing)
				CDAudio_Stop();

			for (n = 0; n < 100; n++)
				remap[n] = n;

			CDAudio_GetAudioDiskInfo();
			return;

		} // End of switch statement

	} // End of args >= 2

	// with no parameters or invalid parameters ends up displaying help
	Con_PrintLinef ("Usage: %s {play|stop|on|off|info|pause|resume} [filename]", line->args[0]);
	Con_PrintLinef ("  Note: music files should be in gamedir\\music");
	Con_PrintLinef ("Example: quake\\id1\\music\\track04.mp3 ");
	Con_PrintLine ();
	Con_PrintLinef ("%s is set to " QUOTED_S " and if set to 0, will prohibit music.", external_music.name, external_music.string);
}


LONG WIN_MediaPlayer_MessageHandler (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#ifdef SUPPORTS_MP3_MUSIC // Baker change
	// pass to direct show stuff
	MediaPlayer_Message (/*(int)Looping*/ 1 );

	return 0;
#endif // Baker change +
}


LONG WIN_CDAudio_MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (lParam != (int)wDeviceID)
		return 1;

#ifdef SUPPORTS_MP3_MUSIC // Baker change
	// don't handle CD messages if using directshow
	if (using_directshow) return 0;
#endif // Baker change +

	switch (wParam)
	{
		case MCI_NOTIFY_SUCCESSFUL:

			if (playing)
			{
				playing = false;

				if (playLooping)
					CDAudio_Play(playTrack, true);
			}

			break;

		case MCI_NOTIFY_ABORTED:
		case MCI_NOTIFY_SUPERSEDED:
			break;

		case MCI_NOTIFY_FAILURE:
			Con_PrintLinef ("MCI_NOTIFY_FAILURE");
			CDAudio_Stop ();
			cdValid = false;
			break;

		default:
			Con_DPrintLinef ("Unexpected MM_MCINOTIFY type (%d)", wParam);
			return 1;
	}

	return 0;
}

#ifdef SUPPORTS_MP3_MUSIC // Baker change
void MediaPlayer_ChangeVolume (float newvolume);
#endif // Baker change +

//
//
//
//
//
void CDAudio_Update(void)
{
#ifdef SUPPORTS_MP3_MUSIC // Baker change
	static float old_effective_volume = -1;
	float effective_volume = 9999;
#endif // Baker change +

//	if (!enabled) ... failure #1 if using_directhow and failure #2 if using fmod
//		return;

#ifdef SUPPORTS_MP3_MUSIC // Baker change
	if (using_directshow)
	{
		extern cbool muted;
#ifdef _DEBUG
		MediaPlayer_Update ();
#endif
		// Baker: Effective volume with mp3 music is relative to sound volume
		// Sets volume to 0 when not active app or minimized

		if (vid.ActiveApp == false || vid.Minimized == true || muted == true)
			effective_volume = 0;
		else
			effective_volume = bgmvolume.value * sfxvolume.value;
	} else effective_volume = bgmvolume.value;


	if (using_directshow && effective_volume == old_effective_volume)
		return;
	else if (!using_directshow && cdvolume == bgmvolume.value)
		return;

	if (using_directshow) // A volume changed
	{
		if (effective_volume == 0)
			effective_volume = effective_volume;
		MediaPlayer_ChangeVolume (effective_volume);
	}
	else if (bgmvolume.value)
		CDAudio_Resume ();
	else CDAudio_Pause ();		// A zero value will pause

	cdvolume = bgmvolume.value;
	old_effective_volume = effective_volume;
#else // Baker change +
	if (bgmvolume.value != cdvolume)
	{
		if (cdvolume)
		{
			Cvar_SetValueQuick (&bgmvolume, 0.0);
			cdvolume = bgmvolume.value;
			CDAudio_Pause ();
#ifdef SUPPORTS_NEHAHRA
			FMOD_Pause ();
#endif // SUPPORTS_NEHAHRA
		}
		else
		{
			Cvar_SetValueQuick (&bgmvolume, 1.0);
			cdvolume = bgmvolume.value;
			CDAudio_Resume ();
#ifdef SUPPORTS_NEHAHRA
			FMOD_Resume ();
#endif // SUPPORTS_NEHAHRA
		}
	}
#endif // Baker change -
}

cbool command_line_disabled = false;
void external_music_toggle_f (cvar_t* var)
{
	if (command_line_disabled)
		return; // Command line disabled

	if (var->value)
	{
		if (cls.state == ca_connected)
		{
			// First stop the music, previous value might not have been zero
			// Try to start the music
			CDAudio_Play ((byte)cl.cdtrack, true);
		}
	}
	else CDAudio_Stop ();
}

// Baker: The return value is not used, making it void
void CDAudio_Init(void)
{
	DWORD	dwReturn;
	MCI_OPEN_PARMS	mciOpenParms;
    MCI_SET_PARMS	mciSetParms;
	int				n;

	Cmd_AddCommands (CDAudio_Init);

	if (COM_CheckParm("-nosound"))
		command_line_disabled = true; // No sound --> no track music either

	if (COM_CheckParm("-nocdaudio"))
		command_line_disabled = true;

	if (command_line_disabled)
	{
#ifdef SUPPORTS_MP3_MUSIC
		MediaPlayer_Command_Line_Disabled ();
#endif // SUPPORTS_MP3_MUSIC
		Con_PrintLinef ("CD disabled by command line");
		return;
	}

	// Baker: Needs to be registered here in case CD is unavailable (netbook or whatever) but MP3 is
	// Otherwise user cannot control it.

	mciOpenParms.lpstrDeviceType = "cdaudio";

	if (dwReturn = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE, (DWORD) (LPVOID) &mciOpenParms))
	{
		Con_SafePrintLinef ("CDAudio_Init: MCI_OPEN failed (%d)", dwReturn);
		return;
	}

	wDeviceID = mciOpenParms.wDeviceID;

    // Set the time format to track/minute/second/frame (TMSF).
    mciSetParms.dwTimeFormat = MCI_FORMAT_TMSF;

    if (dwReturn = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)(LPVOID) &mciSetParms))
    {
		Con_PrintLinef ("MCI_SET_TIME_FORMAT failed (%d)", dwReturn);
        mciSendCommand(wDeviceID, MCI_CLOSE, 0, (DWORD)NULL);
		return;
    }

	for (n = 0; n < 100; n++)
		remap[n] = n;

	initialized = true;
	enabled = true;

	if (CDAudio_GetAudioDiskInfo())
	{
		Con_SafePrintLinef ("CDAudio_Init: No CD in player.");
		cdValid = false;
	}

	Con_SafePrintLinef ("%s", "CD Audio Initialized");
}


void CDAudio_Shutdown(void)
{
	if (!initialized)
		return;

#ifdef SUPPORTS_MP3_MUSIC // Baker change
	MediaPlayer_Shutdown ();
#endif // Baker change +

	CDAudio_Stop();

	if (mciSendCommand(wDeviceID, MCI_CLOSE, MCI_WAIT, (DWORD)NULL))
		Con_PrintLinef ("CDAudio_Shutdown: MCI_CLOSE failed");
}

#endif // PLATFORM_WINDOWS - Header level define

#endif // !CORE_SDL