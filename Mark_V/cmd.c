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
// cmd.c -- Quake script command processing module

#include "quakedef.h"

cmd_alias_t	*cmd_alias;

cbool	cmd_wait;

//=============================================================================

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
void Cmd_Wait_f (lparse_t *line)
{
	cmd_wait = true;
}

/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

/*
typedef struct sizebuf_s
{
	cbool	allowoverflow;	// if false, do a System_Error
	cbool	overflowed;		// set to true if the buffer size failed
	byte	*data;
	int		maxsize;
	int		cursize;
} sizebuf_t;

void SZ_Alloc (sizebuf_t *buf, int startsize)
{
	if (startsize < 256)
		startsize = 256;
	buf->data = (byte *) Hunk_AllocName (startsize, "sizebuf");
	buf->maxsize = startsize;
	buf->cursize = 0;
}
*/

sizebuf_t	cmd_text;

/*
============
Cbuf_Init
============
*/
void Cbuf_Init (void)
{
	SZ_Alloc (&cmd_text, 256 * 1024);  // 256 KB formerly 8 KB.  //8192);	// space for commands and script files
}


/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText (const char *text)
{
	int		l;

	l = strlen (text);

	if (cmd_text.cursize + l >= cmd_text.maxsize)
	{
		Con_PrintLinef ("Cbuf_AddText: overflow");
		return;
	}

	SZ_Write (&cmd_text, text, strlen (text));
}

int Cbuf_AddTextLinef (const char *fmt, ...) //  __core_attribute__((__format__(__printf__,1,2)))
{
	VA_EXPAND_NEWLINE (text, SYSTEM_STRING_SIZE_1024, fmt);
	Cbuf_AddText (text);
	return 0;
}

void Cbuf_AddTextLine (const char *text)
{
	Cbuf_AddTextLinef ("%s", text);
}




/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
FIXME: actually change the command buffer to do less copying
============
*/
void Cbuf_InsertText (const char *text)
{
	char	*temp;
	int		templen;

// copy off any commands still remaining in the exec buffer
	templen = cmd_text.cursize;
	if (templen)
	{
		temp = (char *) Z_Malloc (templen);
		memcpy (temp, cmd_text.data, templen);
		SZ_Clear (&cmd_text);
	}
	else
		temp = NULL;	// shut up compiler

// add the entire text of the file
	Cbuf_AddText (text);
	SZ_Write (&cmd_text, "\n", 1);
// add the copied off data
	if (templen)
	{
		SZ_Write (&cmd_text, temp, templen);
		Z_Free (temp);
	}
}

/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute (void)
{
	int		i;
	char	*text;
	char	line[1024];
	int		quotes;


	while (cmd_text.cursize)
	{
// find a \n or ; line break
		text = (char *)cmd_text.data;

		quotes = 0;
		for (i=0 ; i< cmd_text.cursize ; i++)
		{
			if (text[i] == '"')
				quotes++;

			if ( !(quotes&1) &&  text[i] == ';')
				break;	// don't break if inside a quoted string

			if (text[i] == '\n')
				break;
		}


		memcpy (line, text, i);
		line[i] = 0;

// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec, alias) can insert data at the
// beginning of the text buffer

		if (i == cmd_text.cursize)
			cmd_text.cursize = 0;
		else
		{
			i++;
			cmd_text.cursize -= i;
			memmove (text, text + i, cmd_text.cursize);
		}

// execute the command line
		Cmd_ExecuteString (line, src_command);

		if (cmd_wait)
		{	// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait = false;
			break;
		}
	}
}

/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/

/*
===============
Cmd_StuffCmds_f -- johnfitz -- rewritten to read the "cmdline" cvar, for use with dynamic mod loading

Adds command line parameters as script statements
Commands lead with a +, and continue until a - or another +
quake +prog jctest.qp +cmd amlev1
quake -nosound +cmd amlev1
===============
*/
void Cmd_StuffCmds_f (lparse_t *unused)
{
	char	cmds[MAX_CMD_256];
	int		i, j, plus;

	plus = true;
	j = 0;
	for (i = 0; cmdline.string[i]; i ++)
	{
		if (cmdline.string[i] == '+')
		{
			plus = true;
			if (j > 0)
			{
				cmds[j-1] = ';';
				cmds[j++] = ' ';
			}
		}
		else if (cmdline.string[i] == '-' &&
			(i==0 || cmdline.string[i-1] == ' ')) //johnfitz -- allow hypenated map names with +map
				plus = false;
		else if (plus)
			cmds[j++] = cmdline.string[i];
	}
	cmds[j] = 0;

	Cbuf_InsertText (cmds);
}


