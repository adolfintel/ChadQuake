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
// cl_parse.c  -- parse a message received from the server

#include "quakedef.h"
#include "net_admin.h"

const char *svc_strings[] =
{
	"svc_bad",
	"svc_nop",
	"svc_disconnect",
	"svc_updatestat",
	"svc_version",			// [long] server version
	"svc_setview",			// [short] entity number
	"svc_sound",			// <see code>
	"svc_time",				// [float] server time
	"svc_print",			// [string] null terminated string
	"svc_stufftext",		// [string] stuffed into client's console buffer
							// the string should be \n terminated
	"svc_setangle",			// [vec3] set the view angle to this absolute value

	"svc_serverinfo",		// [long] version
							// [string] signon string
							// [string]..[0]model cache [string]...[0]sounds cache
							// [string]..[0]item cache
	"svc_lightstyle",		// [byte] [string]
	"svc_updatename",		// [byte] [string]
	"svc_updatefrags",		// [byte] [short]
	"svc_clientdata",		// <shortbits + data>
	"svc_stopsound",		// <see code>
	"svc_updatecolors",		// [byte] [byte]
	"svc_particle",			// [vec3] <variable>
	"svc_damage",			// [byte] impact [byte] blood [vec3] from

	"svc_spawnstatic",
	"OBSOLETE svc_spawnbinary",
	"svc_spawnbaseline",

	"svc_temp_entity",		// <variable>
	"svc_setpause",
	"svc_signonnum",
	"svc_centerprint",
	"svc_killedmonster",
	"svc_foundsecret",
	"svc_spawnstaticsound",
	"svc_intermission",
	"svc_finale",						// [string] music [string] text
	"svc_cdtrack",						// [byte] track [byte] looptrack
	"svc_sellscreen",
	"svc_cutscene",
//johnfitz -- new server messages
	"svc_showlmp",						// NEHAHRA: [string] iconlabel [string] lmpfile [byte] x [byte] y
	"svc_hidelmp",						// NEHAHRA: [string] iconlabel
	"svc_skybox", // 37					// [string] skyname
	"", // 38
	"", // 39
	"svc_bf", // 40						// no data
	"svc_fog", // 41					// [byte] density [byte] red [byte] green [byte] blue [float] time
	"svc_spawnbaseline2", //42			// support for large modelindex, large framenum, alpha, using flags
	"svc_spawnstatic2", // 43			// support for large modelindex, large framenum, alpha, using flags
	"svc_spawnstaticsound2", //	44		// [coord3] [short] samp [byte] vol [byte] aten
	"", // 44
	"", // 45
	"", // 46
	"", // 47
	"", // 48
	"", // 49
//johnfitz
};

cbool warn_about_nehahra_protocol; //johnfitz


//=============================================================================

/*
===============
CL_EntityNum

This error checks and tracks the total number of entities
===============
*/
entity_t	*CL_EntityNum (int num)
{
	//johnfitz -- check minimum number too
	if (num < 0)
		Host_Error ("CL_EntityNum: %d is an invalid number",num);
	//john

	if (num >= cl.num_entities)
	{
		if (num >= cl_max_edicts) //johnfitz -- no more MAX_EDICTS
			Host_Error ("CL_EntityNum: %d is an invalid number",num);

		while (cl.num_entities <= num)
		{
#ifdef GLQUAKE_COLORMAP_TEXTURES
			cl_entities[cl.num_entities].colormap = 0; // Baker: No color map
#endif // GLQUAKE_COLORMAP_TEXTURES

#ifdef WINQUAKE_COLORMAP_TRANSLATION
			cl_entities[cl.num_entities].colormap = vid.colormap;
#endif // WINQUAKE_COLORMAP_TRANSLATION
			cl_entities[cl.num_entities].lerpflags |= LERP_RESETMOVE|LERP_RESETANIM; //johnfitz
			cl.num_entities++;
		}
	}

	return &cl_entities[num];
}


/*
==================
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket(void)
{
    vec3_t  pos;
    int 	channel, ent;
    int 	sound_num;
    int 	volume;
    int 	field_mask;
    float 	attenuation;
 	int		i;

    field_mask = MSG_ReadByte();

    if (field_mask & SND_VOLUME)
		volume = MSG_ReadByte ();
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;

    if (field_mask & SND_ATTENUATION)
		attenuation = MSG_ReadByte () / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;

	//johnfitz -- PROTOCOL_FITZQUAKE
	if (field_mask & SND_LARGEENTITY)
	{
		ent = (unsigned short) MSG_ReadShort ();
		channel = MSG_ReadByte ();
	}
	else
	{
		channel = (unsigned short) MSG_ReadShort ();
		if (cl.protocol == PROTOCOL_BJP3) // Baker: BJP
			ent = (channel & 0x7FFF) >> 3;
		else ent = channel >> 3;
		channel &= 7;
	}

	if (field_mask & SND_LARGESOUND)
		sound_num = (unsigned short) MSG_ReadShort ();
	else
		if (cl.protocol == PROTOCOL_BJP3)
			sound_num = MSG_ReadShort ();
		else sound_num = MSG_ReadByte ();

	//johnfitz

	//johnfitz -- check soundnum
	if (sound_num >= MAX_FITZQUAKE_SOUNDS)
		Host_Error ("CL_ParseStartSoundPacket: %d > MAX_FITZQUAKE_SOUNDS", sound_num);
	//johnfitz

	if (ent > cl_max_edicts) //johnfitz -- no more MAX_EDICTS
		Host_Error ("CL_ParseStartSoundPacket: ent = %d", ent);

	for (i=0 ; i<3 ; i++)
		pos[i] = MSG_ReadCoord ();

    S_StartSound (ent, channel, cl.sound_precache[sound_num], pos, volume/255.0, attenuation);
}

/*
==================
CL_KeepaliveMessage

When the client is taking a long time to load stuff, send keepalive messages
so the server doesn't disconnect.
==================
*/
static byte	net_olddata[NET_MARK_V_MAXMESSAGE];
void CL_KeepaliveMessage (void)
{
	float	time;
	static float lastmsg;
	int		ret;
	sizebuf_t	old;
	byte	*olddata;

	if (sv.active)
		return;		// no need if server is local

	if (cls.demoplayback)
		return;

// read messages from server, should just be nops
	olddata = net_olddata;
	old = net_message;
	memcpy (olddata, net_message.data, net_message.cursize);

	do
	{
		ret = CL_GetMessage ();

		switch (ret)
		{
		default:
			Host_Error ("CL_KeepaliveMessage: CL_GetMessage failed");

		case 0:
			break;	// nothing waiting

		case 1:
			Host_Error ("CL_KeepaliveMessage: received a message");
			break;

		case 2:
			if (MSG_ReadByte() != svc_nop)
				Host_Error ("CL_KeepaliveMessage: datagram wasn't a nop");

			break;
		}
	} while (ret);

	net_message = old;
	memcpy (net_message.data, olddata, net_message.cursize);

// check time
	time = System_DoubleTime ();

	if (time - lastmsg < 5)
		return;

	lastmsg = time;

// write out a nop
	Con_PrintLinef ("--> client to server keepalive");

	MSG_WriteByte (&cls.message, clc_nop);
	NET_SendMessage (cls.netcon, &cls.message);
	SZ_Clear (&cls.message);
}

#ifdef SUPPORTS_PQ_CL_HTTP_DOWNLOAD

static cbool Install_Command_Progress (void *id, int old_total, int new_total)
{
	float percent = cls.download.total_bytes > 0 ?  (float)new_total / cls.download.total_bytes : 0;
	float newpct = CLAMP(0, percent, 1);

	if (newpct - cls.download.percent > 0.005)
	{
		// Every once in a while update the screen
		cls.download.percent = newpct;

		System_SendKeyEvents ();	// Check key events
		SCR_UpdateScreen ();		// Hmmm.
		key_count = -1;				// Necessary?

		if (key_lastpress == K_ESCAPE)
			return (cls.download.user_cancelled = true); // Bail!
	}

	{ 	// System frames aren't actually running while we download.
		// Or are they?  This is similar to modal dialog box.
		static double oldtime;
		double newtime = System_DoubleTime ();
		double timeslice = newtime - oldtime;

		if (!sv.active || !cls.demoplayback)
			CL_KeepaliveMessage();

		Host_Frame (timeslice);
		oldtime = newtime;

		return cls.download.disconnect; // abort if disconnect received
	}
}


