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
// file.c -- file functions

#define CORE_LOCAL // Few

#include "core.h"

#include "file.h"
#include "stringlib.h"

#include <stdio.h> // fopen, etc.
#include <stdlib.h> // malloc, etc.


///////////////////////////////////////////////////////////////////////////////
//  FILE INFORMATION: Baker - These functions operate on a path_to_file
///////////////////////////////////////////////////////////////////////////////




// c:/bob*.png --> c:/bob00000.png
int File_Available_Wildcard (char *buf, size_t bufsiz, const char *fmt, ...)
{
	VA_EXPAND (path_to_file_asterisk, SYSTEM_STRING_SIZE_1024, fmt);
// Not perfect as doesn't lock.  To get perfect, we'd need to multi-instance cache folder and such.  And write-lock files being read.
// Shoot for perfect later.
	{
		char path_percent_d[MAX_OSPATH];
		char testname[MAX_OSPATH];
		const char *find_star = strstr (path_to_file_asterisk, "*");
		const char *find_slash = strstr (path_to_file_asterisk, "/");
		int n;

		if (!find_star) log_fatal ("No asterisk in filename");
		if (find_slash && find_star < find_slash) log_fatal ("Asterisk not in final path element");

		c_strlcpy (path_percent_d, path_to_file_asterisk);
		String_Edit_Replace (path_percent_d, sizeof(testname), "*", "%05d");

		for (n = 0; n < 10000; n++)
		{
			c_snprintf1 (testname, path_percent_d, n);
			if (!File_Exists (testname))
			{
				strlcpy (buf, testname, bufsiz);
				return n;
			}
		}
	}

	log_fatal ("Couldn't create wildcard file " QUOTED_S, path_to_file_asterisk);
	return -1;
}


///////////////////////////////////////////////////////////////////////////////
//  FILE ACTION: Baker - These functions operate on a path_to_file
///////////////////////////////////////////////////////////////////////////////


cbool File_Mkdir_Recursive (const char *path_to_file)
{
	char	path_string[MAX_OSPATH];
	char	*cursor;
	cbool	ret = true, result;
	int		count = 0;

	logd ("Making a path " QUOTED_S, path_to_file);

	c_strlcpy (path_string, path_to_file);

	for (cursor = &path_string[1]; *cursor ; cursor ++)
	{
		if (*cursor != '/')
			continue;

		// Don't try to make the parent of "/" which is an empty string
		//		if (cursor == path_string)
		//			continue;

		// create the directory

#ifdef PLATFORM_WINDOWS // Don't try to make the drive.
		if (cursor > path_string && cursor[-1] == ':')
			continue;
#endif // PLATFORM_WINDOWS

		*cursor = 0;

		count ++;
		//alert ("%d %s", count, path_string);
		ret = result = File_Mkdir (path_string); // Dec 28 2016 - check this
		if (!result) {
			logd ("File_Mkdir_Recursive failed on " QUOTED_S, path_string);
		}
		*cursor = '/';
	}
	return ret;
}


void File_Rmdir_List (clist_t *list_of_urls)
{
	clist_t *cur;
	for (cur = list_of_urls; cur; cur = cur->next)
		File_Rmdir (cur->name);
}


void File_Delete_List (clist_t *list_of_urls)
{
	clist_t *cur;
	for (cur = list_of_urls; cur; cur = cur->next)
		File_Delete (cur->name);

}





///////////////////////////////////////////////////////////////////////////////
//  FILE MEMORY:  Memory_To_File and Memory_From_File
///////////////////////////////////////////////////////////////////////////////

