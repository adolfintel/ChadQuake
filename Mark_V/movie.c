/*
Copyright (C) 2001 Quake done Quick
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
// movie.c -- AVI capture

#include "quakedef.h"

#ifdef WANTED_AVI_CAPTURE // Baker change
#pragma message "Note: AVI Capture disabled; MinGW doesn't support msacm.h at this time :("
#endif

#ifdef SUPPORTS_AVI_CAPTURE // Baker change

void AVI_LoadLibrary (void);
void ACM_LoadLibrary (void);
int Capture_Open (const char *filename, const char *usercodec, cbool silentish);
void Capture_WriteVideo (byte *pixel_buffer);
void Capture_WriteAudio (int samples, byte *sample_buffer);
void Capture_Close (void);


extern cbool	scr_drawloading;
extern	short	*snd_out;
extern	int	snd_linear_count, soundtime;

// Variables for buffering audio
short	capture_audio_samples[44100];	// big enough buffer for 1fps at 44100Hz
int	captured_audio_samples;

static	int	out_size, ssize, outbuf_size;
static	byte	*outbuf, *picture_buf;
static	FILE	*moviefile;

static	float	hack_ctr;


static cbool movie_is_capturing = false;
static cbool movie_is_capturing_temp = false;
cbool	avi_loaded, acm_loaded;

cbool Movie_IsActive (void)
{
	// don't output whilst console is down or 'loading' is displayed
	if ((!capture_console.value && console1.visible_pct) || scr_drawloading)
		return false;

	// Never capture the console if capturedemo is running
	if (cls.capturedemo && console1.visible_pct)
		return false;

	// otherwise output if a file is open to write to
	return movie_is_capturing;
}


char	movie_capturing_name[MAX_QPATH_64];
char	movie_capturing_fullpath[MAX_OSPATH]; // fullpath
char	movie_codec[12];

void Movie_Start_Capturing (const char *moviename)
{

	hack_ctr = capture_hack.value;

	c_strlcpy (movie_capturing_name, moviename);
	File_URL_Edit_Force_Extension (movie_capturing_name, ".avi", sizeof(movie_capturing_name) );
	FS_FullPath_From_QPath (movie_capturing_fullpath, movie_capturing_name);

	if (vid.screen.width % 4) {
		Con_PrintLinef ("Can't video mode width is %d, but must be multiple of 4", vid.screen.width);
		Con_PrintLinef ("Maybe press ALT-ENTER once or twice and try again?");
		return;	
	}

	if (vid.screen.height % 4) {
		Con_PrintLinef ("Can't video mode height is %d, but must be multiple of 4.", vid.screen.height);
		Con_PrintLinef ("Maybe press ALT-ENTER once or twice and try again?");
		return;	
	}

	if (!(moviefile = FS_fopen_write(movie_capturing_fullpath, "wb")))
	{
		File_Mkdir_Recursive (movie_capturing_fullpath);
		if (!(moviefile = FS_fopen_write(movie_capturing_fullpath, "wb")))
		{
			Con_PrintLinef ("ERROR: Couldn't open %s", movie_capturing_name);
			return;
		}
	}

	if (strcasecmp("auto", capture_codec.string) == 0)
	{
		// Automatic
		char *codec_order[] = {"vp80", "xvid", "divx", "none"};
		int	count = sizeof(codec_order) / sizeof(codec_order[0]);
		int result, i;

		for (i = 0; i < count ; i ++)
		{
			result = Capture_Open (movie_capturing_fullpath, codec_order[i], true);
			if (result == true)
			{
				c_strlcpy (movie_codec, codec_order[i]);
				break;
			}
		}
		if (result != true)
		{
			movie_is_capturing = false;
			Con_PrintLinef ("ERROR: Couldn't create video stream");
		}
		else
			movie_is_capturing = true;
	}
	else
	{
		movie_is_capturing = (Capture_Open (movie_capturing_fullpath, capture_codec.string, false) > 0);
		if (movie_is_capturing)
			c_strlcpy (movie_codec, capture_codec.string);
	}
}



void Movie_Stop (void)
{
	movie_is_capturing = false;
	Capture_Close ();
	FS_fclose (moviefile);
	Recent_File_Set_QPath (movie_capturing_name);

	if (cls.demo_hosttime_elapsed /*cls.capturedemo*/) // Because cls.capturedemo already was cleared :(
		Con_PrintLinef ("Video completed: %s in %d:%02d (codec: %s)", movie_capturing_name, Time_Minutes((int)cls.demo_hosttime_elapsed), Time_Seconds((int)cls.demo_hosttime_elapsed), movie_codec);
	else
	VID_Local_Set_Window_Caption (NULL); // Restores it to default
	Con_PrintLinef ("Video completed: %s (codec: %s)", movie_capturing_name, movie_codec);

}

