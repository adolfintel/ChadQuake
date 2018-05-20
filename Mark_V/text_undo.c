/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2009-2015 Baker and others

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
// undo.c

#include "quakedef.h"

//#define DEBUG_UNDO

void Undo_Dump (struct undo_s *u)
{
	int n;
	Con_PrintLinef ("Undo ... Count = %d Undo Level is %d", u->count, u->level);
	
	for (n = 0; n < u->count; n ++)
	{
//		if (!u->undo_entries[n].text)
//			u=u;
		Con_PrintLinef ("%04d:%s " QUOTED_S, n, (n + 1 == u->level)  ? ">" : " ", u->undo_entries[n].text);
	}
	
}

// Clear all the entries, then the buffer
void Undo_Clear (struct undo_s *u)
{
	if (u->undo_entries)
		u->undo_entries = core_free (u->undo_entries);
	u->level = 0;
	u->count = 0;

#ifdef DEBUG_UNDO
	Con_PrintLinef ("Undo Clear Action");
	Undo_Dump (u);
#endif
}

static void Undo_Remove_From_Top_ (struct undo_s *u, int num_deletes)
{
	int new_top = num_deletes;
	int new_size = u->count - num_deletes;

	if (new_size == 0)
		u->undo_entries = core_free (u->undo_entries); // Wipe
	else 
	{
		//int bytes = sizeof(struct undo_entry_s) * move_size;
		memmove (&u->undo_entries[0], &u->undo_entries[new_top], sizeof(struct undo_entry_s) * new_size);
		u->undo_entries = core_realloc (u->undo_entries, sizeof(struct undo_entry_s) * new_size);
	}

	u->count -= num_deletes;
}

// Concats similar actions into one undo point
static void Undo_Compact_ (struct undo_s *u)
{
	int streak_count;
	int cur_action;
	int n;

#ifdef DEBUG_UNDO
	Con_PrintLinef ("Before compact");
	Undo_Dump (u);
#endif
	
	for (n = 1, streak_count = 0, cur_action = 0; n < u->count; n ++, streak_count ++)
	{
		struct undo_entry_s *e = &u->undo_entries[n];
		
		// If non-typing and non-deleting event, get out!
		if (!e->action)
			break;
		
		// If change in action, get out!
		if (cur_action)
		{
			if (e->action != cur_action)
				break; // Change in action
		}
		else cur_action = e->action; // This gets set once if non-zero.
	}

	if (streak_count > 1) // Only have a streak if you have 2.  No such thing as streak of 1.
	{
		int num_deletes = streak_count - 1;
		int new_size = u->count - num_deletes;
		int streak_end = streak_count; 
		int move_size = new_size - 1;
		
		memmove (&u->undo_entries[1], &u->undo_entries[streak_end], sizeof(struct undo_entry_s) * move_size);
		
		u->count -= num_deletes;
		u->undo_entries = core_realloc (u->undo_entries, sizeof(struct undo_entry_s) * u->count);
	
		u->undo_entries[1].action = 0; // Solidify
	}
}

void Undo_Set_Point (struct undo_s *u, const char *text, int cursor, int cursor_length, int action, cbool was_space)
{
	size_t check_size = sizeof(struct undo_entry_s);


	// If into undo buffer ( level 1 is redo) then remove everything above.
	if (u->level > 0) // Size must be at least 2 if this is true (courtesy redo takes 1 space)
	{
		Undo_Remove_From_Top_ (u, u->level - 1);
		u->level = 0;
	}

	// Don't add identical entries
	if (u->level == 0 && u->count)
	{
		struct undo_entry_s *e = &u->undo_entries[0];

		// Text didn't change, dup so get out ...
		if (!strcmp (e->text, text))
		{
			e->cursor = cursor;
			e->cursor_length = cursor_length;

			// But solidify the match if a hard action happened.
			if (action == 0 && e->action != 0)
				e->action = 0;
			return;
		}
	}

	// ADDING.  INCREASE COUNT.
	u->count ++;

	// If ptr is a null pointer, the realloc function behaves like the malloc function for the specified size.
	u->undo_entries = core_realloc (u->undo_entries, sizeof(struct undo_entry_s) * u->count);
	
//	if (!u->undo_entries)
//		System_Error ("Failed undo alloc on %d bytes", sizeof(struct undo_entry_s) * u->count);

	// If we have undo entries, move everything down one.
	if (u->undo_entries)
		memmove (&u->undo_entries[1], &u->undo_entries[0], sizeof(struct undo_entry_s) * (u->count - 1) );

	// fill in the new guy
	{
		struct undo_entry_s *e = &u->undo_entries[0];
		memset (e, 0, sizeof(e));
		c_strlcpy (e->text, text);

		e->cursor = cursor;
		e->cursor_length = cursor_length;
		e->action = action;
		e->was_space = was_space;
	}

	// A space trigger compacting consecutive "typed character" undos
	// Technically this block is only for undo_size > 1, but should pass through fine if 1.

	
	if (action && was_space)
		Undo_Compact_ (u);

}



// Undo
// Redo
// If level is -1 and change > 0 (undo at top level) do we add snapshot now?  Probably.
const char *Undo_Walk (struct undo_s *u, int change, char *text, int *cursor, int *cursor_length)
{
	if (!u->count)
		return NULL; //  No undos or redos

	if (change < 0 && u->level <= 1) return NULL; // Can't redo beyond current or we aren't walking undos
	if (change > 0 && u->level == u->count) return NULL; // maximum depth
	
	// We aren't walking undos, create a redo for the top
	if (change > 0 && u->level == 0)
	{
		// Make the redo slot.
		Undo_Set_Point (u, text, *cursor, *cursor_length, 0, 0);
		u->level = 2;
	}
	else u->level += change;

	// Copy info
	{
		struct undo_entry_s *e = &u->undo_entries[u->level - 1];
		//strlcpy (*text, e->text, s_size);
		*cursor = e->cursor;
		*cursor_length = e->cursor_length;
		// Don't do cursor length?
		return e->text;
	}

}