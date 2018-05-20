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

// net_udp.c

#include <core.h>
#include "q_stdinc.h"
#include "arch_def.h"
#include "net_sys.h"
#include "quakedef.h"
#include "net_defs.h"

#if defined(__GNUC__) // && defined(_WIN32) //&& !defined(PLATFORM_OSX) // Fuck you mingw headers
    #define WIN32_LEAN_AND_MEAN
    #define IDE_COMES_WITH_GOOD_IPV6_HEADERS 0
	#pragma message ("Hello IPV6 hax")
    // Sadly, MinGW version that comes with CodeBlocks the headers are not good
    // I want things to compile nice and easy out of the box, without errands and other foolishness.
#ifndef ULONG
	#define ULONG unsigned int
#endif
#ifndef USHORT
	#define USHORT unsigned short
#endif
#ifndef UCHAR
	#define UCHAR unsigned char
#endif

    typedef struct {
        union {
            struct {
                ULONG Zone : 28;
                ULONG Level : 4;
            };
            ULONG Value;
        };
    } SCOPE_ID, *PSCOPE_ID;

//  // This is what comes with mingw for CodeBlocks 13.12
//	struct sockaddr_in6_mingw {
//	    short   sin6_family;        /* AF_INET6 */
//	    u_short sin6_port;          /* Transport level port number */
//	    u_long  sin6_flowinfo;      /* IPv6 flow information */
//	    struct in6_addr sin6_addr;  /* IPv6 address */
//	    u_long sin6_scope_id;       /* set of interfaces for a scope */
//	};
//


    typedef struct in6_addr_correctly {
        union {
            UCHAR       Byte[16];
            USHORT      Word[8];
        } u;
    } IN6_ADDR_EX; // , *PIN6_ADDR, FAR *LPIN6_ADDR;

    struct sockaddr_in6_EX {
        USHORT sin6_family; // AF_INET6.
        USHORT sin6_port;           // Transport level port number.
        ULONG  sin6_flowinfo;       // IPv6 flow information.
        IN6_ADDR_EX sin6_addr;         // IPv6 address.
        union {
            ULONG sin6_scope_id;     // Set of interfaces for a scope.
            SCOPE_ID sin6_scope_struct;
        };
    }; // SOCKADDR_IN6_LH, *PSOCKADDR_IN6_LH, FAR *LPSOCKADDR_IN6_LH;

    #define sockaddr_in6_HAX sockaddr_in6_EX
#else // !s6_addr

	#define IDE_COMES_WITH_GOOD_IPV6_HEADERS 1
	#define sockaddr_in6_HAX sockaddr_in6 // 'struct in6_addr' has no member named 'u'|
#endif


//ipv4 defs
static sys_socket_t netv4_acceptsocket = INVALID_SOCKET;	// socket for fielding new connections
static sys_socket_t netv4_controlsocket;
static sys_socket_t netv4_broadcastsocket = 0; // Baker: why not INVALID_SOCKET?
static struct sockaddr_in broadcastaddrv4;

static in_addr_t	myAddrv4, bindAddrv4; //spike --keeping separate bind and detected values.

//ipv6 defs

static sys_socket_t netv6_acceptsocket = INVALID_SOCKET;	// socket for fielding new connections
static sys_socket_t netv6_controlsocket;
static struct sockaddr_in6 broadcastaddrv6;
static in_addr6_t	myAddrv6, bindAddrv6;



#include "net_udp.h"

#ifdef _WIN32 // Netdiff
	//#pragma comment (lib, "wsock32.lib")
	#pragma comment (lib, "ws2_32.lib")
	int winsock_initialized = 0;
	#include "wsaerror.h"

#endif

static int UDP_Platform_Startup (void)
{
#ifdef _WIN32
	if (winsock_initialized == 0)
	{
		WSADATA	winsockdata;
		int err = WSAStartup(MAKEWORD(2,2), &winsockdata); // Let's try 2.2
		if (err != 0)
		{
			Con_SafePrintLinef ("Winsock initialization failed (%s)", socketerror(err));
			return 0;
		}
	}
	winsock_initialized++;
#endif

#ifdef PLATFORM_OSX
    // Baker: This tells the sockets to be in non-blocking mode
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
#endif // PLATFORM_OSX
	return 1;
}

static void UDP_Platform_Cleanup (void)
{
#ifdef _WIN32
	if (--winsock_initialized == 0)
		WSACleanup ();
#endif // _WIN32
}

// Baker: The return value for UDP_Init is not used in the current
// source.  Let's return the socket like the source suggests
// even though I believe the return value from the function looks more
// designed for a boolean like return

//=============================================================================