void Movie_Stop_Capturing (void)
{
	if (movie_is_capturing == 0)
	{
		Con_PrintLinef ("Not capturing");
		return;
	}

	if (cls.capturedemo)
		cls.capturedemo = false;

	Movie_Stop ();

}

void Movie_StopPlayback (void);

void Movie_CaptureDemo_f (lparse_t *line)
{
	char demoname[MAX_QPATH_64];

	if (line->count != 2)
	{
		Con_PrintLinef ("Usage: capturedemo <demoname>" NEWLINE NEWLINE "Note: stopdemo will stop video capture" NEWLINE "Use cl_capturevideo_* cvars for codec, fps, etc.");
		return;
	}

	if (movie_is_capturing || movie_is_capturing_temp)
	{
		Con_PrintLinef ("Can't capture demo, video is capturing");
		return;
	}

	// Baker: This is a performance benchmark.  No reason to have console up.
	if (key_dest != key_game)
		Key_SetDest (key_game);

	CL_Clear_Demos_Queue (); // timedemo is a very intentional action

	CL_PlayDemo_f (line);
	if (!cls.demoplayback)
		return;

	c_strlcpy (demoname, line->args[1]);
	// Note: line is about to become stale!!!  If we run a frame, that means additional commands
	// can stomp all over line!!
	// We are also potentially opening a hole if the first 2 frames of the demo run a command.
	// So in many ways this is a bad idea, but let's see what happens here.
movie_is_capturing_temp = true;
	{
		// Due to double buffering
		// And my hate of capturing the console.
		// Run a frame or 2 first to ensure the console
		// Is gone before we capture
		int i;
		for (i = 0; i < 2; i ++) {
			static double oldtime;
			double newtime = System_DoubleTime ();
			double timeslice = newtime - oldtime;

			Host_Frame (timeslice);
			oldtime = newtime;
		}
	}
movie_is_capturing_temp = false;
	Movie_Start_Capturing (demoname);
	cls.capturedemo = true;

	if (!movie_is_capturing)
	{
		Movie_StopPlayback ();
		// Baker: If capturedemo fails, just stop the demo playback.
		// Don't confuse the user
		Host_Stopdemo_f (NULL);

 		// If +capturedemo in command line, we exit after demo capture
		// completed (even if failed .. and this is failure location here)
		if (cls.capturedemo_and_exit)
			Host_Quit ();
	}

}

void Movie_Capture_Toggle_f (lparse_t *line)
{
	if (line->count != 2 || strcasecmp(line->args[1], "toggle") != 0)
	{
		Con_PrintLinef ("usage: %s <toggle>" NEWLINE NEWLINE "set capturevideo_codec and fps first", line->args[0]);
		Con_PrintLinef (movie_is_capturing ? "status: movie capturing" : "status: not capturing");
		return;
	}

	if (cls.capturedemo)
	{
		Con_PrintLinef ("Can't capturevideo toggle, capturedemo running");
		return;
	}

	if (movie_is_capturing)
	{
		Movie_Stop_Capturing ();
	}
	else
	{
//		byte	*buffer;
		char	aviname[MAX_QPATH_64];
		char	checkname[MAX_OSPATH];
		char	barename[MAX_QPATH_64] = "video";
		int		i;

		if (cl.worldmodel)
			File_URL_Copy_StripExtension (barename, File_URL_SkipPath(cl.worldmodel->name) /* skip maps*/, sizeof(barename) );

	// find a file name to save it to
		for (i = 0; i < 10000; i++)
		{
			c_snprintf2 (aviname, "%s%04d.avi", barename, i);
			FS_FullPath_From_QPath (checkname, aviname);
			if (!File_Exists (checkname))
				break;	// file doesn't exist
		}
		if (i == 10000)
		{
			Con_PrintLinef ("Movie_Capture_Toggle_f: Couldn't find an unused filename");
			return;
 		}

		Movie_Start_Capturing (aviname);
	}

}




void Movie_StopPlayback (void)
{
	if (!cls.capturedemo)
		return;

	cls.capturedemo = false;
	Movie_Stop ();

	// If +capturedemo in command line, we exit after demo capture
	// completed (even if failed .. and this is failure location here)

	if (cls.capturedemo_and_exit)
		Host_Quit ();

}

