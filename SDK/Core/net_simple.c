/*
Copyright (C) 2015-2015 Baker

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
// net_simple.c -- download


#include "core.h"
#include "core_net_sys.h"
#include "net_simple.h"

#ifdef CORE_PTHREADS

// Meh ...
int Con_Queue_Printf (const char *fmt, ...)  __core_attribute__((__format__(__printf__,1,2)));
int Con_Queue_PrintLinef (const char *fmt, ...)  __core_attribute__((__format__(__printf__,1,2)));

int Con_Printf (const char *fmt, ...)  __core_attribute__((__format__(__printf__,1,2)));
int Con_PrintLinef (const char *fmt, ...)  __core_attribute__((__format__(__printf__,1,2)));

char *IPv4_String_Validated (char *buf, size_t bufsiz, const char *s);
// End meh


#ifdef _WIN32
#define c_sockerrno WSAGetLastError()
#else
#define c_sockerrno errno
#endif

void NonSocket_Error (printline_fn_t my_printline, const char *errString)
{
	my_printline ("%s", errString);
}


void Socket_Error (printline_fn_t printline_fn, const char *eventString, int errCode)
{
#ifdef _WIN32
//	int errCode = WSAGetLastError();

	// ..and the human readable error string!!
	// Interesting:  Also retrievable by net helpmsg 10060
	LPSTR errString = NULL;  // will be allocated and filled by FormatMessage

	int size = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
		 FORMAT_MESSAGE_FROM_SYSTEM, // use windows internal message table
		 0,       // 0 since source is internal message table
		 errCode, // this is the error code returned by WSAGetLastError()
				  // Could just as well have been an error code from generic
				  // Windows errors from GetLastError()
		 0,       // auto-determine language to use
		 (LPSTR)&errString, // this is WHERE we want FormatMessage
							// to plunk the error string.  Note the
							// peculiar pass format:  Even though
							// errString is already a pointer, we
							// pass &errString (which is really type LPSTR* now)
							// and then CAST IT to (LPSTR).  This is a really weird
							// trip up.. but its how they do it on msdn:
							// http://msdn.microsoft.com/en-us/library/ms679351(VS.85).aspx
		 0,                 // min size for buffer
		 0 );               // 0, since getting message from system tables
	//printf( "Error code %d:  %s",  errCode, errString)
	//printline ();
	//printline ();
	// "Message was %d bytes, in case you cared to know this.", size);
	//printline ();

	printline_fn ("Socket Error %d: %s (action = %s)", errCode, errString, eventString);
	LocalFree( errString ) ; // if you don't do this, you will get an
	// ever so slight memory leak, since we asked
	// FormatMessage to FORMAT_MESSAGE_ALLOCATE_BUFFER,
	// and it does so using LocalAlloc
	// Gotcha!  I guess.
#else
	printline_fn ("Socket Error %d: %s (action = %s)", errCode, strerror(errCode), eventString);

#endif

}


///////////////////////////////////////////////////////////////////////////////
//  IPv4 Socket Address Stuff/UnStuff
///////////////////////////////////////////////////////////////////////////////

void Socket_Address_To_String (struct sockaddr_in *sa, char *buf, size_t bufsiz, int *port)
{
	char tmp[SYSTEM_STRING_SIZE_1024];
	int		haddr = ntohl(sa->sin_addr.s_addr); // 4 bytes for ip

	if (port) *port = ntohs(sa->sin_port);
	c_snprintfc (tmp, sizeof(tmp), "%d.%d.%d.%d", (haddr >> 24) & 0xff, (haddr >> 16) & 0xff, (haddr >> 8) & 0xff, haddr & 0xff);
	if (buf) strlcpy (buf, tmp, bufsiz); // tmp is to allow calling this will NULL buffer.
}

void Socket_Address_From_String (struct sockaddr_in *sa, const char *ipstring, int port)
{ // 0.0.0.0 for server IN_ADDRNONE?
	sa->sin_family = AF_INET, sa->sin_addr.s_addr = inet_addr (ipstring), sa->sin_port = htons ((unsigned short)port);
}

const char *IPv4_Get_Local_IP (printline_fn_t my_printline, char *buf, size_t bufsiz)
{
	const char *invalid = "0.0.0.0";
//	static char tmp[IPV4_STRING_MAX_22] = {0};

	char namebuff[SYSTEM_STRING_SIZE_1024]; // MAXHOSTNAMELEN is 256
	in_addr_t addr; // Which is a u_long

	if (gethostname(namebuff, sizeof(namebuff)) == SOCKET_ERROR) {
		Socket_Error (my_printline, "gethostname", c_sockerrno);
		return invalid;
	}
	else {
		struct hostent	*local = NULL;
		namebuff[sizeof(namebuff) - 1] = 0; // NULL terminate it.

		if ( (local = gethostbyname(namebuff)) == NULL) {
			Socket_Error (my_printline, "gethostbyname", c_sockerrno);
			return invalid;
		}

		if (local->h_addrtype != AF_INET) {
			NonSocket_Error (my_printline, "gethostbyname address type not AF_INET (ipv4)");
			return invalid;
		}

		addr = *(in_addr_t *)local->h_addr_list[0];
		addr = ntohl(addr); // Net byte order to host order (big endian int32 to little endian int32, usually ...
	}

	// We have a valid address

	c_snprintfc (buf, bufsiz, "%d.%d.%d.%d", (int)((addr >> 24) & 0xff), (int)((addr >> 16) & 0xff), (int)((addr >> 8) & 0xff), (int)(addr & 0xff));
	return buf;
}


///////////////////////////////////////////////////////////////////////////////
//  Socket Create/Destroy
///////////////////////////////////////////////////////////////////////////////


socket_info_t *SocketA_Destroy (printline_fn_t my_printline, socket_info_t *socka)
{
	closesocket (socka->socket);
	socka->socket = INVALID_SOCKET; // Yeah sure we're freeing it next ...
	return (socka = core_free (socka));
}

socket_info_t *SocketA_Create (printline_fn_t my_printline, const char *ipstring, int port)
{
	socket_info_t info = { INVALID_SOCKET, IPV4_STRING_MAX_22 };

	c_strlcpy (info.ipstring, ipstring);
	info.port = port;

	Socket_Address_From_String (&info.addrinfo, info.ipstring, info.port);

	if ( (info.socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		Socket_Error (my_printline, "socket", c_sockerrno);
		return NULL;
	}

	return core_memdup (&info, sizeof(info));
}

// Only difference between client and server is that server binds to the socket
socket_info_t *SocketA_Create_TCP_Listen (printline_fn_t my_printline, const char *ipstring, int port)
{
	socket_info_t *out = SocketA_Create (my_printline, ipstring, port);

	if (out && bind (out->socket, (struct sockaddr *) &out->addrinfo, sizeof(out->addrinfo) ) ) {
		Socket_Error (my_printline, "bind", c_sockerrno);
		out = SocketA_Destroy (my_printline, out);  // Release
	}
	return out;
}



///////////////////////////////////////////////////////////////////////////////
//  Server
///////////////////////////////////////////////////////////////////////////////

#define NET_SIMP_READ_MAX_
// Fills the receive buffer

int Receive (connection_t *con)
{
	socket_info_t *socka = con->socka;
	int n, numdigits, datalen;  // b15: s3: i4:
	char *cursor, *start;

	socka->readlen = 0, socka->readtype = 0;

	// We will read up to 65535 at a time.
	if ( (socka->readlen = recv (socka->socket, socka->readbuffer, sizeof(socka->readbuffer) - 1, 0 /*flgs*/)) == SOCKET_ERROR) {
		Socket_Error (con->printline_fn, "recv", c_sockerrno);
		return 0;
	}

	// TCP: A zero-sized packet means the client disconnected.
	if (socka->readlen == 0) {
		con->printline_fn ("Client disconnected");
		return 0;
	}

