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
// net_dgrn.h

#ifndef __NET_DGRM_H__
#define __NET_DGRM_H__


int			Datagram_Init (void);
void		Datagram_Listen (cbool state);
cbool		Datagram_SearchForHosts (cbool xmit);
qsocket_t	*Datagram_Connect (const char *host);
qsocket_t 	*Datagram_CheckNewConnections (void);
qsocket_t	*Datagram_GetAnyMessage (void);
int			Datagram_GetMessage (qsocket_t *sock);
int			Datagram_SendMessage (qsocket_t *sock, sizebuf_t *data);
int			Datagram_SendUnreliableMessage (qsocket_t *sock, sizebuf_t *data);
cbool		Datagram_CanSendMessage (qsocket_t *sock);
cbool		Datagram_CanSendUnreliableMessage (qsocket_t *sock);
void		Datagram_Close (qsocket_t *sock);
void		Datagram_Shutdown (void);

#endif // ! __NET_DGRM_H__

