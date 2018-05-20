/*
Copyright (C) 2011-2014 Baker

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
// core.h -- core functions

// For all functions, it is probably preferable to do this for pointers --->
// char * myfunction ();
// Allows whole word search of "char *" in most text editors.
// The preferred "char *myfunction ();" doesn't allow a whole word search of "char *"

/* NOT True Core ...
core_opengl.h
gl_constants.h
libcurl.dll
math_matrix.c
math_vector.c
unzip_win.cpp
unzip_win.h
zip_win.cpp
zip_win.h
zlib1.dll
*/

// Understanding platform code ...
//
// FILE						TYPE
// -------------------		-------------------
// file_system.c			THERE ARE ONLY 2 WAYS TO DO THESE:  The Unix way or the Windows way.
//							Either an application uses Unix file system or Windows file system.  c:\frogs.txt vs. frogs.txt
//							Either an application uses usleep (microseconds) or Sleep (milliseconds)
//							Either an application uses Unix dirent or Windows equivalent.
//							Either an application uses pthreads, fork, dlsym for GetProcAddress, etc.
//
// shell_osx.m, etc.		THERE ARE COUNTLESS WAYS THESE (OR NOT AT ALL!) AND EVEN A CONSOLE APP HAS THE VARIATION NEEDS THEM (timer, clipboard)
//							SDL DOESN'T OFFER REPLACEMENTS FOR HARDLY ANY OF THESE.
//							These are truly platform specific and not generally SDL territory.
//							Open an explorer folder
//							Bundle work, File Dialog, system timer, put image on clipboard, high precision timer.
//
// vid_osx.m				THERE ARE COUNTLESS WAYS TO DO THESE.  SDL SUPPORT IS AVAILABLE.  VIDEO.

#ifndef __CORE_H__
#define __CORE_H__

#ifdef __GNUC__
	#pragma GCC diagnostic ignored "-Wmissing-field-initializers" // Not a fan of disabling this but messes with struct x mystruct = {0};
#endif // __GNUC__

// #define CORE_LIBCURL

/*
** Allocations in Core SHALL be made with C allocators calloc, free, etc. (*)
**
** Note that Corex and Corex_GL extensions to Core follow different rules.
**
** (*) In part because allocations in core shouldn't be mixed with tracked allocations by the application.
*/


#define __CORE_INCLUDED__

// it's in environment.h
//#if defined (DEBUG) && !defined (_DEBUG)
//	#define _DEBUG
//#endif // Keep everything consistent across platforms

#include "environment.h"

#if defined(_CONSOLE) && defined(PLATFORM_OSX)
//	#include <CoreFoundation/CoreFoundation.h>
#endif
#if defined(PLATFORM_OSX) || defined(PLATFORM_IOS)
	#include <mach/mach_time.h>
#endif


#ifdef __OBJC__ // The Mac
	#ifndef _CONSOLE
		#ifdef PLATFORM_OSX
			#import <Cocoa/Cocoa.h> // core_mac.h sort of
			#import <ApplicationServices/ApplicationServices.h>
#ifndef PLATFORM_OSX // Crusty Mac
			#import "VidWindow.h"
#endif // Crusty Mac
		#endif

		#ifdef PLATFORM_IOS
			#import <GLKit/GLKit.h>
			#import <UIKit/UIKit.h>
#ifndef PLATFORM_OSX // Crusty Mac
			#import "VidUIWindow.h"
#endif  // Crusty Mac
		#endif

	#endif
#endif

#include <stdio.h> // fopen, etc.
#include <stdlib.h> // malloc, etc.

//#include <math.h> // Environment.h already includes this for M_PI defined check
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h> // _stat, etc.
#include <time.h>
#include <ctype.h> // islower, etc.
#include <stddef.h> // offsetof
#include <stdarg.h> // GCC wanted this
#include <assert.h>

#define member_size(type, member) sizeof(((type *)0)->member)

