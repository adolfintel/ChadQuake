/*
 Copyright (C) 1996-2001 Id Software, Inc.
 Copyright (C) 2002-2005 John Fitzgibbons and others
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

// net_admin.c

// Baker: I'm not fond of the network includes being in here, but I'll live with it for now.

#include <core.h>
#include "q_stdinc.h"
#include "arch_def.h"
#include "net_sys.h"
#include "quakedef.h"
#include "net_defs.h"

#ifdef CORE_PTHREADS


void Admin_Mute_f (lparse_t *line) {}
void Admin_Ban_Ip_f (lparse_t *line) {}

///////////////////////////////////////////////////////////////////////////////
//  BANFILE COMMANDS
///////////////////////////////////////////////////////////////////////////////

void Admin_Ban_List_f (lparse_t *line)
{
	clist_t *cur = svs.ban_listing.list;
	int count;

	Con_SafePrintLinef ("Ban List");
	for ( count = 0; cur; cur = cur->next, count ++)
	{
		Con_PrintLinef ("%04d: %s", count + 1, cur->name);
	}
}

void Admin_White_List_f (lparse_t *line)
{
	clist_t *cur = svs.white_listing.list;
	int count;

	Con_SafePrintLinef ("White List");
	for ( count = 0; cur; cur = cur->next, count ++)
	{
		Con_PrintLinef ("%04d: %s", count + 1, cur->name);
	}
}

///////////////////////////////////////////////////////////////////////////////
//  BANFILE THINKING
///////////////////////////////////////////////////////////////////////////////

void Admin_Banlist_URL_Changed_f (cvar_t *var)
{
	if (!var->string[0])
		return; // Empty string
}

void Admin_Whitelist_URL_Changed_f (cvar_t *var)
{
	if (!var->string[0])
		return; // Empty string
}


///////////////////////////////////////////////////////////////////////////////
//  SERVER LOCK: Ability to stop new connections for a few minutes
//  Commands at moment probably are lockserver and unlockserver
//  Has a timeout to ensure can't accidentally (or maliciously) lock server forever.
///////////////////////////////////////////////////////////////////////////////

#define LOCK_AMOUNT_TIME 300 // 5 minutes

void Admin_UnLock_f (lparse_t *line)
{
	svs.lock_until_time = 0;
	Con_PrintLinef ("Server is unlocked, new connections may join.");
}

void Admin_Lock_f (lparse_t *line)
{

	if (!svs.listening)
	{
		Con_PrintLinef ("Server isn't listening.  Set maxplayers.");
		return;
	}

	svs.lock_until_time = realtime + LOCK_AMOUNT_TIME;
	Con_PrintLinef ("Server locked for a few minutes.");
}


///////////////////////////////////////////////////////////////////////////////
//  CONNECTION CHECKS
///////////////////////////////////////////////////////////////////////////////

cbool Admin_Check_ServerLock (const char *unvalidated_ipv4_string)
{
	// This would be a good time to "think"
	if (!svs.lock_until_time)
		return false; // Server not locked

	if (realtime > svs.lock_until_time)
	{
		svs.lock_until_time = 0; // Unlock
		Con_PrintLinef ("Server has unlocked");
		return false;

	}

	Con_PrintLinef ("Locked server has prevent %s from connecting.", unvalidated_ipv4_string);
	return false;
}

cbool Admin_Check_Whitelist (const char *unvalidated_ipv4_string)
{
	char ip_prepped[16];

	if (!svs.white_listing.list)
		return false; // Everyone may enter.

	// svs.whitelist_think = true; // Signal to recheck list

	if (!IPv4_String_Validated (ip_prepped, sizeof(ip_prepped), unvalidated_ipv4_string))
		return false; // Must be special ip let it in.

	if (IPv4_List_Find (svs.white_listing.list, ip_prepped))
		return false; // On list, you are ok.

	Con_PrintLinef ("Unwhitelisted player %s tried to connect", unvalidated_ipv4_string);
	return true;
}

cbool Admin_Check_Ban (const char *unvalidated_ipv4_string)
{
	char ip_prepped[16];

	// svs.banlist_think = true;  // Signal to recheck list
	if (!svs.ban_listing.list)
		return false; // Everyone may enter.

	if (!IPv4_String_Validated (ip_prepped, sizeof(ip_prepped), unvalidated_ipv4_string))
		return false;  // Must be special ip let it in.

	if (!IPv4_List_Find (svs.ban_listing.list, ip_prepped))
	{
		return false; // Not on ban file, you are ok.
	}

	Con_PrintLinef ("Banned player %s tried to connect", unvalidated_ipv4_string);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//  Init
///////////////////////////////////////////////////////////////////////////////
#include "download_procs.h"
void Q_Thread_Event_Dispatch (int event, int code, void *id, void *data)
{


	// We only run in the main thread.  We could use con_printf.  But we use queue_printf so messages print sequentially.
	Con_Queue_PrintLinef ("Event dispatching event:%d code:%d id:%p data:%p ", event, code, id, data);

	switch (event)
	{
	case EVENT_DOWNLOAD_COMPLETE: // Check the code for whitelist vs banlist
		{
			download_x_t *d = (download_x_t *)data;
			remotelist_t *listz;

			switch (code)
			{
			case EVENT_DOWNLOAD_LIST:
				listz = (remotelist_t *)id; // We pass the id
				listz->download_data  = d->mem.memory;
				listz->download_size  = d->mem.size;
				listz->download_complete = true;

				if (listz->download_data)
					Con_DPrintLinef ("Data is empty!  %d %s", d->exit_code, KeyValue_GetKeyString (downloader_error_strings, d->exit_code));
				break;

			default:
				alert ("Invalid code for download event");
				break;
			}

			d = core_free(d); // Release the download memory.
		}
		break;

	default:
		alert1 ("Unknown event number");
	}
	// End of function after event switch
}


#define DOWNLOAD_CHECK_INTERVAL_300 15
void Admin_Remote_Update (void)
{
	// Check messages here?  Would be nice wouldn't it.
	int i;
	remotelist_t *tt[] = { &svs.white_listing, &svs.ban_listing, NULL };

	// NET_CheckNewConnections 	remotelist_t	white_listing; server_static_t
	if (!svs.remote) // Not running remote
		return;

	Q_Thread_Events_Run (); // Probably will move

	for (i = 0; tt[i]; i ++)
	{
		remotelist_t *cur = tt[i];

		if (cur->download_complete)
		{
			if (cur->download_data) // And there might not be because it could have failed
			{
				// Do we accept the list?
				do
				{
					clist_t *test = IPv4_List_From_String (cur->download_data);
					int newcount = List_Count(test);
					cbool changed;

					if (!newcount)
						break; // Reject 0 size list

					// Determine if first
					if (!cur->list)
						Con_PrintLinef ("List initial set");

					// Determine if change
					changed = !cur->list || cur->listcount != newcount || List_Compare (test, cur->list) !=0;

					if (changed)
					{
						// This is where we set the list.
						if (cur->list) alert ("A banlist/whitelist changed and wasn't the first startup");
						// If banlist changed, check everyone.
						// If whitelist changed, ??  Do we still do it?  I think no.
						List_Free (&cur->list); // Discard old list


						cur->list = test;
						cur->listcount = newcount;

					} else List_Free (&test); // Discard new list

					// Now just seeing if anything changed.


				} while (0);
				// List Count.  List compare?
				cur->download_data = core_free (cur->download_data);
				cur->download_size = 0;
				Con_Queue_PrintLinef ("Updated list %s with count %d", cur->remote_url, List_Count (cur->list));
			}
			if (!cur->listcount)
				System_Error ("Remote host with empty banlist or whitelist is not allowed.  Add a fake real valid ip if needed. %s", cur->remote_url);

			cur->download_complete = false;
			cur->nextchecktime = realtime + DOWNLOAD_CHECK_INTERVAL_300;
			cur->thread_running = false;
		}
		else if (realtime > cur->nextchecktime && !cur->thread_running)
		{
			cur->thread_running = true;                       // cancel, finish_fn, code, id
			Download_Http_Async_To_Memory ("UA", cur->remote_url, NULL /*no canceller*/, Q_Thread_Event_Add, EVENT_DOWNLOAD_LIST, cur /*ok*/);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//  Init
