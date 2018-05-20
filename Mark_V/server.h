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

#ifndef __SERVER_H__
#define __SERVER_H__

// server.h


typedef struct
{
	int				maxclients_public;		// The fake cap					Use this to determine if server is full, for the "status" command, test2/test commands
	int				maxclients_internal;	// 16 (maxclientslimit or 1).	Use this for about everything.  
	int				maxclientslimit;
	struct client_s	*clients;			// [maxclients]
	int				serverflags;		// episode completion information
	cbool			changelevel_issued;	// cleared when at SV_SpawnServer

// Baker:
	cbool			listening;
	double			lock_until_time;	 // For lock command.  Will set to +10 minutes.  Thinks on connection attempt.

	cbool			remote;				// Remote command line
	char			remote_host[SYSTEM_STRING_SIZE_1024];

	remotelist_t	ban_listing;
	remotelist_t	white_listing;
										// rcon parsing.
										// Remote rcon.  Hmmm.
										// And then you have how rcon shouldn't be able to do rcon_password
} server_static_t;

//=============================================================================

typedef enum {ss_loading, ss_active} server_state_t;

typedef struct
{
	cbool				active;				// false if only a net client

	cbool				paused;
	cbool				loadgame;			// handle connections specially

	double				time;

// these are only used for client checking on the server so they can stay here
	int					lastcheck;			// used by PF_checkclient
	double				lastchecktime;

	char				name[MAX_QPATH_64];		// map name
	char				modelname[MAX_QPATH_64];	// maps/<name>.bsp, for model_precache[0]
	struct qmodel_s 	*worldmodel;
	const char			*model_precache[MAX_FITZQUAKE_MODELS];	// NULL terminated
	struct qmodel_s		*models[MAX_FITZQUAKE_MODELS];
	const char			*sound_precache[MAX_FITZQUAKE_SOUNDS];	// NULL terminated
	const char			*lightstyles[MAX_LIGHTSTYLES];

	unsigned			protocol; //johnfitz

	int					num_edicts;
	int					max_edicts;
	edict_t				*edicts;			// can NOT be array indexed, because
											// edict_t is variable sized, but can
											// be used to reference the world ent
	server_state_t		state;				// some actions are only valid during load

	sizebuf_t			datagram;
	byte				datagram_buf[MAX_MARK_V_DATAGRAM];

	sizebuf_t			reliable_datagram;	// copied to all clients at end of frame
	byte				reliable_datagram_buf[MAX_MARK_V_DATAGRAM];

	sizebuf_t			signon;
	byte				signon_buf[MAX_MARK_V_MSGLEN-2]; //johnfitz -- was 8192, now uses MAX_MSGLEN

	int					frozen;
	int					disallow_major_cheats; // sv_cheats 0 or -1 (-1 allows some minor cheats like give because some coop maps supply too little ammo/health)
	int					disallow_minor_cheats; // sv_cheats 0
	int					current_skill;
	char				progs_name[MAX_QPATH_64];
	cbool				use_old_setmodel;
	int					fileserver_port;

	char				hintstrings[MAX_NUM_HINTS][MAX_HINT_BUF_64];
	double				auto_save_time;


//
//
// Enhancements and evilness section
//
//	
	// JDHack's forced fish fix.
	int					fish_counted;			// Baker: JDH/Requiem part of sv_fishfix functionality

	// JDHack's missing impulse 12 support.  In rare occasions, conflicts with certain releases.  
	cbool				pr_handles_imp12;					// JDH
	cbool 				pr_imp12_override;

	// Co-op per player scores!
	dfunction_t 		*pr_handles_killed;			// Baker: coop per player kills scoreboard
	int 				pr_in_killed; 				// Reset this on progs load in case of pr error
	int 				pr_in_killed_start_frags;
	eval_t				*pr_in_killed_murderer_frags_ev;

	cbool				level_coop_enhanced;		// 1) Is the level running in coop mode with coop enhancements on?  Set by SV_Host_SpawnServer
	int					coop_physics_clientnum; 	// 2) 1 to 16. 0 means not a player or not on during physics run.		

	// Co-op respawn ghosting ability (walkthru players and cannot be telefragged)
	dfunction_t 		*pr_putclientinserver;

	dfunction_t 		*pr_marcher_key_touch;
	int					pr_in_marcher_key_touch;
	int 				pr_in_marcher_key_touch_solid;
	int 				pr_in_marcher_key_touch_modelstringint;
	int 				pr_in_marcher_key_touch_modelindex;
	edict_t 			*pr_in_marcher_key_touch_edict;

} server_t;