#ifndef _MSC_VER // About everything else needs this header
    #ifdef PLATFORM_WINDOWS
        // MinGW doesn't have alloca.h so we will define it ourselves.
        void *alloca(size_t size);
    #else
        #include <alloca.h>
    #endif
#endif


#ifdef PLATFORM_WINDOWS
	#include <io.h>
	#include <direct.h> // _chdir, etc.
	#include "dirent_win.h" // dirent.h for Windows
#endif // PLATFORM_WINDOWS

#ifndef PLATFORM_WINDOWS
	#include <unistd.h> // read
#include <dirent.h>
#include <dlfcn.h>
	#include <fcntl.h>
#endif // ! PLATFORM_WINDOWS

#ifdef CORE_PTHREADS
#include "pthreads_core.h"
#endif // PTHREADS

#ifdef PLATFORM_ANDROID
	#include <android/log.h>
#endif // PLATFORM_ANDROID

// DON'T DEFINE THIS UNTIL AFTER SYSTEM LIBRARIES!!!
#ifndef reply
	#define reply // We use this as a hint keyword in conjunction with NOT_MISSING_ASSIGN  (optional is vague, a reply is always optional)
#endif // We can't include core.h

//#ifndef advised // Better name?  Can always search and replace
//	#define advised // We use this as a hint keyword, meaning optional (KILL THIS)
//#endif // We can't include core.h

// REPLY is generally write only
// OPTIONAL is generally read only ?   USE OF OPTIONAL IS A FAILURE EXCEPT WHEN PROVIDING AN OPTIONAL START POINT OR SOMETHING.
// REQUIRED notes it is not optional just for clarity.
// What about a guaranteed reply?  So caller knows a variable need not be reset?

// "Optional" conveys no information other than it can be omitted.  Not very precise.
// Doesn't say communicate any expections of read or write or if changes or stays the same.

//#ifndef optional // Better name?  Can always search and replace
//	#define optional // We use this as a hint keyword, meaning optional (KILL THIS)
//#endif // We can't include core.h

#ifndef required // A pointer sent that is not optional
	#define required
#endif // We can't include core.h

#ifndef modify // A pointer sent that is not optional
	#define modify
#endif // We can't include core.h

typedef int (*errorline_fn_t) (const char *error, ...) __core_attribute__((__format__(__printf__,1,2), __noreturn__));
typedef int (*printline_fn_t) (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));


#include "lists.h"
#include "file.h"
//#include "image.h" // Moved to below function declares ...
#include "interface.h"
#include "math_general.h"
#include "math_vector.h"
#include "math_matrix.h"
#include "color.h"
#include "pak.h"
#include "stringlib.h"

#include "stick.h"

#include "base64.h"
#include "zip.h"
// #include "download.h" // Moved to below function declares
#include "enumbits.h"
#include "timelib.h"
#include "music.h"
#include "links.h"
#include "memchain.h"

///////////////////////////////////////////////////////////////////////////////
//  CORE: Basic function setup
///////////////////////////////////////////////////////////////////////////////


#define KEYMAP_COUNT_512 512
#define NUM_MOUSE_BUTTONS_5 5
extern int keymap [KEYMAP_COUNT_512][5]; // Decimal representation, Platform constant, Our constant, Special instruction, Special instruction value
extern keyvalue_t key_scancodes_table [108];

#define CORE_KEYMAP_EMISSION_2 2 		// The key event came from local interpretation (character emission), not scancode.
#define CORE_SHIFTBITS_UNREAD_NEG1 -1 	// We did not read the shiftbits, this value must be ignored by receiver.


#define K_SHIFT_MASK_1		1
#define K_CTRL_MASK_2		2
#define K_ALT_MASK_4		4
#define K_COMMAND_MASK_8	8

#define K_MOUSE_1_MASK_1	1
#define K_MOUSE_2_MASK_2	2
#define K_MOUSE_3_MASK_4	4
#define K_MOUSE_4_MASK_8	8
#define K_MOUSE_5_MASK_16	16

