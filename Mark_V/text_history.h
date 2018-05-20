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
// history.h

#ifndef __HISTORY_H__
#define __HISTORY_H__

#define		HISTORY_FILE_NAME	"history.txt"
#define		HISTORY_LINES_64	64

extern char		history_lines[HISTORY_LINES_64][CONSOLE_MAX_CMDLINE_256];
extern int		edit_line; // This is the current history line we are editing

// Baker: As far as I know history_line is the previous command in the wrapping history buffer
extern int		history_line; //johnfitz




void History_Init (void);
void History_Save (void);
void History_Shutdown (void);

#endif // ! __HISTORY_H__