///////////////////////////////////////////////////////////////////////////////

// Ensures thread and main thread can't stomp.  Probably pretty damn unlikely anyway.

void Set_List_Download_Status (pthread_mutex_t *plock, int *var_int, int newstatus)
{
	pthread_mutex_lock (plock);
	*var_int = newstatus;
	pthread_mutex_unlock (plock);
}


#define	BANLIST				"banlist"		// banfile.  Lives in id1.
#define	WHITELIST			"whitelist"		// banfile.  Lives in id1.


void Admin_Init (void)
{
	void Install_Init (void);
//	return;
	Install_Init ();
//	exit (1);
	svs.remote = COM_CheckParm ("-remote"); // Should this be a command line param?

	if (svs.remote)
	{
		// server_static_t  remotelist_t ReadList_NewGame Install_Command_f
		if (svs.remote && !(svs.remote + 1 < com_argc) )
			System_Error ("Remote requires extra parameter"); // http://quakeone.com/something

		c_strlcpy (svs.remote_host, com_argv[svs.remote + 1 ] ); // Copy next parameter in
		File_URL_Edit_Remove_Any_Trailing_Slash (svs.remote_host);

		c_snprintf2 (svs.ban_listing.remote_url, "%s/%s.txt",  svs.remote_host, BANLIST); // Multiple servers could stomp each other?
		c_snprintf2 (svs.white_listing.remote_url, "%s/%s.txt",  svs.remote_host, WHITELIST); // Multiple servers could stomp each other?
		Con_SafePrintLinef ("Remote control activated " QUOTED_S ":banlist: %s" NEWLINE "whitelist: %s", svs.remote_host, svs.ban_listing.remote_url, svs.white_listing.remote_url);
		return;
	}

	// We still read the banfile from file right?
	c_snprintf3 (svs.ban_listing.local_url, "%s/%s/%s.txt",  com_basedir, GAMENAME_ID1, BANLIST); // Multiple servers could stomp each other?
	svs.ban_listing.list = IPv4_List_From_File (svs.ban_listing.local_url);

	//Con_SafePrintLinef ("Admin Init");
}