// Make a 2 value table.
// Quake name	Core Name	 //
// K_INS		= K_INSERT	 // Do this to get the constants the same value assured.
							 // Remember the Quake constants names don't matter at all
							// Because K_INS isn't the same as the bind name "INS"
							// Same with K_DEL and "DEL"
// So one step could be to knock off keys.h
// And Core keyvalue_t, fuck that.  Should be what it says, key and value.
// Multiple table stuff, figure out some other name for it.

#ifdef PLATFORM_OSX // Crusty Mac
	typedef int key_scancode_e;
#else
	typedef enum {
		__UNUSED_K			= -1,   // Ensure MinGW makes us an int type and not an unsigned type
		K_INVALID_0			= 0,
		K_BACKSPACE         = 8,
		K_TAB               = 9,
		K_ENTER             = 13,	// Are we not able to detect the keypad enter?  Yes?  No?  Some operating systems yes/some no?  Grrr.
		K_ESCAPE            = 27,	// Remember that scan code is different from keymap.  It may keymap as enter, but it should scan code as kp_enter
		K_SPACE             = 32,	// Anyways ...
		K_APOSTROPHE        = 39,
		K_COMMA             = 44,
		K_MINUS             = 45,
		K_PERIOD            = 46,
		K_SLASH             = 47,
		K_SEMICOLON         = 59,
	
		K_EQUALS            = 61,
		K_LEFTBRACKET       = 91,
		K_BACKSLASH         = 92,
		K_RIGHTBRACKET      = 93,
		K_GRAVE             = 96,
	
		K_LCTRL             = 128,
		K_RCTRL             = 129,
		K_LALT              = 130,
		K_RALT              = 131,
		K_LSHIFT            = 132,
		K_RSHIFT            = 133,
		K_LWIN              = 134,
		K_RWIN              = 135,
		K_MENU              = 136,
		K_CAPSLOCK          = 137,
		K_NUMLOCK           = 138,
		K_SCROLLLOCK        = 139,
		K_PAUSE             = 140,
	
		// RESERVED: K_BREAK           = 141, ?  Or sysreq or who knows.
	
		K_PRINTSCREEN       = 142,
		K_INSERT            = 143,
		K_DELETE            = 144,
		K_LEFTARROW         = 145,
		K_RIGHTARROW        = 146,
		K_UPARROW           = 147,
		K_DOWNARROW         = 148,
		K_PAGEUP            = 149,
		K_PAGEDOWN          = 150,
		K_HOME              = 151,
		K_END               = 152,
		K_F1                = 153,
		K_F2                = 154,
		K_F3                = 155,
		K_F4                = 156,
		K_F5                = 157,
		K_F6                = 158,
		K_F7                = 159,
		K_F8                = 160,
		K_F9                = 161,
		K_F10               = 162,
		K_F11               = 163,
		K_F12               = 164,
	
		K_NUMPAD_0          = 177,		// BEGIN: These may do a keyboard emission ... (Numlock on presumably)
		K_NUMPAD_1          = 178,
		K_NUMPAD_2          = 179,
		K_NUMPAD_3          = 180,
		K_NUMPAD_4          = 181,
		K_NUMPAD_5          = 182,
		K_NUMPAD_6          = 183,
		K_NUMPAD_7          = 184,
		K_NUMPAD_8          = 185,
		K_NUMPAD_9          = 186,
		K_NUMPAD_MULTIPLY   = 187,
		K_NUMPAD_PLUS       = 188,
		K_NUMPAD_SEPARATOR  = 189,
		K_NUMPAD_MINUS      = 190,
		K_NUMPAD_PERIOD     = 191,
		K_NUMPAD_DIVIDE     = 192,		// END: These may do a keyboard emission
		K_NUMPAD_ENTER      = 193,
	
	// Plenty of extra space from 194 to 255 for future oddball keys
	
		K_MOUSE1            = 256,
		K_MOUSE2            = 257,
		K_MOUSE3            = 258,
		K_MOUSE4            = 259,
		K_MOUSE5            = 260,
	// RESERVED: Possible Extra mouse buttons
		K_MOUSEWHEELUP      = 264,
		K_MOUSEWHEELDOWN    = 265,
	// Future, right and up wheel?  A Mac touch pad simulates thoses.  Not sure, a Mac touch pad jumbles the ideas of dragging with scrolling.
	// And we probably wouldn't treat that as a button --- there's no pressed or released state of a gesture.
	
	//	K_MOUSEWHEELLEFT    = 266,
	//	K_MOUSEWHEELRIGHT   = 267,
		K_JOY1              = 268,
		K_JOY2              = 269,
		K_JOY3              = 270,
		K_JOY4              = 271,
		K_AUX1              = 272,
		K_AUX2              = 273,
		K_AUX3              = 274,
		K_AUX4              = 275,
		K_AUX5              = 276,
		K_AUX6              = 277,
		K_AUX7              = 278,
		K_AUX8              = 279,
		K_AUX9              = 280,
		K_AUX10             = 281,
		K_AUX11             = 282,
		K_AUX12             = 283,
		K_AUX13             = 284,
		K_AUX14             = 285,
		K_AUX15             = 286,
		K_AUX16             = 287,
		K_AUX17             = 288,
		K_AUX18             = 289,
		K_AUX19             = 290,
		K_AUX20             = 291,
		K_AUX21             = 292,
		K_AUX22             = 293,
		K_AUX23             = 294,
		K_AUX24             = 295,
		K_AUX25             = 296,
		K_AUX26             = 297,
		K_AUX27             = 298,
		K_AUX28             = 299,
		K_AUX29             = 300,
		K_AUX30             = 301,
		K_AUX31             = 302,
		K_AUX32				= 303,
	// Reserve a block starting at 384 for custom stuff?
	} key_scancode_e;