// File to download is like progs/g_rock2.mdl or maps/intro.bsp ... no leading
char *VersionString (void);
cbool CL_Download_Attempt (const char *file_to_download)
{
	char local_tempname_url[MAX_OSPATH];
	char remote_url[SYSTEM_STRING_SIZE_1024];
	char download_finalname_url[MAX_OSPATH];
	cbool is_success;
	int errorcode = -1; //
	int svr_pq_version;
	cbool is_proquake_server_35; // Download if we are connected to Quakespasm, DarkPlaces, ProQuake.  Don't download if Mark V or Quakespasm Spiked.

	// Don't download while playing back a demo or if we are the server.
	if (cls.demoplayback || sv.active || !pq_download_http.value || !pq_download_http_url.string[0])
		return false;

	svr_pq_version = NET_QSocketIsProQuakeServer (cls.netcon);
	is_proquake_server_35 = !in_range (30, svr_pq_version, 34);

	// Only download if the server is ProQuake version 3.5 or greater
 	if (!is_proquake_server_35)
		return false;

	// Would do a gamedir check here, but that's on the user.

	// Local temp file for download; make the path in case it doesn't exist
	c_snprintf2 (local_tempname_url,     "%s/%s.tmp", com_gamedir, file_to_download);		// id1/maps/arenax.bsp.tmp  // gamedir_shortname()
	c_snprintf2 (download_finalname_url, "%s/%s"    , com_gamedir, file_to_download);		// id1/maps/arenax.bsp

	// FS_fopen_write_create_path
	//	COM_CreatePath (local_tempname);

	// Remote URL for download
	c_snprintf2 (remote_url, "http://%s/%s", pq_download_http_url.string, file_to_download);

	Con_PrintLinef ("HTTP downloading: %s (%s)", file_to_download, pq_download_http_url.string); // File_URL_SkipPath

	// CL_Download_After
	// We detect whether or not a download is going on solely by cls.download.name[0]
	if (cls.download.name[0])
		Host_Error ("Already downloading: %s", cls.download.name); // Can this happen?

	memset (&cls.download, 0, sizeof(cls.download));

	cls.download.percent = 0;
	cls.download.total_bytes = -1;
	cls.download.is_blocking = false;	// Map download is not blocking
	c_strlcpy (cls.download.name, file_to_download);

	SCR_EndLoadingPlaque ();

	// BEGIN THE DOWNLOAD

	//success = Web_Get(remote_url, NULL, local_tempname, false, 600, 30, CL_WebDownloadProgress);

	// Because of the nature of the internet.  We don't know if this file exists or we are getting some dumb "page not found" page.
	// So we can't trust the size or trust what we get to even be a map
	// However, we have a bit more control over this because a depot can have a standard.
	is_success = Download_To_File(VersionString(), remote_url, local_tempname_url, Install_Command_Progress, NULL, &cls.download.total_bytes, &errorcode);

	// Problems:  We get an "ok" if we get a 302.

	// Install_Download_After


	CL_KeepaliveMessage ();

	if (is_success && !cls.download.total_bytes) {
		is_success = false; // Not sure how this happens
	}

	memset (&cls.download, 0, sizeof(cls.download));

	// We get an error code 4 on invalid domain.
	if (!is_success) {
		File_Delete (local_tempname_url);  // Delete the temp file
		Con_PrintLinef ("HTTP download failed (%d): " QUOTED_S, errorcode, file_to_download );
	}

	if (is_success) {
		File_Rename (local_tempname_url, download_finalname_url /*newname*/);	// Rename it to the correct name
		Con_PrintLinef ("HTTP download success: " QUOTED_S, file_to_download );
	}

	// If we wanted to be super-thorough, we would disallow a number of things that can cause a disconnect.
	// Single player->new game, gamedir change, playing a demo, loading a game, and on and on.
	// Frankly, I'm not sure this is strictly necessary anyway.  If our download is wired correctly my new mechanism would clean up.
	// Anyways ...
	if (cls.download.disconnect) {
		// if the user type disconnect in the middle of the download
		cls.download.disconnect = false;
		CL_Disconnect_f (NULL);
		Con_PrintLinef ("HTTP download abort");
	}

	return is_success; // or failure
}


#endif // SUPPORTS_PQ_CL_HTTP_DOWNLOAD


/*
==================
CL_ParseServerInfo
==================
*/
void CL_ParseServerInfo (void)
{
	const char	*str;
	int		i;
	int		nummodels, numsounds;
	char	model_precache[MAX_FITZQUAKE_MODELS][MAX_QPATH_64];
	char	sound_precache[MAX_FITZQUAKE_SOUNDS][MAX_QPATH_64];

	// this function can call Con_Printf so explicitly wipe the particles in case Con_Printf
	// needs to call SCR_UpdateScreen.
	Con_DPrintLinef ("Serverinfo packet received.");


// parse protocol version number
	i = MSG_ReadLong ();

	//johnfitz -- support multiple protocols
	if (i != PROTOCOL_NETQUAKE && i != PROTOCOL_FITZQUAKE && i != PROTOCOL_FITZQUAKE_PLUS && i != PROTOCOL_BJP3)
	{
		Con_PrintLine (); //because there's no newline after serverinfo print
		Host_Error ("Server returned version %d, not %d, %d, %d or %d", i, PROTOCOL_NETQUAKE, PROTOCOL_FITZQUAKE, PROTOCOL_FITZQUAKE_PLUS, PROTOCOL_BJP3);
	}

// wipe the client_state_t struct
	CL_ClearState (i); // Baker --- Sets the protocol here

// parse maxclients
	cl.maxclients = MSG_ReadByte ();

	if (cl.maxclients < 1 || cl.maxclients > MAX_SCOREBOARD_16)
	{
		Host_Error ("Bad maxclients (%u) from server", cl.maxclients);
		return;
	}

	cl.scores = (scoreboard_t *)Hunk_AllocName (cl.maxclients*sizeof(*cl.scores), "scores");
	cl.pq_teamscores = Hunk_AllocName (14 * sizeof(*cl.pq_teamscores), "teamscor"); // JPG - for teamscore status bar

// parse gametype
	cl.gametype = MSG_ReadByte ();

// parse signon message
	str = MSG_ReadString ();
	c_strlcpy (cl.levelname, str);

// Baker: Clear level information prior to loading world.
	if (!sv.active)
		memset (&level, 0, sizeof(level));


// separate the printfs so the server message can have a color
	Con_PrintLine ();
	Con_PrintLinef ("%s", Con_Quakebar(40)); //johnfitz
	Con_PrintLinef ("%c%s", 2, str);

//johnfitz -- tell user which protocol this is
	Con_PrintLinef ("Using protocol %d", i);

// first we go through and touch all of the precache data that still
// happens to be in the cache, so precaching something else doesn't
// needlessly purge it

// precache models
#ifdef GLQUAKE_SUPPORTS_QMB
	for (i = 0; i < modelindex_max; i++)
		cl_modelindex[i] = -1;
#endif // GLQUAKE_SUPPORTS_QMB

	memset (cl.model_precache, 0, sizeof(cl.model_precache));

	for (nummodels = 1 ; ; nummodels++)
	{
		str = MSG_ReadString ();

		if (!str[0])
			break;

		if (nummodels == MAX_FITZQUAKE_MODELS)
		{
			Host_Error ("Server sent too many model precaches");
			return;
		}

		//if (nummodels == 76)
		//	alert ("76 is %s", str);

		strlcpy (model_precache[nummodels], str, MAX_QPATH_64);

#ifdef GLQUAKE_SUPPORTS_QMB // After the for loop
		if (qmb_is_available) {
			// Update the QMB model index for this model, if applicable.
			for (i = 0; i < modelindex_max; i++) {
				if (String_Does_Match (cl_modelnames[i], model_precache[nummodels])) {
					cl_modelindex[i] = nummodels;
					break;
				}
			}
		}
#endif // GLQUAKE_SUPPORTS_QMB

		Mod_TouchModel (str);
	}

	//johnfitz -- check for excessive models
	if (nummodels >= MAX_WINQUAKE_MODELS)
		Con_DWarningLine ("%d models exceeds standard limit of %d.", nummodels, MAX_WINQUAKE_MODELS); //256

//#ifdef GLQUAKE_SUPPORTS_QMB // After the for loop
//// joe: load the extra "no-flamed-torch" model  NOTE: this is an ugly hack
//// Baker: Causing the extra flame0.mdl to be inserted.
//	if (qmb_is_available) {
//		if (nummodels == MAX_FITZQUAKE_MODELS)
//		{
//			Con_PrintLinef ("Server sent too many model precaches -> replacing flame0.mdl with flame.mdl");
//			cl_modelindex[mi_flame0] = cl_modelindex[mi_flame1];
//		}
//		else
//		{
//			c_strlcpy (model_precache[nummodels], cl_modelnames[mi_flame0]);
//			cl_modelindex[mi_flame0] = nummodels++;
//		}
//	}
//#endif // GLQUAKE_SUPPORTS_QMB



	// precache sounds
	memset (cl.sound_precache, 0, sizeof(cl.sound_precache));

	for (numsounds = 1 ; ; numsounds++)
	{
		str = MSG_ReadString ();

		if (!str[0])
			break;

		if (numsounds == MAX_FITZQUAKE_SOUNDS)
		{
			Host_Error ("Server sent too many sound precaches");
			return;
		}

		strlcpy (sound_precache[numsounds], str, MAX_QPATH_64);
		S_TouchSound (str);
	}

	//johnfitz -- check for excessive sounds
	if (numsounds >= MAX_WINQUAKE_SOUNDS)
		Con_DWarningLine ("%d sounds exceeds standard limit of %d.", numsounds, MAX_WINQUAKE_SOUNDS); // 256

	// Baker: We need this early for external vis to know if a model is worldmodel or not
	File_URL_Copy_StripExtension (cl.worldname, File_URL_SkipPath (model_precache[1]), sizeof (cl.worldname) ); // Baker: maps/e1m1.bsp ---> e1m1

#if 1
// Baker: Preparing a list of files we might download from server.
// I think our plan was to use a download svc.
	{
		clist_t *list = NULL;
		for (i = 1 ; i < nummodels ; i++)
			if (model_precache[i][0] != '*')
				List_Add_No_Case_To_Lower (&list,  model_precache[i]);

		for (i = 1 ; i < numsounds ; i++)
			List_Add_No_Case_To_Lower (&list,  va ("sound/%s", sound_precache[i])  );

#ifdef CORE_PTHREADS
		Admin_Game_Files_List_Update_Client (list);
#endif // CORE_PTHREADS

		List_Free (&list); // Discard
	}
#endif

//
// now we try to load everything else until a cache allocation fails
//

	for (i = 1 ; i < nummodels ; i++)
	{
		// Baker: Using this location as an opportunity to warn about upper case being used in model names
		// Which is very toxic to Linux or any case-sensitive operating system
		COM_Uppercase_Check (model_precache[i]); // Will warn if upper case is used.

		cl.model_precache[i] = Mod_ForName (model_precache[i], false);
		if (cl.model_precache[i] == NULL)
		{
// DOWNLOAD START
#ifdef SUPPORTS_PQ_CL_HTTP_DOWNLOAD
			// Maybe try download process.
			if (CL_Download_Attempt (model_precache[i]))
			{
				// Download worked.
				i--; // Subtract 1 so we try this model again in next iteration
				continue;  // Bail on loop and resume
			}
#endif // SUPPORTS_PQ_CL_HTTP_DOWNLOAD

			Con_PrintLinef ("Model %s not found", model_precache[i]);
			return;  //don't disconnect, let them sit in console and ask for help.
		}

		CL_KeepaliveMessage ();
	}

	S_BeginPrecaching ();

	for (i = 1 ; i < numsounds ; i++)
	{
		cbool precached_worked = true;
		cl.sound_precache[i] = S_PrecacheSound (sound_precache[i], &precached_worked);
#ifdef SUPPORTS_PQ_CL_HTTP_DOWNLOAD
		if (precached_worked == false)
		{
// download start

			cbool download_try_worked = CL_Download_Attempt (va ("sound/%s", sound_precache[i]) ) ;

			if (download_try_worked)
				S_PrecacheSound_Again (cl.sound_precache[i]);
		}
#endif // SUPPORTS_PQ_CL_HTTP_DOWNLOAD
		COM_Uppercase_Check (sound_precache[i]); // Baker: Use this as a place to warn about dumbness
		CL_KeepaliveMessage ();
	}

	S_EndPrecaching ();

// local state
	cl_entities[0].model = cl.worldmodel = cl.model_precache[1];

	str = LOC_LoadLocations(); // NULL on success.  Pointer to qpath string on failure.

	// If no loc file and we download .locs, try to download it and if so load .loc again.
	if (str &&  pq_download_http_locs.value && CL_Download_Attempt (str))
		LOC_LoadLocations(); // Attempt #2

	R_NewMap ();
	Lists_Refresh_NewMap ();

	Hunk_Check_f (NULL);		// make sure nothing is hurt

//johnfitz -- reset developer stats
	memset(&dev_stats, 0, sizeof(dev_stats));
	memset(&dev_peakstats, 0, sizeof(dev_peakstats));
	memset(&dev_overflows, 0, sizeof(dev_overflows));
}

