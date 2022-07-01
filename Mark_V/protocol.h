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

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

// protocol.h -- communications protocols

typedef enum
{
	PROTOCOL_NETQUAKE 		= 15,
	PROTOCOL_FITZQUAKE 		= 666, 		// fitz
	PROTOCOL_FITZQUAKE_PLUS = 668, 		// Baker: smooth rotation, sends entity angles as shorts not bytes.
	PROTOCOL_BJP3 			= 10002,	// Baker: Warpspasm uses this.  Supporting for playback only to play Warpspasm demos.
} protocolnum_t;

#define SUPPORTS_ROTATION(_protocol) (_protocol == PROTOCOL_FITZQUAKE_PLUS)

// if the high bit of the servercmd is set, the low bits are fast update flags:
#define	U_MOREBITS		(1<<0)
#define	U_ORIGIN1		(1<<1)
#define	U_ORIGIN2		(1<<2)
#define	U_ORIGIN3		(1<<3)
#define	U_ANGLE2		(1<<4)
#define	U_STEP			(1<<5)	//johnfitz -- was U_NOLERP, renamed since it's only used for MOVETYPE_STEP
#define	U_FRAME			(1<<6)
#define U_SIGNAL		(1<<7)	// just differentiates from other updates

// svc_update can pass all of the fast update bits, plus more
#define	U_ANGLE1		(1<<8)
#define	U_ANGLE3		(1<<9)
#define	U_MODEL			(1<<10)
#define	U_COLORMAP		(1<<11)
#define	U_SKIN			(1<<12)
#define	U_EFFECTS		(1<<13)
#define	U_LONGENTITY	(1<<14)
//johnfitz -- PROTOCOL_FITZQUAKE -- new bits
#define U_EXTEND1		(1<<15)
#define U_ALPHA			(1<<16) // 1 byte, uses ENTALPHA_ENCODE, not sent if equal to baseline
#define U_FRAME2		(1<<17) // 1 byte, this is .frame & 0xFF00 (second byte)
#define U_MODEL2		(1<<18) // 1 byte, this is .modelindex & 0xFF00 (second byte)
#define U_LERPFINISH	(1<<19) // 1 byte, 0.0-1.0 maps to 0-255, not sent if exactly 0.1, this is ent->v.nextthink - sv.time, used for lerping
#define U_UNUSED20		(1<<20)
#define U_UNUSED21		(1<<21)
#define U_UNUSED22		(1<<22)
#define U_EXTEND2		(1<<23) // another byte to follow, future expansion
//johnfitz

//johnfitz -- PROTOCOL_NEHAHRA transparency
#define U_TRANS			(1<<15)
//johnfitz

#define	SU_VIEWHEIGHT	(1<<0)
#define	SU_IDEALPITCH	(1<<1)
#define	SU_PUNCH1		(1<<2)
#define	SU_PUNCH2		(1<<3)
#define	SU_PUNCH3		(1<<4)
#define	SU_VELOCITY1	(1<<5)
#define	SU_VELOCITY2	(1<<6)
#define	SU_VELOCITY3	(1<<7)
#define	SU_UNUSED8		(1<<8)  //AVAILABLE BIT
#define	SU_ITEMS		(1<<9)
#define	SU_ONGROUND		(1<<10)	// no data follows, the bit is it
#define	SU_INWATER		(1<<11)	// no data follows, the bit is it
#define	SU_WEAPONFRAME	(1<<12)
#define	SU_ARMOR		(1<<13)
#define	SU_WEAPON		(1<<14)
//johnfitz -- PROTOCOL_FITZQUAKE -- new bits
#define SU_EXTEND1		(1<<15) // another byte to follow
#define SU_WEAPON2		(1<<16) // 1 byte, this is .weaponmodel & 0xFF00 (second byte)
#define SU_ARMOR2		(1<<17) // 1 byte, this is .armorvalue & 0xFF00 (second byte)
#define SU_AMMO2		(1<<18) // 1 byte, this is .currentammo & 0xFF00 (second byte)
#define SU_SHELLS2		(1<<19) // 1 byte, this is .ammo_shells & 0xFF00 (second byte)
#define SU_NAILS2		(1<<20) // 1 byte, this is .ammo_nails & 0xFF00 (second byte)
#define SU_ROCKETS2		(1<<21) // 1 byte, this is .ammo_rockets & 0xFF00 (second byte)
#define SU_CELLS2		(1<<22) // 1 byte, this is .ammo_cells & 0xFF00 (second byte)
#define SU_EXTEND2		(1<<23) // another byte to follow
#define SU_WEAPONFRAME2	(1<<24) // 1 byte, this is .weaponframe & 0xFF00 (second byte)
#define SU_WEAPONALPHA	(1<<25) // 1 byte, this is alpha for weaponmodel, uses ENTALPHA_ENCODE, not sent if ENTALPHA_DEFAULT
#define SU_UNUSED26		(1<<26)
#define SU_UNUSED27		(1<<27)
#define SU_UNUSED28		(1<<28)
#define SU_UNUSED29		(1<<29)
#define SU_UNUSED30		(1<<30)
#define SU_EXTEND3		(1<<31) // another byte to follow, future expansion
//johnfitz