#endif  // ! Crusty Mac

//
// Mighty fucking Apple specific here aren't we?  Is a problem?
//
typedef enum {
	device_type_unknown,
	device_type_iphone,
	device_type_ipodtouch,
	device_type_ipad,
} device_type_e;

typedef double ticktime_t; // Better than having a fucking double that doesn't convey how the time is stored.

typedef enum {
	_os_enum_fake		= - 1,
	os_enum_invalid_0	= 0,
	os_enum_windows,
	os_enum_mac,
	os_enum_linux,
	os_enum_iphone,
	os_enum_android,
} os_enum_e;

///////////////////////////////////////////////////////////////////////////////
//  CORE: ...
///////////////////////////////////////////////////////////////////////////////
#include "memstick.h"
///////////////////////////////////////////////////////////////////////////////
//  CORE: Basic function setup
///////////////////////////////////////////////////////////////////////////////


typedef FILE * (*fopenread_fn_t) (const char *, const char *);
typedef FILE * (*fopenwrite_fn_t) (const char *, const char *);
typedef int (*fclose_fn_t) (FILE*);

typedef void * (*malloc_fn_t) (size_t);
typedef void * (*calloc_fn_t) (size_t, size_t);
typedef void * (*realloc_fn_t)(void *, size_t);
typedef char * (*strdup_fn_t) (const char*);
typedef void (*free_fn_t)(void *);


typedef struct
{
	errorline_fn_t	ferrorline_fn;		// log_fatal.  Unrecoverable error.
	printline_fn_t	fdprintline_fn;		// log_debug.  Function for uncategorized and non-assured debug messages.
										// logd - stripped messages
	printline_fn_t	fprintlinelevel_fn;	// log_level_5.  Something that really wants to print must receive a my_printline

//	printline_fn_t	fprint_fn;			// logc - application defined "general print".  Which an application may discard.
//	error_fn_t		ferror_fn;			// log_abort.  THEORETICAL Cleanup and lngjmp level, theoretical.

	malloc_fn_t		fmalloc_fn;
	calloc_fn_t		fcalloc_fn;
	realloc_fn_t	frealloc_fn;
	strdup_fn_t		fstrdup_fn;

	free_fn_t		ffree_fn;

	fopenread_fn_t	ffopenread_fn;
	fopenwrite_fn_t	ffopenwrite_fn;
	fclose_fn_t		ffclose_fn;
} fn_set_t;