/*
==================
CL_ParseUpdate

Parse an entity update message from the server
If an entities model or origin changes from frame to frame, it must be
relinked.  Other attributes can change without relinking.
==================
*/
void CL_ParseUpdate (int bits)
{
	int			i;
	qmodel_t	*model;
	int			modnum;
	cbool	forcelink;
	entity_t	*ent;
	int			num;
	int			skin;

	if (cls.signon == SIGNONS - 1)
	{
		// first update is the final signon stage
		cls.signon = SIGNONS;
		CL_SignonReply ();
	}

	if (bits & U_MOREBITS)
	{
		i = MSG_ReadByte ();
		bits |= (i<<8);
	}

	//johnfitz -- PROTOCOL_FITZQUAKE
	if (cl.protocol == PROTOCOL_FITZQUAKE || cl.protocol == PROTOCOL_FITZQUAKE_PLUS)
	{
		if (bits & U_EXTEND1)
			bits |= MSG_ReadByte() << 16;

		if (bits & U_EXTEND2)
			bits |= MSG_ReadByte() << 24;
	}

	//johnfitz

	if (bits & U_LONGENTITY)
		num = MSG_ReadShort ();
	else
		num = MSG_ReadByte ();

	ent = CL_EntityNum (num);

// Baker: At this point we have the entity number.

	if (ent->msgtime != cl.mtime[1])
	{
		forcelink = true;	// no previous frame to lerp from
	}
	else forcelink = false;

	//johnfitz -- lerping
	if (ent->msgtime + 0.2 < cl.mtime[0]) //more than 0.2 seconds since the last message (most entities think every 0.1 sec)
		ent->lerpflags |= LERP_RESETANIM; //if we missed a think, we'd be lerping from the wrong frame
	//johnfitz

	ent->msgtime = cl.mtime[0];

	if (bits & U_MODEL)
	{
		if (cl.protocol == PROTOCOL_BJP3)
			modnum = MSG_ReadShort ();
		else
		modnum = MSG_ReadByte ();
		if (modnum >= MAX_FITZQUAKE_MODELS)
			Host_Error ("CL_ParseModel: bad modnum");
	}
	else modnum = ent->baseline.modelindex;

//#ifdef GLQUAKE_SUPPORTS_QMB
	ent->modelindex = modnum;
//#endif // GLQUAKE_SUPPORTS_QMB

	if (bits & U_FRAME)
		ent->frame = MSG_ReadByte ();
	else ent->frame = ent->baseline.frame;

	if (bits & U_COLORMAP)
		i = MSG_ReadByte();
	else i = ent->baseline.colormap;

	if (!i)
	{
#ifdef GLQUAKE_COLORMAP_TEXTURES
		ent->colormap = 0;
#endif // GLQUAKE_COLORMAP_TEXTURES

#ifdef WINQUAKE_COLORMAP_TRANSLATION
		ent->colormap = vid.colormap;
#endif // WINQUAKE_COLORMAP_TRANSLATION
	}
	else
	{
#if 0
		Con_PrintLinef ("ent: num %d colormap %d frame %d", num, i, ent->frame);
#endif
		if (i > cl.maxclients)
			Host_Error ("i >= cl.maxclients");
#ifdef GLQUAKE_COLORMAP_TEXTURES
		ent->colormap = i;
#endif // GLQUAKE_COLORMAP_TEXTURES
#ifdef WINQUAKE_COLORMAP_TRANSLATION
		ent->colormap = cl.scores[i-1].translations;
#endif // WINQUAKE_COLORMAP_TRANSLATION
	}

	if (bits & U_SKIN)
		skin = MSG_ReadByte();
	else skin = ent->baseline.skin;

	if (skin != ent->skinnum)
	{
		ent->skinnum = skin;
	}

	if (bits & U_EFFECTS)
		ent->effects = MSG_ReadByte();
	else ent->effects = ent->baseline.effects;

// shift the known values for interpolation
	VectorCopy (ent->msg_origins[0], ent->msg_origins[1]);
	VectorCopy (ent->msg_angles[0], ent->msg_angles[1]);

	if (bits & U_ORIGIN1)
		ent->msg_origins[0][0] = MSG_ReadCoord ();
	else ent->msg_origins[0][0] = ent->baseline.origin[0];

	if (bits & U_ANGLE1)
	{
		if (cl.protocol == PROTOCOL_FITZQUAKE_PLUS)
			ent->msg_angles[0][0] = MSG_ReadAngle16();
		else ent->msg_angles[0][0] = MSG_ReadAngle();
	}
	else ent->msg_angles[0][0] = ent->baseline.angles[0];

	if (bits & U_ORIGIN2)
		ent->msg_origins[0][1] = MSG_ReadCoord ();
	else ent->msg_origins[0][1] = ent->baseline.origin[1];

	if (bits & U_ANGLE2)
	{
		if (cl.protocol == PROTOCOL_FITZQUAKE_PLUS)
			ent->msg_angles[0][1] = MSG_ReadAngle16();
		else ent->msg_angles[0][1] = MSG_ReadAngle();
	}
	else ent->msg_angles[0][1] = ent->baseline.angles[1];

	if (bits & U_ORIGIN3)
		ent->msg_origins[0][2] = MSG_ReadCoord ();
	else ent->msg_origins[0][2] = ent->baseline.origin[2];

	if (bits & U_ANGLE3)
	{
		if (cl.protocol == PROTOCOL_FITZQUAKE_PLUS)
			ent->msg_angles[0][2] = MSG_ReadAngle16();
		else ent->msg_angles[0][2] = MSG_ReadAngle();
	}
	else ent->msg_angles[0][2] = ent->baseline.angles[2];

	//johnfitz -- lerping for movetype_step entities
	if ( bits & U_STEP )
	{
		ent->lerpflags |= LERP_MOVESTEP;
		ent->forcelink = true;
	}
	else ent->lerpflags &= ~LERP_MOVESTEP;
	//johnfitz

	//johnfitz -- PROTOCOL_FITZQUAKE and PROTOCOL_NEHAHRA
	if (cl.protocol == PROTOCOL_FITZQUAKE || cl.protocol == PROTOCOL_FITZQUAKE_PLUS)
	{
		if (bits & U_ALPHA)
			ent->alpha = MSG_ReadByte();
		else ent->alpha = ent->baseline.alpha;

		if (bits & U_FRAME2) ent->frame = (ent->frame & 0x00FF) | (MSG_ReadByte() << 8);
		if (bits & U_MODEL2) modnum = (modnum & 0x00FF) | (MSG_ReadByte() << 8);

		if (bits & U_LERPFINISH)
		{
			ent->lerpfinish = ent->msgtime + ((float)(MSG_ReadByte()) / 255);
			ent->lerpflags |= LERP_FINISH;
		}
		else ent->lerpflags &= ~LERP_FINISH;
	}
	else if (cl.protocol == PROTOCOL_NETQUAKE)
	{
		//HACK: if this bit is set, assume this is PROTOCOL_NEHAHRA
		if (bits & U_TRANS)
		{
			float a,b;

			if (!cl.warned_about_nehahra_protocol)
			{
#ifdef SUPPORTS_NEHAHRA
				if (!nehahra_active)
#endif // SUPPORTS_NEHAHRA
					Con_WarningLinef ("nonstandard update bit, assuming Nehahra protocol");
				cl.warned_about_nehahra_protocol = true;
			}

			a = MSG_ReadFloat();
			b = MSG_ReadFloat(); // alpha

			if (a == 2)
				MSG_ReadFloat(); // Baker: fullbright (not using this yet -- and Nehahra doesn't seem to use at all?)

			ent->alpha = ENTALPHA_ENCODE(b);
		}
		else ent->alpha = ent->baseline.alpha;
	}
	//johnfitz

	//johnfitz -- moved here from above
	model = cl.model_precache[modnum];

	if (model != ent->model)
	{
		ent->model = model;

	// automatic animation (torches, etc) can be either all together
	// or randomized
		if (model)
		{
			if (model->synctype == ST_RAND)
				ent->syncbase = (float)(rand()&0x7fff) / 0x7fff;
			else ent->syncbase = 0.0;
		}
		else forcelink = true;	// hack to make null model players work

		ent->lerpflags |= LERP_RESETANIM; //johnfitz -- don't lerp animation across model changes
	}
	//johnfitz

	if ( forcelink )
	{
		// didn't have an update last message
		VectorCopy (ent->msg_origins[0], ent->msg_origins[1]);
		VectorCopy (ent->msg_origins[0], ent->origin);
		VectorCopy (ent->msg_angles[0], ent->msg_angles[1]);
		VectorCopy (ent->msg_angles[0], ent->angles);
		ent->forcelink = true;
	}
}


