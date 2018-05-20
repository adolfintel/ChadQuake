/*
Copyright (C) 2013-2014 Baker

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
// zip.c -- zip

#define CORE_LOCAL
#include "environment.h"
#include "core.h"
#include "zip.h"

#ifdef  __VISUAL_STUDIO_6__
#include "zip_win.h"
#include "unzip_win.h"
#else // Later Visual Studio and non-Windows
#include "miniz.h"
#endif // ! __VISUAL_STUDIO_6__

enum query_e {query_exists, query_index, query_filesize, query_offset};




#ifdef __VISUAL_STUDIO_6__
int Zip_File_Query (const char *zipfile_url, const char *filename, enum query_e query_type)
{
	HZIP curzip = OpenZipFile (zipfile_url, 0);

	if (curzip)
	{
		ZIPENTRY zipinfo, cur;
		int i;
	
		// -1 gives overall information about the zipfile
		GetZipItem (curzip, -1, &zipinfo); 
		
		for (i = 0; i < zipinfo.index; i++)
		{ 
			// get individual details
			GetZipItem (curzip, i, &cur); 
			
			// If name doesn't match keep going
			if (strcasecmp (cur.name, filename))
				continue;

			switch (query_type)
			{
			case query_exists:
			case query_index:
				CloseZip (curzip);
				return i;
//			case query_offset:
//				CloseZip (curzip);
//				return curzip->files[i].filepos;
			case query_filesize:
				CloseZip (curzip);
				return cur.unc_size;
			}
		}
		CloseZip (curzip);
	}
	return -1;
}


size_t Zip_File_Size (const char *zipfile_url, const char *filename)
{
	int result = Zip_File_Query (zipfile_url, filename, query_exists);
	cbool found = (result == - 1) ? false : true;

	if (found)
		return result;

	return 0;
}


cbool Zip_Has_File (const char *zipfile_url, const char *filename)
{
	int result = Zip_File_Query (zipfile_url, filename, query_exists);

	cbool found = (result == - 1) ? false : true;

	return found;
}



#pragma message ("Baker: Make sure zip extract can do zero length extraction ok")
// If inside_zip_filename is NULL, we do them all
int sZip_Extract_File (const char *zipfile_url, const char *inside_zip_filename, const char *destfile_url, printline_fn_t my_printline)
{
	HZIP curzip = OpenZipFile (zipfile_url, 0);
	int written;

	if (curzip)
	{
		ZIPENTRY zipinfo, cur;
		ZRESULT result;
		char current_url[MAX_OSPATH];
		const char *curfile = inside_zip_filename ? destfile_url : current_url;
		int i;
		
		// -1 gives overall information about the zipfile
		GetZipItem (curzip, -1, &zipinfo); 
			
		for (i = 0, written = 0; i < zipinfo.index; i++)
		{
			// get individual details
			GetZipItem (curzip, i, &cur); 
	
			// If inside_zip_filename is NULL, we do them all
			if (inside_zip_filename && strcasecmp (cur.name, inside_zip_filename))
				continue;

			// If we are extracting the whole thing, destfile_url is folder + cur->name becomes url for curfile
			if (!inside_zip_filename)
				c_snprintf2 (current_url, "%s/%s", destfile_url, cur.name);

			// make the folder
			File_Mkdir_Recursive (curfile);

			// extract
			result = UnzipItemFile (curzip, i, curfile);

			if ( result!= ZR_OK) // ZR_OK = 0
			{
				my_printline ("Error trying to write file %s", curfile);
				CloseZip (curzip);
				return false;
			}

			my_printline ("Extracted to %s", curfile);
			written ++;

			if (inside_zip_filename)
				break; // Asked for just one so get out
		}
		
		CloseZip (curzip);

		return written;
	}

	my_printline ("Couldn't open zip %s", zipfile_url);
	return false;
}


cbool Zip_Extract_File (const char *zipfile_url, const char *inside_zip_filename, const char *destfile_url)
{
	if (sZip_Extract_File (zipfile_url, inside_zip_filename, destfile_url, logflex))
		return true;

	return false;
}


int Zip_Unzip (const char *zipfile_url, const char *dest_folder_url)
{
	int n = sZip_Extract_File (zipfile_url, NULL, dest_folder_url, log_debug);

	return n;
}


void Zip_List_Print (const char * zipfile_url)
{
	HZIP curzip = OpenZipFile (zipfile_url, 0);


	if (curzip)
	{
		ZIPENTRY zipinfo, cur;
		int i, found;

		// -1 gives overall information about the zipfile
		GetZipItem (curzip, -1, &zipinfo); 
			
		for (i = 0, found = 0; i < zipinfo.index; i++, found ++)
		{ 
			// get individual details
			GetZipItem (curzip, i, &cur); 
			logc ("%04d: %s", found, cur.name); // I would hope a print function would print!!!
		}
		
		CloseZip (curzip);
	}
}


clist_t * Zip_List_Alloc (const char *zipfile_url)
{
	HZIP curzip = OpenZipFile (zipfile_url, 0);
	clist_t * list = NULL;

	if (curzip)
	{
		ZIPENTRY zipinfo, cur;
		int i, found;
		
		// -1 gives overall information about the zipfile
		GetZipItem (curzip, -1, &zipinfo); 
			
		for (i = 0, found = 0; i < zipinfo.index; i++, found ++)
		{ 
			// get individual details
			GetZipItem (curzip, i, &cur); 
			List_Add (&list, cur.name);
		}
		
		CloseZip (curzip);
	}

	return list;
}


int Zip_Zip_Folder (const char *zipfile_url, const char *source_folder_url)
{
	clist_t *files = File_List_Recursive_Relative_Alloc (source_folder_url, NULL /* no wildcard */);


	if (!files)
		return 0;
	else
	{
	    HZIP curzip;
		int count;
		clist_t *listitem;

		// Make the location of the zip file
		File_Mkdir_Recursive (zipfile_url);
	 	curzip = CreateZipFile (zipfile_url, 0);

		if (!curzip)
		{
			List_Free (&files);
			return 0;
		}

		for (listitem = files, count = 0; listitem; listitem = listitem->next, count ++)
		{
			const char * full_url = va ("%s/%s", source_folder_url, listitem->name);

			ZipAddFile (curzip, listitem->name,  full_url);

			logflex ("%04d: Added %s", count, listitem->name);
		}


		// close the file
		CloseZip (curzip);

		// Free the list
		List_Free (&files);
		return count;
	}

}