// This function should be culled by the compiler in any given source file if nothing uses it.
static void *__passthru (void *v, int set)
{
	static void *stored;
	if (set)
		stored = v;

	return stored;
}

#define OR_DIE(_x, _msg) __passthru( (void *)_x, 1) ? __passthru( NULL, 0) : (void *)log_fatal ("%s\n%s", _msg,  __func__)

// Won't appear in a release build
#define c_assert(_x) assert (!!OR_DIE((_x), "Assertion failure"  )) // Generic_StdError_Printf_NoReturn

// Initializer, application passes function set.  appname is important
// and affects appdata folder names and window titles.

#include "clip.h" // Should be area.h ?  rect.h?
#include "image.h" // Moved to below function declares ...
#include "download.h" // Moved to below function declares ...
#include "vidco.h"

#ifdef CORE_SDL
	#include "core_sdl.h"
	typedef void (*plat_dispatch_fn_t) (SDL_Event *sdl_event);
#else
	#define plat_dispatch_fn_t void *  // LAZY
#endif


void Core_Init (const char *appname, fn_set_t *fnset, sys_handle_t handle );


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: MESSAGEBOX
///////////////////////////////////////////////////////////////////////////////

#define alert1(s) alert("%s", s)
int alert (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2))) ;
int alerti (double d);

// I had this as unlimited.
#define MAX_MSGBOX_TEXT_SIZE_BECAUSE_UNLIMITED_IS_SLOW_2048 2048
int msgbox (const char *_title, const char *fmt, ...)	__core_attribute__((__format__(__printf__,2,3))) ;
char *inputbox_alloc (sys_handle_t parent_window, const char *prompt, const char *default_text);
char *inputbox_multiline_alloc (sys_handle_t parent_window, const char *prompt, const char *default_text);
char *prompt_alloc (const char *prompt, const char *default_text, const char *qualifier, cbool is_abortable);

///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: PROCESSES
///////////////////////////////////////////////////////////////////////////////

sys_handle_t System_Process_Create (const char *path_to_file, const char *args, const char *working_directory_url);
int System_Process_Still_Running (sys_handle_t pid);
int System_Process_Terminate_Console_App (sys_handle_t pid);
int System_Process_Close (sys_handle_t pid);


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: EVENTS
///////////////////////////////////////////////////////////////////////////////

void System_Sleep_Milliseconds (int milliseconds);
void Platform_Sleep_Milliseconds (int milliseconds);
double Platform_MachineTime (void); // // no set metric

void Platform_Events_SleepForInput (required sys_handle_t *phandle_tevent, double max_wait_seconds);
cbool Platform_Events_Do (plat_dispatch_fn_t dispatcher_function);


// #define FSOUND_GETFUNC(f, g) (qFSOUND_##f = (void *)GetProcAddress(fmod_handle, "_FSOUND_" #f #g))
// #define FSOUND_GETFUNC(f, g) (qFSOUND_##f = (void *)dlsym(fmod_handle, "FSOUND_" #f))

#define ZeroFormat(_p) memset (_p, 0, sizeof(*(_p))) // Assert size isn't sizeof (void *)
#define ZeroAlloc(_p) calloc (sizeof(*(_p)), 1)

#define ZeroCreate(_p) (_p) = calloc (sizeof(*(_p)), 1) // I am opposite of freenull, pretty much
#define ZeroDestroy freenull

#define MemDup(_p) core_memdup(_p, sizeof(*(_p)))
#define MemCmp(_a,_b) memcmp(_a, _b, sizeof(*(_b)))
#define MemCpy(px,py) memcpy ((px), (py), sizeof(*(py)))