// a sound with no channel is a local only sound
#define	SND_VOLUME		(1<<0)	// a byte
#define	SND_ATTENUATION	(1<<1)	// a byte
#define	SND_LOOPING		(1<<2)	// a long

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

//johnfitz -- PROTOCOL_FITZQUAKE -- new bits
#define	SND_LARGEENTITY	(1<<3)	// a short + byte (instead of just a short)
#define	SND_LARGESOUND	(1<<4)	// a short soundindex (instead of a byte)
//johnfitz

//johnfitz -- PROTOCOL_FITZQUAKE -- flags for entity baseline messages
#define B_LARGEMODEL	(1<<0)	// modelindex is short instead of byte
#define B_LARGEFRAME	(1<<1)	// frame is short instead of byte
#define B_ALPHA			(1<<2)	// 1 byte, uses ENTALPHA_ENCODE, not sent if ENTALPHA_DEFAULT
//johnfitz

//johnfitz -- PROTOCOL_FITZQUAKE -- alpha encoding
#define ENTALPHA_DEFAULT	0	//entity's alpha is "default" (i.e. water obeys r_wateralpha) -- must be zero so zeroed out memory works
#define ENTALPHA_ZERO		1	//entity is invisible (lowest possible alpha)
#define ENTALPHA_ONE		255 //entity is fully opaque (highest possible alpha)
#define ENTALPHA_ENCODE(a)	(((a)==0)?ENTALPHA_DEFAULT:c_rint(CLAMP(1,(a)*254.0f+1,255))) //server convert to byte to send to client
#define ENTALPHA_DECODE(a)	(((a)==ENTALPHA_DEFAULT)?1.0f:((float)(a)-1)/(254)) //client convert to float for rendering
#define ENTALPHA_TOSAVE(a)	(((a)==ENTALPHA_DEFAULT)?0.0f:(((a)==ENTALPHA_ZERO)?-1.0f:((float)(a)-1)/(254))) //server convert to float for savegame
//johnfitz

// defaults for clientinfo messages
#define	DEFAULT_VIEWHEIGHT	22

// game types sent by serverinfo
// these determine which intermission screen plays
#define	GAME_COOP			0
#define	GAME_DEATHMATCH		1

//==================
// note that there are some defs.qc that mirror to these numbers
// also related to svc_strings[] in cl_parse
//==================

//
// server to client
//
#define	svc_bad					0
#define	svc_nop					1
#define	svc_disconnect			2
#define	svc_updatestat			3	// [byte] [long]
#define	svc_version				4	// [long] server version
#define	svc_setview				5	// [short] entity number
#define	svc_sound				6	// <see code>
#define	svc_time				7	// [float] server time
#define	svc_print				8	// [string] null terminated string
#define	svc_stufftext			9	// [string] stuffed into client's console buffer
									// the string should be \n terminated
#define	svc_setangle			10	// [angle3] set the view angle to this absolute value
#define	svc_serverinfo			11	// [long] version
									// [string] signon string
									// [string]..[0]model cache
									// [string]...[0]sounds cache
#define	svc_lightstyle			12	// [byte] [string]
#define	svc_updatename			13	// [byte] [string]
#define	svc_updatefrags			14	// [byte] [short]
#define	svc_clientdata			15	// <shortbits + data>
#define	svc_stopsound			16	// <see code>
#define	svc_updatecolors		17	// [byte] [byte]
#define	svc_particle			18	// [vec3] <variable>
#define	svc_damage				19
#define	svc_spawnstatic			20
//#define svc_spawnbinary		21
#define	svc_spawnbaseline		22
#define	svc_temp_entity			23
#define	svc_setpause			24	// [byte] on / off
#define	svc_signonnum			25	// [byte]  used for the signon sequence
#define	svc_centerprint			26	// [string] to put in center of the screen
#define	svc_killedmonster		27
#define	svc_foundsecret			28
#define	svc_spawnstaticsound	29	// [coord3] [byte] samp [byte] vol [byte] aten
#define	svc_intermission		30	// [string] music
#define	svc_finale				31	// [string] music [string] text
#define	svc_cdtrack				32	// [byte] track [byte] looptrack
#define svc_sellscreen			33
#define svc_cutscene			34
#define	svc_showlmp				35	// Nehahra: [string] slotname [string] lmpfilename [coord] x [coord] y
#define	svc_hidelmp				36	// Nehahra: [string] slotname

