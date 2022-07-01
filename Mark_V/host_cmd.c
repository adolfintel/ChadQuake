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

#include "quakedef.h"

/*
==================
Host_Quit_f
==================
*/

void Host_Quit (void)
{
	CL_Disconnect ();
	Host_ShutdownServer(false);

	System_Quit ();
}

// This is the "quit command".  I can only come from the console command.
void Host_Quit_f (lparse_t *unused)
{
	if (cmd_from_server)
		Con_WarningLinef ("Server send us a " QUOTEDSTR("quit") " command.  Ignoring ...");
	// Force shareware sell screen?
	Host_Quit ();
}


const char *gamedir_type_names[MAXGAMETYPES] =
{
	"", // id1 doesn't require one
	"-rogue" ,
	"-hipnotic",
	"-quoth",
	"-nehahra",
};

const char *Gamedir_TypeName (void)
{
	return gamedir_type_names[com_gametype];
}

typedef enum
{
	game_fail_none = 0,
	game_fail_rogue = 1,
	game_fail_hipnotic = 2,
	game_fail_quoth = 3,
	game_fail_nehahra = 4,
	MAX_GAMEDIR_TYPES =5,
	game_fail_shareware = 6,
	game_fail_relative_path_not_allowed = 7,
	game_fail_not_installed = 8,
} gamedir_fail_t;

const char *fail_reason_strings[] =
{
	NULL, // id1 doesn't require one
	"Rogue is not installed.", // 2
	"Hipnotic is not installed.", // 2
	"Quoth is not installed.", // 3
	"Nehahra is not installed.", // 3
	NULL, // 4
	"You must have the registered version to use modified games", // 5
	"Relative pathnames are not allowed.", // 6
	"Game not installed.", // 7
};



gametype_t gametype_eval (const char *hudstring)
{
	if	 	 (!hudstring)							return gametype_standard; // To avoid potential NULL comparison
	else if  (!strcasecmp (hudstring, "-rogue"))	return gametype_rogue;
	else if  (!strcasecmp (hudstring, "-hipnotic"))	return gametype_hipnotic;
	else if  (!strcasecmp (hudstring, "-quoth"))	return gametype_quoth;
	else if  (!strcasecmp (hudstring, "-nehahra"))	return gametype_nehahra;

	return gametype_standard;
}

typedef enum
{
	game_change_fail = -1,
	game_no_change = 0,
	game_change_success = 1
} game_result_t;

// returns 0 if everything is ok
gamedir_fail_t game_available (const char *dir, gametype_t gm)
{
	cbool custom_game = !!strcasecmp (dir, GAMENAME_ID1 /* "id1*/);

	if (static_registered == false && (custom_game || gm != gametype_standard))
		return game_fail_shareware; // shareware and modified gamedir

	if (strstr (dir, ".."))
		return game_fail_relative_path_not_allowed;

	if (gm == gametype_rogue && File_Exists (basedir_to_url("rogue"))  == false)
		return game_fail_rogue;

	if (gm == gametype_hipnotic && File_Exists (basedir_to_url ("hipnotic")) == false)
		return game_fail_hipnotic;

	if (gm == gametype_quoth && File_Exists (basedir_to_url ("quoth")) == false)
		return game_fail_quoth;

	if (gm == gametype_nehahra && File_Exists (basedir_to_url ("nehahra")) == false)
		return game_fail_nehahra;

	if (custom_game && File_Exists (basedir_to_url (dir)) == false)
		return game_fail_not_installed;

	return game_fail_none /* 0 */;
}


#pragma message ("Do not allow a user command in console to change gamedir while demo is playing")
#pragma message ("Technically a demo could change the gamedir while we are in menu!!")
cbool HD_Folder_Ok (char *s)
{
	const char *cursor;
	int ch;

	File_URL_Edit_SlashesForward_Like_Unix (s);

	if (strstr (s, "//"))
		return false;

	for (cursor = s; *cursor; cursor ++) {
		ch = *cursor;

		if (isdigit (ch) || isalpha (ch) || ch == '_' || ch == '/' || ch == ',')
			continue;

		return false;
	}

	return true; // It's only alpha numeric plus specified character.
}

cbool in_hdfolder_cmd;
void HD_Folder_f (lparse_t *line)
{
	const char *new_basepath = line->args[1];
	const char *fail_string = NULL;
	int result;

	if (line->count !=2) {
		Con_SafePrintLinef ("Set high content resolution folder.  Set to " QUOTEDSTR("") " for none."); // Con_SafePrintLinef ("Pak files in a hd folder will not be read, only free standing files will.", hd_folder.description);
		Con_SafePrintLinef ("Pak files in a hd folder will not be read, only free standing files will.");
		Con_SafePrintLine ();
		Con_SafePrintLinef ("\02Current HD folder is set to " QUOTED_S ".", hd_folder.string);
		return;
	}

#if 0
	if (sv.active || cls.state == ca_connected) {
		Con_SafePrintLinef ("Disconnect first.");
		return;
	}
#endif

	do {
		char folder_url[MAX_OSPATH];
		if (!new_basepath[0])
			break; // Fine!

		FS_FullPath_From_Basedir (folder_url, new_basepath);

		if (!HD_Folder_Ok (/* yes we edit it*/ (char *)new_basepath)) {
			Con_SafePrintLinef ("Folder " QUOTED_S " contains illegal characters.", new_basepath);
			Con_SafePrintLinef ("Alphanumeric and underscore is ok.");
			Con_SafePrintLine ();
			return;
		}
#if 0
		if (!File_Exists (folder_url) || !File_Is_Folder(folder_url)) {
			Con_SafePrintLinef ("Folder " QUOTED_S " does not exist.", new_basepath);
			return;
		}
#endif
		// Should be ok
	} while (0);

	Cvar_SetQuick (&hd_folder, new_basepath);
	Con_SafePrintLinef ("HD folder set to " QUOTED_S ".", hd_folder.string);

	in_hdfolder_cmd = true;
	{
		char samedir_as_current[MAX_OSPATH];
		c_strlcpy (samedir_as_current, File_URL_SkipPath(com_gamedir));
		result = Host_Gamedir_Change (samedir_as_current, "" /*hud type skipped*/, false /*not liveswitch*/, &fail_string, true /*force*/);
		if (result == -1 /*fail*/)
			Con_SafePrintLinef ("A problem occurred when trying to reset the gamedir.  (%s)", fail_string);

	}
	in_hdfolder_cmd = false;
	//Con_SafePrintLinef ("Game is %s", com_gamedir);
}

int Host_Gamedir_Change (const char *gamedir_new, const char *new_hud_typestr, cbool liveswitch, const char** info_string, cbool force)
{
	gametype_t	new_gametype	= gametype_eval (new_hud_typestr);
	cbool		is_custom		= !!strcasecmp(gamedir_new, GAMENAME_ID1); // GAMENAME_ID1 is "id1"

	cbool		gamedir_change	= !!strcasecmp (gamedir_shortname(), gamedir_new );
	cbool		gametype_change	= (new_gametype != com_gametype);
	cbool		any_change		= (gamedir_change || gametype_change);

	int			change_fail		= game_available (gamedir_new, new_gametype);

	// Don't do anything, just go.
	if (force)
		goto force_only;

	if (any_change == false)
	{
		Con_DPrintLinef ("Gamedir change is no change");
		return game_no_change;
	}

	if (change_fail)
	{
		*info_string = fail_reason_strings[change_fail];
		Con_DPrintLinef ("%s", *info_string);
		return game_change_fail;
	}

	// Everything ok now ....
	Con_DPrintLinef ("New game and/or hud style requested");

	com_gametype = new_gametype;

force_only:
	// If we aren't receiving this via connected signon switch
	// then kill the server.

	if (liveswitch == false)
	{
		//Kill the server
		CL_Disconnect ();
		Host_ShutdownServer(true);
	}

	//Write config file
	Host_WriteConfiguration ();

	//Kill the extra game if it is loaded. Note: RemoveAllPaths and COM_AddGameDirectory set com_gamedir
	COM_RemoveAllPaths ();

	com_modified = true;

	// add these in the same order as ID do (mission packs always get second-lowest priority)
	switch (com_gametype)
	{
	case gametype_rogue:			COM_AddGameDirectory ("rogue", false /*real*/);		break;
	case gametype_hipnotic:			COM_AddGameDirectory ("hipnotic", false /*real*/);	break;
	case gametype_quoth:			COM_AddGameDirectory ("quoth", false /*real*/);		break;
	case gametype_nehahra:			COM_AddGameDirectory ("nehahra", false /*real*/);	break; // Nehahra must manually be added
	case gametype_standard:			com_modified = false;				break;
	default:						break; // Nehahra hits here
	}

	if (is_custom)
	{
		com_modified = true;

		COM_AddGameDirectory (gamedir_new, false /*real*/);
	}

	com_hdfolder_count = 0;
	if (!isDedicated && hd_folder.string[0] && HD_Folder_Ok (/* editing it maybe !*/(char *) hd_folder.string)) {
		char		this_qpath[MAX_QPATH_64];
		const char	*cursor = hd_folder.string;

		while (  (cursor = String_Get_Word (cursor, ",", this_qpath, sizeof(this_qpath)))  ) {
			char folder_url[MAX_OSPATH];
			// Construct the full url
			FS_FullPath_From_Basedir (folder_url, this_qpath);
			if (File_Exists (folder_url) && File_Is_Folder(folder_url)) {
				// It's going to put screenshots and configs and demos in there.
				// Needs to be a read only folder, though.  I think we are fine.  We write to com_gamedir.

				com_modified = true;
				COM_AddGameDirectory (this_qpath, true /*hd only*/);
			}
			else {
				extern cbool in_hdfolder_cmd;
				if (in_hdfolder_cmd)
					Con_SafePrintLinef ("Folder " QUOTED_S " does not exist.", folder_url);
			}

		}
	}

	//clear out and reload appropriate data
#ifdef SUPPORTS_NEHAHRA
	Nehahra_Shutdown ();
#endif // SUPPORTS_NEHAHRA



	Keys_Flush_ServerBinds ();		// Flush server keybinds.  sv_gameplayfix_alias_flush could be set to 0, which means we must do it here.
	Cmd_Unalias_ServerAliases ();	// Flush server keybinds.  sv_gameplayfix_binds_flush could be set to 0, which means we must do it here.


	Cache_Flush (NULL);
	Mod_ClearAll_Compact (); // Baker: Prevent use of wrong cache data

	if (!isDedicated)
	{
#ifdef GLQUAKE_TEXTURE_MANAGER // Baker: No texture mgr in WinQuake
		TexMgr_NewGame ();
#endif // GLQUAKE_TEXTURE_MANAGER
#ifdef WINQUAKE_PALETTE // FitzQuake does this in TexMgr_NewGame
		VID_Palette_NewGame ();
#endif // WINQUAKE_PALETTE

		Draw_NewGame ();

		R_NewGame ();

		Cvar_ResetQuick (&sv_progs);
		cls.music_run = false;
	}

	Lists_NewGame ();
	Recent_File_NewGame ();

#ifdef SUPPORTS_NEHAHRA
	Nehahra_Init ();
#endif // SUPPORTS_NEHAHRA

	// Return description of current setting
	*info_string = gamedir_new;
	return game_change_success;
}


