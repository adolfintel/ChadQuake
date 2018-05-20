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
// undo.h

#ifndef __UNDO_H__
#define __UNDO_H__

/*
  things about undo
	  === depth, size, textsizemax, buffer pointer

	buffer entry
	- text
	- cursor
	- selection length
	- action (for compacting)

*/


struct undo_entry_s
{
	char		text[256];
	int			cursor;
	int			cursor_length;
	int			action;						// -1 delete, 0 not typing, 1 typing
	cbool		was_space;					// Was the char deleted or typed a space?
};

typedef struct undo_s
{
	int						level;			// Depth into undo buffer.  0 to start, which should be treated like -1.
	int						count;			// Number of undos in buffer
	struct undo_entry_s		*undo_entries;	// We will grow this and move things around (realloc, memmove	
} undo_t;


void Undo_Set_Point (struct undo_s *u, const char *text, int cursor, int cursor_length, int action, cbool was_space);
void Undo_Clear (struct undo_s *u);
void Undo_Dump (struct undo_s *u);
const char *Undo_Walk (struct undo_s *u, int change, char *text, int *cursor, int *cursor_length);




#endif // ! __UNDO_H__