void *memimem (const void *buf, size_t buf_len, const void *byte_sequence, size_t byte_sequence_len);
void *memichr (const void *buf, int c, size_t buf_len);

#ifdef CORE_NEEDS_MEMMEM // BSD license http://opensource.apple.com//source/Libc/Libc-825.40.1/string/FreeBSD/memmem.c
void *memmem (const void *haystack, size_t haystacklen, const void *needle, size_t needlelen);
#endif // CORE_NEEDS_MEMMEM



size_t memcmp0 (const void *v, size_t len);

#define freenull(x) if (x) (x) = core_free ((x)) // I can handle const void fine.
void *core_free (const void *ptr); // Returns null
void *core_realloc_zero (const void *ptr, size_t new_size, size_t old_size); // Formats new block
void *core_memdup (const void *src, size_t len);
void *core_memdup_z (const void *src, size_t len, required size_t *bufsize_made); // Null terminating + 1 copy writing out bufsize
void *core_memcpy_z(void *dest, const void *src, size_t siz);

char *core_strndup (const char *s, size_t n);

FILE *core_fopen_write_create_path (const char *path_to_file, const char *mode);

#ifdef CORE_LOCAL

	///////////////////////////////////////////////////////////////////////////////
	//  CORE: Private Local Shared
	///////////////////////////////////////////////////////////////////////////////
	
		char gCore_Appname[SYSTEM_STRING_SIZE_1024];
	sys_handle_t gCore_hInst; // Why is this a pointer?  Undid that.
	sys_handle_t gCore_Window; // DO NOT LIKE
	
	extern fopenread_fn_t	core_fopen_read;
	extern fopenwrite_fn_t	core_fopen_write;
	extern fclose_fn_t		core_fclose;
	
	extern malloc_fn_t		core_malloc;
	extern calloc_fn_t		core_calloc;
	extern realloc_fn_t		core_realloc;
	extern strdup_fn_t		core_strdup;
	extern free_fn_t		_core_free;
	
	extern errorline_fn_t	log_fatal; // Terminate
	extern printline_fn_t	log_debug; // Secondary notification
	extern printline_fn_t	log_level_5; // Secondary notification

	//void *core_free (const void* ptr); // Returns null  .. keep or no?

#ifdef _DEBUG
	#define logd log_debug
#else
	#define logd // Not in release build
#endif

	///////////////////////////////////////////////////////////////////////////////
	//  PLATFORM PRIVATE
	///////////////////////////////////////////////////////////////////////////////

	char *_Platform_Clipboard_Get_Text_Alloc (void);
	cbool _Platform_Clipboard_Set_Text (const char *text_to_clipboard);
	int _Platform_MessageBox (const char *title, const char *text);
	char *_Platform_Text_Dialog_Popup_Alloc (sys_handle_t parent_window, const char *prompt, const char *text, cbool _is_multiline);
	
	///////////////////////////////////////////////////////////////////////////////
	//  SYSTEM OS: URL QUERY
	///////////////////////////////////////////////////////////////////////////////
	
	//const char * System_URL_Binary (void);
	//const char * System_URL_Binary_Folder (void);
	const char *_Shell_Folder_Caches_By_AppName (const char *appname);
	//void *_Shell_Bundle_Load_Alloc (void);
    const void *Shell_Data_From_Resource (size_t *mem_length, cbool *must_free);
	
	///////////////////////////////////////////////////////////////////////////////
	//  SYSTEM OS: FILE DIALOGS PROMPT FOR A FILE OR FOLDER
	///////////////////////////////////////////////////////////////////////////////
	
	const char *_Shell_Folder_Caches_By_AppName (const char *appname);

