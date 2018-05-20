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
// client.h

#ifndef __CLIENT_H__
#define __CLIENT_H__

// client.h

typedef struct
{
	int		length;
	char	map[MAX_STYLESTRING];		// Baker unsigned char issue?
	char	average; //johnfitz			// Baker unsigned char issue?
	char	peak; //johnfitz			// Baker unsigned char issue?
} lightstyle_t;

typedef struct
{
	char	name[MAX_SCOREBOARDNAME_32];
	float	entertime;
	int		frags;
	int		colors;			// two 4 bit fields
	int		ping;
#ifdef WINQUAKE_COLORMAP_TRANSLATION
	byte	translations[VID_GRADES * PALETTE_COLORS_256];
#endif // WINQUAKE_COLORMAP_TRANSLATION
} scoreboard_t;

// JPG - added this for teamscore status bar
typedef struct
{
	int colors;
	int frags;
} teamscore_t;



typedef struct
{
	int		destcolor[3];
	int		percent;		// 0-256
} cshift_t;

#define	CSHIFT_CONTENTS	0
#define	CSHIFT_DAMAGE	1
#define	CSHIFT_BONUS	2
#define	CSHIFT_POWERUP	3
#define	NUM_CSHIFTS		4

#define	NAME_LENGTH	64


//
// client_state_t should hold all pieces of the client state
//

#define	SIGNONS		4			// signon messages to receive before connected


typedef struct
{
	vec3_t	origin;
	vec3_t	transformed;
	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less
	int		key;
#ifdef GLQUAKE_COLORED_LIGHTS // Baker: I haven't added colored lights
	Point3D	color;				// johnfitz -- lit support via lordhavoc
#endif // GLQUAKE_COLORED_LIGHTS
} dlight_t;


typedef struct
{
	int		entity;
	struct qmodel_s	*model;
	float	endtime;
	vec3_t	start, end;
} beam_t;

#define	MAX_MAPSTRING_2048	2048
#define	MAX_DEMOS_32		32				// JoeQuake
#define	MAX_DEMONAME_64		MAX_QPATH_64	// JoeQuake

typedef enum
{
ca_dedicated, 		// a dedicated server with no ability to start a client
ca_disconnected, 	// full screen console with no connection
ca_connected		// valid netcon, talking to a server
} cactive_t;

#ifdef SUPPORTS_HTTP_DOWNLOAD
// Baker: Can't use this for threaded.
typedef struct
{
//	cbool			web;
	char			name[MAX_QPATH_64];	// Short name
	float			percent;	
	int				total_bytes;	// total / 1024
	cbool			is_blocking;		// Blocking download, like "install command"
	cbool			user_cancelled;

	cbool			disconnect;			// set when user tries to disconnect, to allow cleaning up webdownload	
} download_t; // What are we downloading?  Could be model/sound (LIVE), a -game while connected (LIVE), "install mod" (Not-Live), server browser (Neither)
#endif // SUPPORTS_HTTP_DOWNLOAD

//
// the client_static_t structure is persistant through an arbitrary number
// of server connections
//
typedef struct
{
	cactive_t	state;

// personalization data sent to server
	char		mapstring[MAX_QPATH_64];
	char		spawnparms[MAX_MAPSTRING_2048];	// to restart a level		// Baker unsigned char issue?

// demo loop control
	int			demonum;		// -1 = don't play demos
	char		demos[MAX_DEMOS_32][MAX_DEMONAME_64];		// when not playing

// demo recording info must be here, because record is started before
// entering a map (and clearing client_state_t)
	cbool		demorecording;
	cbool		demoplayback;
	cbool		demorewind;
	float		demospeed;

	char		demo_url[MAX_OSPATH];	// So we can print demo whatever completed.

	int			demo_file_length;		// VectorLength of file in bytes
	int			demo_offset_start;		// If in a pak, the offset into the file otherwise 0.
	int			demo_offset_current;	// Current offset into the file, updated as the demo is player

	float		demo_hosttime_start;	// For measuring capturedemo time completion estimates.
	float		demo_hosttime_elapsed;	// Does not advance if paused.
	float		demo_cltime_start;		// Above except cl.time
	float		demo_cltime_elapsed;	// Above except cl.time

	cbool	titledemo;				// Does not display HUD, notify, centerprint or crosshair when played as part of startdemos

	cbool	capturedemo;			// Indicates if we are capturing demo playback
	cbool	capturedemo_and_exit;	// Quit after capturedemo

	cbool	timedemo;
	cbool	autodemo;

	int			forcetrack;			// -1 = use normal cd track
	FILE		*demofile;
	int			td_lastframe;		// to meter out one message a frame
	int			td_startframe;		// host_framecount at start
	float		td_starttime;		// realtime at second frame of timedemo

// connection information
	int			signon;			// 0 to SIGNONS
	struct qsocket_s	*netcon;
	sizebuf_t	message;		// writing buffer to send to server
	
	cbool		music_run;			// Tells us to run setmusic.cfg (I think)

	char		dz_temp_url[MAX_OSPATH];	// To play .dz file demos

	download_t	download;
} client_static_t;