double Movie_FrameTime (void)
{
	double	time;

	if (capture_fps.value > 0)
		time = !capture_hack.value ? 1.0 / capture_fps.value : 1.0 / (capture_fps.value * (capture_hack.value + 1.0));
	else
		time = 1.0 / 30.0;
	return CLAMP (1.0 / 1000, time, 1.0);
}

void Movie_UpdateScreen (void)
{
	int	size = clwidth * clheight * RGB_3; // vid.screen.width * vid.screen.height * 3;
#pragma message ("Baker: GL gets from ReadPixels, which can be specified.  WinQuake gets from vid.buffer")
	byte	*buffer;

	if (!Movie_IsActive())
		return;

	if (capture_hack.value)
	{
		if (hack_ctr != capture_hack.value)
		{
			if (!hack_ctr)
				hack_ctr = capture_hack.value;
			else
				hack_ctr--;
			return;
		}
		hack_ctr--;
	}

	buffer = malloc (size);
#ifdef GLQUAKE_VIDBUFFER_ACCESS
	{
		byte	temp;
		int i;
		// Baker: GL allows us to read a block of pixels so x,y-w,h is fine
		//eglPixelStorei (GL_PACK_ALIGNMENT, 1);  Moved to GL_SetupState
		eglReadPixels (clx, cly, clwidth, clheight, GL_RGB, GL_UNSIGNED_BYTE, buffer);
		//	ApplyGamma (buffer, size);  Baker: a thought

		for (i = 0 ; i < size ; i += 3)
		{
			temp = buffer[i];
			buffer[i] = buffer[i+2];
			buffer[i+2] = temp;
		}
	}
#endif // GLQUAKE_VIDBUFFER_ACCESS

#ifdef WINQUAKE_VIDBUFFER_ACCESS
	{
		int	i, j, rowp;
		byte *p = buffer;

		// Baker: For a resizable software renderer window with margins
		// We would have to modify this
		for (i = clheight - 1 ; i >= 0 ; i--)
		{
			rowp = i * vid.rowbytes;
			for (j = 0 ; j < clwidth ; j++)
			{
				*p++ = vid.curpal[vid.buffer[rowp]*3+2];
				*p++ = vid.curpal[vid.buffer[rowp]*3+1];
				*p++ = vid.curpal[vid.buffer[rowp]*3+0];
				rowp++;
			}
		}
	}
#endif // WINQUAKE_VIDBUFFER_ACCESS

	Capture_WriteVideo (buffer);

	free (buffer);
}

void Movie_TransferStereo16 (void)
{
	if (!Movie_IsActive())
		return;

	// Copy last audio chunk written into our temporary buffer
	memcpy (capture_audio_samples + (captured_audio_samples << 1), snd_out, snd_linear_count * shm->channels);
	captured_audio_samples += (snd_linear_count >> 1);

	if (captured_audio_samples >= c_rint (s_frametime * shm->speed))
	{
		// We have enough audio samples to match one frame of video
		Capture_WriteAudio (captured_audio_samples, (byte *)capture_audio_samples);
		captured_audio_samples = 0;
	}
}

cbool Movie_GetSoundtime (void)
{
	if (!Movie_IsActive())
		return false;

	soundtime += c_rint (s_frametime * shm->speed * (Movie_FrameTime() / s_frametime));

	return true;
}

#endif // Baker change +

void Movie_Init (void)
{
#ifdef SUPPORTS_AVI_CAPTURE // Baker change
	AVI_LoadLibrary ();
	if (!avi_loaded)
		return;
	
	captured_audio_samples = 0;
	
	ACM_LoadLibrary ();
	if (!acm_loaded)
		return;
	
	Cmd_AddCommands (Movie_Init);
#endif // SUPPORTS_AVI_CAPTURE
}

void CaptureCodec_Validate (cvar_t *var)
{
#ifdef SUPPORTS_AVI_CAPTURE // Baker change
	
	// Baker: We are going to assume an empty string was automatic codec
	if (capture_codec.string[0] == '0') // Begins with 0 ... set to auto
	{
		Cvar_SetQuick (&capture_codec, "auto");
		Con_PrintLinef ("%s set to " QUOTED_S, capture_codec.name, capture_codec.string);
	}
#endif // SUPPORTS_AVI_CAPTURE
	
}