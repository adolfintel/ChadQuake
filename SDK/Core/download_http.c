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
// download_http.c -- download functions


#define CORE_LOCAL
#include "core.h"
#include "download_procs.h"

keyvalue_t downloader_error_strings[MAX_NUM_DOWNLOADER + 1] =
{
	{ "Response OK", DOWNLOADER_ERROR_OK_NO_ERROR },
	{ "Response started", DOWNLOADER_ERROR_STARTUP },
	{ "Response URL must start http://", DOWNLOADER_ERROR_NOT_HTTP },
	{ "Response cancelled by user", DOWNLOADER_ERROR_CANCEL },
	{ "Response download incomplete", DOWNLOADER_ERROR_DOWNLOAD_INCOMPLETE },
	{ "Response remote host not responsive or URL invalid", DOWNLOADER_REMOTE_HOST_NOT_RESPONSIVE },
	{ "Response remote host timeout", DOWNLOADER_REMOTE_HOST_TIMEOUT_3_SECONDS },
	{ "Response remote host reported size of file zero", DOWNLOADER_FILE_NOT_FOUND },
	{ "", 0 },
};


int http_client (download_x_t *download);



char *default_user_agent = "User Agent";

static download_x_t *Stuff_Download_Alloc (const char *user_agent, const char *www_url, const char *path_to_file,
										   progress_fn_t update_fn, printline_fn_t my_printline, int *pset_size, int *pexit_code,  // Blocking
											cbool *async_cancellator, eventfunc_t finish_event_fn, int finish_code, void *id) // Threaded (non-Blocking)
{
	download_x_t *download = core_calloc(sizeof(download_x_t), 1);

	download->id = id;
	c_strlcpy (download->user_agent, user_agent ? user_agent : default_user_agent);

	c_strlcpy (download->remote_url, www_url);

	if (!path_to_file)  // No path to file is download to memory.
		download->mem.memory = !path_to_file ? core_calloc(1, 1) : 0;	// For write to memory, need to alloc a byte or so for realloc.  Or not?
	else c_strlcpy (download->local_url, path_to_file);

	download->async_cancellator = async_cancellator;
	download->finish_event_fn = finish_event_fn;
	download->finish_event_data = NULL; // Will become download later.
	download->finish_code = finish_code; // Will become download later.

	download->printline_fn	= my_printline; // For async this will be Con_Queue_Printf.  For blocking, probably will be NULL.
	download->update_fn = update_fn; // For async this will be NULL.

	download->out_expected_size = pset_size;			// Howz this work
	download->out_exit_code = pexit_code;

	return download;
}

// Threaded or unthreaded can call me
static void *Download_Http_ (download_x_t *download)
{
	// Everything's been stuff into the download struct.
	// We could be called by a thread or by a
	int ret;

	// Yes this is rather late for this check, but I'm designing this to be oriented towards proper usage.
	if (!String_Does_Start_With (download->remote_url, "http://"))
		ret = -1;
	else
	{
		c_snprintf1 (download->argbuckets.cmdline, "anything /h:%s", download->remote_url);
		String_To_Arg_Buckets (&download->argbuckets, download->argbuckets.cmdline);
		ret = http_client (download); // argbuckets.argcount, argbuckets.argvs); HTTP_CLIENT_EOS = 1000 which is good
	}

// Determine the exit code
	if (download->user_cancelled)		download->exit_code = download->exit_code = DOWNLOADER_ERROR_CANCEL;
	else if (in_range(400, ret, 499))	download->exit_code = ret; // Baker
	else if (ret == -1)					download->exit_code = DOWNLOADER_ERROR_NOT_HTTP;
	else if (ret == 10)					download->exit_code = DOWNLOADER_REMOTE_HOST_NOT_RESPONSIVE; // This may not happen any longer since we set timeout to 3
	else if (ret == 8)					download->exit_code = DOWNLOADER_REMOTE_HOST_TIMEOUT_3_SECONDS;
	else if (!download->expected_size && ret != 1000)	download->exit_code = DOWNLOADER_FILE_NOT_FOUND; // This may not happen any longer since we set timeout to 3
// Sometimes expected size not set by server?
	else if (download->mem.memory && download->expected_size !=0 && download->mem.size != (size_t)download->expected_size)
										download->exit_code = DOWNLOADER_ERROR_DOWNLOAD_INCOMPLETE;
	else if (!download->mem.memory && File_Length(download->local_url) != (size_t) download->expected_size)
										download->exit_code = DOWNLOADER_ERROR_DOWNLOAD_INCOMPLETE;
	else download->exit_code = 0; // GOOD


	if (download->out_exit_code)
		*(download->out_exit_code) = download->exit_code;

// If failure, clean up
	if (download->exit_code)
	{

		if (!download->mem.memory)
		{
			if (File_Exists (download->local_url)) // Delete the download file if it exists.
				File_Delete (download->local_url);
		}
		else download->mem.memory = core_free (download->mem.memory);


		return NULL;
	}

	if (download->mem.memory)
	{
		// Add an unmentioned byte in case it is a string.  But don't increase reported size, that'd be messing with the data.
		download->mem.memory = core_realloc (download->mem.memory, download->mem.size + 1);
		download->mem.memory[download->mem.size] = 0;
	}

// Return applicable data
	if (download->mem.memory && !*(download->out_expected_size))
		*(download->out_expected_size) = download->mem.size;
	return download->mem.memory ? download->mem.memory : (void *) true;
}
//		download = core_free (download); // Release it.  But if download to mem, we passed the memory stream.
//		Core_Print Linef ("Result was %d:%s", ret, KeyValue_GetKeyString (downloader_error_strings, *errorcode));


