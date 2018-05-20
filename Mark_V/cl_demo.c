/*
Copyright (C) 1996-2001 Id Software, Inc.
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

#include "quakedef.h"

static void CL_FinishTimeDemo (void);



typedef struct framepos_s
{
	long				baz;
	struct framepos_s	*next;
} framepos_t;

framepos_t	*dem_framepos = NULL;
cbool	start_of_demo = false;
cbool	bumper_on = false;

/*
==============================================================================

DEMO CODE

When a demo is playing back, all NET_SendMessages are skipped, and
NET_GetMessages are read from the demo file.

Whenever cl.time gets past the last received message, another message is
read from the demo file.
==============================================================================
*/

// from ProQuake: space to fill out the demo header for record at any time
static byte	demo_head[3][MAX_MARK_V_MSGLEN];
static int		demo_head_size[2];




/*
==============
CL_StopPlayback

Called when a demo file runs out, or the user starts a game
==============
*/

void CL_StopPlayback (void)
{
	if (!cls.demoplayback)
		return;

	FS_fclose (cls.demofile);

	cls.demoplayback = false;
	cls.demofile = NULL;
	cls.state = ca_disconnected;

#ifdef BUGFIX_DEMO_RECORD_NOTHING_FIX
	SCR_EndLoadingPlaque ();
#endif // BUGFIX_DEMO_RECORD_NOTHING_FIX

#ifdef SUPPORTS_CUTSCENE_PROTECTION // Baker: Revert any cvars a demo set to the user's settings
	Cvar_Clear_Untrusted ();
#endif // SUPPORTS_CUTSCENE_PROTECTION

	// If a dz temp file demo is playing (a .dem extracted from a .dz), delete it 
	if (cls.dz_temp_url[0])
	{
		File_Delete (cls.dz_temp_url);
		cls.dz_temp_url[0] = 0;
	}

	if (cls.timedemo)
		CL_FinishTimeDemo ();

#ifdef SUPPORTS_AVI_CAPTURE
	if (cls.capturedemo)
		VID_Local_Set_Window_Caption (NULL); // Restores it to default of "engine name"

	Movie_StopPlayback ();
#endif

	if (cls.titledemo)
	{
		vid.recalc_refdef = true; // also triggers sbar refresh
		cls.titledemo = 0;
	}
}

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length and view angles
====================
*/
static void CL_WriteDemoMessage (void)
{
	int		len;
	int		i;
	float	f;

	len = LittleLong (net_message.cursize);
	fwrite (&len, 4 /*sizeof int*/, 1, cls.demofile);

	for (i=0 ; i<3 ; i++)
	{
		f = LittleFloat (cl.viewangles[i]);
		fwrite (&f, 4 /*sizeof float*/, 1, cls.demofile);
	}

	fwrite (net_message.data, net_message.cursize, 1, cls.demofile);
	fflush (cls.demofile);
}

void PushFrameposEntry (long fbaz)
{
	framepos_t	*newf;

	newf = (framepos_t *)malloc (sizeof(framepos_t)); // Demo rewind
	newf->baz = fbaz;

	if (!dem_framepos)
	{
		newf->next = NULL;
		start_of_demo = false;
	}
	else
	{
		newf->next = dem_framepos;
	}

	dem_framepos = newf;
}


static void EraseTopEntry (void)
{
	framepos_t	*top;

	top = dem_framepos;
	dem_framepos = dem_framepos->next;
	if (top) free (top);
}


/*
====================
CL_GetMessage

Handles recording and playback of demos, on top of NET_ code
====================
*/