// This keeps the changes handly.  I'm not sure what the use of that is right now though, now that I've thought it through.
// Not null proof so don't get happy yet.  Pure text files
void *File_Edit_String_Replace_Alloc (const char *path_to_file, const char *s_find, const char *s_replace, size_t *pmem_length, int *preplace_count)
{
	size_t mem_length; byte *mem_a = File_To_Memory_Alloc (path_to_file, &mem_length);
	if (!mem_a) {
		logd ("Couldn't read file " QUOTED_S ".", path_to_file);
		return NULL;
	}

	{
		int replace_count;
		char *replaced_a = String_Replace_Len_Count_Alloc (mem_a, s_find, s_replace, NULL /*don't need strlen*/, &mem_length, &replace_count);
		mem_a = core_free (mem_a);

//		Clipboard_Set_Text (replaced_a);

		// EXTRA_BYTE_NULL_ASSURANCE_ALLOC_+_1 MINUS ONE !!!!!!!
		// We know this is text.  We don't write a null terminator!!!!!!
		if (!File_Memory_To_File (path_to_file, replaced_a, mem_length - 1)) {
			logd ("Couldn't write file " QUOTED_S ".", path_to_file);
			replaced_a = core_free (replaced_a);
			return NULL;
		}

//		{
//			size_t mem_length; byte *mem_a = File_To_Memory_Alloc (path_to_file, &mem_length);
//
//			alert ("Review me");
//
//			free (mem_a);
//		}

		if (pmem_length)	*pmem_length	= mem_length;
		if (preplace_count)	*preplace_count = replace_count;
		return replaced_a;
	}
}

int File_Edit_String_Replace (const char *path_to_file, const char *s_find, const char *s_replace)
{
	int count;
	size_t mem_length; byte *mem_a = File_Edit_String_Replace_Alloc (path_to_file, s_find, s_replace, &mem_length, &count);

	if (!mem_a)
		return -1;

	mem_a = core_free (mem_a);
	return count;
}

// All files must exist or there is a message box.
int File_Edit_Mass_String_Replace (const char *path_to_folder, const char **rel_files_ray, const char **replace_tokens2)
{
	const char **curfile_rel;

	for (curfile_rel = rel_files_ray; *curfile_rel; curfile_rel ++) {
		char *path_to_file = c_strdupf ("%s/%s", path_to_folder, *curfile_rel); // text_a
		cbool ok = File_Exists (path_to_file);
		int mooo = strlen(replace_tokens2[25]);
		if (ok) {
			const char **curt2;
			for (curt2 = replace_tokens2; *curt2; curt2 +=2) {
				const char *s_find = curt2[0], *s_replace = curt2[1];
				int s_replace_len = strlen(s_replace);
				cbool g=Clipboard_Set_Text(s_replace);
				int replace_count = File_Edit_String_Replace (path_to_file, s_find, s_replace);
				switch (replace_count) {
				default:		//alert ("Did %d replacements for " QUOTED_S " on " QUOTED_S " with " QUOTED_S ".", replace_count, path_to_file, s_find, s_replace);
				case_break 0:	break; // alert ("Did %d replacements for " QUOTED_S " on " QUOTED_S " with " QUOTED_S ".", replace_count, current_file_name, s_find, s_replace);
				case_break -1:	break; //alert ("Replace fail because file not found: " QUOTED_S ".", path_to_file);
				}
			}

		}
		else log_fatal ("File '%s' not found", path_to_file);

		txtfree (&path_to_file);
	}
	return 1;
}


int File_Edit_String_Replace_Token_Array (const char *path_to_file, const char **replace_tokens2)
{
	int total_replaces = 0;
	const char **curt;
	for (curt = replace_tokens2; *curt; curt +=2 ) {
		const char *s_find = curt[0], *s_replace = curt[1];

		int replace_count = File_Edit_String_Replace (path_to_file, s_find, s_replace);

		switch (replace_count) {
		default:		logd ("Did %d replacements for " QUOTED_S " on " QUOTED_S " with " QUOTED_S ".", replace_count, path_to_file, s_find, s_replace);
		case_break 0:	break;
		case_break -1:	logd ("Replace fail because file not found: " QUOTED_S ".", path_to_file);
		}

		if (replace_count >= 0) total_replaces += replace_count;
	}

	return total_replaces;
}

cbool File_Memory_To_File (const char *path_to_file, void *data, size_t numbytes)
{
	FILE* fout = core_fopen_write (path_to_file, "wb");

	if (!fout)											return false;
	if ( fwrite (data, 1, numbytes, fout) != numbytes)	return false;

	core_fclose (fout);

	return true;
}

// Improved version of List_From_String_Lines_Alloc
clist_t *File_To_Lines_Alloc (const char *path_to_file)
{
	const char *s_file = File_To_Memory_Alloc (path_to_file, NULL);

	if (s_file)
		return List_String_Split_NewLines_Scrub_CarriageReturns_Alloc (s_file, 0 /* entire string */);
	return NULL;
}