// If you want a temp name, feed us the temp name.  This function shouldn't be deciding temp names.  Doesn't know cache.
// Doesn't know folders structure for binary running, etc.
cbool Download_To_File (const char *user_agent, const char *www_url, const char *path_to_file, progress_fn_t update_fn, printline_fn_t my_printline, int *pset_size, int *pexit_code)
{
	download_x_t *download = Stuff_Download_Alloc (user_agent, www_url, path_to_file,
								update_fn, my_printline, pset_size, pexit_code,
								NULL /* async cancel */, NULL /*event finish */, 0 /* finish code */, NULL /* id */);

	cbool result = (Download_Http_ (download) != NULL);
	download = core_free (download); // We have to free it here now.
	return result;
}

void *Download_To_Memory_Alloc (const char *user_agent, const char *www_url, progress_fn_t update_fn, printline_fn_t my_printline, int *pset_size, int *pexit_code)
{
	download_x_t *download = Stuff_Download_Alloc (user_agent, www_url, NULL /* no path, memory write */,
								update_fn, my_printline, pset_size, pexit_code,
								NULL /* async cancel */, NULL /*event finish */, 0 /* finish code */, NULL /* id */);

	void * ret = Download_Http_ (download);
	download = core_free (download); // We have to free it here now.
	return ret; // Receiver must free memory.
}
#ifdef CORE_PTHREADS
static void *Download_Async_Go (void *_download)
{
	// We just treat it like normal now.  download->finish_event_fn = Download_Async_End;
	download_x_t *download = _download;

	void * ret = Download_Http_ (download);

	// Messenger is responsible for freeing download?  Sure.  Dangerous?
	// We can rework later.

	download->finish_event_fn (EVENT_DOWNLOAD_COMPLETE, download->finish_code, download->id, download /*data*/);

//	Con_Queue_PrintLinef ("Download Thread ended"); // Thread-safe, blocking.
	pthread_exit (NULL); // This is optional unless you want to return something for pthread_join to read.
	return NULL;
}


// It's your responsibility to make sure something isn't downloading twice or at same time, not the downloaders
// One way to do this is is_downloading that is only set in main thread.
// Or use of download to memory for some things.
//int download_count; // How would we make this wait?
//pthread_t download_threads[MAX_DOWNLOAD_THREADS]; // No limit!
// Let's get this going first.  I'm thinking we'd have to make a queue and have completed download call next.
// So how would it start?  This function would do download_next if count = 0 or something.

void Download_Http_Async_To_Memory (const char *user_agent, const char *www_url, cbool *async_cancellator, eventfunc_t finish_event_fn, int finish_code, void *id) // Threaded (non-Blocking)
{
	pthread_t my_thread;
	download_x_t *download = Stuff_Download_Alloc (user_agent, www_url, NULL /*no path_to_file*/, NULL, NULL, NULL, NULL, async_cancellator, finish_event_fn, finish_code, id);

#pragma message ("make sure there is a blocking form of print for this")
	//Con_Queue_PrintLinef ("Download Thread: %s", download->remote_url); // Thread-safe, blocking.  Really?

	pthread_create (&my_thread, NULL /* no attributes */, Download_Async_Go, (void *)download);
}
#endif // CORE_PTHREADS



int Download_HTTP_Write (download_x_t *download, void *newdata, size_t size, size_t count /* usually 1 */)
{
	size_t download_procs_memwrite (void *contents, size_t size, size_t nmemb, memstruct_t *mem);
	size_t download_procs_filewrite (void *contents, size_t size, size_t nmemb, void *path_url);
	//float pct;

	int should_cancel = false;
	int old_total = download->total_received;
	// Traffic direction
	download->total_received += size * count;
	//pct = (download->expected_size > 0) ? download->total_received / download->expected_size : 0;

	if (download->mem.memory)
		download_procs_memwrite (newdata,  size, count, &download->mem);
	else download_procs_filewrite (newdata, size, count, download->local_url);
				//printf ("%d", nTotal);

	// If there is an update function (there usually is)
	// Give it an update.  This also gives the opportunity to cancel.
	// update_fn is only decent for blocking variant at the moment because doesn't identify what is downloading.
	if (download->update_fn && download->update_fn (NULL, old_total, download->total_received) )
		should_cancel = true;

	// Thread method.
	if (download->async_cancellator && *(download->async_cancellator))
		should_cancel = true;

	if (should_cancel)
		download->user_cancelled = true; // Note we only affirmatively set this.
	return should_cancel;
}

