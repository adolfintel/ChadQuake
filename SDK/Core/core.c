/*
Copyright (C) 2011-2016 Baker

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
// core.c -- core functions


#include "core.h"


///////////////////////////////////////////////////////////////////////////////
//  CORE: Basic function setup
///////////////////////////////////////////////////////////////////////////////

const char * const empty_string = "";


fopenread_fn_t		core_fopen_read;
fopenwrite_fn_t 	core_fopen_write;

fclose_fn_t			core_fclose;

malloc_fn_t			core_malloc;
calloc_fn_t			core_calloc;
realloc_fn_t		core_realloc;
strdup_fn_t			core_strdup;
free_fn_t			_core_free; // Because real core_free returns NULL

// I do not think the continuator versions should be supported at all.
// It is unformed and incomplete line, the application should complete the line
// And the application usually does.
// If continuator versions are good idea, why doesn't a msgbox support them?  See!


errorline_fn_t		log_fatal;		// log_fatal
printline_fn_t		log_debug;		// 
printline_fn_t		log_level_5;		//
// Never logc, always my_printline
 
//error_fn_t			log_abort;		// log_fatal  (THEORETICAL)
//printline_fn_t		Core_Warning;	// Endangered in favor logd?  
									// Why?  Because what is a warning?
									// No definition.
									// An ultimate an application decides how it is presented.
									// And on the flip side log_debug says what errors.

//printline_fn_t		Core_DPrintf;	// to become log hush or something

//printline_fn_t		logc;			// general user print.  Somewhat endangered.;
//printline_fn_t		loghush;		// application conditional level print
//printline_fn_t		logd;			// Pure DEBUG only printing removed at RELEASE compilation time.

char gCore_Appname[SYSTEM_STRING_SIZE_1024];
sys_handle_t gCore_hInst;
sys_handle_t gCore_Window; // Focus window really.  I DO NOT LIKE THIS.  Works ok for single instance of Window.

#ifdef PLATFORM_WINDOWS
#include "core_windows.h" // GetModuleHandle
#endif // PLATFORM_WINDOWS
/*
typedef struct
{
	errorline_fn_t	ferrorline_fn;		// log_fatal.  Unrecoverable error.
	printline_fn_t	fdprintline_fn;		// log_debug.  Function for uncategorized and non-assured debug messages.
									// logd - stripped messages
	printline_fn_t	fprintlinelevel_fn;	// log_level_5.  Something that really wants to print must receive a my_printline
//	printline_fn_t	fprint_fn;		// logc - application defined "general print".  Which an application may discard.
//	error_fn_t		ferror_fn;		// log_abort.  THEORETICAL Cleanup and lngjmp level, theoretical.

	malloc_fn_t		fmalloc_fn;
	calloc_fn_t		fcalloc_fn;
	realloc_fn_t	frealloc_fn;
	strdup_fn_t		fstrdup_fn;

	free_fn_t		ffree_fn;

	fopenread_fn_t	ffopenread_fn;
	fopenwrite_fn_t	ffopenwrite_fn;
	fclose_fn_t		ffclose_fn;
} fn_set_t;
*/



static fn_set_t default_function_set = { 
	perrorlinef, 	// ferror_fn
//	printlinef, 	// fwarning_fn		// Core_Warning
	printlinef, 	// fdprint_fn
	printlinef, 	// flevelprint_fn

	malloc, 		// fmalloc_fn
	calloc, 		// fcalloc_fn
	realloc, 		// frealloc_fn
	strdup, 		// fstrdup_fn
	free, 			// ffree_fn
	fopen, 			// ffopenread_fn
	fopen, 			// ffopenwrite_fn
	fclose,			// ffclose_fn
} ;