/*
===============
Cmd_Exec_f
===============
*/
cbool exec_silent;
void Cmd_Exec_f (lparse_t *line)
{
	char	*file_text;
	int		mark;

	if (line->count  != 2)
	{
		Con_PrintLinef ("exec <filename> : execute a script file");
		return;
	}

	mark = Hunk_LowMark ();

	file_text = (char *)COM_LoadHunkFile (line->args[1]);
	if (!file_text)
	{
		if (!exec_silent)
			Con_PrintLinef ("couldn't exec %s", line->args[1] );
		exec_silent = false;
		return;
	}

	#define DEFAULT_CFG_SIZE_1914 1914
	if (!isDedicated && !strcasecmp (line->args[1], DEFAULT_CFG)) {
		char *s_insert = "alias +zoom_key \"valsave fov 1;valsave sensitivity 2;valsave r_viewmodel_fov 3;r_viewmodel_fov 0;mul fov 0.7;mul sensitivity 0.5;wait;mul fov 0.7;mul sensitivity 0.5;wait;mul fov 0.7;mul sensitivity 0.5;wait;mul fov 0.7;mul sensitivity 0.5; fov 10\" \n"
			"alias -zoom_key \"mul sensitivity 1.43;mul fov 1.43;wait;mul sensitivity 1.43;mul fov 1.43;wait;mul sensitivity 1.43;mul fov 1.43;wait;mul sensitivity 1.43;mul fov 1.43;valload fov 1;valload sensitivity 2;valload r_viewmodel_fov 3\" \n"
			;
		Cbuf_InsertText (s_insert);

		if (com_filesrcpak == 2 && com_filesize == DEFAULT_CFG_SIZE_1914) {
			// Specifically id1 pak0

			// Evil additions.
			//Cbuf_InsertText (f); // Does need newline?  Nope.  Do we add or insert?
			// What changes are we making?
			char *soverride =
							"unalias zoom_in  \n"
							"unalias zoom_out \n"
							"alias +quickgrenade \"-attack;wait;impulse 6;wait;+attack\" \n"
							"alias -quickgrenade \"-attack;wait;bestweapon 7 8 5 3 4 2 1\" \n"
	//						"alias bestsafe \"bestweapon 8 5 3 4 2 1\" \n" // They can make their own.
							"bind    ALT     +strafe \n"
							"unbind  ,       //was +moveleft \n"
							"unbind  .       //was +moveright \n"
							"unbind  DEL     // +lookdown \n"
							"bind    PGDN    +lookdown   // formerly lookup  \n"
							"bind    PGUP    +lookup \n"
							"unbind  z       // +lookdown \n"
							"bind    w       +forward \n"
							"bind    a       +moveleft \n"
							"bind    s       +back \n"
							"bind    d       +moveright \n"
							"unbind  c       // +movedown \n"
							"bind    q       +moveup \n"
							"bind    e       +movedown \n"
							"bind    [       \"impulse 10\"	// change weapon \n"
							"bind    ]       \"impulse 12\" // change weapon  \n"
							"bind    MWHEELUP       \"impulse 10\"	// change weapon \n"
							"bind    MWHEELDOWN       \"impulse 12\" // change weapon  \n"
							"bind    .		+mlook  \n"
							"bind    ,		+klook  \n"
							"bind    MOUSE3 +zoom_key  \n"
							"unbind F11      // stupid zoom key  \n"
							"unbind  \\      // +mlook  \n"
							"unbind  /      // next weapon  \n"
							"unbind  INS     // +klook  \n"
							;
			Cbuf_InsertText (soverride);			
		}
	}


	if (String_Does_Match_Caseless (line->args[1], CONFIG_CFG))
	{
		const char *check_string = va("// %s", ENGINE_FAMILY_NAME);
		cbool is_our = String_Does_Start_With (file_text, check_string);
		//int len = strlen(check_string);
		
//		cbool is_ok = false;
		// Baker: Make sure it is a Mark V config
		if (!is_our)
		{
			// Not good
			const char *retryname = va("%s/%s/%s", Folder_Caches_URL (), File_URL_SkipPath (game_startup_dir), CONFIG_CFG);
			//int filelen = File_Length (retryname);
			size_t bytesread;
			byte *data = File_To_Memory_Alloc (retryname, &bytesread);

			if (data)
			{
				Hunk_FreeToLowMark (mark);
				mark = Hunk_LowMark ();
				file_text = Hunk_Alloc (bytesread + 1);
				memcpy (file_text, data, bytesread);
				free (data);
			}
			else
			{
				Con_PrintLinef ("couldn't exec %s", line->args[1]);
				return;
			}
		}
	}

/* Baker: In theory we could perform surgery on
default.cfg Size = 1914, CRC32 = 17245 here to default
decent keys and always run on
	{
		int filesize = com_filesize;
		int crc32 = CRC_Block (f, com_filesize);

		Con_PrintLinef ("execing %s",line->args[1]);
	}

*/
	if (!exec_silent)
	{
		Con_SafePrintLinef ("execing %s",line->args[1]);
		exec_silent = false;
	}

	Cbuf_InsertText (file_text);
	Hunk_FreeToLowMark (mark);


}