/*
==================
Host_Game_f
==================
*/



void Host_Game_f (lparse_t *line)
{
	const char *gametypename;
	const char *gamedir_directory;
	const char *hudstyle = NULL;
	const char *feedback_string;
	int result;

	switch (line->count)
	{
	case 3:
		hudstyle = line->args[2];
		// Falls through ...
	case 2:
		gamedir_directory = line->args[1];
		result = Host_Gamedir_Change (line->args[1], line->args[2], false, &feedback_string, false /*don't force*/);

		switch (result)
		{
		case game_change_fail:
			Con_PrintLinef ("%s", feedback_string);
			break;
		case game_no_change:
			Con_PrintLinef ("Game already set!");
			break;
		case game_change_success:
			Con_PrintLinef (QUOTEDSTR("game") " changed to " QUOTED_S, feedback_string);
			break;
		}
		break;

	default:
		//Diplay the current gamedir
		gametypename = Gamedir_TypeName ();
		Con_PrintLinef (QUOTEDSTR("game") " is " QUOTEDSTR("%s%s"), gamedir_shortname(), gametypename[0] ?
					va(" %s", gametypename) : "" );
		if (hd_folder.string[0])
			Con_PrintLinef ("HD Folder = " QUOTED_S, hd_folder.string);
		Con_PrintLinef ("Start map is %s", game_startmap);
		break;
	}

}

#if 0 // Fare thee well.  "map" now does this pretty well!
/*
=============
Host_Mapname_f -- johnfitz
=============
*/
void Host_Mapname_f (void)
{
	if (sv.active)
	{
		Con_PrintLinef (QUOTEDSTR ("mapname") " is " QUOTED_S, sv.name);
		return;
	}

	if (cls.state == ca_connected)
	{
		Con_PrintLinef (QUOTEDSTR ("mapname") " is " QUOTED_S, cl.worldname);
		return;
	}

	Con_PrintLinef ("no map loaded");
}
#endif // Fare thee well, "mapname" command.  "map" does this well now.


/*
==================
Host_Status_f
==================
*/
void Host_Status_f (lparse_t *line)
{
	client_t	*client;
	int			seconds;
	int			minutes;
	int			hours = 0;
	int			j;
	int		    (*printline_fn) (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));

	if (cmd_source == src_command)
	{
		if (!sv.active)
		{
			Cmd_ForwardToServer (line);
			return;
		}
		printline_fn = Con_PrintLinef;
	}
	else
		printline_fn = SV_ClientPrintLinef;

	printline_fn ("host:    %s", hostname.string); // Cvar_VariableString ("hostname"));
	printline_fn ("version: %1.2f build %d", (float)QUAKE_VERSION, (int)ENGINE_BUILD);
	if (svs.listening) {
		if (ipv4Available) printline_fn ("ipv4:    %s:%d", my_ipv4_address, net_hostport);
		if (ipv6Available) printline_fn ("ipv6:    %s:%d", my_ipv6_address, net_hostport);
	}

	printline_fn ("map:     %s", sv.name);
	printline_fn ("players: %d active (%d max)" NEWLINE, net_activeconnections, svs.maxclients_public); // Because we are telling them the cap
	for (j = 0, client = svs.clients ; j < svs.maxclients_internal ; j++, client++) // Because we can now change the cap in-game.
	{
		if (!client->active)
			continue;
		seconds = (int)(net_time - NET_QSocketGetTime(client->netconnection) );
		minutes = seconds / 60;
		if (minutes)
		{
			seconds -= (minutes * 60);
			hours = minutes / 60;
			if (hours)
				minutes -= (hours * 60);
		}
		else
			hours = 0;

		printline_fn ("#%-2d %-16.16s  %3d  %2d:%02d:%02d", j + 1, client->name, (int)client->edict->v.frags, hours, minutes, seconds);
		//print_fn ("   %s\n", NET_QSocketGetAddressString(client->netconnection));

		if (cmd_source == src_command || !pq_privacy_ipmasking.value)
			printline_fn ("   %s", NET_QSocketGetTrueAddressString(client->netconnection));
		else if (pq_privacy_ipmasking.value ==1)
			printline_fn ("   %s", NET_QSocketGetMaskedAddressString(client->netconnection));
		else printline_fn ("   private");
	}
}


/*
==================
Host_God_f

Sets client to godmode
==================
*/
void Host_God_f (lparse_t *line)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer (line);
		return;
	}

	if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	if (sv.disallow_major_cheats && !host_client->privileged)
	{
		SV_ClientPrintLinef ("No cheats allowed, use sv_cheats 1 and restart level to enable.");
		return;
	}

	//johnfitz -- allow user to explicitly set god mode to on or off
	switch (line->count)
	{
	case 1:
		sv_player->v.flags = (int)sv_player->v.flags ^ FL_GODMODE;
		if (!((int)sv_player->v.flags & FL_GODMODE) )
			SV_ClientPrintLinef ("godmode OFF");
		else
			SV_ClientPrintLinef ("godmode ON");
		break;
	case 2:
		if (atof(line->args[1]))
		{
			sv_player->v.flags = (int)sv_player->v.flags | FL_GODMODE;
			SV_ClientPrintLinef ("godmode ON");
		}
		else
		{
			sv_player->v.flags = (int)sv_player->v.flags & ~FL_GODMODE;
			SV_ClientPrintLinef ("godmode OFF");
		}
		break;
	default:
		Con_PrintLinef ("god [value] : toggle god mode. values: 0 = off, 1 = on");
		break;
	}
	//johnfitz
}

/*
==================
Host_Notarget_f
==================
*/
void Host_Notarget_f (lparse_t *line)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer (line);
		return;
	}

	if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	if (sv.disallow_major_cheats && !host_client->privileged)
	{
		SV_ClientPrintLinef ("No cheats allowed, use sv_cheats 1 and restart level to enable.");
		return;
	}

	//johnfitz -- allow user to explicitly set notarget to on or off
	switch (line->count)
	{
	case 1:
		sv_player->v.flags = (int)sv_player->v.flags ^ FL_NOTARGET;
		if (!((int)sv_player->v.flags & FL_NOTARGET) )
			SV_ClientPrintLinef ("notarget OFF");
		else
			SV_ClientPrintLinef ("notarget ON");
		break;
	case 2:
		if (atof(line->args[1]))
		{
			sv_player->v.flags = (int)sv_player->v.flags | FL_NOTARGET;
			SV_ClientPrintLinef ("notarget ON");
		}
		else
		{
			sv_player->v.flags = (int)sv_player->v.flags & ~FL_NOTARGET;
			SV_ClientPrintLinef ("notarget OFF");
		}
		break;
	default:
		Con_PrintLinef ("notarget [value] : toggle notarget mode. values: 0 = off, 1 = on");
		break;
	}
	//johnfitz
}


/*
==================
Host_Noclip_f
==================
*/
void Host_Noclip_f (lparse_t *line)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer (line);
		return;
	}

	if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	if (sv.disallow_major_cheats && !host_client->privileged)
	{
		SV_ClientPrintLinef ("No cheats allowed, use sv_cheats 1 and restart level to enable.");
		return;
	}

	//johnfitz -- allow user to explicitly set noclip to on or off
	switch (line->count)
	{
	case 1:
		if (sv_player->v.movetype != MOVETYPE_NOCLIP)
		{
			cl.noclip_anglehack = true;
			sv_player->v.movetype = MOVETYPE_NOCLIP;
			SV_ClientPrintLinef ("noclip ON");
		}
		else
		{
			cl.noclip_anglehack = false;
			sv_player->v.movetype = MOVETYPE_WALK;
			SV_ClientPrintLinef ("noclip OFF");
		}
		break;
	case 2:
		if (atof(line->args[1]))
		{
			cl.noclip_anglehack = true;
			sv_player->v.movetype = MOVETYPE_NOCLIP;
			SV_ClientPrintLinef ("noclip ON");
		}
		else
		{
			cl.noclip_anglehack = false;
			sv_player->v.movetype = MOVETYPE_WALK;
			SV_ClientPrintLinef ("noclip OFF");
		}
		break;
	default:
		Con_PrintLinef ("noclip [value] : toggle noclip mode. values: 0 = off, 1 = on");
		break;
	}
	//johnfitz
}