int CL_GetMessage (void)
{
	int		r, i;
	float	f;

	if (cl.paused & 2)
		return 0;

	if (cls.demoplayback)
	{
		if (start_of_demo && cls.demorewind)
			return 0;

		if (cls.signon < SIGNONS)	// clear stuffs if new demo
			while (dem_framepos)
				EraseTopEntry ();

	// decide if it is time to grab the next message
		if (cls.signon == SIGNONS)	// always grab until fully connected
		{
			if (cls.timedemo)
			{
				if (host_framecount == cls.td_lastframe)
					return 0;		// already read this frame's message

				cls.td_lastframe = host_framecount;

			// if this is the second frame, grab the real td_starttime
			// so the bogus time on the first frame doesn't count
				if (host_framecount == cls.td_startframe + 1)
					cls.td_starttime = realtime;
			}
			else if (!cls.demorewind && cl.ctime <= cl.mtime[0])
				return 0;		// don't need another message yet
			else if (cls.demorewind && cl.ctime >= cl.mtime[0])
				return 0;

			// joe: fill in the stack of frames' positions
			// enable on intermission or not...?
			// NOTE: it can't handle fixed intermission views!
			if (!cls.demorewind /*&& !cl.intermission*/)
				PushFrameposEntry (ftell(cls.demofile));

		}

	// get the next message
		cls.demo_offset_current = ftell(cls.demofile);
		fread (&net_message.cursize, 4, 1, cls.demofile);
		VectorCopy (cl.mviewangles[0], cl.mviewangles[1]);

		for (i=0 ; i < 3 ; i++)
		{
			r = fread (&f, 4, 1, cls.demofile);
			cl.mviewangles[0][i] = LittleFloat (f);
		}

		net_message.cursize = LittleLong (net_message.cursize);

		if (net_message.cursize > MAX_MARK_V_MSGLEN)
			Host_Error ("Demo message > MAX_MARK_V_MSGLEN");

		r = fread (net_message.data, net_message.cursize, 1, cls.demofile);

		if (r != 1)
		{
			CL_StopPlayback ();
			return 0;
		}

		// joe: get out framestack's top entry
		if (cls.demorewind /*&& !cl.intermission*/)
		{
			if (dem_framepos/* && dem_framepos->baz*/)	// Baker: in theory, if this occurs we ARE at the start of the demo with demo rewind on
			{
				fseek (cls.demofile, dem_framepos->baz, SEEK_SET);
				EraseTopEntry (); // Baker: we might be able to improve this better but not right now.
			}
			if (!dem_framepos)
				bumper_on = start_of_demo = true;
		}

		return 1;
	}

	while (1)
	{
		r = NET_GetMessage (cls.netcon);

		if (r != 1 && r != 2)
			return r;

	// discard nop keepalive message
		if (net_message.cursize == 1 && net_message.data[0] == svc_nop)
			Con_PrintLinef ("<-- server to client keepalive");
// Could have svc_download handled here to keep it out of demo file? 
		else
			break;
	}

	if (cls.demorecording)
		CL_WriteDemoMessage ();

	if (cls.signon < 2)
	{
	// record messages before full connection, so that a
	// demo record can happen after connection is done
		memcpy(demo_head[cls.signon], net_message.data, net_message.cursize);
		demo_head_size[cls.signon] = net_message.cursize;
	}

	return r;
}


/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f (lparse_t *unused)
{
	if (cmd_source != src_command)
		return;

	if (!cls.demorecording)
	{
		Con_PrintLinef ("Not recording a demo.");
		return;
	}

// write a disconnect message to the demo file
	SZ_Clear (&net_message);
	MSG_WriteByte (&net_message, svc_disconnect);
	CL_WriteDemoMessage ();

// finish up
	FS_fclose (cls.demofile);
	Recent_File_Set_FullPath (cls.demo_url); // Close demo instance #2 (record end)

	cls.demofile = NULL;
	cls.demorecording = false;
	Lists_Update_Demolist ();
	if (!cls.autodemo)
		Con_PrintLinef ("Completed demo %s, type " QUOTEDSTR ("showfile") " to open folder", File_URL_SkipPath (cls.demo_url));
	else
	{
		Con_DPrintLinef ("Completed demo %s", File_URL_SkipPath (cls.demo_url) );
		cls.autodemo = false;
	}
}

void CL_Clear_Demos_Queue (void)
{
	int i;
	for (i = 0;i < MAX_DEMOS_32; i ++)	// Clear demo loop queue
		cls.demos[i][0] = 0;
	cls.demonum = -1;				// Set next demo to none
}
#pragma message ("All the demo code is ugly")
#pragma message ("All rendering code ugly")
#pragma message ("All disconnect and cls.maprunning code is ugly")

