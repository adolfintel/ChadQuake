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
// download_procs.c -- general download support not specific to how


#define CORE_LOCAL
#include "core.h"
#include "download_procs.h"


///////////////////////////////////////////////////////////////////////////////
//  DOWNLOAD OPS: Baker -- simple internal operations
///////////////////////////////////////////////////////////////////////////////

size_t download_procs_memwrite (void *contents, size_t size, size_t nmemb, memstruct_t *mem)
{
	size_t realsize = size * nmemb;

	mem->memory = core_realloc(mem->memory, mem->size + realsize + 1);
	
	if (mem->memory == NULL)
	{
		// out of memory!
		log_fatal ("not enough memory (realloc returned NULL)");
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

size_t download_procs_filewrite (void *contents, size_t size, size_t nmemb, void *path_url)
{
	FILE *f = core_fopen_write ( (char *)path_url, "ab");

	if (f)
	{
		fwrite (contents, size, nmemb, f);
		core_fclose (f);
	}
	return size * nmemb;
}