#else // Not Visual Studio 6 ...

int Zip_File_Query (const char *zipfile_url, const char *filename, enum query_e query_type)
{
	mz_zip_archive curzip = {0};
	mz_bool status = mz_zip_reader_init_file(&curzip, zipfile_url, 0);

	if (status)
	{
		mz_zip_archive_file_stat cur;
	
		int i;

		// Get and print information about each file in the archive.
		for (i = 0; i < (int) curzip.m_total_files; i++)
		{
			if (!mz_zip_reader_file_stat (&curzip, i, &cur))
			{
				logd ("mz_zip_reader_file_stat() failed!");
				break;
			}

			// If name doesn't match keep going
			if (strcasecmp (cur.m_filename, filename))
				continue;

			switch (query_type)
			{
			case query_exists:
			case query_index:
				mz_zip_reader_end (&curzip);
				return i;
//			case query_offset:
//				CloseZip (curzip);
//				return curzip->files[i].filepos;
			case query_filesize:
				mz_zip_reader_end (&curzip);
				return cur.m_uncomp_size;
			}
		}

		mz_zip_reader_end (&curzip);
	}
	return -1;
}


size_t Zip_File_Size (const char *zipfile_url, const char *filename)
{
	int result = Zip_File_Query (zipfile_url, filename, query_exists);
	cbool found = (result == - 1) ? false : true;

	if (found)
		return result;

	return 0;
}


cbool Zip_Has_File (const char *zipfile_url, const char *filename)
{
	int result = Zip_File_Query (zipfile_url, filename, query_exists);

	cbool found = (result == - 1) ? false : true;

	return found;
}

// If inside_zip_filename is NULL, we do them all
int sZip_Extract_File (const char *zipfile_url, const char *inside_zip_filename, const char *destfile_url, printline_fn_t my_printline)
{
	mz_zip_archive curzip = {0};
	mz_bool status = mz_zip_reader_init_file(&curzip, zipfile_url, 0);
	int written;

	if (status)
	{
		mz_zip_archive_file_stat cur;
		char current_url[MAX_OSPATH];
		const char *curfile = inside_zip_filename ? destfile_url : current_url;
		int i;
					
		for (i = 0, written = 0; i < (int) curzip.m_total_files; i++)

		{
			// get individual details
			if (!mz_zip_reader_file_stat (&curzip, i, &cur))
			{
				logd ("mz_zip_reader_file_stat() failed!");
				mz_zip_reader_end (&curzip);
				return false;
			}
	
			// If inside_zip_filename is NULL, we do them all
			if (inside_zip_filename && strcasecmp (cur.m_filename, inside_zip_filename))
				continue;

			// If it is a directory, we aren't writing it
			if (String_Does_End_With (cur.m_filename, "/"))
				continue;

			// If we are extracting the whole thing, destfile_url is folder + cur->name becomes url for curfile
			if (!inside_zip_filename) // 13 May 2015 commented out
				c_snprintf2 (current_url, "%s/%s", destfile_url, cur.m_filename);
			else c_strlcpy (current_url, destfile_url);

			// make the folder
			File_Mkdir_Recursive (curfile);

			// extract	 
			if (!mz_zip_reader_extract_to_file (&curzip, cur.m_file_index, current_url, 0))
			{
				logd ("mz_zip_reader_extract_to_file() failed!");
				mz_zip_reader_end (&curzip);
				return false;
			}

			my_printline ("Extracted to %s", curfile);
			written ++;

			if (inside_zip_filename)
				break; // Asked for just one so get out
		}
		
		mz_zip_reader_end (&curzip);

		return written;
	}

	logd ("Couldn't open zip %s", zipfile_url);
	return false;
}