//	Con_Queue_PrintLinef ("Read len %d", socka->readlen);

	socka->readbuffer[socka->readlen] = 0;

	// We have a command, string, number or binary.

	socka->readtype = socka->readbuffer[0];

	if ( !(socka->readtype == 'b' || socka->readtype == 's' ) ) {
		con->printline_fn ("Received data of unknown encoding %c, expected 'b' or 's'", socka->readtype);
		return 0; // Unknown type
	}

	#define MAX_DIGITS_16 16
	datalen = 0;
	for (n = 1, cursor = start = &socka->readbuffer[n] ;
		 n < socka->readlen && n < MAX_DIGITS_16 ;
		 n++, cursor ++)
	{
		if (*cursor == ':')
		{
			*cursor = 0; // Term.
			numdigits = n - 1;  // b24:
			datalen = atoi (start);
			n++;
			break;
		}
	}

	if (!datalen) {
		con->printline_fn ("Illegible encoding specifies no size"); //, socka->readtype);
		return 0; // Illegible message.
	}

	if (datalen > socka->readlen - n) {
		con->printline_fn ("Data length %d is longer than message length %d less offet %d = %d", datalen, socka->readlen, n, socka->readlen - n);
		return 0; // Says data is longer than our message.
	}

	memmove (socka->readbuffer, &socka->readbuffer[n], datalen);

	if (socka->readtype == 's')
		socka->readbuffer[datalen++] = 0; // Null out current position, increase size +1

	socka->readtype = socka->readtype;
	return (socka->readlen = datalen);
}

