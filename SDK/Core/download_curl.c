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
// download_curl.c -- download functions


#define CORE_LOCAL
#include "core.h"
#include "download_procs.h"
#ifdef CORE_LIBCURL
#define HTTP_ONLY  // A curl define to prevent other protocols.
#define CURL_STATICLIB
#include <curl/curl.h>


#pragma comment (lib, "delayimp.lib")
#pragma comment (lib, "libcurl.lib")


static void scurl_shutdown (CURL **pcurl)
{
	if (*pcurl)
	{
		curl_easy_cleanup(*pcurl); // Cleanup
		curl_global_cleanup(); // Cleanup
		*pcurl = NULL;
	}

	if (g_updatefn) g_updatefn = NULL;
}





static void scurl_startup (CURL **pcurl, progress_fn_t updatefn)
{
	scurl_shutdown (pcurl); // Ensure it is shutdown first

	// Common initialization
	curl_global_init(CURL_GLOBAL_ALL);
	*pcurl = curl_easy_init();

	curl_easy_setopt(*pcurl, CURLOPT_USERAGENT, g_user_agent);// *)  Make up a user agent
	curl_easy_setopt(*pcurl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(*pcurl, CURLOPT_MAXREDIRS, 2 );

	if (updatefn && (g_updatefn = updatefn) )
	{
		curl_easy_setopt(*pcurl, CURLOPT_PROGRESSFUNCTION, download_procs_progress);
		curl_easy_setopt(*pcurl, CURLOPT_NOPROGRESS, 0);
	}
}


///////////////////////////////////////////////////////////////////////////////
//  DOWNLOAD COMMON: Baker -- common operations shared in various main funcs
///////////////////////////////////////////////////////////////////////////////


int Download_Query (const char *www_url)
{
	CURL *hcurl = NULL;
	memstruct_t chunk = {malloc(1), 0};
	double ret_size_of_file = -1;
	int curl_code, http_code;

	// Fill in curl request
	scurl_startup (&hcurl, NULL);
	curl_easy_setopt(hcurl, CURLOPT_URL, www_url);								// *)  Specify url
	curl_easy_setopt(hcurl, CURLOPT_WRITEFUNCTION, download_procs_memwrite);	// *)  Send all data to this function
	curl_easy_setopt(hcurl, CURLOPT_WRITEDATA, &chunk);							// *)  Pass chunk struct to callback

	curl_easy_setopt(hcurl, CURLOPT_NOBODY, 1);									// *)  HEAD REQUEST
	curl_easy_setopt(hcurl, CURLOPT_FILETIME, 1);								// *)  Get file time.  You have to ask!

	// Execute curl task
	if ( (curl_code = curl_easy_perform(hcurl)) == CURLE_OK )
	{
		curl_easy_getinfo (hcurl, CURLINFO_RESPONSE_CODE, &http_code);

		switch (http_code) // Determine response code
		{
		case 200:
			curl_easy_getinfo(hcurl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &ret_size_of_file);
//			curl_easy_getinfo(hcurl, CURLINFO_FILETIME, &fileTime); // long
			break;
		default:
			break;
		}	
	}

	core_free (chunk.memory);
	scurl_shutdown (&hcurl);

	return (int)ret_size_of_file;
}

int sDownload_Prep (const char *www_url, const char *path_to_file)
{
	// Run a head request
	int expected_size = Download_Query (www_url);

	// Size of -1 indicates cannot get file
	if (expected_size == -1)
		return -1;

	// If successful and there is a file prepare it
	if (path_to_file)
	{
		File_Delete (path_to_file);
		File_Mkdir_Recursive (path_to_file);
	}
	return expected_size;
}

///////////////////////////////////////////////////////////////////////////////
//  DOWNLOAD MAIN FUNCTIONS: Baker -- Entry points
///////////////////////////////////////////////////////////////////////////////


cbool Download_To_File (const char *www_url, const char *path_to_file, progress_fn_t update_fn, printline_fn_t my_printline, int *setsize)
{
	// Run prep
	int expected_size = sDownload_Prep (www_url, path_to_file);
	int actual_size;
	cbool ismatch;

	// Size of -1 indicates cannot get file
	if (my_printline) my_printline ("Expected filesize of download is %d", expected_size);
	if (expected_size != -1)
	{
		CURL *hcurl = NULL;

		if (setsize) *setsize = expected_size;

		// Fill in curl request
		scurl_startup (&hcurl, update_fn);
		curl_easy_setopt(hcurl, CURLOPT_URL, www_url);
		curl_easy_setopt(hcurl, CURLOPT_FILE, path_to_file);
		curl_easy_setopt(hcurl, CURLOPT_WRITEFUNCTION, download_procs_filewrite);
		curl_easy_setopt(hcurl, CURLOPT_USERAGENT, g_user_agent); 
		
		// Execute curl task, then shut it down.
		curl_easy_perform(hcurl);
		scurl_shutdown (&hcurl);

		actual_size = File_Length (path_to_file);
		ismatch = (actual_size == expected_size);
		if (my_printline) my_printline ("Actual size %d, expected size %d, match? %d", actual_size, expected_size, ismatch );

		return ismatch;  // Right?  What if aborted?
	} 
	
	return false;
}


byte *Download_To_Memory_Alloc (const char *www_url, size_t *len, progress_fn_t updatefn)
{
	// Run prep
	int expected_size = sDownload_Prep (www_url, NULL);
alert ("need EXTRA_BYTE_NULL_ASSURANCE_ALLOC_+_1");
	// Size of -1 indicates cannot get file
	if (expected_size != -1)
	{
		CURL *hcurl = NULL;
		memstruct_t chunk = {malloc(1), 0};

		// Fill in curl request
		scurl_startup (&hcurl, updatefn);
		curl_easy_setopt(hcurl, CURLOPT_URL, www_url);
		curl_easy_setopt(hcurl, CURLOPT_WRITEFUNCTION, download_procs_memwrite); // send all data to this function
		curl_easy_setopt(hcurl, CURLOPT_WRITEDATA, &chunk);  //we pass our 'chunk' struct to the callback function
		
		// Execute curl task, then shut it down.
		curl_easy_perform(hcurl);
		scurl_shutdown (&hcurl);

		if ((int)chunk.size == expected_size)
		{
			chunk.memory = core_realloc (chunk.memory, chunk.size + 1); // Add + 1 in case we have a string
			chunk.memory[chunk.size] = 0;
			if (len) *len = chunk.size;  // Don't add the +1, not part of the data.
			return chunk.memory;
		}

		core_free (chunk.memory);
	}
	
	return NULL;
}

#endif // CORE_LIBCURL