//	const void *_Shell_Bundle_Load_Alloc (void);
//	const void *_Shell_Bundle_Free (void *pack);

	cbool _Shell_Clipboard_Set_Image_RGBA (const unsigned *rgba, int width, int height);
	unsigned *_Shell_Clipboard_Get_Image_RGBA_Alloc (int *outwidth, int *outheight);

	const char *_Shell_Dialog_Open_Directory (const char *title, const char *starting_folder_url);
	const char *_Shell_Dialog_Open_Type (const char *title, const char * starting_folder_url, const char *extensions_comma_delimited);
	const char *_Shell_Dialog_Save_Type (const char *title, const char * starting_folder_url, const char *extensions_comma_delimited);

	cbool _Shell_Folder_Open_Folder_Must_Exist (const char *path_to_file);
	cbool _Shell_Folder_Open_Highlight_URL_Must_Exist (const char *path_to_file);
	
	///////////////////////////////////////////////////////////////////////////////
	//  SYSTEM OS: FILE DIALOGS PROMPT FOR A FILE OR FOLDER
	///////////////////////////////////////////////////////////////////////////////
	
	/*
	const char * System_Dialog_Open_Directory (const char *title, const char *starting_folder_url);
	
	const char * System_Dialog_Open_Type (const char *title, const char * starting_folder_url, const char *extensions_comma_delimited);
	
	
	// starting_file_url is default save name, add a "/" merely to suggest starting folder
	const char * System_Dialog_Save_Type (const char *title, const char * starting_file_url, const char *extensions_comma_delimited);
	*/

	// Sound

	void Shell_Play_Wav_File (const char *path_to_file);
	//void Sound_Play_Wav_Memory (const void *wav_pointer, size_t wav_length);
	void Shell_Play_Wav_Bundle (const char *path_to_file);
	void Sound_Play_Stop (void);
	//cbool Sound_Play_Wav_MemoryEx (const char *tag, const void *in_wav_pointer, size_t in_wav_length);
	
	
	///////////////////////////////////////////////////////////////////////////////
	//  SYSTEM OS: FILE MANAGER INTERACTION
	///////////////////////////////////////////////////////////////////////////////
	
	//cbool System_Folder_Open (const char *path_url);
	cbool _Shell_Folder_Open_Folder_Must_Exist (const char *path_to_file);
	cbool _Shell_Folder_Open_Highlight_URL_Must_Exist (const char *path_to_file);


	///////////////////////////////////////////////////////////////////////////////
	//  SYSTEM OS: FILE AND DIRECTORY MANAGEMENT
	///////////////////////////////////////////////////////////////////////////////
	
	/*void System_chdir (const char *path_url); // change dir
	const char *System_getcwd (void); // current working directory
	void System_mkdir (const char *path_url); // make dir
	int System_rmdir (const char *path_url); // remove dir
	cbool System_File_Exists (const char *path_to_file); // file existence
	cbool System_File_Is_Folder (const char *path_to_file);
	size_t System_File_Length (const char *path_to_file);
	double System_File_Time (const char *path_to_file);
	cbool System_File_URL_Is_Relative (const char *path_to_file);*/
	
	///////////////////////////////////////////////////////////////////////////////
	//  SYSTEM OS: CLIPBOARD OPERATIONS
	///////////////////////////////////////////////////////////////////////////////
	
	//#define SYS_CLIPBOARD_SIZE_256 256
	//const char *System_Clipboard_Get_Text_Line (void);
	//const char *System_Clipboard_Get_Text_Alloc (void);
	//cbool System_Clipboard_Set_Text (const char *text_to_clipboard);
	//cbool System_Clipboard_Set_Image_RGBA (const unsigned *rgba, int width, int height);
	//unsigned *System_Clipboard_Get_Image_RGBA_Alloc (int *outwidth, int *outheight);

#endif // CORE_LOCAL

///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: INPUT SHELL
///////////////////////////////////////////////////////////////////////////////

cbool Shell_Input_KeyBoard_Capture (cbool bDoCapture, cbool ActOnStickeyKeys, cbool bSuppressWindowskey);
void Shell_Input_ResetDeadKeys (void); // Windows
void Shell_Platform_Init_DPI (void);
void Shell_Platform_Init_Timer_Sleep (void);

#endif // ! __CORE_H__