int Receive_String (connection_t *con)
{
	int ok = Receive (con);

	if (!ok)
		return 0;

	if (con->socka->readtype != 's') {
		con->printline_fn ("Expected a string, received non-string");
		return 0;
	}

	return 1; // I guess?  Alternative is to return "ok".
}

int Send_String (connection_t *con, const char *s)
{
	char outbuffer[65536];
	size_t slen = strlen(s);

	c_snprintf2 (outbuffer, "s%d:%s", (int)slen, s);	// s5:frogs
	Con_PrintLinef ("I Sent: %s",  outbuffer);
	return send (con->socka->socket, outbuffer, strlen(outbuffer), 0);
}

// How will we shut it down?  Kill thread?  Feed it a command ;-)
// PTW32_DLLPORT int PTW32_CDECL pthread_create (pthread_t * tid,
//                            const pthread_attr_t * attr,
  //                          void *(PTW32_CDECL *start) (void *),
//                            void *arg);

void *Server_Connection_Async (void *con_)
{
    connection_t *con = con_; // gcc complained
	con->printline_fn ("Client connected: %p %s %d", con, con->socka->ipstring, (int)con->socka->port);
//	recv

	while (Receive_String (con))
	{
		if (!con->socka->readtype == 's')
			break; // Expecting string

		Con_Queue_PrintLinef ("Received: %s", con->socka->readbuffer);

		/// Send // Something.
	}

	Con_Queue_PrintLinef ("Closing connection %p", con);
	con->socka = SocketA_Destroy (con->printline_fn, con->socka);
	con = core_free (con);
	return NULL;
}



void *Server_Async (void *con_)
{
    connection_t *con = con_;
	if (con->notify_socket)
		*(con->notify_socket) = con->socka->socket;

	while (1)
	{
		socket_info_t new_socka = { INVALID_SOCKET, IPV4_STRING_MAX_22 };
		new_socka.socket = accept(con->socka->socket, (struct sockaddr *)&new_socka.addrinfo, &new_socka.addrsize );  // int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)

		if (new_socka.socket == INVALID_SOCKET) {
			Socket_Error (con->printline_fn, "accept", c_sockerrno);
			break; // Let's break out
		}

		if (new_socka.socket != INVALID_SOCKET) {
			pthread_t client_thread;
			connection_t *forked_connection;
			char prepared_ip_string[16];

			Socket_Address_To_String (&new_socka.addrinfo, new_socka.ipstring, sizeof(new_socka.ipstring), &new_socka.port);

			// 192, 127, 169, 10 addresses won't be checked, since validation doesn't pass
			if (IPv4_String_Validated (prepared_ip_string, sizeof(prepared_ip_string), new_socka.ipstring)) {
				if (con->whitelist_fn && !con->whitelist_fn (new_socka.ipstring)) {
					con->printline_fn ("Rejected new connection %s because not on approved list", prepared_ip_string);
					closesocket (new_socka.socket); // Don't know who you are.
					continue;
				}
			}

			Con_Queue_PrintLinef ("Accepted");


			// Thread will be responsible for freeing both of these.
			forked_connection = core_memdup (con, sizeof(*con));
			forked_connection->socka = core_memdup (&new_socka, sizeof(new_socka) );

			con->printline_fn ("Socket %d port %d:  New conn %p on %d port %d", con->socka->socket, (int) con->socka->port, forked_connection->socka,
				forked_connection->socka->socket, (int) forked_connection->socka->port);
			pthread_create (&client_thread, NULL /* no attributes */, Server_Connection_Async, (void *)forked_connection );

			continue;
		}
	}

	// UNREACHABLE.  RIGHT?  Need to define what could happen to cause this to need to exit.

	if (con->notify_socket)
		*(con->notify_socket) = INVALID_SOCKET; // Notify main thread.
	con->printline_fn ("Server closing");

	con->socka = SocketA_Destroy (con->printline_fn, con->socka);
	con = core_free (con);
	return NULL; //
}