extern client_static_t	cls;

//
// the client_state_t structure is wiped completely at every
// server signon
//
typedef struct
{
	int			movemessages;	// since connecting to this server
								// throw out the first couple, so the player
								// doesn't accidentally do something the
								// first frame
	usercmd_t	cmd;			// last command sent to the server

// information for local display
	int			stats[MAX_CL_STATS];	// health, etc
	int			items;			// inventory bit flags
	float	item_gettime[32];	// cl.time of aquiring item, for blinking
	float		faceanimtime;	// use anim frame if cl.time < this

	cshift_t	cshifts[NUM_CSHIFTS];	// color shifts for damage, powerups
	cshift_t	prev_cshifts[NUM_CSHIFTS];	// and content types

// Baker: Relocated these view variables so they can be automatically reset on Host_ClearMemory for new map
	float		v_dmg_time;
	float		v_dmg_roll;
	float		v_dmg_pitch;
	cshift_t	cshift_empty;

// the client maintains its own idea of view angles, which are
// sent to the server each frame.  The server sets punchangle when
// the view is temporarliy offset, and an angle reset commands at the start
// of each level and after teleporting.
	vec3_t		mviewangles[2];	// during demo playback viewangles is lerped
								// between these
	vec3_t		viewangles;

	vec3_t		mvelocity[2];	// update by server, used for lean+bob
								// (0 is newest)
	vec3_t		velocity;		// lerped between mvelocity[0] and [1]

	vec3_t		punchangle;		// temporary offset

// pitch drifting vars
	float		idealpitch;
	float		pitchvel;
	cbool		nodrift;
	float		driftmove;
	double		laststop;

	float		viewheight;
	float		crouch;			// local amount for smoothing stepups.  Unused?

	cbool		paused;			// send over by server
	cbool		onground;
	cbool		inwater;

	int			intermission;	// don't change view angle, full screen, etc
	int			completed_time;	// latched at intermission start

	double		mtime[2];		// the timestamp of last two messages
	double		time;			// clients view of time, should be between
								// servertime and oldservertime to generate
								// a lerp point for other data
	double		ctime;			// inclusive of demo speed (can go backwards)
	double		oldtime;		// previous cl.time, time-oldtime is used
								// to decay light values and smooth step ups


	float		last_received_message;	// (realtime) for net trouble icon

//
// information that is static for the entire time connected to a server
//
	struct qmodel_s		*model_precache[MAX_FITZQUAKE_MODELS];
	struct sfx_s		*sound_precache[MAX_FITZQUAKE_SOUNDS];

	char			worldname[MAX_QPATH_64];
	char			levelname[128];	// for display on solo scoreboard //johnfitz -- was 40.

	int				viewentity_player;		// cl_entities[cl.viewentity_player] = player
	int				maxclients;
	int				gametype;

// refresh related state

// Baker: Moved all of these to cl.
	beam_t			beams[MAX_FITZQUAKE_BEAMS];
	dlight_t		dlights[MAX_FITZQUAKE_DLIGHTS];

	efrag_t			efrags[MAX_MARK_V_EFRAGS];
	lightstyle_t	lightstyle[MAX_LIGHTSTYLES];
	entity_t		temp_entities[MAX_FITZQUAKE_TEMP_ENTITIES];
	int				num_temp_entities;
//	entity_t		static_entities[MAX_MARK_V_STATIC_ENTITIES_8192];   Feb 4 2016 - static ents on hunk
	int				num_statics;	// held in cl_staticentities array

	int				num_entities;	// held in cl_entities array

	entity_t		*visedicts[MAX_MARK_V_VISEDICTS];
	int				numvisedicts;

	struct qmodel_s	*worldmodel;	// cl_entities[0].model
	struct efrag_s	*free_efrags;

	struct mleaf_s	*r_viewleaf, *r_oldviewleaf;

	int				r_framecount;
	int				r_visframecount;

	entity_t		viewent_gun;			// the gun model

	int				cdtrack, looptrack;	// cd audio

	teamscore_t		*pq_teamscores;			// [13] - JPG for teamscores in status bar
	cbool			pq_teamgame;			// JPG = true for match, false for individual
	int				pq_minutes;				// JPG - for match time in status bar
	int				pq_seconds;				// JPG - for match time in status bar
	double			pq_last_match_time;		// JPG - last time match time was obtained
	double			last_ping_time;			// JPG - last time pings were obtained
	cbool			console_ping;			// JPG 1.05 - true if the ping came from the console
//	double			last_status_time;		// JPG 1.05 - last time status was obtained
//	cbool			pq_console_status;		// JPG 1.05 - true if the status came from the console
	double			pq_match_pause_time;	// JPG - time that match was paused (or 0)
	
	vec3_t			death_location;			// JPG 3.20 - used for %d formatting

// frag scoreboard
	scoreboard_t	*scores;		// [cl.maxclients]

//	double			last_ping_time;
	cbool			expecting_ping;
	cbool			in_ping_parse;
	int				in_ping_parse_slot;

//	vec3_t			death_location;		// used for %d formatting

	double			last_angle_time;
	vec3_t			lerpangles;

	cbool			noclip_anglehack;
	unsigned		protocol; //johnfitz
	cbool			warned_about_nehahra_protocol; // johnfitz
	char			hintstrings[MAX_NUM_HINTS][MAX_HINT_BUF_64];
	int				skillhint;
	int				fileserver_port;


	char			lastcenterstring[1024];

	float			scr_centertime_off;


	float			q_version_next_reply_time;

	cbool			stored_set;
	vec3_t			stored_origin;
	vec3_t			stored_angles;
} client_state_t;