// EXTRA_BYTE_NULL ASSURANCE ALLOC + 1
// We allocate one unnecessary byte in case we need string treatment, this isn't reflected in the size_t returned
void *File_To_Memory_Offset_Alloc (const char *path_to_file, reply size_t *numbytes, size_t offset_into_file, size_t read_len)
{
	size_t filesize, read_end;
	FILE *f;

	// If we find the file size is 0, we get out
	filesize = File_Length (path_to_file);
	if (!filesize) return NULL;

	// If len = 0 is specified, we read as much as possible
	if (read_len == 0) read_len = (filesize - offset_into_file );

	// Calc the read end, make sure file is long enough for read we want
	read_end = offset_into_file + read_len;
	if (filesize < read_end) return NULL;

	// Open file
	f = core_fopen_read (path_to_file, "rb");
	if (!f)
		return NULL;
	else
	{
		int ret = fseek (f, offset_into_file, SEEK_SET);
		byte *membuf = malloc (read_len + 1);
		size_t bytes_read = fread (membuf, 1, read_len, f);

		// If numbytes read did not meet expectations say it
		if (bytes_read != read_len) 
			logd ("bytesread != filesize: did not read entire file!");

		membuf[read_len] = 0;

		// Close file
		core_fclose (f);

		// If caller requested to know amount read, report it back
		NOT_MISSING_ASSIGN(numbytes, bytes_read);

		return membuf;
	}
}

// EXTRA_BYTE_NULL_ASSURANCE_ALLOC_+_1
// We allocate one unnecessary byte in case we need string treatment, this isn't reflected in the size_t returned
void *File_To_Memory_Alloc (const char *path_to_file, reply size_t *numbytes)
{
	return File_To_Memory_Offset_Alloc (path_to_file, numbytes, 0 /* offset of none*/, 0 /*len 0 means read all*/);

/* Obsolete but may be nice for reference because is simple
	FILE *f = core_fopen_read (path_to_file, "rb");

	if (f)
	{
		size_t	filesize	= FileHandle_GetLength (f);
		byte	*membuf		= malloc (filesize);
		size_t	bytes_read	= fread (membuf, 1, filesize, f);

		if (bytes_read != filesize)
			logd ("bytesread != filesize: did not read entire file!");

		core_fclose (f);

		if (numbytes != NULL)  // A function might not request this be written
			*numbytes = bytes_read; //filesize; // Optimistic?  What if fread didn't read whole thing (rare I know )...

		return membuf;
	}

	return NULL;
*/
}

///////////////////////////////////////////////////////////////////////////////
//  FILE HANDLE OPS:  File handle manipulation
///////////////////////////////////////////////////////////////////////////////


#if 0
// These are on ice for now.
cbool FileHandle_Lock (const char *path_to_file)
{
	return System_flock (path_to_file);

}

cbool FileHandle_Unlock (const char *path_to_file)
{
	return System_funlock (path_to_file);
}
#endif


size_t FileHandle_Block_Copy (FILE* fdst, FILE* fsrc, size_t len)
{
	char   buf[4096];
	size_t bufsize = sizeof(buf);
	size_t written, remaining, bytes_this_pass;
	size_t bytes_in, bytes_out;

	for (remaining = len, written = 0; remaining > 0; /* nothing */ )
	{
		bytes_this_pass = (remaining < bufsize) ? remaining : bufsize;

		remaining -= (bytes_in  = fread  ( buf, 1, bytes_this_pass, fsrc));  // read 4096 or less
		written   += (bytes_out = fwrite ( buf, 1, bytes_this_pass, fdst));  // write 4096 bytes or less

		if (!bytes_in || !bytes_out || bytes_in !=bytes_this_pass || bytes_out != bytes_this_pass)
		{
			logd ("Couldn't write all data");
			return 0; // Something bad happened
		}
	}

	return written;
}

size_t FileHandle_Block_To_File (FILE* fsrc, size_t len, const char *path_to_file)
{
	FILE *fdst = core_fopen_write (path_to_file, "wb");

	if (fdst)
	{
		size_t written = FileHandle_Block_Copy (fdst, fsrc, len);

		core_fclose (fdst);
		return written;
	}
	else return 0; // Error
}


size_t FileHandle_Append_File (FILE* fdst, size_t len, const char *path_to_file)
{
	FILE *fsrc = core_fopen_write (path_to_file, "rb");

	if (fsrc)
	{
		size_t written = FileHandle_Block_Copy (fdst, fsrc, len);

		core_fclose (fsrc);
		return written;
	}  else return 0; // Error
}


