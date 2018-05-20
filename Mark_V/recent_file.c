/*
Copyright (C) 2009-2013 Baker
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
// recent_file.c -- Recent file.

#include "quakedef.h"

static char recent_file_url[MAX_OSPATH];


static void Recent_File_Update_Event (void)
{
	// Updated recent file
#ifdef _DEBUG
//	Con_PrintLinef ("Recent file set: '%s'", recent_file);
#endif
}

void Recent_File_Set_FullPath (const char *path_to_file)
{
	c_strlcpy (recent_file_url, path_to_file);
	Recent_File_Update_Event ();
}

void Recent_File_Set_QPath (const char *qpath_filename)
{
	const char *file_url = qpath_to_url(qpath_filename);
	Recent_File_Set_FullPath (file_url);
}

void Recent_File_NewGame (void)
{
	recent_file_url[0] = 0;
	Recent_File_Update_Event ();
}

const char *Recent_File_Get (void)
{
	return recent_file_url;
}

void Recent_File_Show_f (lparse_t *line)
{
	if (vid.screen.type != MODE_WINDOWED)
	{
		Con_PrintLinef ("'showfile' command only works in windowed mode");
		Con_PrintLinef ("alt-enter and try again?");
		return;
	}

	if (!isDedicated && line->count == 2 && !strcmp(line->args[1], "caches")) {
		Con_PrintLinef ("Open caches folder " QUOTED_S " ...", Folder_Caches_URL());
		Folder_Open (Folder_Caches_URL());
		goto open_ok;
	}

	if (!isDedicated && line->count == 2 && !strcmp(line->args[1], "hd")) {
		if (hd_folder.string[0]) {
			char folder_url[MAX_OSPATH];
			// Construct the full url
			FS_FullPath_From_Basedir (folder_url, hd_folder.string);
			Con_PrintLinef ("Open HD folder " QUOTED_S " ...", folder_url);
			Folder_Open (folder_url );
			goto open_ok;
		}
		Con_PrintLinef ("No HD folder is in use.");
	}

	if (recent_file_url[0] && File_Exists(recent_file_url))
	{
		switch (File_Is_Folder(recent_file_url))
		{
		case true: // folder
			if (Folder_Open (recent_file_url))
				goto open_ok;
			break;

		case false: // file
			if (Folder_Open_Highlight (recent_file_url))
				goto open_ok;
			break;
		}
	}

	// Couldn't do the above, try exploring to the gamedir
	if (!Folder_Open (com_gamedir))
	{
		Con_PrintLinef ("Opening folder failed");
		return;
	}

open_ok:
	Con_PrintLinef ("Explorer opening folder ...");

}

void Recent_File_Init (void)
{
	Cmd_AddCommands (Recent_File_Init);
}