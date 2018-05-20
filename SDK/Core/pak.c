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
// pak.c -- pak file

#define CORE_LOCAL
#include "environment.h"
#include "core.h"
#include "pak.h"


#define LittleLong(value) (value)




enum query_e {query_exists, query_index, query_filesize, query_offset};

///////////////////////////////////////////////////////////////////////////////
//  Pak from memory
///////////////////////////////////////////////////////////////////////////////


clist_t *Pack_Files_List_Alloc (pak_t *pack, const char *wild_patterns, reply int *num_matches)
{
	clist_t *files_list = NULL;
	clist_t *item;
	int n, count;

	for (n = 0, count = 0; n < pack->numfiles; n ++) {
		const char *inside_pak_file_url = pack->files[n].name;

		if (wild_patterns && !wildcompare (inside_pak_file_url, wild_patterns))
			continue; // The file didn't match our search criteria, so skip!

		item = List_Add_No_Case_To_Lower (&files_list, inside_pak_file_url);
		count ++;
	}

	NOT_MISSING_ASSIGN (num_matches, count);
	return files_list;
}


// returns null
void *dPack_Find_File_Caseless (pak_t *pack, const char *path_to_file)
{
	int n;

	for (n = 0; n < pack->numfiles; n ++) {
		const char *fn = pack->files[n].name;
		if (!strcasecmp (path_to_file, fn))
			return &pack->files[n];
	}

	return NULL; // Not found
}

// EXTRA_BYTE_NULL_ASSURANCE_ALLOC_+_1
void *Pack_File_Entry_To_Memory_Alloc (pak_t *pack, const byte *pak_blob, const char *path_to_file, reply size_t *mem_length)
{
	dpackfile_t *row = dPack_Find_File_Caseless (pack, path_to_file);
	byte *mem_o = NULL;

	if (row) {
		size_t _mem_length = 0;
		mem_o = core_memdup (&pak_blob[row->filepos], (_mem_length = row->filelen) + 1 /*null term assurance*/ );
		mem_o[_mem_length] = 0; // For possible string treatment
		NOT_MISSING_ASSIGN(mem_length, _mem_length);

	}
	return mem_o;
}

const void *Pack_File_Memory_Pointer (pak_t *pack, const byte *pak_blob, const char *path_to_file, reply size_t *mem_length)
{
	dpackfile_t *row = dPack_Find_File_Caseless (pack, path_to_file);
	const byte *mem_o = mem_o = row ? &pak_blob[row->filepos] : NULL;

	if (!row) {
		logd ("File not found in pack: '%s'", path_to_file);
		return NULL;
	}

	NOT_MISSING_ASSIGN (mem_length, row->filelen);
	return mem_o;
}


cbool Pack_Extract_File (pak_t *pack, const byte *pak_blob, const char *path_to_file, const char *destfile_url)
{
	//pak_t *pack = _pack;
	size_t mem_length; byte *mem_a = Pack_File_Entry_To_Memory_Alloc (pack, pak_blob, path_to_file, &mem_length);

	if (!mem_a) {
		logd ("File %s couldn't be extracted from pack", path_to_file);
		return false;
	}

	File_Mkdir_Recursive (destfile_url);

	if (!File_Memory_To_File (destfile_url, mem_a, mem_length)) {
		logd ("Couldn't write %s", destfile_url);
		return false;
	}

	mem_a = core_free (mem_a);
	return true;
}


cbool Pack_Extract_All_To (pak_t *pack, const byte *pak_blob, const char *dest_folder, const char *wild_patterns, const char **replace_tokens2)
{
	int n;
	char dest_url[MAX_OSPATH];
	char extrabuf[MAX_OSPATH];
	cbool ret = true;

	for (n = 0; n < pack->numfiles; n ++) {
		const char *inside_pak_file_url = pack->files[n].name;
		const char *extract_as_name = inside_pak_file_url;

		if (wild_patterns && !wildcompare (inside_pak_file_url, wild_patterns)) {
//			alert ("%s failed to match pattern - won't be unpaked", inside_pak_file_url);
			continue; // The file didn't match our search criteria, so skip!
		}

		if (replace_tokens2) {
			c_strlcpy (extrabuf, inside_pak_file_url);
			String_Edit_Replace_Token_Array (extrabuf, sizeof(extrabuf), replace_tokens2);
			extract_as_name = extrabuf;
		}

		c_snprintf2 (dest_url, "%s/%s", dest_folder, extract_as_name);



//		alertz (inside_pak_file_url, "->", dest_url, NULL);
		ret = ret & Pack_Extract_File (pack, pak_blob, inside_pak_file_url, dest_url);
	}

	return ret; // This will be false if any operation fails.
}


