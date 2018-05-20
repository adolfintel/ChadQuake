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
// net_dgrm.c

// This is enables a simple IP banning mechanism
#define BAN_TEST

#include <core.h>
#include "q_stdinc.h"
#include "arch_def.h"
#include "net_sys.h"
#include "quakedef.h"
#include "net_defs.h"
#include "net_dgrm.h"

// these two macros are to make the code more readable
#define sfunc	net_landrivers[sock->landriver]
#define dfunc	net_landrivers[net_landriverlevel]

static int net_landriverlevel;

/* statistic counters */
static int	packetsSent = 0;
static int	packetsReSent = 0;
static int packetsReceived = 0;
static int receivedDuplicateCount = 0;
static int shortPacketCount = 0;
static int droppedDatagrams;


static struct
{
	unsigned int	length;
	unsigned int	sequence;
	byte			data[MAX_MARK_V_DATAGRAM];
} packetBuffer;

static int myDriverLevel;

extern cbool m_return_onerror;
extern char m_return_reason[32];
static double heartbeat_time;	//when this is reached, send a heartbeat to all masters.

static cbool testInProgress = false;
static int		testPollCount;
static int		testDriver;
static sys_socket_t		testSocket;

static void Test_Poll(void *);
static PollProcedure	testPollProcedure = {NULL, 0.0, Test_Poll};
static void Rcon_Poll (void *);
PollProcedure	rconPollProcedure = {NULL, 0.0, Rcon_Poll};

static const char *Strip_Port (const char *host);

static char *StrAddr (struct qsockaddr *addr)
{
	static char buf[34];
	byte *p = (byte *)addr;
	int n;

	for (n = 0; n < 16; n++)
		sprintf (buf + n * 2, "%02x", *p++);
	return buf;
}



// JPG 3.02 - rcon
extern cvar_t rcon_password;
extern cvar_t rcon_server;
extern char server_name[MAX_QPATH_64];

void Rcon_f (lparse_t *line)
{
	const char	*host;
	int		n;
	struct qsockaddr sendaddr;
	size_t offsetz = line->args[1] - line->chopped; // arg1 and beyond, skipping "rcon" command
	char *cmd_after_whitespace = &line->original[offsetz];

	// Baker: A server shouldn't be sending rcon commands
	if (cmd_from_server) {
		Con_WarningLinef ("Server has attempted to get to us to send an rcon command.  Highly inappropriate.  Rejected.");
		return;
	}

	if (testInProgress)
	{
		Con_PrintLinef ("There is already a test/rcon in progress");
		return;
	}

	if (line->count < 2)
	{
		Con_PrintLinef ("usage: rcon <command>");
		return;
	}

	if (!*rcon_password.string)
	{
		Con_PrintLinef ("rcon_password has not been set");
		return;
	}

	host = rcon_server.string;

	if (!*rcon_server.string)
	{
		// JPG 3.50 - use current server
		if (cls.state == ca_connected)
		{
			// Baker: This is dangerous and has to go.  You are giving just any server the rcon password.
			// Someone can setup an evil server and intercept this.
			host = server_name;
		}
		else
		{
			Con_PrintLinef ("rcon_server has not been set");
			return;
		}
	}

	Strip_Port(host);

	if (host && hostCacheCount)
	{
		for (n = 0; n < hostCacheCount; n++)
			if (strcasecmp (host, hostcache[n].name) == 0)
			{
				if (hostcache[n].driver != myDriverLevel)
					continue;
				net_landriverlevel = hostcache[n].ldriver;
				memcpy(&sendaddr, &hostcache[n].addr, sizeof(struct qsockaddr));
				break;
			}
		if (n < hostCacheCount)
			goto JustDoIt;
	}

	for (net_landriverlevel = 0; net_landriverlevel < net_numlandrivers; net_landriverlevel++)
	{
		if (!net_landrivers[net_landriverlevel].initialized)
			continue;

		// see if we can resolve the host name
		if (dfunc.GetAddrFromName(host, &sendaddr) != -1)
			break;
	}
	if (net_landriverlevel == net_numlandrivers)
	{
		Con_PrintLinef ("Could not resolve %s", host);
		return;
	}

JustDoIt:
	testSocket = dfunc.Open_Socket(0);
	if (testSocket == INVALID_SOCKET /*  -1*/)
	{
		Con_PrintLinef ("Could not open socket");
		return;
	}

	testInProgress = true;
	testPollCount = 20;
	testDriver = net_landriverlevel;

	SZ_Clear(&net_message);
	// save space for the header, filled in later
	MSG_WriteLong(&net_message, 0);
	MSG_WriteByte(&net_message, CCREQ_RCON);
	MSG_WriteString(&net_message, rcon_password.string);
	MSG_WriteString(&net_message, cmd_after_whitespace);
	*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
	dfunc.Write (testSocket, net_message.data, net_message.cursize, &sendaddr);
	SZ_Clear(&net_message);
	SchedulePollProcedure(&rconPollProcedure, 0.05);
}


static void Rcon_Poll (void* unused)
{
	struct qsockaddr clientaddr;
	int		control, len;

	net_landriverlevel = testDriver;

	len = dfunc.Read (testSocket, net_message.data, net_message.maxsize, &clientaddr);

	if (len < (int)sizeof(int))
	{
		testPollCount--;
		if (testPollCount)
		{
			SchedulePollProcedure(&rconPollProcedure, 0.25);
			return;
		}
		Con_PrintLinef ("rcon: no response");
		goto Done;
	}

	net_message.cursize = len;

	MSG_BeginReading ();
	control = BigLong(*((int *)net_message.data));
	MSG_ReadLong();
	if (control == -1)
		goto Error;
	if ((control & (~NETFLAG_LENGTH_MASK)) !=  (int)NETFLAG_CTL) // Matters?
		goto Error;
	if ((control & NETFLAG_LENGTH_MASK) != len)
		goto Error;

	if (MSG_ReadByte() != CCREP_RCON)
		goto Error;

	Con_PrintLinef ("%s", MSG_ReadString());

	goto Done;

Error:
	Con_PrintLinef ("Unexpected response to rcon command");

Done:
	dfunc.Close_Socket(testSocket);
	testInProgress = false;
	return;
}

// JPG 3.00 - this code appears multiple times, so factor it out

cbool Datagram_Reject (const char *message, sys_socket_t acceptsock, struct qsockaddr *pclientaddr)
{
	SZ_Clear(&net_message);
	// save space for the header, filled in later
	MSG_WriteLong(&net_message, 0);
	MSG_WriteByte(&net_message, CCREP_REJECT);
	MSG_WriteString(&net_message, message);
	*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
	dfunc.Write (acceptsock, net_message.data, net_message.cursize, pclientaddr);
	SZ_Clear(&net_message);

	return false;
}

extern cvar_t pq_password;			// JPG 3.00 - password protection
extern unsigned long qsmackAddr;	// JPG 3.02 - allow qsmack bots to connect to server
#ifdef SUPPORTS_PQ_RCON_FAILURE_BLACKOUT // Baker change
typedef struct
{
	char	ip_address[22];
	float	when;
	int		count;
} rcon_fail_t;

rcon_fail_t rcon_ips_fails[100];
const int num_rcon_ips_fails = sizeof(rcon_ips_fails) / sizeof(rcon_ips_fails[0]);
int rcon_cursor;

cbool Rcon_Blackout (const char* address, float nowtime)
{
	int i;

	for (i = 0; i < num_rcon_ips_fails; i ++)
	{
		if (rcon_ips_fails[i].ip_address[0] == 0)
			continue; // Unused slot
		else
		{
			rcon_fail_t* slot = &rcon_ips_fails[i];
			if (strcmp(slot->ip_address, address) == 0) // found
				if (slot->count == 0 && realtime < slot->when + 300)
				{
					Con_PrintLinef ("Slot %d has rcon black out until %f (remaining is %f)", i, slot->when + 300, realtime - (slot->when + 300));
					return true;
				}
				else break;
		}
	}

	return false;
}

void Rcon_Fails_Log (const char* address, float nowtime)
{
	cbool found = false;
	float oldest_time = nowtime;
	int i, empty = -1, oldest = -1;

	// Find either this ip address or empty slot.
	for (i = 0; i < num_rcon_ips_fails; i ++)
	{
		if (rcon_ips_fails[i].ip_address[0] == 0)
		{
			if  (empty == -1)
			{
				empty = i;
				break;
			}
			else continue;
		}
		else if (rcon_ips_fails[i].ip_address[0] != 0 && strcmp(rcon_ips_fails[i].ip_address, address) == 0)
		{
			found = true;
			break;
		}
		else if (rcon_ips_fails[i].when <= oldest_time)
		{
			oldest_time = rcon_ips_fails[i].when;
			oldest = i;
		}
	}

	if (found == false)
	{
		// Use empty slot or oldest slot
		int myslot = empty >=0 ? empty : oldest;
		rcon_fail_t* slot = &rcon_ips_fails[myslot];
		c_strlcpy (slot->ip_address, address);
		slot->count = 1;
		slot->when = nowtime;
		Con_PrintLinef ("Rcon failure recorded to new slot %d", myslot);
	}
	else
	{
		rcon_fail_t* slot = &rcon_ips_fails[i];
		c_strlcpy (slot->ip_address, address);
		slot->count ++;
		slot->when = nowtime;

		if (slot->count > 3)
			slot->count = 0; // Black out

		Con_PrintLinef ("Rcon failure  count for existing slot of %d is count = %d", i, slot->count);

	}

}
#include <time.h>
#endif // Baker change + SUPPORTS_PQ_RCON_FAILURE_BLACKOUT

#ifdef BAN_TEST
// October 2016:  This is complete junk and doesn't ipv6
static struct in_addr	banAddr;
static struct in_addr	banMask;

// This will be going away.
void NET_Ban_f (lparse_t *line)
{
	char	addrStr [32];
	char	maskStr [32];
	int	    (*printline_fn)(const char *fmt, ...)__core_attribute__((__format__(__printf__,1,2)));

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
	{
		if (pr_global_struct->deathmatch && !host_client->privileged)
			return;
		printline_fn = SV_ClientPrintLinef;
	}

// This function is just a gatekeeper now.
//	Ban_f (line);
	switch (line->count)
	{
		case 1:
		if (banAddr.s_addr != INADDR_ANY)
			{
			c_strlcpy (addrStr, inet_ntoa(banAddr));
			c_strlcpy (maskStr, inet_ntoa(banMask));
				printline_fn ("Banning %s [%s]", addrStr, maskStr);
			}
			else
				printline_fn ("Banning not active");
			break;

		case 2:
			if (strcasecmp(line->args[1], "off") == 0)
				banAddr.s_addr = INADDR_ANY;
			else
				banAddr.s_addr = inet_addr(line->args[1]);
			banMask.s_addr = INADDR_NONE;
			break;

		case 3:
			banAddr.s_addr = inet_addr(line->args[1]);
			banMask.s_addr = inet_addr(line->args[2]);
			break;

		default:
			printline_fn ("BAN ip_address [mask]");
			break;
	}
}
#endif	// BAN_TEST