/*
==================
Host_Fly_f

Sets client to flymode
==================
*/
void Host_Fly_f (lparse_t *line)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer (line);
		return;
	}

	if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	if (sv.disallow_major_cheats && !host_client->privileged)
	{
		SV_ClientPrintLinef ("No cheats allowed, use sv_cheats 1 and restart level to enable.");
		return;
	}

	//johnfitz -- allow user to explicitly set noclip to on or off
	switch (line->count)
	{
	case 1:
		if (sv_player->v.movetype != MOVETYPE_FLY)
		{
			sv_player->v.movetype = MOVETYPE_FLY;
			SV_ClientPrintLinef ("flymode ON");
		}
		else
		{
			sv_player->v.movetype = MOVETYPE_WALK;
			SV_ClientPrintLinef ("flymode OFF");
		}
		break;
	case 2:
		if (atof(line->args[1]))
		{
			sv_player->v.movetype = MOVETYPE_FLY;
			SV_ClientPrintLinef ("flymode ON");
		}
		else
		{
			sv_player->v.movetype = MOVETYPE_WALK;
			SV_ClientPrintLinef ("flymode OFF");
		}
		break;
	default:
		Con_PrintLinef ("fly [value] : toggle fly mode. values: 0 = off, 1 = on");
		break;
	}
	//johnfitz
}

void Host_Legacy_FreezeAll_f (lparse_t *unused)
{
	Con_PrintLinef ("Use 'freezeall' instead of sv_freezenonclients.  It is shorter.");
}

void Host_Apropos_f (lparse_t *line)
{
	Con_PrintLinef ("Use 'find' instead of apropos.  It is shorter.");
}


void Host_Freezeall_f (lparse_t *line)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer (line);
		return;
	}


	if (pr_global_struct->deathmatch && !host_client->privileged) {
		// Allow freezeall in deathmatch if just 1 player, allows debugging.
		// Or sv_cheats -1 or 1.
		if (svs.maxclients_internal > 1 && sv.disallow_minor_cheats)
			return;
	}

	if (sv.disallow_major_cheats && !host_client->privileged)
	{
		SV_ClientPrintLinef ("No cheats allowed, use sv_cheats 1 and restart level to enable.");
		return;
	}

	switch (line->count)
	{
	case 1:
		sv.frozen = !sv.frozen;

		if (sv.frozen)
			SV_ClientPrintLinef ("freeze mode ON");
		else
			SV_ClientPrintLinef ("freeze mode OFF");

		break;
	case 2:
		if (atof(line->args[1]))
		{
			sv.frozen = true;
			SV_ClientPrintLinef ("freeze mode ON");
		}
		else
		{
			sv.frozen = false;
			SV_ClientPrintLinef ("freeze mode OFF");
		}
		break;
	default:
		Con_PrintLinef ("freezeall [value] : toggle freeze mode. values: 0 = off, 1 = on");
		break;
	}

}

/*
==================
Host_Ping_f

==================
*/
void Host_Ping_f (lparse_t *line)
{
	int		i, j, ping_display;
	float	total;
	client_t	*client;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer (line);
		return;
	}

	SV_ClientPrintLinef ("Client ping times:");
	for (i=0, client = svs.clients ; i<svs.maxclients_internal ; i++, client++) // Because we can now change the cap in game.
	{
		if (!client->active)
			continue;
		total = 0;
		for (j=0 ; j<NUM_PING_TIMES ; j++)
			total+=client->ping_times[j];
		total /= NUM_PING_TIMES;

		ping_display = (int)(total*1000);
		if (pq_ping_rounding.value)
			ping_display = CLAMP(40, c_rint(ping_display / 40) * 40, 999);
		SV_ClientPrintLinef ("%4d %s", ping_display, client->name);
	}
}


void Host_Changelevel_Required_Msg (cvar_t* var)
{
	if (host_post_initialized)
		Con_PrintLinef ("%s change takes effect on map restart/change.", var->name);
}


/*
===============================================================================

SERVER TRANSITIONS

===============================================================================
*/

int Host_ActiveWeapon_0_to_24_or_Neg1 (void)
{
	int active_weapon = cl.stats[STAT_ACTIVEWEAPON];
	int weapon_number = -1;

	// active_weapon 0 = axe
	if (!active_weapon)
		return (weapon_number = 0);
	else {
		int j;  for (j = 0; j < MAX_EFFECTIVE_WEAPON_COUNT_25 - 1 ; j ++) {
			int thisweap = ( 1 << j );
			if (active_weapon == ( 1 << j ) )
				return (weapon_number = j + 1);
		}
		return weapon_number; // Which is -1
	}
}

/*
======================
Host_Map_f

handle a
map <servername>
command from the console.  Active clients are kicked off.
======================
*/
void Host_Map_f (lparse_t *line)
{
	int		i;
	char	name[MAX_QPATH_64];

	// Quakespasm: map with no parameter says name
	if (line->count < 2)	//no map name given
	{
		if (isDedicated)
		{
			if (sv.active)
				Con_PrintLinef ("Current map: %s", sv.name);
			else
				Con_PrintLinef ("Server not active");
		}
		else if (cls.state == ca_connected)
		{
			char buf[32] = {0};

			Con_PrintLine ();
			Con_PrintLinef ("Current map: %s (title: %s)", cl.worldname, cl.levelname);
			Con_PrintLine ();
			Con_PrintLinef ("Sky key:         %s", level.sky_key);
			Con_PrintLinef ("Fog key:	      %s", level.fog_key);

			Con_PrintLinef ("Skyfog key:      %s", level.is_skyfog		? String_Write_NiceFloatString(buf, sizeof(buf), level.skyfog)		: "" );
			Con_PrintLinef ("Lava key:        %s", level.is_lavaalpha	? String_Write_NiceFloatString(buf, sizeof(buf), level.lavaalpha)	: "" );
			Con_PrintLinef ("Slime key:       %s", level.is_slimealpha ? String_Write_NiceFloatString(buf, sizeof(buf), level.slimealpha)	: "" );
//			Con_PrintLinef ("Teleport key:    %s", level.is_telealpha	? String_Write_NiceFloatString(buf, sizeof(buf), level.telealpha)	: "" );
			Con_PrintLinef ("Water key:       %s", level.is_wateralpha ? String_Write_NiceFloatString(buf, sizeof(buf), level.wateralpha)	: "" );
			Con_PrintLinef ("Sound clip dist: %s", level.is_sound_nominal_clip_dist ? String_Write_NiceFloatString(buf, sizeof(buf), level.sound_nominal_clip_dist) : "" );
			Con_PrintLine ();
			Con_PrintLinef ("Water-vised:     %s", level.water_vis_known ? (level.water_vis ? "Yes" : "No") : "Not determined yet" );

			Con_PrintLine ();
			Con_PrintLinef ("Active Weapon #: %d", Host_ActiveWeapon_0_to_24_or_Neg1() + 1 );
			Con_PrintLine ();
			Con_PrintLinef ("Type " QUOTEDSTR ("copy ents") " to copy entities to clipboard");
			Con_PrintLine ();
		}
		else
		{
			Con_PrintLinef ("map <levelname>: start a new server");
		}
		return;
	}


	if (cmd_source != src_command)
		return;

	CL_Clear_Demos_Queue (); // timedemo is a very intentional action

#ifdef BUGFIX_DEMO_RECORD_BEFORE_CONNECTED_FIX
	// Baker: Don't cause demo shutdown if started recording before
	// playing map
	// Since this is map startup, it doesn't affect anything else
	// Like typing "map <my name>" while already connected.
	if (cls.state == ca_connected)
#endif // BUGFIX_DEMO_RECORD_BEFORE_CONNECTED_FIX
		CL_Disconnect ();

	Host_ShutdownServer(false);

	SCR_BeginLoadingPlaque_Force_NoTransition ();
	Key_SetDest (key_game);
	console1.visible_pct = 0;

	cls.mapstring[0] = 0;
	for (i = 0 ; i < line->count ; i++)
	{
		c_strlcat (cls.mapstring, line->args[i]);
		c_strlcat (cls.mapstring, " ");
	}
	c_strlcat (cls.mapstring, "\n");

	svs.serverflags = 0;			// haven't completed an episode yet
	c_strlcpy (name, line->args[1]);
	SV_SpawnServer (name);
	if (!sv.active)
		return;

	if (!isDedicated)
	{
		memset (cls.spawnparms, 0, sizeof(cls.spawnparms));
		for (i = 2 ; i < line->count ; i++)
		{
			c_strlcat (cls.spawnparms, line->args[i]);
			c_strlcat (cls.spawnparms, " ");
		}

		Cmd_ExecuteString ("connect local", src_command);
	}
}

/*
==================
Host_Changelevel_f

Goes to a new map, taking all clients along
==================
*/
void Host_Changelevel_f (lparse_t *line)
{
	char	newlevel[MAX_QPATH_64];
	int		i; //johnfitz

	if (line->count != 2)
	{
		Con_PrintLinef ("changelevel <levelname> : continue game on a new level");
		return;
	}
	if (!sv.active || cls.demoplayback)
	{
		Con_PrintLinef ("Only the server may changelevel");
		return;
	}

	//johnfitz -- check for client having map before anything else
	c_snprintf1 (newlevel, "maps/%s.bsp", line->args[1]);
	if (COM_OpenFile (newlevel, &i) == -1)
		Host_Error ("cannot find map %s", newlevel);

// Baker: Shouldn't this close it?
#if 1
	COM_CloseFile (i);
#endif

	//johnfitz

	if (!isDedicated)
	{
		#if 1 // Baker:  Clear the noise
			S_BlockSound ();
			S_ClearBuffer ();
			S_UnblockSound ();
		#endif
	}

	SV_Host_Changelevel_SaveSpawnparms ();
	c_strlcpy (newlevel, line->args[1]);
	SV_SpawnServer (newlevel);
}

/*
==================
Host_Restart_f

Restarts the current server for a dead player
==================
*/
void Host_Restart_f (lparse_t *unnused)
{
	char	mapname[MAX_QPATH_64];

	if (cls.demoplayback || !sv.active)
		return;

	if (cmd_source != src_command)
		return;
	c_strlcpy (mapname, sv.name);	// mapname gets cleared in spawnserver
	SV_SpawnServer (mapname);
}