/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline (entity_t *ent, int version) //johnfitz -- added argument
{
	int	i;
	int bits; //johnfitz

	if (cl.protocol == PROTOCOL_BJP3)
	{
		ent->baseline.modelindex = MSG_ReadShort ();
		ent->baseline.frame = MSG_ReadByte ();
		bits = 0;
	}
	else
	{
		//johnfitz -- PROTOCOL_FITZQUAKE
		bits = (version == 2) ? MSG_ReadByte() : 0;
		ent->baseline.modelindex = (bits & B_LARGEMODEL) ? MSG_ReadShort() : MSG_ReadByte();
		ent->baseline.frame = (bits & B_LARGEFRAME) ? MSG_ReadShort() : MSG_ReadByte();
		//johnfitz
	}

	ent->baseline.colormap = MSG_ReadByte();
	ent->baseline.skin = MSG_ReadByte();

	for (i=0 ; i<3 ; i++)
	{
		ent->baseline.origin[i] = MSG_ReadCoord ();
		ent->baseline.angles[i] = MSG_ReadAngle ();
	}

	ent->baseline.alpha = (bits & B_ALPHA) ? MSG_ReadByte() : ENTALPHA_DEFAULT; //johnfitz -- PROTOCOL_FITZQUAKE

#if 0 //def GLQUAKE_COLORMAP_TEXTURES
// Mirrors
	if (level.mirror) {
		/*ent->static_mirror_numsurfs =*/ GL_Mirrors_Scan_Entity (ent);
		// Operate on the assumption that func_illusionary is fixed in map.
		// How do we expand vis on server side?
	}
#endif // GLQUAKE_COLORMAP_TEXTURES

	//One more thing ...
}


/*
==================
CL_ParseClientdata

Server information pertaining to this client only
==================
*/
void CL_ParseClientdata (void)
{
	int		i, j;
	int		bits; //johnfitz

	bits = (unsigned short)MSG_ReadShort (); //johnfitz -- read bits here instead of in CL_ParseServerMessage()

	//johnfitz -- PROTOCOL_FITZQUAKE
	if (bits & SU_EXTEND1)
		bits |= (MSG_ReadByte() << 16);

	if (bits & SU_EXTEND2)
		bits |= (MSG_ReadByte() << 24);

	//johnfitz

	if (bits & SU_VIEWHEIGHT)
		cl.viewheight = MSG_ReadChar ();
	else
		cl.viewheight = DEFAULT_VIEWHEIGHT;

	if (bits & SU_IDEALPITCH)
		cl.idealpitch = MSG_ReadChar ();
	else
		cl.idealpitch = 0;

	VectorCopy (cl.mvelocity[0], cl.mvelocity[1]);

	for (i = 0 ; i < 3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i) )
			cl.punchangle[i] = MSG_ReadChar();
		else
			cl.punchangle[i] = 0;

		if (bits & (SU_VELOCITY1<<i) )
			cl.mvelocity[0][i] = MSG_ReadChar()*16;
		else
			cl.mvelocity[0][i] = 0;
	}

	//johnfitz -- update v_punchangles
	if (v_punchangles[0][0] != cl.punchangle[0] || v_punchangles[0][1] != cl.punchangle[1] || v_punchangles[0][2] != cl.punchangle[2])
	{
		VectorCopy (v_punchangles[0], v_punchangles[1]);
		VectorCopy (cl.punchangle, v_punchangles[0]);
	}

	//johnfitz

// [always sent]	if (bits & SU_ITEMS)
		i = MSG_ReadLong ();

	if (cl.items != i)
	{	// set flash times
		Sbar_Changed ();

		for (j=0 ; j < INT32_BITCOUNT_32 ; j++)
			if ( (i & (1<<j)) && !(cl.items & (1<<j)))
				cl.item_gettime[j] = cl.time;

		cl.items = i;
	}

	cl.onground = (bits & SU_ONGROUND) != 0;
	cl.inwater = (bits & SU_INWATER) != 0;

	if (bits & SU_WEAPONFRAME)
		cl.stats[STAT_WEAPONFRAME] = MSG_ReadByte ();
	else
		cl.stats[STAT_WEAPONFRAME] = 0;

	if (bits & SU_ARMOR)
		i = MSG_ReadByte ();
	else
		i = 0;

	if (cl.stats[STAT_ARMOR] != i)
	{
		cl.stats[STAT_ARMOR] = i;
		Sbar_Changed ();
	}

	if (bits & SU_WEAPON)
		if (cl.protocol == PROTOCOL_BJP3)
			i = MSG_ReadShort ();
		else
		i = MSG_ReadByte ();
	else
		i = 0;

	if (cl.stats[STAT_WEAPON] != i)
	{
		cl.stats[STAT_WEAPON] = i;
		Sbar_Changed ();
	}

	i = MSG_ReadShort ();

	if (cl.stats[STAT_HEALTH] != i)
	{
		if (i <= 0)
			memcpy(cl.death_location, cl_entities[cl.viewentity_player].origin, sizeof(vec3_t));
		cl.stats[STAT_HEALTH] = i;
		Sbar_Changed ();
	}

	i = MSG_ReadByte ();

	if (cl.stats[STAT_AMMO] != i)
	{
		cl.stats[STAT_AMMO] = i;
		Sbar_Changed ();
	}

	for (i = 0 ; i < 4 ; i++)
	{
		j = MSG_ReadByte ();

		if (cl.stats[STAT_SHELLS+i] != j)
		{
			cl.stats[STAT_SHELLS+i] = j;
			Sbar_Changed ();
		}
	}

	i = MSG_ReadByte ();

	// Baker: If hipnotic or rogue, written message varies (hipnotic, rogue)
