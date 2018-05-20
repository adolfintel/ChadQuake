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
// cvar.c -- dynamic variable tracking

#include "quakedef.h"

#pragma message ("DEP: Refresh rate")
#pragma message ("DEP: Mirrors")

static cvar_t	*cvar_vars;


//==============================================================================
//
//  USER COMMANDS
//
//==============================================================================

void Cvar_Reset (const char *name); //johnfitz


void _Help_f (lparse_t *line)
{
	cbool match = false;
	const char 	*scommandfind = line->args[1];

	switch (line->count) {
	case 1:
		// Quake help menu
		M_Menu_Help_f ();
		return;

	case 2:
		// That's us!
		break;

	default:
		// Your holding it wrong.
		Con_PrintLinef ("Usage: %s <text> - describe help for command or console variable.", line->args[0]);
		return;
	}

	// Provide help for a command
	if (1) { // Commands
		extern cmd_function_t *cmd_functions;
		cmd_function_t *cmd;
		for (cmd = cmd_functions ; cmd ; cmd = cmd->next) {
			if (String_Does_Match_Caseless (cmd->name, scommandfind)) {
				Con_PrintContf ("%cCommand:", 2); // Bronzy
				Con_PrintLinef (" %s", cmd->description);
				return;
			}
		}
	}

	if (1) { // Cvars
		cvar_t *cvar;
		for (cvar = cvar_vars ; cvar ; cvar = cvar->next) {
			if ((cvar->flags & CVAR_COURTESY))
				continue; // Not really available

			if (String_Does_Match_Caseless (cvar->name, scommandfind)) {
				Con_PrintContf ("%cConsole variable:", 2); // Bronzy
				//Con_PrintLinef (" %s" NEWLINE "%s", cvar->name, cvar->description);
				Con_PrintLinef (" %s", cvar->description);
				return;
			}
		}
	}

	Con_PrintLinef ("Command or console variable " QUOTED_S " not found", scommandfind);
}


void Cvar_Find_f (lparse_t *line)
{
	const char 	*partial = line->args[1];
	int			count = 0;
	// 15. 1. 8. 1. 52 = 77
	// 20. 1. 8. 1. 47 = 77
	// Cmd_AddCommands
	char		dashes1[256];
	char		dashes2[256];
	char		dashes3[256];
	const int	col_a_20 = 20;
	const int	col_b_8  = 8;
	int			col_c_length = (console1.buffer_columns - 1) - ( col_a_20 + 1) - (col_b_8 + 1);

	col_c_length = CLAMP(0, col_c_length, (int) (sizeof(dashes3) - 1)  ) ;

	String_Edit_Repeat (dashes1, sizeof(dashes1), "\x1E", col_a_20);		dashes1[0] = '\x1D';	dashes1[col_a_20 - 1]		= '\x1F';
	String_Edit_Repeat (dashes2, sizeof(dashes2), "\x1E", col_b_8);			dashes2[0] = '\x1D';	dashes1[col_b_8 - 1]		= '\x1F';
	String_Edit_Repeat (dashes3, sizeof(dashes3), "\x1E", col_c_length);	dashes3[0] = '\x1D';	dashes1[col_c_length - 1]	= '\x1F';

	// printf("%.*s\n", str_len, str);

	// console1.buffer_columns
	if (line->count != 2) {
		Con_PrintLinef ("Usage: %s <text> - find settings containing text in name or description", line->args[0]);
		Con_PrintLinef ("Type " QUOTEDSTR("find all") " for everything");
		return;
	}

	if (String_Does_Match_Caseless (partial, "all")) {
		partial = NULL;
	}

	//Con_SafePrintLinef (NEWLINE "Find " QUOTED_S, partial);

	Con_SafePrintLinef ("%c" NEWLINE "%-*.*s %-*.*s %-*.*s", 2 /*bronzy*/,
		col_a_20, col_a_20, "Name",
		col_b_8, col_b_8, "Value",
		col_c_length, col_c_length, "Help");

	//Con_SafePrintLinef ("%Name            Value    Help");//, 2 /*bronzy cheat*/ );
	Con_SafePrintLinef ("%s %s %s", dashes1, dashes2, dashes3);

	if (1) { // Commands
		extern cmd_function_t	*cmd_functions;
		cmd_function_t	*cmd;

		for (cmd = cmd_functions ; cmd ; cmd = cmd->next)
		{
			if (partial &&  !strcasestr(cmd->name, partial) && !strcasestr(cmd->description, partial))
				continue;

			Con_SafePrintLinef ("%-*.*s %-*.*s %-*.*s",
				col_a_20, col_a_20, cmd->name,
				col_b_8, col_b_8, "command",
				col_c_length, col_c_length, cmd->description);

			count++;
		}
	}

	if (1) { // Cvars
		cvar_t		*cvar;
		for (cvar = cvar_vars ; cvar ; cvar = cvar->next) {
			if ((cvar->flags & CVAR_COURTESY))
				continue; // Not really available

			if (partial && !strcasestr(cvar->name, partial) && !strcasestr(cvar->description, partial))
				continue;

			Con_SafePrintLinef ("%-*.*s %-*.*s %-*.*s",
				col_a_20, col_a_20, cvar->name,
				col_b_8, col_b_8, cvar->string,
				col_c_length, col_c_length, cvar->description);
			count ++;
		}
	}



	Con_SafePrintLinef (NEWLINE "%d matches related to " QUOTED_S " -- type " QUOTEDSTR ("help <name>") " for full description.", count, line->args[1]);
}