/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
void Cmd_Echo_f (lparse_t *line)
{
	int		i;

	for (i = 1 ; i < line->count ; i++)
		Con_PrintContf ("%s ", line->args[i]); // It's printing each arg after another.
	Con_PrintLine ();
}

/*
===============
Cmd_Alias_f -- johnfitz -- rewritten

Creates a new command that executes a command string (possibly ; separated)
===============
*/
void Cmd_Alias_f (lparse_t *line)
{
	cmd_alias_t	*a;
	char		cmd[1024];
	int			i, c;
	const char	*s;

	switch (line->count)
	{
	case 1: //list all aliases
		for (a = cmd_alias, i = 0; a; a=a->next, i++) {
			// Remember, aliasees are newline terminated!!
			Con_SafePrintContf (" %s %s: %s",
			(a->is_server_alias) ? "*" : " ",
			a->name,
			a->value);

//			Con_SafePrintContf ("   %s: %s", a->name, a->value);
		}



		if (i)
			Con_SafePrintLinef ("%d alias command(s)", i);
		else
			Con_SafePrintLinef ("no alias commands found");
		break;
	case 2: //output current alias string
		for (a = cmd_alias ; a ; a=a->next)
			if (!strcmp(line->args[1], a->name))
				Con_PrintContf ("   %s: %s", a->name, a->value); // Aliases have a newline built into them, right?
		break;
	default: //set alias string
		s = line->args[1];
		if (strlen(s) >= MAX_ALIAS_NAME)
		{
			Con_PrintLinef ("Alias name is too long");
			return;
		}

		// if the alias already exists, reuse it
		for (a = cmd_alias ; a ; a=a->next)
		{
			if (!strcmp(s, a->name))
			{
				Z_Free (a->value);
				break;
			}
		}

		if (!a)
		{
			a = (cmd_alias_t *) Z_Malloc (sizeof(cmd_alias_t));
			a->next = cmd_alias;
			cmd_alias = a;
		}
		c_strlcpy (a->name, s);

		a->is_server_alias = cmd_from_server;

		// copy the rest of the command line
		cmd[0] = 0;		// start out with a null string
		c = line->count;
		for (i = 2 ; i < c ; i++)
		{
			c_strlcat (cmd, line->args[i]);
			if (i != c - 1)
				c_strlcat (cmd, " ");
		}
		if (c_strlcat(cmd, "\n") >= sizeof(cmd))
		{
			Con_PrintLinef ("alias value too long!");
			cmd[0] = '\n';	// nullify the string
			cmd[1] = 0;
		}

		a->value = Z_Strdup (cmd);
		break;
	}
}

/*
===============
Cmd_Unalias_f -- johnfitz
===============
*/
void Cmd_Unalias_f (lparse_t *line)
{
// Baker: Has bug where if try to unalias top entry, it doesn't work.
	cmd_alias_t	*a, *prev;

	switch (line->count)
	{
	default:
	case 1:
		Con_PrintLinef ("unalias <name> : delete alias");
		break;
	case 2:
		prev = NULL;
		for (a = cmd_alias; a; a = a->next)
		{
			if (!strcmp(line->args[1], a->name))
			{
				if (prev)
				prev->next = a->next;
				else
					cmd_alias  = a->next;

				Z_Free (a->value);
				Z_Free (a);
				return;
			}
			prev = a;
		}
		Con_PrintLinef ("No alias named %s", line->args[1]);
		break;
	}
}

