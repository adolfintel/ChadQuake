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
// file_system.c -- Windows platform interface

#define CORE_LOCAL
#include "core.h"

#ifdef PLATFORM_WINDOWS
	#include "core_windows.h" // <windows.h> // GetCurrentDirectory, etc.
//	#include <direct.h> // _chdir, etc.
//	#include <sys/types.h>
//	#include <sys/stat.h> // _stat, etc.
	#include <Shlwapi.h>  // PathIsRelative function

	#pragma comment (lib, "Shlwapi.lib")  // PathIsRelative function

	#define chdir 	_chdir
	#define mkdir 	_mkdir
	#define rmdir 	_rmdir
	#define stat 	_stat

#endif // PLATFORM_WINDOWS

#ifdef CORE_SDL
	#include "core_sdl.h"
#endif


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: FILE AND DIRECTORY MANAGEMENT
///////////////////////////////////////////////////////////////////////////////

#ifdef PLATFORM_WINDOWS
// http://stackoverflow.com/questions/735126/are-there-alternate-implementations-of-gnu-getline-interface (#include <stdio.h>)
size_t getline(char **lineptr, size_t *n, FILE *stream)
{
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;

    if (lineptr == NULL) {
    	return -1;
    }
    if (stream == NULL) {
    	return -1;
    }
    if (n == NULL) {
    	return -1;
    }
    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) {
    	return -1;
    }
    if (bufptr == NULL) {
    	bufptr = malloc(128);
    	if (bufptr == NULL) {
    		return -1;
    	}
    	size = 128;
    }
    p = bufptr;
    while(c != EOF) {
		if ((p - bufptr) > (int)(size - 1)) { // Added (int) cast.  2015 Sept. 2
    		size = size + 128;
    		bufptr = realloc(bufptr, size);
    		if (bufptr == NULL) {
    			return -1;
    		}
    	}
    	*p++ = c;
    	if (c == NEWLINE_CHAR_10) {
    		break;
    	}
    	c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;

    return p - bufptr - 1;
}
#endif // PLATFORM_WINDOWS


cbool File_Chdir (const char *path_url)
{
	return (chdir (path_url) == 0); // chdir returns 0 on success.  Our function returns true on success.
}

//Get current working directory.
const char *File_Getcwd (void)
{
	static char workingdir[MAX_OSPATH];
	int ok = 0;

#ifdef PLATFORM_WINDOWS
	// Fails = returns 0
	if (GetCurrentDirectory (sizeof(workingdir), workingdir)) {
		if (workingdir[strlen(workingdir)-1] == '/')
			workingdir[strlen(workingdir)-1] = 0;

		File_URL_Edit_SlashesForward_Like_Unix (workingdir);
		ok = 1;
	}
#else
    if (getcwd (workingdir, sizeof(workingdir) - 1))
		ok = 1;
#endif

	if (!ok)
		log_fatal ("Couldn't determine current directory");

	return workingdir;
}


cbool File_Rename (const char *path_to_file, const char *new_name_url)
{
	cbool success = (rename(path_to_file, new_name_url) == 0);

	if (success == false)
		logd ("Error in renaming file."); // Whether or not this prints is up to app, we supply a return value, that's our role.

	return success;
}