static in_addr_t UDP4_GetHostNameIP (char *namebuf, size_t namebuf_size, char * ipbuf, size_t ipbuf_size)
{
	char buff[MAXHOSTNAMELEN];

    if (gethostname(buff, sizeof(buff)) == SOCKET_ERROR) {
		int err = SOCKETERRNO;
		Con_SafePrintLinef ("UDP4_GetHostName: gethostname failed (%s)", socketerror(err));
		return 0;
	}

	buff[sizeof(buff) - 1] = 0;

	do {
		struct hostent *local = gethostbyname(buff);
		in_addr_t netaddr;
		if (local == NULL) {
			int err = SOCKETERRNO;
			Con_SafePrintLinef ("UDP4_GetHostName: gethostbyname failed for hostname [%s](%s)", buff, socketerror(err)); // This probably never happens.
			return 0;
		}
		else if (local->h_addrtype != AF_INET) {
			Con_SafePrintLinef ("UDP4_GetHostName: address from gethostbyname not IPv4");
			return 0;
		}

		// Success
		netaddr = *(in_addr_t *)local->h_addr_list[0];

		if (namebuf) {
			// Copy out the name buf if we have one
			strlcpy (namebuf, buff, namebuf_size);
		}

		if (ipbuf) {
			// If ip address, fill that in.
	//		alert ("%p: %d", ipbuf, (int)ipbuf_size);
			in_addr_t	haddr = ntohl(netaddr); // Net byte order to host order
			// int is the right tool for this job.
			//c_snprintfc (ipbuf, ipbuf_size,  "%ld.%ld.%ld.%ld", (haddr >> 24) & 0xff, (haddr >> 16) & 0xff, (haddr >> 8) & 0xff, haddr & 0xff);
			c_snprintfc (ipbuf, ipbuf_size,  "%d.%d.%d.%d", (int)((haddr >> 24) & 0xff), (int)((haddr >> 16) & 0xff), (int)((haddr >> 8) & 0xff), (int)(haddr & 0xff));
		}
		return netaddr; // In network form.
	} while (0);
}




static void UDP4_GetLocalAddress (void)
{
	if (myAddrv4 == INADDR_ANY) {
		myAddrv4 = UDP4_GetHostNameIP (NULL, 0, my_ipv4_address, sizeof(my_ipv4_address));
	}
}




static sys_socket_t UDP_CheckNewConnections (sys_socket_t netacceptsock_ipvany)
{
	if (netacceptsock_ipvany == INVALID_SOCKET)
		return INVALID_SOCKET;

	// Baker: Check socket for connection.
	// WinSock vs. BSD have different methods for reading the buffer without
	// removing the read from the buffer (WinSock: recvfrom with  MSG_PEEK vs. BSD where
	// we request ioctl FIONREAD and available returns the count and if the count > 0
	// we have data
	{
#ifdef _WIN32 // Netdiff
		char		buf[4096];
		if (recvfrom (netacceptsock_ipvany, buf, sizeof(buf), MSG_PEEK, NULL, NULL) == SOCKET_ERROR)
			return INVALID_SOCKET;
#else
		int		bytes_available_count;
		if (ioctl (netacceptsock_ipvany, FIONREAD, &bytes_available_count) == -1)
		{
			int err = SOCKETERRNO;
			System_Error ("UDP: ioctlsocket (FIONREAD) failed (%s)", socketerror(err));
		}

		if (!bytes_available_count)
		{
			// no bytes available - quietly absorb empty packets
			struct sockaddr_in	from;
			socklen_t	fromlen;
			char		buff[1];

			recvfrom (netacceptsock_ipvany, buff, 0, 0, (struct sockaddr *) &from, &fromlen);

			return INVALID_SOCKET;
		}
#endif // end netdiff
	}

	return netacceptsock_ipvany;
}