///////////////////////////////////////////////////////////////////////////////
//  FILE STRING OPERATIONS:  Baker - string operations for file URLs
///////////////////////////////////////////////////////////////////////////////

// If no extension, add it
void File_URL_Edit_Default_Extension (char *path_to_file, const char *dot_new_extension, size_t len)
{
	const char *extension = File_URL_GetExtension (path_to_file);

	if (extension[0] == 0)
	{
		// No extension so default it
		strlcat (path_to_file, dot_new_extension, len);
	}
}

// Removes and changes the extension if necessary
void File_URL_Edit_Change_Extension (char *path_to_file, const char *dot_new_extension, size_t len)
{
	const char *extension = File_URL_GetExtension (path_to_file);

	if (extension[0] == 0 || strcasecmp (extension, dot_new_extension))
	{
		File_URL_Edit_Remove_Extension (path_to_file);
		strlcat (path_to_file, dot_new_extension, len);
	}
}


// If no extension or the extension isn't right, add it
void File_URL_Edit_Force_Extension (char *path_to_file, const char *dot_new_extension, size_t len)
{
	const char *extension = File_URL_GetExtension (path_to_file);

	if (extension[0] == 0 || strcasecmp (extension, dot_new_extension))
	{
		// 1) No extension so default it
		// 2) Doesn't match so append it
		strlcat (path_to_file, dot_new_extension, len);
	}
}


char *File_URL_Edit_Reduce_To_Parent_Path (char* path_to_file)
{
	char* terminate_point = strrchr (path_to_file, '/');

	if (terminate_point)
		*terminate_point = '\0';

	return path_to_file;
}

char *File_URL_Edit_Remove_Any_Trailing_Slash (char *path_to_file)
{
	if (String_Does_End_With (path_to_file, "/"))
		path_to_file[strlen(path_to_file)-1] = 0; // Null it out

	return path_to_file;
}


char *File_URL_Edit_Remove_Extension (char *path_to_file)
{
	char* terminate_point = strrchr (path_to_file, '.');

	if (terminate_point)
		*terminate_point = '\0';

	return path_to_file;
}


// Turns c:\quake\id1 into c:/quake/id1
char *File_URL_Edit_SlashesForward_Like_Unix (char *windows_path_to_file)
{
	return String_Edit_Replace_Char (windows_path_to_file, '\\' /*find*/, '/' /*replace with*/, NULL /* don't want count */);
}


// Turns c:\quake\id1 into c:/quake/id1
char *File_URL_Strdup_SlashesForward_Like_Unix (const char *windows_path_to_file)
{
	char *s_o = strdup(windows_path_to_file);
	File_URL_Edit_SlashesForward_Like_Unix (s_o);

	return s_o;
}

// Turns c:/quake/id1 into c:\quake\id1
char *File_URL_Strdup_SlashesBack_Like_Windows (const char *unix_path_to_file)
{
	char *s_o = strdup(unix_path_to_file);
	File_URL_Edit_SlashesBack_Like_Windows (s_o);

	return s_o;
}

// Turns c:/quake/id1 into c:\quake\id1
char * File_URL_Edit_SlashesBack_Like_Windows (char *unix_path_to_file)
{
	// Translate "/" to "\"
	return String_Edit_Replace_Char (unix_path_to_file, '/' /*find*/, '\\' /*replace with*/, NULL /* don't want count */);
}


#if 0  // Obsolete
// These are commented out, but not sure why.  The fact they were static must mean something better replaced them.
// Looks like String_Skip_Char_Reverse replaced
static const char *sFile_URL_SkipCharReverse (const char *path_to_file, char skipme)
{
	const char *found_char = strrchr (path_to_file, skipme);

	if (found_char)
		return &found_char[1]; // path + 1

	return path_to_file; // Wasn't found
}

static const char *sFile_URL_SkipCharForward (const char *path_to_file, char skipme)
{
	const char *found_char = strchr (path_to_file, skipme);

	if (found_char)
		return &found_char[1]; // path + 1

	return path_to_file; // Wasn't found
}
#endif


