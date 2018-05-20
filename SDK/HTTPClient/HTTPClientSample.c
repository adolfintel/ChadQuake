

///////////////////////////////////////////////////////////////////////////////
//
// Module Name:                                                                
//   Sample.c                                                                  
//                                                                             
// Abstract: Demonstrate the HTTP API usage                                    
// Author:	 Eitan Michaelson                                                  
// Platform: Win32                                                             
//
///////////////////////////////////////////////////////////////////////////////

#ifndef CORE_LIBCURL

#include "HTTPClientSample.h"

#ifdef _WIN32
	#pragma comment (lib, "wsock32.lib")
#endif // _WIN32

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPOSInit
// Purpose      : Win32 Socket Init
// Returns      : UINT32 - status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
UINT32 HTTPOSInit(UINT32 nState // Requested operation        
                  // nState could be:
                  // 0 = Shutdown sockets       
                  // 1 = Turn on sockets        
                  )
{
    // Windows Specific - Sockets initialization 
    unsigned short      wVersionRequested;
    WSADATA             wsaData;
    UINT32              nErr = 0;

    // We want to use Winsock v 1.2 (can be higher)
    wVersionRequested = MAKEWORD(2, 2 );

    if(nState > 0) // Initialize Winsock
    {
        nErr = WSAStartup( wVersionRequested, &wsaData );
    }
    else
    {
        // Windows sockets cleanup 
        WSACleanup();
    }    
    return nErr;
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPDebug
// Purpose      : HTTP API Debugging callback
// Gets         : arguments
// Returns      : UINT32
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

VOID HTTPDebug(const CHAR* FunctionName,const CHAR *DebugDump,UINT32 iLength,CHAR *DebugDescription,...) // Requested operation
{

    va_list            pArgp;
    char               szBuffer[2048];

    memset(szBuffer,0,2048);
    va_start(pArgp, DebugDescription);
    vsprintf((char*)szBuffer, DebugDescription, pArgp); //Copy Data To The Buffer
    va_end(pArgp);

    printf("%s %s %s\n", FunctionName,DebugDump,szBuffer);
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : 
// Purpose      : 
// Gets         : 
// Returns      : 
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

void HTTPDumpHelp(CHAR *ExtraInfo)
{

    printf("\nUsage: HTTPClient [/H:Host name] [/R:Proxy host name (Host:Port)] \n"\
        "\t[/C:Credentials (User@Password)] [/A:Authentication type (b or d)]\n");
    printf("For example:\n HTTPClient /H:http://www.myhost.com:82 /R:www.myproxy.com:8080 /C:john@qwerty /A:b\n [/V]");
    printf("\tWill get http://www.myhost.com on TCP port 82 using the myproxy.com amd basic authentication\n\n");

    if(ExtraInfo != NULL)
    {

        printf("%s\n\n",ExtraInfo);

    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : 
// Purpose      : 
// Gets         : 
// Returns      : 
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

INT32 HTTPParseCommandLineArgs(UINT32 argc, CHAR *argv[],HTTPParameters *pClientParams)
{

    UINT32                  nArg;
    UINT32                  nResult;
    CHAR                    *pSearchPtr = NULL;
    CHAR                    PortNum[64];

    // Check for minimal input from the user
    if(argc <= 1)
    {
        HTTPDumpHelp("Error: did not get key parameters.");
        return -1;
    };

    // Parse the arguments
    for(nArg = 1;nArg < argc; nArg++)
    {

        // Did we got a request for help?
        if(strncasecmp(argv[nArg],"/?",2) == 0)
        {
            HTTPDumpHelp(NULL);
            return -1;
        }

        if(strncasecmp(argv[nArg],"/V",2) == 0)
        {
            pClientParams->Verbose = TRUE;
            continue;
        }

        // Did we got the Host name parameter
        if(strncasecmp(argv[nArg],"/H:",3) == 0)
        {
            strcpy(pClientParams->Uri,argv[nArg] + 3);
            continue;
        }

        // Did we got the Proxy parameter
        if(strncasecmp(argv[nArg],"/R:",3) == 0)
        {
            strcpy(pClientParams->ProxyHost,argv[nArg] + 3);
            pClientParams->UseProxy = TRUE; // So we would know later that we have to use proxy
            // Do we have the port name within the input string?
            pSearchPtr = strstr(pClientParams->ProxyHost,":");
            if(pSearchPtr)
            {
                PortNum[0] = 0; // Reset the string
                nResult = (int)(pSearchPtr - pClientParams->ProxyHost); // Look for the offest in bytes
                strcpy(PortNum,pClientParams->ProxyHost + nResult + 1); // copy to temporary buffer
                pClientParams->ProxyHost[nResult] = 0;    // null terminate the host string
                pClientParams->ProxyPort = atol(PortNum); // convert the port to a numeric value
            }
            else
            {
                // Simply use commonly ussed proxy port
                pClientParams->ProxyPort = 8080;
            }
            continue;                
        }

        // Do we have the credentrials?
        if(strncasecmp(argv[nArg],"/C:",3) == 0)
        {
            strcpy(pClientParams->UserName,argv[nArg] + 3);
            // look for the password\user name sepaerator
            pSearchPtr = strstr(pClientParams->UserName,":");
            if(!pSearchPtr)
            {
                HTTPDumpHelp("Error: /C argument must be in the form of user:password");
                return -1;
            }
            nResult = (int)(pSearchPtr - pClientParams->UserName); // Look for the offest in bytes
            strcpy(pClientParams->Password,pClientParams->UserName + nResult +1);
            pClientParams->UserName[nResult] = 0;    // null terminate the host string
            continue;
        }
        // Do we have the authentication method?
        if(strncasecmp(argv[nArg],"/A:",3) == 0)
        {
            pSearchPtr = argv[nArg] +3; 
            if(*pSearchPtr == 'b' || *pSearchPtr == 'B')
            {
                pClientParams->AuthType = AuthSchemaBasic; 
                continue;
            }
            if(*pSearchPtr == 'd' || *pSearchPtr == 'D')
            {
                pClientParams->AuthType = AuthSchemaDigest;
                continue;
            }
            if(pClientParams->AuthType == AuthSchemaNone)
            {
                HTTPDumpHelp("Error: /A argument must be 'b' (for basic) or 'd' (for digest)");
                return -1;
            }
        }
    }

    // The host name is a mandatory parameter
    if(strlen(pClientParams->Uri) == 0)
    {
        HTTPDumpHelp("Error: /H argument is missing");
        return -1;
    }
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// Function     : 
// Purpose      : 
// Gets         : 
// Returns      : 
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////


#ifdef QUAKE_GAME
#undef strncasecmp
#include "core.h"
#include "download_procs.h"
int http_client (download_x_t *download) // We jam argc and argv into there
#else
int main(int argc, CHAR *argv[])
#endif
{

	INT32                   nRetCode;
    UINT32                  nSize,nTotal = 0;
    HTTPParameters          ClientParams;
    CHAR                    Buffer[HTTP_CLIENT_BUFFER_SIZE];
    HTTP_SESSION_HANDLE     pHTTP;
#ifdef QUAKE_GAME
	int						count = 0;
#endif

#ifdef _WIN32
    // OS specific call to start Winsock
    HTTPOSInit(1);
#endif

    do
    {

#ifdef _HTTP_DEBUGGING_
        HTTPClientSetDebugHook(pHTTP,&HTTPDebug);
#endif

        printf("\nHTTP Client v1.0\n\n");
        // Reset the parameters structure
        memset(&ClientParams,0,sizeof(HTTPParameters));

        // Parse the user command line arguments
        nRetCode = HTTPParseCommandLineArgs(download->argbuckets.argcount, download->argbuckets.argvs, &ClientParams);

        if(nRetCode == -1)
        {
            // Problem while parsing command arguments
            return -1;
        }

        // Open the HTTP request handle
        pHTTP = HTTPClientOpenRequest(0);

        // Set the Verb
        if((nRetCode = HTTPClientSetVerb(pHTTP,VerbGet)) != HTTP_CLIENT_SUCCESS)
        {
            break;
        }

        // Set authentication
        if(ClientParams.AuthType != AuthSchemaNone)
        {
            if((nRetCode = HTTPClientSetAuth(pHTTP,ClientParams.AuthType,NULL)) != HTTP_CLIENT_SUCCESS)
            {
                break;
            }

            // Set authentication
            if((nRetCode = HTTPClientSetCredentials(pHTTP,ClientParams.UserName,ClientParams.Password)) != HTTP_CLIENT_SUCCESS)
            {
                break;
            }
        }

        // Use Proxy server
        if(ClientParams.UseProxy == TRUE)
        {
            if((nRetCode = HTTPClientSetProxy(pHTTP,ClientParams.ProxyHost,(unsigned short)ClientParams.ProxyPort,NULL,NULL)) != HTTP_CLIENT_SUCCESS)
            {

                break;
            }
        }

        // Send a request for the home page 
		// Baker: This needs a timeout.  Yes, it does need a timeout.  Try an invalid url, heh.
		// 2nd to last parameter is a timeout.  Don't know what we would set
		// Why did I say that?  There is a timeout later of 3.
        if((nRetCode = HTTPClientSendRequest(pHTTP, ClientParams.Uri, download->user_agent, NULL, 0, FALSE, 3, 0)) != HTTP_CLIENT_SUCCESS)
        {
            break;
        }

        // Retrieve the the headers and analyze them
        if((nRetCode = HTTPClientRecvResponse(pHTTP, 3, &download->expected_size, &download->http_status_code)) != HTTP_CLIENT_SUCCESS)
        {
            break;
        }

		// 100s - Informational  (100: continue, 101 switching protocols, 102 processing)
		// 200s - ok.  And friends.  I think 200 is primary?
		// 300s - 302 = found, 300 = multiple choices (bad for us), 301 = moved, others.
		// 400s - 400 bad request, 401 unauthorized, 402 payment reqd?, 403 = forbidden, 404 = not found, etc.
		// 500s - Internal server error and friends. 502 bad gateway

		// Baker:
		if ( in_range(400, download->http_status_code, 499)) {
			// Error codes like 404 etc.
			//nRetCode = 0; 
			nRetCode = download->http_status_code;
			goto http_status_error_code_baker;
		}

		if (download->out_expected_size) // 404 etc.
			*(download->out_expected_size) = download->expected_size; // Relay back to main thread if applicable.

        // Get the data until we get an error or end of stream code
        // printf("Each dot represents %d bytes:\n",HTTP_BUFFER_SIZE );
        while(nRetCode == HTTP_CLIENT_SUCCESS || nRetCode != HTTP_CLIENT_EOS)
        {
            // Set the size of our buffer
            nSize = HTTP_CLIENT_BUFFER_SIZE;   

            // Get the data
            nRetCode = HTTPClientReadData(pHTTP,Buffer,nSize,0,&nSize);
			if (Download_HTTP_Write (download, Buffer, nSize, 1))
				break; // User cancelled
        }

http_status_error_code_baker: // Baker added goto label only
        HTTPClientCloseRequest(&pHTTP);

    } while(0); // Run only once

    if(ClientParams.Verbose == TRUE)
    {
        printf("\n\nHTTP Client terminated %d (got %d kb)\n\n",(int)nRetCode,(int)(nTotal/ 1024));
    }

	#ifdef _WIN32
		// OS specific call to close Winsock
		HTTPOSInit(0);
	#endif

#if 1 // Nov 2016 fix.
	if (nRetCode && download->out_expected_size == 0) {
		HTTPClientCloseRequest(&pHTTP);
	}
#endif // Nov 2016 fix.

    return (download->exit_code = nRetCode);

}

#endif // !CORE_LIBCURL

