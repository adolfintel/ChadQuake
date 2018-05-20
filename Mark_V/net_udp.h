/*
Copyright (C) 1996-1997 Id Software, Inc.
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
// net_udp.h

#ifndef __NET_UDP_H__
#define __NET_UDP_H__

int  UDP_CloseSocket (sys_socket_t socketid);
int  UDP_Connect (sys_socket_t socketid, struct qsockaddr *addr);
int  UDP_Read (sys_socket_t socketid, byte *buf, int len, struct qsockaddr *addr);
int  UDP_Write (sys_socket_t socketid, byte *buf, int len, struct qsockaddr *addr);
int  UDP_GetSocketAddr (sys_socket_t socketid, struct qsockaddr *addr);
const char *UDP_AddrToString (struct qsockaddr *addr, cbool masked);
int  UDP_GetNameFromAddr (struct qsockaddr *addr, char *name, size_t len);
int  UDP_AddrCompare (struct qsockaddr *addr1, struct qsockaddr *addr2);
int  UDP_GetSocketPort (struct qsockaddr *addr);
int  UDP_SetSocketPort (struct qsockaddr *addr, int port);


sys_socket_t  UDP4_Init (void);
void UDP4_Shutdown (void);
sys_socket_t UDP4_Listen (cbool state);
sys_socket_t  UDP4_OpenSocket (int port);
sys_socket_t  UDP4_CheckNewConnections (void);
int  UDP4_Broadcast (sys_socket_t socketid, byte *buf, int len);
int  UDP4_StringToAddr (const char *string, struct qsockaddr *addr);
int  UDP4_GetAddrFromName (const char *name, struct qsockaddr *addr);

sys_socket_t  UDP6_Init (void);
void UDP6_Shutdown (void);
sys_socket_t UDP6_Listen (cbool state);
sys_socket_t  UDP6_OpenSocket (int port);
sys_socket_t  UDP6_CheckNewConnections (void);
int  UDP6_Broadcast (sys_socket_t socketid, byte *buf, int len);
int  UDP6_StringToAddr (const char *string, struct qsockaddr *addr);
int  UDP6_GetAddrFromName (const char *name, struct qsockaddr *addr);

#endif // ! __NET_UDP_H__