/*
============
Cvar_List_f -- johnfitz
============
*/
void Cvar_List_f (lparse_t *line)
{
	cvar_t	*cvar;
	const char 	*partial;
	int		len, count;

	if (line->count > 1)
	{
		partial = line->args[1];
		len = strlen(partial);
	}
	else
	{
		partial = NULL;
		len = 0;
	}

	count = 0;
	for (cvar = cvar_vars ; cvar ; cvar = cvar->next)
	{
		// Baker: Courtesy cvar do not get updated after initialization
		if ((cvar->flags & CVAR_COURTESY))
			continue;

		if (partial && strncmp(partial, cvar->name, len))
		{
			continue;
		}
		Con_SafePrintLinef ("%s%s %s " QUOTED_S,
			(cvar->flags & CVAR_ARCHIVE) ? "*" : " ",
			(cvar->flags & CVAR_NOTIFY)  ? "s" : " ",
			cvar->name,
			cvar->string);
		count++;
	}

	Con_SafePrintContf ("%d cvars", count);
	if (partial)
	{
		Con_SafePrintContf (" beginning with " QUOTED_S, partial);
	}
	Con_SafePrintLine ();
}

/*
============
Cvar_Inc_f -- johnfitz
============
*/
void Cvar_Inc_f (lparse_t *line)
{
	switch (line->count)
	{
	default:
	case 1:
		Con_PrintLinef ("inc <cvar> [amount] : increment cvar");
		break;
	case 2:
		Cvar_SetValueByName (line->args[1], Cvar_VariableValue(line->args[1]) + 1);
		break;
	case 3:
		Cvar_SetValueByName (line->args[1], Cvar_VariableValue(line->args[1]) + atof(line->args[2]));
		break;
	}
}

// Baker: Added
#define COUNT_10 10
static float saved_values[COUNT_10];

void Cvar_ValSave_f (lparse_t *line)
{
//	char sval[32];
	cvar_t *var = NULL;

	int slotnum;
	switch (line->count) {
	default:
	case 1: Con_PrintLinef ("Usage: %s <cvar> <slotnum 0 to %d> : Save a cvar value to slot", line->args[0], COUNT_10);
			for (slotnum = 0; slotnum < COUNT_10; slotnum ++)
				Con_PrintLinef ("Slot %2d: %g", slotnum, saved_values[slotnum]  );
			break;
	case 3: slotnum = atoi(line->args[2]);
			if (!in_range(0, slotnum, COUNT_10)) {
				Con_PrintLinef ("Slotnum is invalid %d, must be 0 to %d", slotnum, COUNT_10);
				break;
			}
			var = Cvar_Find (line->args[1]);
			if (!var) {
				Con_PrintLinef ("Cvar %s not found", line->args[1]);
				break;
			}

			saved_values[slotnum] = Cvar_VariableValue(line->args[1]);
			break;
	}
}

void Cvar_ValLoad_f (lparse_t *line)
{
//	char sval[32];
	cvar_t *var = NULL;

	int slotnum;
	switch (line->count) {
	default:
	case 1: Con_PrintLinef ("Usage: %s <cvar> <slotnum 0 to %d>  : Set cvar value from slot value", line->args[0], COUNT_10);
			for (slotnum = 0; slotnum < COUNT_10; slotnum ++)
				Con_PrintLinef ("Slot %2d: %g", slotnum, saved_values[slotnum]  );
			break;
	case 3: slotnum = atoi(line->args[2]);
			if (!in_range(0, slotnum, COUNT_10)) {
				Con_PrintLinef ("Slotnum is invalid %d, must be 0 to %d", slotnum, COUNT_10);
				break;
			}
			var = Cvar_Find (line->args[1]);
			if (!var) {
				Con_PrintLinef ("Cvar %s not found", line->args[1]);
				break;
			}
			Cvar_SetValueByName (line->args[1], saved_values[slotnum]);
			break;
	}
}