/*
==================
Host_Reconnect_f

This command causes the client to wait for the signon messages again.
This is sent just before a server changes levels
==================
*/
// This can have 2 different scenarios.  Entry might be at 0, in that case 4 should clear plaque
//If signon is 4, that is death or changelevel.  What do we do?  Clear immediately?  But in 0 case, don't

#define REAL_CONNECT_NEG_1 -1
cbool in_load_game; // Baker: Sheesh
void Host_Reconnect_f (lparse_t *unused)
{
	if (cls.state == ca_disconnected) {
		_Host_Connect (NULL); // Spikes's Quakeworld reconnect
		return;
	}


#if 1 // Baker:  Clear the noise
	S_BlockSound ();
	S_ClearBuffer ();
	S_UnblockSound ();
#endif

	// Consider stopping sound here?

	if (cls.demoplayback)
	{
		Con_DPrintLinef ("Demo playing; ignoring reconnect");
		SCR_EndLoadingPlaque (); // reconnect happens before signon reply #4
		return;
	}// Fixes? else if (cls.state == ca_disconnected)

	{
		if (!unused || unused->count != REAL_CONNECT_NEG_1) { // Dear Lord ... Why must it be so fooey.
			// Prevent begin loading from begin called if already called.
			if (!in_load_game)
				SCR_BeginLoadingPlaque_Force_Transition ();

		}
		cls.signon = 0;		// need new connection messages
	}
}

/*
=====================
Host_Connect_f

User command to connect to server
=====================
*/

void _Host_Connect (const char *name)
{
	lparse_t cheetz = {0};

	cls.demonum = -1;		// stop demo loop in case this fails
	if (cls.demoplayback)
	{
		CL_StopPlayback ();

		CL_Clear_Demos_Queue (); // connecting to a server is a very intentional action
		CL_Disconnect ();
	}

	if (name && String_Does_Match(name, "local")) {

		// Test plan?
		SCR_BeginLoadingPlaque_Force_NoTransition ();
		Key_SetDest (key_game);
		console1.visible_pct = 0;
	}
	else {
		// Not a local game of any kind including listen server (coop, bots, multiplayer)
		cheetz.count = REAL_CONNECT_NEG_1; // What a shitty hack I'm doing.
		// We don't want to stop demo record or do we?  This is effectively a disconnect.
		//CL_Disconnect (); // Is this too much?
		SCR_EndLoadingPlaque ();

	}

	CL_EstablishConnection (name); // Name may be null
	Host_Reconnect_f (&cheetz);
	if (name)
		c_strlcpy (server_name, name); // JPG - 3.50
}

#ifdef _DEBUG
void Host_Crash_f (void)
{
	void (*Crash_Me_Now) (void);

	Crash_Me_Now = NULL;
	Crash_Me_Now ();	// CRASH!!  To test crash protection.
}


void Host_SysError_f (void)
{
	System_Error ("User initiated System Error");
}

#endif // _DEBUG

void Host_Connect_f (lparse_t *line)
{
	_Host_Connect (line->args[1]);
}

keyvalue_t hintnames[MAX_NUM_HINTS + 1] =
{
	{ "game", hint_game },
	{ "skill", hint_skill },
	{ "fileserver_port", hint_fileserver_port },
	{ "", 0 },
};


/*
===============================================================================

LOAD / SAVE GAME

===============================================================================
*/

#define	SAVEGAME_VERSION	5
#define SAVEGAME_VERSION_6	6