int Datagram_SendMessage (qsocket_t *sock, sizebuf_t *data)
{
	unsigned int	packetLen;
	unsigned int	dataLen;
	unsigned int	eom;

#ifdef _DEBUG
	if (data->cursize == 0)
		System_Error ("Datagram_SendMessage: zero length message");

	if (data->cursize > NET_MARK_V_MAXMESSAGE)
		System_Error ("Datagram_SendMessage: message too big %u", data->cursize);

	if (sock->canSend == false)
		System_Error ("SendMessage: called with canSend == false");
#endif

//	Con_PrintLinef ("Sending data to %s with length of %d (max: %d) with maxsize of %d", sock->address,  data->cursize, data->maxsize, sv.datagram.maxsize);

	memcpy (sock->sendMessage, data->data, data->cursize);
	sock->sendMessageLength = data->cursize;

#ifdef SUPPORTS_SERVER_PROTOCOL_15
	if (data->cursize <= host_protocol_datagram_maxsize)
#endif // SUPPORTS_SERVER_PROTOCOL_15
	{
		dataLen = data->cursize;
		eom = NETFLAG_EOM;
	}
	else
	{
#ifdef SUPPORTS_SERVER_PROTOCOL_15
		dataLen = host_protocol_datagram_maxsize;
#endif // SUPPORTS_SERVER_PROTOCOL_15
		eom = 0;
	}
	packetLen = NET_HEADERSIZE + dataLen;

	packetBuffer.length = BigLong(packetLen | (NETFLAG_DATA | eom));
	packetBuffer.sequence = BigLong(sock->sendSequence++);
	memcpy (packetBuffer.data, sock->sendMessage, dataLen);

	sock->canSend = false;

//	Con_PrintLinef ("Datagram sent with size %d, maxsize should be %d", packetLen, host_protocol_datagram_maxsize);

	if (sfunc.Write (sock->socket, (byte *)&packetBuffer, packetLen, &sock->addr) == -1)
		return -1;

	sock->lastSendTime = net_time;
	packetsSent++;

	return 1;
}


static int SendMessageNext (qsocket_t *sock)
{
	unsigned int	packetLen;
	unsigned int	dataLen;
	unsigned int	eom;

#ifdef SUPPORTS_SERVER_PROTOCOL_15
	if (sock->sendMessageLength <= host_protocol_datagram_maxsize)
#endif // SUPPORTS_SERVER_PROTOCOL_15
	{
		dataLen = sock->sendMessageLength;
		eom = NETFLAG_EOM;
	}
	else
	{
#ifdef SUPPORTS_SERVER_PROTOCOL_15
		dataLen = host_protocol_datagram_maxsize;
#endif // SUPPORTS_SERVER_PROTOCOL_15
		eom = 0;
	}
	packetLen = NET_HEADERSIZE + dataLen;

	packetBuffer.length = BigLong(packetLen | (NETFLAG_DATA | eom));
	packetBuffer.sequence = BigLong(sock->sendSequence++);
	memcpy (packetBuffer.data, sock->sendMessage, dataLen);

	sock->sendNext = false;

	if (sfunc.Write (sock->socket, (byte *)&packetBuffer, packetLen, &sock->addr) == -1)
		return -1;

	sock->lastSendTime = net_time;
	packetsSent++;

	return 1;
}


static int ReSendMessage (qsocket_t *sock)
{
	unsigned int	packetLen;
	unsigned int	dataLen;
	unsigned int	eom;

#ifdef SUPPORTS_SERVER_PROTOCOL_15
	if (sock->sendMessageLength <= host_protocol_datagram_maxsize)
#endif // SUPPORTS_SERVER_PROTOCOL_15
	{
		dataLen = sock->sendMessageLength;
		eom = NETFLAG_EOM;
	}
	else
	{
#ifdef SUPPORTS_SERVER_PROTOCOL_15
		dataLen = host_protocol_datagram_maxsize;
#endif // SUPPORTS_SERVER_PROTOCOL_15
		eom = 0;
	}
	packetLen = NET_HEADERSIZE + dataLen;

	packetBuffer.length = BigLong(packetLen | (NETFLAG_DATA | eom));
	packetBuffer.sequence = BigLong(sock->sendSequence - 1);
	memcpy (packetBuffer.data, sock->sendMessage, dataLen);

	sock->sendNext = false;

	if (sfunc.Write (sock->socket, (byte *)&packetBuffer, packetLen, &sock->addr) == -1)
		return -1;

	sock->lastSendTime = net_time;
	packetsReSent++;

	return 1;
}


cbool Datagram_CanSendMessage (qsocket_t *sock)
{
	if (sock->sendNext)
		SendMessageNext (sock);

	return sock->canSend;
}


cbool Datagram_CanSendUnreliableMessage (qsocket_t *sock)
{
	return true;
}


int Datagram_SendUnreliableMessage (qsocket_t *sock, sizebuf_t *data)
{
	int 	packetLen;

#ifdef _DEBUG
	if (data->cursize == 0)
		System_Error ("Datagram_SendUnreliableMessage: zero length message");

	if (data->cursize > MAX_MARK_V_DATAGRAM)
		System_Error ("Datagram_SendUnreliableMessage: message too big %u", data->cursize);
#endif

	packetLen = NET_HEADERSIZE + data->cursize;

	packetBuffer.length = BigLong(packetLen | NETFLAG_UNRELIABLE);
	packetBuffer.sequence = BigLong(sock->unreliableSendSequence++);
	memcpy (packetBuffer.data, data->data, data->cursize);

	if (sfunc.Write (sock->socket, (byte *)&packetBuffer, packetLen, &sock->addr) == -1)
		return -1;

	packetsSent++;
	return 1;
}

// This should be called ProcessServerPacket
static void _Datagram_ServerControlPacket (sys_socket_t acceptsock, struct qsockaddr *clientaddr, byte *data, int length);
// Only called by Datagram_GetAnyMessage
cbool Datagram_ProcessPacket(unsigned int length, qsocket_t *sock)
{
	unsigned int	flags;
	unsigned int	sequence;
	unsigned int	count;

	if (length < NET_HEADERSIZE)
	{
		shortPacketCount++;
		return false;
	}

	length = BigLong(packetBuffer.length);
	flags = length & (~NETFLAG_LENGTH_MASK);
	length &= NETFLAG_LENGTH_MASK;

	if (flags & NETFLAG_CTL)
		return false;	//should only be for OOB packets.

	sequence = BigLong(packetBuffer.sequence);
	packetsReceived++;

	if (flags & NETFLAG_UNRELIABLE)
	{
		if (sequence < sock->unreliableReceiveSequence)
		{
			Con_PrintLinef ("Got a stale datagram");
			return false;
		}
		if (sequence != sock->unreliableReceiveSequence)
		{
			count = sequence - sock->unreliableReceiveSequence;
			droppedDatagrams += count;
			Con_DPrintLinef ("Dropped %u datagram(s)", count);
		}
		sock->unreliableReceiveSequence = sequence + 1;

		length -= NET_HEADERSIZE;

		if (length > (unsigned int)net_message.maxsize)
		{	//is this even possible? maybe it will be in the future! either way, no sys_errors please.
			Con_PrintLinef ("Over-sized unreliable");
			return -1;
		}

		SZ_Clear (&net_message);
		SZ_Write (&net_message, packetBuffer.data, length);

		unreliableMessagesReceived++;
		return true;	//parse the unreliable
	}

	if (flags & NETFLAG_ACK)
	{
		if (sequence != (sock->sendSequence - 1))
		{
			Con_PrintLinef ("Stale ACK received");
			return false;
		}
		if (sequence == sock->ackSequence)
		{
			sock->ackSequence++;
			if (sock->ackSequence != sock->sendSequence)
				Con_PrintLinef ("ack sequencing error");
		}
		else
		{
			Con_PrintLinef ("Duplicate ACK received");
			return false;
		}
		//sock->sendMessageLength -= RELIABLE_MTU;
		sock->sendMessageLength -= host_protocol_datagram_maxsize;
		if (sock->sendMessageLength > 0)
		{
			//memmove (sock->sendMessage, sock->sendMessage + RELIABLE_MTU, sock->sendMessageLength);
			memmove (sock->sendMessage, sock->sendMessage + host_protocol_datagram_maxsize, sock->sendMessageLength);
			sock->sendNext = true;
		}
		else
		{
			sock->sendMessageLength = 0;
			sock->canSend = true;
		}
		return false;
	}

	if (flags & NETFLAG_DATA)
	{
		packetBuffer.length = BigLong(NET_HEADERSIZE | NETFLAG_ACK);
		packetBuffer.sequence = BigLong(sequence);
		sfunc.Write (sock->socket, (byte *)&packetBuffer, NET_HEADERSIZE, &sock->addr);

		if (sequence != sock->receiveSequence)
		{
			receivedDuplicateCount++;
			return false;
		}
		sock->receiveSequence++;

		length -= NET_HEADERSIZE;

		if (flags & NETFLAG_EOM)
		{
			if (sock->receiveMessageLength + length > (unsigned int)net_message.maxsize)
			{
				Con_PrintLinef ("Over-sized reliable");
				return -1;
			}
			SZ_Clear(&net_message);
			SZ_Write(&net_message, sock->receiveMessage, sock->receiveMessageLength);
			SZ_Write(&net_message, packetBuffer.data, length);
			sock->receiveMessageLength = 0;

			messagesReceived++;
			return true;	//parse this reliable!
		}

		if (sock->receiveMessageLength + length > sizeof(sock->receiveMessage))
		{
			Con_PrintLinef ("Over-sized reliable");
			return -1;
		}
		memcpy(sock->receiveMessage + sock->receiveMessageLength, packetBuffer.data, length);
		sock->receiveMessageLength += length;
		return false;	//still watiting for the eom
	}
	//unknown flags
	Con_PrintLinef ("Unknown packet flags");
	return false;
}

