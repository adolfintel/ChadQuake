
#ifndef HTTP_CLIENT_SAMPLE
#define HTTP_CLIENT_SAMPLE

#define HTTP_CLIENT_BUFFER_SIZE     8192

#if _MSC_VER > 1400 // Baker: VS 2008
	#define _CRT_SECURE_NO_WARNINGS // Get rid of error messages about secure functions
	#define POINTER_64 __ptr64 // VS2008+ include order involving DirectX SDK can cause this not to get defined PVOID64 stupidity
	#pragma warning(disable:4996) // VS2008+ do not like fileno, stricmp, strdup but we aren't using threads and Microsoft only functions = NO
#endif // Baker

#include <stdio.h>
#include "API/HTTPClient.h"

typedef struct _HTTPParameters
{
    CHAR                    Uri[1024];        
    CHAR                    ProxyHost[1024];  
    UINT32                  UseProxy ;  
    UINT32                  ProxyPort;
    UINT32                  Verbose;
    CHAR                    UserName[64];
    CHAR                    Password[64];
    HTTP_AUTH_SCHEMA        AuthType;

} HTTPParameters;





#endif // HTTP_CLIENT_SAMPLE