// Baker: Added
void Cvar_Multiply_f (lparse_t *line)
{
	switch (line->count) {
	default:
	case 1: Con_PrintLinef ("Usage: %s <cvar> [amount] : multiply cvar", line->args[0]);  break;
	case 3: Cvar_SetValueByName (line->args[1], Cvar_VariableValue(line->args[1]) * atof(line->args[2])); break;
	}
}

// Baker: Added
void Cvar_Dec_f (lparse_t *line)
{
	switch (line->count)
	{
	default:
	case 1:
		Con_PrintLinef ("dec <cvar> [amount] : decrement cvar");
		break;
	case 2:
		Cvar_SetValueByName (line->args[1], Cvar_VariableValue(line->args[1]) - 1);
		break;
	case 3:
		Cvar_SetValueByName (line->args[1], Cvar_VariableValue(line->args[1]) - atof(line->args[2]));
		break;
	}
}


/*
============
Cvar_Toggle_f -- johnfitz
============
*/
void Cvar_Toggle_f (lparse_t *line)
{
	switch (line->count)
	{
	default:
	case 1:
		Con_PrintLinef ("toggle <cvar> : toggle cvar");
		break;
	case 2:
		if (Cvar_VariableValue(line->args[1]))
			Cvar_SetValueByName (line->args[1], 0);
		else
			Cvar_SetValueByName (line->args[1], 1);
		break;
	}
}

/*
============
Cvar_Cycle_f -- johnfitz
============
*/
void Cvar_Cycle_f (lparse_t *line)
{
	int i;

	if (line->count < 3)
	{
		Con_PrintLinef ("cycle <cvar> <value list>: cycle cvar through a list of values");
		return;
	}

	//loop through the args until you find one that matches the current cvar value.
	//yes, this will get stuck on a list that contains the same value twice.
	//it's not worth dealing with, and i'm not even sure it can be dealt with.
	for (i = 2; i < line->count; i++)
	{
		// Baker: Removed the float check.  String check should work fine in every circumstance
		// The float check was superfluous.
		if (!strcmp(line->args[i], Cvar_VariableString(line->args[1]))) // <--- Only user of Cvar_VariableString!
			break;
	}

	if (i == line->count)
		Cvar_SetByName (line->args[1], line->args[2]); // no match
	else if (i + 1 == line->count)
		Cvar_SetByName (line->args[1], line->args[2]); // matched last value in list
	else
		Cvar_SetByName (line->args[1], line->args[i + 1]); // matched earlier in list
}

/*
============
Cvar_Reset_f -- johnfitz
============
*/
void Cvar_Reset_f (lparse_t *line)
{
	switch (line->count)
	{
	default:
	case 1:
		Con_PrintLinef ("reset <cvar> : reset cvar to default");
		break;
	case 2:
		Cvar_Reset (line->args[1]);
		break;
	}
}

/*
============
Cvar_ResetAll_f -- johnfitz
============
*/
void Cvar_ResetAll_f (lparse_t *unused)
{
	cvar_t	*var;

	for (var = cvar_vars ; var ; var = var->next)
	{
		// Courtesy cvars can't be reset.  They don't know a default value.
		if (var->flags & CVAR_COURTESY)
			continue;

		Cvar_Reset (var->name);
	}
}

/*
============
Cvar_ResetCfg_f -- QuakeSpasm
============
*/
void Cvar_ResetCfg_f (lparse_t *line)
{
	cvar_t	*var;

	for (var = cvar_vars ; var ; var = var->next)
	{
		if (var->flags & CVAR_COURTESY)
			continue;

		if (var->flags & CVAR_ARCHIVE) Cvar_Reset (var->name);
	}
}

//==============================================================================
//
//  INIT
//
//==============================================================================

/*
============
Cvar_Init -- johnfitz
============
*/

void Cvar_Init (void)
{
	Cmd_AddCommands (Cvar_Init);
}

//==============================================================================
//
//  CVAR FUNCTIONS
//
//==============================================================================

/*
============
Cvar_Find
============
*/
cvar_t *Cvar_Find (const char *var_name)
{
	cvar_t	*var;

	for (var = cvar_vars ; var ; var = var->next)
	{
		if (!strcmp(var_name, var->name))
		{
			// Baker: Courtesy cvar do not get updated after initialization
			if (host_post_initialized && var->flags & CVAR_COURTESY)
				continue;

			return var;
		}
	}

	return NULL;
}

