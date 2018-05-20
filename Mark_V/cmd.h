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
// cmd.h - commands

#ifndef __CMD_H__
#define __CMD_H__

// cmd.h -- Command buffer and command execution

//===========================================================================

/*

Any number of commands can be added in a frame, from several different sources.
Most commands come from either keybindings or console line input, but remote
servers can also send across commands and entire text files can be execed.

The + command line options are also added to the command buffer.

The game starts with a Cbuf_AddTextLine ("exec quake.rc"); Cbuf_Execute ();

*/

void Cbuf_Init (void);
// allocates an initial text buffer that will grow as needed

void Cbuf_AddText (const char *text);
int Cbuf_AddTextLinef (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));
void Cbuf_AddTextLine (const char *text); // Fairly popular

// as new commands are generated from the console or keybindings,
// the text is added to the end of the command buffer.

void Cbuf_InsertText (const char *text);
// when a command wants to issue other commands immediately, the text is
// inserted at the beginning of the buffer, before any remaining unexecuted
// commands.

void Cbuf_Execute (void);
// Pulls off \n terminated lines of text from the command buffer and sends
// them through Cmd_ExecuteString.  Stops when the buffer is empty.
// Normally called once per frame, but may be explicitly invoked.
// Do not call inside a command function!

//===========================================================================

/*

Command execution takes a null terminated string, breaks it into tokens,
then searches for a command or variable that matches the first token.

Commands can come from three sources, but the handler functions may choose
to dissallow the action or forward it to a remote server if the source is
not apropriate.

*/

#define	MAX_ALIAS_NAME	32

typedef struct cmdalias_s
{
	struct cmdalias_s		*next;
	char					name[MAX_ALIAS_NAME];
	char					*value;
	cbool					is_server_alias;
} cmd_alias_t;

cmd_alias_t *Alias_Find (const char *s);
typedef void (*xcmd_t) (lparse_t *line);

typedef struct cmd_function_s
{
	struct cmd_function_s	*next;
	const char				*name;
	xcmd_t					function;
	const char				*description;
} cmd_function_t;

cmd_function_t *Cmd_Find (const char *s);

typedef struct mass_cmds_s
{
	voidfunc_t		init_func;
	const char* 	cmdname;
	xcmd_t			cmdfunc;
	const char		*description;
} mass_cmds_t;

void Cmd_AddCommands (voidfunc_t initializer);
void Cmd_RemoveCommands (voidfunc_t initializer);

typedef enum
{
	src_client,		// came in over a net connection as a clc_stringcmd
					// host_client will be valid during this state.
	src_command		// from the command buffer
} cmd_source_t;

#ifdef SUPPORTS_CUTSCENE_PROTECTION
extern cbool cmd_from_server;
#endif // SUPPORTS_CUTSCENE_PROTECTION

extern	cmd_source_t	cmd_source;

void Cmd_Init (void);

void Cmd_AddCommand (const char *cmd_name, xcmd_t function, const char *description);
// called by the init functions of other parts of the program to
// register commands and functions to call for them.
// The cmd_name is referenced later, so it should not be in temp memory

void	Cmd_RemoveCommand (const char *cmd_name);
// Baker: Removes a command.  Requires understanding about how and when
// memory is allocated to handle this correctly.

cbool Cmd_Exists (const char *cmd_name);
// used by the cvar code to check for cvar / command name overlap

const char 	*Cmd_CompleteCommand (const char *partial);
// attempts to match a partial command for automatic command line completion
// returns NULL if nothing fits

//int		Cmd_Argc (void);
//const char	*Cmd_Argv (int arg);
const char	*Cmd_Args (void);
// The functions that execute commands get their parameters with these
// functions. Cmd_Argv () will return an empty string, not a NULL
// if arg > argc, so string operations are always safe.

//int Cmd_CheckParm (const char *parm); // Baker: unused in source
// Returns the position (1 to argc-1) in the command's argument list
// where the given parameter apears, or 0 if not present

//void Cmd_TokenizeString (const char *text);
// Takes a null terminated string.  Does not need to be /n terminated.
// breaks the string up into arg tokens.

void	Cmd_ExecuteString (const char *text, cmd_source_t src);
// Parses a single line of text into arguments and tries to execute it.
// The text can come from the command buffer, a remote client, or stdin.

void	Cmd_ForwardToServer (lparse_t *line);
// adds the current command line as a clc_stringcmd to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.

//void	Cmd_Print (const char *text); // Unused
// used by command functions to send output to either the graphics console or
// passed as a print message to the client


void Cmd_Wait_f (lparse_t *line);
// Stops execution of command buffer.  Same as "wait" command, but we can call it directly.

void Cmd_Unalias_ServerAliases (void); // Baker: Clear server aliases


#endif // __CMD_H__