/*
===============
Host_SavegameComment

Writes a 39 character comment describing the current level
===============
*/
static const char *Host_Savegame_Comment (void)
{
	static char	comment[SAVEGAME_COMMENT_LENGTH_39 + 1];
	int		i;
	char	kills[20];

	for (i = 0 ; i < SAVEGAME_COMMENT_LENGTH_39 ; i++)
		comment[i] = ' ';

	memcpy (comment, cl.levelname, c_min(strlen(cl.levelname),22)); //johnfitz -- only copy 22 chars.
	c_snprintf2 (kills, "kills:%3d/%3d", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
	memcpy (comment + 22, kills, strlen(kills));

	// convert space to _ to make stdio happy
	for (i = 0 ; i < SAVEGAME_COMMENT_LENGTH_39 ; i++)
	{
		if (comment[i] == ' ' || comment[i] == '\n' || comment[i] == 0x1A) // Aguirre save game comment fix part 1
			comment[i] = '_';
	}
	comment[SAVEGAME_COMMENT_LENGTH_39] = '\0';


	return comment;
}

const char *Host_Savegame (const char *in_savename, cbool user_initiated)
{
	static char	savegame_name[MAX_OSPATH];
	FILE	*f;

	// If the server is running multi-player, a different sv_progs.dat or has coop or deathmatch set, use version 6.
	// Remember, you can set coop 1 even in single player and a few single player mods use "deathmatch" to control behaviors.
	// So our goal is to use a version 6 save for anything that needs extra stuff.
	// A classic Quake client will refuse to load the game saving "Not a version 5 save game"
	cbool save_version_6 = svs.maxclients_internal > 1 || String_Does_Not_Match_Caseless (sv.progs_name, DEFAULT_PROGS_DAT_NAME) || pr_global_struct->coop || pr_global_struct->deathmatch;

	FS_FullPath_From_QPath (savegame_name, in_savename);
	File_URL_Edit_Force_Extension (savegame_name, ".sav", sizeof(savegame_name));

	if (user_initiated)
		Con_PrintLinef ("Saving game to %s...", savegame_name);

	f = FS_fopen_write (savegame_name, "w"); // Would need to add 'b' for binary here.
	if (!f)
	{
		Con_PrintLinef ("ERROR: couldn't open save file for writing.");
		return NULL;
	}

	fprintf (f, "%d\n", save_version_6 ? SAVEGAME_VERSION_6 : SAVEGAME_VERSION);
	fprintf (f, "%s\n", Host_Savegame_Comment() );

	if (save_version_6) {
		//  This should be enough for coop and typical deathmatch.
		//  Not going to overkill with writing all of them like samelevel, noexit, temp1, saved1, saved2, ...
		//  Something that fragile isn't our target.
		char cvar_settings[MAX_CMD_256] = {0};
		char	buf[32];
		c_strlcat (cvar_settings, va("%s %s;", "sv_progs",   sv.progs_name));
		c_strlcat (cvar_settings, va("%s %d;", "maxplayers", svs.maxclients_internal));
		c_strlcat (cvar_settings, va("%s %s;", "coop",  String_Write_NiceFloatString (buf, sizeof(buf), pr_global_struct->coop)));
		c_strlcat (cvar_settings, va("%s %s;", "deathmatch",  String_Write_NiceFloatString (buf, sizeof(buf), pr_global_struct->deathmatch)));
		c_strlcat (cvar_settings, va("%s %s;", pr_teamplay.name,  String_Write_NiceFloatString (buf, sizeof(buf), pr_teamplay.value)));
		fprintf (f, "%s\n", cvar_settings );
	}

	// NEED MORE HERE
	{
		int i, plnum;
		client_t *player;

		for (plnum = 0, host_client = svs.clients ; plnum < svs.maxclients_internal ; plnum ++) {
			player = &svs.clients[plnum];
			for (i = 0 ; i < NUM_SPAWN_PARMS ; i++) {
				fprintf (f, "%f\n", player->spawn_parms[i]);
			}
		}
	}
	//for (i = 0 ; i < NUM_SPAWN_PARMS ; i++)
	//	fprintf (f, "%f\n", svs.clients->spawn_parms[i]);

	fprintf (f, "%d\n", sv.current_skill);
	fprintf (f, "%s\n", sv.name);
	fprintf (f, "%f\n", sv.time);

// write the light styles
	{
		int i; for (i = 0 ; i < MAX_LIGHTSTYLES ; i++)
		{
			if (sv.lightstyles[i])
				fprintf (f, "%s\n", sv.lightstyles[i]);
			else
				fprintf (f,"m\n");
		}
	}

	ED_WriteGlobals (f);

	{
		int i; for (i = 0 ; i < sv.num_edicts ; i++)
			ED_Write (f, EDICT_NUM(i));
		// fflush (f); // Baker: This makes save games slow as hell.  Fix from MH
	}

	FS_fclose (f);

	Lists_Update_Savelist (); // Update the list - johnny.
	return savegame_name;
}


/*
===============
Host_Savegame_f
===============
*/
void Host_Savegame_f (lparse_t *line)
{
	const char *saved_name = NULL;
	int i;

	if (cmd_source != src_command)
		return;

	if (!sv.active)
	{
		Con_PrintLinef ("Not playing a local game.");
		return;
	}

	if (cl.intermission)
	{
		Con_PrintLinef ("Can't save in intermission.");
		return;
	}

#ifndef SUPPORTS_MULTIPLAYER_SAVES
	if (svs.maxclients_internal != 1)	// Because internal will still be set to 1 in single player.
	{
		Con_PrintLinef ("Can't save multiplayer games.");
		return;
	}
#endif // !SUPPORTS_MULTIPLAYER_SAVES

	if (line->count != 2)
	{
		Con_PrintLinef ("save <savename> : save a game");
		return;
	}

	if (strstr(line->args[1], ".."))
	{
		Con_PrintLinef ("Relative pathnames are not allowed.");
		return;
	}

	for (i = 0; i < svs.maxclients_internal; i ++) // Because the cap change in-game, in theory
	{
		if (svs.clients[i].active && (svs.clients[i].edict->v.health <= 0) )
		{
			Con_PrintLinef ("Can't savegame with a dead player");
			return;
		}
	}

	saved_name = Host_Savegame (line->args[1], true);

	if (saved_name)
	{
		Recent_File_Set_FullPath (saved_name);
		Con_PrintLinef ("done.");
	}
}


/*
===============
Host_Loadgame_f
===============
*/
void Host_Loadgame_f (lparse_t *line)
{
	char	name[MAX_OSPATH];
	FILE	*f;
	char	mapname[MAX_QPATH_64];
	float	time, tfloat;
	char	str[32768];
	const char *start;
	int		i, r;
	edict_t	*ent;
	int		entnum;
	int		version;
	cbool	anglehack;
	float	spawn_parms[NUM_SPAWN_PARMS];
	cbool	multiplayer_load = false;

	if (cmd_source != src_command)
		return;

	if (line->count != 2)
	{
		Con_PrintLinef ("load <savename> : load a game");
		return;
	}

	cls.demonum = -1;		// stop demo loop in case this fails

	FS_FullPath_From_QPath (name, line->args[1]);
	File_URL_Edit_Default_Extension (name, ".sav", sizeof(name));

// we can't call SCR_BeginLoadingPlaque, because too much stack space has
// been used.  The menu calls it before stuffing loadgame command
//	SCR_BeginLoadingPlaque ();

	Con_PrintLinef ("Loading game from %s...", name);
#pragma message ("Aguirre has a read-binary fix for save games with special characters")
	f = FS_fopen_read (name, "r"); // aguirRe: Use binary mode to avoid EOF issues in savegame files
	// Baker changed back to "r" from "rb" because it adds extra new lines..
	if (!f)
	{
		Con_PrintLinef ("ERROR: couldn't open load file for reading.");
		SCR_EndLoadingPlaque ();
		return;
	}

	fscanf (f, "%d\n", &version);
	if (version != SAVEGAME_VERSION && version != SAVEGAME_VERSION_6)
	{
		FS_fclose (f);
		Con_PrintLinef ("Savegame is version %d, not %d", version, SAVEGAME_VERSION);
		SCR_EndLoadingPlaque ();
		return;
	}

#if 0
	// aguirre: Kludge to read saved games with newlines in title
	do
		fscanf (f, "%s\n", str);
	while (!feof(f) && !strstr(str, "kills:"));
#else
	fscanf (f, "%s\n", str); // Read to end of line.
#endif


	if (version == SAVEGAME_VERSION_6) {
		char		this_setting[MAX_CMD_256];
		char		cvar_settings[MAX_CMD_256] = {0};
		const char	*cursor = cvar_settings;
		char		*tempstr_alloc = NULL;
		size_t		siz = 0, siz2 = 0;
		siz2 = getline (&tempstr_alloc, &siz, f);
		if (tempstr_alloc) {
			c_strlcpy (cvar_settings, tempstr_alloc);
			free (tempstr_alloc);
		}

		//fscanf (f, "%s\n", cvar_settings); // Read settings to end of line.

		while (  (cursor = String_Get_Word (cursor, ";", this_setting, sizeof(this_setting)))  ) {
			if (this_setting[0]) {
				float fval;
				char *cvname = this_setting;
				char *svalue = String_Skip_Char (this_setting, 32); // Space 32
				if (svalue <= cvname)
					break;

				svalue[-1] = 0;

				if (String_Does_Match_Caseless( cvname, "maxplayers")) {
					int save_maxplayers = atoi (svalue);
					if (save_maxplayers == 1)
						continue; // Standard rules apply.

					if (save_maxplayers > 1) {
						// This addresses the load multiplayer without multiplayer set.
						if (!sv.active || svs.maxclients_internal != save_maxplayers) {
							FS_fclose (f);
							Con_PrintLinef ("Multi-player load requires server started with correct maxplayers and all participants connected.");
							Con_PrintLinef ("1) Disconnect and set " QUOTEDSTR ("maxplayers %d") ".", save_maxplayers);
							Con_PrintLinef ("2) Then start a map get all particpants connected.");
							Con_PrintLinef ("3) Then load this game!");

							SCR_EndLoadingPlaque ();
							return;
						}
						multiplayer_load = true;
					}
					continue;
				} // End of maxplayers

				// Not maxplayers.  We'll just use trust and hope.
				fval = atof (svalue);
				Cvar_SetByName (cvname, svalue);
				svalue = svalue;


			} // End of if
			//
		} // End of while

	}

#if 1 // I would prefer this to be here.  It cannot be.   Causes an out of stack space for software renderer.
	SCR_BeginLoadingPlaque_Force_NoTransition ();
	Key_SetDest (key_game);
	console1.visible_pct = 0;
#endif
	{
		int i, plnum;
		client_t *player;

		for (plnum = 0, host_client = svs.clients ; plnum < svs.maxclients_internal; plnum ++) {
			player = &svs.clients[plnum];
			for (i = 0 ; i < NUM_SPAWN_PARMS ; i++) {
				fscanf (f, "%f\n", &spawn_parms[i]);
				player->spawn_parms[i] = spawn_parms[i];
				// How do we get them to reconnect?  SV_Spawn should do this
			}
		}
	}


	//for (i = 0; i < NUM_SPAWN_PARMS; i++)
	//	fscanf (f, "%f\n", &spawn_parms[i]);

// this silliness is so we can load 1.06 save files, which have float skill values
	fscanf (f, "%f\n", &tfloat);
	sv.current_skill = (int)(tfloat + 0.1);
	Cvar_SetValueQuick (&pr_skill, (float)sv.current_skill);

	fscanf (f, "%s\n", mapname);	// Read to end of line.  Limit 64 chars.  I think we have free reign to add stuff here.
	fscanf (f, "%f\n", &time);

	if (!multiplayer_load) {
		// Single player load game
		CL_Clear_Demos_Queue (); // timedemo is a very intentional action
		CL_Disconnect ();
	}

	in_load_game = true;
	SV_SpawnServer (mapname); // Will clear in_load_game


	if (!sv.active)
	{
		FS_fclose (f);
		Con_PrintLinef ("Couldn't load map");
		SCR_EndLoadingPlaque ();
		return;
	}
	sv.paused = true;		// pause until all clients connect
	sv.loadgame = true;

// load the light styles

	for (i = 0 ; i < MAX_LIGHTSTYLES ; i++)
	{
		fscanf (f, "%s\n", str);
		sv.lightstyles[i] = (const char *)Hunk_Strdup (str, "lightstyles");
	}

// load the edicts out of the savegame file
	entnum = -1;		// -1 is the globals
	while (!feof(f))
	{
		for (i = 0 ; i < (int) sizeof(str) - 1 ; i ++)
		{
			r = fgetc (f);
			if (r == EOF || !r)
				break;
			str[i] = r;
			if (r == '}')
			{
				i++;
				break;
			}
		}
		if (i ==  (int) sizeof(str)-1)
		{
			FS_fclose (f);
			System_Error ("Loadgame buffer overflow");
		}
		str[i] = 0;
		start = str;
		start = COM_Parse(str);
		if (!com_token[0])
			break;		// end of file
		if (strcmp(com_token,"{"))
		{
			FS_fclose (f);
			System_Error ("First token isn't a brace");
		}

		if (entnum == -1)
		{	// parse the global vars
			ED_ParseGlobals (start);
		}
		else
		{	// parse an edict

			ent = EDICT_NUM(entnum);
			memset (&ent->v, 0, progs->entityfields * 4);
			ent->free = false;
			ED_ParseEdict (start, ent, &anglehack);

		// link it into the bsp tree
			if (!ent->free)
				SV_LinkEdict (ent, false);
		}

		entnum++;
	}

	sv.num_edicts = entnum;
	sv.time = time;
	sv.auto_save_time = sv.time + AUTO_SAVE_MINIMUM_TIME;

	FS_fclose (f);

	if (!multiplayer_load)
	{
		//// This is the method for loading a single player game.	
#if 1
		for (i = 0 ; i < NUM_SPAWN_PARMS ; i++)
			svs.clients->spawn_parms[i] = spawn_parms[i];

		if (!isDedicated)
		{
			in_load_game = true;
			CL_EstablishConnection ("local");
			Host_Reconnect_f (NULL);
			in_load_game = false;
		}
	}
#endif
	// End of function
}

//============================================================================

/*
======================
Host_Name_f
======================
*/
void Host_Name_f (lparse_t *line)
{
	char	newName[32];

	if (line->count == 1)
	{
		Con_PrintLinef (QUOTEDSTR ("name") " is " QUOTED_S, cl_name.string);
		return;
	}
	if (line->count == 2)
	{
		c_strlcpy(newName, line->args[1]);
	}
	else
	{
		size_t offsetz = line->args[1] - line->chopped; // Offset into line after whitespace
		char *args_after_command = &line->original[offsetz];

		c_strlcpy(newName, args_after_command);
	}

	newName[15] = 0;	// client_t structure actually says name[32].

	if (cmd_source == src_command)
	{
		if (strcmp(cl_name.string, newName) == 0)
			return;
		Cvar_SetQuick (&cl_name, newName);
		if (cls.state == ca_connected)
			Cmd_ForwardToServer (line);
		return;
	}

	if (host_client->name[0] && strcmp(host_client->name, "unconnected") )
	{
		if (strcmp(host_client->name, newName) != 0)
			Con_PrintLinef ("%s renamed to %s", host_client->name, newName);
	}
	c_strlcpy (host_client->name, newName);
	host_client->edict->v.netname = PR_SetEngineString(host_client->name);

// send notification to all clients

	MSG_WriteByte (&sv.reliable_datagram, svc_updatename);
	MSG_WriteByte (&sv.reliable_datagram, host_client - svs.clients);
	MSG_WriteString (&sv.reliable_datagram, host_client->name);
}

#ifdef SUPPORTS_PQ_WORD_FILTER // Baker change
// TODO: Despace and remove non-alphanumeric and check
const char* bad_words[] =
{
// Lite bad words are replaced with asterisks
	"fuck", "f**k",
	"shit", "s__t",
	"butt", "****",
	"cock", "****",
	"douche", "******",

// Worse bad words are replaced with something stupid to make speaker feel awkward
	"wetback", "houston",
	"fukking", "beijing",
	"fucking", "cheddar",
	"anus", "snus",
	"fag", "fan",
	"fgt", "yam",
	"faggot", "maddog",
	"faget", "italy",
	"fggt", "hawk",
	"fagot", "cairo",
	"ngr", "ron",
	"nigger", "friend",
	"niger", "frank",
	"n1g", "man",
	"queer", "zebop",
	"cunt", "cute",
	"homo", "mang",
	"penis", "sonar",
	"penus", "sugar",
	"vagina", "robert",

	NULL, NULL
};

// 2 elements

char *String_Edit_Normalize_Text (const char *text)
{
	static unsigned char normalized_buffer[SYSTEM_STRING_SIZE_1024];
	unsigned char *cur = normalized_buffer;

	c_strlcpy (normalized_buffer, text);

	for ( ; *cur; cur ++)
	{
		if (*cur > 128) *cur -= 128;  // debronze
		*cur = tolower(*cur); // lower
		     if (*cur == '4') *cur = 'a';
		else if (*cur == '3') *cur = 'e';
		else if (*cur == '1') *cur = 'i';
		else if (*cur == '0') *cur = 'o';
	}

	return normalized_buffer;
}


char* WordFilter_Check (const char* text)
{
	static char new_text[SYSTEM_STRING_SIZE_1024];
	int i;

	const char *norm_text = String_Edit_Normalize_Text (text);
	char *curword;
	cbool replacement = false;

	for (i = 0; bad_words[i]; i += 2)
	{
		if ( (curword = strstr(norm_text, bad_words[i]) ) )
		{
			int replace_len = strlen(bad_words[i + 1]);
			int replace_offset = curword - norm_text;
#ifdef _DEBUG
			int src_len = strlen(bad_words[i + 0]);
			cbool ok = src_len = replace_len;
			if (!ok) System_Error ("No match length word filter!");
#endif
			if (replacement == false && (replacement = true) /* evile assignment */)
				c_strlcpy (new_text, text);

			memcpy (&new_text[replace_offset], bad_words[i + 1], replace_len);
		}
	}

	if (replacement)
		return new_text;
	else return NULL;
}

#endif // Baker change + SUPPORTS_PQ_WORD_FILTER



void Host_Say (lparse_t *line, cbool teamonly)
{
	int		j;
	client_t *client;
	client_t *save;
	char	*p;
	char	text[64];
	cbool	fromServer = false;

	if (cmd_source == src_command)
	{
		if (isDedicated)
		{
			fromServer = true;
			teamonly = false;
		}
		else
		{
			Cmd_ForwardToServer (line);
			return;
		}
	}

	if (line->count < 2)
		return;

	save = host_client;

// remove quotes if present
	p = (char*)&line->original[line->args[1]-line->chopped-1]; // Back us up 1 space before first arg in original unchopped line.

	if (*p == '\"')
	{
//		p++;
		p[strlen(p) - 1] = 0;
	}
	p++; // Baker, advance forward again since it was a space not a quote.

// turn on color set 1
	if (!fromServer)
	{
		char *text_filtered = NULL;

		double connected_time = (net_time - NET_QSocketGetTime(host_client->netconnection));
		// R00k - dont allow new connecting players to spam obscenities...
		if (pq_chat_connect_mute_seconds.value && connected_time < pq_chat_connect_mute_seconds.value)
			return;

		// JPG 3.00 - don't allow messages right after a colour/name change
		if (pq_chat_color_change_delay.value && sv.time - host_client->color_change_time < 1)
			return;

		if (pq_chat_frags_to_talk.value && connected_time < 90 && host_client->old_frags < pq_chat_frags_to_talk.value)
		{
			SV_ClientPrintLinef ("Server: Play some and then you can talk");
			return;
		}

		// JPG 3.20 - optionally remove '\r'
		if (pq_remove_cr.value)
		{
			char *ch;
			for (ch = p ; *ch ; ch++)
			{
				if (*ch == '\r')
					*ch += 128;
			}
		}

#ifdef SUPPORTS_PQ_WORD_FILTER // Baker change
		if (pq_chat_word_filter.string[0] != '0' && pq_chat_word_filter.string[0] && (text_filtered = WordFilter_Check (p)) )
			p = text_filtered;
#endif // Baker change + SUPPORTS_PQ_WORD_FILTER


		// JPG 3.11 - feature request from Slot Zero
		if (pq_chat_log_player_number.value)
			Dedicated_Printf ("(%s %s) #%d  ", NET_QSocketGetTrueAddressString(host_client->netconnection), text_filtered ? "word filtered": "", NUM_FOR_EDICT(host_client->edict)  );

		if (pr_teamplay.value && teamonly) // JPG - added () for mm2
			c_snprintf1 (text, "\001(%s): ", save->name);
		else c_snprintf1 (text, "\001%s: ", save->name);

	}
	else
		c_snprintf1 (text, "\001<%s> ", hostname.string);

	j = sizeof(text) - 2 - strlen(text);  // -2 for /n and null terminator
	if ((int)strlen(p) > j)
		p[j] = 0;

	c_strlcat (text, p);
	//c_strlcat (text, "\n");

	for (j = 0, client = svs.clients; j < svs.maxclients_internal; j++, client++) // Because the cap can now change in-game
	{
		if (!client || !client->active || !client->spawned)
			continue;
		if (pr_teamplay.value && teamonly && client->edict->v.team != save->edict->v.team)
			continue;
		host_client = client;
		SV_ClientPrintLinef ("%s", text);
	}
	host_client = save;

	// JPG 3.20 - optionally write player binds to server log
	if (pq_chat_to_log.value)
		Con_PrintLinef ("(%s) %s", NET_QSocketGetTrueAddressString(host_client->netconnection),  &text[1]); // Because
	else Dedicated_PrintLinef ("%s", &text[1]);
}


void Host_Say_f (lparse_t *line)
{
	Host_Say (line, false);
}


void Host_Say_Team_f (lparse_t *line)
{
	Host_Say (line, true);
}


void Host_Tell_f (lparse_t *line)
{
	int			j;
	client_t	*client;
	client_t	*save;
	char		*p;
	char		text[64];
	size_t		offsetz;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer (line);
		return;
	}

	if (line->count < 3)
		return;

	c_strlcpy (text, host_client->name);
	c_strlcat (text, ": ");

	offsetz = line->args[2] - line->chopped; // Offset into line after whitespace
	p = &line->original[offsetz]; // evil

// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[strlen(p)-1] = 0;
	}