/*
===============
Cmd_Unaliasall_f -- johnfitz
===============
*/
void Cmd_Unalias_ServerAliases (void)
{
	cmd_alias_t	*a, *prev;

	for (prev = NULL, a = cmd_alias; a; /* moved */) {
		if (a->is_server_alias) {
			cmd_alias_t	*next_one = a->next;
			if (a == cmd_alias) cmd_alias = a->next; // Top replacement.

			if (prev)
				prev->next = a->next; // Not top
			else
				cmd_alias  = a->next; // Is top.  Why doesn't this work?
			
			Z_Free (a->value);
			Z_Free (a);
			
			a = next_one;
			continue; // Next please
		}
		// Didn't match
		a = a->next; // Move to next.
		
	}

}



void Cmd_Unaliasall_f (lparse_t *unused)
{
	cmd_alias_t	*blah;

	while (cmd_alias)
	{
		blah = cmd_alias->next;
		Z_Free(cmd_alias->value);
		Z_Free(cmd_alias);
		cmd_alias = blah;
	}
}

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/



cmd_source_t	cmd_source;


cmd_function_t	*cmd_functions;		// possible commands to execute


/*
============
Cmd_List_f -- johnfitz
============
*/
void Cmd_List_f (lparse_t *line)
{
	cmd_function_t	*cmd;
	cmd_function_t	*prev = NULL;
	cmd_function_t	*prevprev = NULL;
	const char	*partial = NULL;
	int			count = 0;

	if (line->count > 1)
	{
		partial = line->args[1];
	}

	for (cmd = cmd_functions ; cmd ; cmd = cmd->next)
	{
		if (partial && String_Does_Not_Start_With (cmd->name, partial))
			continue;

		Con_SafePrintLinef ("   %s", cmd->name);
		count++;
		prevprev = prev;
		prev = cmd;
		
	}

	Con_SafePrintContf ("%d commands", count);

	if (partial)
	{
		Con_SafePrintContf (" beginning with " QUOTED_S, partial);
	}
	Con_SafePrintLine ();
}

/*
============
Cmd_Init
============
*/
void Cmd_Init (void)
{
	Cmd_AddCommands (Cmd_Init);
}


/*
============
Cmd_RemoveCommand
============
*/

void	Cmd_RemoveCommand (const char *cmd_name)
{
	cmd_function_t	*cursor,*prev; //johnfitz -- sorted list insert

	for (cursor=cmd_functions, prev = NULL ; cursor ; prev = cursor, cursor=cursor->next)
	{
		if (!strcmp (cmd_name,cursor->name))
			break;
	}

	if (cursor == NULL)
	{
		Con_PrintLinef ("Couldn't remove command %s", cmd_name);
		return;
	}

	// Link it away
	if (prev) prev->next = cursor->next;
}


// Mass commands extern
#define CMD_DEF(_c,_d,_n,_f,_h) extern void _f (lparse_t *);
#include "cmd_list_sheet.h" // array

/*
typedef struct mass_cmds_s
{
	xcmd_t		init_func;
	const char* 	cmdname;
	xcmd_t		cmdfunc;
} mass_cmds_t;
*/

mass_cmds_t cmd_list[] =
{
#define CMD_DEF(_c,_d,_n,_f,_h) { _c, _n, _f, _h },
#include "cmd_list_sheet.h" // array
	{ NULL }, // Trailing empty name
};

#pragma message ("Baker: Kill this when opporunity permits")
void Cmd_No_Command (lparse_t *line) {};

void Cmd_AddCommands (voidfunc_t initializer)
{
	mass_cmds_t *cur;

	for (cur = &cmd_list[0]; cur->init_func; cur ++)
	{
		if (cur->init_func == initializer)
			Cmd_AddCommand (cur->cmdname, cur->cmdfunc, cur->description);
	}


	Cvar_AddCvars (initializer);
}