//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
// old: sys_socket_t UDP_Init (void)
sys_socket_t UDP4_Init (void)
{
	char	buff[MAXHOSTNAMELEN];
	int		t;


	if (COM_CheckParm ("-noudp") || COM_CheckParm ("-noudp4"))
		return INVALID_SOCKET;

	if (!UDP_Platform_Startup())
		return INVALID_SOCKET; // UDP_Platform_Startup already printed a fail message

	// determine my name & address
	if (gethostname(buff, MAXHOSTNAMELEN) != 0) {
		int err = SOCKETERRNO;
		Con_SafePrintLinef ("UDP4_Init: gethostname failed (%s)", socketerror(err));
	}
	else
	{
		buff[MAXHOSTNAMELEN - 1] = 0;
	}

	t = COM_CheckParm ("-ip");
	if (t)
	{
		if (t < com_argc-1)
		{
			bindAddrv4 = inet_addr(com_argv[t + 1]);
			if (bindAddrv4 == INADDR_NONE) // Value is -1
				System_Error ("%s is not a valid IPv4 address", com_argv[t + 1]);
			c_strlcpy (my_ipv4_address, com_argv[t + 1]);
			c_strlcpy (my_ipv4_server_address, com_argv[t + 1]);
		}
		else
		{
			System_Error ("UDP4_Init: you must specify an IP address after -ip");
		}
	}
	else
	{
		UDP4_GetHostNameIP (NULL, 0, my_ipv4_server_address, sizeof(my_ipv4_server_address)); // For reference purposes
		bindAddrv4 = INADDR_ANY; // Value is 0.
		c_strlcpy (my_ipv4_address, "INADDR_ANY");
	}

	myAddrv4 = bindAddrv4;

	if ((netv4_controlsocket = UDP4_OpenSocket(0)) == INVALID_SOCKET)
	{
		Con_SafePrintLinef ("UDP4_Init: Unable to open control socket, UDP disabled");
		UDP_Platform_Cleanup ();
		return INVALID_SOCKET;
	}

	broadcastaddrv4.sin_family = AF_INET;
	broadcastaddrv4.sin_addr.s_addr = INADDR_BROADCAST;
	broadcastaddrv4.sin_port = htons((unsigned short)net_hostport);

#if 0 // Baker: old experiment from a couple of years ago
	// This reconstructs the my_tcpip_address out of the socket, validating what we got.
	UDP_GetSocketAddr (netv4_controlsocket, &addr);
	c_strlcpy (my_ipv4_address, UDP_AddrToString (&addr)); // was my_tcpip_address
	colon = strrchr (my_ipv4_address, ':');
	if (colon)
		*colon = 0;
#endif

	Con_SafePrintLinef ("UDP4 Initialized: %s, %s", my_ipv4_address, my_ipv4_server_address);
	ipv4Available = true;

	return netv4_controlsocket;
}

//=============================================================================

void UDP4_Shutdown (void)
{
	UDP4_Listen (false);
	UDP_CloseSocket (netv4_controlsocket);
	UDP_Platform_Cleanup ();
}

//=============================================================================

sys_socket_t UDP4_Listen (cbool state)
{
	// enable listening
	if (state && netv4_acceptsocket == INVALID_SOCKET) {
		UDP4_GetLocalAddress();
 		if ((netv4_acceptsocket = UDP4_OpenSocket (net_hostport)) == INVALID_SOCKET)
			System_Error ("UDP4_Listen: Unable to open accept socket");
	}

	// disable listening
	if (!state && netv4_acceptsocket != INVALID_SOCKET) {
		UDP_CloseSocket (netv4_acceptsocket);
		netv4_acceptsocket = INVALID_SOCKET;
	}
	return netv4_acceptsocket;
}

//=============================================================================