/*
====================
CL_Record_f

record <demoname> <map> [cd track]
====================
*/
void CL_Record_f (lparse_t *line)
{
	int			c;
	char		record_name_url[MAX_OSPATH];
	int			track;
	cbool	is_automatic = false;

	if (cmd_source != src_command)
		return;

	if (cls.demoplayback)
	{
		Con_PrintLinef ("Can't record during demo playback");
		return;
	}

	c = line->count;
	if (c != 2 && c != 3 && c != 4)
	{
		Con_PrintLinef ("record <demoname> [<map> [cd track]]");
		return;
	}

	if (strstr(line->args[1], ".."))
	{
		Con_PrintLinef ("Relative pathnames are not allowed.");
		return;
	}

	if (c == 2 && cls.state == ca_connected)
	{
#if 0
		Con_PrintLinef ("Can not record - already connected to server" NEWLINE "Client demo recording must be started before connecting");
		return;
#endif
		if (cls.signon < 2)
		{
			Con_PrintLinef ("Can't record - try again when connected");
			return;
		}
	}

	if (cls.demorecording)
		CL_Stop_f(NULL);

// write the forced cd track number, or -1
	if (c == 4)
	{
		track = atoi(line->args[3]);
		Con_PrintLinef ("Forcing CD track to %d", cls.forcetrack);
	}
	else
	{
		track = -1;
	}

	if (!strcmp(AUTO_DEMO_NAME, line->args[1])  )
	{
		// Automatic demo
		char autodemo2_url[MAX_OSPATH];
		char autodemo1_url[MAX_OSPATH];
		char autodemo0_url[MAX_OSPATH];
		int checkit;

		FS_FullPath_From_QPath (autodemo2_url, "auto_demo_2.dem");
		FS_FullPath_From_QPath (autodemo1_url, "auto_demo_1.dem");
		FS_FullPath_From_QPath (autodemo0_url, "auto_demo_0.dem");

		if (File_Exists (autodemo2_url))
			checkit = File_Delete (autodemo2_url);
		if (File_Exists (autodemo1_url))
			checkit = File_Rename (autodemo1_url, autodemo2_url);
		if (File_Exists (autodemo0_url))
			checkit = File_Rename (autodemo0_url, autodemo1_url);

		c_strlcpy (record_name_url, autodemo0_url);

		cls.autodemo = is_automatic = true;
	}
	else FS_FullPath_From_QPath (record_name_url, line->args[1]);

	CL_Clear_Demos_Queue (); // timedemo is a very intentional action

// start the map up

	if (c > 2)
	{
		Cmd_ExecuteString ( va("map %s", line->args[2]), src_command);
		if (cls.state != ca_connected)
			return;
	}

// open the demo file
	File_URL_Edit_Force_Extension (record_name_url, ".dem", sizeof(record_name_url));

	if (!is_automatic)
		Con_PrintLinef ("recording to %s", record_name_url);
	else Con_DPrintLinef ("recording to %s", record_name_url);

	cls.demofile = FS_fopen_write (record_name_url, "wb");
	if (!cls.demofile)
	{
		Con_PrintLinef ("ERROR: couldn't create %s", File_URL_SkipPath(record_name_url));
		return;
	}

	// Officially recording ... copy the name for reference
	c_strlcpy (cls.demo_url, record_name_url);

	cls.forcetrack = track;
	fprintf (cls.demofile, "%d\n", cls.forcetrack);

	cls.demorecording = true;

	// from ProQuake: initialize the demo file if we're already connected
	if (c == 2 && cls.state == ca_connected)
	{
		byte *data = net_message.data;
		int cursize = net_message.cursize;
		int i;

		for (i = 0 ; i < 2 ; i++)
		{
			net_message.data = demo_head[i];
			net_message.cursize = demo_head_size[i];
			CL_WriteDemoMessage();
		}

		net_message.data = demo_head[2];
		SZ_Clear (&net_message);

		// current names, colors, and frag counts
		for (i=0 ; i < cl.maxclients ; i++)
		{
			MSG_WriteByte (&net_message, svc_updatename);
			MSG_WriteByte (&net_message, i);
			MSG_WriteString (&net_message, cl.scores[i].name);
			MSG_WriteByte (&net_message, svc_updatefrags);
			MSG_WriteByte (&net_message, i);
			MSG_WriteShort (&net_message, cl.scores[i].frags);
			MSG_WriteByte (&net_message, svc_updatecolors);
			MSG_WriteByte (&net_message, i);
			MSG_WriteByte (&net_message, cl.scores[i].colors);
		}

		// send all current light styles
		for (i = 0 ; i < MAX_LIGHTSTYLES ; i++)
		{
			MSG_WriteByte (&net_message, svc_lightstyle);
			MSG_WriteByte (&net_message, i);
			MSG_WriteString (&net_message, cl.lightstyle[i].map);
		}

		// what about the CD track or SVC fog... future consideration.
		MSG_WriteByte (&net_message, svc_updatestat);
		MSG_WriteByte (&net_message, STAT_TOTALSECRETS);
		MSG_WriteLong (&net_message, cl.stats[STAT_TOTALSECRETS]);

		MSG_WriteByte (&net_message, svc_updatestat);
		MSG_WriteByte (&net_message, STAT_TOTALMONSTERS);
		MSG_WriteLong (&net_message, cl.stats[STAT_TOTALMONSTERS]);

		MSG_WriteByte (&net_message, svc_updatestat);
		MSG_WriteByte (&net_message, STAT_SECRETS);
		MSG_WriteLong (&net_message, cl.stats[STAT_SECRETS]);

		MSG_WriteByte (&net_message, svc_updatestat);
		MSG_WriteByte (&net_message, STAT_MONSTERS);
		MSG_WriteLong (&net_message, cl.stats[STAT_MONSTERS]);

		// view entity
		MSG_WriteByte (&net_message, svc_setview);
		MSG_WriteShort (&net_message, cl.viewentity_player);

		// signon
		MSG_WriteByte (&net_message, svc_signonnum);
		MSG_WriteByte (&net_message, 3);

		CL_WriteDemoMessage();

		// restore net_message
		net_message.data = data;
		net_message.cursize = cursize;
	}
}


