/*
Copyright (C) 2014-2014 Baker

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
// core_net.h -- network


#ifndef __CORE_NET_H__
#define __CORE_NET_H__

#include "environment.h"
#include "core.h"
#include "core_net_sys.h"


typedef struct
{
	cbool		connected;
	time_t		start_time;
	
	cbool		active_wsa;			// After WSAStartup, must be WSACleanup
	cbool		active_listensock; 	// After socket, right?
	cbool		active_remotesock;	// After accept

// Input
	short 		nport;
	char 		client_string[256];	// We make this and it isn't required, we do it after accept
	time_t		connect_time;
	
// Connection	
	WSADATA 	wsa_data;				// Created by WSAStartup  (shutdown required)
	SOCKET		listen_sock;			// Created by socket  (shutdown required)
	SOCKADDR_IN saServer;				// Created by bind, has port and other information in it
	char 		hostname[256];			// Created by gethostname
										// listen	
 	struct sockaddr_in client_addr; 	// accept
	int			addr_size;
	SOCKET 		remote_sock;			// accept (shutdown required)
	
	char		recvbuffer[256]; 		// recv
	int			recvcount;
	
	char		sendbuffer[256]; 		// send
	int			sendcount;
} nserver_t;



nserver_t *Server_Instance (short port);
nserver_t *Server_Shutdown (nserver_t *me);
cbool Server_Connected_Frame (nserver_t *me);
cbool Server_Listen (nserver_t *me);
cbool Server_Error (nserver_t *me, const char *msg);
void Server_Close (nserver_t *me);


#endif	// ! __CORE_NET_H__