sys_socket_t UDP4_OpenSocket (int port)
{
	sys_socket_t newsocket;
	struct sockaddr_in address;
	ioctl_uint32_t _true = 1;
	int err;

	if ((newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		err = SOCKETERRNO;
		Con_SafePrintLinef ("UDP4_OpenSocket: %s", socketerror(err));
		return INVALID_SOCKET;
	}

	// Set socket to non-blocking.  fnctl is the POSIX way
	if (ioctlsocket (newsocket, FIONBIO, &_true) == SOCKET_ERROR)
		goto ErrorReturn;

	memset(&address, 0, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = bindAddrv4;
	address.sin_port = htons((unsigned short)port);

	if (bind (newsocket, (struct sockaddr *)&address, sizeof(address)) == 0)
	{
		int newport = ((struct sockaddr_in*)&address)->sin_port;
		//alert ("Client port on the server is %d\n", newport);
		//alert (UDP_AddrToString (&address));
		return newsocket;
	}

	if (ipv4Available)
	{
		// Baker: Can this happen if another server is running on 26000?
		// Find out
		err = SOCKETERRNO;
		Host_Error ("Unable to bind to %s (%s)", UDP_AddrToString ((struct qsockaddr *) &address, false), socketerror(err));
		return INVALID_SOCKET;	// unreachable
	}
	/* else: we are still in init phase, no need to error */

ErrorReturn:
	err = SOCKETERRNO;
	Con_SafePrintLinef ("UDP4_OpenSocket: %s", socketerror(err));
	closesocket (newsocket);  // Spike changed to this in R4.  No need to shutdown broadcast on socket that we just opened.
	return INVALID_SOCKET;
}

//=============================================================================

int UDP_CloseSocket (sys_socket_t socketid)
{
	if (socketid == netv4_broadcastsocket)
		netv4_broadcastsocket = 0; // Spike says 0, not INVALID_SOCKET;
	return closesocket (socketid);
}

//=============================================================================

/*
 ============
 PartialIPAddress

 this lets you type only as much of the net address as required, using
 the local network components to fill in the rest
 ============
 */
static int PartialIPAddress (const char *in, struct qsockaddr *hostaddr)
{
	char	buff[256];
	char	*b;
	int	addr, mask, num, port, run;

	buff[0] = '.';
	b = buff;
	strlcpy (buff + 1, in, sizeof(buff)-1);
	if (buff[1] == '.')
		b++;

	addr = 0;
	mask = -1;
	while (*b == '.')
	{
		b++;
		num = 0;
		run = 0;
		while (!( *b < '0' || *b > '9'))
		{
			num = num*10 + *b++ - '0';
			if (++run > 3)
				return -1;
		}
		if ((*b < '0' || *b > '9') && *b != '.' && *b != ':' && *b != 0)
			return -1;
		if (num < 0 || num > 255)
			return -1;
		mask <<= 8;
		addr = (addr<<8) + num;
	}

	if (*b++ == ':')
		port = atoi(b);
	else
		port = net_hostport;

	hostaddr->qsa_family = AF_INET;
	((struct sockaddr_in *)hostaddr)->sin_port = htons((unsigned short) port);
	((struct sockaddr_in *)hostaddr)->sin_addr.s_addr = (myAddrv4 & htonl(mask)) | htonl(addr);

	return 0;
}

//=============================================================================

int UDP_Connect (sys_socket_t socketid, struct qsockaddr *addr)
{
	return 0;
}

//=============================================================================

sys_socket_t UDP4_CheckNewConnections (void)
{
	return UDP_CheckNewConnections (netv4_acceptsocket);
}

//=============================================================================

// dfunc.Read
int UDP_Read (sys_socket_t socketid, byte *buf, int len, struct qsockaddr *addr)
{
	socklen_t addrlen = sizeof(struct qsockaddr);
	int ret;

	// BSD recvfrom arg2 buf is type void *, Windows is type char * hence cast for Windows
	ret = recvfrom (socketid, (char *)buf, len, 0, (struct sockaddr *)addr, &addrlen);

	if (ret == SOCKET_ERROR) {
		// This can happen as standard operating procedure
		int err = SOCKETERRNO;

		if (err == NET_EWOULDBLOCK || err == NET_ECONNREFUSED) {
			//Con_SafePrintLinef ("UDP_Read zero: %s", socketerror(err));
			return 0; // This can happen as standard operating procedure
		}
		if (err ==  NET_ECONNRESET) {
			// Spike says this is exploitable.
			// A fix me would give them a second to get a valid message
			// Before giving them the boot.
			Con_DPrintLinef ("UDP_Read connection reset, recvfrom: %s", socketerror(err));
			return -1; // Get out the boot and kick this connection.
		}
		Con_SafePrintLinef ("UDP_Read, recvfrom: %s", socketerror(err));
		return 0; // I guess we are forgiving and will let it slide this time.
	}
	return ret;
}

//=============================================================================

static int UDP_MakeSocketBroadcastCapable (sys_socket_t socketid)
{
	int	i = 1;

	// make this socket broadcast capable
	if (setsockopt(socketid, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) == SOCKET_ERROR)
	{
		int err = SOCKETERRNO;
		Con_SafePrintLinef ("UDP, setsockopt: %s", socketerror(err));
		return -1;
	}
	netv4_broadcastsocket = socketid;

	return 0;
}

//=============================================================================

int UDP4_Broadcast (sys_socket_t socketid, byte *buf, int len)
{
	int	ret;

	if (socketid != netv4_broadcastsocket)
	{
		if (netv4_broadcastsocket != 0) // formerly INVALID_SOCKET
			System_Error ("Attempted to use multiple broadcasts sockets");
		UDP4_GetLocalAddress();
		ret = UDP_MakeSocketBroadcastCapable (socketid);
		if (ret == -1)
		{
			Con_SafePrintLinef ("Unable to make socket broadcast capable");
			return ret;
		}
	}

	return UDP_Write (socketid, buf, len, (struct qsockaddr *)&broadcastaddrv4);
}

//=============================================================================

int UDP_Write (sys_socket_t socketid, byte *buf, int len, struct qsockaddr *addr)
{
	int	ret;
	socklen_t addrsize;

	// Spike changed this in R4
	// BSD sendto arg2 buf is type const void *, Windows is type const char * hence cast for Windows
	switch (addr->qsa_family) {
	case AF_INET:		addrsize = sizeof(struct sockaddr_in); break;
	case AF_INET6:		addrsize = sizeof(struct sockaddr_in6); break;
	default:			Con_SafePrintLinef ("UDP_Write: unknown family");
						return -1;	//some kind of error. a few systems get pissy if the size doesn't exactly match the address family
	}

	ret = sendto (socketid, (const char *)buf, len, 0, (struct sockaddr *)addr, addrsize);
	if (!addr->qsa_family) // Spike got rid of this in R4
		Con_SafePrintLinef ("UDP_Write: family was cleared"); // Spike got rid of this in R4
	if (ret == SOCKET_ERROR)
	{
		int err = SOCKETERRNO;
		if (err == NET_EWOULDBLOCK)
			return 0;
//		if (err == ENETUNREACH) //  Spike does use this.
//			Con_SafePrintLinef ("UDP_Write: %s (%s)", socketerror(err), UDP_AddrToString(addr));
		else
		Con_SafePrintLinef ("UDP_Write, sendto: %s", socketerror(err));
	}
	return ret;
}

//=============================================================================

const char *UDP_AddrToString (struct qsockaddr *addr, cbool masked)
{
	//static char buffer[22]; // 192.168.100.100:26001 is 21 chars
	static char buffer[64];
	int		haddr;

	if (addr->qsa_family == AF_INET6)
	{
		if (masked)
		{
			c_snprintf4 (buffer, "[%x:%x:%x:%x::]/64",
						ntohs((
             (struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[0]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[1]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[2]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[3]));
		}
		else
		{
			if (((struct sockaddr_in6 *)addr)->sin6_scope_id)
			{
				c_snprintf10 (buffer, "[%x:%x:%x:%x:%x:%x:%x:%x%%%d]:%d",
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[0]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[1]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[2]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[3]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[4]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[5]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[6]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[7]),
						(int)((struct sockaddr_in6_HAX *)addr)->sin6_scope_id,
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_port));
			}
			else
			{
				c_snprintf9 (buffer, "[%x:%x:%x:%x:%x:%x:%x:%x]:%d",
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[0]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[1]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[2]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[3]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[4]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[5]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[6]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_addr.u.Word[7]),
						ntohs(((struct sockaddr_in6_HAX *)addr)->sin6_port));
			}
		}
	}
	else
	{
		haddr = ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr);
		if (masked)
		{
			c_snprintf3 (buffer, "%d.%d.%d.0/24", (haddr >> 24) & 0xff,
					  (haddr >> 16) & 0xff, (haddr >> 8) & 0xff);
		}
		else
		{
			c_snprintf5 (buffer, "%d.%d.%d.%d:%d", (haddr >> 24) & 0xff,
					  (haddr >> 16) & 0xff, (haddr >> 8) & 0xff, haddr & 0xff,
					  ntohs(((struct sockaddr_in *)addr)->sin_port));
		}
	}

	return buffer;
}