void Cmd_RemoveCommands (voidfunc_t initializer)
{
	mass_cmds_t *cur;

	for (cur = &cmd_list[0]; cur->init_func; cur ++)
	{
		if (cur->init_func == initializer)
			Cmd_RemoveCommand (cur->cmdname);
	}

	// TOO LATE.  THE CVARS WOULD HAVE BECOME COURTESY AT THIS POINT.
	//Cvar_RemoveCvars (initializer);
}

/*
============
Cmd_AddCommand
============
*/
cmd_function_t	*cmd_find; // find->next will be borked.  Let's see if we can determine how it gets borked
						// Keep an eye on host_hunklevel

void Cmd_AddCommand (const char *cmd_name, xcmd_t function, const char *description)
{
	cmd_function_t	*cmd;
	cmd_function_t	*cursor,*prev; //johnfitz -- sorted list insert

	

	if (host_initialized)	// because hunk allocation would get stomped
		if (!nehahra_active)
			System_Error ("Cmd_AddCommand after host_initialized");

// fail if the command is a variable name
	if (Cvar_Find(cmd_name))
	{
		Con_SafePrintLinef ("Cmd_AddCommand: %s already defined as a var", cmd_name);
		return;
	}

// fail if the command already exists
	for (cmd=cmd_functions, prev = NULL; cmd ; prev = cmd, cmd=cmd->next)
	{
		if (!strcmp (cmd_name, cmd->name))
		{
			Con_PrintLinef ("Cmd_AddCommand: %s already defined", cmd_name);
			return;
		}
	}

	cmd = (cmd_function_t *) Hunk_AllocName (sizeof(cmd_function_t), "cmd");
	cmd->name = cmd_name;
	cmd->function = function;
	cmd->description = description;

	if (!strcmp(cmd_name, "find"))
		cmd_find = cmd;

	//johnfitz -- insert each entry in alphabetical order
    if (cmd_functions == NULL || strcmp(cmd->name, cmd_functions->name) < 0) //insert at front
	{
        cmd->next = cmd_functions;
        cmd_functions = cmd;
    }
    else //insert later
	{
        prev = cmd_functions;
        cursor = cmd_functions->next;
        while ((cursor != NULL) && (strcmp(cmd->name, cursor->name) > 0))
		{
            prev = cursor;
            cursor = cursor->next;
        }
        cmd->next = prev->next;
        prev->next = cmd;
    }
	//johnfitz
}

/*
============
Cmd_Exists
============
*/
cbool	Cmd_Exists (const char *cmd_name)
{
	cmd_function_t	*cmd;

	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		if (!strcmp (cmd_name,cmd->name))
			return true;
	}

	return false;
}



