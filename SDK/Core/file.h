/*
Copyright (C) 2012-2014 Baker

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
// file.h -- file functions

#ifndef __FILE_H__
#define __FILE_H__

#include "environment.h"
#include <stdio.h> // fopen, etc.

#define NO_PATTERN_NULL NULL // No wildcard pattern

///////////////////////////////////////////////////////////////////////////////
//  FILE INFORMATION: Baker - These functions operate on a path_to_file
///////////////////////////////////////////////////////////////////////////////

cbool File_Exists (const char *path_to_file);
int File_Available_Wildcard (char *path_to_file_asterisk, size_t bufsiz, const char *fmt, ...) __core_attribute__((__format__(__printf__,3,4))) ;

cbool File_Is_Folder (const char *path_to_file);
size_t File_Length (const char *path_to_file);
double File_Time (const char *path_to_file); // Returns the seconds since midnight 1970

///////////////////////////////////////////////////////////////////////////////
//  FILE ACTION: Baker - These functions operate on a path_to_file
///////////////////////////////////////////////////////////////////////////////

cbool File_Chdir (const char *path_url); // Change directory
const char *File_Getcwd (void); // Get current directory
cbool File_Mkdir (const char *path_url); // Make directory
cbool File_Mkdir_Recursive (const char *path_to_file); // Recursive mkdir, adding trailing "/" for entire path
cbool File_Rmdir (const char* path_url); // Returns true if successful.
void File_Rmdir_List (clist_t *list_of_urls);
cbool File_Copy (const char *src_path_to_file, const char *dst_path_to_file);
cbool File_Rename (const char *path_to_file, const char *new_name_url);
cbool File_Delete (const char *path_to_file);

///////////////////////////////////////////////////////////////////////////////
//  FILE MEMORY:  Memory_To_File and Memory_From_File
///////////////////////////////////////////////////////////////////////////////

clist_t *File_To_Lines_Alloc (const char *path_to_file);

cbool File_Memory_To_File (const char *path_to_file, void *data, size_t numbytes);
void *File_To_Memory_Alloc (const char *path_to_file, reply size_t *numbytes); // EXTRA_BYTE_NULL_ASSURANCE_ALLOC_+_1 means can be treated as NULL terminated string, extra null is not part of returned length

// Zero means read everything possible
void *File_To_Memory_Offset_Alloc (const char *path_to_file, reply size_t *numbytes, size_t offset_into_file, size_t len); // EXTRA_BYTE_NULL_ASSURANCE_ALLOC_+_1 means can be treated as NULL terminated string, extra null is not part of returned length

// String replacement --- can't deal with NULL, should be fine for ASCII text files.
//void *File_Edit_String_Replace_Alloc (const char *path_to_file, const char *s_find, const char *s_replace, size_t *pmem_length); // Keep the changes.  Why?
int File_Edit_String_Replace (const char *path_to_file, const char *s_find, const char *s_replace); // Don't keep the changes.

// Return value is number of replaces?
int File_Edit_Mass_String_Replace (const char *path_to_folder, const char **rel_files_ray, const char **replace_tokens2);

int File_Edit_String_Replace_Token_Array (const char *path_to_file, const char **replace_tokens2);

///////////////////////////////////////////////////////////////////////////////
//  FILE HANDLE OPS:  File handle manipulation
///////////////////////////////////////////////////////////////////////////////

//size_t FileHandle_GetLength (FILE* filehandle);  DO NOT USE.  Use file system method File_Length

// Takes an open file and writes out to new file.  Set position before calling
size_t FileHandle_Block_To_File (FILE* fsrc, size_t len, const char *path_to_file);

// Takes an open file and writes out to new file.  Set position before calling
size_t FileHandle_Append_File (FILE* fdst, size_t len, const char *path_to_file);

// Open file to open file
size_t FileHandle_Block_Copy (FILE* fdst, FILE* fsrc, size_t len);

///////////////////////////////////////////////////////////////////////////////
//  FILE STRING OPERATIONS:  Baker - string operations for file URLs
///////////////////////////////////////////////////////////////////////////////


void File_URL_Copy_StripExtension (char *dst, const char *src, size_t siz);
const char *File_URL_GetExtension (const char *path_to_file);
void File_URL_Edit_Default_Extension (char *path_to_file, const char *dot_extension, size_t len); // This appends if there is no extension
void File_URL_Edit_Change_Extension (char *path_to_file, const char *dot_new_extension, size_t len); // This removes and appends
void File_URL_Edit_Force_Extension (char *path_to_file, const char *dot_extension, size_t len); // This appends if no extension or wrong extension
cbool File_URL_Is_Relative (const char *path_to_file);
char *File_URL_Edit_Reduce_To_Parent_Path (char *path_to_file);
char *File_URL_Edit_Remove_Extension (char *path_to_file);
char *File_URL_Edit_Remove_Any_Trailing_Slash (char *path_to_file); // Removes a trailing unix slash if found
char *File_URL_Edit_SlashesForward_Like_Unix (char *windows_path_to_file);
char *File_URL_Edit_SlashesBack_Like_Windows (char *unix_path_to_file);
#define File_URL_Has_Extension(_filename, _extension) (String_Does_End_With_Caseless(_filename, _extension)

// Duplicators
char* File_URL_Strdup_SlashesBack_Like_Windows (const char *unix_path_to_file);
char* File_URL_Strdup_SlashesForward_Like_Unix (const char *windows_path_to_file);

typedef enum
{
	image_format_invalid_0 = 0,
	image_format_best = 1, // Reserved.  Do not use
	image_format_rgba = 2, // Reserved.  Do not use
	image_format_jpg,
	image_format_png,
	image_format_tga,
//	image_format_pcx,

} image_format_e;


typedef enum
{
	source_none			= 0,
	source_file			= 'F',		// E?  Why not F?
	source_bundle		= 'B',		// Bundle
	source_memory		= 'M',		// Memory
	source_memory_rgba	= 'R',		// RAW MEMORY.  RGBA already.  No let's handle type RGB
	source_string		= 'S',		// Stupid method like base 64 string?
} source_e;

image_format_e File_URL_Image_Format (const char *path_to_file);

const char *File_URL_GetExtension (const char *path_to_file); // So we have a static extension returner but not a static path returner?  ANSWER: It's not static, it returns a pointer in the string
const char *File_URL_SkipPath (const char *path_to_file);
const char *File_URL_SkipFirstElement (const char *path_to_file); // Name sucks!
const char *File_URL_SkipToLastPathElementX2 (const char *path_to_file); // USUALLY ... File_URL_SkipPath is what you want!
																			// c:\quake\id1\pak0.pak ---> id1\pak0.pak.  This really gets a path element!
char *File_URL_GetPathLastElement_Alloc (const char *path_to_file);


///////////////////////////////////////////////////////////////////////////////
//  FILE LIST OPERATIONS:  Returns a list
///////////////////////////////////////////////////////////////////////////////


void File_Delete_List (clist_t *list_of_urls);

clist_t * File_List_Alloc (const char *folder_url, const char *wild_patterns);

// Note for relative, the full url is in item->extradata = strdup(full_url); 
clist_t * File_List_Relative_Alloc (const char *folder_url, const char *wild_patterns);
clist_t * File_List_Recursive_Alloc (const char *folder_url, const char *wild_patterns); // No extension yet!  Is ok, will use pattern later.
clist_t * File_List_Recursive_Relative_Alloc (const char *folder_url, const char *wild_patterns); // No extension yet!


clist_t * File_List_Dirs_Recursive_Alloc (const char *folder_url, const char *wild_patterns);

///////////////////////////////////////////////////////////////////////////////
//  FILE DIALOG OPERATIONS: PROMPT FOR A FILE OR FOLDER
///////////////////////////////////////////////////////////////////////////////

const char * File_Dialog_Open_Directory (const char *title, const char *starting_folder_url);
const char * File_Dialog_Open_Type (const char *title, const char * starting_folder_url, const char *extensions_comma_delimited);

// starting_file_url is default save name, add a "/" merely to suggest starting folder
const char * File_Dialog_Save_Type (const char *title, const char * starting_file_url, const char *extensions_comma_delimited);

///////////////////////////////////////////////////////////////////////////////
//  Misc
///////////////////////////////////////////////////////////////////////////////

const char *DOM_Return_Sequence_Alloc (const char *dom, int num);

#ifdef PLATFORM_WINDOWS
	// http://stackoverflow.com/questions/735126/are-there-alternate-implementations-of-gnu-getline-interface (#include <stdio.h>)
	size_t getline(char **lineptr, size_t *n, FILE *stream);
#endif // PLATFORM_WINDOWS

// Misc.  What about write versions?
// Write version looks like this: 	header[12] = width & 255;  header[13] = width>>8;  header[13] = width>>16;  >> 24

// This is pointless but good quick reference as to how we need to do things.
//	typedef struct _streamx {
//		void *(*sopen) (const void *filename, const char *mode); // Mode starts read or write
//		int (*sgetc) (void *); // return non-negative number on success, EOF (-1) on failure
//		int (*sgetLittleShort) (void *);
//		int (*sgetLittleLong) (void *);
//		int (*sread) (void *dst, size_t size, size_t nitems, void *v);
//		int (*stell) (void *m);
//		size_t (*swrite) ( const void * ptr, size_t size, size_t count, void *v);
//
//		int (*sprintf) (void *, const char *, ...);   //c_len_strdupf(&len
//		int (*sputc) (int, void *);
//		//
//		int (*sputLittleShort) (void *); // Upon successful completion, fputs() shall return a non-negative number. Otherwise, it shall return EOF
//		int (*sputLittleLong) (void *);
//		int (*sclose) (void *v);
//		// fprintf?
//	} stream_fns_t;



int fgetLittleShort (FILE *f); // We don't have an error mechanism for this.  And would be too weird.  fgetc turns -1 on failure, could be valid.
int fgetLittleLong (FILE *f);
int fputLittleShort (FILE *f, int v); // Return 0 on ok, -1 on failure
int fputLittleLong (FILE *f, int v); // Return on ok, -1 on failure


///////////////////////////////////////////////////////////////////////////////
//  MEMORY FAKERS:  File operation functions that instead work on a memory block.
///////////////////////////////////////////////////////////////////////////////


// Mem read --- bad home but whatever
typedef struct _MILE_S
{
	int			is_write;
	const byte *src;
	byte		*dst;

	int			len;
	int			pos;
	// For dest we would need to alloc a buffer and keep reallocing it.  Mostly for frwite but what about fprintf?
	// Shittily we could use txtcat as cheap and dirty way to do this.
	// But that would be remarkably shitty
	size_t		dst_allocated_blocks_of_1024;
	size_t		dst_allocated;
} MILE;
#define MILE_BLOCK_1024 1024

// ONCE AND FOR ALL
//void memblock_realloc_n (void **p /* void *pp */, int addbytes, int curbytes, int *alloced_bytes, int *alloced_blocks, int blocksize);

// Memory fakers
void *mopen (const void *ptr, size_t ptr_len); // NULL to create


// Read
int mseek (MILE *m, int offset, int seek_type);
int mtell (MILE *m);
int mgetc (MILE *m);
size_t mread(void *dst, size_t size, size_t nitems, MILE *m);


// Write
int mputc (int c, MILE *m); // Mirrors stupid order that fputc uses
int mgetLittleShort (MILE *m);
int mgetLittleLong (MILE *m);

int mprintf (MILE *m, const char *, ...) __core_attribute__((__format__(__printf__,2,3))) ;   //c_len_strdupf(&len
size_t mwrite ( const void * ptr, size_t size, size_t count, MILE *m); // MAKE ME!


int mclose (MILE *m);
int mclose_transfer (MILE *m, void *ppacq /* void **acq*/, size_t *acq_len);


#endif	// ! __FILE_H__