//=============================================================================

int UDP4_StringToAddr (const char *string, struct qsockaddr *addr)
{
	int	ha1, ha2, ha3, ha4, hp, ipaddr;

	sscanf(string, "%d.%d.%d.%d:%d", &ha1, &ha2, &ha3, &ha4, &hp);
	ipaddr = (ha1 << 24) | (ha2 << 16) | (ha3 << 8) | ha4;

	addr->qsa_family = AF_INET;
	((struct sockaddr_in *)addr)->sin_addr.s_addr = htonl(ipaddr);
	((struct sockaddr_in *)addr)->sin_port = htons((unsigned short)hp);
	return 0;
}

//=============================================================================

// Baker: dfunc.GetSocketAddr returns int
// In the current source, the return value is never used nor checked
// The return value is 0 for success, non-zero for failure.
int UDP_GetSocketAddr (sys_socket_t socketid, struct qsockaddr *addr)
{
	socklen_t addrlen = sizeof(struct qsockaddr);

	memset(addr, 0, sizeof(struct qsockaddr));

	// Baker: getsockname returns 0 on success
	if (getsockname(socketid, (struct sockaddr *)addr, &addrlen) != 0)
		return -1;

	if (addr->qsa_family == AF_INET) {
		in_addr_t a = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
		if (a == 0 || a == htonl(INADDR_LOOPBACK))
		((struct sockaddr_in *)addr)->sin_addr.s_addr = myAddrv4;
	} else if (addr->qsa_family == AF_INET6) {
		static const in_addr6_t in6addr_any = {0}; // IN6ADDR_ANY_INIT;
		if (!memcmp(&((struct sockaddr_in6 *)addr)->sin6_addr, &in6addr_any, sizeof(in_addr6_t)))
			//memcpy(&((struct sockaddr_in6 *)addr)->sin6_addr, &myAddrv6, sizeof(in_addr6_t) /*sizeof(struct sockaddr_in6) ericw caught*/);
			memcpy(&((struct sockaddr_in6 *)addr)->sin6_addr, &myAddrv6, sizeof(((struct sockaddr_in6 *)addr)->sin6_addr)); // Spike
	}

	return 0;
}

//=============================================================================

int UDP_GetNameFromAddr (struct qsockaddr *addr, char *name, size_t len)
{
	/* From ProQuake: "commented this out because it's slow and completely useless"
	 struct hostent *hostentry;

	 hostentry = gethostbyaddr ((char *)&((struct sockaddr_in *)addr)->sin_addr, sizeof(struct in_addr), AF_INET);
	 if (hostentry)
	 {
	 strncpy (name, (char *)hostentry->h_name, NET_NAMELEN_64 - 1); // Fix NET_NAME_LEN to len if we ever uncomment
	 return 0;
	 }
	 */

	strlcpy (name, UDP_AddrToString (addr, false), len); // Baker: name length is unknown
	return 0;
}

//=============================================================================