// check length & truncate if necessary
	j = sizeof(text) - 2 - strlen(text);  // -2 for /n and null terminator
	if ((int)strlen(p) > j)
		p[j] = 0;

	c_strlcat (text, p);
	c_strlcat (text, "\n");

	save = host_client;
	for (j = 0, client = svs.clients; j < svs.maxclients_internal; j++, client++) // Because the cap can now change in-game
	{
		if (!client->active || !client->spawned)
			continue;
		if (strcasecmp(client->name, line->args[1]))
			continue;
		host_client = client;
		SV_ClientPrintf ("%s", text);
		break;
	}
	host_client = save;
}


/*
==================
Host_Color_f
==================
*/
void Host_Color_f (lparse_t *line)
{
	int		top, bottom;
	int		playercolor;

	if (line->count == 1)
	{
		Con_PrintLinef (QUOTEDSTR ("color") " is " QUOTEDSTR ("%d %d"), ((int)cl_color.value) >> 4, ((int)cl_color.value) & 0x0f);
		Con_PrintLinef ("color <0-13> [0-13]");
		return;
	}

	if (line->count == 2)
		top = bottom = atoi(line->args[1]);
	else
	{
		top = atoi(line->args[1]);
		bottom = atoi(line->args[2]);
	}

	top &= 15;
	if (top > 13)
		top = 13;
	bottom &= 15;
	if (bottom > 13)
		bottom = 13;

#ifdef SUPPORTS_COOP_ENHANCEMENTS
	if (vm_coop_enhancements.value &&  cmd_source != src_command && sv.active && pr_global_struct->coop && pr_teamplay.value)
	{
		if (isDedicated)
			bottom = 12; // Dedicated, you get yellow I guess.
		else bottom = (int)cl_color.value & 15;
	}
#endif // SUPPORTS_COOP_ENHANCEMENTS

	playercolor = top*16 + bottom;

	if (cmd_source == src_command)
	{
		Cvar_SetValueQuick (&cl_color, playercolor);
		if (cls.state == ca_connected)
			Cmd_ForwardToServer (line);
		return;
	}

	host_client->colors = playercolor;
	host_client->edict->v.team = bottom + 1;

// send notification to all clients
	MSG_WriteByte (&sv.reliable_datagram, svc_updatecolors);
	MSG_WriteByte (&sv.reliable_datagram, host_client - svs.clients);
	MSG_WriteByte (&sv.reliable_datagram, host_client->colors);
}

/*
==================
Host_Kill_f
==================
*/
void Host_Kill_f (lparse_t *line)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer (line);
		return;
	}

	if (sv_player->v.health <= 0)
	{
		SV_ClientPrintLinef ("Can't suicide -- already dead!");
		return;
	}

	pr_global_struct->time = sv.time;
	pr_global_struct->self = EDICT_TO_PROG(sv_player);
	PR_ExecuteProgram (pr_global_struct->ClientKill);
}


/*
==================
Host_Pause_f
==================
*/

void Host_Pause_f (lparse_t *line)
{
	// If playing back a demo, we pause now
	if (cls.demoplayback && cls.demonum == -1) // Don't allow startdemos to be paused
		cl.paused ^= 2;		// to handle demo-pause
	if (cmd_source == src_command)
	{
		if (!cls.demoplayback)
			Cmd_ForwardToServer (line);
		return;
	}

	if (!sv_pausable.value)
		SV_ClientPrintLinef ("Pause not allowed.");
	else
	{
		// If not playing back a demo, we pause here
		if (!cls.demoplayback) // Don't allow startdemos to be paused
			cl.paused ^= 2;		// to handle demo-pause
		sv.paused ^= 1;

		if (sv.paused)
		{
			SV_BroadcastPrintf ("%s paused the game" NEWLINE, PR_GetString( sv_player->v.netname));
		}
		else
		{
			SV_BroadcastPrintf ("%s unpaused the game" NEWLINE, PR_GetString( sv_player->v.netname));
		}

	// send notification to all clients
		MSG_WriteByte (&sv.reliable_datagram, svc_setpause);
		MSG_WriteByte (&sv.reliable_datagram, sv.paused);
	}
}