// This should be called ServerGetAnyMessage
qsocket_t *Datagram_GetAnyMessage(void)
{
	qsocket_t *s;
	struct qsockaddr addr;
	int length;
	for (net_landriverlevel = 0; net_landriverlevel < net_numlandrivers; net_landriverlevel++)
	{
		sys_socket_t sock;
		if (!dfunc.initialized)
			continue;
		sock = dfunc.listeningSock; // was dfunc.Listen(true) until R4
		if (sock == INVALID_SOCKET)
			continue; // set breakpoint here.  Yes this happens.

		while(1)
		{
			length = dfunc.Read(sock, (byte *)&packetBuffer, NET_MARK_V_DATAGRAMSIZE, &addr);

			if (length < 0) {
				// Connection reset by peer or some other issue.
				for (s = net_activeSockets; s; s = s->next) {
					int i;
					if (s->driver == net_driverlevel && dfunc.AddrCompare(&addr, &s->addr) == 0) {
						// Found the connection.
						if (s->disconnected)
							continue;  // set breakpoint here.  This breakpoint never seems to hit, in the tests in scenarios I could think of.
						if (!s->isvirtual)
							continue; // set breakpoint here. This breakpoint never seems to hit, in the tests in scenarios I could think of.

						for (i = 0; i < svs.maxclients_internal; i ++) {  // Because the cap can change at any time now.
							if (svs.clients[i].netconnection == s) {
								host_client = &svs.clients[i];
								Con_DPrintLinef ("Booting %d for socket error", i);
								SV_DropClient (false);
								break;
							}
							// Keep searching for client with that socket.
						}
						break;
					}
					// Keep searching for socket
				}
				// Whatever happened with this one, move on to the next
				break;
			}

			if (!length)
			{
				//no more packets, move on to the next.
				break;
			}

			if (length < 4)
				continue;
			if (BigLong(packetBuffer.length) & NETFLAG_CTL)
			{
				_Datagram_ServerControlPacket(sock, &addr, (byte *)&packetBuffer, length);
				continue;
			}

			//figure out which qsocket it was for
			for (s = net_activeSockets; s; s = s->next)
			{
				if (s->driver != net_driverlevel)
					continue;
				// disconnected is set to false on new qsocket, isvirtual is set to true on connection accepted
				if (s->disconnected)
					continue;  // set breakpoint here.  This breakpoint never seems to hit, in the tests in scenarios I could think of.
				if (!s->isvirtual)
					continue;  // set breakpoint here.  This breakpoint never seems to hit, in the tests in scenarios I could think of.

				if (dfunc.AddrCompare(&addr, &s->addr) == 0)
				{
					//okay, looks like this is us. try to process it, and if there's new data
					if (Datagram_ProcessPacket(length, s)) {
						s->lastMessageTime = net_time;
						return s;	//the server needs to parse that packet.
					}
				}
			}
			//stray packet... ignore it and just try the next
		}
	}

	// Run everyone
	for (s = net_activeSockets; s; s = s->next)
	{
		if (s->driver != net_driverlevel)
			continue;
#if 11111
		if (!s->isvirtual)
			continue; // set breakpoint here.  This breakpoint never seems to hit, in the tests in scenarios I could think of.
#endif
		if (!s->canSend)
			if ((net_time - s->lastSendTime) > 1.0)
				ReSendMessage (s);
		if (s->sendNext)
			SendMessageNext (s);

#if 1
		// check for a timeout, if so kick them.  Don't do it for the loopback.
		// What about a very slow connector during a changelevel?  Latency and a lot of spawn time b.s?  Or client that needed to load replacement gfx?
		// Baker: loopback never hits here
		// I changed the timeout to 20 seconds.  But I'm not sure that any length would
		// would suffice for, say, DarkPlaces with 4 GB of replacement content on a map change
		// Could take 2 minutes or so in some sluggish instances.  Whatever.
		if (net_connecttimeout.value && net_time - s->lastMessageTime > net_connecttimeout.value) {
			int i; for (i = 0; i < svs.maxclients_internal; i++) {  // Because the cap can change at any time now.
				if (svs.clients[i].netconnection == s) {
					host_client = &svs.clients[i];
					SV_DropClient (true);
					break;
				}
			}
		}
#endif

	}

	return NULL;
}

int	Datagram_GetMessage (qsocket_t *sock)
{
	unsigned int	length;
	unsigned int	flags;
	int				ret = 0;
	struct qsockaddr readaddr;
	unsigned int	sequence;
	unsigned int	count;

	if (!sock->canSend)
		if ((net_time - sock->lastSendTime) > 1.0)
			ReSendMessage (sock);

	while(1)
	{
		length = (unsigned int)sfunc.Read (sock->socket, (byte *)&packetBuffer,
#ifdef SUPPORTS_SERVER_PROTOCOL_15
			host_protocol_datagram_maxsize, &readaddr);
#endif // SUPPORTS_SERVER_PROTOCOL_15

//	if ((rand() & 255) > 220)
//		continue;

		if (length == 0)
			break;

		if (length == (unsigned int)-1)
		{
			Con_PrintLinef ("Datagram_GetMessage: Read error");
			return -1;
		}

		if (sfunc.AddrCompare(&readaddr, &sock->addr) != 0)
		{
#if 0 //def _DEBUG // Baker:  Quake source release doesn't printf this in release build
			Con_PrintLinef ("Forged packet received");
			Con_PrintLinef ("Expected: %s", StrAddr (&sock->addr));
			Con_PrintLinef ("Received: %s", StrAddr (&readaddr));
			continue;
#endif //
		}

		if (length < NET_HEADERSIZE)
		{
			shortPacketCount++;
			continue;
		}

		length = BigLong(packetBuffer.length);
		flags = length & (~NETFLAG_LENGTH_MASK);
		length &= NETFLAG_LENGTH_MASK;

//#ifdef SUPPORTS_NETWORK_FIX // Baker change +
		// From ProQuake:  fix for attack that crashes server
		if (length > NET_MARK_V_DATAGRAMSIZE)
		{
			Con_PrintLinef ("Datagram_GetMessage: Invalid length");
			return -1;
		}
//#endif // Baker change +

		if (flags & NETFLAG_CTL)
			continue;

		sequence = BigLong(packetBuffer.sequence);
		packetsReceived++;

		if (flags & NETFLAG_UNRELIABLE)
		{
			if (sequence < sock->unreliableReceiveSequence)
			{
				Con_PrintLinef ("Got a stale datagram");
				ret = 0;
				break;
			}
			if (sequence != sock->unreliableReceiveSequence)
			{
				count = sequence - sock->unreliableReceiveSequence;
				droppedDatagrams += count;
				Con_DPrintLinef ("Dropped %u datagram(s)", count);
			}
			sock->unreliableReceiveSequence = sequence + 1;

			length -= NET_HEADERSIZE;

			SZ_Clear (&net_message);
			SZ_Write (&net_message, packetBuffer.data, length);

			ret = 2;
			break;
		}

		if (flags & NETFLAG_ACK)
		{
			if (sequence != (sock->sendSequence - 1))
			{
				Con_PrintLinef ("Stale ACK received");
				continue;
			}
			if (sequence == sock->ackSequence)
			{
				sock->ackSequence++;
				if (sock->ackSequence != sock->sendSequence)
					Con_PrintLinef ("ack sequencing error");
			}
			else
			{
				Con_PrintLinef ("Duplicate ACK received");
				continue;
			}
#ifdef SUPPORTS_SERVER_PROTOCOL_15
			sock->sendMessageLength -= host_protocol_datagram_maxsize;
#endif // SUPPORTS_SERVER_PROTOCOL_15
			if (sock->sendMessageLength > 0)
			{
#ifdef SUPPORTS_SERVER_PROTOCOL_15
				memmove (sock->sendMessage, sock->sendMessage + host_protocol_datagram_maxsize, sock->sendMessageLength);
#endif // SUPPORTS_SERVER_PROTOCOL_15
				sock->sendNext = true;
			}
			else
			{
				sock->sendMessageLength = 0;
				sock->canSend = true;
			}
			continue;
		}

		if (flags & NETFLAG_DATA)
		{
			packetBuffer.length = BigLong(NET_HEADERSIZE | NETFLAG_ACK);
			packetBuffer.sequence = BigLong(sequence);
			sfunc.Write (sock->socket, (byte *)&packetBuffer, NET_HEADERSIZE, &readaddr);

			if (sequence != sock->receiveSequence)
			{
				receivedDuplicateCount++;
				continue;
			}
			sock->receiveSequence++;

			length -= NET_HEADERSIZE;

			if (flags & NETFLAG_EOM)
			{
				if (sock->receiveMessageLength + length > (unsigned int)net_message.maxsize)
				{
					Con_PrintLinef ("Over-sized reliable");
					return -1;
				}
				SZ_Clear(&net_message);
				SZ_Write(&net_message, sock->receiveMessage, sock->receiveMessageLength);
				SZ_Write(&net_message, packetBuffer.data, length);
				sock->receiveMessageLength = 0;

				ret = 1;
				break;
			}

			if (sock->receiveMessageLength + length > sizeof(sock->receiveMessage))
			{
				Con_PrintLinef ("Over-sized reliable");
				return -1;
			}
			memcpy (sock->receiveMessage + sock->receiveMessageLength, packetBuffer.data, length);
			sock->receiveMessageLength += length;
			continue;
		}
	}

	if (sock->sendNext)
		SendMessageNext (sock);

	return ret;
}


static void PrintStats(qsocket_t *s)
{
	Con_PrintLinef ("canSend = %4u   ", s->canSend);
	Con_PrintLinef ("sendSeq = %4u   ", s->sendSequence);
	Con_PrintLinef ("recvSeq = %4u   ", s->receiveSequence);
	Con_PrintLine ();
}

void NET_Stats_f (lparse_t *line)
{
	qsocket_t	*s;

	if (line->count == 1)
	{
		Con_PrintLinef ("unreliable messages sent   = %d", unreliableMessagesSent);
		Con_PrintLinef ("unreliable messages recv   = %d", unreliableMessagesReceived);
		Con_PrintLinef ("reliable messages sent     = %d", messagesSent);
		Con_PrintLinef ("reliable messages received = %d", messagesReceived);
		Con_PrintLinef ("packetsSent                = %d", packetsSent);
		Con_PrintLinef ("packetsReSent              = %d", packetsReSent);
		Con_PrintLinef ("packetsReceived            = %d", packetsReceived);
		Con_PrintLinef ("receivedDuplicateCount     = %d", receivedDuplicateCount);
		Con_PrintLinef ("shortPacketCount           = %d", shortPacketCount);
		Con_PrintLinef ("droppedDatagrams           = %d", droppedDatagrams);
	}
	else if (strcmp(line->args[1], "*") == 0)
	{
		for (s = net_activeSockets; s; s = s->next)
			PrintStats(s);
		for (s = net_freeSockets; s; s = s->next)
			PrintStats(s);
	}
	else
	{
		for (s = net_activeSockets; s; s = s->next)
		{
			if (strcasecmp (line->args[1], s->trueaddress) == 0)
				break;
			if (strcasecmp (line->args[1], s->maskedaddress) == 0)
				break;
		}

		if (s == NULL)
		{
			for (s = net_freeSockets; s; s = s->next)
			{
				if (strcasecmp (line->args[1], s->trueaddress) == 0)
					break;
				if (strcasecmp (line->args[1], s->maskedaddress) == 0)
					break;
			}
		}

		if (s == NULL)
			return;

		PrintStats(s);
	}
}