int UDP4_GetAddrFromName (const char *name, struct qsockaddr *addr)
{
	struct hostent *hostentry;
	char *colon;
	unsigned short port = net_hostport;

	if (name[0] >= '0' && name[0] <= '9')
		return PartialIPAddress (name, addr);

	colon = strrchr(name, ':');

	if (colon)
	{
		char dupe[MAXHOSTNAMELEN];
		//set a breakpoint here.  I want to see when/if this happens
		if (colon-name + 1 > MAXHOSTNAMELEN)
			return -1;
		memcpy(dupe, name, colon-name);
		dupe[colon-name] = 0;
		if (strchr(dupe, ':'))
			return -1;	//don't resolve a name to an ipv4 address if it has multiple colons in it. its probably an ipx or ipv6 address, and I'd rather not block on any screwed dns resolves
		hostentry = gethostbyname (dupe);
		port = strtoul(colon+1, NULL, 10);
	}
	else
		hostentry = gethostbyname (name);
	if (!hostentry || hostentry->h_addrtype != AF_INET) // Spike got rid of the AF_INET check in R4
		return -1;

	addr->qsa_family = AF_INET;
	((struct sockaddr_in *)addr)->sin_port = htons(port);
	((struct sockaddr_in *)addr)->sin_addr.s_addr = *(in_addr_t *)hostentry->h_addr_list[0];

	return 0;
}

//=============================================================================

// Note: This returns 3 distinct values
// -1 is no match, 0 is match, 1 is ip match but not port match
int UDP_AddrCompare (struct qsockaddr *addr1, struct qsockaddr *addr2)
{
	if (addr1->qsa_family != addr2->qsa_family)
		return -1;

	switch (addr1->qsa_family) {
	case AF_INET6:
		if (memcmp(	&((struct sockaddr_in6 *)addr1)->sin6_addr, &((struct sockaddr_in6 *)addr2)->sin6_addr, sizeof(((struct sockaddr_in6 *)addr2)->sin6_addr)))
			return -1;

		if (((struct sockaddr_in6 *)addr1)->sin6_port !=
		    ((struct sockaddr_in6 *)addr2)->sin6_port)
			return 1;

		if (((struct sockaddr_in6 *)addr1)->sin6_scope_id &&
			((struct sockaddr_in6 *)addr2)->sin6_scope_id &&
			((struct sockaddr_in6 *)addr1)->sin6_scope_id !=
			((struct sockaddr_in6 *)addr2)->sin6_scope_id)	//the ipv6 scope id is for use with link-local addresses, to identify the specific interface.
			return 1;

		return 0;

	case AF_INET:
		if (((struct sockaddr_in *)addr1)->sin_addr.s_addr !=
		    ((struct sockaddr_in *)addr2)->sin_addr.s_addr)
			return -1;

		if (((struct sockaddr_in *)addr1)->sin_port !=
		    ((struct sockaddr_in *)addr2)->sin_port)
			return 1;

		return 0;

	default: return -1;
	}
}

//=============================================================================

int UDP_GetSocketPort (struct qsockaddr *addr)
{
	switch (addr->qsa_family) {
	case AF_INET:	return ntohs(((struct sockaddr_in *)addr)->sin_port);
	case AF_INET6:	return ntohs(((struct sockaddr_in6 *)addr)->sin6_port);
	default:		return -1;
	}
}


int UDP_SetSocketPort (struct qsockaddr *addr, int port)
{
	if (addr->qsa_family == AF_INET)		((struct sockaddr_in *)addr)->sin_port = htons((unsigned short)port);
	else if (addr->qsa_family == AF_INET6)	((struct sockaddr_in6 *)addr)->sin6_port = htons((unsigned short)port);
	else 									return -1;

	return 0;
}


//winxp (and possibly win2k) is dual stack.
//vista+ has a hybrid stack

static void UDP6_GetLocalAddress (void)
{
	char		buff[MAXHOSTNAMELEN];
	struct addrinfo hints, *local = NULL;

//	if (myAddrv6 != IN6ADDR_ANY)
//		return;

	if (gethostname(buff, MAXHOSTNAMELEN) == SOCKET_ERROR)
	{
		int	err = SOCKETERRNO;
		Con_SafePrintLinef ("UDP6_GetLocalAddress: gethostname failed (%s)", socketerror(err));
		return;
	}
	buff[MAXHOSTNAMELEN - 1] = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	if (getaddrinfo(buff, NULL, &hints, &local) == 0)
	{
		int length;
		c_strlcpy (my_ipv6_address, UDP_AddrToString((struct qsockaddr*)local->ai_addr, false));
		length = strlen(my_ipv6_address);
		if (length > 2 && !strcmp(my_ipv6_address + length - 2, ":0"))
			my_ipv6_address[length - 2] = 0;
		freeaddrinfo(local);
	}

	if (local == NULL) {
		int	err = SOCKETERRNO;
		if (err) { // Linux returns SOCKET_ERROR with error code 0 (success) for anything that resolves to 127.0.0.1
            Con_SafePrintLinef ("UDP6_GetLocalAddress: gethostbyname failed (#%d: %s)", err, socketerror(err));
            return;
		}
	}
}