//johnfitz -- PROTOCOL_FITZQUAKE -- new server messages
#define	svc_skybox				37	// [string] name
#define svc_bf					40
#define svc_fog					41	// [byte] density [byte] red [byte] green [byte] blue [float] time
#define svc_spawnbaseline2		42  // support for large modelindex, large framenum, alpha, using flags
#define svc_spawnstatic2		43	// support for large modelindex, large framenum, alpha, using flags
#define	svc_spawnstaticsound2	44	// [coord3] [short] samp [byte] vol [byte] aten
//johnfitz

//
// client to server
//
#define	clc_bad			0
#define	clc_nop 		1
#define	clc_disconnect	2
#define	clc_move		3		// [usercmd_t]
#define	clc_stringcmd	4		// [string] message

// JPG - added ProQuake commands
#define pqc_nop			1
#define pqc_new_team	2
#define pqc_erase_team	3
#define pqc_team_frags	4
#define	pqc_match_time	5
#define pqc_match_reset	6
#define pqc_ping_times	7
// JPG - end mod


//
// temp entity events
//
typedef enum {
	__te_invalid		= -1, // Fuck you mingw and your unsigned enums
	TE_SPIKE			= 0,
	TE_SUPERSPIKE		= 1,
	TE_GUNSHOT			= 2,
	TE_EXPLOSION		= 3,
	TE_TAREXPLOSION		= 4,
	TE_LIGHTNING1		= 5,
	TE_LIGHTNING2		= 6,
	TE_WIZSPIKE			= 7,
	TE_KNIGHTSPIKE		= 8,
	TE_LIGHTNING3		= 9,
	TE_LAVASPLASH		= 10,
	TE_TELEPORT			= 11,
 	TE_EXPLOSION2		= 12,
// PGM 01/21/97
	TE_BEAM				= 13,
// PGM 01/21/97
// nehahra support,
	TE_EXPLOSION3		= 16,
	TE_LIGHTNING4		= 17,
} te_effect_e;

// Net effect
typedef enum {
	___te_ef_invalid	= -1, 
	TE_EF_NONE			=  0,
	TE_EF_PARSED		=  1,  // Something from QuakeC
	TE_EF_GUNSHOT,
	TE_EF_SPIKE,
	TE_EF_SUPERSPIKE,
	TE_EF_KNIGHTSPIKE,
	TE_EF_WIZSPIKE,		
	TE_EF_VENTILLIATION,
	TE_EF_EXPLOSION_1_3,
	TE_EF_EXPLOSION2,
} te_ef_effect_e;


typedef struct
{
	vec3_t			origin;
	vec3_t			angles;
	unsigned short 	modelindex; //johnfitz -- was int
	unsigned short 	frame; //johnfitz -- was int
	unsigned char 	colormap; //johnfitz -- was int
	unsigned char 	skin; //johnfitz -- was int
	unsigned char	alpha; //johnfitz -- added
	int				effects;
} entity_state_t;

typedef struct
{
	vec3_t	viewangles;

// intended velocities
	float	forwardmove;
	float	sidemove;
	float	upmove;
} usercmd_t;

// Hints from server to client, ignored by nonsupporting clients
// Allows backwards compatible conveniences.

#define HINT_MESSAGE_PREIX "//hint "
#define MAX_HINT_BUF_64 64

typedef enum
{
    hint_invalid = -1,
	hint_game = 0,
	hint_skill,
	hint_fileserver_port,
	MAX_NUM_HINTS,
} hint_type_t;

extern keyvalue_t hintnames[MAX_NUM_HINTS + 1];


//johnfitz -- track developer statistics that vary every frame

typedef struct {
	int		packetsize;
	int		edicts;
	int		visedicts;
	int		efrags;
	int		tempents;
	int		beams;
	int		dlights;
} devstats_t;
extern devstats_t dev_stats, dev_peakstats;
//johnfitz

//ohnfitz -- reduce overflow warning spam
typedef struct {
	double	packetsize;
	double	efrags;
	double	beams;
} overflowtimes_t;
extern overflowtimes_t dev_overflows; //this stores the last time overflow messages were displayed, not the last time overflows occured
#define CONSOLE_RESPAM_TIME 3 // seconds between repeated warning messages

// this stores the last time overflow
// messages were displayed, not the last time overflows occured
extern overflowtimes_t dev_overflows;

const char *Gamedir_TypeName (void);

#endif // ! __PROTOCOL_H__