// recognize ip:port (based on ProQuake)
static const char *Strip_Port (const char *host)
{
	static char	noport[MAX_QPATH_64];
			/* array size as in Host_Connect_f() */
	char		*p;
	int		port;

	if (!host || !*host)
		return host;
	c_strlcpy (noport, host);
	if ((p = strrchr(noport, ':')) == NULL)
		return host;
	if (strchr(p, ']'))
		return host;	//[::] should not be considered port 0
	*p++ = '\0';
	port = atoi (p);
	if (port > 0 && port < 65536 && port != net_hostport)
	{
		net_hostport = port;
		Con_PrintLinef ("Port set to %d", net_hostport);
	}
	return noport;
}



static void Test_Poll(void *unused)
{
	struct qsockaddr clientaddr;
	int		control;
	int		len;
	char	name[32];
	char	address[64];
	int		colors;
	int		frags;
	int		connectTime;
	byte	playerNumber;

	net_landriverlevel = testDriver;

	while (1)
	{
		len = dfunc.Read (testSocket, net_message.data, net_message.maxsize, &clientaddr);
		if (len < (int)sizeof(int))
			break;

		net_message.cursize = len;

		MSG_BeginReading ();
		control = BigLong(*((int *)net_message.data));
		MSG_ReadLong();
		if (control == -1)
			break;
		if ((control & (~NETFLAG_LENGTH_MASK)) !=  (int) NETFLAG_CTL)
			break;
		if ((control & NETFLAG_LENGTH_MASK) != len)
			break;

		if (MSG_ReadByte() != CCREP_PLAYER_INFO)
		{
			Con_PrintLinef ("Unexpected response to Player Info request");
			break;
		}

		playerNumber = MSG_ReadByte();
		c_strlcpy (name, MSG_ReadString());
		colors = MSG_ReadLong();
		frags = MSG_ReadLong();
		connectTime = MSG_ReadLong();
		c_strlcpy (address, MSG_ReadString());

		Con_PrintLinef ("%s", name);
		Con_PrintLinef ("  frags:%3d  colors:%d %d  time:%d", frags, colors >> 4, colors & 0x0f, connectTime / 60);
		Con_PrintLinef ("%s", address);
	}

	testPollCount--;
	if (testPollCount)
	{
		SchedulePollProcedure(&testPollProcedure, 0.1);
	}
	else
	{
		dfunc.Close_Socket (testSocket);
		testInProgress = false;
	}
}

void Test_f (lparse_t *line)
{
	const char	*host;
	int		n;
	int		maxusers = MAX_SCOREBOARD_16;
	struct qsockaddr sendaddr;

	if (testInProgress)
	{
		Con_PrintLinef ("There is already a test/rcon in progress");
		return;
	}

	if (line->count < 2)
	{
		Con_PrintLinef ("Usage: test <host>");
		return;
	}

	host = Strip_Port (line->args[1]);

	if (host && hostCacheCount)
	{
		for (n = 0; n < hostCacheCount; n++)
		{
			if (strcasecmp (host, hostcache[n].name) == 0)
			{
				if (hostcache[n].driver != myDriverLevel)
					continue;
				net_landriverlevel = hostcache[n].ldriver;
				maxusers = hostcache[n].maxusers;
				memcpy (&sendaddr, &hostcache[n].addr, sizeof(struct qsockaddr));
				break;
			}
		}
		if (n < hostCacheCount)
			goto JustDoIt;
	}

	for (net_landriverlevel = 0; net_landriverlevel < net_numlandrivers; net_landriverlevel++)
	{
		if (!net_landrivers[net_landriverlevel].initialized)
			continue;

		// see if we can resolve the host name
		if (dfunc.GetAddrFromName(host, &sendaddr) != -1)
			break;
	}

	if (net_landriverlevel == net_numlandrivers)
	{
		Con_PrintLinef ("Could not resolve %s", host); 	// JPG 3.00 - added error message
		return;
	}

JustDoIt:
	testSocket = dfunc.Open_Socket(0);
	if (testSocket == INVALID_SOCKET) {
		Con_PrintLinef ("Could not open socket");  // JPG 3.00 - added error message
		return;
	}

	testInProgress = true;
	testPollCount = 20;
	testDriver = net_landriverlevel;

	for (n = 0; n < maxusers; n++)
	{
		SZ_Clear(&net_message);
		// save space for the header, filled in later
		MSG_WriteLong(&net_message, 0);
		MSG_WriteByte(&net_message, CCREQ_PLAYER_INFO);
		MSG_WriteByte(&net_message, n);
		*((int *)net_message.data) = BigLong(NETFLAG_CTL | 	(net_message.cursize & NETFLAG_LENGTH_MASK));
		dfunc.Write (testSocket, net_message.data, net_message.cursize, &sendaddr);
	}
	SZ_Clear(&net_message);
	SchedulePollProcedure(&testPollProcedure, 0.1);
}

/* JPG 3.00 - got rid of these.  Just use test vars; only ONE outstanding test of any kind.
static cbool test2InProgress = false;
static int		test2Driver;
static sys_socket_t		test2Socket;
*/

static void Test2_Poll (void *);
static PollProcedure	test2PollProcedure = {NULL, 0.0, Test2_Poll};

static void Test2_Poll (void *unused)
{
	struct qsockaddr clientaddr;
	int		control;
	int		len;
	char	name[256];
	char	value[256];

	net_landriverlevel = testDriver;
	name[0] = 0;

	len = dfunc.Read (testSocket, net_message.data, net_message.maxsize, &clientaddr);
	if (len < (int) sizeof(int))
		goto Reschedule;

	net_message.cursize = len;

	MSG_BeginReading ();
	control = BigLong(*((int *)net_message.data));
	MSG_ReadLong();
	if (control == -1)
		goto Error;
	if ((control & (~NETFLAG_LENGTH_MASK)) !=  (int) NETFLAG_CTL)
		goto Error;
	if ((control & NETFLAG_LENGTH_MASK) != len)
		goto Error;

	if (MSG_ReadByte() != CCREP_RULE_INFO)
		goto Error;

	c_strlcpy (name, MSG_ReadString());
	if (name[0] == 0)
		goto Done;
	c_strlcpy (value, MSG_ReadString());

	Con_PrintLinef ("%-16.16s  %-16.16s", name, value);

	SZ_Clear(&net_message);
	// save space for the header, filled in later
	MSG_WriteLong(&net_message, 0);
	MSG_WriteByte(&net_message, CCREQ_RULE_INFO);
	MSG_WriteString(&net_message, name);
	*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
	dfunc.Write (testSocket, net_message.data, net_message.cursize, &clientaddr);
	SZ_Clear(&net_message);

Reschedule:
	// JPG 3.00 - added poll counter
	testPollCount--;
	if (testPollCount)
	{
		SchedulePollProcedure(&test2PollProcedure, 0.05);
		return;
	}
	goto Done;

Error:
	Con_PrintLinef ("Unexpected response to Rule Info request");

Done:
	dfunc.Close_Socket (testSocket);
	testInProgress = false;
	return;
}

void Test2_f (lparse_t *line)
{
	const char	*host;
	int		n;
	struct qsockaddr sendaddr;

	if (testInProgress)
	{
		Con_PrintLinef ("There is already a test/rcon in progress");
		return;
	}

	if (line->count < 2)
	{
		Con_PrintLinef ("Usage: test2 <host>");
		return;
	}

	host = Strip_Port (line->args[1]);

	if (host && hostCacheCount)
	{
		for (n = 0; n < hostCacheCount; n++)
		{
			if (strcasecmp (host, hostcache[n].name) == 0)
			{
				if (hostcache[n].driver != myDriverLevel)
					continue;
				net_landriverlevel = hostcache[n].ldriver;
				memcpy (&sendaddr, &hostcache[n].addr, sizeof(struct qsockaddr));
				break;
			}
		}

		if (n < hostCacheCount)
			goto JustDoIt;
	}

	for (net_landriverlevel = 0; net_landriverlevel < net_numlandrivers; net_landriverlevel++)
	{
		if (!net_landrivers[net_landriverlevel].initialized)
			continue;

		// see if we can resolve the host name
		if (dfunc.GetAddrFromName(host, &sendaddr) != -1)
			break;
	}

	if (net_landriverlevel == net_numlandrivers)
	{
		Con_PrintLinef ("Could not resolve %s", host);	// JPG 3.00 - added error message
		return;
	}

JustDoIt:
	testSocket = dfunc.Open_Socket (0);
	if (testSocket == INVALID_SOCKET) {
		Con_PrintLinef ("Could not open socket"); // JPG 3.00 - added error message
		return;
	}

	testInProgress = true;				// JPG 3.00 test2InProgress->testInProgress
	testPollCount = 20;					// JPG 3.00 added this
	testDriver = net_landriverlevel;	// JPG 3.00 test2Driver->testDriver

	SZ_Clear(&net_message);
	// save space for the header, filled in later
	MSG_WriteLong(&net_message, 0);
	MSG_WriteByte(&net_message, CCREQ_RULE_INFO);
	MSG_WriteString(&net_message, "");
	*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
	dfunc.Write (testSocket, net_message.data, net_message.cursize, &sendaddr);
	SZ_Clear(&net_message);
	SchedulePollProcedure(&test2PollProcedure, 0.05);
}


int Datagram_Init (void)
{
	int	i, num_inited;
	sys_socket_t csock;

#ifdef BAN_TEST
	banAddr.s_addr = INADDR_ANY;
	banMask.s_addr = INADDR_NONE;
#endif
	myDriverLevel = net_driverlevel;

	if (COM_CheckParm("-nolan"))
		return -1;

	num_inited = 0;
	for (i = 0; i < net_numlandrivers; i++)
		{
		csock = net_landrivers[i].Init ();
		if (csock == INVALID_SOCKET)
			continue;
		net_landrivers[i].initialized = true;
		net_landrivers[i].controlSock = csock;
		net_landrivers[i].listeningSock = INVALID_SOCKET;
		num_inited++;
		}

	if (num_inited == 0)
		return -1;

	return 0;
}


void Datagram_Shutdown (void)
{
	int i;

	Datagram_Listen(false);

// shutdown the lan drivers
	for (i = 0; i < net_numlandrivers; i++)
	{
		if (net_landrivers[i].initialized)
		{
			net_landrivers[i].Shutdown ();
			net_landrivers[i].initialized = false;
		}
	}
}


void Datagram_Close (qsocket_t *sock)
{
	if (sock->isvirtual)
	{
		sock->isvirtual = false;
		sock->socket = INVALID_SOCKET;
	}
	else
		sfunc.Close_Socket (sock->socket);
}


