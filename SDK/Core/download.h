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
// download.h -- download


#ifndef __DOWNLOAD_H__
#define __DOWNLOAD_H__

// Returns -1 on failure, otherwise size of file.
int Download_Query (const char *www_url);

// Downloads to file, returns true or false 
//cbool Download_To_File (const char *www_url, const char *path_to_file, progress_fn_t update_fn, printline_fn_t my_printline, int *setsize);

typedef void  (*eventfunc_t ) (int event, int code, void *id, void *data); // Event num and extra data

void Download_Http_Async_To_Memory (const char *user_agent, const char *www_url, cbool *async_cancellator, eventfunc_t finish_event_fn, int finish_code, void *id); // Threaded (non-Blocking)
void *Download_To_Memory_Alloc (const char *user_agent, const char *www_url, progress_fn_t update_fn, printline_fn_t my_printline, int *pset_size, int *pexit_code);
cbool Download_To_File (const char *user_agent, const char *www_url, const char *path_to_file, progress_fn_t update_fn, printline_fn_t my_printline, int *pset_size, int *pexit_code);


// Downloads to memory
//byte *Download_To_Memory_Alloc (const char *www_url, const char *useragent, int *len, progress_fn_t updatefn) alert ("need EXTRA_BYTE_NULL_ASSURANCE_ALLOC_+_1");;

//cbool Download_Set_User_Agent (const char *user_agent_text);



#define EVENT_DOWNLOAD_COMPLETE 1

#define EVENT_DOWNLOAD_LIST	65536

#endif // __DOWNLOAD_H__




