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
// history.c

#include "quakedef.h"

char		history_lines[HISTORY_LINES_64][CONSOLE_MAX_CMDLINE_256];
int			edit_line;
int			history_line;

void History_Init (void)
{
	int i, c;
	FILE *hf;

	for (i = 0; i < HISTORY_LINES_64; i++)
	{
		history_lines[i][0] = ']';
		history_lines[i][1] = 0;
	}
	Key_Console_Cursor_Move (0, cursor_reset); // Reset selection

	hf = FS_fopen_read(basedir_to_url(HISTORY_FILE_NAME), "rt");
	if (hf != NULL)
	{
		do
		{
			i = 1;
			do
			{
				c = fgetc(hf);
				history_lines[edit_line][i++] = c;
			} while (c != '\r' && c != '\n' && c != EOF && i < CONSOLE_MAX_CMDLINE_256);
			history_lines[edit_line][i - 1] = 0;
			edit_line = (edit_line + 1) & (HISTORY_LINES_64 - 1);
			/* for people using a windows-generated history file on unix: */
			if (c == '\r' || c == '\n')
			{
				do
					c = fgetc(hf);
				while (c == '\r' || c == '\n');
				if (c != EOF)
					ungetc(c, hf);
				else	c = 0; /* loop once more, otherwise last line is lost */
			}
		} while (c != EOF && edit_line < HISTORY_LINES_64);
		FS_fclose(hf);

		history_line = edit_line = (edit_line - 1) & (HISTORY_LINES_64 - 1);
		history_lines[edit_line][0] = ']';
		history_lines[edit_line][1] = 0;
	}
}

void History_Save (void)
{
	int i;
	FILE *hf;
	char lastentry[1024] = {0};

	hf = FS_fopen_write (basedir_to_url(HISTORY_FILE_NAME), "wt");
	if (hf != NULL)
	{
		i = edit_line;
		do
		{
			i = (i + 1) & (HISTORY_LINES_64 - 1);
		} while (i != edit_line && !history_lines[i][1]);

		while (i != edit_line && history_lines[i][1])
		{
			if (lastentry[0]==0 || strcasecmp (lastentry, history_lines[i] + 1) != 0) // Baker: prevent saving the same line multiple times in a row
				if (strncasecmp(history_lines[i]+1, "quit", 4) != 0) // Baker: why save quit to the history file
					fprintf(hf, "%s\n", history_lines[i] + 1);

			c_strlcpy (lastentry, history_lines[i] + 1);
			i = (i + 1) & (HISTORY_LINES_64 - 1);
		}
		FS_fclose(hf);
	}
}

void History_Shutdown (void)
{
	History_Save ();
}