void Datagram_Listen (cbool state)
{
	qsocket_t *s;
	int i;

	heartbeat_time = 0;	//reset it

	for (i = 0; i < net_numlandrivers; i++)
	{
		if (net_landrivers[i].initialized)
		{
			net_landrivers[i].listeningSock = net_landrivers[i].Listen (state);

			for (s = net_activeSockets; s; s = s->next)
			{
				if (s->isvirtual)
				{
					s->isvirtual = false;
					s->socket = INVALID_SOCKET;
				}
			}
		}
	}
}



// ServerControlPacket is called when
// packet is marked with NETFLAG_CTL
// We are not reading anything from the network here, it's already been read
// Return value means nothing.
// Only called by Datagram_GetAnyMessage --> Datagram_ProcessPacket --> us
static void _Datagram_ServerControlPacket (sys_socket_t acceptsock, struct qsockaddr *pclientaddr, byte *data, int length)
{
	struct qsockaddr newaddr;
	int			command;
	int			control;
	const char *ipstring = "FIXME";
	byte		cl_proquake_connection, cl_proquake_version, cl_proquake_flags;	// JPG 3.02 - bugfix!
	int			cl_proquake_password;


	control = BigLong(*((int *)data));

	if (control == -1)
	{
		// Server heartbeat stuff goes here
		char firstword[256];
		const char *cursor; //, *mystring = cursor = "  dpmaster.deathmask.net:27950 , dpmaster.tchr.no:27950  ";
//while ( (cursor = String_Get_Word (cursor, ",", word, sizeof(word))) ) {
//	alert (QUOTED_S, word);
//	cursor=cursor;
//}
		int i;

		if (!sv_public.value)
			return;

		// Server heartbeat stuff goes here
		data[length] = 0;
		cursor = String_Get_Word ((char *)data + 4, " ", firstword, sizeof(firstword));


		if (String_Does_Match_Caseless (firstword, "getinfo") || String_Does_Match_Caseless (firstword, "getstatus"))
		{
			cbool full_reply = String_Does_Match_Caseless (firstword, "getstatus");
			#define SL "\\"
			char cookie[128];

			const char *gamedir = gamedir_shortname();  // Gamedir name
			unsigned int numclients = 0, numbots = 0;

			cursor = String_Get_Word (cursor, " ", firstword, sizeof(firstword)); // Hit it again
			c_strlcpy (cookie, cursor ? firstword : ""); // Copy the word in there otherwise it is blank

			// Count players and bots for the reply
			for (i = 0; i < svs.maxclients_internal; i++) {  // Because the cap can change at any time now.
				if (svs.clients[i].active) {
					numclients++;
					if (!svs.clients[i].netconnection)
						numbots++;
				}
			}

			#define fmt_string  "\\%s\\%s"
			#define fmt_integer "\\%s\\%d"

			SZ_Clear			(&net_message);
			MSG_WriteLong		(&net_message, -1);
			MSG_WriteString		(&net_message, full_reply ? "statusResponse":"infoResponse" NEWLINE); net_message.cursize--;

			cursor = String_Get_Word (com_protocolname.string, ",", firstword, sizeof(firstword)); // Get first word off protocol string

			//the master server needs this. This tells the master which game we should be listed as.
			if (firstword[0])			{ MSG_WriteStringf (&net_message, fmt_string,  "gamename",      firstword			); net_message.cursize--; }
			if (1 /* gameid I guess*/)	{ MSG_WriteStringf (&net_message, fmt_string,  "protocol",		"3"					); net_message.cursize--; } // Spike comment was: this is stupid
			if (1 /* version */ )		{ MSG_WriteStringf (&net_message, fmt_string,  "ver",			"Mark V 0.99.99"	); net_message.cursize--; }
			if (1)						{ MSG_WriteStringf (&net_message, fmt_integer, "nqprotocol", 	sv.protocol			);	net_message.cursize--; }
			if (gamedir[0])				{ MSG_WriteStringf (&net_message, fmt_string,  "modname", 		gamedir				);	net_message.cursize--; }
			if (sv.name[0])				{ MSG_WriteStringf (&net_message, fmt_string,  "mapname", 		sv.name				);	net_message.cursize--; }
			if (pr_deathmatch.string[0]){ MSG_WriteStringf (&net_message, fmt_string,  "deathmatch", 	pr_deathmatch.string	);	net_message.cursize--; }
			if (pr_teamplay.string[0])	{ MSG_WriteStringf (&net_message, fmt_string,  "teamplay", 		pr_teamplay.string		);	net_message.cursize--; }
			if (*hostname.string)		{ MSG_WriteStringf (&net_message, fmt_string,  "hostname", 		hostname.string		);	net_message.cursize--; }
			if (1 /*players playing*/)	{ MSG_WriteStringf (&net_message, fmt_integer, "clients", 		numclients			);	net_message.cursize--; }
			if (numbots)				{ MSG_WriteStringf (&net_message, fmt_integer, "bots", 			numbots				);	net_message.cursize--; }
			if (1 /*maxclients*/)		{ MSG_WriteStringf (&net_message, fmt_integer, "sv_maxclients", svs.maxclients_public); net_message.cursize--; } // Because this public is the public limit
			if (*cookie)				{ MSG_WriteStringf (&net_message, fmt_string,  "challenge", 	cookie				); net_message.cursize--; }

			if (full_reply) {
				for (i = 0; i < svs.maxclients_internal; i++) {  // Because the cap can change at any time now.
					if (svs.clients[i].active) {
						int j;
						float total = 0;
						for (j = 0; j < NUM_PING_TIMES; j++)
							total += svs.clients[i].ping_times[j];
						total /= NUM_PING_TIMES;
						total *= 1000;	//put it in ms

						MSG_WriteStringf (&net_message, NEWLINE "%d %d %d_%d " QUOTED_S,
									svs.clients[i].old_frags,
									(int)total,
									svs.clients[i].colors & 15,
									svs.clients[i].colors / 16 /* >>4 */,
									svs.clients[i].name
						);
						net_message.cursize--;
					}
				}
			}

			dfunc.Write (acceptsock, net_message.data, net_message.cursize, pclientaddr);
			SZ_Clear(&net_message);
		}
		return;
	}
	if ((control & (~NETFLAG_LENGTH_MASK)) !=  (int)NETFLAG_CTL)
		return;
	if ((control & NETFLAG_LENGTH_MASK) != length)
		return;

	//sigh... FIXME: potentially abusive memcpy
	SZ_Clear(&net_message);
	SZ_Write(&net_message, data, length);

	MSG_BeginReading ();
	MSG_ReadLong();

	command = MSG_ReadByte();
	if (command == CCREQ_SERVER_INFO)
	{
		// http://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_8.htm says this is the first thing sent from the client to the server
		// This is also used by the slist command and probably XFire/All Seeing Eye/QView (i.e. a server browser app) and qstat (Quakeserver.nets, Quakeone.com, X
		// HOWEVER, the client seems to get the maxplayers used in-game from SV_SendServerinfo svc_serverinfo.
		// So we should be able to "lie" here and say it isn't 16 but 4, so use _public and not _internal
		if (strcmp(MSG_ReadString(), "QUAKE") != 0)
			return;

		SZ_Clear(&net_message);
		// save space for the header, filled in later
		MSG_WriteLong(&net_message, 0);
		MSG_WriteByte(&net_message, CCREP_SERVER_INFO);
		dfunc.GetSocketAddr(acceptsock, &newaddr);
		MSG_WriteString(&net_message, dfunc.AddrToString(&newaddr, false));
		MSG_WriteString(&net_message, hostname.string);
		MSG_WriteString(&net_message, sv.name);
		MSG_WriteByte(&net_message, net_activeconnections);
		MSG_WriteByte(&net_message, svs.maxclients_public);	// Client uses svc_serverinfo, not this.
		MSG_WriteByte(&net_message, NET_PROTOCOL_VERSION);
		*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		dfunc.Write (acceptsock, net_message.data, net_message.cursize, pclientaddr);
		SZ_Clear(&net_message);

		return;
	}

	if (command == CCREQ_PLAYER_INFO)
	{
		int			playerNumber, activeNumber, clientNumber;
		const char 	*name_display;
		client_t	*client;

		playerNumber = MSG_ReadByte();
		activeNumber = -1;

		for (clientNumber = 0, client = svs.clients; clientNumber < svs.maxclients_internal; clientNumber++, client++) // Because the cap can change at any time now.
		{
			if (client->active)
			{
				activeNumber++;
				if (activeNumber == playerNumber)
					break;
			}
		}

		if (clientNumber == svs.maxclients_internal) // Because the cap can change at any time now.
			return;

		SZ_Clear(&net_message);
		// save space for the header, filled in later
		MSG_WriteLong(&net_message, 0);
		MSG_WriteByte(&net_message, CCREP_PLAYER_INFO);
		MSG_WriteByte(&net_message, playerNumber);

		// If name privacy set, external requests (test command) send "private"
		if (pq_privacy_name.value)
			name_display = "private";
		else name_display = client->name;

		MSG_WriteString(&net_message, name_display);
		MSG_WriteLong(&net_message, client->colors);
		MSG_WriteLong(&net_message, (int)client->edict->v.frags);
		MSG_WriteLong(&net_message, (int)(net_time - client->netconnection->connecttime));

		// 0 - True, 1 - Masked, 2 - Not a damned thing
		switch ((int)pq_privacy_ipmasking.value) {
		case 0:		MSG_WriteString(&net_message, NET_QSocketGetTrueAddressString(client->netconnection)); break; // True
		case 1: 	MSG_WriteString(&net_message, NET_QSocketGetMaskedAddressString(client->netconnection)); break; // Masked
		default:	MSG_WriteString(&net_message, "private"); break; // Private
		}

		*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		dfunc.Write (acceptsock, net_message.data, net_message.cursize, pclientaddr);
		SZ_Clear(&net_message);

		return;
	}

	if (command == CCREQ_RULE_INFO)
	{
		const char	*prevCvarName;
		cvar_t	*var;

		// find the search start location
		prevCvarName = MSG_ReadString();
		var = Cvar_FindAfter (prevCvarName, CVAR_SERVERINFO);

		// send the response
		SZ_Clear(&net_message);
		// save space for the header, filled in later
		MSG_WriteLong(&net_message, 0);
		MSG_WriteByte(&net_message, CCREP_RULE_INFO);
		if (var)
		{
			MSG_WriteString(&net_message, var->name);
			MSG_WriteString(&net_message, var->string);
		}
		*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		dfunc.Write (acceptsock, net_message.data, net_message.cursize, pclientaddr);
		SZ_Clear(&net_message);

		return;
	}

	// JPG 3.00 - rcon
	if (command == CCREQ_RCON)
	{
		char pass[2048];	// 2048 = largest possible return from MSG_ReadString
		char cmd[2048];		// 2048 = largest possible return from MSG_ReadString

#ifdef SUPPORTS_PQ_RCON_FAILURE_BLACKOUT // Baker change
		char rcon_client_ip[NET_NAMELEN_64], *colon; //  me!!!!!
		float attempt_time = Time_Now (); // Not used?
		time_t  ltime;
		time (&ltime);

		c_strlcpy (rcon_client_ip, dfunc.AddrToString(pclientaddr, false));
		if ( (colon = strchr(rcon_client_ip, ':')) ) // Null terminate at colon
			*colon = 0;

#endif // Baker change + SUPPORTS_PQ_RCON_FAILURE_BLACKOUT

		c_strlcpy (pass, MSG_ReadString());
		c_strlcpy (cmd, MSG_ReadString());

		SZ_Clear(&rcon_message);
		// save space for the header, filled in later
		MSG_WriteLong(&rcon_message, 0);
		MSG_WriteByte(&rcon_message, CCREP_RCON);

#ifdef SUPPORTS_PQ_RCON_FAILURE_BLACKOUT // Baker change
		if (Rcon_Blackout (rcon_client_ip, realtime))
			MSG_WriteString(&rcon_message, "rcon ignored: too many failures, wait several minutes and try again");
		else
#endif // Baker change + SUPPORTS_PQ_RCON_FAILURE_BLACKOUT

		if (!*rcon_password.string)
			MSG_WriteString(&rcon_message, "rcon is disabled on this server");
		else if (strcmp(pass, rcon_password.string))
		{

#ifdef SUPPORTS_PQ_RCON_ATTEMPTS_LOGGED // Baker change
			Rcon_Fails_Log (rcon_client_ip, realtime);
			MSG_WriteString(&rcon_message, "rcon incorrect password (attempt logged with ip)");
			Con_PrintLinef ("(%s) rcon invalid password on " QUOTED_S " %s", rcon_client_ip, cmd, ctime( &ltime ) ); //(%s) %s", host_client->netconnection->address,  &text[1]);
#endif // Baker change + SUPPORTS_RCON_ATTEMPTS_LOGGED
		}
		else
		{
#ifdef SUPPORTS_PQ_RCON_ATTEMPTS_LOGGED // Baker change
			MSG_WriteString(&rcon_message, "");
			rcon_active = true;
			Con_PrintLinef ("(%s) Rcon command: " QUOTED_S " %s", rcon_client_ip, cmd, ctime( &ltime ) ); //(%s) %s", host_client->netconnection->address,  &text[1])
			Cmd_ExecuteString (cmd, src_command);
			rcon_active = false;
#endif // Baker change + SUPPORTS_RCON_ATTEMPTS_LOGGED

		}

		*((int *)rcon_message.data) = BigLong(NETFLAG_CTL | (rcon_message.cursize & NETFLAG_LENGTH_MASK));
		dfunc.Write (acceptsock, rcon_message.data, rcon_message.cursize, pclientaddr);
		SZ_Clear(&rcon_message);

		return;
	}

	if (command != CCREQ_CONNECT)
		return;

// MUST BE CCREQ_CONNECT PHASE AT THIS POINT.  EVERYTHING ELSE RETURNED
// MUST BE CCREQ_CONNECT PHASE AT THIS POINT.  EVERYTHING ELSE RETURNED
// MUST BE CCREQ_CONNECT PHASE AT THIS POINT.  EVERYTHING ELSE RETURNED
// WE ALREADY KNOW IF THE CLIENT IS PROQUAKE.  HOW DOES IT KNOW IF THE SERVER IS PROQUAKE?

	if (strcmp (MSG_ReadString(), "QUAKE") != 0)
		return;

	if (MSG_ReadByte() != NET_PROTOCOL_VERSION) {
		Datagram_Reject ("Incompatible version." NEWLINE, acceptsock, pclientaddr);
		return;
	}

// LOCKED_SERVER
#ifdef CORE_PTHREADS
	if (Admin_Check_ServerLock(ipstring)) { // ipstring isn't needed, but if someone is prevented from joining log it
		Datagram_Reject ("Server isn't accepting new players at the moment." NEWLINE, acceptsock, pclientaddr);
		return;
	}
#endif // CORE_PTHREADS

#if 0 // def BAN_TEST
	ipstring = dfunc.AddrToString (pclientaddr);

	// check for a ban
	if (Admin_Check_Ban (ipstring) )
		return Datagram_Reject ("You have been banned." NEWLINE, acceptsock, &clientaddr);

	if (Admin_Check_Whitelist (ipstring) )
		return Datagram_Reject ("You aren't whitelisted.  If you should be and are very new, try again in a minute." NEWLINE, acceptsock, &clientaddr);
#endif

	// see if this guy is already connected
	{
		qsocket_t	*s;
		for (s = net_activeSockets; s; s = s->next)
		{
			int	address_compare; // Baker: was >= 0  0 means SAME IP AND PORT.  1 means SAME IP, different port.  -1 means neither
			if (s->driver == net_driverlevel && !s->disconnected && (address_compare = dfunc.AddrCompare(pclientaddr, &s->addr)) == 0) {
				// is this a duplicate connection request?
				if (address_compare == 0 && net_time - s->connecttime < 2.0) {
					// yes, so send a duplicate reply
					SZ_Clear(&net_message);
					// save space for the header, filled in later
					MSG_WriteLong(&net_message, 0);
					MSG_WriteByte(&net_message, CCREP_ACCEPT);
					dfunc.GetSocketAddr(s->socket, &newaddr);
					MSG_WriteLong(&net_message, dfunc.GetSocketPort(&newaddr));
					Con_DPrintLinef ("Client port on the server is %s", dfunc.AddrToString(&newaddr, false));
					*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
					dfunc.Write (acceptsock, net_message.data, net_message.cursize, pclientaddr);
					SZ_Clear(&net_message);

					return;
				}
				// it's somebody coming back in from a crash/disconnect
				// so close the old qsocket and let their retry get them back in
				//NET_Close(s);  // JPG - finally got rid of the worst mistake in Quake
				//return NULL;
			}
		}
	}

	// Now find out if this is a ProQuake client

	// JPG - support for mods.  Baker: Removed Qsmack check.
#define NO_PASSWORD_NEG_1 -1
	cl_proquake_connection 	= length > 12 ? MSG_ReadByte() : 0;
	cl_proquake_version 	= length > 13 ? MSG_ReadByte() : 0;
	cl_proquake_flags 		= length > 14 ? MSG_ReadByte() : 0;
	cl_proquake_password	= length > 18 ? MSG_ReadLong() : NO_PASSWORD_NEG_1; // Yes.  -1 means missing.

	// Baker: Skipped cheat-free check
	if (pq_password.value > 0) {
		if (cl_proquake_password == NO_PASSWORD_NEG_1) {
			Datagram_Reject ("Password protected server.  Set pq_password to specify the password." NEWLINE "Requires Mark V, ProQuake, DirectQ or other ProQuake compatible client." NEWLINE, acceptsock, pclientaddr);
			return;
		}
		if (cl_proquake_password != pq_password.value) {
			Datagram_Reject ("Password is wrong (pq_password)." NEWLINE, acceptsock, pclientaddr);
			return;
		}
		// We are ok!
	}

	// allocate a QSocket
	{
		qsocket_t	*sock = NULL;
		int			plnum;
		// find a free player slot
		// We now use this method to determine if there is a free slot.
		for (plnum = 0; plnum < svs.maxclients_public; plnum ++) // This is a place where the public limit must be used.
			if (!svs.clients[plnum].active)
				break;

		if (plnum < svs.maxclients_public) // can be false due to bot clients spike says (but requires the bot client extension we don't have implemented?)
			sock = NET_NewQSocket ();

		// no room; try to let him know
		if (sock == NULL) {
			Datagram_Reject("Server is full." NEWLINE, acceptsock, pclientaddr);
			return;
		}

		// everything is allocated, just fill in the details
		sock->isvirtual = true;
		sock->socket = acceptsock;
		sock->landriver = net_landriverlevel;
		sock->addr = *pclientaddr;
		c_strlcpy (sock->trueaddress, dfunc.AddrToString(pclientaddr, false));
		c_strlcpy (sock->maskedaddress, dfunc.AddrToString(pclientaddr, true));

		// send him back the info about the server connection he has been allocated
		SZ_Clear(&net_message);
		// save space for the header, filled in later
		MSG_WriteLong(&net_message, 0);
		MSG_WriteByte(&net_message, CCREP_ACCEPT);
		dfunc.GetSocketAddr(sock->socket, &newaddr);
		MSG_WriteLong(&net_message, dfunc.GetSocketPort(&newaddr)); // OUT: Here is where port is sent
#if 1 // Don't pretend to be a ProQuake server?
		// Spike only sends to ProQuake clients, we send to everyone right?  ProQuake does.
		// This causes Mark V and glpro to fail but why?
		MSG_WriteByte(&net_message, MOD_PROQUAKE_1);	//proquake
		MSG_WriteByte(&net_message, PROQUAKE_SERVER_VERSION_3_30 * 10);//ver 30 should be safe. 34 screws with our single-server-socket stuff.
		MSG_WriteByte(&net_message, 0 /*not cheat-free server*/);
#endif
		if (cl_proquake_connection) {
			sock->proquake_connection = cl_proquake_connection;
			sock->proquake_version	  = cl_proquake_version;
			sock->proquake_flags	  = cl_proquake_flags;
//			Con_DPrintLinef ("A ProQuake client connected reporting as version %d", cl_proquake_version);
		}


		*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		dfunc.Write (acceptsock, net_message.data, net_message.cursize, pclientaddr);
		SZ_Clear(&net_message);

		//spawn the client.
		//FIXME: come up with some challenge mechanism so that we don't go to the expense of spamming serverinfos+modellists+etc until we know that its an actual connection attempt.
		svs.clients[plnum].netconnection = sock;
		SV_ConnectClient (plnum);
	}
}