//===========================================================================


/*
==================
Host_PreSpawn_f
==================
*/
void Host_PreSpawn_f (lparse_t *unused)
{
	if (cmd_source == src_command)
	{
		Con_PrintLinef ("prespawn is not valid from the console");
		return;
	}

	if (host_client->spawned)
	{
		Con_PrintLinef ("prespawn not valid -- already spawned");	// JPG 3.02 already->already
		return;
	}

	SZ_Write (&host_client->message, sv.signon.data, sv.signon.cursize);
	MSG_WriteByte (&host_client->message, svc_signonnum);
	MSG_WriteByte (&host_client->message, 2);
	host_client->sendsignon = true;
}

/*
==================
Host_Spawn_f
==================
*/
void Host_Spawn_f (void)
{
	int		i;
	client_t	*client;
	edict_t	*ent;
#ifdef SUPPORTS_NEHAHRA
	func_t		RestoreGame;
    dfunction_t	*f;
#endif // SUPPORTS_NEHAHRA

	if (cmd_source == src_command)
	{
		Con_PrintLinef ("spawn is not valid from the console");
		return;
	}

	if (host_client->spawned)
	{
		Con_PrintLinef ("Spawn not valid -- already spawned");	// JPG 3.02 already->already
		return;
	}

// run the entrance script
	if (sv.loadgame)
	{	// loaded games are fully inited already
		// if this is the last client to be connected, unpause
		sv.paused = false;

#ifdef SUPPORTS_NEHAHRA
		// nehahra stuff
	    if ((f = ED_FindFunction("RestoreGame"))) {
			if ((RestoreGame = (func_t)(f - pr_functions))) {
				Con_DPrintLinef ("Calling RestoreGame");
				pr_global_struct->time = sv.time;
				pr_global_struct->self = EDICT_TO_PROG(sv_player);
				PR_ExecuteProgram (RestoreGame);
			}
		}
#endif // SUPPORTS_NEHAHRA
	}
	else
	{
		// set up the edict
		ent = host_client->edict;

		memset (&ent->v, 0, progs->entityfields * 4);
		ent->v.colormap = NUM_FOR_EDICT(ent);
		ent->v.team = (host_client->colors & 15) + 1;
		ent->v.netname = PR_SetEngineString(host_client->name);

		// copy spawn parms out of the client_t
		for (i=0 ; i< NUM_SPAWN_PARMS ; i++)
			(&pr_global_struct->parm1)[i] = host_client->spawn_parms[i];

		// call the spawn function
		pr_global_struct->time = sv.time;
		pr_global_struct->self = EDICT_TO_PROG(sv_player);
		PR_ExecuteProgram (pr_global_struct->ClientConnect);

		if ((System_DoubleTime() - NET_QSocketGetTime(host_client->netconnection) ) <= sv.time)
			Dedicated_PrintLinef ("%s entered the game", host_client->name);

		PR_ExecuteProgram (pr_global_struct->PutClientInServer);
	}


// send all current names, colors, and frag counts
	SZ_Clear (&host_client->message);

// send time of update
	MSG_WriteByte (&host_client->message, svc_time);
	MSG_WriteFloat (&host_client->message, sv.time);

	for (i = 0, client = svs.clients ; i < svs.maxclients_internal ; i++, client++) // Because the cap can now change in-game
	{
		MSG_WriteByte (&host_client->message, svc_updatename);
		MSG_WriteByte (&host_client->message, i);
		MSG_WriteString (&host_client->message, client->name);
		MSG_WriteByte (&host_client->message, svc_updatefrags);
		MSG_WriteByte (&host_client->message, i);
		MSG_WriteShort (&host_client->message, client->old_frags);
		MSG_WriteByte (&host_client->message, svc_updatecolors);
		MSG_WriteByte (&host_client->message, i);
		MSG_WriteByte (&host_client->message, client->colors);
	}

// send all current light styles
	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		MSG_WriteByte (&host_client->message, svc_lightstyle);
		MSG_WriteByte (&host_client->message, (char)i);
		MSG_WriteString (&host_client->message, sv.lightstyles[i]);
	}

//
// send some stats
//
	MSG_WriteByte (&host_client->message, svc_updatestat);
	MSG_WriteByte (&host_client->message, STAT_TOTALSECRETS);
	MSG_WriteLong (&host_client->message, pr_global_struct->total_secrets);

	MSG_WriteByte (&host_client->message, svc_updatestat);
	MSG_WriteByte (&host_client->message, STAT_TOTALMONSTERS);
	MSG_WriteLong (&host_client->message, pr_global_struct->total_monsters);

	MSG_WriteByte (&host_client->message, svc_updatestat);
	MSG_WriteByte (&host_client->message, STAT_SECRETS);
	MSG_WriteLong (&host_client->message, pr_global_struct->found_secrets);

	MSG_WriteByte (&host_client->message, svc_updatestat);
	MSG_WriteByte (&host_client->message, STAT_MONSTERS);
	MSG_WriteLong (&host_client->message, pr_global_struct->killed_monsters);

//
// send a fixangle
// Never send a roll angle, because savegames can catch the server
// in a state where it is expecting the client to correct the angle
// and it won't happen if the game was just loaded, so you wind up
// with a permanent head tilt
	ent = EDICT_NUM( 1 + (host_client - svs.clients) );
	MSG_WriteByte (&host_client->message, svc_setangle);

	if (sv.loadgame) // MH load game angles fix ...
	{
		MSG_WriteAngle (&host_client->message, ent->v.v_angle[0]);
		MSG_WriteAngle (&host_client->message, ent->v.v_angle[1]);
		MSG_WriteAngle (&host_client->message, 0 );
	}
	else
	{
		MSG_WriteAngle (&host_client->message, ent->v.angles[0] );
		MSG_WriteAngle (&host_client->message, ent->v.angles[1] );
		MSG_WriteAngle (&host_client->message, 0 );
	}


	SV_WriteClientdataToMessage (sv_player, &host_client->message);

	MSG_WriteByte (&host_client->message, svc_signonnum);
	MSG_WriteByte (&host_client->message, 3);
	host_client->sendsignon = true;
}

/*
==================
Host_Begin_f
==================
*/
void Host_Begin_f (void)
{
	if (cmd_source == src_command)
	{
		Con_PrintLinef ("begin is not valid from the console");
		return;
	}

	host_client->spawned = true;
}

//===========================================================================


/*
==================
Host_Kick_f

Kicks a user off of the server
==================
*/
void Host_Kick_f (lparse_t *line)
{
	const char		*who;
	const char		*message = NULL;
	client_t	*save;
	int			i;
	cbool	byNumber = false;

	if (cmd_source == src_command)
	{
		if (!sv.active)
		{
			Cmd_ForwardToServer (line);
			return;
		}
	}
	else if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	save = host_client;

	if (line->count > 2 && strcmp(line->args[1], "#") == 0)
	{
		i = atof(line->args[2]) - 1;
		if (i < 0 || i >= svs.maxclients_internal) // Because the cap can now change in-game
			return;
		if (!svs.clients[i].active)
			return;
		host_client = &svs.clients[i];
		byNumber = true;
	}
	else
	{
		for (i = 0, host_client = svs.clients; i < svs.maxclients_internal; i++, host_client++) // Because the cap can now change in-game
		{
			if (!host_client->active)
				continue;
			if (strcasecmp(host_client->name, line->args[1]) == 0)
				break;
		}
	}

	if (i < svs.maxclients_internal) // Because the cap can now change in-game
	{
		if (cmd_source == src_command)
			if (isDedicated)
				who = "Console";
			else
				who = cl_name.string;
		else
			who = save->name;

		// can't kick yourself!
		if (host_client == save)
			return;

		if (line->count > 2)
		{
			size_t offsetz = line->args[1] - line->chopped; // Offset into line after whitespace
			char *args_after_command = &line->original[offsetz];

			message = COM_Parse(args_after_command);
			if (byNumber)
			{
				message++;							// skip the #
				while (*message == ' ')				// skip white space
					message++;
				message += strlen(line->args[2]);	// skip the number
			}
			while (*message && *message == ' ')
				message++;
		}
		if (message)
			SV_ClientPrintLinef ("Kicked by %s: %s", who, message);
		else
			SV_ClientPrintLinef ("Kicked by %s", who);
		SV_DropClient (false);
	}

	host_client = save;
}

/*
===============================================================================

DEBUGGING TOOLS

===============================================================================
*/

/*
==================
Host_Give_f
==================
*/