#define	NUM_PING_TIMES		16
#define	NUM_SPAWN_PARMS		16

typedef struct client_s
{
	cbool				active;				// false = client is free
	cbool				spawned;			// false = don't send datagrams
	cbool				dropasap;			// has been told to go to another level
	cbool				privileged;			// can execute any host command
	cbool				sendsignon;			// only valid before spawned

	double				last_message;		// reliable messages must be sent
										// periodically

	struct qsocket_s 	*netconnection;	// communications handle

	usercmd_t			cmd;				// movement
	vec3_t				wishdir;			// intended motion calced from cmd

	sizebuf_t			message;			// can be added to at any time,
													// copied and clear once per frame
	byte				msgbuf[MAX_MARK_V_MSGLEN];
	edict_t				*edict;				// EDICT_NUM(clientnum+1)
	char				name[MAX_SCOREBOARDNAME_32];			// for printing to other people
	int					colors;

	float				ping_times[NUM_PING_TIMES];
	int					num_pings;			// ping_times[num_pings%NUM_PING_TIMES]

// spawn parms are carried from level to level
	float				spawn_parms[NUM_SPAWN_PARMS];

// client known data for deltas
	int					old_frags;

	// JPG 3.00 - prevent clients from rapidly changing their name/colour and doing a say or say_team
	double				color_change_time;
	
//
//
// Enhancements and evilness section
//
//	
	double				coop_protect_end_time;
} client_t;

#define COOP_PROTECT_INTERVAL_5_0 5.0
//=============================================================================

// edict->movetype values
#define	MOVETYPE_NONE			0		// never moves
#define	MOVETYPE_ANGLENOCLIP	1
#define	MOVETYPE_ANGLECLIP		2
#define	MOVETYPE_WALK			3		// gravity
#define	MOVETYPE_STEP			4		// gravity, special edge handling
#define	MOVETYPE_FLY			5
#define	MOVETYPE_TOSS			6		// gravity
#define	MOVETYPE_PUSH			7		// no clip to world, push and crush
#define	MOVETYPE_NOCLIP			8
#define	MOVETYPE_FLYMISSILE		9		// extra size to monsters
#define	MOVETYPE_BOUNCE			10

#define MOVETYPE_FOLLOW			12		// track movement of aiment

// edict->solid values
#define	SOLID_NOT				0		// no interaction with other objects
#define	SOLID_TRIGGER			1		// touch on edge, but not blocking
#define	SOLID_BBOX				2		// touch on edge, block
#define	SOLID_SLIDEBOX			3		// touch on edge, but not an onground
#define	SOLID_BSP				4		// bsp clip, touch on edge, block
#define SOLID_EXT_CORPSE		5		// passes through slidebox+other corpses, but not bsp/bbox/triggers

// edict->deadflag values
#define	DEAD_NO					0
#define	DEAD_DYING				1
#define	DEAD_DEAD				2

#define	DAMAGE_NO				0
#define	DAMAGE_YES				1
#define	DAMAGE_AIM				2

// edict->flags
#define	FL_FLY					1
#define	FL_SWIM					2
//#define	FL_GLIMPSE			4
#define	FL_CONVEYOR				4
#define	FL_CLIENT				8
#define	FL_INWATER				16
#define	FL_MONSTER				32
#define	FL_GODMODE				64
#define	FL_NOTARGET				128
#define	FL_ITEM					256
#define	FL_ONGROUND				512
#define	FL_PARTIALGROUND		1024	// not all corners are valid
#define	FL_WATERJUMP			2048	// player jumping out of water
#define	FL_JUMPRELEASED			4096	// for jump debouncing