qsocket_t *Datagram_CheckNewConnections (void)
{
	// Master server stuff goes here
	//only needs to do master stuff now
	if (sv_public.value > 0)
	{
		if (System_DoubleTime() > heartbeat_time)
		{
			//darkplaces here refers to the master server protocol, rather than the game protocol
			//(specifies that the server responds to infoRequest packets from the master)
			char *str = "\377\377\377\377heartbeat DarkPlaces\n";
			char this_master[256];
			const char *cursor = net_masters.string;

			struct qsockaddr addr;
			heartbeat_time = System_DoubleTime() + 300; // Add 5 minutes

			while (  (cursor = String_Get_Word (cursor, ",", this_master, sizeof(this_master)))  ) {
				for (net_landriverlevel = 0; net_landriverlevel < net_numlandrivers; net_landriverlevel++) {
					if (net_landrivers[net_landriverlevel].initialized && dfunc.listeningSock != INVALID_SOCKET) {
						if (dfunc.GetAddrFromName(this_master, &addr) >= 0) {
							if (sv_reportheartbeats.value)
								Con_PrintLinef ("Sending heartbeat to %s", this_master);
							dfunc.Write(dfunc.listeningSock, (byte*)str, strlen(str), &addr);
						} else {
							if (sv_reportheartbeats.value)
								Con_PrintLinef ("Unable to resolve %s", this_master);
						}
					}
				}
			}
		}
	}

	return NULL;
}