cbool Zip_Extract_File (const char *zipfile_url, const char *inside_zip_filename, const char *destfile_url)
{
	if (sZip_Extract_File (zipfile_url, inside_zip_filename, destfile_url, log_debug))
		return true;

	return false;
}


int Zip_Unzip (const char *zipfile_url, const char *dest_folder_url)
{
	int n = sZip_Extract_File (zipfile_url, NULL, dest_folder_url, log_debug);

	return n;
}


void Zip_List_Print (const char *zipfile_url)
{
	mz_zip_archive curzip = {0};
	mz_bool status = mz_zip_reader_init_file(&curzip, zipfile_url, 0);
	
	
	if (status)
	{
		mz_zip_archive_file_stat cur;
		int i, found;

		for (i = 0, found = 0; i < (int) curzip.m_total_files; i++, found ++)
		{
			// get individual details
			if (!mz_zip_reader_file_stat (&curzip, i, &cur))
			{
				logd ("mz_zip_reader_file_stat() failed!");
				break;
			}

			logd ("%04d: %s", found, cur.m_filename);
		}

		mz_zip_reader_end (&curzip);
	}
}


clist_t * Zip_List_Alloc (const char *zipfile_url)
{
	mz_zip_archive curzip = {0};
	mz_bool status = mz_zip_reader_init_file(&curzip, zipfile_url, 0);
	clist_t * list = NULL;
	
	if (status)
	{
		mz_zip_archive_file_stat cur;
		int i, found;

		// Get and print information about each file in the archive.
		for (i = 0, found = 0; i < (int) curzip.m_total_files; i++, found ++)
		{
			// get individual details
			if (!mz_zip_reader_file_stat (&curzip, i, &cur))
			{
				logd ("mz_zip_reader_file_stat() failed!");
				mz_zip_reader_end (&curzip);
				List_Free (&list);
				return NULL;
			}

			List_Add (&list, cur.m_filename);
		}

		mz_zip_reader_end (&curzip);
	}

	return list;
}

clist_t * Zip_List_Details_Alloc (const char *zipfile_url, const char *delimiter)
{
	mz_zip_archive curzip = {0};
	mz_bool status = mz_zip_reader_init_file(&curzip, zipfile_url, 0);
	clist_t * list = NULL;
	
	if (!delimiter) delimiter = "\t";

	if (status)
	{
		mz_zip_archive_file_stat cur;
		int i, found;

		// Get and print information about each file in the archive.
		for (i = 0, found = 0; i < (int) curzip.m_total_files; i++, found ++)
		{
			// get individual details
			if (!mz_zip_reader_file_stat (&curzip, i, &cur))
			{
				logd ("mz_zip_reader_file_stat() failed!");
				mz_zip_reader_end (&curzip);
				List_Free (&list);
				return NULL;
			}

			List_Add(&list, va("%s%s%g",cur.m_filename,delimiter,(double)cur.m_uncomp_size));
			// Future: ,delimiter,(double)cur.m_time));
		}

		mz_zip_reader_end (&curzip);
	}

	return list;
}

int Zip_Zip_Folder (const char *zipfile_url, const char *source_folder_url)
{
	clist_t *files = File_List_Recursive_Relative_Alloc (source_folder_url, NULL);
	int count = 0;

	if (files)
	{
		mz_zip_archive curzip = {0};
		clist_t *listitem;
		
		// Make the location of the zip file
		File_Mkdir_Recursive (zipfile_url);

		if (!mz_zip_writer_init_file(&curzip, zipfile_url, MZ_ZIP_FLAG_COMPRESSED_DATA))
		{
			List_Free (&files);
			return 0;
		}
		
		for (listitem = files, count = 0; listitem; listitem = listitem->next, count ++)
		{
			const char * full_url = va ("%s/%s", source_folder_url, listitem->name);
	
			if (!mz_zip_writer_add_file(&curzip, listitem->name, full_url, NULL, 0, MZ_DEFAULT_LEVEL /* flags*/))
			{
				mz_zip_writer_finalize_archive(&curzip);
				mz_zip_writer_end(&curzip);
				List_Free (&files);
				return 0;
			}
			
			logd ("%04d: Added %s", count, listitem->name);
		}
	
		// close the file
		mz_zip_writer_finalize_archive(&curzip);
		mz_zip_writer_end(&curzip);

		// Free the list
		List_Free (&files);
		return count;
	}
	return 0;
}

#endif // ! __VISUAL_STUDIO_6__