//	if (standard_quake)
	switch (com_gametype)
	{
	case gametype_hipnotic:
	case gametype_quoth:
	case gametype_rogue:
		if (cl.stats[STAT_ACTIVEWEAPON] == (1 << i) )
			break; // No change

		// Change
		cl.stats[STAT_ACTIVEWEAPON] = (1 << i);
		Sbar_Changed ();
		break;

	default: // standard quake, nehahra, ...
		if (cl.stats[STAT_ACTIVEWEAPON] == i)
			break; // No change

		// Change
		cl.stats[STAT_ACTIVEWEAPON] = i;
		Sbar_Changed ();
	}

	//johnfitz -- PROTOCOL_FITZQUAKE
	if (bits & SU_WEAPON2)
		cl.stats[STAT_WEAPON] |= (MSG_ReadByte() << 8);

	if (bits & SU_ARMOR2)
		cl.stats[STAT_ARMOR] |= (MSG_ReadByte() << 8);

	if (bits & SU_AMMO2)
		cl.stats[STAT_AMMO] |= (MSG_ReadByte() << 8);

	if (bits & SU_SHELLS2)
		cl.stats[STAT_SHELLS] |= (MSG_ReadByte() << 8);

	if (bits & SU_NAILS2)
		cl.stats[STAT_NAILS] |= (MSG_ReadByte() << 8);

	if (bits & SU_ROCKETS2)
		cl.stats[STAT_ROCKETS] |= (MSG_ReadByte() << 8);

	if (bits & SU_CELLS2)
		cl.stats[STAT_CELLS] |= (MSG_ReadByte() << 8);

	if (bits & SU_WEAPONFRAME2)
		cl.stats[STAT_WEAPONFRAME] |= (MSG_ReadByte() << 8);

	if (bits & SU_WEAPONALPHA)
		cl.viewent_gun.alpha = MSG_ReadByte();
	else
		cl.viewent_gun.alpha = ENTALPHA_DEFAULT;

	//johnfitz

	//ericw -- this was done before the upper 8 bits of cl.stats[STAT_WEAPON] were filled in, breaking on large maps like zendar.bsp
	//johnfitz -- lerping
	if (cl.viewent_gun.model != cl.model_precache[cl.stats[STAT_WEAPON]])
		cl.viewent_gun.lerpflags |= LERP_RESETANIM; //don't lerp animation across model changes

	//johnfitz

}

/*
=====================
CL_NewTranslation
=====================
*/
void CL_NewTranslation (int slot)
{
#ifdef GLQUAKE_COLORMAP_TEXTURES
	// Baker: Do something here to mark dead skins in future
#endif

#ifdef WINQUAKE_COLORMAP_TRANSLATION
	int		i, j, top, bottom;
	byte	*dest, *source;

	if (slot > cl.maxclients)
		Host_Error ("CL_NewTranslation: slot > cl.maxclients");
	dest 	= cl.scores[slot].translations;
	source 	= vid.colormap;

	memcpy (dest, vid.colormap, sizeof(cl.scores[slot].translations));

	top 	= cl.scores[slot].colors & 0xf0;
	bottom 	= (cl.scores[slot].colors &15)<<4;


	for (i=0 ; i<VID_GRADES ; i++, dest += 256, source+=256)
	{
		if (top < 128)	// the artists made some backwards ranges.  sigh.
			memcpy (dest + TOP_RANGE, source + top, 16);
		else
			for (j=0 ; j<16 ; j++)
				dest[TOP_RANGE+j] = source[top+15-j];

		if (bottom < 128)
			memcpy (dest + BOTTOM_RANGE, source + bottom, 16);
		else
			for (j=0 ; j<16 ; j++)
				dest[BOTTOM_RANGE+j] = source[bottom+15-j];
}
#endif // WINQUAKE_COLORMAP_TRANSLATION
}

/*
=====================
CL_ParseStatic
=====================
*/
void CL_ParseStatic (int version) //johnfitz -- added a parameter
{
	entity_t *ent;
	int		i;

	i = cl.num_statics;

#if 0 // Feb 4 2016 - static ents on hunk
	if (i >= MAX_FITZQUAKE_STATIC_ENTITIES)
		Host_Error ("Too many static entities.  Limit is %d", MAX_FITZQUAKE_STATIC_ENTITIES);

	ent = &cl.static_entities[i];
#else
	// Hunk allocation way -- Feb 04 2016
#endif

	ent = (entity_t *) Hunk_Alloc (sizeof(entity_t));  // Feb 4 2016 - static ents on hunk
	cl.num_statics ++;
	CL_ParseBaseline (ent, version); //johnfitz -- added second parameter

// copy it to the current state
	ent->model = cl.model_precache[ent->baseline.modelindex];
	ent->lerpflags |= LERP_RESETANIM | LERP_RESETMOVE; //johnfitz -- lerping  Baker: Added LERP_RESETMOVE to list
	ent->frame = ent->baseline.frame;

#ifdef GLQUAKE_COLORMAP_TEXTURES
	ent->colormap = 0; // Baker: no colormap
	ent->is_static_entity = true;	// In case we ever care
#endif // GLQUAKE_COLORMAP_TEXTURES

#ifdef WINQUAKE_COLORMAP_TRANSLATION
	ent->colormap = vid.colormap;
#endif // WINQUAKE_COLORMAP_TRANSLATION

	ent->skinnum = ent->baseline.skin;
	ent->effects = ent->baseline.effects;
	ent->alpha = ent->baseline.alpha; //johnfitz -- alpha
	ent->modelindex = ent->baseline.modelindex;

	VectorCopy (ent->baseline.origin, ent->origin);
	VectorCopy (ent->baseline.angles, ent->angles);

// Baker: MH does a lightspot check for static entities here
// and stores it off, since they never move

#if 0 //def GLQUAKE_COLORMAP_TEXTURES
// Mirrors
	if (level.mirror) {
		/* ent->static_mirror_numsurfs = */ GL_Mirrors_Scan_Entity (ent);
		// Operate on the assumption that func_illusionary is fixed in map.
		// How do we expand vis on server side?
	}
#endif // GLQUAKE_COLORMAP_TEXTURES

	R_AddEfrags (ent);
}


/*
===================
CL_ParseStaticSound
===================
*/
void CL_ParseStaticSound (int version) //johnfitz -- added argument
{
	vec3_t		org;
	int			sound_num, vol, atten;
	int			i;

	for (i = 0 ; i < 3 ; i++)
		org[i] = MSG_ReadCoord ();

	//johnfitz -- PROTOCOL_FITZQUAKE
	if (version == 2)
		sound_num = MSG_ReadShort ();
	else
		sound_num = MSG_ReadByte ();

	//johnfitz

	vol = MSG_ReadByte ();
	atten = MSG_ReadByte ();

	S_StaticSound (cl.sound_precache[sound_num], org, vol, atten);
}


char *VersionString (void)
{
	static char str[256];
	//                        "12345678901234567890123456"  //26
	// Return something like: "Windows GL Mark V 0.99"
	//                        "Mac OS X GL Mark V 0.99"
	//                        "Mac OS X Win Mark V 0.99"
	//                        "Linux GL Mark V 0.99"
	//                        "Windows DX8 Mark V 0.99"

	c_snprintf5 (str, "%s (Build: %d) %s %s %s", ENGINE_NAME, (int)ENGINE_BUILD, PLATFORM_SHORTNAME, RENDERER_NAME, __DATE__);

	return str;
}

void Q_Version (const char *s)
{
	const char *t;
	int l, n;

	if (realtime < cl.q_version_next_reply_time)
		return;

	// Baker: &s[1] = lazy, to avoid name "q_version" triggering this; later do it "right"
	#pragma message ("Baker: rewrite this q_version check better")
	for (t = &s[1], n = 0, l = strlen(t); n < l; n++, t++)
	{
		if (!strncmp(t, ": q_version", 9))
		{
			const char *vers = VersionString();
			Cbuf_AddTextLinef ("say %s", vers);

			// Baker: do not allow spamming of it
			cl.q_version_next_reply_time = realtime + 60;

			break; // Baker: only do once per string
		}
	}
}