static void _Datagram_SendServerQuery(struct qsockaddr *addr)
{
	SZ_Clear(&net_message);
	// save space for the header, filled in later
	MSG_WriteLong(&net_message, 0);
	MSG_WriteByte(&net_message, CCREQ_SERVER_INFO);
	MSG_WriteString(&net_message, "QUAKE");
	MSG_WriteByte(&net_message, NET_PROTOCOL_VERSION);
	*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
	dfunc.Write(dfunc.controlSock, net_message.data, net_message.cursize, addr);
	SZ_Clear(&net_message);
}
static struct
{
	int driver;
	cbool requery;
	struct qsockaddr addr;
} *hostlist;
int hostlist_count;
int hostlist_max;
static void _Datagram_AddPossibleHost(struct qsockaddr *addr)
{
	int u;
	for (u = 0; u < hostlist_count; u++)
	{
		if (!memcmp(&hostlist[u].addr, addr, sizeof(struct qsockaddr)) && hostlist[u].driver == net_landriverlevel)
		{	//we already know about it. it must have come from some other master. don't respam.
			return;
		}
	}
	if (hostlist_count == hostlist_max)
	{
		hostlist_max = hostlist_count + 16;
		hostlist = Z_Realloc(hostlist, sizeof(*hostlist)*hostlist_max);
	}
	hostlist[hostlist_count].addr = *addr;
	hostlist[hostlist_count].requery = true;
	hostlist[hostlist_count].driver = net_landriverlevel;
	hostlist_count++;
}


static cbool _Datagram_SearchForHosts (cbool xmit)
{
	int		ret;
	int		n;
	int		i;
	struct qsockaddr readaddr;
	struct qsockaddr myaddr;
	int		control;
	cbool sentsomething = false;

	dfunc.GetSocketAddr (dfunc.controlSock, &myaddr);
	if (xmit)
	{
		for (i = 0; i < hostlist_count; i++)
			hostlist[i].requery = true;

		SZ_Clear(&net_message);
		// save space for the header, filled in later

		MSG_WriteLong(&net_message, 0);
		MSG_WriteByte(&net_message, CCREQ_SERVER_INFO);
		MSG_WriteString(&net_message, "QUAKE");
		MSG_WriteByte(&net_message, NET_PROTOCOL_VERSION);
		*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		dfunc.Broadcast(dfunc.controlSock, net_message.data, net_message.cursize);
		SZ_Clear(&net_message);
		// Search for hosts
		if (slistScope == SLIST_INTERNET)
		{
			struct qsockaddr masteraddr;
			char *str;
			char this_master[256];
			const char *cursor = net_masters.string;

//			struct qsockaddr addr;
			heartbeat_time = System_DoubleTime() + 300; // Add 5 minutes
			while (  (cursor = String_Get_Word (cursor, ",", this_master, sizeof(this_master)))  ) {
				//alert ("This master [%s]", this_master);
				if (dfunc.GetAddrFromName(this_master, &masteraddr) >= 0) {
					const char *cursor2, *protocol_string = cursor2 = com_protocolname.string;
					char this_protocol[256];
					while ( (cursor2 = String_Get_Word (cursor2, ",", this_protocol, sizeof(this_protocol)))   )  {
						//send a request for each Quake server protocol
						int retcode = 0;
						if (masteraddr.qsa_family == AF_INET6)
							str = va("%c%c%c%cgetserversExt %s %u empty full ipv6"/*\x0A\n"*/, 255, 255, 255, 255, this_protocol, NET_PROTOCOL_VERSION);
						else
							str = va("%c%c%c%cgetservers %s %u empty full"/*\x0A\n"*/, 255, 255, 255, 255, this_protocol, NET_PROTOCOL_VERSION);
						retcode = dfunc.Write (dfunc.controlSock, (byte*)str, strlen(str), &masteraddr);
						// Every other write is ok.
						//if (retcode == SOCKET_ERROR) {
						//	Con_SafePrintLinef ("Master server [%s] may not be reachable for protocol [%s] INET %d", this_master, this_protocol, masteraddr.qsa_family);
						//}
						//else Con_SafePrintLinef ("Master server [%s] WAS GOOD for protocol [%s] INET %d", this_master, this_protocol, masteraddr.qsa_family);
					} // End while each protocol
				} // end if

			}
		}
		sentsomething = true;
	}

	while ((ret = dfunc.Read (dfunc.controlSock, net_message.data, net_message.maxsize, &readaddr)) > 0)
	{
		if (ret < (int) sizeof(int))
			continue;
		net_message.cursize = ret;

		// Con_PrintLinef ("Received reply from %s", dfunc.AddrToString (&readaddr));

		// don't answer our own query
		// Spike: Note: this doesn't really work too well if we're multi-homed.
		// Spike: we should probably just refuse to respond to serverinfo requests while we're scanning (chances are our server is going to die anyway).
		if (dfunc.AddrCompare(&readaddr, &myaddr) >= 0)
			continue;

		// is the cache full?
		if (hostCacheCount == HOSTCACHESIZE)
			continue;

		MSG_BeginReading ();
		control = BigLong(*((int *)net_message.data));
		MSG_ReadLong();
		if (control == -1)
		{
			if (msg_readcount+19 <= net_message.cursize && !strncmp((char*)net_message.data + msg_readcount, "getserversResponse", 18))
			{
				struct qsockaddr addr;
				int i;
				msg_readcount += 18;
				for(;;)
				{
					switch(MSG_ReadByte())
					{
					case '\\':
						memset(&addr, 0, sizeof(addr));
						addr.qsa_family = AF_INET;
						for (i = 0; i < 4; i++)
							((byte*)&((struct sockaddr_in*)&addr)->sin_addr)[i] = MSG_ReadByte();
						((byte*)&((struct sockaddr_in*)&addr)->sin_port)[0] = MSG_ReadByte();
						((byte*)&((struct sockaddr_in*)&addr)->sin_port)[1] = MSG_ReadByte();
						if (!((struct sockaddr_in*)&addr)->sin_port)
							msg_badread = true;
						break;
					case '/':
						memset(&addr, 0, sizeof(addr));
						addr.qsa_family = AF_INET6;
						for (i = 0; i < 16; i++)
							((byte*)&((struct sockaddr_in6*)&addr)->sin6_addr)[i] = MSG_ReadByte();
						((byte*)&((struct sockaddr_in6*)&addr)->sin6_port)[0] = MSG_ReadByte();
						((byte*)&((struct sockaddr_in6*)&addr)->sin6_port)[1] = MSG_ReadByte();
						if (!((struct sockaddr_in6*)&addr)->sin6_port)
							msg_badread = true;
						break;
					default:
						memset(&addr, 0, sizeof(addr));
						msg_badread = true;
						break;
					}
					if (msg_badread)
						break;
					_Datagram_AddPossibleHost(&addr);
					sentsomething = true;
				}
			}
			continue;
		}

		if ((control & (~NETFLAG_LENGTH_MASK)) !=  (int)NETFLAG_CTL)
			continue;
		if ((control & NETFLAG_LENGTH_MASK) != ret)
			continue;

		if (MSG_ReadByte() != CCREP_SERVER_INFO)
			continue;

		MSG_ReadString();
		//dfunc.GetAddrFromName(MSG_ReadString(), &peeraddr); // Spike commented out in R4
		/*if (dfunc.AddrCompare(&readaddr, &peeraddr) != 0)
		{
			char read[NET_NAMELEN];
			char peer[NET_NAMELEN];
			q_strlcpy(read, dfunc.AddrToString(&readaddr), sizeof(read));
			q_strlcpy(peer, dfunc.AddrToString(&peeraddr), sizeof(peer));
			Con_SafePrintLinef ("Server at %s claimed to be at %s", read, peer);
		}*/

		// search the cache for this server
		for (n = 0; n < hostCacheCount; n++)
		{
			if (dfunc.AddrCompare(&readaddr, &hostcache[n].addr) == 0)
				break;
		}

		// is it already there?
		if (n < hostCacheCount)
		{
			if (*hostcache[n].cname)
			continue; // Continue the while loop
		}

		// add it
		hostCacheCount++;
		//alert("%d Added hostCacheCount", hostCacheCount);

		c_strlcpy (hostcache[n].name, MSG_ReadString());
		c_strlcpy (hostcache[n].map, MSG_ReadString());
		hostcache[n].users = MSG_ReadByte();
		hostcache[n].maxusers = MSG_ReadByte();
		if (MSG_ReadByte() != NET_PROTOCOL_VERSION)
		{
			c_strlcpy (hostcache[n].cname, hostcache[n].name);
			hostcache[n].cname[14] = 0;
			c_strlcpy (hostcache[n].name, "*");
			c_strlcat (hostcache[n].name, hostcache[n].cname);
		}
		// Baker: Slist This is where we write the address it.
		memcpy (&hostcache[n].addr, &readaddr, sizeof(struct qsockaddr));
		hostcache[n].driver = net_driverlevel;
		hostcache[n].ldriver = net_landriverlevel;
		c_strlcpy (hostcache[n].cname, dfunc.AddrToString(&readaddr, false));
        //alert ("Server name #%d is " QUOTED_S " and [%s].", n, hostcache[n].cname, hostcache[n].name);
		// check for a name conflict
		for (i = 0; i < hostCacheCount; i++)
		{
			if (i == n)
				continue;
			if (strcasecmp (hostcache[n].cname, hostcache[i].cname) == 0)
			{	//this is a dupe.
				hostCacheCount--;
				break;
			}
			if (strcasecmp (hostcache[n].name, hostcache[i].name) == 0)
			{
				i = strlen(hostcache[n].name);
				if (i < 15 && hostcache[n].name[i-1] > '8')
				{
					hostcache[n].name[i] = '0';
					hostcache[n].name[i+1] = 0;
				}
				else
				{
					hostcache[n].name[i-1]++;
				}
				i = -1;
			}
		}
	}

	if (!xmit)
	{
		n = 4; //should be time-based. meh.
		for (i = 0; i < hostlist_count; i++)
		{
			if (hostlist[i].requery && hostlist[i].driver == net_landriverlevel)
			{
				hostlist[i].requery = false;
				_Datagram_SendServerQuery(&hostlist[i].addr);
				sentsomething = true;
				n--;
				if (!n)
					break;
			}
		}
	}
	return sentsomething;
}

cbool Datagram_SearchForHosts (cbool xmit)
{
	cbool ret = false;
	for (net_landriverlevel = 0; net_landriverlevel < net_numlandrivers; net_landriverlevel++)
	{
		if (hostCacheCount == HOSTCACHESIZE)
			break;
		if (net_landrivers[net_landriverlevel].initialized)
			ret |= _Datagram_SearchForHosts (xmit);
	}
	return ret;
}