// Baker: This function does unix and window style paths
void File_URL_Copy_StripExtension (char *dst, const char *src, size_t siz)
{
	int	length;

	if (!*src)
	{
		*dst = '\0';
		return;
	}
	if (src != dst)	/* copy when not in-place editing */
		strlcpy (dst, src, siz);
	length = (int)strlen(dst) - 1;
	while (length > 0 && dst[length] != '.')
	{
		--length;
		if (dst[length] == '/' || dst[length] == '\\')
			return;	/* no extension */
	}
	if (length > 0)
		dst[length] = '\0';
}

// Doesn't return NULL rather emptystring, return the dot (".png", etc ..)
const char *File_URL_GetExtension (const char *path_to_file)
{
	const char *finddot = strrchr (path_to_file, '.');

	// Make sure we found it and that what we found wasn't a dot
	// in path
	if (finddot && strchr (finddot, '/') == NULL )
	{
		return finddot;
	}

	return "";
}


const char *DOM_Return_Sequence_Alloc (const char *dom, int num) // Name sucks!
{
	const char *s_start = NULL, *s_end = NULL;
	int n;

	for (n = 0, s_end = dom; s_end && n < num; n ++) {
		// If n is 0, we aren't on the dot
		if (n > 0) s_end ++;
		s_start = s_end; //alerts (s_start);
		s_end	= strchr (s_end, '.');
	}

	if (!s_start /*never started*/ || n < num /*premature termination*/)
		return NULL;

	if (s_end)
		s_end --;

	return s_end ? String_Range_Dup_Alloc (s_start, s_end) : strdup(s_start);
}


const char *File_URL_SkipPath (const char *path_to_file)
{
	return String_Skip_Char_Reverse (path_to_file, '/');
}


const char *File_URL_SkipFirstElement (const char *path_to_file) // Name sucks!
{
	return String_Skip_Char (path_to_file, '/');
}


// in: "c:\quake\id1\frogs"  out: "id1\frogs"
const char *File_URL_SkipToLastPathElementX2 (const char *path_to_file) // Name sucks!
{
	int n;
	int s_len = strlen(path_to_file);
	int count = 0;
	for (n = s_len - 1; n >= 0; n --) {
		if (path_to_file[n] == '/') {
			count ++;
			if (count == 2)
				return &path_to_file[n + 1];
		}
	}

	return NULL; // Fail
}

char *File_URL_GetPathLastElement_Alloc (const char *path_to_file) // Name sucks!
{
	int n;
	int s_len = strlen(path_to_file);
	int first = 0; // We want second / starting from the back

	for (n = s_len - 1; n >= 0; n --) {
		if (path_to_file[n] == '/') {
			if (first)
				return strndup(&path_to_file[n + 1], n - first - 1); // We have the result.

			first = n;
		}
	}

#pragma message ("Test this part on something like mypath/bob")
	if (first)
		return strndup (path_to_file, n -1);
	return NULL; // Fail
}


///////////////////////////////////////////////////////////////////////////////
//  FILE STRING OPERATIONS:  Baker - string operations for file URLs
///////////////////////////////////////////////////////////////////////////////

clist_t *sFile_List_WildCmp_Alloc (const char *folder_url, const char *wild_pattern, cbool relative_url)
{
	// List files in folder
#pragma message ("TODO: sFile_List_Recursive_Alloc and this are about identical.  Eventually there should only be one.  sFile_List_Recursive_Alloc is recursive and this is not.")
	DIR		*dir_p = opendir (folder_url);
	clist_t *list = NULL;
	const char *full_url;

	struct dirent	*dir_t;
	clist_t *item;

	if (!dir_p)
		return NULL;

	while ((dir_t = readdir(dir_p)) != NULL)
	{
		if (dir_t->d_name[0] == '.')
			continue; // Do not want

		if (wild_pattern && !wildcompare (dir_t->d_name, wild_pattern))
			continue;

		full_url = va("%s/%s", folder_url, dir_t->d_name);

		if (File_Is_Folder (full_url))
			continue; // Just files, not folders

//		if (extension && !String_Does_End_With_Caseless(full_url, extension))
//			continue; // Not what we are looking for

		item = List_Add_No_Case_To_Lower (&list, relative_url ? dir_t->d_name : full_url);
		if (item && relative_url) {
			item->extradata = strdup(full_url); // Store full url for reference even though only relative URL requested.  Gets freed on list free.
		}
	}
	closedir(dir_p);
	return list;
}