cvar_t *Cvar_FindAfter (const char *prev_name, unsigned int with_flags)
{
	cvar_t	*var;

	if (*prev_name)
	{
		var = Cvar_Find (prev_name);
		if (!var)
			return NULL;
		var = var->next;
	}
	else
		var = cvar_vars;

	// search for the next cvar matching the needed flags
	while (var)
	{
		// Baker: Courtesy cvar do not get updated after initialization
		if (!(var->flags & CVAR_COURTESY))
		{
			if ((var->flags & with_flags) || !with_flags)
				break;
		}
		var = var->next;
	}
	return var;
}


/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValue (const char *var_name)
{
	cvar_t	*var;

	var = Cvar_Find (var_name);
	if (!var) {
		Con_DPrintLinef  ("Cvar %s not found", var_name);
		return 0;
	}
	return atof (var->string);
}


/*
============
Cvar_VariableString
============
*/
const char *Cvar_VariableString (const char *var_name)
{
	cvar_t *var;

	var = Cvar_Find (var_name);
	if (!var)
		return empty_string;
	return var->string;
}


/*
============
Cvar_CompleteVariable
============
*/
const char *Cvar_CompleteVariable (const char *partial)
{
	cvar_t	*cvar;
	int	len;

	len = strlen(partial);
	if (!len)
		return NULL;

// check functions
	for (cvar = cvar_vars ; cvar ; cvar = cvar->next)
	{
		// Baker: Courtesy cvar do not get updated after initialization
		if ((cvar->flags & CVAR_COURTESY))
			continue;

		if (!strncmp(partial, cvar->name, len))
			return cvar->name;
	}

	return NULL;
}

/*
============
Cvar_Reset -- johnfitz
============
*/
void Cvar_ResetQuick (cvar_t *var)
{
	if (var->flags & CVAR_COURTESY)
	{
#ifdef _DEBUG
		alert ("Courtesy variable reset attempt");
#endif
		System_Error ("Tried to reset courtesy variable");
	}

	Cvar_SetQuick (var, var->default_string);
}

void Cvar_Reset (const char *name)
{
	cvar_t	*var;

	var = Cvar_Find (name);
	if (!var)
		Con_PrintLinef ("variable " QUOTED_S " not found", name);
	else
		Cvar_ResetQuick (var);
}

void Cvar_SetQuick (cvar_t *var, const char *value)
{
// 	if (var->flags & (CVAR_ROM|CVAR_LOCKED))
//		return;
	if (!(var->flags & CVAR_REGISTERED))
		return;

	if (!var->string)
		var->string = Z_Strdup (value);
	else
	{
		size_t len;

		if (!strcmp(var->string, value)) {
			// Keep going so callback happens.  How often does this really happen that an exact duplicate is set. Let's find out.
			// Con_DPrintLinef ("Identical set %s = " QUOTED_S ".", var->name, var->string);
			// Have to otherwise we get recursive endless loop.  Sucks.
			return;	// no change
		}

		len = strlen (value);
		if (len != strlen(var->string))
		{
			Z_Free ((void *)var->string);
			var->string = (char *) Z_Malloc (len + 1);
		}
		memcpy ((char *)var->string, value, len + 1);
	}

#ifdef SUPPORTS_CUTSCENE_PROTECTION
	var->user_value = var->value = atof (var->string);
#endif // SUPPORTS_CUTSCENE_PROTECTION

	//johnfitz -- save initial value for "reset" command
#if 0 // Baker ... ???  We were setting the default constantly.  Good job.
	if (!var->default_string) {
		// if ( !Flag_Check (var->flags, CVAR_REGISTERED) )
		var->default_string = Z_Strdup (var->string);
	}

	//johnfitz -- during initialization, update default too
	else if (!host_initialized)
	{
	//	Dedicated_PrintLinef ("changing default of %s: %s -> %s",
	//		   var->name, var->default_string, var->string);
		Z_Free ((void *)var->default_string);
		var->default_string = Z_Strdup (var->string);
	}
	//johnfitz
#endif

	if (!(var->flags & CVAR_COURTESY))
	{
		if (var->callback)
			var->callback (var);

		// JPG 3.00 - rcon (64 doesn't mean anything special, but we need some extra space because NET_MAXMESSAGE == RCON_BUFF_SIZE)
		if (rcon_active && (rcon_message.cursize < rcon_message.maxsize - (int)strlen(var->name) - (int)strlen(var->string) - 64))
		{
			rcon_message.cursize--;
			MSG_WriteStringf (&rcon_message, QUOTED_S " set to " QUOTED_S NEWLINE, var->name, var->string); // Should be good.
		}
	}
}