// Check text for useful information
void CL_ExamineParseText (char *string)
{
	int ping;//, i;
	char *s, *s2, *s3;

	s = string;
	if (!strcmp (string, "Client ping times:" NEWLINE) && scr_scoreboard_pings.value)
	{
		// Receiving a ping status.  Begin a new parse.
		cl.last_ping_time = cl.time;

		cl.in_ping_parse = true;
		cl.in_ping_parse_slot = 0;

		// If if we requested this ping from the server via the scoreboard
		// Let's null out the string so it doesn't print to console
		if (cl.expecting_ping)
			*string = 0;
	}
	else if (cl.in_ping_parse == true) // In a parse
	{
	// Calculate the ping
		while (*s == ' ')
			s++;
		ping = 0;
		if (*s >= '0' && *s <= '9') // Process the numbers
		{
			while (*s >= '0' && *s <= '9')
				ping = 10 * ping + *s++ - '0';
			if ((*s++ == ' ') && *s && (s2 = strchr(s, '\n')))
			{
				s3 = cl.scores[cl.in_ping_parse_slot].name;
				while ((s3 = strchr(s3, '\n')) && s2)
				{
					s3++;
					s2 = strchr(s2 + 1, '\n');
				}
				if (s2)
				{
					*s2 = 0;
					if (!strncmp(cl.scores[cl.in_ping_parse_slot].name, s, 15))
					{
						cl.scores[cl.in_ping_parse_slot].ping = ping > 9999 ? 9999 : ping;
						for (cl.in_ping_parse_slot++ ; !*cl.scores[cl.in_ping_parse_slot].name && cl.in_ping_parse_slot < cl.maxclients ; cl.in_ping_parse_slot++);
					}
					*s2 = '\n';
				}
				// If expecting a scoreboard ping (requested from scoreboard)
				// Then null out the response so it doesn't print to console
				if (cl.expecting_ping)
					*string = 0;
				// Ok ... received too many pings for the player slots so
				// Assume this is invalid and reset
				if (cl.in_ping_parse_slot == cl.maxclients)
					cl.in_ping_parse = false;
			}
			else
				cl.in_ping_parse = false;
		}
		else
			cl.in_ping_parse = false;

		// If anything odd happened with the parse, reset.
		if (cl.in_ping_parse == false)
			cl.expecting_ping = false;
	}

	Q_Version (string); // Baker: check for Q_version requests
}

void CL_Hints_List_f (lparse_t *unused)
{
#pragma message ("Remember to make it so demos will play even if can't change to gamedir")
	hint_type_t hint_num;
	Con_PrintLinef ("Client hints:");
	for (hint_num = 0; hint_num < MAX_NUM_HINTS; hint_num ++)
	{
		const char *hintname = hintnames[hint_num].keystring;
		const char *hintvalue = cl.hintstrings[hint_num];

		Con_PrintLinef ("%-10s: %s", hintname, hintvalue);
	}

//	Con_PrintLinef ("Note that cl.skillhint is %d", cl.skillhint);
}

static void CL_Hint_Set (int cl_new_hintnum, const char *cl_set_hintstring)
{
	char * str, *arg1, *arg2;
	int result;
	char stringbufx[MAX_HINT_BUF_64] = {0};
	// Recognized strings on cmd_argv 0 look up and do a switch
	Con_DPrintLinef_Net ("CL_Hint_Set: #%d " QUOTED_S, cl_new_hintnum, cl_set_hintstring);
	switch ( cl_new_hintnum )
	{
	case hint_game:
		// Host_Game_f
		Con_DPrintLinef_Net ("Game hint %s to %s", hintnames[cl_new_hintnum].keystring, cl_set_hintstring);
		c_strlcpy (stringbufx, cl_set_hintstring);

		for (arg1 = stringbufx, arg2 = NULL; *arg1; arg1++)
			if (*arg1 == ' ')
			{
				*arg1 = 0; // NULL it
				arg2 = arg1 + 1;
				break; // Done
			}

		Con_DPrintLinef_Net ("Game change request: cls.signon is %d", cls.signon);
		result = Host_Gamedir_Change (stringbufx, arg2, true, (const char **)&str, false /*don't force*/);
		if (result == -1 /*fail*/)
		{
			Con_PrintLinef ("game change to \"%s%s\" failed." NEWLINE "Reason: %s", stringbufx, arg2 ? va(" %s", arg2) : "", str);
		}
		break;

	case hint_skill:
		cl.skillhint = CLAMP(0, atoi(cl_set_hintstring), 3);
		cl.skillhint ++; // So that 0 means unknown.
		Con_DPrintLinef_Net ("Set skill hint to %d", cl.skillhint);
		break;

	case hint_fileserver_port:
		cl.fileserver_port = (int)atoi(cl_set_hintstring); // 0 is invalid
		break;

	default:
		// A client shouldn't error on an unknown hint
		Con_DPrintLinef_Net ("Unknown hintnum %d from %s", cl_new_hintnum, hintnames[cl_new_hintnum].keystring );
		return;
	}

	// Copy it off so we can display it with cl_hints.
	strlcpy (cl.hintstrings[cl_new_hintnum], cl_set_hintstring, MAX_HINT_BUF_64);
}

void CL_Hint_f (const char *in_text)
{
	char textbuf[MAX_HINT_BUF_64];
	char *src = textbuf;
	c_strlcpy (textbuf, in_text);

	// find arg2
	while (*src > ' ')
		src++;


	if (src[0] == ' ' && src[1] > ' ' && (src[0] = 0) == 0 /*evile*/)
	{
		const char *harg1 = textbuf;
		const char *harg2 = &src[1];
		keyvalue_t *hint_entry = KeyValue_GetEntry (hintnames, harg1);

		// Kill newlines ...
		src = &src[1];
		while (*src)
		{
			if (*src == '\n')
				*src = 0;
			src ++;
		}

		// If not recognized, ignore it
		if (!hint_entry)
			return;

		Con_DPrintLinef_Net ("Server hint: %s %s", harg1, harg2);
		Con_DPrintLinef_Net ("Setting hint %d = %s", hint_entry->value, harg2);
		CL_Hint_Set (hint_entry->value, harg2);
	}
}

#define SHOWNET(x) if (cl_shownet.value == 2) Con_PrintLinef ("%3d:%s", msg_readcount-1, x);



/* JPG - added this function for ProQuake messages
=======================
CL_ParseProQuakeMessage
=======================
*/
 void CL_ParseProQuakeMessage (void)
{
	int cmd, i;
	int team, frags, shirt, ping;

	MSG_ReadByte();
	cmd = MSG_ReadByte();

	switch (cmd)
	{
	case pqc_new_team:
		Sbar_Changed ();
		team = MSG_ReadByte() - 16;
		if (team < 0 || team > 13)
			Host_Error ("CL_ParseProQuakeMessage: pqc_new_team invalid team");
		shirt = MSG_ReadByte() - 16;
		cl.pq_teamgame = true;
		// cl.teamscores[team].frags = 0;	// JPG 3.20 - removed this
		cl.pq_teamscores[team].colors = 16 * shirt + team;
		//Con_PrintLinef ("pqc_new_team %d %d", team, shirt);
		break;

	case pqc_erase_team:
		Sbar_Changed ();
		team = MSG_ReadByte() - 16;
		if (team < 0 || team > 13)
			Host_Error ("CL_ParseProQuakeMessage: pqc_erase_team invalid team");
		cl.pq_teamscores[team].colors = 0;
		cl.pq_teamscores[team].frags = 0;		// JPG 3.20 - added this
		//Con_PrintLinef ("pqc_erase_team %d", team);
		break;

	case pqc_team_frags:
		cl.pq_teamgame = true; // Baker: I'm not sure all this stuff gets in a demo, so any teamscore update should trigger scoreboard for level.
		Sbar_Changed ();
		team = MSG_ReadByte() - 16;
		if (team < 0 || team > 13)
			Host_Error ("CL_ParseProQuakeMessage: pqc_team_frags invalid team");
		frags = MSG_ReadShortPQ();
		if (frags & 32768)
			frags = frags - 65536;
		cl.pq_teamscores[team].frags = frags;
		//Con_PrintLinef ("pqc_team_frags %d %d", team, frags);
		break;

	case pqc_match_time:
		Sbar_Changed ();
		cl.pq_minutes = MSG_ReadBytePQ();
		cl.pq_seconds = MSG_ReadBytePQ();
		cl.pq_last_match_time = cl.time;
		//Con_PrintLinef ("pqc_match_time %d %d", cl.pq_minutes, cl.pq_seconds);
		break;

	case pqc_match_reset:
		Sbar_Changed ();
		for (i = 0 ; i < 14 ; i++)
		{
			cl.pq_teamscores[i].colors = 0;
			cl.pq_teamscores[i].frags = 0;		// JPG 3.20 - added this
		}
		//Con_PrintLinef ("pqc_match_reset");
		break;

	case pqc_ping_times:
		while (ping = MSG_ReadShortPQ())
		{
			if ((ping / 4096) >= cl.maxclients)
				Host_Error ("CL_ParseProQuakeMessage: pqc_ping_times > MAX_SCOREBOARD_16");
			cl.scores[ping / 4096].ping = ping & 4095;
		}
		cl.last_ping_time = cl.time;
		/*
		Con_PrintLinef ("pqc_ping_times ");
		for (i = 0 ; i < cl.maxclients ; i++)
			Con_PrintLinef ("%4d ", cl.scores[i].ping);
		Con_PrintLine ();
		*/
		break;
	}
}

