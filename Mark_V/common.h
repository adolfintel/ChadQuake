/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2009-2014 Baker and others

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
// common.h

#ifndef __COMMON_H__
#define __COMMON_H__

///////////////////////////////////////////////////////////////////////////////
//  Misc
///////////////////////////////////////////////////////////////////////////////

// Baker: There are link_t handling functions, relocated to world.c (only use)
typedef struct link_s
{
	struct link_s	*prev, *next;
} link_t; // used by edict_t (progs.h) and world.c

cbool COM_ListMatch (const char *liststr, const char *itemstr);

// Used by host.c to read config early
//cbool COM_Parse_Float_From_String (float *retval, const char *string_to_search, const char *string_to_find);
cbool COM_Parse_Float_From_String (float *retval, const char *string_to_search, const char *string_to_find, char *string_out, size_t string_out_size);

//============================================================================

void MSG_WriteChar (sizebuf_t *sb, int c);
void MSG_WriteByte (sizebuf_t *sb, int c);
void MSG_WriteShort (sizebuf_t *sb, int c);
void MSG_WriteLong (sizebuf_t *sb, int c);
void MSG_WriteFloat (sizebuf_t *sb, float f);
void MSG_WriteString (sizebuf_t *sb, const char *s);
void MSG_WriteStringf (sizebuf_t *sb, const char *fmt, ...); //__core_attribute__((__format__(__printf__,2,3)));  // Baker
void MSG_WriteCoord (sizebuf_t *sb, float f);
void MSG_WriteAngle (sizebuf_t *sb, float f);
void MSG_WriteAngle16 (sizebuf_t *sb, float f); //johnfitz

extern int msg_readcount;
extern cbool msg_badread; // set if a read goes beyond end of message

void MSG_BeginReading (void);
int MSG_ReadChar (void);
int MSG_ReadByte (void);
int MSG_ReadShort (void);
int MSG_ReadLong (void);
float MSG_ReadFloat (void);
char *MSG_ReadString (void);

int MSG_PeekByte (void); // ProQuake
int MSG_ReadBytePQ (void); 	// ProQuake.  ProQuake embedded messages appear to use this.
int MSG_ReadShortPQ (void);	// ProQuake.  Ping in scoreboard uses.

float MSG_ReadCoord (void);
float MSG_ReadAngle (void);
float MSG_ReadAngle16 (void); //johnfitz


///////////////////////////////////////////////////////////////////////////////
//  Bit-ordering functions: big-endian (PPC), little-endian (Intel), etc.
///////////////////////////////////////////////////////////////////////////////

extern	cbool		host_bigendian;

extern	short	(*BigShort) (short l);
extern	short	(*LittleShort) (short l);
extern	int		(*BigLong) (int l);
extern	int		(*LittleLong) (int l);
extern	float	(*BigFloat) (float l);
extern	float	(*LittleFloat) (float l);


///////////////////////////////////////////////////////////////////////////////
//  Parse
///////////////////////////////////////////////////////////////////////////////

extern int com_argc;
extern char **com_argv;

extern char	com_token[1024]; // parsing use

int COM_CheckParm (const char *parm);
const char *COM_Parse (const char *data);

void COM_Init (void);
void COM_InitArgv (int argc, char **argv);
void COM_InitFilesystem (void);

void COM_FileBase (const char *in, char *out, size_t outsize);


//============================================================================

extern int com_filesize; // Baker: Filesize of opened file, important for pak reading

// Baker: Added these for more intuitive menus to handle id1 maps and custom
// maps and demos differently.

extern int com_filesrcpak;  // 1 = file opened from a .pak, 0 = file system
extern char	com_filepath[MAX_OSPATH]; // Baker: Added, source of opened file.

extern char	com_basedir[MAX_OSPATH]; // Baker: for instance, c:\quake
extern char com_gamedir[MAX_OSPATH]; // Baker: for instance, c:\quake\travail

