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
// common.c -- misc functions used in client and server

#include "quakedef.h"
#include "common.h" // courtesy
#include <errno.h>

///////////////////////////////////////////////////////////////////////////////
//  Misc
///////////////////////////////////////////////////////////////////////////////

// Finds match to whole word within a string (comma and null count as delimiters)
cbool COM_ListMatch (const char *liststr, const char *itemstr)
{
	const char	*start, *matchspot, *nextspot;
	int		itemlen;

	// Check for disqualifying situations ...
	if (liststr == NULL || itemstr == NULL || *itemstr == 0 || strchr (itemstr, ','))
		return false;

	itemlen = strlen (itemstr);

	for (start = liststr ; (matchspot = strstr (start, itemstr)) ; start = nextspot)
	{
		nextspot = matchspot + itemlen;

		// Validate that either the match begins the string or the previous character is a comma
		// And that the terminator is either null or a comma.  This protects against partial
		// matches

		if ((matchspot == start || *(matchspot - 1) == ',') && (*nextspot == 0 || *nextspot == ','))
			return true; // matchspot;
	}

	return false;
}

// Baker: Used to read config early; lots of work to look around the file and find information.
cbool COM_Parse_Float_From_String (float *retval, const char *string_to_search, const char *string_to_find, char *string_out, size_t string_out_size)
{
	int beginning_of_line, value_spot, value_spot_end;
	int spot, end_of_line, i;
	char *cursor;

	cursor = strstr(string_to_search, string_to_find);

	while (1)
	{
		if (cursor == NULL)
			return false; // Didn't find it.

		spot = cursor - string_to_search; // Offset into string.

		// Find beginning of line.  Go from location backwards until find newline or hit beginning of buffer

		for (i = spot - 1, beginning_of_line = -1 ; i >= 0 && beginning_of_line == -1; i-- )
			if (string_to_search[i] == '\n')
				beginning_of_line = i + 1;
			else if (i == 0)
				beginning_of_line = 0;

		if (beginning_of_line == -1)
			return false; // Didn't find beginning of line?  Errr.  This shouldn't happen

		if (beginning_of_line != spot)
		{
			// Be skeptical of matches that are not beginning of the line
			// These might be aliases or something and the engine doesn't write the config
			// in this manner.  So advance the cursor past the search result and look again.
			cursor = strstr(cursor + strlen(string_to_find), string_to_find);
			continue; // Try again
		}

		break;
	}

	// Find end of line. Go from location ahead of string and stop at newline or it automatically stops at EOF

	for (i = spot + strlen(string_to_find), end_of_line = -1; string_to_find[i] && end_of_line == -1; i++ )
		if (string_to_search[i] == '\r' || string_to_search[i] == '\n')
			end_of_line = i - 1;

	if (end_of_line == -1) // Hit null terminator
		end_of_line = strlen(string_to_search) - 1;

	// We have beginning of line and end of line.  Go from spot + strlen forward and skip spaces and quotes.
	for (i = spot + strlen(string_to_find), value_spot = -1, value_spot_end = -1; i <= end_of_line && (value_spot == -1 || value_spot_end == -1); i++)
		if (string_to_search[i] == ' ' || string_to_search[i] == '\"')
		{
			// If we already found the start, we are looking for the end and just found it
			// Otherwise we are just skipping these characters because we ignore them
			if (value_spot != -1)
				value_spot_end = i - 1;

		}
		else if (value_spot == -1)
			value_spot = i; // We didn't have the start but now we found it

	// Ok check what we found

	if (value_spot == -1)
		return false; // No value

	if (value_spot_end == -1)
		value_spot_end = end_of_line;

	do
	{
		// Parse it and set return value
		char temp = string_to_search[value_spot_end + 1];
		char *temptermspot = (char*)&string_to_search[value_spot_end + 1]; // slightly evil

		//string_to_search[value_spot_end + 1] = 0;
		*temptermspot = 0;
		strlcpy (string_out, &string_to_search[value_spot], string_out_size);
		*retval = atof (&string_to_search[value_spot]);
		*temptermspot = temp;

	} while (0);

	return true;
}



/*

All of Quake's data access is through a hierchal file system, but the contents
of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all
game directories.  The sys_* files pass this to host_init in quakeparms_t->basedir.
This can be overridden with the "-basedir" command line parm to allow code
debugging in a different directory.  The base directory is only used during
filesystem initialization.

The "game directory" is the first tree on the search path and directory that all
generated files (savegames, screenshots, demos, config files) will be saved to.
This can be overridden with the "-game" command line parameter.  The game
directory can never be changed while quake is executing.  This is a precacution
against having a malicious server instruct clients to write files over areas they
shouldn't.

The "cache directory" is only used during development to save network bandwidth,
especially over ISDN / T1 lines.  If there is a cache directory specified, when
a file is found by the normal search path, it will be mirrored into the cache
directory, then opened there.

FIXME:
The file "parms.txt" will be read out of the game directory and appended to the
current command line arguments to allow different games to initialize startup
parms differently.  This could be used to add a "-sspeed 22050" for the high
quality sound edition.  Because they are added at the end, they will not
override an explicit setting on the original command line.

*/


/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

//
// writing functions
//

void MSG_WriteChar (sizebuf_t *sb, int c)
{
	byte    *buf;

#ifdef PARANOID
	if (c < -128 || c > 127)
		System_Error ("MSG_WriteChar: range error");
#endif

	buf = (byte *) SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteByte (sizebuf_t *sb, int c)
{
	byte    *buf;

#ifdef PARANOID
	if (c < 0 || c > 255)
		System_Error ("MSG_WriteByte: range error");
#endif

	buf = (byte *) SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteShort (sizebuf_t *sb, int c)
{
	byte    *buf;

#ifdef PARANOID
	if (c < ((short)0x8000) || c > (short)0x7fff)
		System_Error ("MSG_WriteShort: range error");
#endif

	buf = (byte *) SZ_GetSpace (sb, 2);
	buf[0] = c&0xff;
	buf[1] = c>>8;
}

void MSG_WriteLong (sizebuf_t *sb, int c)
{
	byte    *buf;

	buf = (byte *) SZ_GetSpace (sb, 4);
	buf[0] = c&0xff;
	buf[1] = (c>>8)&0xff;
	buf[2] = (c>>16)&0xff;
	buf[3] = c>>24;
}

void MSG_WriteFloat (sizebuf_t *sb, float f)
{
	union
	{
		float   f;
		int     l;
	} dat;

	dat.f = f;
	dat.l = LittleLong (dat.l);

	SZ_Write (sb, &dat.l, 4);
}

void MSG_WriteString (sizebuf_t *sb, const char *s)
{
	if (!s)
		SZ_Write (sb, "", 1);
	else
		SZ_Write (sb, s, strlen(s)+1);
}

void MSG_WriteStringf (sizebuf_t *sb, const char *fmt, ...) // __core_attribute__((__format__(__printf__,2,3)));
{
	VA_EXPAND (text, MAXPRINTMSG_4096, fmt); // cls.spawnparms is
	MSG_WriteString (sb, text);
}



//johnfitz -- original behavior, 13.3 fixed point coords, max range +-4096
void MSG_WriteCoord16 (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, c_rint(f*8));
}

//johnfitz -- 16.8 fixed point coords, max range +-32768
void MSG_WriteCoord24 (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, f);
	MSG_WriteByte (sb, (int)(f*255)%255);
}

//johnfitz -- 32-bit float coords
void MSG_WriteCoord32f (sizebuf_t *sb, float f)
{
	MSG_WriteFloat (sb, f);
}

void MSG_WriteCoord (sizebuf_t *sb, float f)
{
	MSG_WriteCoord16 (sb, f);
}

void MSG_WriteAngle (sizebuf_t *sb, float f)
{
	MSG_WriteByte (sb, c_rint(f * 256.0 / 360.0) & 255); //johnfitz -- use Q_rint instead of (int)
}

//johnfitz -- for PROTOCOL_FITZQUAKE
void MSG_WriteAngle16 (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, c_rint(f * 65536.0 / 360.0) & 65535);
}
//johnfitz

//
// reading functions
//
int                     msg_readcount;
cbool        msg_badread;

void MSG_BeginReading (void)
{
	msg_readcount = 0;
	msg_badread = false;
}