sys_socket_t UDP6_Init (void)
{
	char	buff[MAXHOSTNAMELEN];
	int	t;

	if (COM_CheckParm ("-noudp") || COM_CheckParm ("-noudp6"))
		return INVALID_SOCKET;

	if (!UDP_Platform_Startup())
		return INVALID_SOCKET; // UDP_Platform_Startup already printed a fail message

	// determine my name & address
	if (gethostname(buff, MAXHOSTNAMELEN) != 0) {
		int err = SOCKETERRNO;
		Con_SafePrintLinef ("UDP6_Init: gethostname failed (%s)", socketerror(err));
	}
	else
	{
		buff[MAXHOSTNAMELEN - 1] = 0;
	}

	t = COM_CheckParm ("-ip6");
	if (t)
	{
		if (t < com_argc-1)
		{
			if (UDP6_GetAddrFromName(com_argv[t + 1], (struct qsockaddr*)&bindAddrv6))
				System_Error ("%s is not a valid IPv6 address", com_argv[t + 1]);
			if (!*my_ipv6_address)
				c_strlcpy (my_ipv6_address, com_argv[t + 1]);  // Set breakpoint here
		}
		else
		{
			System_Error ("UDP6_Init: you must specify an IP address after -ip6");
		}
	}
	else
	{
		memset(&bindAddrv6, 0, sizeof(bindAddrv6));
		if (!*my_ipv6_address)
		{
			strcpy(my_ipv6_address, "[::]");
			UDP6_GetLocalAddress();
		}
	}

	myAddrv6 = bindAddrv6;

	if ((netv6_controlsocket = UDP6_OpenSocket(0)) == INVALID_SOCKET)
	{
		Con_SafePrintLinef ("UDP6_Init: Unable to open control socket, UDPv6 disabled");
		UDP_Platform_Cleanup ();
		return INVALID_SOCKET;
	}

	broadcastaddrv6.sin6_family = AF_INET6;
	memset(&broadcastaddrv6.sin6_addr, 0, sizeof(broadcastaddrv6.sin6_addr));

    {
        struct sockaddr_in6_HAX *broadcaster = (struct sockaddr_in6_HAX *)&broadcastaddrv6;
        broadcaster->sin6_addr.u.Byte[0] = 0xff;
        broadcaster->sin6_addr.u.Byte[1] = 0x03;
        broadcaster->sin6_addr.u.Byte[15] = 0x01;
        broadcaster->sin6_port = htons((unsigned short)net_hostport);
//        broadcastaddrv6.sin6_addr.u.Byte[0] = 0xff;
//        broadcastaddrv6.sin6_addr.u.Byte[1] = 0x03;
//        broadcastaddrv6.sin6_addr.u.Byte[15] = 0x01;
//        broadcastaddrv6.sin6_port = htons((unsigned short)net_hostport);
    }

	Con_SafePrintLinef ("IPv6 Initialized: %s", my_ipv6_address);
	ipv6Available = true;

	return netv6_controlsocket;
}

sys_socket_t UDP6_Listen (cbool state)
{
	if (state)
	{
		// enable listening
		if (netv6_acceptsocket == INVALID_SOCKET)
		{
			if ((netv6_acceptsocket = UDP6_OpenSocket (net_hostport)) == INVALID_SOCKET)
				System_Error ("UDP6_Listen: Unable to open accept socket");
		}
	}
	else
	{
		// disable listening
		if (netv6_acceptsocket != INVALID_SOCKET)
		{
			UDP_CloseSocket (netv6_acceptsocket);
			netv6_acceptsocket = INVALID_SOCKET;
		}
	}
	return netv6_acceptsocket;
}

void UDP6_Shutdown (void)
{
	UDP6_Listen (false);
	UDP_CloseSocket (netv6_controlsocket);
	UDP_Platform_Cleanup ();
}

