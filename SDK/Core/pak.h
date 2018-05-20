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
// pak.h -- pak file header

#ifndef __PAK_H__
#define __PAK_H__


#define MAX_PAK_FILENAME 56
#define PAK_FILE_ENTRY_SIZE_64  64 // Hopefully sizeof(dpackfile_t)
#define PAK_FILE_HEADER_SIZE_12 12 // Hopefully sizeof(dpackheader_t)

typedef struct
{
	char    name[MAX_PAK_FILENAME];
	int     filepos, filelen;
} dpackfile_t;

typedef struct
{
	char    id[4];
	int     dirofs;
	int     dirlen;
} dpackheader_t;

#define MAX_FILES_IN_PACK_2048       2048
#define PAK_HEADER "PACK"

typedef struct pak_s
{
	char			url[MAX_OSPATH];
	dpackheader_t   header;
	dpackfile_t		files[MAX_FILES_IN_PACK_2048];
// Add a structure for new files to add?
// When we write, do we need to qsort?
	int				numfiles;
	FILE			*f;

// Extra
	byte			*pHeader;
	byte			*pDirectory;
	int				length;
//	clist_t			*files_list;
} pak_t;


cbool Pak_Has_File (const char *packfile_url, const char *filename);
void Pak_List_Print (const char * packfile_url);
clist_t * Pak_List_Alloc (const char* packfile_url);
clist_t * Pak_List_Details_Alloc (const char *packfile_url, const char *delimiter);

cbool Pak_Add_File (const char *packfile_url, const char *inside_pak_filename, const char *srcfile_url);
cbool Pak_Extract_File (const char *packfile_url, const char *inside_pak_filename, const char *destfile_url);
int Pak_Unzip (const char *packfile_url, const char *dest_folder_url);
int Pak_Zip_Folder (const char *packfile_url, const char *source_folder_url);
cbool Pak_Remove_File (const char *packfile_url, const char *inside_pak_filename);
cbool Pak_Rename_File (const char *packfile_url, const char *inside_pak_filename, const char *new_name);
void Pak_Replace_File (const char *packfile_url, const char *inside_pak_filename, const char *srcfile_url);

cbool Pak_Compress (const char *packfile_url);
size_t Pak_Is_Compressable (const char *packfile_url); // Returns size gained by compression

///////////////////////////////////////////////////////////////////////////////
//  Pak from memory
///////////////////////////////////////////////////////////////////////////////


clist_t *Pack_Files_List_Alloc (pak_t *pack, const char *wild_patterns, reply int *num_matches);
pak_t *Pack_Open_Memory_Alloc (const void *mem, size_t mem_length);
pak_t *Pack_Open_Memory_Free (pak_t *pack);
void *dPack_Find_File_Caseless (pak_t *pack, const char *path_to_file);
void *Pack_File_Entry_To_Memory_Alloc (pak_t *pack, const byte *pak_blob, const char *path_to_file, reply size_t *mem_length); // EXTRA_BYTE_NULL_ASSURANCE_ALLOC_+_1

const void *Pack_File_Memory_Pointer (pak_t *pack, const byte *pak_blob, const char *path_to_file, reply size_t *mem_length);

cbool Pack_Extract_File (pak_t *pack, const byte *pak_blob, const char *path_to_file, const char *destfile_url);

// replace_tokens2 is an array of strings (find, replace, find, replace, NULL, NULL) - double null terminated and should be multiple of 2 size
// technically it can be single null terminated, but double null termination helps prevent errors in the array declaration.
cbool Pack_Extract_All_To (pak_t *pack, const byte *pak_blob, const char *dest_folder, const char *wild_patterns, const char **replace_tokens2);

#endif // ! __PAK_H__