/*
====================
CL_PlayDemo_f

play [demoname]
====================
*/



// Baker: So we know this is a real start demo
cbool play_as_start_demo = false;
void CL_PlayDemo_NextStartDemo_f (lparse_t *line)
{
	play_as_start_demo = true;
	CL_PlayDemo_f (line); // Inherits the cmd_argc and cmd_argv
	play_as_start_demo = false;
}

// cls.demofile is already opened, just pass us the name
void CL_PlayDemo_Opened (const char *in_demo_name_url)
{
	int c;
	cbool neg = false;

	c_strlcpy (cls.demo_url, in_demo_name_url);

	// Revert
	cls.demorewind = false;
	cls.demospeed = 0; // 0 = Don't use
	bumper_on = false;

	cls.demo_offset_start = ftell (cls.demofile);	// qfs_lastload.offset instead?
	cls.demo_file_length = com_filesize;
	cls.demo_hosttime_start	= cls.demo_hosttime_elapsed = 0; // Fill this in ... host_time;
	cls.demo_cltime_start = cls.demo_cltime_elapsed = 0; // Fill this in

	// Title demos get no HUD, no crosshair, no con_notify, FOV 90, viewsize 120
	if (COM_ListMatch (cl_titledemos_list.string, cls.demo_url))
	{
		vid.recalc_refdef = true;
		cls.titledemo = true;
	}

	if (!play_as_start_demo)
	{
		CL_Clear_Demos_Queue ();
		SCR_BeginLoadingPlaque_Force_NoTransition ();
		Key_SetDest (key_game);
		console1.visible_pct = 0;
	}

	cls.demoplayback = true;
	cls.state = ca_connected;
	cls.forcetrack = 0;

	while ((c = getc(cls.demofile)) != '\n')
		if (c == '-')
			neg = true;
		else
			cls.forcetrack = cls.forcetrack * 10 + (c - '0');

	if (neg)
		cls.forcetrack = -cls.forcetrack;
// ZOID, fscanf is evil
//	fscanf (cls.demofile, "%d\n", &cls.forcetrack);

}