void Cvar_SetValueQuick (cvar_t *var, const float value)
{
	char sval[32];

	String_Write_NiceFloatString (sval, sizeof(sval), value); // Baker: %f doesn't write scientific notation, %g will on big numbers

	Cvar_SetQuick (var, sval);
}

/*
============
Cvar_Set
============
*/
void Cvar_SetByName (const char *var_name, const char *value)
{
	cvar_t		*var;

	var = Cvar_Find (var_name);
	if (!var)
	{	// there is an error in C code if this happens
		Con_SafePrintLinef ("Cvar_Set: variable %s not found", var_name);
		return;
	}

	Cvar_SetQuick (var, value);
}


#ifdef SUPPORTS_CUTSCENE_PROTECTION
// Baker: This comes from the progs if it sets requests a client-only cvar.
//			Or from the function below if from a demo or server.
static void Cvar_SetValue_Untrusted (const char *var_name, float newvalue)
{
	cvar_t		*var;

	var = Cvar_Find (var_name);
	if (!var)
	{	// there is an error in C code if this happens
		Con_PrintLinef ("Cvar_Set: variable %s not found", var_name);
		return;
	}

	// Baker: These are all untrusted sets.
	// However, if they aren't protected cvars, we are going to trust them anyway.
	// This is vital because especially in single player a mod might very legitimately
	// use cvars to store things across levels or such (even if that is bad practice.)

#if 1 // Trust some cvar sets
	if (!(var->flags & CVAR_CLIENT))
	{
		// Baker: Not a client oriented cvar so demo/progs/server is likely behaving properly
		// Or at least not suspiciously
		// So we won't firewall this cvar set.
#ifdef CUTSCENE_DEBUG
		Con_PrintLinef ("Cvar_Set: Untrusted set of %s to %g but we are accepting as real", var->name, newvalue);
#endif // CUTSCENE_DEBUG
		Cvar_SetValueQuick (var, newvalue);
		return;
	}
#endif // #if 1


	if (var->value == newvalue)
	{
		// Ok, but no effect.  Let's continue anyway
	}

	// Avoid the multiple set of a bad value scenario
	if (!(var->flags & CVAR_TAMPERED))
		var->user_value = var->value;

	var->flags |= CVAR_TAMPERED;
	var->value = newvalue;
#ifdef CUTSCENE_DEBUG
	Con_PrintLinef ("Cvar_Set: Untrusted set of %s to %g", var->name, newvalue);
#endif // CUTSCENE_DEBUG
}



// Baker: Everything that happens from a demo or a server comes through here.
void Cvar_Set_Untrusted (const char *var_name,  const char *newvaluestr)
{
	float newvalue = atof (newvaluestr);
	if (newvalue == 0 && !isdigit(newvaluestr[0]) && newvaluestr[0] )
	{
		Con_PrintLinef ("Cvar_Set: Rejected request to set cvar %s to " QUOTED_S, var_name, newvaluestr);
		return;
	}

	Cvar_SetValue_Untrusted (var_name, newvalue);
}

void Cvar_Clear_Untrusted (void)
{
	cvar_t	*var;
	int num_restored = 0;

	for (var = cvar_vars ; var ; var = var->next)
	{
		if ((var->flags & CVAR_TAMPERED))
		{
#ifdef CUTSCENE_DEBUG
			Con_PrintLinef ("Restoring %s to %g (untrusted value was %g)", var->name, var->user_value, var->value);
#endif // CUTSCENE_DEBUG
			var->value = var->user_value;
			var->flags -= CVAR_TAMPERED;
			num_restored ++;
		}
	}
#ifdef CUTSCENE_DEBUG
	Con_PrintLinef ("Cvar_Clear_Untrusted: %d untrusted cvar(s) restored", num_restored);
#endif // CUTSCENE_DEBUG
}

#endif // SUPPORTS_CUTSCENE_PROTECTION


/*
============
Cvar_SetValueByName
============
*/
void Cvar_SetValueByName (const char *var_name, const float value)
{
	char sval[32];

	String_Write_NiceFloatString (sval, sizeof(sval), value); // Baker: %f doesn't write scientific notation, %g will on big numbers

	Cvar_SetByName (var_name, sval);
}





/*
============
Cvar_UnRegisterVariable

Removes a variable
============
*/