///////////////////////////////////////////////////////////////////////////////
//  File Transfer
///////////////////////////////////////////////////////////////////////////////

#include "core_net_sys.h"
#include "net_simple.h"


// Update this anytime someone connects or disconnects.
clist_t *player_list_thread_safe;
pthread_mutex_t player_list_lock = PTHREAD_MUTEX_INITIALIZER;


// The writing of the list is only done in the main thread.
void Player_IPv4_List_Update (void)
{
	client_t	*client;
	int n;
#if 0
	Con_PrintLinef ("Updated player list");
#endif // 0

	pthread_mutex_lock (&player_list_lock);

	if (player_list_thread_safe)
		List_Free (&player_list_thread_safe);

	for (n = 0, client = svs.clients ; n < svs.maxclients_internal ; n++, client++)
	{
		const char *unvalidated_ipstring = NET_QSocketGetTrueAddressString(client->netconnection);
		char prepared_ip_string[16];

		if (!client->active)
			continue;

#if 0
		Con_PrintLinef ("Found an active one %s", NET_QSocketGetTrueAddressString(client->netconnection));
#endif // 0

		if (!IPv4_String_Validated (prepared_ip_string, sizeof(prepared_ip_string), unvalidated_ipstring))
			continue; // Something like "local"
		Con_PrintLinef ("%s", prepared_ip_string);
		List_Add (&player_list_thread_safe, prepared_ip_string);
	}

	pthread_mutex_unlock (&player_list_lock);
}

cbool Player_IP_List_Find (const char *unvalidated_ipstring)
{
	cbool result;
	pthread_mutex_lock (&player_list_lock);
	result = IPv4_List_Find (player_list_thread_safe, unvalidated_ipstring);
	pthread_mutex_unlock (&player_list_lock);
	return result;
}

void GetFile_Command_f (void *p)
{
	Net_Simple_Client ("192.168.1.8", 26010, "c:/", (errorline_fn_t)Con_Queue_Printf, Con_Queue_Printf, Con_Queue_Printf);

}

volatile sys_socket_t fileserver_notify_socket = INVALID_SOCKET;
void ServeFile_Shutdown_Command_f (lparse_t *unused)
{
	if (fileserver_notify_socket == INVALID_SOCKET)
	{
		Con_PrintLinef ("Server not running");
		return;
	}

	Net_Simple_Server_Force_Shutdown (fileserver_notify_socket);
}