clist_t * File_List_Alloc (const char *folder_url, const char *wild_patterns)
{
	return sFile_List_WildCmp_Alloc (folder_url, wild_patterns, false /* not relative*/ );
}

clist_t * File_List_Relative_Alloc (const char *folder_url, const char *wild_patterns)
{
	return sFile_List_WildCmp_Alloc (folder_url, wild_patterns, true /* relative url */);
}


static void sFile_List_Recursive_Alloc (clist_t **list, const char *folder_url, int skipchars, const cbool wants_relative_url, cbool wants_dirs, const char *wild_patterns)
{
	char full_url[MAX_OSPATH];

	DIR		*dir_p = opendir (folder_url);
	struct dirent	*dir_t;
	clist_t *item;

	if (dir_p)
	{
		while ( (dir_t = readdir(dir_p)) )
		{
			if (dir_t->d_name[0] == '.')
				continue; // Do not want

			if (wild_patterns && !wildcompare (dir_t->d_name, wild_patterns))
				continue;

			c_snprintf2 (full_url, "%s/%s", folder_url, dir_t->d_name);

			if (File_Is_Folder (full_url)) {
				sFile_List_Recursive_Alloc (list, full_url, skipchars, wants_relative_url, wants_dirs /* we don't want folders */, wild_patterns);
				if (!wants_dirs)
					continue; // Advance .. do not pass go
				
				item = List_Add_No_Case_To_Lower (list, &full_url[skipchars]); // Adding after should allow us to more easily remove folders.
			} else {
				// File
				item = List_Add_No_Case_To_Lower (list, &full_url[skipchars]);
			}
			if (item && wants_relative_url) {
				item->extradata = strdup(full_url); // Store full url for reference even though only relative URL requested.  Gets freed on list free.
			}
		}
		closedir(dir_p);
	}
}


clist_t *File_List_Recursive_Alloc (const char *folder_url, const char *wild_patterns)
{
	// 0123456789
	// c:/mydir  .. skip chars = 0 for full url, 9 for relative
	// 0123456789
	// c:/mydir/myfile.abc  [0] = full, [9] = myfile.abc
	clist_t *list = NULL;
	int skipchars = 0;//strlen (folder_url) + 1;

	sFile_List_Recursive_Alloc (&list, folder_url, skipchars, false /*absolute urls*/, false /* we don't want folders */, wild_patterns);

	return list;
}

clist_t *File_List_Dirs_Recursive_Alloc (const char *folder_url, const char *wild_patterns)
{
	// 0123456789
	// c:/mydir  .. skip chars = 0 for full url, 9 for relative
	// 0123456789
	// c:/mydir/myfile.abc  [0] = full, [9] = myfile.abc
	clist_t *list = NULL;
	int skipchars = 0;//strlen (folder_url) + 1;

	sFile_List_Recursive_Alloc (&list, folder_url, skipchars, false /*absolute urls*/, true /* we don't want folders */, wild_patterns);

	return list;
}


clist_t *File_List_Recursive_Relative_Alloc (const char *folder_url, const char *wild_patterns)
{
	// 0123456789
	// c:/mydir  .. skip chars = 0 for full url, 9 for relative
	// 0123456789
	// c:/mydir/myfile.abc  [0] = full, [9] = myfile.abc
	clist_t *list = NULL;
	int skipchars = strlen (folder_url) + 1;

	sFile_List_Recursive_Alloc (&list, folder_url, skipchars, true /*absolute urls*/, false /* we don't want folders */, wild_patterns);

	return list;
}


///////////////////////////////////////////////////////////////////////////////
//  FILE DIALOG OPERATIONS: PROMPT FOR A FILE OR FOLDER
///////////////////////////////////////////////////////////////////////////////

const char * File_Dialog_Open_Directory (const char *title, const char *starting_folder_url)
{
	return _Shell_Dialog_Open_Directory (title, starting_folder_url);
}

const char * File_Dialog_Open_Type (const char *title, const char * starting_folder_url, const char *extensions_comma_delimited)
{
	return _Shell_Dialog_Open_Type (title, starting_folder_url, extensions_comma_delimited);
}

const char * File_Dialog_Save_Type (const char *title, const char * starting_file_url, const char *extensions_comma_delimited)
{
	return _Shell_Dialog_Save_Type (title, starting_file_url, extensions_comma_delimited);
}