void Cvar_UnregisterVariable (cvar_t *variable)
{
	cvar_t	*prev, *cursor;

	// Baker: We still have to look through the list to find prev. :(
	for (cursor = cvar_vars, prev = NULL ; cursor ; prev = cursor, cursor = cursor->next)
	{
		if (!strcmp(variable->name, cursor->name))
			break;
	}

	if (cursor == NULL)
	{
		Con_PrintLinef ("Cvar_UnregisterVariable: Couldn't find variable %s", variable->name);
		return;
	}

#pragma message ("Baker: We aren't supposed to unlinking courtesy cvars!!!")
	// Instead: cur->string = cvar_empty_string; and add COURTESY_FLAG

	// Link it away
	if (prev) prev->next = cursor->next;

	Z_Free ((void *)variable->string);
}



/*
============
Cvar_RegisterVariable

Adds a freestanding variable to the variable list.
============
*/


void Cvar_Register (cvar_t *variable)
{
	char	value[512];
//	cbool	set_rom;
	cvar_t	*cursor,*prev; //johnfitz -- sorted list insert

// first check to see if it has already been defined
	if (Cvar_Find (variable->name))
	{
		Con_SafePrintLinef ("Can't register variable %s, already defined", variable->name);
		return;
	}

// check for overlap with a command
	if (Cmd_Exists (variable->name))
	{
		Con_SafePrintLinef ("Cvar_RegisterVariable: %s is a command", variable->name);
		return;
	}

// link the variable in
	//johnfitz -- insert each entry in alphabetical order
	if (cvar_vars == NULL ||
	    strcmp(variable->name, cvar_vars->name) < 0) // insert at front
	{
		variable->next = cvar_vars;
		cvar_vars = variable;
	}
	else //insert later
	{
		prev = cvar_vars;
		cursor = cvar_vars->next;
		while (cursor && (strcmp(variable->name, cursor->name) > 0))
		{
			prev = cursor;
			cursor = cursor->next;
		}
		variable->next = prev->next;
		prev->next = variable;
	}
	//johnfitz
	variable->flags |= CVAR_REGISTERED;

// copy the value off, because future sets will Z_Free it
	c_strlcpy (value, variable->string);

//	variable->default_string = NULL;  // No keep!  Nov

// Don't unregister cvars or do we?
#if 1 // Baker Nov 24
	variable->default_string = NULL;
	variable->default_string = Z_Strdup (variable->string);
	// Don't update it during initialization.  A true reset would reset need to re-run the quake.rc which should do the autoexec.cfg
#endif
	variable->string = NULL;
//	if (!(variable->flags & CVAR_CALLBACK))
//		variable->callback = NULL;

// set it through the function to be consistent
//	set_rom = (variable->flags & CVAR_ROM);
//	variable->flags &= ~CVAR_ROM;
	Cvar_SetQuick (variable, value);
//	if (set_rom)
//		variable->flags |= CVAR_ROM;
}


/*
void Cvar_RegisterVariableWithCallback (cvar_t *var, cvarcallback_t func)
{
	var->callback = func;

	Cvar_RegisterVariable (var);
}
*/

void Cvar_RegisterTemp (cvar_t *variable)
{
	Cvar_Register (variable);
}


/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
#ifdef SUPPORTS_CUTSCENE_PROTECTION
cbool	Cvar_Command (cbool src_server, cvar_t *var, lparse_t *line)
#endif // SUPPORTS_CUTSCENE_PROTECTION
{
#if 0
	cvar_t			*v;

// check variables
	v = Cvar_Find (line->args[0]);
	if (!v)
		return false;
#endif

// perform a variable print or set
	if (line->count == 1)
	{
		if ((var->flags & CVAR_TAMPERED) && var->value != var->user_value)
		{
			Con_PrintLinef (QUOTED_S " is " QUOTED_G " (altered!)", var->name, var->value);
			Con_PrintLinef ("user preference is " QUOTED_S, var->string);
		}
		else {
			Con_PrintContf (QUOTED_S " is " QUOTED_S " ", var->name, var->string);
			if (String_Does_Match(var->string, var->default_string) )
				Con_PrintLinef ("[default value]");
			else
				Con_PrintLinef ("[default: " QUOTED_S "]", var->default_string);
			if (con_verbose.value) {
				Con_PrintLinef ("info: %s", var->description);
			}
		}
		return true;
	}

#ifdef SUPPORTS_CUTSCENE_PROTECTION
// Baker:
	if (src_server == true) // From demo or server
	{
		// Baker: Remember if it looks like a cut-scene restore, it cannot be allowed to execute
		// and instead we use our restore
		if (Cutscene_Guardian_PF_cvar_set_Think(var->name, line->args[1]))
		{
			return true; // The cut-scene guardian would have restored our cvars too
		}
		Cvar_Set_Untrusted (var->name, line->args[1]);
	}
	else
#endif // SUPPORTS_CUTSCENE_PROTECTION
		Cvar_SetQuick (var, line->args[1]);
	return true;
}