void Core_Init (const char *appname, fn_set_t *fnset, sys_handle_t pmainwindow )
{
	if (!fnset) fnset = &default_function_set;
	c_strlcpy (gCore_Appname, appname);
	gCore_Window = pmainwindow;

#if defined(PLATFORM_WINDOWS) && !defined(CORE_SDL)
	gCore_hInst = GetModuleHandle(NULL);

	{
		void Vidco_Local_InitOnce_Windows (void);
		Vidco_Local_InitOnce_Windows (); // Hook up default GL if applicable
	}
#endif // defined(PLATFORM_WINDOWS) && !defined(CORE_SDL)





	log_fatal	= fnset->ferrorline_fn;
	log_debug	= fnset->fdprintline_fn;
	log_level_5	= fnset->fprintlinelevel_fn;
//	Core_Warning= fnset->fwarning_fn;
//	Core_Printf	= fnset->fprint_fn;
//	Core_DPrintf= fnset->fdprint_fn;

//	error_fn_t		ferror_fn;		// log_fatal.  Unrecoverable error.
//	printline_fn_t	fdprintline_fn;		// log_debug.  Function for uncategorized and non-assured debug messages.
									// logd - stripped messages
//	printline_fn_t	fprintlinelevel_fn;	// log_level.  Something that really wants to print must receive a my_printline


//error_fn_t			log_fatal;		// log_fatal
//printline_fn_t		log_debug;		// 
//printline_fn_t		log_level;		//

	core_malloc	= fnset->fmalloc_fn;
	core_calloc	= fnset->fcalloc_fn;
	core_realloc= fnset->frealloc_fn;
	core_strdup	= fnset->fstrdup_fn;
	_core_free	= fnset->ffree_fn;

	core_fopen_read	= fnset->ffopenread_fn;
	core_fopen_write = fnset->ffopenwrite_fn;
	core_fclose	= fnset->ffclose_fn;

#pragma message ("gCache folder -- Determine cache folder and get a session id.  Lock the session file in there fcntl.flock(fd, fcntl.LOCK_EX) or ")
//  logd ("Cache folder")
	/*
HANDLE WINAPI CreateFile(
  _In_      LPCTSTR lpFileName,
  _In_      DWORD dwDesiredAccess,
  _In_      DWORD dwShareMode,  <-------------------------- set to 0 and we have exclusive
  _In_opt_  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  _In_      DWORD dwCreationDisposition,
  _In_      DWORD dwFlagsAndAttributes,
  _In_opt_  HANDLE hTemplateFile
);*/
	logd ("Core initialized");
}


// Finds first non-zero
/* memicmp.c (emx/gcc) -- Copyright (c) 1990-1992 by Eberhard Mattes */

#include <string.h>
#include <ctype.h>

#ifdef CORE_NEEDS_MEMICMP
int memicmp (const void *s1, const void *s2, size_t n)
{
	size_t i;
	int d;

	for (i = 0; i < n; ++i)
		{
		d = tolower (((unsigned char *)s1)[i]) -
			tolower (((unsigned char *)s2)[i]);
		if (d != 0)
			{
			if (d > 0)
				return (1);
			else
				return (-1);
			}
		}
	return (0);
}
#endif // CORE_NEEDS_MEMICMP

#ifdef CORE_NEEDS_MEMMEM
/*
 * Find the first occurrence of the byte string s in byte string l.
 */

void *
memmem(const void *l, size_t l_len, const void *s, size_t s_len)
{
	register char *cur, *last;
	const char *cl = (const char *)l;
	const char *cs = (const char *)s;

	/* we need something to compare */
	if (l_len == 0 || s_len == 0)
		return NULL;

	/* "s" must be smaller or equal to "l" */
	if (l_len < s_len)
		return NULL;

	/* special case where s_len == 1 */
	if (s_len == 1)
		return memchr(l, (int)*cs, l_len);

	/* the last position where its possible to find "s" in "l" */
	last = (char *)cl + l_len - s_len;

	for (cur = (char *)cl; cur <= last; cur++)
		if (cur[0] == cs[0] && memcmp(cur, cs, s_len) == 0)
			return cur;

	return NULL;
}

#endif // CORE_NEEDS_MEMMEM

#if 1

// XMemString.cpp  Version 1.0
// Public domain.