#ifndef SERVER_ONLY
void CL_PlayDZDemo (const char *dz_quake_folder_url)
{
	extern int dzip_runner(int argc, char **argv);
	
	char safedir_dem_url[MAX_OSPATH];	// c:\Users\ ....\Roaming\...\mydem.dem
	struct arg_buckets_64_s argbuckets_dz = {0};
	int expected_dem_exists;
	//cls.dz_handle = System_Process_Create (PATH_TO_DZIP, cmdline, com_safedir);    
	
	cls.dz_temp_url[0] = 0;

	// check if the file exists
	if (!File_Exists(dz_quake_folder_url))
	{
		Con_PrintLinef ("ERROR: couldn't open %s", File_URL_SkipPath(dz_quake_folder_url));
		return;
	}

	// 2. Construct the safedir url
	c_snprintf2 (safedir_dem_url, "%s/%s", com_safedir, File_URL_SkipPath(dz_quake_folder_url));
	File_URL_Edit_Change_Extension (safedir_dem_url, ".dem", sizeof(safedir_dem_url));

	// 4.  If we are here, we have work to do.
	Con_PrintLinef ("\x02" "\nunpacking demo. please wait..." NEWLINE);
	
	FS_SafeDirClean (); // Remove anything in our safe dir
	File_Chdir (com_safedir); // Change directory to our safe directory.
	c_snprintf1 (argbuckets_dz.cmdline, "anything -x -f " QUOTED_S, dz_quake_folder_url);
	String_To_Arg_Buckets (&argbuckets_dz, argbuckets_dz.cmdline);
	dzip_runner (argbuckets_dz.argcount, argbuckets_dz.argvs);

	// 5. Ran DZip now see if we got the expected file.
	expected_dem_exists = File_Exists(safedir_dem_url);
	cls.demofile = expected_dem_exists ? FS_fopen_read(safedir_dem_url, "rb") : NULL;

	if (!cls.demofile)
	{
		const char *msg_no_exist = "File " QUOTED_S " was not able to be extracted from archive";
		const char *msg_exists_couldnt_play  = "ERROR: couldn't open " QUOTED_S;
		const char *msg = expected_dem_exists ? msg_exists_couldnt_play : msg_no_exist;
		Con_PrintLinef (msg, File_URL_SkipPath(safedir_dem_url));
		cls.demonum = -1;
		FS_SafeDirClean (); // We are done with the safedir temps
		return; // Get out, right?
	}
		
	// start playback
	cls.demoplayback = true;
	com_filesize = File_Length(safedir_dem_url); // Outside the file system!!
	CL_PlayDemo_Opened (safedir_dem_url);
	c_strlcpy (cls.dz_temp_url, safedir_dem_url); // old-->new  ... Rename is a move
}
#endif // !SERVER_ONLY


void CL_PlayDemo_f (lparse_t *line)
{
	char	playdemo_name_qpath[MAX_QPATH_64];
	char	playdemo_name_url[MAX_OSPATH];

	if (cmd_source != src_command)
		return;

	if (line->count != 2)
	{
		Con_PrintLinef ("playdemo <demoname> : plays a demo");
		return;
	}

// disconnect from server
	CL_Disconnect ();

// open the demo file
	FS_FullPath_From_QPath (playdemo_name_url, line->args[1]);

#ifndef SERVER_ONLY
	// If a dzipped demo ending in .dz handling playing differently
	if (String_Does_End_With (playdemo_name_url, ".dz"))
	{
		CL_PlayDZDemo (playdemo_name_url);
		return;
	}
#endif // SERVER_ONLY
	
	c_strlcpy (playdemo_name_qpath, line->args[1]);
	File_URL_Edit_Force_Extension (playdemo_name_qpath, ".dem", sizeof(playdemo_name_qpath));
	Con_PrintLinef ("Playing demo from %s.", playdemo_name_qpath);

	COM_FOpenFile (playdemo_name_qpath, &cls.demofile);
	if (!cls.demofile)
	{
		Con_PrintLinef ("ERROR: couldn't open %s", playdemo_name_qpath);
		cls.demonum = -1;		// stop demo loop
		return;
	}

	CL_PlayDemo_Opened (playdemo_name_qpath);
}

/*
====================
CL_FinishTimeDemo

====================
*/
static void CL_FinishTimeDemo (void)
{
	int		frames;
	float	time;

	cls.timedemo = false;

// the first frame didn't count
	frames = (host_framecount - cls.td_startframe) - 1;
	time = realtime - cls.td_starttime;

	if (!time)
		time = 1;

	Con_PrintLinef ("%d frames %5.1f seconds %5.1f fps", frames, time, frames/time);
}

/*
====================
CL_TimeDemo_f

timedemo [demoname]
====================
*/
void CL_TimeDemo_f (lparse_t *line)
{
	if (cmd_source != src_command)
		return;

	if (line->count != 2)
	{
		Con_PrintLinef ("timedemo <demoname> : gets demo speeds");
		return;
	}


	CL_Clear_Demos_Queue (); // timedemo is a very intentional action

	CL_PlayDemo_f (line);

	// don't trigger timedemo mode if playdemo fails
	if (!cls.demofile)
		return;

// cls.td_starttime will be grabbed at the second frame of the demo, so
// all the loading time doesn't get counted

	cls.timedemo = true;
	cls.td_startframe = host_framecount;
	cls.td_lastframe = -1;		// get a new message this frame

	// Baker: This is a performance benchmark.  No reason to have console up.
	SCR_BeginLoadingPlaque_Force_NoTransition ();
	Key_SetDest (key_game);
	console1.visible_pct = 0;
}