extern	client_state_t	cl;


extern	entity_t		*cl_entities; //johnfitz -- was a static array, now on hunk
extern	int				cl_max_edicts; //johnfitz -- only changes when new map loads

//=============================================================================

//
// cl_main.c
//
dlight_t *CL_AllocDlight (int key);


void CL_DecayLights (void);

void CL_Init (void);
void CL_ClearState (unsigned int protocol_num);

void CL_EstablishConnection (const char *host);
void CL_Disconnect (void);
void CL_Disconnect_f (lparse_t *unused);
void CL_NextDemo (void);
void CL_UpdateClient (double frametime, cbool readfromserver);
void CL_SignonReply (void);

//
// cl_input.c
//
typedef struct
{
	int		down[2];		// key nums holding it down
	int		state;			// low bit is down state
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;
extern	kbutton_t	in_attack;

void CL_InitInput (void);
void CL_SendCmd (void);
void CL_SendMove (const usercmd_t *cmd);
float CL_KeyState (kbutton_t *key);

void CL_BaseMove (usercmd_t *cmd);

void CL_BoundViewPitch (float *viewangles);

//
// cl_demo.c
//
void CL_StopPlayback (void);
int CL_GetMessage (void);

void CL_Stop_f (lparse_t *unused);
//cmd void CL_Record_f (void);
void CL_PlayDemo_f (lparse_t *line);
//cmd void CL_PlayDemo_NextStartDemo_f (void);
//cmd void CL_TimeDemo_f (void);
void CL_Clear_Demos_Queue (void);

//
// cl_parse.c
//
#ifdef SUPPORTS_CUTSCENE_PROTECTION
void CL_ParseServerMessage (cbool *found_server_command);
#endif // SUPPORTS_CUTSCENE_PROTECTION
void CL_NewTranslation (int slot);
//cmd void CL_Hints_List_f (void);

//
// cl_tent.c
//
void CL_InitTEnts (void);
void CL_ParseTEnt (void);
void CL_UpdateTEnts (void);

//
// chase.c
//

extern int chase_mode;

void Chase_Init (void);
void Chase_UpdateForClient (void); //johnfitz
void Chase_UpdateForDrawing (void); //johnfitz
cbool TraceLine (vec3_t start, vec3_t end, vec3_t impact); // Baker: cbool = was traceline blocked?

#endif // ! __CLIENT_H__