#ifndef SERVER_ONLY
	extern char com_safedir[MAX_OSPATH]; // Baker: for instance, c:\quake\travail
#endif // SERVER_ONLY

// Baker: This is important.  This is the starting gamedir when engine starts.
// Mark V will *ONLY* write config to this location.  So if you change gamedir
// it won't write config there.  This keeps behavior consistent for those using
// game console command mostly.  And for those using -game, same behavior as
// they expect as well.

extern char game_startup_dir[MAX_OSPATH];

// Baker: COM_FindFile is the central clearing house for all reading functions

void COM_WriteFile (const char *filename, const void *data, int len);
int COM_OpenFile (const char *filename, int *hndl);
int COM_FOpenFile (const char *filename, FILE **file);
void COM_CloseFile (int h);

// these procedures open a file using COM_FindFile and loads it into a proper
// buffer. the buffer is allocated with a total size of com_filesize + 1. the
// procedures differ by their buffer allocation method.

cbool COM_FindFile_Obsolete (const char *filename);
int COM_FindFile (const char *filename, int *handle, FILE **file, const char *media_owner_path);

int COM_FOpenFile_Limited (const char *filename, FILE **file, const char *media_owner_path);


// Baker: These load a file without any restrictions
byte *COM_LoadStackFile (const char *path, void *buffer, int bufsize);
byte *COM_LoadTempFile (const char *path);
byte *COM_LoadHunkFile (const char *path);
byte *COM_LoadMallocFile (const char *path);

// Baker: This loads a file with restrictions if media_owner_path != NULL
// The restriction is that that it will not find a file if it is higher up
// the search path.

byte *COM_LoadHunkFile_Limited (const char *path, const char *media_owner_path);

// This prevents, say, chapters/start.bsp from using id1/start.lit or id1/start.vis.

// Quakespasm was the first engine to have the feature and used it for
// colored lights.  I took the concept and built upon it, applying it to
// .vis files, .loc files, .ent files, external textures for all types of
// files and maximized the potential benefits.  Note that QIP engine might
// have had the seeds of a feature like this.

// Baker: This engine doesn't support .mp3 in pak files.  I actually could do
// it for Windows, but I have a feeling I would be hurting the future portability
// to platforms like Android or iOS or other things to come that may not have
// API so fancy as Windows.  For example, I have no idea even on a Mac how I
// could take advantage of music play for a file within a .pak and I want
// hardware acceleration.  Quakespasm does not have hardware accelerated music
// playback and slows to a crawl if you use music.

#ifdef SUPPORTS_MP3_MUSIC // Baker change
char *COM_FindFile_NoPak (const char *file_to_find);
#endif // Baker change +

///////////////////////////////////////////////////////////////////////////////
//  Gamedir and registered: Baker ... I think these need overhauled
///////////////////////////////////////////////////////////////////////////////

extern int static_registered;

extern cbool com_modified;
extern int com_hdfolder_count;


typedef enum
{
	gametype_standard = 0,
	gametype_rogue = 1,
	gametype_hipnotic = 2,
	gametype_quoth = 3,
	gametype_nehahra = 4,
	MAXGAMETYPES,
} gametype_t;

extern gametype_t com_gametype;

extern const char *gamedir_type_names[MAXGAMETYPES];

#ifdef SUPPORTS_NEHAHRA
extern cbool nehahra_active;
#endif // SUPPORTS_NEHAHRA

void COM_AddGameDirectory (const char *relative_dir, cbool hd_only);
void COM_RemoveAllPaths (void);

cbool Is_Game_Level (const char *stripped_name);

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////

void COM_Uppercase_Check (const char *name);


char *COM_CL_Worldspawn_Value_For_Key (char *source, const char *find_keyname);

#ifdef SUPPORTS_NEHAHRA
char *COM_CL_Value_For_Key_Find_Classname (const char *classname, const char *find_keyname);
#endif // SUPPORTS_NEHAHRA