// returns -1 and sets msg_badread if no more characters are available
int MSG_ReadChar (void)
{
	int     c;

	if (msg_readcount+1 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = (signed char)net_message.data[msg_readcount];
	msg_readcount++;

	return c;
}

int MSG_ReadByte (void)
{
	int     c;

	if (msg_readcount+1 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = (unsigned char)net_message.data[msg_readcount];
	msg_readcount++;

	return c;
}

int MSG_PeekByte (void)
{
	if (msg_readcount+1 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	return (unsigned char)net_message.data[msg_readcount];
}

// JPG - added this
int MSG_ReadBytePQ (void)
{
 	// Baker I wish I knew the reason for the 272:  256 + 16.  Subtract the first read by 17?  Why?
	// I suppose we'll have to watch it in real time. :(
	return MSG_ReadByte() * 16 + MSG_ReadByte() - 272;
}

// JPG - added this
int MSG_ReadShortPQ (void)
{
	return MSG_ReadBytePQ() * 256 + MSG_ReadBytePQ();
}

int MSG_ReadShort (void)
{
	int     c;

	if (msg_readcount+2 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = (short)(net_message.data[msg_readcount]
	+ (net_message.data[msg_readcount+1]<<8));

	msg_readcount += 2;

	return c;
}

int MSG_ReadLong (void)
{
	int     c;

	if (msg_readcount+4 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = net_message.data[msg_readcount]
	+ (net_message.data[msg_readcount+1]<<8)
	+ (net_message.data[msg_readcount+2]<<16)
	+ (net_message.data[msg_readcount+3]<<24);

	msg_readcount += 4;

	return c;
}

float MSG_ReadFloat (void)
{
	union
	{
		byte    b[4];
		float   f;
		int     l;
	} dat;

	dat.b[0] =      net_message.data[msg_readcount];
	dat.b[1] =      net_message.data[msg_readcount+1];
	dat.b[2] =      net_message.data[msg_readcount+2];
	dat.b[3] =      net_message.data[msg_readcount+3];
	msg_readcount += 4;

	dat.l = LittleLong (dat.l);

	return dat.f;
}

char *MSG_ReadString (void)
{
	static char     string[2048];
	int		c;
	size_t		l;

	l = 0;
	do
	{
		c = MSG_ReadChar ();
		if (c == -1 || c == 0)
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string)-1);

	string[l] = 0;

	return string;
}

//johnfitz -- original behavior, 13.3 fixed point coords, max range +-4096
float MSG_ReadCoord16 (void)
{
	return MSG_ReadShort() * (1.0/8);
}

//johnfitz -- 16.8 fixed point coords, max range +-32768
float MSG_ReadCoord24 (void)
{
	return MSG_ReadShort() + MSG_ReadByte() * (1.0/255);
}

//johnfitz -- 32-bit float coords
float MSG_ReadCoord32f (void)
{
	return MSG_ReadFloat();
}

float MSG_ReadCoord (void)
{
	return MSG_ReadCoord16();
}

float MSG_ReadAngle (void)
{
	return MSG_ReadChar() * (360.0/256);
}

//johnfitz -- for PROTOCOL_FITZQUAKE
float MSG_ReadAngle16 (void)
{
	return MSG_ReadShort() * (360.0 / 65536);
}
//johnfitz


/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

cbool host_bigendian;

short   (*BigShort) (short l);
short   (*LittleShort) (short l);
int     (*BigLong) (int l);
int     (*LittleLong) (int l);
float   (*BigFloat) (float l);
float   (*LittleFloat) (float l);

short ShortSwap (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short ShortNoSwap (short l)
{
	return l;
}

int LongSwap (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int LongNoSwap (int l)
{
	return l;
}

float FloatSwap (float f)
{
	union
	{
		float   f;
		byte    b[4];
	} dat1, dat2;


	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

float FloatNoSwap (float f)
{
	return f;
}

///////////////////////////////////////////////////////////////////////////////
//  Parse
///////////////////////////////////////////////////////////////////////////////

static char *largv[MAX_NUM_Q_ARGVS_50 + 1];
static char argvdummy[] = " ";

cbool com_modified;   // set true if using non-id files
int com_hdfolder_count;
char game_startup_dir[MAX_OSPATH];

struct searchpath_s	*com_base_searchpaths;	// without id1 and its packs

int static_registered = 1;  // only for startup check, then set

// if a packfile directory differs from this, it is assumed to be hacked
#define PAK0_COUNT		339	/* id1/pak0.pak - v1.0x */
#define PAK0_CRC_V100		13900	/* id1/pak0.pak - v1.00 */
#define PAK0_CRC_V101		62751	/* id1/pak0.pak - v1.01 */
#define PAK0_CRC_V106		32981	/* id1/pak0.pak - v1.06 */
#define PAK0_CRC	(PAK0_CRC_V106)
#define PAK0_COUNT_V091		308	/* id1/pak0.pak - v0.91/0.92, not supported */
#define PAK0_CRC_V091		28804	/* id1/pak0.pak - v0.91/0.92, not supported */

char	com_token[1024];
int		com_argc;
char	**com_argv;

char	com_cmdline[MAX_CMD_256];

gametype_t com_gametype;



#ifdef SUPPORTS_NEHAHRA
cbool nehahra_active;
#endif // SUPPORTS_NEHAHRA

// this graphic needs to be in the pak file to use registered features
static unsigned short pop[] =
{
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x6600,0x0000,0x0000,0x0000,0x6600,0x0000,
	0x0000,0x0066,0x0000,0x0000,0x0000,0x0000,0x0067,0x0000,
	0x0000,0x6665,0x0000,0x0000,0x0000,0x0000,0x0065,0x6600,
	0x0063,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6563,
	0x0064,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6564,
	0x0064,0x6564,0x0000,0x6469,0x6969,0x6400,0x0064,0x6564,
	0x0063,0x6568,0x6200,0x0064,0x6864,0x0000,0x6268,0x6563,
	0x0000,0x6567,0x6963,0x0064,0x6764,0x0063,0x6967,0x6500,
	0x0000,0x6266,0x6769,0x6a68,0x6768,0x6a69,0x6766,0x6200,
	0x0000,0x0062,0x6566,0x6666,0x6666,0x6666,0x6562,0x0000,
	0x0000,0x0000,0x0062,0x6364,0x6664,0x6362,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0062,0x6662,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0061,0x6661,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x6500,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x6400,0x0000,0x0000,0x0000
};


///////////////////////////////////////////////////////////////////////////////
//  Parse
///////////////////////////////////////////////////////////////////////////////

/*
============
COM_FileBase
take 'somedir/otherdir/filename.ext',
write only 'filename' to the output
============
*/
void COM_FileBase (const char *in, char *out, size_t outsize)
{
	const char	*dot, *slash, *s;

	s = in;
	slash = in;
	dot = NULL;
	while (*s)
	{
		if (*s == '/')
			slash = s + 1;
		if (*s == '.')
			dot = s;
		s++;
	}
	if (dot == NULL)
		dot = s;

	if (dot - slash < 2)
		strlcpy (out, "?model?", outsize);
	else
	{
		size_t	len = dot - slash;
		if (len >= outsize)
			len = outsize - 1;
		memcpy (out, slash, len);
		out[len] = '\0';
	}
}



const char *COM_Parse (const char *data)
{
	int             c;
	int             len;

	len = 0;
	com_token[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;	// end of file
		data++;
	}

// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}

// skip /*..*/ comments
	if (c == '/' && data[1] == '*')
	{
		data += 2;
		while (*data && !(*data == '*' && data[1] == '/'))
			data++;
		if (*data)
			data += 2;
		goto skipwhite;
	}

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			if ((c = *data) != 0)
				++data;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

// parse single characters
	if (c == '{' || c == '}'|| c == '(' || c == ')' ||  c == '\'' || c==':')
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
		/* commented out the check for ':' so that ip:port works */
		if (c=='{' || c=='}'|| c=='(' || c==')' || c=='\'' /* || c == ':' */)
			break;
	} while (c > SPACE_CHAR_32);

	com_token[len] = 0;
	return data;
}


/*
================
COM_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/
int COM_CheckParm (const char *parm)
{
	int             i;

	for (i=1 ; i<com_argc ; i++)
	{
		if (!com_argv[i])
			continue;               // NEXTSTEP sometimes clears appkit vars.
		if (!strcmp (parm,com_argv[i]))
			return i;
	}

	return 0;
}


/*
================
COM_CheckRegistered

Looks for the pop.txt file and verifies it.
Sets the "registered" cvar.
Immediately exits out if an alternate game was attempted to be started without
being registered.
================
*/

static void COM_CheckRegistered (void)
{
	int             h;
	unsigned short  check[128];
	int                     i;

	static_registered = 0;

	COM_OpenFile("gfx/pop.lmp", &h);
	static_registered = 0;

	if (h == -1)
	{
		Con_PrintLinef ("Playing shareware version.");
		if (com_modified)
			System_Error ("You must have the registered version to use modified games");
		return;
	}

	System_FileRead (h, check, sizeof(check));
	COM_CloseFile (h);

	for (i = 0 ; i < 128 ; i++)
	{
	     if (pop[i] != (unsigned short)BigShort (check[i]))
	        System_Error ("Corrupted data file.");
	}

	for (i = 0; com_cmdline[i]; i++)
	{
		if (com_cmdline[i]!= ' ')
			break;
	}

	Cvar_SetQuick (&cmdline, &com_cmdline[i]);
	Cvar_SetValueQuick (&registered, 1);
	static_registered = 1;
	Con_SafePrintLinef ("Playing registered version.");
}


/*
================
COM_InitArgv
================
*/

void COM_InitArgv (int argc, char **argv)
{
	int             i, j, n;


#pragma message ("We should probably replace this with our args to string function")
// reconstitute the command line for the cmdline externally visible cvar
	n = 0;

	for (j=0 ; (j<MAX_NUM_Q_ARGVS_50) && (j< argc) ; j++)
	{
		i = 0;

		while ((n < (MAX_CMD_256 - 1)) && argv[j][i])
		{
			com_cmdline[n++] = argv[j][i++];
		}

		if (n < (MAX_CMD_256 - 1))
			com_cmdline[n++] = ' ';
		else
			break;
	}

	// Remove trailing spaces
	String_Edit_RemoveTrailingSpaces (com_cmdline);

	Con_SafePrintLinef ("Command line: %s", com_cmdline);

	for (com_argc = 0 ; (com_argc < MAX_NUM_Q_ARGVS_50) && (com_argc < argc) ; com_argc++)
	{
		largv[com_argc] = argv[com_argc];
	}

	largv[com_argc] = argvdummy;
	com_argv = largv;

//
// Baker: Sort of what kind of engine we have here
//

// How much could this cut down on ifdefs?

#if id386
	build.assembly_langauage = true;
#endif // id386 --- Otherwise it is false

#ifdef SERVER_ONLY
	// Baker:  This is not the same as -dedicated.
	// A server only .exe cannot be a client and may have differences
	// in expectations.
	build.host_type = host_server_only;
#else
	build.host_type = host_both;
#endif // SERVER_ONLY

#ifdef GLQUAKE
	build.renderer = renderer_hardware;
#else
	build.renderer = renderer_software;
#endif // GLQUAKE

//#ifdef DIRECT3DX_WRAPPER
//	build.direct3d = true;
//#endif // DIRECT3DX_WRAPPER --- Otherwise it is false

#ifdef SUPPORTS_MP3
	build.music_mp3 = true;
#endif // SUPPORTS_MP3 -- Otherwise it is false

#ifdef SUPPORTS_CD
	build.music_cd = true;
#endif // SUPPORTS_CD -- Otherwise it is false

#ifdef SUPPORTS_AVI_CAPTURE
	build.video_avi_capture = true;
#endif // SUPPORTS_AVI_CAPTURE -- Otherwise it is false


#ifdef GLQUAKE_SUPPORTS_VSYNC
	build.video_vsync = true;
#endif // GLQUAKE_SUPPORTS_VSYNC -- Otherwise it is false

#ifdef WINQUAKE_SUPPORTS_VSYNC
	build.video_vsync = true;
#endif // WINQUAKE_SUPPORTS_VSYNC -- Otherwise it is false

#ifdef CORE_GL
	build.video_vsync = true; // Cheesy
#endif // CORE_GL

	com_gametype = gametype_standard;

	if (COM_CheckParm ("-rogue"))		com_gametype = gametype_rogue;
	if (COM_CheckParm ("-hipnotic"))	com_gametype = gametype_hipnotic;
	if (COM_CheckParm ("-quoth"))		com_gametype = gametype_quoth;

#ifdef SUPPORTS_NEHAHRA
	if (COM_CheckParm("-nehahra"))		com_gametype = gametype_nehahra;
#endif // SUPPORTS_NEHAHRA

	isDedicated = (COM_CheckParm ("-dedicated") != 0);

	host_parms.argc = com_argc;
	host_parms.argv = com_argv;
}


/*
================
COM_Init
================
*/
void COM_Init (void)
{
	int	i = 0x12345678;
		/*    U N I X */

	/*
	BE_ORDER:  12 34 56 78
		   U  N  I  X

	LE_ORDER:  78 56 34 12
		   X  I  N  U

	PDP_ORDER: 34 12 78 56
		   N  U  X  I
	*/
	if ( *(char *)&i == 0x12 )
		host_bigendian = true;
	else if ( *(char *)&i == 0x78 )
		host_bigendian = false;
	else /* if ( *(char *)&i == 0x34 ) */
		System_Error ("Unsupported endianism.");

	if (host_bigendian)
	{
		BigShort = ShortNoSwap;
		LittleShort = ShortSwap;
		BigLong = LongNoSwap;
		LittleLong = LongSwap;
		BigFloat = FloatNoSwap;
		LittleFloat = FloatSwap;
	}
	else /* assumed LITTLE_ENDIAN. */
	{
		BigShort = ShortSwap;
		LittleShort = ShortNoSwap;
		BigLong = LongSwap;
		LittleLong = LongNoSwap;
		BigFloat = FloatSwap;
		LittleFloat = FloatNoSwap;
	}

}



/*
=============================================================================

QUAKE FILESYSTEM

=============================================================================
*/

char	com_filepath[MAX_OSPATH];
int     com_filesize;
int		com_filesrcpak;


char    com_gamedir[MAX_OSPATH];
char 	com_basedir[MAX_OSPATH];

#ifndef SERVER_ONLY
char	com_safedir[MAX_OSPATH]; // Isolated area for unsafe files (i.e. dzip doesn't unpack single files)
#endif // SERVER_ONLY

//============================================================================
typedef struct
{
	char    name[MAX_QPATH_64];
	int             filepos, filelen;
} packfile_t;


#define NUM_QFS_FILE_EXT 24
#define NUM_QFS_FOLDER_PREFIX 12
#define NUM_QFS_TYPES (NUM_QFS_FILE_EXT * NUM_QFS_FOLDER_PREFIX)

const char *qfs_file_extensions[NUM_QFS_FILE_EXT] =
{
// Texture lookups are slow and numerous, maps may have hundreds of textures
	"_glow.pcx",
	"_glow.tga",
	"_luma.pcx",
	"_luma.tga",
	".pcx",
	".tga",
// game assets
	".lmp",
	".mdl",
	".wav",
	".spr",
	".bsp",
	".wad",
	".dat",
// potential game assets
	".way",		// Frikbot waypoint
	".mp3",		// mp3 music
	".ogg",		// ogg music
// map related
	".lit",
	".ent",
	".vis",
	".loc",
// other media
	".dem",
	".cfg",
	".rc",
	NULL		// Catch all
};

const char *qfs_folder_prefixes[NUM_QFS_FOLDER_PREFIX] =
{
	"textures/exmy/",
	"textures/*/",		// Special: TEXTURES_WILDCARD_SUBPATH (for textures/<mapname>/
	"textures/",
	"maps/",
	"progs/",
	"gfx/env/",			// Skyboxes live here
	"gfx/particles/",	// Not that we support this
	"gfx/crosshairs/",	// Not that we support this
	"gfx/",
	"sound/",
	"music/",			// We don't support playing tracks out of pak files :(  I could, but ...
	NULL,
};

typedef struct
{
	cbool		contains;
	int				first_index;
	int				last_index;
} pakcontent_t;

typedef struct pack_s
{
	char			filename[MAX_OSPATH];
	int             handle;
	int             numfiles;
	packfile_t      *files;

	pakcontent_t	contents[NUM_QFS_TYPES];
} pack_t;

typedef struct searchpath_s
{
	char    filename[MAX_OSPATH];
	pack_t  *pack;          // only one of filename / pack will be used
	struct searchpath_s *next;
} searchpath_t;

extern searchpath_t *com_searchpaths;


#define TEXTURES_WILDCARD_SUBPATH_1 1


searchpath_t    *com_searchpaths;

/*
============
COM_Path_f
============
*/
void COM_Path_f (void)
{
	searchpath_t    *s;

	Con_PrintLinef ("Current search path:");
	for (s=com_searchpaths ; s ; s=s->next)
	{
		if (s->pack)
		{
			Con_PrintLinef ("%s (%d files)", s->pack->filename, s->pack->numfiles);
		}
		else
			Con_PrintLinef ("%s", s->filename);
	}
}

void COM_Exists_f (lparse_t *line)
{
	if (line->count == 2)
	{
		FILE	*f;
		const char *check_filename = line->args[1];
		COM_FOpenFile (check_filename, &f);

		if (f)
		{
			FS_fclose (f);
			Con_PrintLinef ("File %s exists with size of %d", line->args[1], com_filesize);
		} else Con_PrintLinef ("%s does not exist", line->args[1]);
	} else Con_PrintLinef ("Exists: Indicates if a file exists in the path");
}


#if 0000 // Baker 16Dec2016
/*
============
COM_WriteFile

The filename will be prefixed by the current game directory
============
*/
void COM_WriteFile (const char *filename, const void *data, int len)
{
	int             handle;
	char    name[MAX_OSPATH];

	FS_FullPath_From_QPath (name, filename);
	File_Mkdir_Recursive (name);

	handle = System_FileOpenWrite (name);
	if (handle == -1)
	{
		Dedicated_Printf ("COM_WriteFile: failed on %s\n", name);
		return;
	}

	Dedicated_PrintLinef ("COM_WriteFile: %s", name);
	System_FileWrite (handle, data, len);
	System_FileClose (handle);
}
#endif


// Return filename fullpath from successful open.  For mp3 music currently.
char *COM_FindFile_NoPak (const char *file_to_find)
{
	static char namebuffer [MAX_OSPATH];
	searchpath_t	*search;
	FILE *f;

	// Look through each search path
	for (search = com_searchpaths ; search ; search = search->next)
	{
		// Ignore pack files, we could one up this by not ignoring pak files and copying bytes out to temp dir file
		if (!search->pack)
		{
			c_snprintf2 (namebuffer, "%s/%s", search->filename, file_to_find);
			f = FS_fopen_read (namebuffer, "rb");

			if (f)
			{
				FS_fclose (f);
				Con_DPrintLinef ("Located file '%s' in '%s'", file_to_find, search->filename);
				return namebuffer;
			}
			else Con_DPrintLinef ("Failed to locate file '%s' in '%s'", file_to_find, search->filename);
		}
	}

	return NULL;	// Failure
}

/*
=================
COM_FindFile

finds files in given path including inside paks as well
=================
*/
#pragma message ("I really ought to delete this function.  See maps and what fitz does to detect if a file exists")
cbool COM_FindFile_Obsolete (const char *filename)
{
	searchpath_t	*search;
	char		netpath[MAX_OSPATH];
	pack_t		*pak;
	int		i;

	for (search = com_searchpaths ; search ; search = search->next)
	{
		if (search->pack)
		{
			pak = search->pack;
			for (i=0 ; i<pak->numfiles ; i++)
				if (!strcmp(pak->files[i].name, filename))
					return true;
		}
		else
		{
			c_snprintf2 (netpath, "%s/%s", search->filename, filename);
			if (File_Exists (netpath))
				return true;
		}
	}

	return false;
}



int QFS_File_Category(const char *qpath_filename)
{
	const char *file_ext;
	int			file_ext_class;
	const char *file_folder_prefix;
	int			file_folder_prefix_class;
	int			classification = 0;
	int			i;

	for (i = 0; i < NUM_QFS_FILE_EXT && qfs_file_extensions[i] != NULL; i ++)
	{
		const char *this_ext = qfs_file_extensions[i];

		if ( String_Does_End_With (qpath_filename, this_ext) )
			break;
	}
	file_ext_class = i;
	file_ext = qfs_file_extensions[i];

	for (i = 0; i < NUM_QFS_FOLDER_PREFIX && qfs_folder_prefixes[i] != NULL; i ++)
	{
		const char *this_prefix = qfs_folder_prefixes[i];

		// Baker: If I could eliminate this I would ...
		if (i == TEXTURES_WILDCARD_SUBPATH_1) // Special exception
		{
			// Is there a "/" past "textures/"
			if (String_Does_Start_With (qpath_filename, "textures/") && strstr (qpath_filename + 9, "/") )
				break;
			else continue; // Don't bother with the rest
		}

		if ( String_Does_Start_With (qpath_filename, this_prefix) )
			break;
	}
	file_folder_prefix_class = i;
	file_folder_prefix = qfs_folder_prefixes[i];

	classification = file_folder_prefix_class * NUM_QFS_FILE_EXT + file_ext_class;
	return classification;
}



int COM_FindFile (const char *filename, int *handle, FILE **file, const char *media_owner_path)
{
	searchpath_t    *search;
	cbool 			pak_limiter_blocking = false;
	int				file_category = QFS_File_Category(filename);

	char            netpath[MAX_OSPATH];
	int             i;

// Invalidate existing data
	com_filesize		= -1;	// File length
	com_filepath[0]		= 0;
	com_filesrcpak		= -1;

	if (file && handle)
		System_Error ("COM_FindFile: both handle and file set");
	if (!file && !handle)
		System_Error ("COM_FindFile: neither handle or file set");

//
// search through the path, one element at a time
//
	for (search = com_searchpaths; search ; search = search->next)
	{
		// is the element a pak file?
		if (search->pack && pak_limiter_blocking)	// We hit earliest accepted pak file.
			continue;
		else if (search->pack)
		{
			pack_t          *pak = search->pack;

			int		starting_index  = 0;
			int		ending_index    = pak->numfiles;	// Baker: Yeah, this isn't the "ending_index" unless we add +1

			do
			{
				if (!pak->contents[file_category].contains)
				{
//					static int prevent = 0;

//					prevent++;
//					Con_SafePrintLinef ("Prevented %d on %s", prevent++, filename);
					break;	// There are no files with that extension class in the pak
				}

				// Ok this file type exists in pak
				starting_index = pak->contents[file_category].first_index;
				ending_index   = pak->contents[file_category].last_index + 1;	// Baker: must +1, you know ... its a "for loop" don't wanna skip last file

				// look through all the pak file elements
				// Formerly from i = 0 to i < pak->numfiles
				for (i = starting_index ; i < ending_index  ; i++)
				{
					if (strcmp (pak->files[i].name, filename) != 0)
						continue; // No match

					// found it!
//					Dedicated_PrintLinef ("PackFile: %s : %s", pak->filename, filename);  // Baker: This spams!
					if (handle)
					{
						*handle = pak->handle;
						System_FileSeek (pak->handle, pak->files[i].filepos);
					}
					else
					{
						// open a new file on the pakfile
						*file = FS_fopen_read (pak->filename, "rb");
						// advance to correct seek position
						if (*file)

							fseek (*file, pak->files[i].filepos, SEEK_SET);


					}
					com_filesize = pak->files[i].filelen;
					com_filesrcpak = (search->next && !search->next->next) ? 2 : 1; // Baker: (2 means pak0.pak) pak0.pak is 2nd from last in proper Quake.
					c_strlcpy (com_filepath, pak->filename);
					return com_filesize;



				}
			} while (0);
			// File is not in this pack
			if (media_owner_path && strcasecmp (media_owner_path, pak->filename) == 0)
				pak_limiter_blocking = true; // Found pak of media owner.  No more pak checking.

			continue;
		}
		else /* check a file in the directory tree */
		{
			if (!static_registered)
			{ /* if not a registered version, don't ever go beyond base */
				if ( strchr (filename, '/') || strchr (filename,'\\'))
					continue;
			}

			c_snprintf2 (netpath, "%s/%s", search->filename, filename);

			if (!File_Exists (netpath))
			{
				if (media_owner_path && (strcasecmp (media_owner_path, search->filename) == 0 || (pak_limiter_blocking && strncasecmp(media_owner_path, search->filename, strlen(search->filename))==0 )))
					break; // If this search path is the media owner path or the full search path is in the media owner path, stop.  Baker: the 2nd case is for media owner's in a subfolder.  Like search path is quake/id and media owner is quake/id1/progs
				else
					continue;
			}

//			Dedicated_PrintLinef ("FindFile: %s", netpath);  Baker: This spams
			com_filesize = System_FileOpenRead (netpath, &i);
			com_filesrcpak = 0;
			if (handle)
				*handle = i;
			else
			{
				System_FileClose (i);
				*file = FS_fopen_read (netpath, "rb");
			}
			c_strlcpy (com_filepath, search->filename);
			return com_filesize;
		}
	}

	//if (developer.value > 1) // Baker: This spams too much
	Con_DPrintLinef_Files ("FindFile: can't find %s", filename);

	if (handle)
		*handle = -1;
	else
		*file = NULL;
	com_filesize = -1;
	return -1;
}


/*
===========
COM_OpenFile

filename never has a leading slash, but may contain directory walks
returns a handle and a length
it may actually be inside a pak file
===========
*/

int COM_OpenFile_Limited (const char *filename, int *handle, const char *media_owner_path)
{
	return COM_FindFile (filename, handle, NULL, media_owner_path);
}

int COM_OpenFile (const char *filename, int *handle)
{
	return COM_FindFile (filename, handle, NULL, NULL /* no media owner path */);
}

/*
===========
COM_FOpenFile

If the requested file is inside a packfile, a new FILE * will be opened
into the file.
===========
*/
int COM_FOpenFile_Limited (const char *filename, FILE **file, const char *media_owner_path)
{
	return COM_FindFile (filename, NULL, file, media_owner_path);
}

int COM_FOpenFile (const char *filename, FILE **file)
{
	return COM_FOpenFile_Limited (filename, file, NULL /* no media owner path */);
}

/*
============
COM_CloseFile

If it is a pak file handle, don't really close it
============
*/
void COM_CloseFile (int h)
{
	searchpath_t    *s;

	for (s = com_searchpaths ; s ; s=s->next)
		if (s->pack && s->pack->handle == h)
			return;

	System_FileClose (h);
}


/*
============
COM_LoadFile

Filename are relative to the quake directory.
always appends a 0 byte.
============
*/
#define	LOADFILE_ZONE		0
#define	LOADFILE_HUNK		1
#define	LOADFILE_TEMPHUNK	2
#define	LOADFILE_CACHE		3
#define	LOADFILE_STACK		4
#define LOADFILE_MALLOC		5

static byte    *loadbuf;
static cache_user_t *loadcache;
static int             loadsize;

static byte *COM_LoadFile_Limited (const char *path, int usehunk, const char *media_owner_path)
{
	int             h;
	byte    *buf;
	char    base[MAX_OSPATH]; // MH
	int             len;

	buf = NULL;     // quiet compiler warning

//#ifdef GLQUAKE_SUPPORTS_QMB
//	if (usehunk == LOADFILE_STACK && String_Does_Match_Caseless (path, "progs/flame0.mdl")) {
//		extern const size_t qmb_flame0_mdl_size;
//
//
//		len = qmb_flame0_mdl_size;
//
//		buf = (byte *) Hunk_TempAlloc (len + 1);
//		memcpy (buf, qmb_flame0_mdl, len);
//
//		goto flame0_cheetz;
//	}
//#endif // GLQUAKE_SUPPORTS_QMB


// look for it in the filesystem or pack files
	len = COM_OpenFile_Limited (path, &h, media_owner_path);
	if (h == -1)
		return NULL;

// extract the filename base name for hunk tag
	COM_FileBase (path, base, sizeof(base));

	switch (usehunk)
	{
	case LOADFILE_HUNK:
		buf = (byte*) Hunk_AllocName (len+1, base);
		break;
	case LOADFILE_TEMPHUNK:
		buf = (byte*) Hunk_TempAlloc (len+1);
		break;
	case LOADFILE_ZONE:
		buf = (byte*) Z_Malloc (len+1);
		break;
	case LOADFILE_CACHE:
		buf = (byte*)  Cache_Alloc (loadcache, len+1, base);
		break;
	case LOADFILE_STACK:
		if (len < loadsize)
			buf = loadbuf;
		else
			buf = (byte *) Hunk_TempAlloc (len+1);
		break;
	case LOADFILE_MALLOC:
		buf = (byte *) malloc (len+1);
		break;
	default:
		System_Error ("COM_LoadFile: bad usehunk");
	}

	if (!buf)
		System_Error ("COM_LoadFile: not enough space for %s", path);

	((byte *)buf)[len] = 0;

#if 0 // Baker: Loading icons
	Draw_BeginDisc ();
#endif // Baker
	System_FileRead (h, buf, len);
	COM_CloseFile (h);
#if 0 // Baker: Loading icons
	Draw_EndDisc ();
#endif // Baker

//flame0_cheetz:
	if (com_filesrcpak == 2 && String_Does_Match_Caseless (path, "progs/flame.mdl"))
		com_filesrcpak = 3; // Evile!
	return buf;
}

byte *COM_LoadFile (const char *path, int usehunk)
{
	return COM_LoadFile_Limited (path, usehunk, NULL /* no media owner path */);
}

byte *COM_LoadHunkFile_Limited (const char *path, const char *media_owner_path)
{
	return COM_LoadFile_Limited (path, LOADFILE_HUNK, media_owner_path);
}

byte *COM_LoadHunkFile (const char *path)
{
	return COM_LoadFile (path, LOADFILE_HUNK);
}

byte *COM_LoadTempFile (const char *path)
{
	return COM_LoadFile (path, LOADFILE_TEMPHUNK);
}

void COM_LoadCacheFile (const char *path, struct cache_user_s *cu)
{
	loadcache = cu;
	COM_LoadFile (path, LOADFILE_CACHE);
}

// uses temp hunk if larger than bufsize
byte *COM_LoadStackFile (const char *path, void *buffer, int bufsize)
{
	byte    *buf;

	loadbuf = (byte *)buffer;
	loadsize = bufsize;
	buf = COM_LoadFile (path, LOADFILE_STACK);

	return buf;
}

// returns malloc'd memory
byte *COM_LoadMallocFile (const char *path)
{
	return COM_LoadFile (path, LOADFILE_MALLOC);
}


/*
=================
COM_LoadPackFile -- johnfitz -- modified based on topaz's tutorial

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
pack_t *COM_LoadPackFile (const char *packfile)
{
	dpackheader_t   header;
	int                             i;
	packfile_t              *newfiles;
	int                             numpackfiles;
	pack_t                  *pack;
	int                             packhandle;
	dpackfile_t             info[MAX_FILES_IN_PACK_2048];
	unsigned short          crc;

	if (!File_Exists(packfile)) // Fail faster?  Added Nov 2016, shouldn't make any difference except place to set a breakpoint
		return NULL;

	if (System_FileOpenRead (packfile, &packhandle) == -1)
		return NULL;
	System_FileRead (packhandle, (void *)&header, sizeof(header));
	if (header.id[0] != 'P' || header.id[1] != 'A' || header.id[2] != 'C' || header.id[3] != 'K')
		System_Error ("%s is not a packfile", packfile);
	header.dirofs = LittleLong (header.dirofs);
	header.dirlen = LittleLong (header.dirlen);

	numpackfiles = header.dirlen / sizeof(dpackfile_t);

	if (numpackfiles > MAX_FILES_IN_PACK_2048)
		System_Error ("%s has %d files", packfile, numpackfiles);

	if (numpackfiles != PAK0_COUNT)
		com_modified = true;    // not the original file

	//johnfitz -- dynamic gamedir loading
	//Hunk_AllocName (numpackfiles * sizeof(packfile_t), "packfile");
	newfiles = (packfile_t *) Z_Malloc(numpackfiles * sizeof(packfile_t));
	//johnfitz

	System_FileSeek (packhandle, header.dirofs);
	System_FileRead (packhandle, (void *)info, header.dirlen);

	//johnfitz -- dynamic gamedir loading
	//pack = Hunk_Alloc (sizeof (pack_t));
	pack = (pack_t *) Z_Malloc (sizeof (pack_t));
	//johnfitz

	c_strlcpy (pack->filename, packfile);
	pack->handle = packhandle;
	pack->numfiles = numpackfiles;
	pack->files = newfiles;


	// crc the directory to check for modifications
	CRC_Init (&crc);
	for (i = 0; i < header.dirlen ; i++)
		CRC_ProcessByte (&crc, ((byte *)info)[i]);
	if (crc != PAK0_CRC_V106 && crc != PAK0_CRC_V101 && crc != PAK0_CRC_V100)
		com_modified = true;

	{	// Speed up begin
		int k;
		for (k = 0; k < (NUM_QFS_FILE_EXT * NUM_QFS_FOLDER_PREFIX) ; k++)
		{
			pack->contents[k].contains	= false;
			pack->contents[k].first_index = -1;
			pack->contents[k].last_index = 0;
		}
	}

	// parse the directory
	for (i = 0; i < pack->numfiles ; i++)
	{
		int file_category = QFS_File_Category(info[i].name);

		c_strlcpy (newfiles[i].name, info[i].name);
		newfiles[i].filepos = LittleLong(info[i].filepos);
		newfiles[i].filelen = LittleLong(info[i].filelen);

		// Speed up begin

		pack->contents[file_category].contains = true;
		if (pack->contents[file_category].first_index < 0)
			pack->contents[file_category].first_index = i;
		pack->contents[file_category].last_index = i; // Because i always increases, no need to see if is greater than last one
		// Speed up end
	}


	//Con_PrintLinef ("Added packfile %s (%d files)", packfile, numpackfiles);
	return pack;
}

/*
=================
COM_AddGameDirectory -- johnfitz -- modified based on topaz's tutorial
=================
*/
void COM_AddGameDirectory (const char *relative_dir, cbool hd_only)
{
	char dir[MAX_OSPATH];
	int i;
	searchpath_t *search;
	pack_t *pak;
	char pakfile[MAX_OSPATH];

	FS_FullPath_From_Basedir (dir, relative_dir);

	// Baker: I'm not found of this next line really but it works ...
	if (!hd_only) {
		c_strlcpy (com_gamedir, dir); // Remember: com_gamedir is full directory.
		com_hdfolder_count = 0;
	}
	else com_hdfolder_count ++; // Add 1

	for (search = com_searchpaths; search; search = search->next)
		if (strcasecmp (search->filename, dir) == 0)
			return; // Already added this dir.

	// add the directory to the search path
	search = (searchpath_t *) Z_Malloc(sizeof(searchpath_t));
	c_strlcpy (search->filename, dir);
	search->next = com_searchpaths;
	com_searchpaths = search;

	if (hd_only)
		return;

	// add any pak files in the format pak0.pak pak1.pak, ...
	for (i = 0; ; i++)
	{
		c_snprintf2 (pakfile, "%s/pak%d.pak", dir, i);
		pak = COM_LoadPackFile (pakfile);
		if (!pak)
			break;
		search = (searchpath_t *) Z_Malloc(sizeof(searchpath_t));
		search->pack = pak;
		search->next = com_searchpaths;
		com_searchpaths = search;
	}
	//	alert ("Last pak file %s", pakfile);
}



/*
=================
COM_InitFilesystem
=================
*/
void COM_InitFilesystem (void) //johnfitz -- modified based on topaz's tutorial
{
	int i, j;

#ifndef SERVER_ONLY
	c_snprintf1 (com_safedir, "%s/__tempfiles", Folder_Caches_URL());
	if (!isDedicated)
	{
		File_Mkdir_Recursive (com_safedir); // Ensure parent path exists
		File_Mkdir (com_safedir);
		FS_SafeDirClean();
	}
#endif

	Cmd_AddCommands (COM_InitFilesystem);

	i = COM_CheckParm ("-basedir");
	if (i && i < com_argc - 1)
	{
		c_strlcpy (com_basedir, com_argv[i + 1]);
	} else c_strlcpy (com_basedir, host_parms.basedir);

	// Baker: Removes trailing slash off basedir.  I would eliminate except I think
	// all paths should have a trailing '/' to be proper, but battle for the future

	j = strlen (com_basedir);
	if (j > 0)
	{
		if ((com_basedir[j-1] == '\\') || (com_basedir[j-1] == '/'))
			com_basedir[j-1] = 0;
	}

	com_gametype = gametype_standard;

	if (COM_CheckParm ("-rogue"))		com_gametype = gametype_rogue;
	if (COM_CheckParm ("-hipnotic"))	com_gametype = gametype_hipnotic;
	if (COM_CheckParm ("-quoth"))		com_gametype = gametype_quoth;
#ifdef SUPPORTS_NEHAHRA
	// Baker: We aren't adding the game directory for Nehahra
	// Need to use -game nehahra -nehahra
	if (COM_CheckParm("-nehahra"))		com_gametype = gametype_nehahra;
#endif // SUPPORTS_NEHAHRA

	// start up with GAMENAME_ID1 by default (id1)
	COM_AddGameDirectory (GAMENAME_ID1, false /*not a hd only folder*/);

	// any set gamedirs will be freed up to here
	com_base_searchpaths = com_searchpaths;

	com_modified = true;

	switch (com_gametype)
	{
	case gametype_rogue:			COM_AddGameDirectory ("rogue", false /*real*/);		break;
	case gametype_hipnotic:			COM_AddGameDirectory ("hipnotic", false /*real*/);	break;
	case gametype_quoth:			COM_AddGameDirectory ("quoth", false /*real*/);		break;
	case gametype_nehahra:			COM_AddGameDirectory ("nehahra", false /*real*/);	break;  // Nehahra must manually be added
	case gametype_standard:			com_modified = false;				break;
	}

	i = COM_CheckParm ("-game");
	if (i && i < com_argc-1)
	{
		char folder_url[MAX_OSPATH];

		FS_FullPath_From_Basedir (folder_url, com_argv[i+1]);
		if (!File_Exists (folder_url) || !File_Is_Folder(folder_url))
			System_Error ("Folder %s does not exist", folder_url);

		if (!strstr(com_argv[i+1], ".."))
		{
			com_modified = true;
			COM_AddGameDirectory (com_argv[i+1], false /*not a hd only folder*/);
		} else Con_SafePrintLinef ("Relative gamedir not allowed");
	}

	c_strlcpy (game_startup_dir, com_gamedir);
	COM_CheckRegistered ();
}

// Baker: Make users aware of bad practices like mixed-case or uppercase names
// No this isn't perfect, but maybe will get ball rolling so those that do that
// are more aware without someone having to tell them.
void COM_Uppercase_Check (const char *in_name)
{
	const char *name = File_URL_SkipPath (in_name);
	if (String_Does_Have_Uppercase (name))
		Con_WarningLinef ("Compatibility: filename " QUOTED_S " contains uppercase", name);
}




//==============================================================================
//johnfitz -- dynamic gamedir stuff
//==============================================================================

void COM_RemoveAllPaths (void)
{
	searchpath_t *next;
	while (com_searchpaths != com_base_searchpaths)
	{
		next = com_searchpaths->next;
		if (com_searchpaths->pack)
		{
			Con_DPrintLinef ("Releasing %s ...", com_searchpaths->pack->filename);
			System_FileClose (com_searchpaths->pack->handle);

			Z_Free (com_searchpaths->pack->files);
			Z_Free (com_searchpaths->pack);
		}

		Z_Free (com_searchpaths);

		com_searchpaths = next;
	}
	FS_FullPath_From_Basedir (com_gamedir, GAMENAME_ID1 );
	Con_PrintLinef ("Release complete." NEWLINE);
}

// Return the number of games in memory
int NumGames(searchpath_t *search)
{
	int found = 0;
	while (search)
	{
		if (*search->filename)
			found++;
		search = search->next;
	}
	return found;
}

char *COM_CL_Worldspawn_Value_For_Key (char *source, const char *find_keyname)
{
	static char		valuestring[4096];
	char			current_key[128];
	const char		*data;
	const char		*copy_start;

	// Read some data ...
	if (!(data = COM_Parse(data = source)) || com_token[0] != '{')	// Opening brace is start of worldspawn
		return NULL; // error

	while (1)
	{
		// Read some data ...
		if (!(data = COM_Parse(data)))
			return NULL; // End of data

		if (com_token[0] == '}')	// Closing brace is end of worldspawn
			return NULL; // End of worldspawn

		// Copy data over, skipping a prefix of '_' in a keyname
		copy_start = &com_token[0];

		if (*copy_start == '_')
			copy_start++;

		c_strlcpy (current_key, copy_start);

		String_Edit_RemoveTrailingSpaces (current_key);

		if (!(data = COM_Parse(data)))
			return NULL; // error

		if (strcasecmp (find_keyname, current_key) == 0)
		{
			c_strlcpy (valuestring, com_token);
			return valuestring;
		}

	}

	return NULL;
}

#ifdef SUPPORTS_NEHAHRA
char *COM_CL_Value_For_Key_Find_Classname (const char *classname, const char *find_keyname)
{
	static char		valuestring[4096];
	char			current_key[128];
	const char		*data;
	const char		*copy_start;


	data = strstr(cl.worldmodel->entities, classname);

	if (!data)
		return NULL;

	// Baker: Go back a bit
	for ( ; data > cl.worldmodel->entities; data--)
		if (*data == '{')
		{
			data++;
			break;
		}

	while (1)
	{
		// Read some data ...
		if (!(data = COM_Parse(data)))
			return NULL; // End of data

		if (com_token[0] == '}')	// Closing brace is end of worldspawn
			return NULL; // End of worldspawn

		// Copy data over, skipping a prefix of '_' in a keyname
		copy_start = &com_token[0];

		if (*copy_start == '_')
			copy_start++;

		c_strlcpy (current_key, copy_start);

		String_Edit_RemoveTrailingSpaces (current_key);

		if (!(data = COM_Parse(data)))
			return NULL; // error

		if (strcasecmp (find_keyname, current_key) == 0)
		{
			c_strlcpy (valuestring, com_token);
			return valuestring;
		}

	}

	return NULL;
}
#endif // SUPPORTS_NEHAHRA


/* Baker: Inline ASM example
__declspec (naked) void Q_sincos (float angradians, float *angsin, float *angcos)
{
   __asm fld dword ptr [esp + 4]
   __asm fsincos

   __asm mov ebx, [esp + 12]
   __asm fstp dword ptr [ebx]

   __asm mov ebx, [esp + 8]
   __asm fstp dword ptr [ebx]

   __asm ret
}
*/


#define STRIP_LEN_6 6 // "rt.tga"
cbool List_Filelist_Rebuild (clist_t** list, const char *slash_subfolder, const char *dot_extension, int pakminfilesize, int directives)
{
	cbool			retval = false;
	char			filenamebuf[MAX_QPATH_64];
	searchpath_t    *search;
	int				depth = 0;
	int				count = 0;
	cbool			isdemo = !strcmp (".dem", dot_extension);

	// If list exists, erase it first.
	if (*list)
		List_Free (list);

	search = com_searchpaths;


	if (isin3( directives, SPECIAL_GAMEDIR_ONLY_IF_COUNT, SPECIAL_GAMEDIR_PREFERENCE, SPECIAL_GAMEDIR_ONLY) && com_hdfolder_count) {
		// Advance past all HD folders.
		int	i; for (i = 0; i < com_hdfolder_count; i ++) {
			search = search->next; // Skip the HD folder.
		}

	}

	for (/*nada*/ ; search ; search = search->next)
	{
		if (search->filename[0]) //directory
		{
			// Baker: The next line excludes listing maps we can't play with shareware
			if (static_registered || !pakminfilesize)
			{
				char curname_url[MAX_OSPATH];
				DIR		*dir_p;
				struct dirent	*dir_t;
				c_snprintf2 (curname_url, "%s%s", search->filename, slash_subfolder);

				dir_p = opendir(curname_url);
				if (dir_p)
				{

					while ((dir_t = readdir(dir_p)) != NULL)
					{
						if (isdemo && String_Does_End_With_Caseless(dir_t->d_name, ".dz"))
						{
							// keep going
						}
						else if (!String_Does_End_With_Caseless(dir_t->d_name, dot_extension))
							continue;

						if (directives & SPECIAL_GAMEDIR_PREFERENCE)
						{
							// Baker: 2 character prefix to tell where it was found
							// So levels menu can display
							c_snprintf2 (filenamebuf, "%d:%s", !!depth, dir_t->d_name);
						}
						else c_strlcpy (filenamebuf, dir_t->d_name);

						if (directives == SPECIAL_STRIP_6_SKYBOX)
						{
							int len = strlen(filenamebuf);
							// length 7 - strip 6 = null @ 1 = strlen - 6 = 0
							if (len > STRIP_LEN_6)
								filenamebuf[len - STRIP_LEN_6] = 0;
							else Con_PrintLinef ("Couldn't strip filename");
						}
						else
						if ( (directives & SPECIAL_DONOT_STRIP_EXTENSION) == 0)
						{
							if (!(isdemo && String_Does_End_With_Caseless(dir_t->d_name, ".dz")))
								File_URL_Edit_Remove_Extension (filenamebuf);
						}
						List_Add (list, filenamebuf);
						count ++;
					}
					closedir(dir_p);
				}
			}
			depth ++;

			// This works because paks are the top of the priority list
			if ((directives & SPECIAL_GAMEDIR_ONLY_IF_COUNT) && depth == 1 && count)
			{
				retval = true;
				break;
			}

			if ((directives & SPECIAL_GAMEDIR_ONLY))
				break;
			// End of directory
		}
		else //pakfile
		{
			pack_t          *pak;
			int             i;

			// This is used to exclude id1 maps from results (e1m1 etc.)
			// This is used to exclude id1 maps from results (e1m1 etc.)
			if ( (directives & SPECIAL_GAMEDIR_DIRECTORY_ONLY) )
				continue;

			if ( (directives & SPECIAL_NO_ID1_PAKFILE) && strstr(search->pack->filename, va("/%s/", GAMENAME_ID1)) )
				continue;

			for (i = 0, pak = search->pack; i < pak->numfiles ; i++)
			{
				// Baker --- if we are specifying a subfolder, for a pak
				// This must prefix the file AND then we must skip it.
				const char *current_filename = pak->files[i].name;

				if (isdemo && String_Does_End_With_Caseless(current_filename, ".dz"))
				{
					// accepted
				}
				else if (!String_Does_End_With_Caseless(current_filename, dot_extension))
					continue;

				// we are passed something like "/maps" but for pak system we
				// need to skip the leading "/" &so slash_subfolder[1]
				if (slash_subfolder[0])
				{
					if (String_Does_Start_With_Caseless(current_filename, va("%s/", &slash_subfolder[1]) )  )
					{
						current_filename = File_URL_SkipPath (current_filename);
					} else continue; // Doesn't start with subfolder
				}

				// Baker: johnfitz uses this to keep ammo boxes, etc. out of list
				if (pak->files[i].filelen < pakminfilesize)
					continue;

				if (directives & SPECIAL_GAMEDIR_PREFERENCE)
				{
					// Baker: 2 character prefix to tell where it was found
					// So levels menu can display
					if (current_filename == NULL)
						current_filename = current_filename;

					if (strstr(search->pack->filename, va("/%s/", GAMENAME_ID1)))
						c_snprintf1 (filenamebuf, "Q:%s", current_filename);
					else
						c_snprintf2 (filenamebuf, "%d:%s", !!depth, current_filename);
				}
				else c_strlcpy (filenamebuf, current_filename);

				if ( (directives & SPECIAL_STRIP_6_SKYBOX) )
				{
					int len = strlen(filenamebuf);
					// length 7 - strip 6 = null @ 1 = strlen - 6 = 0
					if (len > STRIP_LEN_6)
						filenamebuf[len-STRIP_LEN_6] = 0;
					else Con_PrintLinef ("Couldn't strip filename");
				}
				else
				if ( (directives & SPECIAL_DONOT_STRIP_EXTENSION) == 0)
				{
					if (!(isdemo && String_Does_End_With_Caseless(current_filename, ".dz")))
						File_URL_Edit_Remove_Extension (filenamebuf);
				}
				List_Add (list, filenamebuf);
				count ++;
			}
			// End of pak
		}
		// End of main for loop
	}
	// End of function

	return retval;
}




///////////////////////////////////////////////////////////////////////////////
//  FS REGISTER: Baker - These functions operate on a path_to_file
//  And they keep track of what is open and the file mode
//
//  INTERNAL:
///////////////////////////////////////////////////////////////////////////////

#define	MAX_HANDLES		100

typedef struct
{
	cbool	isopen;
	void*		addy;
	char		filename[MAX_OSPATH];
} fs_handles_t;


typedef struct
{
	int				first_slot;
	int				num_open;
	fs_handles_t	files[MAX_HANDLES];
} fs_mgr_t;

fs_mgr_t file_mgr;

static void FS_RegisterClose (FILE* f)
{
	int i;
	for (i = 0; i < MAX_HANDLES; i ++)
	{
		fs_handles_t* cur = &file_mgr.files[i];
		if (cur->addy == f)
		{
			file_mgr.num_open --;
//			Con_DPrintLinef ("FILECLOSE: %s found and closed", cur->filename);
			//Con_PrintLinef ("Files open is %d", file_mgr.num_open);
			cur->filename[0] = 0;
			cur->addy = NULL;
			return;
		}
	}
	Con_DPrintLinef ("Couldn't reconcile closed file (%p)", f);
}




void FS_List_Open_f (lparse_t *unused)
{
	int i;
	Con_PrintLinef ("Open files:");
	for (i = 0; i < MAX_HANDLES; i ++)
	{
		fs_handles_t* cur = &file_mgr.files[i];
		if (cur->addy)
			Con_PrintLinef ("%d: %p %s", i, cur->addy, cur->filename);
	}
	Con_PrintLine ();
}



static void FS_RegisterOpen (const char *filename, FILE* f)
{
	int i;
	for (i = 0; i < MAX_HANDLES; i ++)
	{
		fs_handles_t* cur = &file_mgr.files[i];
		if (cur->filename[0] == 0)
		{
			c_snprintf1 (cur->filename, "%s", filename);
			cur->addy = f;
			file_mgr.num_open ++;

//			Con_DPrintLinef ("FS_fopen: %p -> %s", cur->addy, cur->filename);

			if (file_mgr.num_open > 25)
			{
				Con_WarningLinef ("Over 25 files open!");
				FS_List_Open_f (NULL);
			}

			return;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//  FS REGISTER: Baker - These functions operate on a path_to_file
//  And they keep track of what is open and the file mode
//
//  EXTERNAL: versions of fopen, fclose
///////////////////////////////////////////////////////////////////////////////


FILE *FS_fopen_write (const char *path_to_file, const char *mode)
{
	FILE *f;

	f = fopen (path_to_file, mode); // fopen OK

	if (f)
	{
//		Con_PrintLinef ("FOPEN_WRITE: File open for write: '%s' (%s) %s", filename, mode, f ? "" : "(couldn't find file)");
		FS_RegisterOpen (path_to_file, f);
	}


	return f;
}


// Baker: This version will make the path
FILE *FS_fopen_write_create_path (const char *path_to_file, const char *mode)
{
#pragma message ("Baker: If we ever want some sandboxing")
#if 0
	// SECURITY: Compare all file writes to com_basedir
	if (strlen(filename) < strlen(com_basedir) || strncasecmp (filename, com_basedir, strlen(com_basedir))!=0 || strstr(filename, "..") )
		System_Error ("Security violation:  Attempted path is %s" NEWLINE NEWLINE "Write access is limited to %s folder tree.", filename, com_basedir);
#endif

	FILE *f = fopen (path_to_file, mode);

	// If specified, on failure to open file for write, create the path
	if (!f)
	{
		File_Mkdir_Recursive (path_to_file);
		f = fopen (path_to_file, mode); // fopen OK
	}

	if (f)
	{
		FS_RegisterOpen (path_to_file, f);
	}

	return f;
}

FILE *FS_fopen_read (const char *path_to_file, const char *mode)
{
	FILE *f = fopen (path_to_file, mode); // fopen OK

	if (f)
	{
//		Con_PrintLinef ("FOPEN_READ: File open for read: '%s' (%s) %s", filename, mode, f ? "" : "(couldn't find file)");
		FS_RegisterOpen (path_to_file, f);
	}

	return f;
}


int FS_fclose (FILE* myfile)
{
	FS_RegisterClose (myfile);
	return fclose (myfile);
}





#ifndef SERVER_ONLY
void FS_SafeDirClean (void)
{
	clist_t *tempfiles;
	tempfiles = File_List_Alloc (com_safedir, NULL /*no extension filter*/);
	File_Delete_List (tempfiles);
	List_Free (&tempfiles);
}
#endif // ! SERVER_ONLY


void _FS_FullPath_From_QPath (char s[], size_t len, const char *qpath)
{
#ifdef _DEBUG
	if (len != MAX_OSPATH)
		System_Error ("MAX_OSPATH string wrong length");
#endif // _DEBUG

	// Check for ".." here?

	c_snprintfc (s, len, "%s/%s", com_gamedir, qpath);
}


void _FS_FullPath_From_Basedir (char s[], size_t len, const char *basedirpath)
{
#ifdef _DEBUG
	if (len != MAX_OSPATH)
		System_Error ("MAX_OSPATH string wrong length");
#endif // _DEBUG
	c_snprintfc (s, len, "%s/%s", com_basedir, basedirpath);
}


char *qpath_to_url (const char *relative_url)
{
	return va("%s/%s", com_gamedir, relative_url);
}


char *basedir_to_url (const char *relative_url)
{
	return va("%s/%s", com_basedir, relative_url);
}

char *downloads_folder_url (const char *filename_dot_zip)
{
	return va("%s/%s/%s", com_basedir, DOWNLOADS_FOLDER, filename_dot_zip);
}


const char *gamedir_shortname (void)
{
	return File_URL_SkipPath(com_gamedir);
}



// Baker: ProQuake-like dequake
void COM_DeQuake_String (char *s_edit)
{
	static char dequake[256];

	// If buffer isn't initialized, do so.
	if (!dequake['A'])
	{
		int				i;

		// 32 to 128 is itself, Convert everything over 128 to be 0-127 range.  Exceptions noted ...
		//Tab				// Newline			// Carriage return	// Line feed becomes a space
		// Iterations ...
		for (i =   1 ;	i < 256 ;	i++)		dequake[i] =   i;	// Ensure no control characters.  But see below for 10, 13, etc.
		for (i =   1 ;	i <  31 ;	i++)		dequake[i] = '#';	// Ensure no control characters.  But see below for 10, 13, etc.
		for (i = 127 ;	i < 159 ;	i++)		dequake[i] = '#';	// Ensure no control characters.  But see below for 10, 13, etc.
		for (i = 255 ;	i < 256 ;	i++)		dequake[i] = '#';	// Ensure no control characters.  But see below for 10, 13, etc.

		for (i =  18 ;	i <  28 ;	i++)		dequake[i] = '0' + i - 18;			// Qnumbers
		for (i = 146 ;	i < 156 ;	i++)		dequake[i] = '0' + i - (128 + 18);	// More Qnumbers
		for (i = 160 ;	i < 255 ;	i++)		dequake[i] = dequake[i & 127];		// These should translate ok

		// Final touches
		dequake[9] = '#';	dequake[10] = 10;	dequake[13] = 13;	dequake[12] = '#';
		dequake[1] =		dequake[5] =		dequake[14] =		dequake[15] =		dequake[28] = '.';
		dequake[16] = '[';	dequake[17] = ']';	dequake[29] = '<';	dequake[30] = '-';	dequake[31] = '>';
		dequake[127] = '#';
		dequake[128] = '(';	dequake[129] = '=';	dequake[130] = ')';	dequake[131] = '*';	dequake[141] = '>';
		dequake[255] = '#';
	}

	if (1) {
		char before[1024];
		const char *after = s_edit;
		c_strlcpy (before, s_edit);
		for ( ; *s_edit; s_edit++ ) {
			int ch = (byte)(*s_edit);
//			int new_ch = dequake[ch];
//			if (ch >= 128 || ch < 0)
//				ch = ch;
//			if (new_ch < SPACE_CHAR_32 || new_ch >= MAX_ASCII_DELETE_CHAR_127)
//				new_ch = new_ch;

			*s_edit = dequake[ch];
		}
		//Con_PrintLinef ("Before %s" NEWLINE "After %s", before, after);
		after =after=before;
	}
}

// DeQuakes a name and turns carriage returns and linefeeds into spaces
void COM_DeQuake_Name (char *s_edit)
{
	COM_DeQuake_String (s_edit);

	for (; *s_edit; s_edit++)
		if (*s_edit == 10 || *s_edit == 13)
			*s_edit = ' ';	// Special case: strip wannabe linefeeds from name
}

///////////////////////////////////////////////////////////////////////////////
//  Bulk Read
///////////////////////////////////////////////////////////////////////////////


// No special treatment for carriage returns.

char * FS_ReadLine (FILE *f, char *buf, size_t bufsiz, fs_read_flags_e fs_read_flags)
{
	int ch = 0;
	size_t readcount;

	for (readcount = 0, ch = 0; readcount <  bufsiz - 1; /* nada! */)
	{
		ch = fgetc(f);
		if (ch == EOF && !readcount)
			return NULL;

		if (ch == EOF || ch == '\n' )
			break;

		buf[readcount++] = ch;
	}

	if (!readcount)
		return NULL; // Shouldn't happen except if someone uses bufsize 0?

	// Post-processing
	if ( fs_read_flags & FS_STRIP_CR )
		if ( buf[readcount - 1] == '\r')
			readcount --; // Cause it to overwrite the CR

	buf[readcount++] = 0;

	if ( fs_read_flags & FS_WHITESPACE_TO_SPACE )
		String_Edit_Whitespace_To_Space (buf);

	if ( fs_read_flags & FS_TRIM )
		String_Edit_Trim (buf);

	return buf;
}

char * FS_ReadLine_Text_1024 (FILE *f)
{
	static char readbuf[SYSTEM_STRING_SIZE_1024];

	return FS_ReadLine (f, readbuf, sizeof(readbuf), FS_TEXT_TYPICAL);
}


clist_t *FS_File_Lines_List_Alloc (const char *path_to_file)
{
	clist_t *out = NULL;
	FILE *f = FS_fopen_read (path_to_file, "rb");
	if (f)
	{
		char *thisline;

		while ( (thisline = FS_ReadLine_Text_1024(f) ) )
			List_Add_Unsorted (&out, thisline);

		FS_fclose (f);
	}

	return out;
}



///////////////////////////////////////////////////////////////////////////////
//  IPv4 List Management
///////////////////////////////////////////////////////////////////////////////

void IPv4_List_To_File (const char *path_to_file, clist_t *mylist)
{
	if (mylist)
	{
		FILE * f = FS_fopen_write (path_to_file, "wb");
		if (f)
		{
			clist_t *cur;
			for (cur = mylist; cur; cur = cur->next)
				fprintf (f, "%s\n", cur->name);
			FS_fclose (f);
		}
	}
}

char *IPv4_String_Validated (char *buf, size_t bufsiz, const char *s)
{
	int a, b, c;

	if (bufsiz < 16)
		return NULL; // Need minimum amount of space 3x4 + 3 dots + null term = 16

	if (sscanf (s, "%d.%d.%d", &a, &b, &c) != 3)
		return NULL; // Invalid ip string

	if (a == 169 || a == 127 || a == 192 || a == 0 || a == 10) // Yeah, yeah 172 ... whatever
		return NULL;

	if (!in_range(0, a, 255) || !in_range(0, b, 255) || !in_range(0, c, 255))
		return NULL; // 0-255 only

	// Construct proper ip string for query
	c_snprintfc (buf, bufsiz, "%d.%d.%d.xxx", a, b, c); // sscanf %d is very important to not confuse with octal

	return buf;
}

cbool IPv4_List_Find (const clist_t *list, const char *unvalidated_ipstring)
{
	char prepared_ip_string[16];
	if (!IPv4_String_Validated (prepared_ip_string, sizeof(prepared_ip_string), unvalidated_ipstring))
		return false; // Can't find if it doesn't parse.

	return List_Find (list, prepared_ip_string);
}




static clist_t *IPv4_List_From_Any_ (const char *unknown, cbool fromfile)
{
	// Try to read
	clist_t *new_list = NULL;

	clist_t *textlines = fromfile ? FS_File_Lines_List_Alloc (unknown) : List_From_String_Lines_Alloc (unknown);
	clist_t *cur;
	for (cur = textlines ; cur; cur = cur->next)
	{
		char newip[16];
		if (IPv4_String_Validated (newip, sizeof(newip), cur->name))
			List_Add (&new_list, newip);  // Note: List_Add does not allow duplicates and forces to lowercase
	}

	if (fromfile)
		FS_File_Lines_List_Free (&textlines);
	else List_Free (&textlines); // These are both calling List_Free, but whatever ...

	return new_list;
}


clist_t *IPv4_List_From_String (const char *s)
{
	return IPv4_List_From_Any_ (s, false /* not file, string */);
}

clist_t *IPv4_List_From_File (const char *path_to_file)
{
	return IPv4_List_From_Any_ (path_to_file, true /* yes, from file */);
}



#ifdef CORE_PTHREADS

typedef struct
{
	int event;
	int code;
	void *id;
	void *data;
// timestamp?
	int zeropad; // Why?
} event_x_t;

clist_t *q_thread_events;
pthread_mutex_t q_thread_events_lock = PTHREAD_MUTEX_INITIALIZER;



void Q_Thread_Event_Add (int event, int code, void *id, void *data)
{
	event_x_t eventa = {event, code, id, data, 0 /* zero pad*/}; // Download download_finished id

	Con_Queue_PrintLinef ("Event recorded event:%d code:%d id:%p data:%p ", event, code, id, data); // Thread-safe, blocking.
	List_Add_Raw_Unsorted (&q_thread_events, &eventa, sizeof(eventa));
	// Enure exclusive access to the list.
	pthread_mutex_unlock (&q_thread_events_lock);
}



void Q_Thread_Events_Run (void)
{
	clist_t *events, *cur;

	// Steal the list
	pthread_mutex_lock (&q_thread_events_lock);
	events = q_thread_events;
	q_thread_events = NULL;
	pthread_mutex_unlock (&q_thread_events_lock);

	for (cur = events ; cur; cur = cur->next)
	{
		event_x_t *eventa = (event_x_t *)cur->name;
		Q_Thread_Event_Dispatch (eventa->event, eventa->code, eventa->id, eventa->data);
	}
	List_Free (&events);
}



void Q_Thread_Events (const char *fmt, ...)
{
	VA_EXPAND (text, SYSTEM_STRING_SIZE_1024, fmt);

	// Enure exclusive access to the list.
	pthread_mutex_lock (&q_thread_events_lock);
// Do something
	pthread_mutex_unlock (&q_thread_events_lock);
}


clist_t *con_queue_prints;
pthread_mutex_t queue_printf_mutex = PTHREAD_MUTEX_INITIALIZER;

int Con_Queue_Printf (const char *fmt, ...)
{
	VA_EXPAND (text, SYSTEM_STRING_SIZE_1024, fmt);

	// Enure exclusive access to the list.
	pthread_mutex_lock (&queue_printf_mutex);
	List_Add_Unsorted (&con_queue_prints, text);
	pthread_mutex_unlock (&queue_printf_mutex);
	return 0;
}

int Con_Queue_PrintLinef (const char *fmt, ...)
{
	VA_EXPAND_NEWLINE (text, SYSTEM_STRING_SIZE_1024, fmt);
	return Con_Queue_Printf ("%s", text);
}


void Con_Queue_PrintRun (void /*const char *url <--- huh? */)
{
	clist_t *cur;
	// Enure exclusive access to the list.
	pthread_mutex_lock (&queue_printf_mutex);

	for (cur = con_queue_prints ; cur; cur = cur->next)
		Con_SafePrintContf ("%s", cur->name);  // Don't tie up time -- SafePrint doesn't SCR_Update like Con_Printf

	List_Free (&con_queue_prints);

	pthread_mutex_unlock (&queue_printf_mutex);
}

void File_Read_Touch (const char *path_to_file)
{
	FILE *f = fopen (path_to_file, "rb"); // Don't use FS_open_read --- the register isn't thread safe
	if (f)
	{
		char buf[4];
		fread (buf, 1, sizeof(buf), f);
		fclose (f); // Don't use FS_close --- the register isn't thread safe
	}
}

clist_t *file_read_list_urls; // MAX_QPATH_64 * num
pthread_t read_list_thread;
volatile cbool read_list_cancel;


void *ReadList_Reader (void *pclist)
{
	clist_t *cur = NULL;
	int	count;

#if 0
	Con_Queue_PrintLinef ("Thread started"); // Thread-safe, blocking.
#endif

	// We don't need to lock anything here, we have exclusive access to the list by design
	for (cur = file_read_list_urls, count = 0; cur && !read_list_cancel; cur = cur->next, count ++)
	{
		File_Read_Touch (cur->name);
		// Con_Queue_PrintLinef ("Touched %04d %s", count, cur->name); // Thread-safe, blocking.
	}

	if (read_list_cancel)
		Con_Queue_PrintLinef ("Thread received stop signal %d %s", count, cur->name); // Thread-safe, blocking.

	List_Free (&file_read_list_urls);
#if 0
	Con_Queue_PrintLinef ("Thread ended"); // Thread-safe, blocking.
#endif
	pthread_exit (NULL); // This is optional unless you want to return something for pthread_join to read.
	return NULL;
}

void ReadList_Ensure_Shutdown (void)
{
	// Make sure our list reading thread isn't running.
	read_list_cancel = true; // Tell it to wrap up
	pthread_join (read_list_thread, NULL);
}


void ReadList_NewGame (void)
{
	pthread_attr_t attr;
	searchpath_t *search;

	// Make sure our list reading thread isn't running.
	ReadList_Ensure_Shutdown();
//	read_list_cancel = true; // Tell it to wrap up
//	pthread_join(&read_list_thread, NULL);

	// We have to construct the list here because File_List_Alloc isn't thread safe at all (uses va)
	// Free the list first
	if (file_read_list_urls)
		List_Free (&file_read_list_urls);

	// Get the last gamedir
	for (search = com_searchpaths; search; search = search->next)
		if (search->filename[0]) //directory
		{
			clist_t *new_files = NULL;
			char tmp_path[MAX_OSPATH];
			c_strlcpy (tmp_path, search->filename);
			new_files = File_List_Alloc (tmp_path, ".dz");  List_Concat_Unsorted (&file_read_list_urls, new_files);
			new_files = File_List_Alloc (tmp_path, ".dem");  List_Concat_Unsorted (&file_read_list_urls, new_files);
			c_snprintf1 (tmp_path, "%s", search->filename);
			// Ok, this will return healthbox bsps and such, it's ok to touch those.
			new_files = File_List_Alloc (tmp_path, ".bsp");  List_Concat_Unsorted (&file_read_list_urls, new_files);
		}

	read_list_cancel = false;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&read_list_thread, &attr, ReadList_Reader, NULL);
}




#endif // CORE_PTHREADS
