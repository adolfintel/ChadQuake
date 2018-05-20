/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2009-2010 Ozkan Sezer
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

/*
	net.h
	quake's interface to the networking layer
	network functions and data, common to the
	whole engine
*/

#ifndef __NET_H__
#define __NET_H__


#define	NET_NAMELEN_64			64

//#define NET_MAXMESSAGE		32000	/* johnfitz -- was 8192 */

extern int			DEFAULTnet_hostport;
extern int			net_hostport;

extern	double		net_time;
extern	sizebuf_t	net_message;
extern	int			net_activeconnections;


void		NET_Init (void);
void		NET_Shutdown (void);

struct qsocket_s	*NET_CheckNewConnections (void);
// returns a new connection number if there is one pending, else -1

struct qsocket_s	*NET_Connect (const char *host);
// called by client to connect to a host.  Returns -1 if not able to

double NET_QSocketGetTime (const struct qsocket_s *sock);
const char *NET_QSocketGetTrueAddressString (const struct qsocket_s *sock);
const char *NET_QSocketGetMaskedAddressString (const struct qsocket_s *sock);

#if 1
int NET_QSocketIsProQuakeServer (const struct qsocket_s *sock);
#endif


cbool NET_CanSendMessage (struct qsocket_s *sock);
// Returns true or false if the given qsocket can currently accept a
// message to be transmitted.

struct qsocket_s *NET_GetServerMessage(void);
//returns data in net_message, qsocket says which client its from

int			NET_GetMessage (struct qsocket_s *sock);
// returns data in net_message sizebuf
// returns 0 if no data is waiting
// returns 1 if a message was received
// returns 2 if an unreliable message was received
// returns -1 if the connection died

int			NET_SendMessage (struct qsocket_s *sock, sizebuf_t *data);
int			NET_SendUnreliableMessage (struct qsocket_s *sock, sizebuf_t *data);
// returns 0 if the message connot be delivered reliably, but the connection
//		is still considered valid
// returns 1 if the message was sent properly
// returns -1 if the connection died

int	NET_SendToAll(sizebuf_t *data, double blocktime);
// This is a reliable *blocking* send to all attached clients.

void		NET_Close (struct qsocket_s *sock);
// if a dead connection is returned by a get or send function, this function
// should be called when it is convenient

// Server calls when a client is kicked off for a game related misbehavior
// like an illegal protocal conversation.  Client calls when disconnecting
// from a server.
// A netcon_t number will not be reused until this function is called for it

void NET_Poll(void);


// Server list related globals:
extern	cbool	slistInProgress;
extern	cbool	slistSilent;
extern	enum slistScope_e
{
	SLIST_LOOP,
	SLIST_LAN,
	SLIST_INTERNET
} slistScope;

extern	int		hostCacheCount; // 1.4 // dupped in net_defs.h needed for menu.c here

void NET_Slist_f (lparse_t *unused);
void NET_SlistSort (void);
const char *NET_SlistPrintServer (int idx); // 1.5
const char *NET_SlistPrintServerName (int idx); // 1.6


/* FIXME: driver related, but public:
 */

extern	cbool	ipv4Available;  // 1.7
extern	cbool	ipv6Available; // 1.8

extern	char		my_ipv4_address[NET_NAMELEN_64];			// 1.9				
extern	char		my_ipv4_server_address[NET_NAMELEN_64];		// Baker:  Keep separate from netcontrolsocket
extern	char		my_ipv6_address[NET_NAMELEN_64];			// 1.10


// RCON
extern	char		server_name[MAX_QPATH_64];
extern sizebuf_t	rcon_message;
extern cbool		rcon_active;


#endif // ! __NET_H__