void Host_Give_f (lparse_t *line)
{
	char		tbuf[256];
	const char	*t = tbuf;
	int			v;
	eval_t		*val;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer (line);
		return;
	}

	if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	// This allows sv_cheats -1 to disallow major cheats but still allow give
	// for cooperative play where a coop map might have too little ammo or health
	if (sv.disallow_minor_cheats && !host_client->privileged)
	{
		SV_ClientPrintLinef ("No cheats allowed, use sv_cheats 1 and restart level to enable.");
		return;
	}

	if (line->count == 1)
	{
		// Help
		Con_PrintLinef ("usage: give <item> <quantity>");
		Con_PrintLinef (" 1-8 = weapon, a = armor");
		Con_PrintLinef (" h = health, silverkey, goldkey");
		Con_PrintLinef (" s,n,r,c = ammo, rune1-rune4");
		Con_PrintLinef (" rune/key toggles if held");
		return;
	}

	c_strlcpy (tbuf, line->args[1]);
	v = atoi (line->args[2]);

	     if (!strcmp(tbuf, "goldkey")) c_strlcpy (tbuf, "kg");
	else if (!strcmp(tbuf, "silverkey")) c_strlcpy (tbuf, "ks");
	else if (!strcmp(tbuf, "rune1")) c_strlcpy (tbuf, "q1");
	else if (!strcmp(tbuf, "rune2")) c_strlcpy (tbuf, "q2");
	else if (!strcmp(tbuf, "rune3")) c_strlcpy (tbuf, "q3");
	else if (!strcmp(tbuf, "rune4")) c_strlcpy (tbuf, "q4");



	switch (t[0])
	{
#if 1
	case 'k':
		// Baker: Give key will remove key if you have it, add key if you don't
		// Helps debug maps where you need to test trigger.
		if (t[1] && t[1] == 'g')
			sv_player->v.items = (int)sv_player->v.items ^ IT_KEY2;
		else sv_player->v.items = (int)sv_player->v.items ^ IT_KEY1;
		break;

	case 'q':
		{
			int sigil;
			switch (t[1])
			{
			case '1': sigil = 1;	break;
			case '2': sigil = 2;	break;
			case '3': sigil = 4;	break;
			case '4': sigil = 8;	break;
			default:  sigil = 0;	break;
			}
			if (sigil)
			{
				SV_ClientPrintLinef ("may require 'changelevel start' or equivalent for intended effect.");
				pr_global_struct->serverflags = (int)pr_global_struct->serverflags ^ sigil;
			}
			break;
		}
#endif
   case '0':
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
   case '9':
      // MED 01/04/97 added hipnotic give stuff
      if (com_gametype == gametype_hipnotic || com_gametype == gametype_quoth)
      {
         if (t[0] == '6')
         {
            if (t[1] == 'a')
               sv_player->v.items = (int)sv_player->v.items | HIT_PROXIMITY_GUN;
            else
               sv_player->v.items = (int)sv_player->v.items | IT_GRENADE_LAUNCHER;
         }
         else if (t[0] == '9')
            sv_player->v.items = (int)sv_player->v.items | HIT_LASER_CANNON;
         else if (t[0] == '0')
            sv_player->v.items = (int)sv_player->v.items | HIT_MJOLNIR;
         else if (t[0] >= '2')
            sv_player->v.items = (int)sv_player->v.items | (IT_SHOTGUN << (t[0] - '2'));
      }
      else
      {
         if (t[0] >= '2')
            sv_player->v.items = (int)sv_player->v.items | (IT_SHOTGUN << (t[0] - '2'));
      }
		break;

    case 's':
		if (com_gametype == gametype_rogue)
		{
			val = GETEDICTFIELDVALUE(sv_player, eval_ammo_shells1);
		    if (val)
			    val->_float = v;
		}
        sv_player->v.ammo_shells = v;
        break;

    case 'n':
		if (com_gametype == gametype_rogue)
		{
			val = GETEDICTFIELDVALUE(sv_player, eval_ammo_nails1);
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon <= IT_LIGHTNING)
					sv_player->v.ammo_nails = v;
			}
		}
		else
		{
			sv_player->v.ammo_nails = v;
		}
        break;

    case 'l':
		if (com_gametype == gametype_rogue)
		{
			val = GETEDICTFIELDVALUE(sv_player, eval_ammo_lava_nails);
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon > IT_LIGHTNING)
					sv_player->v.ammo_nails = v;
			}
		}
        break;

    case 'r':
		if (com_gametype == gametype_rogue)
		{
			val = GETEDICTFIELDVALUE(sv_player, eval_ammo_rockets1);
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon <= IT_LIGHTNING)
					sv_player->v.ammo_rockets = v;
			}
		}
		else
		{
			sv_player->v.ammo_rockets = v;
		}
        break;

    case 'm':
		if (com_gametype == gametype_rogue)
		{
			val = GETEDICTFIELDVALUE(sv_player, eval_ammo_multi_rockets);
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon > IT_LIGHTNING)
					sv_player->v.ammo_rockets = v;
			}
		}
        break;

    case 'h':
        sv_player->v.health = v;
        break;

    case 'c':
		if (com_gametype == gametype_rogue)
		{
			val = GETEDICTFIELDVALUE (sv_player, eval_ammo_cells1);
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon <= IT_LIGHTNING)
					sv_player->v.ammo_cells = v;
			}
		}
		else
		{
			sv_player->v.ammo_cells = v;
		}
        break;

    case 'p':
		if (com_gametype == gametype_rogue)
		{
			val = GETEDICTFIELDVALUE (sv_player, eval_ammo_plasma);
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon > IT_LIGHTNING)
					sv_player->v.ammo_cells = v;
			}
		}
        break;

	//johnfitz -- give armour
    case 'a':
		if (v > 150)
		{
		    sv_player->v.armortype = 0.8;
	        sv_player->v.armorvalue = v;
			sv_player->v.items = sv_player->v.items -
				((int)(sv_player->v.items) & (int)(IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3)) +
				IT_ARMOR3;
		}
		else if (v > 100)
		{
		    sv_player->v.armortype = 0.6;
	        sv_player->v.armorvalue = v;
			sv_player->v.items = sv_player->v.items -
				((int)(sv_player->v.items) & (int)(IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3)) +
					IT_ARMOR2;
		}
		else if (v >= 0)
		{
		    sv_player->v.armortype = 0.3;
	        sv_player->v.armorvalue = v;
			sv_player->v.items = sv_player->v.items -
				 ((int)(sv_player->v.items) & (int)(IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3)) +
				IT_ARMOR1;
		}
		break;
	//johnfitz
    }

	//johnfitz -- update currentammo to match new ammo (so statusbar updates correctly)
	switch ((int)(sv_player->v.weapon))
	{
	case IT_SHOTGUN:
	case IT_SUPER_SHOTGUN:
		sv_player->v.currentammo = sv_player->v.ammo_shells;
		break;
	case IT_NAILGUN:
	case IT_SUPER_NAILGUN:
	case RIT_LAVA_SUPER_NAILGUN:
		sv_player->v.currentammo = sv_player->v.ammo_nails;
		break;
	case IT_GRENADE_LAUNCHER:
	case IT_ROCKET_LAUNCHER:
	case RIT_MULTI_GRENADE:
	case RIT_MULTI_ROCKET:
		sv_player->v.currentammo = sv_player->v.ammo_rockets;
		break;
	case IT_LIGHTNING:
	case HIT_LASER_CANNON:
	case HIT_MJOLNIR:
		sv_player->v.currentammo = sv_player->v.ammo_cells;
		break;
	case RIT_LAVA_NAILGUN: //same as IT_AXE
		if (com_gametype == gametype_rogue)
			sv_player->v.currentammo = sv_player->v.ammo_nails;
		break;
	case RIT_PLASMA_GUN: //same as HIT_PROXIMITY_GUN
		if (com_gametype == gametype_rogue)
			sv_player->v.currentammo = sv_player->v.ammo_cells;
		if (com_gametype == gametype_hipnotic || com_gametype == gametype_quoth)
			sv_player->v.currentammo = sv_player->v.ammo_rockets;
		break;
	}
	//johnfitz
}



/*
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


/*
==================
Host_Startdemos_f
==================
*/
void Host_Startdemos_f (lparse_t *line)
{
	int		i, c;

	if (isDedicated)
	{
#pragma message ("Baker: Eliminate this somehow or automatically add 'map start' to dedicated server server BEFORE stuffcmds?")
#pragma message ("If I start a dedicated server with +map dm6, what stops this from happening?  The sv.active?")
		if (!sv.active)
			Cbuf_AddTextLine ("map start");
		return;
	}

	if (!host_startdemos.value)
		return;

	// Baker: Old behavior restored.
	if (cls.demonum == -1)
		return;

	c = line->count - 1;
	if (c > MAX_DEMOS_32)
	{
		Con_PrintLinef ("Max %d demos in demoloop", MAX_DEMOS_32);
		c = MAX_DEMOS_32;
	}

	if (line->count != 1)
	{
		cls.demonum = 0;

	}
	Con_PrintLinef ("%d demo(s) in loop", c);
#if 1
	// Warpspasms hack
	if (String_Does_Match_Caseless(gamedir_shortname(), "warpspasm") || String_Does_Match_Caseless(gamedir_shortname(), "warp")) {
		if (line->count > 1) {
			c_strlcpy (cls.demos[0], "demo1");
			c_strlcpy (cls.demos[1], "demo2");
			c_strlcpy (cls.demos[2], "demo3");
			i = 4;
			goto warpspasm_skip;
		}
	}
#endif

	for (i = 1; i < c + 1; i++)
		c_strlcpy (cls.demos[i - 1], line->args[i]);


warpspasm_skip:

	// LordHavoc: clear the remaining slots
	for ( ; i <= MAX_DEMOS_32; i++)
		cls.demos[i - 1][0] = 0;
/*
	if (line->count == 0)
	{
		for (i = 1;i <= MAX_DEMOS_32;i++)
		cls.demos[i-1][0] = 0;
		CL_Clear_Demos_Queue ();
		cls.demonum = -1;
		CL_Disconnect ();
		CL_NextDemo (); // Baker attempt
		CL_Disconnect ();


	}
*/
	if (!sv.active && cls.demonum != -1 && !cls.demoplayback && cls.state != ca_connected)
	{
		cls.demonum = 0;
		CL_NextDemo ();
	}
	else
	{
		cls.demonum = -1;
	}
}



/*
==================
Host_Stopdemo_f

Return to looping demos
==================
*/
void Host_Stopdemo_f (lparse_t *unused)
{
	if (isDedicated)
		return;

	if (!cls.demoplayback)
		return;
	CL_StopPlayback ();

// Baker :Since this is an intentional user action,
// clear the demos queue.
	CL_Clear_Demos_Queue ();

	CL_Disconnect ();
}

//=============================================================================