cbool File_Copy (const char *src_path_to_file, const char *dst_path_to_file)
{
	// Has to copy the file without changing the date
#pragma message ("TODO: File_Copy messes up the date and loads to memory on non-Windows fixme")
#pragma message ("TODO: Note folder dates will probably be changed. ")
	if (!File_Exists (src_path_to_file))
		return false;

	block_start__

#ifdef PLATFORM_WINDOWS
	// More complicated than it should be.
	// CopyFile: Requires target directory to exist.
	cbool ret;
	File_Mkdir_Recursive (dst_path_to_file);
	// CopyFile doesn't preserve date modified  Neither does Ex
	// CopyFileEx (src_path_to_file, dst_path_to_file, NULL, NULL, NULL, 0);

	//return CopyFile (src_path_to_file, dst_path_to_file, false /*don't fail if file exists*/) != 0;
	ret = CopyFile (src_path_to_file, dst_path_to_file, false /*don't fail if file exists*/) != 0;

#if 0 // TURNS out I don't need this.  File copy really does set the date and time.
	if (ret) {
		text_a *winnameA = File_URL_Edit_SlashesBack_Like_Windows(strdup(src_path_to_file));
		text_a *winnameB = File_URL_Edit_SlashesBack_Like_Windows(strdup(dst_path_to_file));
		HANDLE hFileA = CreateFile(winnameA, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		HANDLE hFileB = CreateFile(winnameB, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

		FILETIME ftCreate, ftAccess, ftWrite;
		cbool ok = true;

		if (hFileA == INVALID_HANDLE_VALUE)								ok = false, logd ("CreateFile '%s' failed with %d", src_path_to_file, GetLastError());
		if (hFileB == INVALID_HANDLE_VALUE)								ok = false, logd ("CreateFile '%s' failed with %d", src_path_to_file, GetLastError());

		// Retrieve the file times for the file.
		if (ok && !GetFileTime(hFileA, &ftCreate, &ftAccess, &ftWrite))	ok = false, logd ("Could GetFileTime on %s", src_path_to_file);
		if (ok && !SetFileTime(hFileB, &ftCreate, &ftAccess, &ftWrite)) ok = false, logd ("Could SetFileTime on %s", src_path_to_file);

		if (hFileA != INVALID_HANDLE_VALUE)  CloseHandle(hFileA);
		if (hFileB != INVALID_HANDLE_VALUE)  CloseHandle(hFileB);
		freenull (winnameA);
		freenull (winnameB);
	}
#endif

#else
	// Stupid method for now
	cbool ret;
	size_t mem_length; byte *mem = File_To_Memory_Alloc (src_path_to_file, &mem_length);
	if (!mem)
		return false;
	ret = File_Memory_To_File (dst_path_to_file, mem, mem_length);
	freenull (mem);


#endif
	return ret;
	__block_end

}


cbool File_Mkdir (const char *path_url)
{
	int ret;

	// If it exists, no point in trying to make it
	if (File_Exists(path_url)) {
		if (!File_Is_Folder (path_url)) {
			logd ("mkdir warning: " QUOTED_S " exists and is a file", path_url);
			return false;
		}

        logd (SPRINTSFUNC "Folder %s already exists", __func__, path_url);
		return true; // Already exists.
	}

   // Check attributes to make sure 777?

	// mkdir returns 0 on success, -1 on failure
#ifdef PLATFORM_WINDOWS
	ret = mkdir (path_url) == 0;

	if (!ret)
		logd ("mkdir " QUOTED_S " failed", path_url);
#else
    ret = mkdir (path_url, 0777) == 0;

	if (!ret && errno != EEXIST) {
        logd ("mkdir " QUOTED_S " failed, reason: " QUOTED_S ".", path_url, strerror (errno));
		return false;
    }
#endif

	return ret;
}

cbool File_Delete (const char *path_to_file)
{
	cbool success = remove(path_to_file) != -1;

	return success;
}

cbool File_Rmdir (const char *path_url)
{
	return !rmdir (path_url); // rmdir returns 0 if ok
}


cbool File_Exists (const char *path_to_file)
{
	struct stat st_buf = {0};
	int status = stat (path_to_file, &st_buf);

	if (status != 0)
		return false;

	return true;
}


cbool File_Is_Folder (const char *path_to_file)
{
#ifdef PLATFORM_WINDOWS
// Requires at least Windows XP
//	#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
	DWORD attributes = GetFileAttributes (path_to_file);
	if (attributes & FILE_ATTRIBUTE_DIRECTORY)
		return true;

	if (INVALID_FILE_ATTRIBUTES)
		return false;
#else
    struct stat st_buf = {0};
	int status = stat (path_to_file, &st_buf);

	if (status != 0)
		return false;

    if (S_ISREG (st_buf.st_mode))
		return false; // Is regular file ...

    if (S_ISDIR (st_buf.st_mode))
        return true; // directory
#endif
	return false;
}


// File length
size_t File_Length (const char *path_to_file)
{
	struct stat st_buf = {0};
	int status = stat (path_to_file, &st_buf );
	if (status != 0)
		return 0;

	return st_buf.st_size;
}


// Returns the seconds since midnight 1970
double File_Time (const char *path_to_file)
{
	struct stat st_buf = {0};

	int status = stat (path_to_file, &st_buf );
	if (status != 0)
		return 0;

	return (double)st_buf.st_mtime;
}


cbool File_URL_Is_Relative (const char *path_to_file)
{
#ifdef PLATFORM_WINDOWS
// Not sure I trust this entirely.  Does it handle Unix paths ok?
	return PathIsRelative (path_to_file);
#else
	return path_to_file[0] != '/'; // Right?  That's it isn't it.
#endif
}

///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: ROGUES - Sleep and ???
//  There is only the Windows and non-Windows way for these ...
///////////////////////////////////////////////////////////////////////////////



void Platform_Sleep_Milliseconds (int milliseconds)
{
#ifdef CORE_SDL
		SDL_Delay (milliseconds);
#else
	#ifdef PLATFORM_WINDOWS
		// Windows sleep function works in milliseconds
		Sleep (milliseconds);
	#else
		// Usleep works in microseconds, not milliseconds
		usleep (milliseconds * 1000);
	#endif
#endif // !CORE_SDL
}


// Hmmm.  Alt name, some advantages and some disadvantages.
void System_Sleep_Milliseconds (int milliseconds)
{
	Platform_Sleep_Milliseconds (milliseconds);
}


#if 0
// These aren't defined in opengl32.dll!!!
void *Windows_ProcAddress (const char *windows_dll_name, const char *fn_name)
{
	sys_handle_t handle_dll = LoadLibrary(windows_dll_name);
	void *funcaddress = NULL;

	if (handle_dll) {
		funcaddress = GetProcAddress(handle_dll, fn_name);
		if (!funcaddress)
			alert ("Couldn't find fn address %s in %s", fn_name, windows_dll_name);


		FreeLibrary (handle_dll);


	}
	else alert ("Couldn't open %s", windows_dll_name);
	return funcaddress;
}
#endif

#ifndef PLATFORM_WINDOWS // The one environment that doesn't use this
void *_Platform_GetProcAddress (const char *pfunction_name)
{
#ifdef CORE_SDL
    void *psymbol = SDL_GL_GetProcAddress (pfunction_name);
#else
    void *psymbol = dlsym (RTLD_DEFAULT, pfunction_name);
#endif

//	    if (psymbol == NULL)
//	    {
//	        log_fatal ("Failed to import a required function!");
//	    }

    return psymbol;
}
#endif // !PLATFORM_WINDOWS





