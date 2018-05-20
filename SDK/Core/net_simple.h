/*
Copyright (C) 2012-2014 Baker

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net_simple.h -- download


#ifndef __NET_SIMPLE_H__
#define __NET_SIMPLE_H__



#define IPV4_STRING_MAX_22 22 // 192.192.192.192:22222 = 15 + 1 + 5 = 21 + null = 22
typedef cbool (*whitelist_fn_t) (const char *);

typedef struct socket_info_s
{
	sys_socket_t		socket;
	int					addrsize;							// Has info crammed in there like ip and port when making a connection.
	struct sockaddr_in	addrinfo;							// Has info crammed in there like ip and port when making a connection.
	char				ipstring[IPV4_STRING_MAX_22];		// Note that this is an ip address.
	int					port;

	char				readtype;
	char				readbuffer[65536];
	int					readlen;

	char				writebuffer[65536];
	int					writelen;
} socket_info_t;

typedef struct server_x_s
{
	// Baggage
	socket_info_t			*socka;
	errorline_fn_t			error_fn;
	printline_fn_t			printline_fn;
	printline_fn_t			dprint_fn;
	whitelist_fn_t			whitelist_fn;
	volatile sys_socket_t	*notify_socket; // Mostly for forcing server thread to shutdown by closing accept socket.

	char					basedir[MAX_OSPATH];
} connection_t;



cbool Net_Simple_Server_Async (const char *_ipstring, int port, const char *basedir, errorline_fn_t errorline_fn, printline_fn_t my_printline, printline_fn_t dprintline_fn, whitelist_fn_t whitelist_fn, volatile sys_socket_t *notify_socket);
cbool Net_Simple_Client (const char *_ipstring, int port, const char *basedir, errorline_fn_t errorline_fn, printline_fn_t my_printline, printline_fn_t dprintline_fn);

void Net_Simple_Server_Force_Shutdown (sys_socket_t notify_socket);

#endif // __NET_SIMPLE_H__