void Net_Simple_Server_Force_Shutdown (sys_socket_t notify_socket)
{
	// Should cause accept to fail.
	closesocket (notify_socket);
	// Hope!
}

cbool Net_Simple_Server_Async (const char *_ipstring, int port, const char *basedir, errorline_fn_t error_fn, printline_fn_t printline_fn, printline_fn_t dprint_fn, whitelist_fn_t whitelist_fn, volatile sys_socket_t *notify_socket)
{
	char localipbuf[IPV4_STRING_MAX_22];
	const char *ipstring = _ipstring ? _ipstring : IPv4_Get_Local_IP (printline_fn, localipbuf, sizeof(localipbuf));
	socket_info_t *socka = SocketA_Create_TCP_Listen (printline_fn, ipstring, port);
	connection_t con_ = {socka, error_fn, printline_fn, dprint_fn, whitelist_fn, notify_socket};
	pthread_t server_thread;

    if (!socka)
		return false;

	if (listen (socka->socket, SOMAXCONN /*MAX_CLIENTS_NUM_16*/) == SOCKET_ERROR) {
		Socket_Error (printline_fn,  "listen", c_sockerrno);
        socka = SocketA_Destroy (printline_fn, socka);
		return false;
	}

	//IPv4_Get_Local_IP_Temp ();
	Socket_Address_To_String (&socka->addrinfo, NULL, 0, NULL);

	printline_fn ("Created server @ %s port %d", ipstring, port);

	// Spin us up a thread ...
	c_strlcpy (con_.basedir, basedir);
	pthread_create (&server_thread, NULL /* no attributes */, Server_Async, (void *)core_memdup (&con_, sizeof(con_) )   );

	return socka->socket;
}

///////////////////////////////////////////////////////////////////////////////
//  Client ... foreground only right now.
///////////////////////////////////////////////////////////////////////////////

cbool Net_Simple_Client (const char *_ipstring, int port, const char *basedir, errorline_fn_t errorline_fn, printline_fn_t my_printline, printline_fn_t dprintline_fn)
{
	//char localipbuf[IPV4_STRING_MAX_22];
	const char *ipstring = _ipstring ? _ipstring : "0.0.0.0"; // IPv4_Get_Local_IP (my_printline, localipbuf, sizeof(localipbuf)); // "0.0.0.0";
	socket_info_t *socka = SocketA_Create (my_printline, ipstring, 0); //IPv4_Get_Local_IP(my_printline, localipbuf, sizeof(localipbuf)) /*  ipstring*/, 0 /*port*/);
	connection_t con_ = {socka, errorline_fn, my_printline, dprintline_fn, NULL, NULL};
	//pthread_t client_thread;

    if (!socka)
		return false;

	Socket_Address_From_String (&socka->addrinfo, _ipstring, port);

	Socket_Address_To_String (&socka->addrinfo, NULL, 0, NULL);

	if (connect (socka->socket, (struct sockaddr *) &socka->addrinfo, sizeof(socka->addrinfo) ) == SOCKET_ERROR) {
		Socket_Error (my_printline,  "connect", c_sockerrno);
		socka = SocketA_Destroy (my_printline, socka);
		return false;
	}

	// Spin us up a thread ...
	c_strlcpy (con_.basedir, basedir);
	//send
//	pthread_create (&server_thread, NULL /* no attributes */, Server_Async, (void *)c_memdup (&server, sizeof(server) )   );
//#endif
	{
		int i;
		for (i = 0; i < 4; i ++)
		{
			Send_String (&con_, "Hello");
		}
	}

	socka = SocketA_Destroy (my_printline, socka);
// What about rest of self-destruct
	return true;
}

#endif // CORE_PTHREADS