void ServeFile_Command_f (lparse_t *unused)
{
	int baseport = net_hostport ? net_hostport : DEFAULTnet_hostport /*26000*/;
	int port = sv_fileserver_port.string[0] == '+' ? baseport + atoi (&sv_fileserver_port.string[0]) : (int)sv_fileserver_port.value;

	if (fileserver_notify_socket != INVALID_SOCKET)
	{
		Con_PrintLinef ("Server already active");
		return;
	}

	if (!Net_Simple_Server_Async (NULL, port, "c:/",
		(errorline_fn_t)Con_Queue_Printf, Con_Queue_Printf, Con_Queue_Printf, Player_IP_List_Find, &fileserver_notify_socket))
	{
		Con_PrintLinef ("Server failed to start");
		return;
	}

	// fileserver_notify_socket should be set at some point.
	Con_PrintLinef ("Server started ok");
}

clist_t *cl_gamefiles_list;
clist_t *gamefiles_list_thread_safe;
pthread_mutex_t gamefiles_list_lock = PTHREAD_MUTEX_INITIALIZER;

// Provide our local list, output a paklike list.
clist_t *Create_Source_List_Alloc (const clist_t *list)
{
	// STAGE 2:  Find out where it came from, the size and record entire path

	clist_t *out = NULL;
	int path_strip = strlen (com_basedir) + 1;
	const clist_t *cur;
	int count, h;
	const char *tmp;

	for (cur = list, count = 0; cur; cur = cur->next)
	{
		COM_FindFile (cur->name, &h, NULL, NULL); // returns filesize or -1

//		if (h != -1)
//		{	//name           source     size   offset  inzip
			//"progs/backpack.mdl" "quoth/pak1.pak" size 331328 ispak: 1
			// Get source size?

			tmp = com_filepath[0] ? &com_filepath[path_strip] : com_filepath; // Don't skip null terminator if file wasn't found ;-)
			// com_filesize is -1 is wasn't found
			tmp = va (QUOTED_S " " QUOTED_S " %d %s", cur->name, tmp, com_filesize, com_filesrcpak ? "pak" : "dir");
			List_Add_No_Case_To_Lower (&out, tmp);
			if (com_filepath[0] == 0)
				h=h;
		if (h != -1)
			COM_CloseFile (h);
//		}
//		else Con_SafePrintLinef ("%s wasn't found with COM_FindFile", cur->name); // This could happen for client!
	}

	return out;
}

// List cmp
// Get sv list if we have a

clist_t *Admin_Game_Files_List_Client_Missing (clist_t *cl_list, clist_t *sv_list)
{
	// If enhanced qpath (gamedir inclusive) gamedir matches (arg2) we are ok (even if filesize doesn't match to allow replacement content)
	// Client list needs to indicate missing files (zero length?  Does it do this now?)
	// Build a request list.
	// IF the missing file in a pack, let's just ask for the pak (in fact, technically I think we have to!  An outside pak files isn't equivalent).
	// Weird possible stupid problem:  if pak2 and let's say map doesn't use any of pak1, we still need pak1. Because need sequential paks.
	// So we will need to gamedir change later???? EEK!
	// Because once this is done, we effectively must gamedir change.
	return 0; // I guess?
}


void Admin_Game_Files_List_Update_Client (clist_t *cl_list)
{
	// STAGE 1:  Generate
	if (cl_gamefiles_list)
		List_Free (&cl_gamefiles_list);

#if 0  // This is where we can compare
	List_Print (cl_list);
#endif // 0
	cl_gamefiles_list = Create_Source_List_Alloc (cl_list);
#if 0
	List_Print (cl_gamefiles_list);
#endif // 0

//	alert ( "%d", List_Compare (cl_gamefiles_list, gamefiles_list_thread_safe ) );


}