// Misc.  What about write versions?
// Write version looks like this: 	header[12] = width & 255;  header[13] = width>>8;  header[13] = width>>16;  >> 24
int fgetLittleShort (FILE *f)
{
	byte	b1, b2;
	int c;

	if ( (c = fgetc(f)) < 0) return EOF; else b1 = c;  // This is a bit flawed because EOF -1 could be a valid read value , but whatever ...
	if ( (c = fgetc(f)) < 0) return EOF; else b2 = c;

	return (short)(b1 + b2 * 256);
}


int fgetLittleLong (FILE *f)
{
	byte	b1, b2, b3, b4;
	int c;

	if ( (c = fgetc(f)) < 0) return EOF; else b1 = c;  // This is a bit flawed because EOF -1 could be a valid read value , but whatever ...
	if ( (c = fgetc(f)) < 0) return EOF; else b2 = c;
	if ( (c = fgetc(f)) < 0) return EOF; else b3 = c;
	if ( (c = fgetc(f)) < 0) return EOF; else b4 = c;

	return b1 + (b2 << 8) + (b3 << 16) + (b4 << 24);
}

int fputLittleShort (FILE *f, int v)
{
	if ( fputc ( v >>  0 & 0xFF, f) < 0) return EOF;
	if ( fputc ( v >>  8 & 0xFF, f) < 0) return EOF; // Divide by 256, AND 255
	return 0; // It's good
}


int fputLittleLong (FILE *f, int v)
{
	if ( fputc ( v >>  0 & 0xFF, f) < 0) return EOF;
	if ( fputc ( v >>  8 & 0xFF, f) < 0) return EOF; // Divide by 256, AND 255
	if ( fputc ( v >> 16 & 0xFF, f) < 0) return EOF; // Divide by 256, AND 255
	if ( fputc ( v >> 24 & 0xFF, f) < 0) return EOF; // Divide by 256, AND 255
	return 0; // It's good
}


// Memory fakers

static void smile_addbytes (MILE *m, int count)
{
	memblock_realloc_n (&m->dst, count, m->len, &m->dst_allocated, &m->dst_allocated_blocks_of_1024, MILE_BLOCK_1024);

}

static void smile_add (MILE *m, const void *data, int data_length)
{
	memblock_realloc_n (&m->dst, data_length, m->len, &m->dst_allocated, &m->dst_allocated_blocks_of_1024, MILE_BLOCK_1024);
	memcpy (&m->dst[m->len], data, data_length);
	m->len += data_length;
}

void *mopen (const void *ptr, size_t ptr_len)
{
	MILE *m =  ZeroAlloc(m);
	m->is_write = !ptr;
	switch (m->is_write) {
	default /*true*/:	smile_addbytes (m, 0); // Creates and reallocs dst pointer.
	case_break false:	m->src = ptr;
						m->len = ptr_len;
	}
	return m;
}

int mclose_transfer (MILE *m, reply void *ppacq, reply size_t *acq_len)
{
    void **acq = (void **)ppacq;
	if (acq && m->is_write) {
		*acq = realloc (m->dst, m->len + 1 ); // +1 for NULL terminator which we are assured of having, since there is always an extra 1-1024 bytes of 0 space.
		NOT_MISSING_ASSIGN (acq_len, m->len);
		m->dst = NULL, m->len = 0; // taken
	}
	freenull (m->dst); // If we allocated.
	freenull (m);
	return 0;
}

#pragma message ("Sup mclose")
int mclose (MILE *m)
{
	return mclose_transfer (m, NULL, NULL);
}

//
//	typedef struct _MILE_S
//	{
//		const byte *src;
//		int len;
//		int pos;
//	} MILE;
//
//	typedef struct _streamx {
//		int (*sgetc) (void *);
//		int (*sgetLittleShort) (void *);
//		int (*sgetLittleLong) (void *m);
//		int (*sread) (void *dst, size_t size, size_t nitems, void *v);
//		int (*stell) (void *m);
//		size_t (*swrite) ( const void * ptr, size_t size, size_t count, void *v);
//		// fprintf?
//	} stream_fns_t;

//Upon successful completion, fputc() shall return the value it has written. Otherwise, it shall return EOF
int mputc (int c, MILE *m)
{
	byte b1 = c;
	smile_add (m, &b1, 1);
	return c;
}


