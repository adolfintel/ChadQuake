
#ifndef HTTP_CLIENT_WRAPPER
#define HTTP_CLIENT_WRAPPER


///////////////////////////////////////////////////////////////////////////////
//
// Section      : Microsoft Windows Support
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

#if _MSC_VER > 1400 // Baker: VS 2008
	#define _CRT_SECURE_NO_WARNINGS // Get rid of error messages about secure functions
	#define POINTER_64 __ptr64 // VS2008+ include order involving DirectX SDK can cause this not to get defined PVOID64 stupidity
	#pragma warning(disable:4996) // VS2008+ do not like fileno, stricmp, strdup but we aren't using threads and Microsoft only functions = NO
#endif // Baker

#include	<stdlib.h>
#include	<string.h>
#include	<memory.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<time.h>

#include	<winsock.h>

// Sockets (Winsock wrapper)
#define		HTTP_ECONNRESET     (WSAECONNRESET)
#define		HTTP_EINPROGRESS    (WSAEINPROGRESS)
#define		HTTP_EWOULDBLOCK    (WSAEWOULDBLOCK)

// Kluge alert: redefining strncasecmp() as memicmp() for Windows.
//

#define ASCII_A_65					65
#define ASCII_Z_90					90

#define		strncasecmp			memicmp
#define		strcasecmp			stricmp


#else // Non Win32 : GCC Linux

#include	<unistd.h>
#include	<errno.h>
#include	<pthread.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	<time.h>
#include	<sys/socket.h>
#include	<sys/un.h>
#include	<netinet/in.h>
#include	<netinet/tcp.h>
#include	<netdb.h>
#include	<arpa/inet.h>
#include	<sys/ioctl.h>
#include	<errno.h>
#include	<stdarg.h>

#define		SOCKET_ERROR			-1

// Sockets (Winsock wrapper)
#define		HTTP_EINPROGRESS    (EINPROGRESS)
#define		HTTP_EWOULDBLOCK    (EWOULDBLOCK)

// Generic types
typedef unsigned long                UINT32;
typedef long                         INT32; 

#endif	// #ifdef _HTTP_BUILD_WIN32

// Note: define this to prevent timeouts while debugging.
// #define							 NO_TIMEOUTS

///////////////////////////////////////////////////////////////////////////////
//
// Section      : Functions that are not supported by the AMT stdc framework
//                So they had to be specificaly added.
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus 
extern "C" { 
#endif

    // STDC Wrapper implimentation
    int                                 HTTPWrapperIsAscii              (int c);
    int                                 HTTPWrapperToUpper              (int c);
    int                                 HTTPWrapperToLower              (int c);
    int                                 HTTPWrapperIsAlpha              (int c);
    int                                 HTTPWrapperIsAlNum              (int c);
    char*                               HTTPWrapperItoa                 (char *buff,int i);
    void                                HTTPWrapperInitRandomeNumber    ();
    long                                HTTPWrapperGetUpTime            ();
    int                                 HTTPWrapperGetRandomeNumber     ();
    int                                 HTTPWrapperGetSocketError       (int s);
    unsigned long                       HTTPWrapperGetHostByName        (char *name,unsigned long *address);
    int                                 HTTPWrapperShutDown             (int s,int in);  
    // SSL Wrapper prototypes
    int                                 HTTPWrapperSSLConnect           (int s,const struct sockaddr *name,int namelen,char *hostname);
    int                                 HTTPWrapperSSLNegotiate         (int s,const struct sockaddr *name,int namelen,char *hostname);
    int                                 HTTPWrapperSSLSend              (int s,char *buf, int len,int flags);
    int                                 HTTPWrapperSSLRecv              (int s,char *buf, int len,int flags);
    int                                 HTTPWrapperSSLClose             (int s);
    int                                 HTTPWrapperSSLRecvPending       (int s);
    // Global wrapper Functions
#define                             IToA                            HTTPWrapperItoa
#define                             GetUpTime                       HTTPWrapperGetUpTime
#define                             SocketGetErr                    HTTPWrapperGetSocketError 
#define                             HostByName                      HTTPWrapperGetHostByName
#define                             InitRandomeNumber               HTTPWrapperInitRandomeNumber
#define                             GetRandomeNumber                HTTPWrapperGetRandomeNumber

#ifdef __cplusplus 
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Section      : Global type definitions
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

#define VOID                         void
#ifndef NULL
#define NULL                         0
#endif
#define TRUE                         1
#define FALSE                        0
typedef char                         CHAR;
typedef unsigned short               UINT16;
typedef int                          BOOL;
//typedef unsigned long                ULONG;

// Global socket structures and definitions
#define                              HTTP_INVALID_SOCKET (-1)
typedef struct sockaddr_in           HTTP_SOCKADDR_IN;
typedef struct timeval               HTTP_TIMEVAL; 
typedef struct hostent               HTTP_HOSTNET;
typedef struct sockaddr              HTTP_SOCKADDR;
typedef struct in_addr               HTTP_INADDR;


#endif // HTTP_CLIENT_WRAPPER