// Size is a bit tight!
pak_t *Pack_Open_Memory_Free (pak_t *pack)
{
//	pak_t *pack = _pack;

//	List_Free (&pack->files_list);

	pack = core_free (pack);
	return NULL;
}


// On OSX, we will need to have the pak file in memory the entire time?
// And we won't be using this function at all because it is "on disk"?
pak_t *Pack_Open_Memory_Alloc (const void *mem, size_t mem_length)
{
	const byte *bmem = mem;
	size_t sz;

	pak_t *pack = calloc(sz=sizeof(*pack), 1);
//	alert ("%d", (int)sz);
	pack->pHeader			= (void *)bmem; // This doesn't hurt.
	pack->length			= mem_length;

	memcpy (&pack->header, pack->pHeader, sizeof(pack->header));

	if (memcmp (pack->header.id, PAK_HEADER, strlen(PAK_HEADER)) ) {
		log_debug ("Data not a packfile"); // Communicate nature of the problem
		goto pakopenerr;
	}

	pack->header.dirofs	= LittleLong (pack->header.dirofs); // LittleLong on Little-endian processors like Intel just are the value
	pack->header.dirlen	= LittleLong (pack->header.dirlen);
	pack->numfiles		= pack->header.dirlen / PAK_FILE_ENTRY_SIZE_64; // sizeof(dpackfile_t);

	if (pack->numfiles > MAX_FILES_IN_PACK_2048) {
		log_debug ("%s has %d files > max %d", "<embedded packfile>", pack->numfiles, MAX_FILES_IN_PACK_2048); // Communicate nature of the problem
		goto pakopenerr;
	}

	pack->pDirectory		= &pack->pHeader[pack->header.dirofs];
	memcpy ( (dpackfile_t *)&pack->files, pack->pDirectory, pack->header.dirlen);

#if 0 // Super debug
    {
        int n;
        for (n = 0; n < pack->numfiles; n ++) {
            const char *fn = pack->files[n].name;
//            alert ("pak %d: %s", n, fn);
        }
    }
#endif // 1
	//alert ("%d @ %p", pack->numfiles, pack);
//	alert ("Pack_Open_Memory_Alloc Bundle: numfiles is %d, last file %s", pack->numfiles, pack->files[pack->numfiles - 1].name);
	return pack;

// Fail
pakopenerr:
	free (pack);
	return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//  PAK from file
///////////////////////////////////////////////////////////////////////////////


pak_t* Pak_Open (const char *packfile_url, cbool dont_close)
{
	static pak_t curpak = {0};
	c_strlcpy (curpak.url, packfile_url);

	if (dont_close)
		curpak.f = core_fopen_write (packfile_url, "r+b"); // r+b read with update
	else curpak.f = core_fopen_read (packfile_url, "rb");

	if (!curpak.f)
	{
		logd ("Couldn't open pak %s", curpak.url);
		return NULL;
	}
	//alert ("Opened the pak");
	fread (&curpak.header, 1, sizeof(curpak.header), curpak.f);

	if (memcmp (curpak.header.id, PAK_HEADER, strlen(PAK_HEADER)) )
	{
		log_debug ("%s is not a packfile", curpak.url); // Communicate unusual nature of the problem
		goto pakopenerr;
	}

	curpak.header.dirofs	= LittleLong (curpak.header.dirofs); // LittleLong on Little-endian processors like Intel just are the value
	curpak.header.dirlen	= LittleLong (curpak.header.dirlen);
	curpak.numfiles			= curpak.header.dirlen / PAK_FILE_ENTRY_SIZE_64; // sizeof(dpackfile_t);

	if (curpak.numfiles > MAX_FILES_IN_PACK_2048)
	{
		log_debug ("%s has %d files > max %d", curpak.url, curpak.numfiles, MAX_FILES_IN_PACK_2048); // Communicate unusual nature of the problem
		goto pakopenerr;
	}

	// Baker: Advance to the header
	fseek (curpak.f, curpak.header.dirofs, SEEK_SET);
	fread (&curpak.files, 1, curpak.header.dirlen, curpak.f); // curpak.files now populated.

	if (!dont_close)
	{
		core_fclose (curpak.f);
		curpak.f = NULL;
	}
	return &curpak;

pakopenerr:
	core_fclose (curpak.f);
	return NULL;
}


int Pak_File_Query (const char *packfile_url, const char *filename, enum query_e query_type)
{
	pak_t* curpak = Pak_Open (packfile_url, false);
	const char *cur;
	int i;

	for (i = 0; i < curpak->numfiles; i++)
	{
		cur = curpak->files[i].name;

		// If name doesn't match keep going
		if (strcasecmp (cur, filename))
			continue;

		switch (query_type)
		{
		case query_exists:
		case query_index:
			return i;
		case query_offset:
			return curpak->files[i].filepos;
		case query_filesize:
			return curpak->files[i].filelen;
		}

	}
	return -1;
}

size_t Pak_File_Size (const char *packfile_url, const char *filename)
{
	int result = Pak_File_Query (packfile_url, filename, query_exists);
	cbool found = (result == - 1) ? false : true;

	if (found)
		return result;

	return 0;
}


cbool Pak_Has_File (const char *packfile_url, const char *filename)
{
	int result = Pak_File_Query (packfile_url, filename, query_exists);

	cbool found = (result == - 1) ? false : true;

	return found;
}



#pragma message ("Baker: Make sure pak extract can do zero length extraction ok")
// If inside_pak_filename is NULL, we do them all
int sPak_Extract_File (const char *packfile_url, const char *inside_pak_filename, const char *destfile_url) //, printline_fn_t my_printline) Give it a legit application provided output function or don't, no winging it.
{
	pak_t* curpak = Pak_Open (packfile_url, true /* keep it open! */);
	int written;

	if (curpak)
	{
		char current_url[MAX_OSPATH];
		const char *curfile = inside_pak_filename ? destfile_url : current_url;
		int i;

		for (i = 0, written = 0; i < curpak->numfiles; i++)
		{
			dpackfile_t* cur = &curpak->files[i];

			if (!cur->name[0])
				continue; // Has no name.  Might considering doing a break here, actually.

			// If inside_pak_filename is NULL, we do them all
			if (inside_pak_filename && strcasecmp (cur->name, inside_pak_filename))
				continue;

			// If we are extracting the whole thing, destfile_url is folder + cur->name becomes url for curfile
			if (!inside_pak_filename)
				c_snprintf2 (current_url, "%s/%s", destfile_url, cur->name);

			fseek (curpak->f, cur->filepos, SEEK_SET);

			if (!FileHandle_Block_To_File (curpak->f, cur->filelen, curfile))
			{
				logd ("Error trying to write file %s", curfile); // Obvious?
				return false; // Don't need to print error message?
			}

			log_debug ("Extracted to %s", curfile);
			written ++;

			if (inside_pak_filename)
				break; // Asked for just one so get out
		}
		core_fclose (curpak->f);

		return written;
	}

	// Pak_Open already printed an error message, just exit
	return false;
}

cbool Pak_Extract_File (const char *packfile_url, const char *inside_pak_filename, const char *destfile_url)
{
	if (sPak_Extract_File (packfile_url, inside_pak_filename, destfile_url))
		return true;

	return false;
}

int sPak_Free_Entry (const pak_t *curpak)
{
	int i;
	for (i = 0; i < curpak->numfiles; i++)
	{
		const dpackfile_t* cur = &curpak->files[i];
		if (!cur->name[0])
			return i;
	}
	return -1;
}

void Pak_Replace_File (const char *packfile_url, const char *inside_pak_filename, const char *srcfile_url)
{
	Pak_Remove_File (packfile_url, inside_pak_filename);
	Pak_Add_File (packfile_url, inside_pak_filename, srcfile_url);
}

cbool Pak_Add_File (const char *packfile_url, const char *inside_pak_filename, const char *srcfile_url)
{
	pak_t *curpak = Pak_Open (packfile_url, true /* keep it open! */);

	if (curpak)
	{
		int empty_entry = sPak_Free_Entry (curpak);
		int free_entry = (empty_entry >= 0) ? empty_entry : (curpak->numfiles < MAX_FILES_IN_PACK_2048) ? curpak->numfiles : -1;
		dpackfile_t* cur = (free_entry != -1) ? &curpak->files[free_entry] : NULL;

		int i;

		if (curpak->numfiles >= MAX_FILES_IN_PACK_2048)
		{
			log_debug ("File file is full %d files", MAX_FILES_IN_PACK_2048);
			return false;
		}

		// Note that our pak_t struct contains room for all the files
		// We just need to be careful when writing that to disk.

		c_strlcpy (cur->name, inside_pak_filename);
		cur->filepos = curpak->header.dirofs; // Write at the directory offset
		cur->filelen = File_Length (srcfile_url);
		if (cur->filelen < 0) cur->filelen = 0; // safety against big files and signed int32

		// Append the data
		if (cur->filelen)
		{
			fseek (curpak->f, cur->filepos, SEEK_SET);
			FileHandle_Append_File (curpak->f, cur->filelen, srcfile_url);
		}
		curpak->numfiles ++;

		// Update the header
		curpak->header.dirofs = cur->filepos + cur->filelen; // Right?
		curpak->header.dirlen = curpak->numfiles * PAK_FILE_ENTRY_SIZE_64; // dpackfile_t
		fseek (curpak->f, 0, SEEK_SET);
		fwrite (&curpak->header, PAK_FILE_HEADER_SIZE_12, 1, curpak->f); // curpak.header is size 12

		// Write the directory
		fseek (curpak->f, curpak->header.dirofs, SEEK_SET);
		for (i = 0; i < curpak->numfiles; i ++)
		{
			fwrite (  &curpak->files[i], PAK_FILE_ENTRY_SIZE_64, 1, curpak->f);
		}

		core_fclose (curpak->f);

		return true;
	}

	// Pak_Open already printed an error message, just exit
	return false;
}

int Pak_Unzip (const char *packfile_url, const char *dest_folder_url)
{
	int n = sPak_Extract_File (packfile_url, NULL, dest_folder_url);//, logc);

	return n;
}


void Pak_List_Print (const char *packfile_url)
{
	pak_t* curpak = Pak_Open (packfile_url, false);
	const char *cur;
	int i, found;

	for (i = 0, found = 0; i < curpak->numfiles; i++, found ++)
	{
		cur = curpak->files[i].name;

		logd ("%04d: %s", found, cur[0] ? cur : "**deleted**");

		if (curpak->files[i].filelen == 0)
			logd ("%s", "---Zero length file!");
	}
}

clist_t * Pak_List_Alloc (const char *packfile_url)
{
	pak_t* curpak = Pak_Open (packfile_url, false);
	int i, found;
	clist_t * list = NULL;

	if (curpak)
	{
		for (i = 0, found = 0; i < curpak->numfiles; i++, found ++)
		{
			//alert (curpak->files[i].name);
			List_Add(&list, curpak->files[i].name);
		}
	}
	return list;
}

clist_t * Pak_List_Details_Alloc (const char *packfile_url, const char *delimiter)
{
	pak_t* curpak = Pak_Open (packfile_url, false);
	int i, found;
	clist_t * list = NULL;
	if (!delimiter) delimiter = "\t";

	if (curpak)
	{
		for (i = 0, found = 0; i < curpak->numfiles; i++, found ++)
		{
			//alert (curpak->files[i].name);
			List_Addf (&list, "%s%s%d", curpak->files[i].name, delimiter, curpak->files[i].filelen);
		}
	}
	return list;
}


int Pak_Zip_Folder (const char *packfile_url, const char *source_folder_url)
{
#if 0
	clist_t *files = File_List_Relative_Alloc (source_folder_url);
#else
	clist_t *files = File_List_Recursive_Relative_Alloc (source_folder_url, NULL /*no wildcard*/);
#endif

	if (!files)
		return 0;
	else
	{
		int count;
		pak_t _curpak = {0};
		pak_t *curpak = &_curpak;
		clist_t *listitem;

		curpak->f = core_fopen_write (packfile_url, "w+b");

		if (!curpak->f)
		{
			List_Free (&files);
			return 0;
		}

		// Write the header, the data is invalid at this point.
		fwrite (&curpak->header, PAK_FILE_HEADER_SIZE_12, 1, curpak->f);

		for (listitem = files, count = 0; listitem; listitem = listitem->next, count ++)
		{
			const char * full_url = va ("%s/%s", source_folder_url, listitem->name);
			dpackfile_t *cur = &curpak->files[count];

			// Fill in the directory entry
			c_strlcpy (cur->name, listitem->name); // Relative
			cur->filepos = ftell (curpak->f);
			cur->filelen = File_Length (full_url);
			if (cur->filelen < 0) cur->filelen = 0; // safety against big files and signed int32

			if (cur->filelen > 0)
				FileHandle_Append_File (curpak->f, cur->filelen, full_url);

			logd ("%04d: Added %s", count, cur->name);
			curpak->numfiles ++;
		}

		// Finalize header
		curpak->header.dirofs = ftell (curpak->f);
		curpak->header.dirlen = curpak->numfiles * PAK_FILE_ENTRY_SIZE_64; // dpackfile_t
		curpak->header.id[0] = 'P', curpak->header.id[1] = 'A', curpak->header.id[2] = 'C', curpak->header.id[3] = 'K';

		// Write the directory listing
		for (listitem = files, count = 0; listitem; listitem = listitem->next, count ++)
		{
			dpackfile_t *cur = &curpak->files[count];
			fwrite (&curpak->files[count], PAK_FILE_ENTRY_SIZE_64, 1, curpak->f);
		}

		// Write the header
		fseek (curpak->f, 0, SEEK_SET);
		fwrite (&curpak->header, PAK_FILE_HEADER_SIZE_12, 1, curpak->f); // curpak.header is size 12

		// close the file
		core_fclose (curpak->f);

		// Free the list
		List_Free (&files);

		return curpak->numfiles;
	}

}

cbool Pak_Remove_File (const char *packfile_url, const char *inside_pak_filename)
{
	int file_index = Pak_File_Query (packfile_url, inside_pak_filename, query_exists);

	if (file_index == -1)
	{
		logd ("File %s not found in pak", inside_pak_filename);
		return false;
	}
	else
	{
		pak_t *curpak = Pak_Open (packfile_url, true /* keep it open! */);
		dpackfile_t *cur = &curpak->files[file_index];
		size_t dirloc = curpak->header.dirofs + file_index * PAK_FILE_ENTRY_SIZE_64;

		cur->name[0] = 0;

		fseek (curpak->f, dirloc, SEEK_SET);

		fwrite (cur, PAK_FILE_ENTRY_SIZE_64, 1, curpak->f);
		core_fclose (curpak->f);
		return true;
	}

}

cbool Pak_Rename_File (const char *packfile_url, const char *inside_pak_filename, const char *new_name)
{
	int file_index = Pak_File_Query (packfile_url, inside_pak_filename, query_exists);

	if (file_index == -1)
	{
		logd ("File %s not found in pak", inside_pak_filename);
		return false;
	}
	else
	{
		pak_t *curpak = Pak_Open (packfile_url, true /* keep it open! */);
		dpackfile_t *cur = &curpak->files[file_index];
		size_t dirloc = curpak->header.dirofs + file_index * PAK_FILE_ENTRY_SIZE_64;

		c_strlcpy (cur->name, new_name);

		fseek (curpak->f, dirloc, SEEK_SET);

		fwrite (cur, PAK_FILE_ENTRY_SIZE_64, 1, curpak->f);
		core_fclose (curpak->f);
		return true;
	}

}

size_t Pak_Is_Compressable (const char *packfile_url)
{
	pak_t *curpak = Pak_Open (packfile_url, false /* We don't need it open */);
	size_t pak_file_size = File_Length (packfile_url);
	size_t pak_data_size;

	int i;

	if (!curpak)
		return 0;

	for (i = 0, pak_data_size = PAK_FILE_HEADER_SIZE_12; i < curpak->numfiles; i ++)
	{
		// First check directory entry
		if (curpak->files[i].name[0] == 0)
			continue; // Deleted entry so we don't count that.

		// We need a directory entry
		pak_data_size += PAK_FILE_ENTRY_SIZE_64;

		// And size of the data.
		pak_data_size += curpak->files[i].filelen;

	}

	return (pak_file_size - pak_data_size);
}



static int sname_compare (const void *a, const void *b)
{
	/*
	if (((byte*)a)[0] == 0)
		return  1;
	if (((byte*)b)[0] == 0)
		return -1;
	*/
	return memcmp (a, b, MAX_PAK_FILENAME);
}

cbool Pak_Compress (const char *packfile_url)
{
	pak_t *srcpak = Pak_Open (packfile_url, true /* Keep it open */);
	pak_t  _dstpak = {0};
	pak_t *curpak = &_dstpak;
	char tempfile[MAX_OSPATH];
	int src_i, count, i;
	int skips;

	if (!srcpak)
		return false;

	if (srcpak->numfiles > MAX_FILES_IN_PACK_2048)
	{
		logd ("%s has %d files > max (%d)", packfile_url, srcpak->numfiles, MAX_FILES_IN_PACK_2048);
		return false;
	}

	qsort (srcpak->files, srcpak->numfiles, PAK_FILE_ENTRY_SIZE_64, sname_compare);

	for (i = 0, skips = 0; i < srcpak->numfiles; i ++)
	{
		if (srcpak->files[i].name[0] == 0)
			skips ++;
	}

	// Construct temp file name
	c_strlcpy (tempfile, packfile_url);
	File_URL_Edit_Remove_Extension (tempfile);
	c_strlcat (tempfile, "_");

	// Open a new pak
	if (File_Exists (tempfile))
	{
		core_fclose (srcpak->f);
		logd ("tempfile " QUOTED_S " already exists", tempfile);
		return false;
	}
	curpak->f = core_fopen_write (tempfile,"w+b");

	if (!curpak->f)
	{
		core_fclose (srcpak->f);
		return false;
	}

	// Begin writing by starting with the header
	// Write the header, the data is invalid at this point.
	fwrite (&curpak->header, PAK_FILE_HEADER_SIZE_12, 1, curpak->f);

	for (src_i = skips, count = 0; src_i < srcpak->numfiles; src_i ++, count ++)
	{
		dpackfile_t *cur = &curpak->files[count];

		// Fill in the directory entry
		c_strlcpy (cur->name, srcpak->files[src_i].name);
		if (srcpak->files[src_i].name[0] == 0)
			count = count;

		cur->filepos = ftell (curpak->f);
		cur->filelen = srcpak->files[src_i].filelen;
		if (cur->filelen > 0)
		{
			// Seek in source, dest should be writing sequential
			fseek (srcpak->f, srcpak->files[src_i].filepos, SEEK_SET);
			FileHandle_Block_Copy (curpak->f, srcpak->f, cur->filelen);
		}

		logd ("%04d: Added %s", count, cur->name);

		curpak->numfiles ++;
	}

	// Done with source, close it
	core_fclose (srcpak->f);

	// Finalize header
	curpak->header.dirofs = ftell (curpak->f);
	curpak->header.dirlen = curpak->numfiles * PAK_FILE_ENTRY_SIZE_64; // dpackfile_t
	curpak->header.id[0] = 'P', curpak->header.id[1] = 'A', curpak->header.id[2] = 'C', curpak->header.id[3] = 'K';

	// Write the directory listing
	for (count = 0; count < curpak->numfiles; count ++)
	{
		dpackfile_t *cur = &curpak->files[count];
		fwrite (&curpak->files[count], PAK_FILE_ENTRY_SIZE_64, 1, curpak->f);
	}

	// Write the header
	fseek (curpak->f, 0, SEEK_SET);
	fwrite (&curpak->header, PAK_FILE_HEADER_SIZE_12, 1, curpak->f); // curpak.header is size 12

	// close the file
	core_fclose (curpak->f);

	File_Delete (packfile_url);
	File_Rename (tempfile, packfile_url);

	return true;
}