#ifdef WINQUAKE_RENDERER_SUPPORT
// WinQuake
struct cache_user_s;
void COM_LoadCacheFile (const char *path, struct cache_user_s *cu);
#endif // WINQUAKE_RENDERER_SUPPORT

///////////////////////////////////////////////////////////////////////////////
//  FS REGISTER: Baker - These functions operate on a path_to_file
//  And they keep track of what is open and the file mode
//
//  INTERNAL:
///////////////////////////////////////////////////////////////////////////////

FILE *FS_fopen_write_create_path (const char *filename, const char *mode);
FILE *FS_fopen_write(const char *filename, const char *mode);
FILE *FS_fopen_read(const char *filename, const char *mode);
int FS_fclose (FILE* myfile);
void FS_List_Open_f (lparse_t *unused); // This poorly named function prints out a list of files that are open.

void FS_SafeDirClean (void);

char *qpath_to_url (const char *relative_url);
char *basedir_to_url (const char *relative_url);
char *downloads_folder_url (const char *filename_dot_zip); // travail.zip ---> quake/id1/_library/travail.zip
const char *gamedir_shortname (void);

// snprintfs (s, len, "%s/%s", com_gamedir, qpath_url)
void _FS_FullPath_From_QPath (char s[], size_t len, const char *qpath_url);
#define FS_FullPath_From_QPath(_s,_qpath) _FS_FullPath_From_QPath (_s, sizeof(_s), _qpath);
void _FS_FullPath_From_Basedir (char s[], size_t len, const char *basedirpath);
#define FS_FullPath_From_Basedir(_s,_qpath) _FS_FullPath_From_Basedir (_s, sizeof(_s), _qpath);

void COM_DeQuake_String (char *s_edit);
void COM_DeQuake_Name (char *s_edit);


typedef enum
{
	FS_STRIP_CR = 1, // Remove ending carriage returns
	FS_WHITESPACE_TO_SPACE = 2, //  chars < 32 become 32
	FS_TRIM = 4, // Trims whitespace
	FS_TEXT_TYPICAL = FS_STRIP_CR + FS_WHITESPACE_TO_SPACE + FS_TRIM,
} fs_read_flags_e;
char * FS_ReadLine (FILE *f, char *buf, size_t bufsiz, fs_read_flags_e fs_read_flags);
char * FS_ReadLine_Text_1024 (FILE *f);


clist_t *FS_File_Lines_List_Alloc (const char *path_to_file);
#define FS_File_Lines_List_Free List_Free

// Reads a clist, produces a file.  Note, there really isn't ip specific, just a list to file function actually.
void IPv4_List_To_File (const char *path_to_file, clist_t *mylist);

// Reads a file, produces a clist.  This is IP specific as only validated reads are written to the list.
clist_t *IPv4_List_From_File (const char *path_to_file);


clist_t *IPv4_List_From_String (const char *s);

// Takes a string like 192.168.1.108:64031 and fill provided buffer with 192.168.1.xxx
// If conversion fails, returns null.  Ensures standardized comparable ip format.
char *IPv4_String_Validated (char *buf, size_t bufsiz, const char *s);

// Finds a loose ip string in a list so you can provide it an unvalidated string
// like 192.168.1.108:64031 or local and it'll be fine.
cbool IPv4_List_Find (const clist_t *list, const char *unvalidated_ipstring);

#define QEVENT_DOWNLOAD_COMPLETE 1
void Q_Thread_Event_Add (int event, int code, void *id, void *data);
void Q_Thread_Events_Run (void);
void Q_Thread_Event_Dispatch (int event, int code, void *id, void *data); 
int Con_Queue_Printf (const char *fmt, ...)  __core_attribute__((__format__(__printf__,1,2)));
int Con_Queue_PrintLinef (const char *fmt, ...)  __core_attribute__((__format__(__printf__,1,2)));;
void Con_Queue_PrintRun (void /*const char *url <--- huh? */);
void ReadList_NewGame (void);
void ReadList_Ensure_Shutdown (void);

#endif // ! __COMMON_H__