sys_socket_t UDP6_OpenSocket (int port)
{
	sys_socket_t newsocket;
	struct sockaddr_in6 address;
	ioctl_uint32_t _true = 1;
	int err;

	if ((newsocket = socket (PF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		err = SOCKETERRNO;
		Con_SafePrintLinef ("UDP6_OpenSocket: %s", socketerror(err));
		return INVALID_SOCKET;
	}

	setsockopt(newsocket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&_true, sizeof(_true));

	if (ioctlsocket (newsocket, FIONBIO, &_true) == SOCKET_ERROR)
		goto ErrorReturn;

	memset(&address, 0, sizeof(address));
	address.sin6_family = AF_INET6;
#ifndef _WIN32 // PLATFORM_OSX // Crusty Mac .. although is IPv6 issue
	memcpy (&address.sin6_addr, &bindAddrv6, sizeof(address.sin6_addr));
#else
	address.sin6_addr = bindAddrv6;
#endif

	address.sin6_port = htons((unsigned short)port);
	if (bind (newsocket, (struct sockaddr *)&address, sizeof(address)) == 0)
	{
		//we don't know if we're the server or not. oh well.
		struct ipv6_mreq req;
		req.ipv6mr_multiaddr = broadcastaddrv6.sin6_addr;
		req.ipv6mr_interface = 0;
		setsockopt(newsocket, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char *)&req, sizeof(req));

		return newsocket;
	}

	if (ipv6Available)
	{
		err = SOCKETERRNO;
		System_Error ("Unable to bind to %s (%s)",
				UDP_AddrToString ((struct qsockaddr *) &address, false),
				socketerror(err));
		return INVALID_SOCKET;	/* not reached */
	}
	/* else: we are still in init phase, no need to error */

ErrorReturn:
	err = SOCKETERRNO;
	Con_SafePrintLinef ("UDP6_OpenSocket: %s", socketerror(err));
	closesocket (newsocket);
	return INVALID_SOCKET;
}

sys_socket_t UDP6_CheckNewConnections (void)
{
	return UDP_CheckNewConnections (netv6_acceptsocket);
}


int UDP6_Broadcast (sys_socket_t socketid, byte *buf, int len)
{
	broadcastaddrv6.sin6_port = htons((unsigned short)net_hostport);
	return UDP_Write (socketid, buf, len, (struct qsockaddr *)&broadcastaddrv6);
}

int UDP6_StringToAddr (const char *string, struct qsockaddr *addr)
{	//This is never actually called... (still ?)
	// Con_SafePrintLinef ("UDP6_StringToAddr: %s", string);
	return -1;
}

int  UDP6_GetNameFromAddr (struct qsockaddr *addr, char *name, int len)
{
	//FIXME: should really do a reverse dns lookup.
	strlcpy (name, UDP_AddrToString(addr, false), len); //NET_NAMELEN_64
	return 0;
}

// Return 0 on success, -1 on failure
int UDP6_GetAddrFromName (const char *name, struct qsockaddr *addr)
{
	struct addrinfo *addrinfo = NULL;
	struct addrinfo *pos;
	struct addrinfo udp6hint;
	int error;
	char *port;
	char dupbase[256];
	size_t len;
	cbool success = false;

	memset(&udp6hint, 0, sizeof(udp6hint));
	udp6hint.ai_family = 0;//Any... we check for AF_INET6 or 4
	udp6hint.ai_socktype = SOCK_DGRAM;
	udp6hint.ai_protocol = IPPROTO_UDP;

	if (*name == '[')
	{
		port = strstr(name, "]");
		if (!port)
			error = EAI_NONAME;
		else
		{
			len = port - (name+1);
			if (len >= sizeof(dupbase))
				len = sizeof(dupbase)-1;
			strncpy(dupbase, name+1, len);
			dupbase[len] = '\0';
			error = getaddrinfo(dupbase, (port[1] == ':')?port+2:NULL, &udp6hint, &addrinfo);
		}
	}
	else
	{
		port = strrchr(name, ':');

		if (port)
		{
			len = port - name;
			if (len >= sizeof(dupbase))
				len = sizeof(dupbase)-1;
			strncpy(dupbase, name, len);
			dupbase[len] = '\0';
			error = getaddrinfo(dupbase, port+1, &udp6hint, &addrinfo);
		}
		else
			error = EAI_NONAME;
		if (error)	//failed, try string with no port.
			error = getaddrinfo(name, NULL, &udp6hint, &addrinfo);	//remember, this func will return any address family that could be using the udp protocol... (ip4 or ip6)
	}

	if (!error)
	{
		((struct sockaddr*)addr)->sa_family = 0;
		for (pos = addrinfo; pos; pos = pos->ai_next)
		{
			if (pos->ai_family == AF_INET) // Uh?  WHY?
			{
				memcpy(addr, pos->ai_addr, pos->ai_addrlen);
				success = true;
				break;
			}
			if (pos->ai_family == AF_INET6 && !success)
			{
				memcpy(addr, pos->ai_addr, pos->ai_addrlen);
				success = true;
			}
		}
		freeaddrinfo (addrinfo);
	}

	if (success)
	{
		if (((struct sockaddr*)addr)->sa_family == AF_INET)
		{
			if (!((struct sockaddr_in *)addr)->sin_port)
				((struct sockaddr_in *)addr)->sin_port = htons((unsigned short)net_hostport);
		}
		else if (((struct sockaddr*)addr)->sa_family == AF_INET6)
		{
			if (!((struct sockaddr_in6 *)addr)->sin6_port)
				((struct sockaddr_in6 *)addr)->sin6_port = htons((unsigned short)net_hostport);
		}
		return 0;
	}

	return -1;
}

//=============================================================================