//////////////////////////////////////////////////////////////////////////////
//
// memichr()
//
// Purpose:     Find character in a buffer (case insensitive)
//
// Parameters:  buf      - pointer to buffer
//              c        - character to look for
//              buf_len  - size of buffer in bytes
//
// Returns:     void *   - if successful, returns a pointer to the first
//                         occurrence of c in buf;  otherwise, returns NULL
//
// Notes;       memichr() will search by ignoring the case of those characters
//              that fall in the ANSI range a-z and A-Z.
//
void *memichr(const void *buf, int c, size_t buf_len)
{
    byte *p = (byte *) buf;
    byte b_lower = (byte) c;
    byte b_upper = (byte) c;
    size_t i;

    if ((b_lower >= 'A') && (b_lower <= 'Z'))
        b_lower = (byte) (b_lower + ('a' - 'A'));

    if ((b_upper >= 'a') && (b_upper <= 'z'))
        b_upper = (byte) (b_upper - ('a' - 'A'));

    for (i = 0; i < buf_len; i++)
    {
        if ((*p == b_lower) || (*p == b_upper))
            return p;
        p++;
    }
    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//
// memimem()
//
// Purpose:     Find a byte sequence within a memory buffer (case insensitive)
//
// Parameters:  buf               - pointer to buffer
//              buf_len           - size of buffer in bytes
//              byte_sequence     - byte sequence to look for
//              byte_sequence_len - size of byte sequence in bytes
//
// Returns:     void * - if successful, returns a pointer to the first
//                       occurrence of byte_sequence in buf;  otherwise,
//                       returns NULL
//
// Notes;       memimem() will search by ignoring the case of those characters
//              that fall in the ANSI range a-z and A-Z.
//
void *memimem(const void *buf,
              size_t buf_len,
              const void *byte_sequence,
              size_t byte_sequence_len)
{
    byte *bf = (byte *)buf;
    byte *bs = (byte *)byte_sequence;
    byte *p  = bf;

    while (byte_sequence_len <= (buf_len - (p - bf)))
    {
        size_t b = *bs & 0xFF;
        if ((p = (byte *) memichr(p, b, buf_len - (p - bf))) != NULL)
        {
            if ((memicmp(p, byte_sequence, byte_sequence_len)) == 0) // _memicmp
                return p;
            else
                p++;
        }
        else
        {
            break;
        }
    }
    return NULL;
}

#endif //

size_t memcmp0 (const void *v, size_t len)
{
	const byte *mem = v;
	size_t n;
	for (n = 0; n < len; n ++) {
		if (mem[n]) {
			return n;
		}
	}
	return 0;
}

void *core_memdup (const void *src, size_t len)
{
	void *buf = core_malloc (len); // Because we are a wrapper
	memcpy (buf, src, len);
	return buf;
}



// Allocates len + 1 and copies, ensuring null termination.
void *core_memdup_z (const void *src, size_t len, required size_t *bufsize_made)
{	REQUIRED_ASSIGN (bufsize_made, len + 1); {
	void *buf = calloc (1, *bufsize_made); // Because we are a wrapper
	memcpy (buf, src, len);
	return buf;
}}

// Copies a block, null at last position.
void *core_memcpy_z(void *dest, const void *src, size_t siz)
{
	memcpy (dest, src, siz);  BYTE_POSITION(dest, siz - 1) = 0;
	return dest;
}

FILE *core_fopen_write_create_path (const char *path_to_file, const char *mode)
{
	FILE *f = fopen (path_to_file, mode);

	// If specified, on failure to open file for write, create the path
	if (!f)
	{
		File_Mkdir_Recursive (path_to_file);
		f = fopen (path_to_file, mode); // fopen OK
	}

	return f;
}


char *core_strndup (const char *s, size_t n)
{
	return strndup (s, n);
}


void *core_free (const void *ptr)
{
	_core_free ((void*)ptr);

	return NULL;
}


void *core_realloc_zero (const void *ptr /*liar*/, size_t new_size, size_t old_size)
{
	ptr = realloc ( (void *)ptr, new_size);
	if (new_size > old_size)
		memset ((byte *)ptr + old_size, 0, new_size - old_size); // Format new block
	return (void *)ptr;
}

#pragma message ("Update the cache marker")
//void Core_Heartbeat (

// Why are we including headers this deep into a c file?  Was this a test?

//#ifdef CORE_SDL
//    #ifdef _MSC_VER // Somewhat stupidly, there are different libraries for Visual Studio vs. MinGW
                    // And ... the include is different, haha :(
//#pragma message ("MSC_VER")
//        #include <SDL.h>	    // Visual Studio SDL2 is SDL.h
//    #else
//        #include <SDL2/SDL.h>	// MinGW + Linux SDL2 is SDL2/SDL.h
//    #endif
//#endif