/*
============
Cmd_CompleteCommand
============
*/
const char *Cmd_CompleteCommand (const char *partial)
{
	cmd_function_t	*cmd;
	int				len;

	len = strlen(partial);

	if (!len)
		return NULL;

// check functions
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
		if (!strncmp (partial,cmd->name, len))
			return cmd->name;

	return NULL;
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
============
*/
#ifdef SUPPORTS_CUTSCENE_PROTECTION
cbool cmd_from_server;
#endif // SUPPORTS_CUTSCENE_PROTECTION
void Cmd_ExecuteString (const char *text, cmd_source_t src)
{
	lparse_t *line = NULL;


	cmd_source = src;



#ifdef SUPPORTS_CUTSCENE_PROTECTION
	if (text[0] == CUTSCENE_CHAR_START_5 && text[1] == 0)
	{
		cmd_from_server = true;
		return;
	}
	else if (text[0] == CUTSCENE_CHAR_END_6 && text[1] == 0)
	{
		cmd_from_server = false;
		return;
	}

#if 0 // Debugging
	if (cmd_from_server)
		Con_PrintLinef ("Cmd_ExecuteString: Cmd from server is %s", text);
#endif  //

#endif // SUPPORTS_CUTSCENE_PROTECTION

//	Cmd_TokenizeString (text);

	line = Line_Parse_Alloc (text, false);
	if (line->count)
	{

	/*
	// execute the command line
		if (!Cmd_Argc())
			return;		// no tokens
	*/

	// check functions
	#if 1
		{
			cmd_function_t *cmd = Cmd_Find (line->args[0]);

			if (cmd)
			{
				cmd->function (line);
				return;
			}
		}
	#else
		for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
		{
	//		if (!strcasecmp (cmd_argv[0],cmd->name))
			if (!strcasecmp (line.args[0],cmd->name))
			{
				cmd->function ();
				return;
			}
		}
	#endif

	// check alias
	#if 1
		{
			cmd_alias_t *alias = Alias_Find (line->args[0]);
			if (alias)
			{
				Cbuf_InsertText (alias->value);
				return;
			}
		}
	#else

		for (a=cmd_alias ; a ; a=a->next)
		{
			if (!strcasecmp (line.args[0], a->name))
			{
				Cbuf_InsertText (a->value);
				return;
			}
		}
	#endif

	// check cvars
	#ifdef SUPPORTS_CUTSCENE_PROTECTION
	#if 1
		{
			cvar_t *var = Cvar_Find (line->args[0]);
			if (var)
			{
				Cvar_Command (cmd_from_server, var, line);
				return;
			}
		}
	#else
		if (!Cvar_Command (cmd_from_server, &line))
	#endif
	#endif // SUPPORTS_CUTSCENE_PROTECTION
		{

			// Baker: Dedicated server must be post-initialized to receive unknown command
			// prevents spams of unknown client commands from the config.cfg, etc.
	#pragma message ("Baker: Downside: Might not alert user to a typo from command line or server config?")
	#pragma message ("Baker: Our new security model will make this go away and we can do another check to see")
	#pragma message ("Baker: If it is a valid cvar that is just client only.  For commands: only bind, unbindall, sky are in a config")
	#pragma message ("Baker: Might have some wacky stuff in a quake.rc but that's ok")
			// DO MAKE DEDICATED NOT PRINT THAT STUFF.
			// INSTEAD make viewsize, gamma, sensitivity, bind, unbindall have dummies for dedicated server.
			//if (!isDedicated || host_post_initialized)
				Con_PrintLinef ("Unknown command " QUOTED_S, line->args[0]);
		}
	}
	free (line);
}


/*
===================
Cmd_ForwardToServer

Sends the entire command line over to the server
===================
*/
void Cmd_ForwardToServer (lparse_t *line)
{
	cbool is_cmd_command = (strcasecmp(line->args[0], "cmd") == 0);
	int startoffset = is_cmd_command ? 1 : 0;
	size_t offsetz = line->args[startoffset] - line->chopped; // Offset into line after whitespace
	char *cmd_after_whitespace = &line->original[offsetz];

	if (cls.state != ca_connected)
	{
		Con_PrintLinef ("Can't " QUOTED_S ", not connected", line->args[0]);
		return;
	}

	if (cls.demoplayback)
		return;		// not really connected

	MSG_WriteByte (&cls.message, clc_stringcmd);

	// If cmd, strip that off.
	// If say, expand the macro
	// Otherwise just send the command

	if (!strcasecmp(line->args[0], "say") || !strcasecmp(line->args[0], "say_team") ) 
	{
		const char *new_text = Talk_Macros_Expand (cmd_after_whitespace);
		SZ_Print (&cls.message, new_text);
	}
	else if (is_cmd_command == false || line->count > 1) // Make sure not a raw "cmd" with no extra args
	{
		SZ_Print (&cls.message, cmd_after_whitespace);
	}
	else SZ_Print (&cls.message, NEWLINE); // "cmd" with no extra args

}

#if 0 // Not used in code
/*
================
Cmd_CheckParm

Returns the position (1 to argc-1) in the command's argument list
where the given parameter apears, or 0 if not present
================
*/

int Cmd_CheckParm (const char *parm)
{
	int i;

	if (!parm)
		System_Error ("Cmd_CheckParm: NULL");

	for (i = 1; i < Cmd_Argc (); i++)
		if (! strcasecmp (parm, Cmd_Argv (i)))
			return i;

	return 0;
}
#endif
#pragma message ("_host_post_initialized Should this command remove itself???")
#pragma message ("r_farclip for software")



cmd_function_t *Cmd_Find (const char *s)
{
	cmd_function_t	*cur;

	for (cur = cmd_functions; cur ; cur = cur->next)
	{
		if (!strcasecmp (cur->name, s))
			return cur;
	}

	return NULL;
}

cmd_alias_t *Alias_Find (const char *s)
{
	cmd_alias_t	*cur;

	for (cur = cmd_alias; cur ; cur = cur->next)
	{
		if (!strcasecmp (cur->name, s))
			return cur;
	}

	return NULL;
}