void CL_ParseProQuakeString (char *string)
{
	static int checkping = -1;
	int ping, i;
	char *s, *s2, *s3;
	static int checkip = -1;	// player whose IP address we're expecting

	// JPG 1.05 - for ip logging
	static int remove_status = 0;
	static int begin_status = 0;
	static int playercount = 0;

	// JPG 3.02 - made this more robust.. try to eliminate screwups due to "unconnected" and '\n'
	s = string;
	if (!strcmp(string, "Client ping times:" NEWLINE) /*&& pq_scoreboard_pings.value*/)
	{
		cl.last_ping_time = cl.time;
		checkping = 0;
		if (!cl.console_ping)
			*string = 0;
	}
	else if (checkping >= 0)
	{
		while (*s == ' ')
			s++;
		ping = 0;
		if (*s >= '0' && *s <= '9')
		{
			while (*s >= '0' && *s <= '9')
				ping = 10 * ping + *s++ - '0';
			if ((*s++ == ' ') && *s && (s2 = strchr(s, '\n')))
			{
				s3 = cl.scores[checkping].name;
				while ((s3 = strchr(s3, '\n')) && s2)
				{
					s3++;
					s2 = strchr(s2+1, '\n');
				}
				if (s2)
				{
					*s2 = 0;
					if (!strncmp(cl.scores[checkping].name, s, 15))
					{
						cl.scores[checkping].ping = ping > 9999 ? 9999 : ping;
						for (checkping++ ; !*cl.scores[checkping].name && checkping < cl.maxclients ; checkping++);
					}
					*s2 = '\n';
				}
				if (!cl.console_ping)
					*string = 0;
				if (checkping == cl.maxclients)
					checkping = -1;
			}
			else
				checkping = -1;
		}
		else
			checkping = -1;
		cl.console_ping = cl.console_ping && (checkping >= 0);	// JPG 1.05 cl.sbar_ping -> cl.console_ping
	}

	// check for match time
	if (!strncmp(string, "Match ends in ", 14))
	{
		s = string + 14;
		if ((*s != 'T') && strchr(s, 'm'))
		{
			sscanf(s, "%d", &cl.pq_minutes);
			cl.pq_seconds = 0;
			cl.pq_last_match_time = cl.time;
		}
	}
	else if (!strcmp(string, "Match paused" NEWLINE))
		cl.pq_match_pause_time = cl.time;
	else if (!strcmp(string, "Match unpaused" NEWLINE))
	{
		cl.pq_last_match_time += (cl.time - cl.pq_match_pause_time);
		cl.pq_match_pause_time = 0;
	}
	else if (!strcmp(string, "The match is over" NEWLINE) || !strncmp(string, "Match begins in", 15))
		cl.pq_minutes = 255; // When does this happen?  Remember this is a ProQuake string, right?
	else if (checkping < 0)
	{
		s = string;
		i = 0;
		while (*s >= '0' && *s <= '9')
			i = 10 * i + *s++ - '0';
		if (!strcmp(s, " minutes remaining" NEWLINE))
		{
			cl.pq_minutes = i;
			cl.pq_seconds = 0;
			cl.pq_last_match_time = cl.time;
		}
	}

	Q_Version (string);//R00k: look for "q_version" requests
}