// Upon successful completion, these functions return the number of bytes transmitted excluding the terminating null
int mprintf (MILE *m, const char *fmt, ...)
{
	VA_EXPAND_ALLOC (text, length, bufsiz, fmt);
	smile_add (m, text, length);
	free (text);
	return length; //
}




// On success, the character read is returned (promoted to an int value).
// If the position indicator was at the end-of-file, the function returns EOF and sets the eof indicator (feof) of stream.
// If some other reading error happens, the function also returns EOF, but sets its error indicator (ferror) instead.
// EOF is basically -1
int mgetc (MILE *m)
{
	byte b;
	if (m->pos + (int)sizeof(char) > m->len)
		return EOF;
	b = m->src[m->pos++];
	return (int)b;
}



int mgetLittleShort (MILE *m)
{
	byte b1, b2;
	if (m->pos + (int)sizeof(short) > m->len)
		return EOF;
	b1 = m->src[m->pos++];
	b2 = m->src[m->pos++];

	return (short)(b1 + b2*256);
}

// Should be called int32 or something but whatever ...
int mgetLittleLong (MILE *m)
{
	byte b1, b2, b3, b4;
	if (m->pos + (int)sizeof(int) > m->len)
		return EOF;
	b1 = m->src[m->pos++];
	b2 = m->src[m->pos++];
	b3 = m->src[m->pos++];
	b4 = m->src[m->pos++];

	return b1 + (b2<<8) + (b3<<16) + (b4<<24);
}

// fread() shall return the number of elements successfully read which is less than nitems only if a read error or end-of-file is encountered.
// If size or nitems is 0, fread() shall return 0 and the contents of the array and the state of the stream remain unchanged.
// Otherwise, if a read error occurs, the error indicator for the stream shall be set, [CX] [Option Start]  and errno shall be set to indicate the error. [Option End]

// However, we are going for all or nothing here.

size_t mread (void *dst, size_t size, size_t nitems, MILE *m)
{
	int count, n;
	byte *bdst =  (byte *)dst;

	for (n = 0, count = 0; n < (int)nitems; n ++) {
		if (m->pos + (int)size > m->len)
			return count;

		memcpy (bdst, &m->src[m->pos], size);
		m->pos += size;
		bdst += size;
		count ++;
	}


	return count;
}

//If successful, the function returns zero.
//Otherwise, it returns non-zero value.
int mseek (MILE *m, int offset, int seek_type)
{
	// Remember, offset can be NEGATIVE.  In theory.
	switch (seek_type) {
	case SEEK_CUR:	if (m->pos + offset > m->len) return -1;		// Failure
					if (m->pos + offset < 0)      return -2;		// Failure other direction.  Don't know if allowed, but whatever.
					m->pos += offset;	// Current position of the file pointer
					return 0;

	case SEEK_SET:	if (0 + offset > m->len) return -1;	// Failure
					if (0 + offset < 0)      return -2;	// Failure other direction.  Don't know if allowed, but whatever.
					m->pos = offset;
					return 0;

	case SEEK_END:	if (m->len + offset > m->len) return -1;		// Failure
					if (m->len + offset < 0)      return -2;		// Failure other direction.  Don't know if allowed, but whatever.
					m->pos = m->len + offset;
					return 0;

	default:		return -1; // Something invalid
	}
}

int mtell (MILE *m)
{
	return m->pos;
}

size_t mwrite ( const void * ptr, size_t size, size_t count, MILE *m)
{
	size_t tot = size * count;
	smile_add (m, ptr, tot);
#pragma message ("Make me")
	return count;


}


// Works!
// data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQAgMAAABinRfyAAAACVBMVEXU0MiAgID///9x1mpmAAAAKUlEQVR4AXXLoRUAAAQAUTxBNIJRLME8RqdoXPjt4C9ycBpUFrYF65waSrIB583VL5QAAAAASUVORK5CYII=
// Works
// data:text/html;charset=utf-8,<h1>Object:display</h1>
// Works
// data:text/html;base64,ZnJvZw== "frog"

// My idea = ascend.png= // WHY?  I think we need to do this.  We'll find



//stream_fns_t SMILE = { mgetc, mgetLittleShort, mgetLittleLong, mread, mtell, mwrite };
//stream_fns_t SFILE = { fgetc, fgetLittleShort, fgetLittleLong, fread, ftell, fwrite };
