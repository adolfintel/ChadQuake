/*
Copyright (C) 1996-2001 Id Software, Inc.
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
// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.
// q_music.c - music

#include "quakedef.h"


#ifdef SUPPORTS_MP3_MUSIC
// Baker: setmusic command, remaps cd tracks.
char musicmaps[MAX_MUSIC_MAPS_100][MAX_QPATH_64];

void Set_Music_f (lparse_t *line)
{
	const char *cmds[] = {"help", "list", "read", "reset", "write", NULL};
	enum arg_e {arg_help, arg_list, arg_read, arg_reset, arg_write, max_args};

	if (line->count >= 2)
	{
		const char *parm2 = line->args[1];
		int cmd_num = String_To_Array_Index (parm2, cmds);

		char music_config_name_qpath[MAX_QPATH_64];
		char music_config_name_url[MAX_OSPATH];

		FILE * f;
		int tracknum, written, i;

		c_snprintf1 (music_config_name_qpath, "music/%s", SETMUSIC_CFG);
		FS_FullPath_From_QPath (music_config_name_url, music_config_name_qpath);

		switch (cmd_num)
		{
		case arg_list: // list current mappings
			Con_PrintLine ();
			Con_PrintLinef ("setmusic list:");
			Con_PrintLine ();
			for (i = 0; i < MAX_MUSIC_MAPS_100; i ++)
			{
				if (musicmaps[i][0])
					Con_PrintLinef ("%02d = %s", i, musicmaps[i]);
			}
			Con_PrintLine ();
			Con_PrintLinef ("end of list");
			Con_PrintLine ();
			return;

		case arg_help: // extended help, not the normal help

			Con_PrintLine ();
			Con_PrintLinef ("Usage: %s <0-99> <yourfile.mp3>");
			Con_PrintLinef ("where yourfile.mp3 is in [gamedir]/music folder", line->args[0]);
			Con_PrintLine ();
			Con_PrintLinef ("Usage: %s list  - lists current mappings", line->args[0]);
			Con_PrintLinef ("Usage: %s reset - resets everything", line->args[0]);
			Con_PrintLinef ("Usage: %s write - writes to [gamedir]/%s", line->args[0], music_config_name_qpath);
			Con_PrintLinef ("Usage: %s read  - resets and loads from [gamedir]/%s", line->args[0], music_config_name_qpath);
			Con_PrintLine ();
			Con_PrintLinef ("Note: music files should be in gamedir/music");
			Con_PrintLine ();
			Con_PrintLinef ("Example: %s 0 quake.mp3", line->args[0]);
			Con_PrintLinef ("Would play quake/id1/music/quake.mp3 for track #0");
			Con_PrintLine ();
			Con_PrintLinef ("Example: %s 6 mymusic.mp3", line->args[0]);
			Con_PrintLinef ("Would play quake/id1/music/mymusic.mp3 for track #6");
			Con_PrintLine ();
			Con_PrintLinef ("Filenames should avoid spaces and only use alphanumeric");
			Con_PrintLinef ("characters and the underscore '_'.");
			Con_PrintLine ();
			Con_PrintLinef ("Type 'folder' to access current gamedir.");
			Con_PrintLine ();
			return;

		case arg_reset: // clear all the tracks

			memset (musicmaps, 0, sizeof(musicmaps));
			Con_PrintLinef ("setmusic mappings have been reset");
			return;

		case arg_read: // clear all the tracks and load them

			memset (musicmaps, 0, sizeof(musicmaps));
			Cbuf_AddTextLine ("exec " SETMUSIC_CFG_FULL);
			return;

		case arg_write:

			f = FS_fopen_write_create_path (music_config_name_url, "wb");

			if (!f)
			{
				Con_PrintLinef ("Couldn't open %s for writing", music_config_name_url);
				return;
			}

			Con_PrintLinef ("Writing %s", music_config_name_qpath);

			for (i = 0, written = 0; i < MAX_MUSIC_MAPS_100; i ++)
			{
				if (musicmaps[i][0])
				{
					fprintf (f, "setmusic %02d %s\n", i, musicmaps[i]);
					written ++;
				}
			}

			FS_fclose (f);
			Con_PrintLinef ("Wrote %d items.  Type 'showfile' to examine.", written);

			Recent_File_Set_FullPath (music_config_name_url);
			return;

		case max_args: // Wasn't an enumeration so hopefully "setmusic 0 mymusic.mp3"
			tracknum = atoi (parm2);

			if (line->count != 3)
				break; // not 3 arguments so display help

			if (!isdigit (parm2[0]) || tracknum < 0 || tracknum >= MAX_MUSIC_MAPS_100)
				break; // parm2 isn't a digit or has a value outside valid track range

			// Set music mapping
			c_strlcpy (musicmaps[tracknum], line->args[2]);

			Con_PrintLinef ("Track %02d: %s", tracknum, line->args[2]);
			return;

		} // End of switch statement

	} // End of args >= 2

	// with no parameters or invalid parameters ends up displaying help
	Con_PrintLine ();
	Con_PrintLinef ("Usage: %s <0-99> <yourfile.mp3>");
	Con_PrintLinef ("where yourfile.mp3 is in [gamedir]/music folder", line->args[0]);
	Con_PrintLine ();
	Con_PrintLinef ("Usage: %s {help|list|reset|write|read}", line->args[0]);
	Con_PrintLinef ("Type '%s help' for examples and detail", line->args[0]);
	Con_PrintLine ();
}

#else // doesn't support ...

void Set_Music_f (void) {}

#endif // ! SUPPORTS_MP3_MUSIC