// entity effects

#define	EF_BRIGHTFIELD				1
#define	EF_MUZZLEFLASH 				2
#define	EF_BRIGHTLIGHT 				4
#define	EF_DIMLIGHT 				8
#define	EF_NODRAW					16	 // Nehahra uses this
#define	EF_BLUE						64
#define	EF_RED						128

#define	SPAWNFLAG_NOT_EASY			256
#define	SPAWNFLAG_NOT_MEDIUM		512
#define	SPAWNFLAG_NOT_HARD			1024
#define	SPAWNFLAG_NOT_DEATHMATCH	2048

//============================================================================


extern	server_static_t	svs;				// persistent server info
extern	server_t		sv;					// local server

extern	client_t	*host_client;
extern	edict_t		*sv_player;

//===========================================================

// Baker: Renamed SV_ functions in some cases to make clear an exclusive function-caller relationship and the chain.
//		  Sue me!

void SV_Init (void);
void SV_Host_Frame_UpdateServer (double frametime); // Called exclusively by Host_Frame.  
											// Outside of SV_SpawnServer (initial map start), we exclusvely call SV_Physics and SV_RunClients

void SV_StartParticle (vec3_t org, vec3_t dir, int color, int count); // Only called by QuakeC via PF_particle
void SV_StartSound (edict_t *entity, int channel, const char *sample, int volume, float attenuation);

void SV_DropClient (cbool crash); // Happens many places

// void SV_SendClientMessages (void); Made static, now named "SV_Host_Frame_UpdateServer_SendClientMessages"
// void SV_ClearDatagram (void); Made static, now named "SV_Host_Frame_UpdateServer_ClearDatagram"

int SV_ModelIndex (const char *name); // Called often, gets model index for a model name.

void SV_SetIdealPitch (void); // SV_WriteClientdataToMessage, which is called from 2 places (see below)

// void SV_AddUpdates (void); // R.I.P.

//void SV_ClientThink (double frametime);  // Exclusively called SV_Physics, made static
//void SV_AddClientToServer (struct qsocket_s *ret);  // R.I.P.

int SV_ClientPrintf (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2))); // Many places like.
int SV_ClientPrintLinef (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2))); // Many places like.
int SV_BroadcastPrintf (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2))); // A few places like "paused the game", "unpaused the game", "deathmatch changed to 1"

void SV_Physics (double frametime); // SV_SpawnServer and "SV_Host_Frame_UpdateServer"

cbool SV_CheckBottom (edict_t *ent); // SV_movestep and SV_NewChaseDir
cbool SV_movestep (edict_t *ent, vec3_t move, cbool relink); // pf_walkmove and SV_StepDirection

void SV_WriteClientdataToMessage (edict_t *ent, sizebuf_t *msg); // Host_Spawn and "SV_Host_Frame_UpdateServer_SendClientMessages_SendClientDatagram"

void SV_ConnectClient (int clientnum);	//called from the netcode to add new clients. also called from pr_ext to spawn new botclients.
void SV_MoveToGoal (void); // Only used by QuakeC --- exposed pr_cmds.c

//void SV_Host_Frame_UpdateServer_CheckForNewClients (void); // Obv, only called by SV_UpdateServer.  Now static!
//void SV_Host_Frame_RunClients_ClientThink (double frametime); // Exclusively called by SV_RunClients.  Now static!
void SV_Host_Frame_RunClients (double frametime); // MH had this as a float, not double?
void SV_Host_Changelevel_SaveSpawnparms (); // Obv, only called by changelevel
void SV_SpawnServer (const char *server); // Called by "map", "changelevel", "restart", "load <savegame>">

void SV_Legacy_FreezeAll_f (lparse_t *unused);
void SV_Hints_List_f (lparse_t *unused);
void SV_Map_Rotation_Refresh_f (cvar_t *var);

#endif // ! __SERVER_H__