void Admin_Game_Files_List_Update_Server (void)
{
	// STAGE 1:  Generate
	clist_t *rawlist = NULL;
	const char		**s;
	int i;

	for (i = 0, s = sv.model_precache + 1; *s; s++, i++) // This list is null terminated?
		if (*s && *s[0] != '*') // Don't add map submodels.
			List_Add_No_Case_To_Lower (&rawlist, *s);

	for (i = 0, s = sv.sound_precache + 1; *s; s++, i++)
		if (*s)
			List_Add_No_Case_To_Lower (&rawlist, /* it was soundz ... on purpse?  on accident? */ va("sound/%s", *s) ); // We are in main thread.  And because threads never use va, this is ok.

	pthread_mutex_lock (&gamefiles_list_lock);

	if (gamefiles_list_thread_safe)
		List_Free (&gamefiles_list_thread_safe);

	gamefiles_list_thread_safe = Create_Source_List_Alloc (rawlist);

#if 0
Con_PrintLinef ("Startssz");
	List_Print (gamefiles_list_thread_safe);
Con_PrintLinef ("Stopsz");
#endif // 0

	pthread_mutex_unlock (&gamefiles_list_lock);


	List_Free (&rawlist); // Discard
}

#if 0
void Admin_Game_Files_List_Update_Server (void)
{
	clist_t *list = NULL;

	pthread_mutex_lock (&gamefiles_list_lock);

	if (gamefiles_list_thread_safe)
		List_Free (&gamefiles_list_thread_safe);

	{ // STAGE 1:  Generate
		const char		**s;
		int i;

		for (i = 0, s = sv.model_precache + 1; *s; s++, i++) // This list is null terminated?
			if (*s && *s[0] != '*') // Don't add map submodels.
				List_Add_No_Case_To_Lower (&list, *s);

		s=s;

		for (i = 0, s = sv.sound_precache + 1; *s; s++, i++)
		{
			char buf[MAX_QPATH_64];
			if (*s)
			{
				c_snprintf1 (buf, "sound/%s", *s);
				List_Add_No_Case_To_Lower (&list, buf);
			}
		}
	}

	{ // STAGE 2:  Find out where it came from, the size and record entire path
		char buf[SYSTEM_STRING_SIZE_1024];
		const clist_t *cur;

//name           extended name     size   offset  inzip
//maps/start.bsp travail           272392 373333  yes
//maps/start.bsp travail/pak0.pak
		int path_strip = strlen (com_basedir) + 1;

		//clist_t *list = NULL;
		clist_t *datalist = NULL;
		int count;
		for (cur = list, count = 0; cur; cur = cur->next)
		{
			char source[MAX_OSPATH];

			cbool ispak; // Is the file from a pak.
			size_t filesize;
			int h;
			COM_FindFile (cur->name, &h, NULL, NULL); // returns filesize or -1
			if (h == -1)
			{
			//	Con_SafePrintLinef ("%s wasn't found with COM_FindFile", cur->name);
			}
			else {
			//	Con_SafePrintLinef ("%s X %s|%d|%d", cur->name, &com_filepath[path_strip], com_filesize, com_filesrcpak ? "pak" : "dir");
				c_snprintfc (buf, sizeof(buf), QUOTED_S " " QUOTED_S " %d %d", cur->name, &com_filepath[path_strip], com_filesize, com_filesrcpak ? "pak" : "dir");
				List_Add_No_Case_To_Lower (&datalist, buf);
				COM_CloseFile (h);
			}

//NOW WHAT?
//Pak reader thread safe.
		}

		gamefiles_list_thread_safe = datalist;
	}

	List_Print (gamefiles_list_thread_safe);
	List_Free (&list);



	pthread_mutex_unlock (&gamefiles_list_lock);
}
#endif // 0


char *Admin_Game_Files_Find (const char *fp_basedir)
{
	//
	clist_t *result;
	pthread_mutex_lock (&gamefiles_list_lock);
	result = List_Find_Item (gamefiles_list_thread_safe, fp_basedir);
	pthread_mutex_unlock (&gamefiles_list_lock);
	return result ? result->name : NULL;
}

/*		cbool inzip;
		cbool havezip; // There could be a couple.
		cbool is_vanilla_id1; // id1 without quoth or hipnotic or anything.
		cbool server_has_zips;
		cbool zips_match_quaddicted;
	{ // Work the list
			len = COM_OpenFile (path, &h);
	if (h == -1)
		return NULL;
COM_CloseFile (h);

*/

#endif // CORE_PTHREADS
