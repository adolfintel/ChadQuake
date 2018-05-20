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
// download_procs.h -- download


#ifndef __DOWNLOAD_PROCS_H__
#define __DOWNLOAD_PROCS_H__

enum downloader_e
{
	DOWNLOADER_ERROR_OK_NO_ERROR,
	DOWNLOADER_ERROR_STARTUP, 
	DOWNLOADER_ERROR_NOT_HTTP,
	DOWNLOADER_ERROR_CANCEL,
	DOWNLOADER_ERROR_DOWNLOAD_INCOMPLETE,
	DOWNLOADER_REMOTE_HOST_NOT_RESPONSIVE,
	DOWNLOADER_REMOTE_HOST_TIMEOUT_3_SECONDS,
	DOWNLOADER_FILE_NOT_FOUND,
	MAX_NUM_DOWNLOADER,
};

extern keyvalue_t downloader_error_strings[MAX_NUM_DOWNLOADER + 1];

typedef struct memstruct_s
{
	char *memory;
	size_t size;
} memstruct_t;


typedef struct
{
	void			*id;
	char			user_agent[SYSTEM_STRING_SIZE_1024];
	
	struct arg_buckets_64_s argbuckets;

// A. MEMORY BUFFER DOWNLOAD
	memstruct_t		mem;				// *memory, size.  If download to file, this is null.

// B. FILE DOWNLOAD
	
	char			remote_url[SYSTEM_STRING_SIZE_1024];
	char			local_url[MAX_OSPATH]; 
	
	cbool			user_cancelled;
	int				total_received;
	int				expected_size;
	int				http_status_code;	// 404 or whatnot
	int				exit_code;			// Finish event exit code?

// C. FEEDBACK.  TYPICALLY FOR A NON-THREADED DOWNLOAD

	progress_fn_t	update_fn;
	printline_fn_t	printline_fn;

	int				*out_expected_size;	// Feedback.  Same as expected size.
	int				*out_exit_code;		// Feedback.  Same as exit_code

// D. THREADED DOWNLOAD	

	volatile cbool	*async_cancellator;	// Pointer to a volatile int (int* volatile ptr; would be a volatile pointer instead).
	eventfunc_t		finish_event_fn;
	int				finish_code;
	void 			*finish_event_data;	// user-defined.  Do we pass the download struct?  Possibly.
} download_x_t;


int Download_HTTP_Write (download_x_t *download, void *newdata, size_t size, size_t count /* usually 1 */);






#endif // __DOWNLOAD_PROCS_H__




