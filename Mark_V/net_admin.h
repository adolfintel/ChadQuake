/*
Copyright (C) 2015-2015 Baker and others

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
// net_admin.h

#ifndef __NET_ADMIN_H__
#define __NET_ADMIN_H__

// Server file port will have be +10 or something.
// Test linux single port server
// #pragma message ("Have dedicated server show ip address and port on startup?")
//sv_x_connections_same_ip = 4;
// Local network 10, 169, 192, 127, part of 127

cbool Admin_Check_ServerLock (const char *unvalidated_ipv4_string); // returns true if the server is locked.
cbool Admin_Check_Ban (const char *unvalidated_ipv4_string); // returns true if you are banned.
cbool Admin_Check_Whitelist (const char *unvalidated_ipv4_string); // returns true if there is a whitelist and you aren't on it.

void Admin_Init (void);

void Admin_Banlist_URL_Changed_f (cvar_t *var);
void Admin_Whitelist_URL_Changed_f (cvar_t *var);

void Admin_Remote_Update (void); // Check if anything remotely done has occurred.

void Admin_Game_Files_List_Update_Server (void);
void Admin_Game_Files_List_Update_Client (clist_t *cl_list);


typedef struct
{
	char			remote_url[SYSTEM_STRING_SIZE_1024];
	size_t			remote_url_size;						// Expected size of this file.
	char			local_url[MAX_OSPATH];					// Possibly a remote file

	cbool			thread_running;							// 0, 1, 2.  Don't set this in the thread?
	double			nextchecktime;							// Wonder if the server atomically provides files?
	clist_t			*list;									// The list
	int				listcount;								// For comparison?

	byte			*download_data;
	int				download_size;
	cbool			download_complete;

//	pthread_mutex_t lock;									// Will have to manually init the mutex
} remotelist_t;


// A home for these?
void Player_IPv4_List_Update (void);


#endif // ! __NET_ADMIN_H__

/*
FUNCS TO WRITE:
Transfer data to client.
List of games from server hint?  How to access the hint.
Does client have GAME
List of different files.
Game hint tag
*/

