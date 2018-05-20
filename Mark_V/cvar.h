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
// cvar.h

#ifndef __CVAR_H__
#define __CVAR_H__

/*

cvar_t variables are used to hold scalar or string variables that can
be changed or displayed at the console or prog code as well as accessed
directly in C code.

it is sufficient to initialize a cvar_t with just the first two fields,
or you can add a ,true flag for variables that you want saved to the
configuration file when the game is quit:

cvar_t	r_draworder = {"r_draworder","1"};
cvar_t	scr_screensize = {"screensize","1",true};

Cvars must be registered before use, or they will have a 0 value instead
of the float interpretation of the string.
Generally, all cvar_t declarations should be registered in the apropriate
init function before any console commands are executed:

Cvar_RegisterVariable (&host_framerate);


C code usually just references a cvar in place:
if ( r_draworder.value )

It could optionally ask for the value to be looked up for a string name:
if (Cvar_VariableValue ("r_draworder"))

Interpreted prog code can access cvars with the cvar(name) or
cvar_set (name, value) internal functions:
teamplay = cvar("teamplay");
cvar_set ("registered", "1");

The user can access cvars from the console in two ways:
r_draworder			prints the current value
r_draworder 0		sets the current value to 0
Cvars are restricted from having the same names as commands to keep this
interface from being ambiguous.

*/

#define	CVAR_NONE					0
#define	CVAR_ARCHIVE		(1U << 0)	// if set, causes it to be saved to config
#define	CVAR_NOTIFY			(1U << 1)	// changes will be broadcasted to all players (q1)
#define	CVAR_SERVERINFO		(1U << 2)	// added to serverinfo will be sent to clients (q1/net_dgrm.c and qwsv)
#define	CVAR_USERINFO		(1U << 3)	// added to userinfo, will be sent to server (qwcl)
#ifdef SUPPORTS_CUTSCENE_PROTECTION
#define CVAR_CLIENT			(1U << 4)	// Baker: To mark cvars for cut-scene protection
#define	CVAR_TAMPERED		(1U << 5)	// Baker: Prevent irregular values saving to config, even with good intentions of demo/progs/server doing it.
#endif // SUPPORTS_CUTSCENE_PROTECTION
//#define	CVAR_ROM			(1U << 6)
//#define	CVAR_LOCKED			(1U << 8)	// locked temporarily
#define	CVAR_REGISTERED		(1U << 10)	// the var is added to the list of variables
#define	CVAR_COURTESY		(1U << 11)	// Receives a value from config, writes to config, otherwise doesn't exist.
#define	CVAR_STRINGISH		(1U << 12)	// Receives a value from config, writes to config, otherwise doesn't exist.
//#define	CVAR_CALLBACK		(1U << 16)	// var has a callback

struct cvar_s;
typedef void (*cvarcallback_t) (struct cvar_s *);

void Cvar_AddCvars (voidfunc_t initializer);

typedef struct cvar_s
{
	const char			*name;
	const char			*string;
	unsigned int		dependency;

	unsigned int		flags;
	voidfunc_t			init_func; // Initializing parent
	cvarcallback_t		callback;
	const char			*description;	// October 2016
// These get populated at run time ...
	float				value;
#ifdef SUPPORTS_CUTSCENE_PROTECTION
	float				user_value; // Baker: Set user value here.
#endif // SUPPORTS_CUTSCENE_PROTECTION
	const char			*default_string; //johnfitz -- remember defaults for reset function
	struct cvar_s		*next;
} cvar_t;

//void 	Cvar_RegisterVariable (cvar_t *variable);
// registers a cvar that already has the name, string, and optionally
// the archive elements set.

void Cvar_RegisterTemp (cvar_t *variable);
void Cvar_UnregisterVariable (cvar_t *variable);

// Baker: the opposite


//void Cvar_RegisterVariableWithCallback (cvar_t *variable, cvarcallback_t func);
//void Cvar_SetCallback (cvar_t *var, cvarcallback_t func);
// set a callback function to the var

void Cvar_SetByName (const char *var_name, const char *value);
// equivalent to "<name> <variable>" typed at the console

void Cvar_SetValueByName (const char *var_name, const float value);
// expands value to a string and calls Cvar_Set


void Cvar_ResetQuick (cvar_t *var);
void Cvar_SetQuick (cvar_t *var, const char *value);
void Cvar_SetValueQuick (cvar_t *var, const float value);
// these two accept a cvar pointer instead of a var name,
// but are otherwise identical to the "non-Quick" versions.
// the cvar MUST be registered.

float Cvar_VariableValue (const char *var_name);
// returns 0 if not defined or non numeric

const char * Cvar_VariableString (const char *var_name);
// returns an empty string if not defined

#ifdef SUPPORTS_CUTSCENE_PROTECTION
cbool	Cvar_Command (cbool src_server, cvar_t *var, lparse_t *line);
#endif // SUPPORTS_CUTSCENE_PROTECTION

// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

void Cvar_WriteVariables (FILE *f);
// Writes lines containing "set variable value" for all variables
// with the CVAR_ARCHIVE flag set

cvar_t *Cvar_Find (const char *var_name);
cvar_t *Cvar_FindAfter (const char *prev_name, unsigned int with_flags);

void Cvar_LockVar (const char *var_name);
void Cvar_UnlockVar (const char *var_name);
void Cvar_UnlockAll (void);

void Cvar_Init (void);

const char *Cvar_CompleteVariable (const char *partial);
// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits

#ifdef SUPPORTS_CUTSCENE_PROTECTION

// Functions

#define CUTSCENE_GUARDIAN_FAKE_VALUE 206

// Reverts values to user's values
void Cvar_Clear_Untrusted (void);

// (*) Firewalled setting of *client-oriented* cvars from non-user sources
// like demos, progs.dat or a server.
//
// These untrusted sets don't get written to config and get restored
// to the user's setting.
void Cvar_Set_Untrusted (const char *var_name,  const char *newvaluestr);

// Helps cut-scenes do it right by detecting and then helping
// a cut-scene attempt to present things nice.
cbool Cutscene_Guardian_PF_cvar_set_Think (const char *var_name, const char *val_string);

// Helps QuakeC cut-scene attempts by giving progs.dat fake cvar values
// if requests certain client-only cvars.  Then if the progs.dat tries
// to revert to this value, we can act upon it.
cbool Cutscene_Guardian_PF_cvar_Get_Think (const char *var_name);

// Cut-scenes often try to fiddle with stuff to make the presentation
// nice, but sometimes forget little things like hiding the crosshair.
void Cutscene_Guardian_Begin (void);


/*
	(*) Although currently it gives a free pass to cvars not marked
	CVAR_CLIENT.  And only maybe 7 are marked that.

	In theory, the way I set it up might correctly deal with mods like
	the original rocket arena that hides your viewmodel with r_drawviewmodel 1
	if I recall --- but I haven't tested this.

*/

#endif // SUPPORTS_CUTSCENE_PROTECTION

void Courtesy_Cvars (void); // Baker: Called after host init to make place for extra vars not registered.


#define CVAR_DEF(_initfunc_,_class_,_dependency_,_internal_name_,_name_,_default_string_,_flags_,_callbackfunc_,_help_) \
	extern cvar_t _internal_name_;

#include "cvar_list_sheet.h"

/*	const char		*name;
	const char		*string;
	unsigned int	dependency;
	unsigned int	flags;
	cvarcallback_t	callback; */


#endif // ! __CVAR_H__