/*
============
Cvar_WriteVariables

Writes lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (FILE *f)
{
	cvar_t	*var;

	for (var = cvar_vars ; var ; var = var->next)
	{
		// Baker: These only write the original value and only if they had one.
		if (var->flags & CVAR_COURTESY)
		{
			if (!var->string[0])
				continue;// Never received a value, do not write to config
		}

		if (var->flags & CVAR_ARCHIVE)
			fprintf (f, "%s " QUOTED_S "\n", var->name, var->string);
	}
}

#ifdef SUPPORTS_CUTSCENE_PROTECTION

typedef struct
{
	const char*		cvarname;
	const float		cut_scene_init_hint;
	const float		cur_scene_end_hint;
} cutscene_vars_t;

cutscene_vars_t cutscene_vars[] =
{
	{"cl_bob",			0,	 0.02	},
	{"crosshair",		0,	 1		},
	{"fov",				90,	 206	},
	{"r_drawviewmodel",	0,	 1		},
	{"viewsize",		120, 100	},
};  int num_cutscene_vars = sizeof(cutscene_vars)/sizeof(cutscene_vars[0]);

// Baker: We help the cut-scene and know to detect end-of-cutscene
void Cutscene_Guardian_Begin (void)
{
	// Help ensure a thorough cut-scene presentation
	Cvar_SetValue_Untrusted (scr_crosshair.name, 0);
	Cvar_SetValue_Untrusted (scr_fov.name, 90);
}


// Baker: Check to see if a cut-scene is starting
cbool Cutscene_Guardian_PF_cvar_set_Think (const char *var_name, const char *val_string)
{
	float newval = atof (val_string);
	int i;

	if (!strcmp(var_name, "viewsize") && newval == 120)
	{
#ifdef CUTSCENE_DEBUG
	Con_PrintLinef ("Cutscene QuakeC Begin" NEWLINE ": Cvar_Set %s %g", var_name, newval);
#endif // CUTSCENE_DEBUG
		Cutscene_Guardian_Begin ();
		return false;
	}

	for (i = 0; i < num_cutscene_vars; i ++)
	{
		cutscene_vars_t *cur = &cutscene_vars[i];
		// Runs through fov, viewsize, crosshair, cl_bob, r_drawviewmodel

		if (!strcmp (cur->cvarname, var_name))
		{
			// Determine intention ...
			if (newval == cur->cur_scene_end_hint || newval == CUTSCENE_GUARDIAN_FAKE_VALUE)
			{
				// The false means don't clear it.
				// More attempts to restore the "right" values may follow
				// and we will restore the true correct values each time it happens.
#ifdef CUTSCENE_DEBUG
				Con_PrintLinef ("Cutscene QuakeC End: Cvar_Set %s %g", var_name, newval);
#endif // CUTSCENE_DEBUG
				Cvar_Clear_Untrusted ();
				return true; // A progs attempt to restore the cvars may never be permitted to happen
			}
#ifdef CUTSCENE_DEBUG
			else Con_PrintLinef ("Cutscene QuakeC ??: Cvar_Set %s %g", var_name, newval);
#endif // CUTSCENE_DEBUG
			return false;
		}
	}
	return false;
}

cbool Cutscene_Guardian_PF_cvar_Get_Think (const char *var_name)
{
	cvar_t* var = Cvar_Find (var_name);

	if (!var)
		return false; // Couldn't find it anyway

	if (var->flags & CVAR_CLIENT)
	{
#ifdef CUTSCENE_DEBUG
		Con_PrintLinef ("QuakeC asked for the value of %s", var_name);
#endif // CUTSCENE_DEBUG
		return true; // Requested a client var associated with cutscenes
	}
	return false; // Looks ok.
}


#endif // SUPPORTS_CUTSCENE_PROTECTION


#ifdef WINQUAKE_RENDERER_SUPPORT

void External_Textures_Change_f (cvar_t *var) {}
void GL_Fullbrights_f (cvar_t *var) {}
void GL_Overbright_f (cvar_t *var) {}
void TexMgr_Anisotropy_f (cvar_t *var) {}
void TexMgr_GreyScale_f (cvar_t *var) {}
void TexMgr_Warp_Downscale_f (cvar_t *var) {}

void SCR_Conwidth_f (cvar_t *var) { }
void R_SetClearColor_f (cvar_t *var) {}
void R_VisChanged (cvar_t *var) {}
void TexMgr_Init (void) {}
void TexMgr_TextureMode_f  (cvar_t *var) {}
#else
void D_Init (void) {};
#endif // WINQUAKE_RENDERER_SUPPORT


extern void CaptureCodec_Validate (cvar_t *var);
extern void External_Textures_Change_f (cvar_t *var);
extern void GL_Fullbrights_f (cvar_t *var);
extern void GL_Overbright_f (cvar_t *var);
extern void Host_Callback_Notify (cvar_t *var);
// extern void PlayNoise_f (cvar_t *var); Security camera mock 

extern void Host_Changelevel_Required_Msg (cvar_t *var);
extern void Max_Edicts_f (cvar_t *var);
extern void Mod_Flags_Refresh_f (cvar_t *var);
extern void R_PolyBlendChanged_f (cvar_t *var);
extern void R_SetClearColor_f (cvar_t *var);
//extern void R_VisChanged (cvar_t *var);
extern void R_SetParticleTexture_f (cvar_t *var);
//extern void R_VisChanged (cvar_t *var);
extern void SCR_Conwidth_f (cvar_t *var);
extern void R_SetClearColor_f (cvar_t *var);
extern void R_VisChanged (cvar_t *var);
extern void SV_Cheats_f (cvar_t *var);
extern void S_Snd_Speed_Notify_f (cvar_t *var);
extern void Stain_Change_f (cvar_t *var);
extern void TexMgr_Anisotropy_f (cvar_t *var);
extern void TexMgr_Warp_Downscale_f (cvar_t *var);
extern void TexMgr_GreyScale_f (cvar_t *var);
extern void VID_Local_Multisample_f (cvar_t *var);
extern void VID_Local_Vsync_f (cvar_t *var);
extern void View_LavaCshift_f (cvar_t *var);
extern void View_SlimeCshift_f (cvar_t *var);
extern void View_WaterCshift_f (cvar_t *var);
extern void external_music_toggle_f (cvar_t *var);

cvar_t* all_vars[] = {
#define CVAR_DEF(_initfunc_,_class_,_dependency_,_internal_name_,_name_,_default_string_,_flags_,_callback_,_help_) \
	&_internal_name_,
#include "cvar_list_sheet.h"
}; int numall_vars = sizeof(all_vars)/sizeof(all_vars[0]);


// Baker


// Baker:  In Visual Studio use /P to generate preprocessed C file if needed to debug macro expansion
// And remember to page down!

#define CVAR_DEF(_initfunc_,_class_,_dependency_,_internal_name_,_name_,_default_string_,_flags_,_callback_,_help_) \
	cvar_t _internal_name_ = { _name_, _default_string_, _dependency_, _flags_, _initfunc_, _callback_, _help_ };

#include "cvar_list_sheet.h"



void Courtesy_Cvars (void)
{
	int i;

	for (i = 0; i < numall_vars; i ++)
	{
		cvar_t* cur = all_vars[i];
//		const char *name = cur->name;

		if (!Cvar_Find (cur->name))
		{
			cur->string = empty_string;
			cur->flags = cur->flags | CVAR_COURTESY | CVAR_ARCHIVE;

			Cvar_Register (cur);
		}
	}
}


void Cvar_AddCvars (voidfunc_t initializer)
{
	int i;

	for (i = 0; i < numall_vars; i ++)
	{
		cvar_t* cur = all_vars[i];

		if (cur->init_func != initializer)
			continue; // Not initialized here

		switch (cur->dependency)
		{
		case DEP_D3D9:
			if (DIRECT3D_WRAPPER_VERSION != 9)
				continue; // d3d9 var in non-d3d9
			break;

		case DEP_GL:
			if (build.renderer != renderer_hardware)
				continue; // gl var in software renderer
			break;

		case DEP_SW:
			if (build.renderer != renderer_software)
				continue; // sw var in hardware renderer
			break;

		case DEP_AVI:
			if (!build.video_avi_capture)
				continue;

			break;

		case DEP_VSYNC:
			if (!build.video_vsync)
				continue;

			break;

		case DEP_NONE:
			break;

		case DEP_MIRROR:
			break;
//			if (!build.render_mirror)

		default: // Watch these!
			break;
		}
#pragma message ("Watch the continues")

		Cvar_Register (cur);
	}

}