static qsocket_t *_Datagram_Connect (struct qsockaddr *serveraddr)
{
	struct qsockaddr readaddr;
	qsocket_t	*sock;
	sys_socket_t newsock;
	int			len; // ProQuake full NAT connect (2/6)
	int			ret;
	int			reps;
	double		start_time;
	int			control;
	const char	*reason;

	newsock = dfunc.Open_Socket (0);
	if (newsock == INVALID_SOCKET)
		return NULL;

	sock = NET_NewQSocket ();
	if (sock == NULL)
		goto ErrorReturn2;

	sock->socket = newsock;
	sock->landriver = net_landriverlevel;

	// connect to the host
	if (dfunc.Connect (newsock, serveraddr) == -1)
		goto ErrorReturn;

	// send the connection request
	Con_SafePrintLinef ("trying...");
	SCR_UpdateScreen ();
	start_time = net_time;

	for (reps = 0; reps < 3; reps++)
	{
		SZ_Clear(&net_message);
		// save space for the header, filled in later
		MSG_WriteLong(&net_message, 0);
		MSG_WriteByte(&net_message, CCREQ_CONNECT);
		MSG_WriteString(&net_message, "QUAKE");
		MSG_WriteByte(&net_message, NET_PROTOCOL_VERSION);

		// ProQuake: I am client and I sent this to server.  Spike is sending 3.00 but we send 5.00 because
		// on a true ProQuake 3.50 server we want the NAT fix.
		MSG_WriteByte(&net_message, MOD_PROQUAKE_1);						// JPG - added this
		MSG_WriteByte(&net_message, PROQUAKE_CLIENT_VERSION_5_00 * 10);		// JPG 3.00 - added this
		MSG_WriteByte(&net_message, 0 /* client flags, never used in any ProQuake ever */);										// JPG 3.00 - added this (flags)
		MSG_WriteLong(&net_message, (int) pq_password.value);				// JPG 3.00 - password protected servers.

		*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		dfunc.Write (newsock, net_message.data, net_message.cursize, serveraddr);
		SZ_Clear(&net_message);
		do
		{
			ret = dfunc.Read (newsock, net_message.data, net_message.maxsize, &readaddr);
			// if we got something, validate it
			if (ret > 0)
			{
				// is it from the right place?
				if (sfunc.AddrCompare(&readaddr, serveraddr) != 0)
				{
					Con_SafePrintLinef ("wrong reply address");
					Con_SafePrintLinef ("Expected: %s | %s", dfunc.AddrToString (serveraddr, false), StrAddr(serveraddr));
					Con_SafePrintLinef ("Received: %s | %s", dfunc.AddrToString (&readaddr, false), StrAddr(&readaddr));
					SCR_UpdateScreen ();
					ret = 0;
					continue;
				}

				if (ret < (int) sizeof(int))
				{
					ret = 0;
					continue;
				}

				net_message.cursize = ret;
				MSG_BeginReading ();

				control = BigLong(*((int *)net_message.data));
				MSG_ReadLong();
				if (control == -1)
				{
					ret = 0;
					continue;
				}
				if ((control & (~NETFLAG_LENGTH_MASK)) !=  (int)NETFLAG_CTL)
				{
					ret = 0;
					continue;
				}
				if ((control & NETFLAG_LENGTH_MASK) != ret)
				{
					ret = 0;
					continue;
				}
			}
		}
		while (ret == 0 && (SetNetTime() - start_time) < 2.5);

		if (ret)
			break;

		Con_SafePrintLinef ("still trying...");
		SCR_UpdateScreen ();
		start_time = SetNetTime();
	}

	if (ret == 0)
	{
		reason = "No Response";
		Con_SafePrintLinef ("%s", reason);
		c_strlcpy (m_return_reason, reason);
		goto ErrorReturn;
	}

	if (ret == -1)
	{
		reason = "Network Error";
		Con_SafePrintLinef ("%s", reason);
		c_strlcpy (m_return_reason, reason);
		goto ErrorReturn;
	}

	len = ret; // JPG - save length for ProQuake connections

	ret = MSG_ReadByte();
	if (ret == CCREP_REJECT)
	{
		reason = MSG_ReadString();
		Con_PrintLinef ("%s", reason);
		c_strlcpy (m_return_reason, reason);
		goto ErrorReturn;
	}

	if (ret == CCREP_ACCEPT)
	{
		// Baker: Here is where we read the port that the server assigned us
		int port = MSG_ReadLong();
		memcpy (&sock->addr, serveraddr, sizeof(struct qsockaddr));

		// Baker: if (port) added by Spike.  This is client code, not server code.
		if (port)	// NETQ 5.2 // spike --- don't change the remote port if the server doesn't want us to. this allows servers to use port forwarding with less issues, assuming the server uses the same port for all clients.
			dfunc.SetSocketPort (&sock->addr, port);

		// Client has received CCREP_ACCEPT meaning client may connect
		// Now find out if this is a ProQuake server ...

		// Does this length stuff still work?

		sock->proquake_connection	= len >  9 ? MSG_ReadByte() : 0;    // ProQuake = 1
		sock->proquake_version		= len > 10 ? MSG_ReadByte() : 0;	// ProQuake server version, like 3.20 or 3.50 (times 10 so 32 or 35)
		sock->proquake_flags		= len > 11 ? MSG_ReadByte() : 0;	// Would be for cheat-free server connection

		if (sock->proquake_connection)
			sock->proquake_connection = sock->proquake_version; // I want to know the version

		// Technically, if proquake_flags flags & 1 then there is 1 more long to read -- the ROT seed.
		// However, we are not supporting cheat-free therefore we will not be reading it.
		// And cheat-free is nigh impossible since I stopped signing ProQuake clients/server @ version 4.00 in 2008
		Con_DPrintLinef ("Client port on the server is %s", dfunc.AddrToString(&sock->addr, false));

		if (sock->proquake_connection)
			Con_DPrintLinef ("Server is ProQuake ? %d", sock->proquake_connection);
	}
	else
	{
		reason = "Bad Response";
		Con_PrintLinef ("%s", reason);
		c_strlcpy (m_return_reason, reason);
		goto ErrorReturn;
	}

	// Baker: WE MUST BE ACCEPT AT THIS POINT.  NON-ACCEPT GOT BOUNCED OUT ABOVE

	dfunc.GetNameFromAddr (serveraddr, sock->trueaddress, sizeof(sock->trueaddress));
	dfunc.GetNameFromAddr (serveraddr, sock->maskedaddress, sizeof(sock->maskedaddress));

	Con_PrintLinef ("Connection accepted");
	sock->lastMessageTime = SetNetTime();

	// If we are connected to a legacy ProQuake server 3.50 or higher
	// We need to use its legacy NAT-fix method.
	if (sock->proquake_connection == MOD_PROQUAKE_1 && sock->proquake_version >= 34) {
		// JPG 3.40 - make NAT work by opening a new socket
		// Do you think this stomps all over stuff Spike wrote?  Verify conclusively.
		sys_socket_t clientsock = dfunc.Open_Socket (0);
		if (clientsock == INVALID_SOCKET /*-1*/)
			goto ErrorReturn;
		dfunc.Close_Socket(newsock);
		sock->socket = newsock = clientsock;
	}

	// switch the connection to the specified address
	// This does NOTHING for UDP, UDP_Connect doesn't do a damn thing
	// Although Loop_Connect does

	if (dfunc.Connect (newsock, &sock->addr) == -1 /* this can never happen*/)
	{
		// Basically this is unreachable.  UDP_Connect always returns 0.  Loop_Connect probably can't fail.
		reason = "Connect to Game failed";
		Con_PrintLinef ("%s", reason);
		c_strlcpy (m_return_reason, reason);

		goto ErrorReturn;
	}

	/*Spike's rant about NATs:
	We sent a packet to the server's control port.
	The server replied from that control port. all is well so far.
	The server is now about(or already did, yay resends) to send us a packet from its data port to our address.
	The nat will (correctly) see a packet from a different remote address:port.
	The local nat has two options here. 1) assume that the wrong port is fine. 2) drop it. Dropping it is far more likely.
	The NQ code will not send any unreliables until we have received the serverinfo. There are no reliables that need to be sent either.
	Normally we won't send ANYTHING until we get that packet.
	Which will never happen because the NAT will never let it through.
	So, if we want to get away without fixing everyone else's server (which is also quite messy),
		the easy way around this dilema is to just send some (small) useless packet to what we believe to be the server's data port.
	A single unreliable clc_nop should do it. There's unlikely to be much packetloss on our local lan (so long as our host buffers outgoing packets on a per-socket basis or something),
		so we don't normally need to resend. We don't really care if the server can even read it properly, but its best to avoid warning prints.
	With that small outgoing packet, our local nat will think we initiated the request.
	HOPEFULLY it'll reuse the same public port+address. Most home routers will, but not all, most hole-punching techniques depend upon such behaviour.
	Note that proquake 3.4+ will actually wait for a packet from the new client, which solves that (but makes the nop mandatory, so needs to be reliable).

	the nop is actually sent inside CL_EstablishConnection where it has cleaner access to the client's pending reliable message.

	Note that we do fix our own server. This means that we can easily run on a home nat. the heartbeats to the master will open up a public port with most routers.
	And if that doesn't work, then its easy enough to port-forward a single known port instead of having to DMZ the entire network.
	I don't really expect that many people will use this, but it'll be nice for the occasional coop game.
	(note that this makes the nop redundant, but that's a different can of worms)
	*/

	m_return_onerror = false;
	return sock;

ErrorReturn:
	NET_FreeQSocket(sock);

ErrorReturn2:
	dfunc.Close_Socket(newsock);
	if (m_return_onerror)
	{
		Key_SetDest (key_menu); m_state = m_return_state; // Baker: A menu keydest needs to know menu item
		m_return_onerror = false;
	}

	return NULL;
}

qsocket_t *Datagram_Connect (const char *host)
{
	qsocket_t *ret = NULL;
	cbool resolved = false;
	struct qsockaddr addr;

	host = Strip_Port (host);
	for (net_landriverlevel = 0; net_landriverlevel < net_numlandrivers; net_landriverlevel++)
	{
		if (net_landrivers[net_landriverlevel].initialized)
		{
			// see if we can resolve the host name
			// Spike -- moved name resolution to here to avoid extraneous 'could not resolves' when using other address families
			if (dfunc.GetAddrFromName(host, &addr) != -1)
			{
				resolved = true;
				if ((ret = _Datagram_Connect (&addr)) != NULL)
					break;
			}
		}
	}
	if (!resolved)
		Con_SafePrintLinef ("Could not resolve %s", host);
	return ret;
}