/*
=====================
CL_ParseServerMessage
=====================
*/
#ifdef SUPPORTS_CUTSCENE_PROTECTION
void CL_ParseServerMessage (cbool *found_server_command)
#endif // SUPPORTS_CUTSCENE_PROTECTION
{
	int			cmd;
	int			i;
	char		*str; //johnfitz
	int			total, j, lastcmd = 0; //johnfitz
//	cbool	strip_pqc;

//
// if recording demos, copy the message out
//
	if (cl_shownet.value == 1)
		Con_PrintContf ("%d ", net_message.cursize); // I guess is a continuator version.
	else if (cl_shownet.value == 2)
		Con_PrintLinef ("------------------");

	cl.onground = false;	// unless the server says otherwise
//
// parse the message
//
	MSG_BeginReading ();

	while (1)
	{
		if (msg_badread)
			Host_Error ("CL_ParseServerMessage: Bad server message");

		cmd = MSG_ReadByte ();

		if (cmd == -1)
		{
			SHOWNET("END OF MESSAGE");
			return;		// end of message
		}

	// if the high bit of the command byte is set, it is a fast update
		if (cmd & U_SIGNAL) //johnfitz -- was 128, changed for clarity
		{
			SHOWNET("fast update");
			CL_ParseUpdate (cmd & 127);
			continue;
		}

		SHOWNET(svc_strings[cmd]);

	// other commands
		switch (cmd)
		{
		default:
			Host_Error ("CL_ParseServerMessage: Illegible server message, previous was %s", svc_strings[lastcmd]); //johnfitz -- added svc_strings[lastcmd]
			break;

		case svc_nop:
//			Con_PrintLinef ("svc_nop");
			break;

		case svc_time:
			cl.mtime[1] = cl.mtime[0];
			cl.mtime[0] = MSG_ReadFloat ();
			break;

		case svc_clientdata:
			CL_ParseClientdata (); //johnfitz -- removed bits parameter, we will read this inside CL_ParseClientdata()
			break;

		case svc_version:
			i = MSG_ReadLong ();

			//johnfitz -- support multiple protocols
			if (i != PROTOCOL_NETQUAKE && i != PROTOCOL_FITZQUAKE && i != PROTOCOL_FITZQUAKE_PLUS)
				Host_Error ("Server returned version %d, not %d, %d or %d", i, PROTOCOL_NETQUAKE, PROTOCOL_FITZQUAKE, PROTOCOL_FITZQUAKE_PLUS);

			cl.protocol = i;

			// MH:
			// svc_version is not sent by the engine, so one presumes that it's used by either QC or an older version of the engine
			// we don't read any protocol flags in this case as the legacy send will not be aware of them

			//johnfitz
			break;

		case svc_disconnect:
			Host_EndGame ("Server disconnected" NEWLINE);

		case svc_print:
			str = MSG_ReadString ();

			CL_ParseProQuakeString(str);
#if 0 // Baker's rewrite not being used, even though easier to read.
			CL_ExamineParseText (str);
#endif // Baker's write
			Con_PrintContf ("%s", str);

			break;

		case svc_centerprint:
			//johnfitz -- log centerprints to console
			str = MSG_ReadString ();
			SCR_CenterPrint (str);
			Con_LogCenterPrint (str);
			//johnfitz
			break;

		case svc_stufftext:
			// JPG - check for ProQuake message
			if (MSG_PeekByte() == MOD_PROQUAKE_1) {
				CL_ParseProQuakeMessage();
			}
			// Still want to add text, even on ProQuake messages.  This guarantees compatibility;
			// unrecognized messages will essentially be ignored but there will be no parse errors
			str = MSG_ReadString();

			// Look for a server hint string.
			// Server hint string begins: "//hint "
			if (String_Does_Start_With (str, HINT_MESSAGE_PREIX))
			{
				//Con_PrintLinef ("Received server hint: %s", str);
				str += strlen (HINT_MESSAGE_PREIX);
				CL_Hint_f (str); // We need this to happen NOW

				break; // Do not continue.
			}

#ifdef SUPPORTS_CUTSCENE_PROTECTION
			if (str[0] == 'b' && str[1] == 'f' && str[2] == 10 && str[3] == 0)
			{
				Cbuf_AddText (str); // bf isn't worth attention
				break; // bonus flashes aren't worth our attention
			}

			if (*found_server_command == false)
			{
				if (str[(strlen(str) - 1)] == '\n' /* 10*/ ) {
					// This gets inserted before the server command
					// And only once for an entire block
					*found_server_command = true;
					Cbuf_AddTextLinef (NEWLINE "%c", CUTSCENE_CHAR_START_5);
				}
			}

			// Baker: Remove control characters except for newlines
			j = strlen (str);
			for (i = 0; i < j; i++)
				if (!str[i] == NEWLINE_CHAR_10 && str[i] < SPACE_CHAR_32)
					str[i] = SPACE_CHAR_32;
#endif // SUPPORTS_CUTSCENE_PROTECTION
			if (str[0]) {
				if (devstuffcmdprint.value) {
					int len = strlen(str);
					int newline = str[len - 1] == '\n' ? true : false;
					if (newline) str[len - 1] = 0; // Keep print less annoying this way.
					Con_PrintLinef ("[Stuffed command]: " QUOTED_S " ", str); //  Used that to check what kind of crazy stuff Nehhra was sending us.
					if (newline) str[len - 1] = 10;
				}
				Cbuf_AddText (str); // <------------------------- we are going to indicate the server buffer here.
			}

			break;

		case svc_damage:
			View_ParseDamage ();
			break;

		case svc_serverinfo:
#if 1
			if (cls.signon == SIGNONS)
			{
				// Baker: This is a restart server due to death scenario (OR) multiplayer level change
				// Either way ... let's make things look nice
				SCR_BeginLoadingPlaque_Force_Transition ();
			}
#endif
			CL_ParseServerInfo ();
			vid.recalc_refdef = true;	// leave intermission full screen
			break;

		case svc_setangle:
			for (i=0 ; i<3 ; i++)
				cl.viewangles[i] = MSG_ReadAngle ();

			if (cls.demoplayback)
			{
				VectorCopy (cl.viewangles, cl.mviewangles[0]);
				VectorCopy (cl.mviewangles[0], cl.mviewangles[1]);
				VectorCopy (cl.mviewangles[0], cl.lerpangles);
				cl.mviewangles[0][0] = cl.mviewangles[1][0] = cl.lerpangles[0] = -cl.lerpangles[0];
			}
			if (!cls.demoplayback)
			{
				VectorCopy (cl.mviewangles[0], cl.mviewangles[1]);

				// From ProQuake - hack with cl.last_angle_time to autodetect continuous svc_setangles
				if (cl.last_angle_time > cl.time - 0.3)
					cl.last_angle_time = cl.time + 0.3;
				else if (cl.last_angle_time > cl.time - 0.6)
					cl.last_angle_time = cl.time;
				else
					cl.last_angle_time = cl.time - 0.3;

				for (i = 0 ; i < 3 ; i++)
					cl.mviewangles[0][i] = cl.viewangles[i];
			}

			break;

		case svc_setview: // This doesn't happen at intermission cam
			cl.viewentity_player = MSG_ReadShort ();
			break;

		case svc_lightstyle:
			i = MSG_ReadByte ();

			if (i >= MAX_LIGHTSTYLES)
				Host_Error ("svc_lightstyle > MAX_LIGHTSTYLES");

			strlcpy (cl.lightstyle[i].map, MSG_ReadString(), MAX_STYLESTRING);
			cl.lightstyle[i].length = strlen(cl.lightstyle[i].map);

			//johnfitz -- save extra info
			if (cl.lightstyle[i].length)
			{
				total = 0;
				cl.lightstyle[i].peak = 'a';

				for (j=0; j<cl.lightstyle[i].length; j++)
				{
					total += cl.lightstyle[i].map[j] - 'a';
					cl.lightstyle[i].peak = c_max(cl.lightstyle[i].peak, cl.lightstyle[i].map[j]);
				}

				cl.lightstyle[i].average = total / cl.lightstyle[i].length + 'a';
			}
			else
				cl.lightstyle[i].average = cl.lightstyle[i].peak = 'm';

			//johnfitz
			break;

		case svc_sound:
			CL_ParseStartSoundPacket();
			break;

		case svc_stopsound:
			i = MSG_ReadShort();
			S_StopSound(i>>3, i&7);
			break;

		case svc_updatename:
			Sbar_Changed ();
			i = MSG_ReadByte ();

			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: svc_updatename > MAX_SCOREBOARD_16");

			strlcpy (cl.scores[i].name, MSG_ReadString(), MAX_SCOREBOARDNAME_32);
			break;

		case svc_updatefrags:
			Sbar_Changed ();
			i = MSG_ReadByte ();

			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: svc_updatefrags > MAX_SCOREBOARD_16");

			cl.scores[i].frags = MSG_ReadShort ();
			break;

		case svc_updatecolors:
			Sbar_Changed ();
			i = MSG_ReadByte ();

			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: svc_updatecolors > MAX_SCOREBOARD_16");

			cl.scores[i].colors = MSG_ReadByte ();
			CL_NewTranslation (i);
			break;

		case svc_particle:
			R_ParseParticleEffect ();
			break;

		case svc_spawnbaseline:
			i = MSG_ReadShort ();
			// must use CL_EntityNum() to force cl.num_entities up
			CL_ParseBaseline (CL_EntityNum(i), 1); // johnfitz -- added second parameter
			break;

		case svc_spawnstatic:
			CL_ParseStatic (1); //johnfitz -- added parameter
			break;

		case svc_temp_entity:
			CL_ParseTEnt ();
			break;

		case svc_setpause:
			cl.paused = MSG_ReadByte ();

			if (cl.paused)
			{
				CDAudio_Pause ();
			}
			else
			{
				CDAudio_Resume ();
			}
			break;

		case svc_signonnum:
			i = MSG_ReadByte ();

			if (i <= cls.signon)
				Host_Error ("Received signon %d when at %d", i, cls.signon);

			cls.signon = i;
			//johnfitz -- if signonnum==2, signon packet has been fully parsed, so check for excessive static ents and efrags
			if (i == 2)
			{
				if (cl.num_statics > 128)
					Con_DWarningLine ("%d static entities exceeds standard limit of 128.", cl.num_statics);
				R_CheckEfrags ();
			}

			//johnfitz
			CL_SignonReply ();
			break;

		case svc_killedmonster:
			if (cls.demoplayback && cls.demorewind)
				cl.stats[STAT_MONSTERS]--;
			else
				cl.stats[STAT_MONSTERS]++;
			break;

		case svc_foundsecret:
			if (cls.demoplayback && cls.demorewind)
				cl.stats[STAT_SECRETS]--;
			else
				cl.stats[STAT_SECRETS]++;
			break;

		case svc_updatestat:
			i = MSG_ReadByte ();

			if (i < 0 || i >= MAX_CL_STATS)
				Host_Error ("svc_updatestat: %d is invalid", i);

			cl.stats[i] = MSG_ReadLong ();
			break;

		case svc_spawnstaticsound:
			CL_ParseStaticSound (1); //johnfitz -- added parameter
			break;

		case svc_cdtrack:
			cl.cdtrack = MSG_ReadByte ();
			cl.looptrack = MSG_ReadByte ();

			if ( (cls.demoplayback || cls.demorecording) && (cls.forcetrack != -1) )
				CDAudio_Play ((byte)cls.forcetrack, true);
			else
				CDAudio_Play ((byte)cl.cdtrack, true);

			break;

		case svc_intermission:
			if (cls.demoplayback && cls.demorewind)
				cl.intermission = 0; // Baker: Demo rewind out of intermission
			else
				cl.intermission = 1;
			cl.completed_time = cl.time;
			vid.recalc_refdef = true;	// go to full screen
			break;

		case svc_finale:
			if (cls.demoplayback && cls.demorewind)
				cl.intermission = 0; // Baker: Demo rewind out of intermission
			else
				cl.intermission = 2;
			cl.completed_time = cl.time;
			vid.recalc_refdef = true;	// go to full screen
			//johnfitz -- log centerprints to console
			str = MSG_ReadString ();
			SCR_CenterPrint (str);
			Con_LogCenterPrint (str);
			//johnfitz
			break;

		case svc_cutscene:
			if (cls.demoplayback && cls.demorewind)
				cl.intermission = 0; // Baker: Demo rewind out of intermission
			else
				cl.intermission = 3;
			cl.completed_time = cl.time;
			vid.recalc_refdef = true;	// go to full screen
			//johnfitz -- log centerprints to console
			str = MSG_ReadString ();
			SCR_CenterPrint (str);
			Con_LogCenterPrint (str);
			//johnfitz
			break;

		case svc_sellscreen:
			Cmd_ExecuteString ("help", src_command);
			break;

#ifdef SUPPORTS_NEHAHRA
		// nehahra support
        case svc_hidelmp:
			SHOWLMP_decodehide ();
			break;

        case svc_showlmp:
			SHOWLMP_decodeshow ();
			break;
#endif // SUPPORTS_NEHAHRA

		//johnfitz -- new svc types
		case svc_skybox:
			Sky_LoadSkyBox (MSG_ReadString());
			break;

		case svc_bf:
			Cmd_ExecuteString ("bf", src_command);
			break;

		case svc_fog:
#ifdef GLQUAKE_FOG
			Fog_ParseServerMessage ();
#else // not GLQUAKE_FOG ...
// Baker: For WinQuake we'll just read the bytes but do nothing with them
// This prevents a protocol error.  For anything that uses these which may only be Nehahra ...
			MSG_ReadByte(); 	// density
			MSG_ReadByte(); 	// red
			MSG_ReadByte(); 	// green
			MSG_ReadByte(); 	// blue
			MSG_ReadShort(); 	// time
#endif // ! GLQUAKE_FOG
			break;

		case svc_spawnbaseline2: //PROTOCOL_FITZQUAKE
			i = MSG_ReadShort ();
			// must use CL_EntityNum() to force cl.num_entities up
			CL_ParseBaseline (CL_EntityNum(i), 2);
			break;

		case svc_spawnstatic2: //PROTOCOL_FITZQUAKE
			CL_ParseStatic (2);
			break;

		case svc_spawnstaticsound2: //PROTOCOL_FITZQUAKE
			CL_ParseStaticSound (2);
			break;
		//johnfitz
		}

		lastcmd = cmd; //johnfitz
//		Con_SafePrintLinef ("Command: %s", svc_strings[cmd] );
	}
}
