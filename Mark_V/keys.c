/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
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
// keys.c


#include "quakedef.h"
#include "keys.h" // courtesy

/* key up events are sent even if in console mode */

int			key_linepos;
int			key_sellength;

int			key_lastpress;
int			key_insert = true;	//johnfitz -- insert key toggle (for editing)
double		key_blinktime; //johnfitz -- fudge cursor blinking to make it easier to spot in certain cases


keydest_e	key_dest = key_game;

int			key_count;			// incremented every key event

#ifdef SUPPORTS_KEYBIND_FLUSH
bind_t		keybindings[KEYMAP_COUNT_512];
#else // old way ...
char		*keybindings[KEYMAP_COUNT_512];
#endif // SUPPORTS_KEYBIND_FLUSH

int			keyshift[KEYMAP_COUNT_512];		// key to map to if shift held down in console
int			key_repeats[KEYMAP_COUNT_512];	// if > 1, it is autorepeating
cbool		consolekeys[KEYMAP_COUNT_512];	// if true, can't be rebound while in console
cbool		menubound[KEYMAP_COUNT_512];	// if true, can't be rebound while in menu
cbool		keydown[KEYMAP_COUNT_512];
cbool		keygamedown[KEYMAP_COUNT_512];  // Baker: to prevent -aliases from triggering
cbool		repeatkeys[KEYMAP_COUNT_512];	//johnfitz -- if true, autorepeat is enabled for this key

typedef struct
{
	const char	*name; // Save this to config
	int			keynum;
	const char	*config_name; // Show this to the user on-screen
} keyname_t;


keyname_t keynames[] =
{
	{"TAB", K_TAB},
	{"ENTER", K_ENTER},
	{"ESCAPE", K_ESCAPE},
	{"SPACE", K_SPACE},
	{"BACKSPACE", K_BACKSPACE},
	{"UPARROW", K_UPARROW},
	{"DOWNARROW", K_DOWNARROW},
	{"LEFTARROW", K_LEFTARROW},
	{"RIGHTARROW", K_RIGHTARROW},

	{"CTRL", K_CTRL},
#ifdef PLATFORM_OSX
	{"OPTION", K_WINDOWS, "WINDOWS"}, // Baker: 3rd param is write to config name
	{"COMMAND", K_ALT, "ALT"}, // Baker: 3rd param is write to config name
#else // Win32 ...
	{"WINDOWS", K_WINDOWS},
	{"ALT", K_ALT},
#endif // ! PLATFORM_OSX
	{"SHIFT", K_SHIFT},

	{"KP_NUMLOCK",		K_NUMLOCK},
	{"KP_SLASH",		K_NUMPAD_DIVIDE },
	{"KP_STAR",			K_NUMPAD_MULTIPLY },
	{"KP_MINUS",		K_NUMPAD_MINUS },
	{"KP_HOME",			K_NUMPAD_7 },   // K_KP_HOME
	{"KP_UPARROW",		K_NUMPAD_8 },   // K_KP_UPARROW
	{"KP_PGUP",			K_NUMPAD_9 },   // K_KP_PGUP
	{"KP_PLUS",			K_NUMPAD_PLUS },   // K_KP_PLUS
	{"KP_LEFTARROW",	K_NUMPAD_4 },   // K_KP_LEFTARROW
	{"KP_5",			K_NUMPAD_5 },   // K_KP_5
	{"KP_RIGHTARROW",	K_NUMPAD_6 },   // K_KP_RIGHTARROW
	{"KP_END",			K_NUMPAD_1 },   // K_KP_END
	{"KP_DOWNARROW",	K_NUMPAD_2 },   // K_KP_DOWNARROW
	{"KP_PGDN",			K_NUMPAD_3 },   // K_KP_PGDN
//	{"KP_ENTER",		K_KP_ENTER },	// Sorry, charlie.
	{"KP_INS",			K_NUMPAD_0}, // K_KP_INS
	{"KP_DEL",			K_NUMPAD_PERIOD }, // K_KP_DEL

	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},

	{"INS", K_INSERT},
	{"DEL", K_DELETE},
	{"PGDN", K_PAGEDOWN},
	{"PGUP", K_PAGEUP},
	{"HOME", K_HOME},
	{"END", K_END},

	{"MOUSE1", K_MOUSE1},
	{"MOUSE2", K_MOUSE2},
	{"MOUSE3", K_MOUSE3},
	{"MOUSE4", K_MOUSE4},
	{"MOUSE5", K_MOUSE5},

	{"JOY1", K_JOY1},
	{"JOY2", K_JOY2},
	{"JOY3", K_JOY3},
	{"JOY4", K_JOY4},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},
	{"AUX17", K_AUX17},
	{"AUX18", K_AUX18},
	{"AUX19", K_AUX19},
	{"AUX20", K_AUX20},
	{"AUX21", K_AUX21},
	{"AUX22", K_AUX22},
	{"AUX23", K_AUX23},
	{"AUX24", K_AUX24},
	{"AUX25", K_AUX25},
	{"AUX26", K_AUX26},
	{"AUX27", K_AUX27},
	{"AUX28", K_AUX28},
	{"AUX29", K_AUX29},
	{"AUX30", K_AUX30},
	{"AUX31", K_AUX31},
	{"AUX32", K_AUX32},

	{"PAUSE", K_PAUSE},

	{"MWHEELUP", K_MOUSEWHEELUP},
	{"MWHEELDOWN", K_MOUSEWHEELDOWN},

	{"SEMICOLON", ';'},	// because a raw semicolon separates commands
	{"BACKQUOTE", '`'},	// allow binding of backquote/tilde to toggleconsole after unbind all
	{"TILDE", '`'},		// allow binding of backquote/tilde to toggleconsole after unbind all

	{NULL, 0 } // Baker: Note that this list has null termination
};



/*
==============================================================================

			LINE TYPING INTO THE CONSOLE

==============================================================================
*/

void AdjustConsoleHeight (const float delta)
{
	if (!cl.worldmodel || cls.signon != SIGNONS)
		return;

	console1.user_pct += (float)delta;
	console1.user_pct = CLAMP (CONSOLE_MINIMUM_PCT_10, console1.user_pct, CONSOLE_MAX_USER_PCT_90);
}


/*
====================
Key_Console -- johnfitz -- heavy revision

Interactive line editing and console scrollback
====================
*/

void Partial_Reset (void)
{
	key_inpartial = false;
	key_partial_start = NULL;
	key_partial_end = NULL;
}

void Partial_Enable (void)
{
	key_inpartial = true;

}


void Partial_Accept (void)
{



}

// If we were in the undo buffer, we aren't now.
// Should happen any time a material change is made to text.
void Con_Undo_Point (int action, cbool was_space)
{
	Undo_Set_Point (&console1.undo_buffer, &history_lines[edit_line][0], key_linepos, key_sellength, action, was_space);
}

cbool Con_Undo_Walk (int direction)
{
	const char *new_text = Undo_Walk (&console1.undo_buffer, direction, &history_lines[edit_line][0], &key_linepos, &key_sellength);

	if (!new_text) // No new text
		return false;

	strlcpy (&history_lines[edit_line][0], new_text, CONSOLE_MAX_CMDLINE_256);
	return true;
}

// If the cursor moves, set action to 0 for most recent entry
void Con_Undo_CursorMove (void)
{
	// What if into undo buffer?  I don't think it matters because top entry is redo which just gets nuked.
	if (console1.undo_buffer.count && console1.undo_buffer.undo_entries[0].action)
		console1.undo_buffer.undo_entries[0].action = 0;
}


// This should happen about every time that key_linepos is set to 1 or the line is cleared or history walked.
void Con_Undo_Clear (void)
{
	Undo_Clear (&console1.undo_buffer);
}



void Key_Console_Cursor_Move(int netchange, cursor_t action)
{
	switch (action)
	{
	case cursor_reset:		key_linepos = 1, key_sellength = 0; break;
	case cursor_reset_abs:	key_linepos = netchange, key_sellength = 0; break;
	case select_clear:		key_linepos += netchange, key_sellength = 0; break;
	case cursor_select:		key_linepos += netchange, key_sellength += netchange; break;
	case cursor_select_all:	key_linepos = netchange + 1, key_sellength = netchange; break;
	}
}

void Key_Console_Delete_Selection_Move_Cursor (char *workline)
{
	// PIX cursor at 2, sellength -2.  cursor at 4, sellength 2.  2 and 3
	int len = strlen(workline);
	int netstart = key_sellength > 0 ? key_linepos - key_sellength : key_linepos;
	int netafter = netstart + abs(key_sellength);
	int cursormovement = key_sellength > 0 ? -key_sellength : 0;
	int netmove = len - netstart + 1; // +1 to move the null term too

	memmove (&workline[netstart], &workline[netafter], netmove);
	Key_Console_Cursor_Move (cursormovement, select_clear); // Selection clear
}


void Key_Console (int key)
{
	static	char current[CONSOLE_MAX_CMDLINE_256] = "";
	int	history_line_last;
	size_t		len;
	char *workline = history_lines[edit_line];
	cbool iscopy = false;
	cbool isremove = false;
	cbool isdone = false;

	//Con_Queue_PrintLinef ("Key_Console: (rnd %3d) code = %d rep: '%c'", (int)(RANDOM_FLOAT * 100), key, key);

	// Any non-shift action should clear the selection?
	// Remember the typing of a normal key needs to stomp the selection
	if (key_sellength)
	{
		// clipboard setters: copy is CTRL+C or CTRL-X (cut, which copies) or SHIFT+DEL (cut, which copies)
		cbool iscopy = (keydown[K_CTRL] && (key == 'X' || key == 'x' || key == 'C' || key == 'c')) ||
						(keydown[K_SHIFT] && key == K_DELETE);

		// clipboard retrievers: CTRL+V and SHIFT+INSERT
		cbool ispaste =  (keydown[K_CTRL] && (key == 'V' || key == 'v')) ||
						(keydown[K_SHIFT] && key == K_INSERT);

		// Remove the selection, which should be several things really but these won't do their normal thing.
		cbool isremoveatom = (keydown[K_CTRL] && (key == 'X' || key == 'x')) || key == K_BACKSPACE || key == K_DELETE;

		if (iscopy)
		{
			char buf[CONSOLE_MAX_CMDLINE_256];
			int netstart = key_sellength > 0 ? key_linepos - key_sellength : key_linepos;

			memcpy (buf, &workline[netstart], abs(key_sellength));
			buf[abs(key_sellength)]=0;
			Clipboard_Set_Text (buf);
//			Con_PrintLinef ("Clipboard Set " QUOTED_S, buf);
			S_LocalSound ("hknight/hit.wav");
		}

		// If we are pasting, we delete the selection first.  If we are removing, obviously same.
		if (ispaste || isremoveatom)
		{
			Con_Undo_Point (0, false);
			Key_Console_Delete_Selection_Move_Cursor (workline);
		}

		if (isremoveatom)
		{
			// Already did everything we needed to do.  This is delete and backspace.
			Partial_Reset ();
			return;
		}
	}

	switch (key)
	{
	case K_ENTER:
//	case K_KP_ENTER:  Not supported.
		Partial_Reset (); Con_Undo_Clear ();
		Cbuf_AddTextLine (&workline[1]);	// skip the prompt
		Con_PrintLinef ("%s", workline);

		// If the last two lines are identical, skip storing this line in history
		// by not incrementing edit_line
		if (strcmp(workline, history_lines[(edit_line-1) & (HISTORY_LINES_64 - 1)]))
			edit_line = (edit_line + 1) & (HISTORY_LINES_64 - 1);

		history_line = edit_line;
		history_lines[edit_line][0] = ']';
		history_lines[edit_line][1] = 0; //johnfitz -- otherwise old history items show up in the new edit line

		Key_Console_Cursor_Move (0, cursor_reset); // Reset selection
		if (cls.state == ca_disconnected)
			SCR_UpdateScreen (); // force an update, because the command may take some time
		return;

	case K_TAB:
		if (key_inpartial == false)
			Con_Undo_Point (0,0); // We don't want to set an undo point for each tab completion

		{
			Key_Console_Cursor_Move (0, select_clear); // Reset selection
			key_linepos += Con_AutoComplete (&history_lines[edit_line][1], CONSOLE_MAX_CMDLINE_256 - 1, key_linepos - 1, false /* no force */, &key_inpartial,
							  &key_completetype, (const char **)&key_partial_start, (const char **)&key_partial_end, keydown[K_SHIFT]);

			// This is now rewritten that we must take the result and do something with it.
			//if (new_partial)
			// Must remove old partial.  How do we do that?
		}
		return;

	case K_SPACE:
		if (keydown[K_CTRL])
		{
			// Ctrl+Space if in partial has no effect
			//if (key_inpartial == false)
			{
				Key_Console_Cursor_Move (0, select_clear); // Reset selection
				Partial_Reset (); Con_Undo_Point (0,0);

				key_linepos += Con_AutoComplete (&history_lines[edit_line][1], CONSOLE_MAX_CMDLINE_256 - 1, key_linepos - 1, true /* yes, force */, &key_inpartial,
								  &key_completetype, (const char **)&key_partial_start, (const char **)&key_partial_end, false /* not reverse */);

				// This is now rewritten that we must take the result and do something with it.
			}
			return;
		}
		break;

	case K_BACKSPACE:
		// Don't need to worry about sellength, it would be handled above + return out of func
		Partial_Reset ();

		//if (keydown[K_SHIFT])
		//	return; // Already did whatever we were going to do.  Really?  Does shift backspace do anyway?
		if (key_linepos > 1)
		{
			Con_Undo_Point (-1, workline[key_linepos - 1] == ' ' ); // -1 = delete, 2nd parameter = was it a space

			// Pull everything back and do move the cursor
			workline += key_linepos - 1;
			if (workline[1])
			{
				len = strlen(workline);
				memmove (workline, workline + 1, len);
			}
			else	*workline = 0;
			key_linepos--;
		}
		return;

	case K_DELETE:
		// Don't need to worry about sellength, it would be handled above + return out of func
		Partial_Reset ();

		if (keydown[K_SHIFT])
			return; // Anything we would do was already handled

		if (workline[key_linepos] == 0)
			return; // End of line, nothing to do

		Con_Undo_Point (-1, workline[key_linepos - 1] == ' ');

		len = strlen(&workline[key_linepos + 1]);
		memmove (&workline[key_linepos], &workline[key_linepos + 1], len + 1); // +1 for null
		return;

	case K_HOME:
		Partial_Reset ();  Con_Undo_Point (0,0);
		if (keydown[K_CTRL])
		{
			//skip initial empty lines
			int		i, x;
			char	*line;

			for (i = console1.cursor_row - console1.buffer_rows + 1 ; i <= console1.cursor_row ; i++)
			{
				line = console1.text + (i % console1.buffer_rows) * console1.buffer_columns;
				for (x = 0 ; x < console1.buffer_columns ; x++)
				{
					if (line[x] != ' ')
						break;
				}
				if (x != console1.buffer_columns)
					break;
			}
			console1.backscroll = CLAMP(0, ((console1.cursor_row - i) % console1.buffer_rows) - 2, console1.buffer_rows-(clheight >> 3) - 1);
		}
		else Key_Console_Cursor_Move (1 - key_linepos, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
		return;

	case K_END:
		Partial_Reset ();  Con_Undo_Point (0,0);
		if (keydown[K_CTRL])
			console1.backscroll = 0; // This doesn't need to set key_linepos?
		else
		{
			len = strlen(workline);
			Key_Console_Cursor_Move (len - key_linepos, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
		}
		return;

	case K_PAGEUP:
	case K_MOUSEWHEELUP:
		// key_tabpartial[0] = 0;  one of the few things that shouldn't reset partial
		console1.backscroll += keydown[K_CTRL] ? ((console1.visible_lines_conscale>>3) - 4) : 2;
		if (console1.backscroll > console1.buffer_rows - (int)(clheight >> 3) - 1)
			console1.backscroll = console1.buffer_rows - (int)(clheight >> 3) - 1;
		return;

	case K_PAGEDOWN:
	case K_MOUSEWHEELDOWN:
		// key_tabpartial[0] = 0;  one of the few things that shouldn't reset partial
		console1.backscroll -= keydown[K_CTRL] ? ((console1.visible_lines_conscale>>3) - 4) : 2;
		if (console1.backscroll < 0)
			console1.backscroll = 0;
		return;

	case K_LEFTARROW:
		Partial_Reset ();  Con_Undo_Point (0,0);
		len = strlen(workline);

		if (keydown[K_CTRL])
		{
			int delta, newpos = key_linepos;
			if (key_linepos > 1)
			{
				// Advance to beginning of previous word or end of line honoring punctuation
				int n, ch, ch_type, ch_type_prev;

				for (n = key_linepos - 1, ch_type_prev = 0, n = key_linepos; n >= 1; n --)
				{
					ch = workline[n];
					if (ch == ' ')										ch_type = 0;
					else if (isdigit(ch) || isalpha(ch) || ch =='_')	ch_type = 2;
					else												ch_type = 1;

					if (n == 1)
					{
						newpos = 1;
						break;
					}
					else if (n < key_linepos - 1 && ch_type_prev > 0 && ch_type != ch_type_prev)
					{
						newpos = n + 1;
						break;
					}
					ch_type_prev = ch_type;
				}
			}
			delta = newpos - key_linepos;
			Key_Console_Cursor_Move (delta, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
			key_blinktime = realtime;
			return;
		}
		Key_Console_Cursor_Move (key_linepos > 1 ? -1 : 0, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
		key_blinktime = realtime;
		return;

	case K_RIGHTARROW:
		Partial_Reset ();  Con_Undo_Point (0,0);
		len = strlen(workline);

		if (keydown[K_CTRL])
		{
			int delta, newpos = key_linepos;
			if (key_linepos < (int)len)
			{
				// Advance to beginning of next word or end of line honoring punctuation
				int n, ch, ch_type, ch_type_prev;

				for (n = key_linepos, ch_type_prev = 0, n = key_linepos; n <= (int)len; n ++)
				{
					ch = workline[n];
					if (ch == ' ')										ch_type = 0;
					else if (isdigit(ch) || isalpha(ch) || ch =='_')	ch_type = 2;
					else												ch_type = 1;

					if (n == (int)len || ( n > key_linepos && ch_type > 0 && ch_type != ch_type_prev))
					{
						newpos = n;
						break;
					}
					ch_type_prev = ch_type;
				}
			}
			delta = newpos - key_linepos;
			Key_Console_Cursor_Move (delta, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
			key_blinktime = realtime;
			return;
		}

		if ((int)len == key_linepos)
		{

			// Pressing RIGHT retreives stuff from previous line.
			len = strlen(history_lines[(edit_line + (HISTORY_LINES_64 - 1)) & (HISTORY_LINES_64 - 1)]);
			// Nothing to get, get out!  But if shift wasn't pressed, do a reset
			if ((int)len <= key_linepos)
			{
				Key_Console_Cursor_Move (0, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
				return; // no character to get
			}
			workline += key_linepos;
			*workline = history_lines[(edit_line + (HISTORY_LINES_64 - 1)) & (HISTORY_LINES_64 - 1)][key_linepos];
			workline[1] = 0;
		}
		Key_Console_Cursor_Move (1, keydown[K_SHIFT] ? cursor_select : select_clear); // Reset selection
		key_blinktime = realtime;
		return;

	case K_UPARROW:
		if (keydown[K_CTRL])
		{
			AdjustConsoleHeight (-0.05); // Baker: Decrease by 5%
			return;
		}

		if (history_line == edit_line)
			c_strlcpy(current, workline);

		Partial_Reset (); Con_Undo_Clear ();

		history_line_last = history_line;
		do
		{
			history_line = (history_line - 1) & (HISTORY_LINES_64 - 1);
		} while (history_line != edit_line && !history_lines[history_line][1]);

		if (history_line == edit_line)
		{
			history_line = history_line_last;
			return;
		}
		strlcpy (workline, history_lines[history_line], CONSOLE_MAX_CMDLINE_256);
		Key_Console_Cursor_Move (strlen(workline), cursor_reset_abs); // Reset selection
		return;

	case K_DOWNARROW:
		if (keydown[K_CTRL])
		{
			AdjustConsoleHeight (0.05); // Baker: Decrease by 5%
			return;
		}

		if (history_line == edit_line) // No effect
			return;

		Partial_Reset (); Con_Undo_Clear ();

		do
		{
			history_line = (history_line + 1) & (HISTORY_LINES_64 - 1);
		} while (history_line != edit_line && !history_lines[history_line][1]);

		if (history_line == edit_line)
			strlcpy (workline, current, CONSOLE_MAX_CMDLINE_256);
		else strlcpy (workline, history_lines[history_line], CONSOLE_MAX_CMDLINE_256);
		Key_Console_Cursor_Move (strlen(workline), cursor_reset_abs); // Reset selection
		return;

	case K_INSERT:
		if (keydown[K_SHIFT])
		{
			// Paste
			const char	*clip_text = Clipboard_Get_Text_Line (); // chars < ' ' removed
			Partial_Reset ();  Con_Undo_Point (1,false);
			key_linepos += String_Edit_Insert_At (workline, CONSOLE_MAX_CMDLINE_256, clip_text, key_linepos);
			return;

		}
		key_insert ^= 1;
		return;

	case 'a':
	case 'A':
		if (keydown[K_CTRL])
		{
			Partial_Reset ();  Con_Undo_Point (0,0);
			Key_Console_Cursor_Move (strlen(&workline[1]), cursor_select_all); // Reset selection
			return;
		}
		break;

	case 'z':
	case 'Z':
		// One level of undo for cut and paste only
		// Any action AFTER cut and paste must clear redo buffer of 2.

		Partial_Reset ();
		if (keydown[K_CTRL])
		{
			if (!Con_Undo_Walk (keydown[K_SHIFT] ? -1 : 1))
				Con_DPrintLinef ("End of undo buffer");

			return;
		}
		break;


	case 'v':
	case 'V':
		if (keydown[K_CTRL])
		{
			const char	*clip_text = Clipboard_Get_Text_Line (); // chars < ' ' removed

			Partial_Reset ();  Con_Undo_Point (0,0);
			key_linepos += String_Edit_Insert_At (workline, CONSOLE_MAX_CMDLINE_256, clip_text, key_linepos);
			return;
		}
		break;

	case 'c':
	case 'C':
	case 'x':
	case 'X':

		if (keydown[K_CTRL])
		{
			// Unreachable for CTRL-X ?
			return; // Don't put a X or C in the buffer
		}
		break;
	}

	if (key < SPACE_CHAR_32 || key > MAX_ASCII_DELETE_CHAR_127 /* should be 126? */) // Baker: Is context scancode or ascii?  I'm not quite sure or both?  Future: Make it clear.  I think context is key_scancode_e  I think 127 is impossible here?
		return;	// non printable

	// If we had a selection, it is getting stomped now ...
	Con_Undo_Point (1, key == SPACE_CHAR_32);

	if (key_sellength)
		Key_Console_Delete_Selection_Move_Cursor (workline);
#pragma message ("If we aren't inserting, this is a defacto replace operation.  Need to effectively temp force key_insert to true if sellenth !=0")

	if (key_linepos < CONSOLE_MAX_CMDLINE_256 - 1 )
	{
		cbool endpos = !workline[key_linepos];

		Partial_Reset ();
		// if inserting, move the text to the right
		if (key_insert && !endpos)
		{
			workline[CONSOLE_MAX_CMDLINE_256 - 2] = 0;
			workline += key_linepos;
			len = strlen(workline) + 1;
			memmove (workline + 1, workline, len);
			*workline = key;
		}
		else
		{
			workline += key_linepos;
			*workline = key;
			// null terminate if at the end
			if (endpos)
				workline[1] = 0;
		}
		Key_Console_Cursor_Move (1, select_clear); // Reset selection
	}
	//johnfitz
}

//============================================================================

cbool chat_team = false;
char chat_buffer[MAX_CHAT_SIZE_45];
static int chat_bufferlen = 0;
void Key_EndChat (void)
{
	Key_SetDest (key_game);
	chat_bufferlen = 0;
	chat_buffer[0] = 0;
}

void Key_Message (int key)
{
	//Con_Queue_PrintLinef ("Key_Message: (rnd %3d) code = %d rep: '%c'", (int)(RANDOM_FLOAT * 100), key, key);
	if (key == K_ENTER)
	{
		if (chat_team)
			Cbuf_AddText ("say_team \"");
		else
			Cbuf_AddText ("say \"");

		Cbuf_AddText (chat_buffer);
		Cbuf_AddTextLine ("\""); // Add trailing quote

		Key_EndChat ();
		return;
	}

	if (key == K_ESCAPE)
	{
		Key_EndChat ();
		return;
	}

	if (key == K_BACKSPACE)
	{
		if (chat_bufferlen)
			chat_buffer[--chat_bufferlen] = 0;
		return;
	}

	if (key < 32 || key > 127)
		return;	// non printable

	if (chat_bufferlen == (int)sizeof(chat_buffer) - (chat_team ? 6 : 1)) // 6 vs 1 = so same amount of text onscreen in "say" versus "say_team"
		return; // all full

	if ( (key == 'v' || key == 'V') && keydown[K_CTRL])
	{
		int pastesizeof = sizeof(chat_buffer);
		pastesizeof -= chat_bufferlen;
		pastesizeof -= (chat_team ? 6 : 1);
		strlcpy (&chat_buffer[chat_bufferlen], Clipboard_Get_Text_Line (), pastesizeof);
		chat_bufferlen = strlen(chat_buffer);
		return;
	}


	chat_buffer[chat_bufferlen++] = key;
	chat_buffer[chat_bufferlen] = 0;
}

//============================================================================


/*
===================
Key_StringToKeynum

Returns a key number to be used to index keybindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int Key_StringToKeynum (const char *str)
{
	keyname_t	*kn;

	if (!str || !str[0])
		return -1;

	if (!str[1])
		return str[0];

	for (kn=keynames ; kn->name ; kn++)
	{
		if (!strcasecmp(str,kn->name) || (kn->config_name &&  !strcasecmp(str,kn->config_name)) )
			return kn->keynum;
	}
	return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
const char *Key_KeynumToString (int keynum, enum keyname_s nametype)
{
	static char tinystr[2];
	keyname_t	*kn;

	if (keynum == -1)
		return "<KEY NOT FOUND>";
	if (keynum > 32 && keynum < 127)
	{	// printable ascii
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}

	for (kn=keynames ; kn->name ; kn++)
	{
		if (keynum == kn->keynum)
		{
			if (nametype == key_export_name && kn->config_name)
				return kn->config_name;
			else return kn->name;
		}
	}

	return "<UNKNOWN KEYNUM>";
}

const char *Key_ListExport (void)
{
	static char returnbuf[32];

	static int last = -1; // Top of list.
	// We want first entry >= this
	int		wanted = CLAMP(0, last + 1, (int) (sizeof(keynames)/sizeof(keynames[0])) );  // Baker: bounds check
	keyname_t	*kn;
	int i;

	for (i = wanted, kn = &keynames[i]; kn->name ; kn ++, i++)
	{
		// Baker ignore single character keynames.
		if (memcmp(kn->name, "JOY", 3)==0)
			continue; // Baker --- Do not want
		if (memcmp(kn->name, "AUX", 3)==0)
			continue; // Baker --- Do not want

		if (kn->name[0] && kn->name[1] && i >= wanted) // Baker: i must be >=want due to way I setup this function
		{
			c_strlcpy (returnbuf, kn->name);

			last = i;
			//Con_PrintLinef ("Added %s", kn->name);
			return returnbuf;
		}
	}

	// Not found, reset to top of list and return NULL
	last = -1;
	return NULL;
}



#ifdef SUPPORTS_KEYBIND_FLUSH
const char *Key_GetBinding (int keynum)
{
	if (keynum == (KEYMAP_COUNT_512 - 1))
		return "toggleconsole";

	if (!in_range(0, keynum, KEYMAP_COUNT_512)) {
		Con_WarningLinef  ("Key_GetBinding: '%c' (%d) is not a valid key", keynum, keynum);
		return "";
	}
	return (keybindings[keynum].server ? keybindings[keynum].server : keybindings[keynum].real);
}
#endif // SUPPORTS_KEYBIND_FLUSH

/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding (int keynum, const char *binding)
{
#ifdef SUPPORTS_KEYBIND_FLUSH
	char **ptarget;
#endif // !SUPPORTS_KEYBIND_FLUSH

	if (keynum == -1)
		return; // Uh?  Seriously.  How would we get -1?


#ifdef SUPPORTS_KEYBIND_FLUSH
	// If the bind is from a player, stomp on the server bind (clear it)
	if (!cmd_from_server && keybindings[keynum].server) {
		// User stomp.  Free the server bind.
		Z_Free (  keybindings[keynum].server  );
		keybindings[keynum].server = NULL;
	}
	// If the bind is from the server, set the server bind.
	// If the bind is from the client, set the client bind.
	ptarget = cmd_from_server ? &keybindings[keynum].server : &keybindings[keynum].real;

	// free old bindings
	if ( (*ptarget) ) { //  keybindings[keynum])
		Z_Free (  (*ptarget)  );
		(*ptarget) = NULL;
	}

	// Only create copy if string is non-zero length
	if (binding[0])
		(*ptarget) = Z_Strdup(binding);  // allocate memory for new binding
#else
// free old bindings
	if (keybindings[keynum])
	{
		Z_Free (keybindings[keynum]);
		keybindings[keynum] = NULL;
	}

// allocate memory for new binding
	keybindings[keynum] = Z_Strdup(binding);

#endif // !SUPPORTS_KEYBIND_FLUSH
}

#ifdef SUPPORTS_KEYBIND_FLUSH
void Keys_Flush_ServerBinds (void)
{
	int j; for (j = 0; j < KEYMAP_COUNT_512; j++) {
		if (!keybindings[j].server)
			continue; // No bind
		Z_Free (keybindings[j].server);
		keybindings[j].server = NULL;
	}
	// End of function
}
#endif // SUPPORTS_KEYBIND_FLUSH

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f (lparse_t *line)
{
	int		b;

	if (cmd_from_server && pq_bindprotect.value) {
		if (pq_bindprotect.value > 1) // 1 means ignore.  2 means ignore + print
			Con_WarningLinef ("Server sent key unbind attempt, ignored: [%s]", line->original);
		return;
	}

	if (line->count != 2)
	{
		Con_PrintLinef ("unbind <key> : remove commands from a key");
		return;
	}

	b = Key_StringToKeynum (line->args[1]);
	if (b==-1)
	{
		Con_PrintLinef (QUOTED_S " isn't a valid key", line->args[1]);
		return;
	}
// SUPPORTS_KEYBIND_FLUSH -- Key_SetBinding is smart and figures out what to do just fine!
// No changes needed!
	Key_SetBinding (b, "");
}

void Key_Unbindall_f (lparse_t *line)
{
	int		i;

	if (cmd_from_server) {
		// pq_bindprotect not required to be set.  This is a presumed hostile
		// action.  Historically, a server admin who wanted to be a jerk
		// would use a menu that exists in some mods to be a jerk to a player.
		Con_WarningLinef ("Server sent unbindall command.  Ignoring.");
		return;
	}
	for (i = 0 ; i < KEYMAP_COUNT_512; i++)
	{
#if !defined(SUPPORTS_KEYBIND_FLUSH)
		if (keybindings[i])
#endif // !SUPPORTS_KEYBIND_FLUSH
		// We no longer think about it, Key_SetBinding does the thinking.
		Key_SetBinding (i, "");
	}
}

/*
============
Key_Bindlist_f -- johnfitz
============
*/
void Key_Bindlist_f (void)
{
	int	k, count;

	count = 0;
	for (k = 0; k < (KEYMAP_COUNT_512 - 1) /* -1 because of hard toggle console key*/ ; k ++)
	{
		const char *binding = Key_GetBinding (k);
		const char *permanent = (keybindings[k].server && keybindings[k].real) ? va ("(User: " QUOTED_S ")",  keybindings[k].real) : "";

		if (binding && *binding) {
			Con_SafePrintLinef ("   %-12s " QUOTED_S " %s", Key_KeynumToString(k, key_local_name), binding, permanent);
			count++;
		}
	}
	Con_SafePrintLinef ("%d bindings", count);
}


/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f (lparse_t *line)
{
	int			i, c, b;
	char		cmd[1024];

	if (cmd_from_server && pq_bindprotect.value) {
		if (pq_bindprotect.value > 1) // 1 means ignore.  2 means ignore + print
			Con_WarningLinef ("Server key bind, ignoring: [%s]", line->original);
		return;
	}

	c = line->count;

	if (c != 2 && c != 3)
	{
		Con_PrintLinef ("bind <key> [command] : attach a command to a key");
		return;
	}
	b = Key_StringToKeynum (line->args[1]);
	if (b==-1)
	{
		Con_PrintLinef (QUOTED_S " isn't a valid key", line->args[1]);
		return;
	}

	if (c == 2) {

		const char *binding = Key_GetBinding (b);
		const char *permanent = (keybindings[b].server && keybindings[b].real) ? va ("(User: " QUOTED_S ")",  keybindings[b].real) : "";

		if (binding)
			Con_PrintLinef (QUOTED_S " = " QUOTED_S " %s", line->args[1], binding, permanent);
		else
			Con_PrintLinef (QUOTED_S " is not bound", line->args[1] );
		return;
	}

// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (i=2 ; i< c ; i++)
	{
		c_strlcat (cmd, line->args[i]);
		if (i != (c-1))
			c_strlcat (cmd, " ");
	}
#ifdef SERVER_BINDS
	// Probably don't need to do anything special here.  Key_SetBinding should check to see if from server.
#endif

	Key_SetBinding (b, cmd);
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings (FILE *f)
{
	int		i;

	// unbindall before loading stored bindings:
	if (cfg_unbindall.value)
		fprintlinef (f, "unbindall");
	for (i = 0; i < KEYMAP_COUNT_512; i++)
	{
#ifdef SUPPORTS_KEYBIND_FLUSH
		// Always use real when writing.  Never the server bind.
		if (keybindings[i].real && *keybindings[i].real)
			fprintlinef (f, "bind " QUOTED_S " " QUOTED_S, Key_KeynumToString(i, key_export_name), keybindings[i].real);
#else // .. oldway
		if (keybindings[i] && *keybindings[i])
			fprintlinef (f, "bind " QUOTED_S " " QUOTED_S, Key_KeynumToString(i, key_export_name), keybindings[i]);
#endif // !SUPPORTS_KEYBIND_FLUSH
	}
}

#ifndef PLATFORM_OSX // Not Crusty Mac
static 
#endif // Not Crusty Mac
	int Key_Event (int key, cbool down, int special);

#ifdef PLATFORM_OSX

double last_key_down_next_time;
int last_key_down;
#define REPEAT_DELAY 0.500
#define REPEAT_INTERVAL 0.025
void Key_Console_Repeats (void)
{
	if (last_key_down && (key_dest != key_game || console1.forcedup) && realtime > last_key_down_next_time)
	{
		Key_Event (last_key_down, true, INSTRUCTION_NONE);
		last_key_down_next_time = realtime + REPEAT_INTERVAL;
	}
}
#endif // PLATFORM_OSX

/*
===================
Key_Init
===================
*/
void Key_Init (void)
{
	int		i;

	History_Init ();

	key_blinktime = realtime; //johnfitz

//
// init ascii characters in console mode
//

#pragma message ("Baker: What isn't a console key?  MOUSE1 + friends, anything else????")
	for (i = 32 ; i<128 ; i++)
		consolekeys[i] = true;
	consolekeys[K_ENTER] = true;
	consolekeys[K_TAB] = true;
	consolekeys[K_LEFTARROW] = true;
	consolekeys[K_RIGHTARROW] = true;
	consolekeys[K_UPARROW] = true;
	consolekeys[K_DOWNARROW] = true;
	consolekeys[K_BACKSPACE] = true;
	consolekeys[K_INSERT] = true;
	consolekeys[K_DELETE] = true;
	consolekeys[K_HOME] = true;
	consolekeys[K_END] = true;
	consolekeys[K_PAGEUP] = true;
	consolekeys[K_PAGEDOWN] = true;
	consolekeys[K_SHIFT] = true;
	consolekeys[K_MOUSEWHEELUP] = true;
	consolekeys[K_MOUSEWHEELDOWN] = true;
	consolekeys['`'] = false;
	consolekeys['~'] = false;

	//johnfitz -- repeating keys
	for (i = 32 ; i < 128 ; i++)
		repeatkeys[i] = true;
	repeatkeys[K_BACKSPACE] = true;
	repeatkeys[K_DELETE] = true;
	repeatkeys[K_PAUSE] = true;
	repeatkeys[K_PAGEUP] = true;
	repeatkeys[K_PAGEDOWN] = true;
	repeatkeys[K_UPARROW] = true;
	repeatkeys[K_DOWNARROW] = true;
	repeatkeys[K_LEFTARROW] = true;
	repeatkeys[K_RIGHTARROW] = true;
	//johnfitz

	// For USA keymap
	for (i = 0; i < KEYMAP_COUNT_512; i++)
		keyshift[i] = i;
	for (i= 'a' ; i <= 'z' ; i++)
		keyshift[i] = i - 'a' + 'A';
	keyshift['1'] = '!';
	keyshift['2'] = '@';
	keyshift['3'] = '#';
	keyshift['4'] = '$';
	keyshift['5'] = '%';
	keyshift['6'] = '^';
	keyshift['7'] = '&';
	keyshift['8'] = '*';
	keyshift['9'] = '(';
	keyshift['0'] = ')';
	keyshift['-'] = '_';
	keyshift['='] = '+';
	keyshift[','] = '<';
	keyshift['.'] = '>';
	keyshift['/'] = '?';
	keyshift[';'] = ':';
	keyshift['\''] = '"';
	keyshift['['] = '{';
	keyshift[']'] = '}';
	keyshift['`'] = '~';
	keyshift['\\'] = '|';

	menubound[K_ESCAPE] = true;
	for (i=0 ; i<12 ; i++)
		menubound[K_F1+i] = true;

//
// register our functions
//

	Cmd_AddCommands (Key_Init);
}

/*
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/

cbool Key_Alt_Down (void) { return keydown[K_ALT]; }
cbool Key_Ctrl_Down (void) { return keydown[K_CTRL]; }
cbool Key_Shift_Down (void) { return keydown[K_SHIFT]; }



cbool ignore_enter_up = false;
cbool ignore_tab_up = false;
cbool ignore_c_up = false;
cbool ignore_m_up = false;
cbool ignore_v_up = false;

#ifndef PLATFORM_OSX // Not Crusty Mac
void Key_Event_Ex (void *ptr, key_scancode_e scancode, cbool down, int ascii, int unicode, int shift)
{
	keydest_e desto;
	int sendkey;

	if (scancode) {
		// Override certain keys.
		switch (scancode) {
		case K_RALT:		scancode = K_LALT;		break;
		case K_RCTRL:		scancode = K_LCTRL;		break;
		case K_RSHIFT:		scancode = K_LSHIFT;	break;
		}


		// Quake uses the lower case as the bind names.
		scancode = tolower(scancode);
	}

	if (!in_keymap.value) {
		// Keymap off does not accept ascii emission
		if (!scancode)
			return;
		Key_Event (scancode, down, 0);  // Emulate the "old way"
		return;
	}

	// ALT-M mute, CTRL-C are just out luck on a drastically different non-US keyboard.
	// Most European keyboards are QWERTY, QWERTZ or AWERTY and all seem to have those keys in same place.


	sendkey = 0;

	//
	// SCAN CODE:  PHYSICAL KEY PRESS.  ALL OF THEM PASS THROUGH HERE
//	Con_PrintLinef ("Scancode: %c %d  ascii %c %d", CLAMP(32, scancode, 127), scancode, CLAMP(32, ascii, 127), ascii);
	if (scancode) {
		// Run Key_Event but tell it to screen out menu/mm/console if in_keymap set.
		desto = Key_Event (scancode, down, (int) in_keymap.value);
		if (!desto) return; 	// Console processed everything, so get out.
		if (!down)  return;		// Keyups only go to game.

		// We are considering passing our scancode to the menu/console.  If it's ascii, don't!
		if (in_range (SPACE_CHAR_32, scancode, MAX_ASCII_PRINTABLE_126))
			return;

		// Send this to console or menu
		sendkey = scancode;
	}

	//
	// KEY EMISSION:  NOT A PHYSICAL KEY EVENT, BUT INTERPRETATION
	//
	if (!scancode) {
		// Control key come from scancode only.  Get out.
		if (!in_range (SPACE_CHAR_32, ascii, MAX_ASCII_PRINTABLE_126))
			return;

		// Quake exception.  Under normal cirumstances, this must bring up the menu.

		if (ascii == '`' /* backquote */)
			return;

		if (ascii == '~' /* tilde */)
			return;




		// Unacceptable destination.  We only send to console/menu.
		if (key_dest == key_game && !console1.forcedup)
			return;

		// Unacceptable destination.  We only send scancodes to key grab.
		if (key_dest == key_menu && m_keys_bind_grab)
			return;

		// We do not do control keys.
		if (Key_Ctrl_Down ())
			return; // We interpret CTRL + combinations as scancodes only.

		sendkey = ascii;
		desto = key_dest;
	}

	// Route it.
	switch (desto) {
	case key_message:	Key_Message (sendkey);  break;
	case key_menu:		M_Keydown (sendkey);	break;
	case key_game:		// Fall through ... may happen in rude disconnect/Host_Error scenarios.
	case key_console:	Key_Console (sendkey);  break;
	}
}


static 
#endif // !PLATFORM_OSX crusty Mac
int Key_Event (int key, cbool down, int special)
{
	const char	*kb;
	char		cmd[1024];
	cbool wasgamekey = false;

#pragma message ("Baker: There is the possibility of crosswired double key ups and/or double key downs")
#pragma message ("for keys that have multiple paths, like right and left shift.  How to handle?")
	//if (key_dest == key_game) Con_Queue_PrintLinef ("Key_Console: (rnd %3d) code = %d rep: '%c'", (int)(RANDOM_FLOAT * 100), key, key);


	switch (key)
	{
	case K_ENTER:
		if (keydown[K_ALT] && down)
		{
			VID_Alt_Enter_f ();
			ignore_enter_up = true;
			return 0; // Didn't happen!
		}
		else if (ignore_enter_up)
		{
			ignore_enter_up = false;
			if (!down) return 0; // Didn't happen!
		}
		break;

	case K_TAB:
#ifdef PLATFORM_OSX
		if (keydown[K_CTRL] && down && vid.screen.type == MODE_WINDOWED)
#else
		if (keydown[K_ALT] && down)
#endif // PLATFORM_OSX
		{
			ignore_tab_up = true;
			return 0;
		} else if (ignore_tab_up)
		{
			ignore_tab_up = false;
			if (!down) return 0; // Didn't happen!
		}
		break;

	case 'm':

		if (keydown[K_ALT] && down)
		{
			Sound_Toggle_Mute_f ();
			ignore_m_up = true;
			return 0; // Didn't happen!
		}
		else if (ignore_m_up)
		{
			ignore_m_up = false;
			if (!down) return 0; // Didn't happen!
		}
		break;

#ifdef GLQUAKE_TEXTURE_POINTER // Baker: Would be ton of work to make work in software
	case 'c':
#pragma message ("abort if key_dest isn't game")
		if (texturepointer_on && keydown[K_CTRL] && down && (key_dest == key_game && !console1.forcedup) )
		{
			TexturePointer_ClipboardCopy ();
			ignore_c_up = true;
			return 0; // Didn't happen!
		}
		else if (ignore_v_up)
		{
			ignore_c_up = false;
			if (!down) return 0; // Didn't happen!
		}
		break;

	case 'v':
		if (texturepointer_on && keydown[K_CTRL] && down && (key_dest == key_game && !console1.forcedup))
		{
			TexturePointer_ClipboardPaste ();
			ignore_v_up = true;
			return 0; // Didn't happen!
		}
		else if (ignore_v_up)
		{
			ignore_v_up = false;
			if (!down) return 0; // Didn't happen!
		}
		break;
#endif // GLQUAKE_TEXTURE_POINTER

	default:
		break;
	}


	keydown[key] = down;

#ifdef PLATFORM_OSX
	if (down && (key_dest != key_game || console1.forcedup) && repeatkeys[key])
	{
		last_key_down = key;
		last_key_down_next_time = realtime + REPEAT_DELAY;
	}
	else last_key_down = 0;
#endif // PLATFORM_OSX

	wasgamekey = keygamedown[key]; // Baker: to prevent -aliases being triggered in-console needlessly
	if (!down)
	{
		keygamedown[key] = false; // We can always set keygamedown to false if key is released
	}

	if (!down)
		key_repeats[key] = 0;

	key_lastpress = key;
	key_count++;
	if (key_count <= 0)
	{
		return 0;		// just catching keys for Con_NotifyBox or Modal or something else.
	}


// update auto-repeat status
	if (down)
	{
#ifdef PLATFORM_OSX
        if (keydown[K_ALT] == true) // Which is K_COMMAND
        {
            key = toupper (key);

            // ignore certain command-key combinations
            switch (key)
            {
                case K_TAB: // Doesn't work in fullscreen mode
                case 'H': // Hide (Baker: Seems to cause us to terminate in fullscreen mode, so I murderered it)
                case 'M': // Minimize (Works in fullscreen mode too.  Well at least it did ???  Seems to no longer work.)
                case 'Q': // Quit (Works in fullscreen mode I believe)
                case '?': // Pulls up help-search in windowed mode (doesn't in fullscreen mode)
                    break;
            }
        }
#endif // PLATFORM_OSX

		key_repeats[key]++;
		if (key_repeats[key] > 1)
		{
			if (key_dest == key_game && !console1.forcedup)
				return 0;	// ignore autorepeats in game mode
		}
	}


//
// handle escape specialy, so the user can never unbind it
//
#pragma message ("Baker: Maybe add toggle console to this for tilde")
	if (key == K_ESCAPE)
	{
		if (!down)
			return 0;

		if (Key_Shift_Down ())
		{
			Con_ToggleConsole_f (NULL);
			return 0;
		}

		switch (key_dest)
		{
		case key_message:
			Key_Message (key);
			break;
		case key_menu:
			M_Keydown (key);
			break;
		case key_game:
		case key_console:
			M_ToggleMenu_f (NULL);
			break;
		default:
			System_Error ("Bad key_dest");
		}
		return 0;
	}

#define DEMO_FAST_FORWARD_REVERSE_SPEED_5 5
// PGUP and PGDN rewind and fast-forward demos
//#ifdef _WIN32
//#define REVERSE_KEY K_PAGEDOWN
//#define FORWARD_KEY K_PAGEUP
//#else // MAC laptops don't usually have those keys
//#define REVERSE_KEY K_LEFTARROW
//#define FORWARD_KEY K_RIGHTARROW
//#endif
	if (cls.demoplayback && cls.demonum == -1 && !cls.timedemo && !cls.capturedemo)
	{
		if (key == K_DOWNARROW)
		{
			if (key_dest == key_game && down)
				Host_Pause_f (NULL);

		}
		else
		if (key == K_LEFTARROW || key == K_RIGHTARROW)
		{
			if (key_dest == key_game && down /* && cls.demospeed == 0 && cls.demorewind == false*/)
			{
				// During normal demoplayback, PGUP/PGDN will rewind and fast forward (if key_dest is game)
				if (key == K_RIGHTARROW)
				{
					cls.demospeed = DEMO_FAST_FORWARD_REVERSE_SPEED_5;
					cls.demorewind =  false;
				}
				else if (key == K_LEFTARROW)
				{
					cls.demospeed = DEMO_FAST_FORWARD_REVERSE_SPEED_5;
					cls.demorewind = true;
				}
				return 0; // If something is bound to it, do not process it.
			}
			else //if (!down && (cls.demospeed != 0 || cls.demorewind != 0))
			{
				// During normal demoplayback, releasing PGUP/PGDN resets the speed
				// We need to check even if not key_game in case something silly happened (to be safe)
				cls.demospeed = 0;
				cls.demorewind = false;

				if (key_dest == key_game)
					return 0; // Otherwise carry on ...
			}
		}
	}

//
// key up events only generate commands if the game key binding is
// a button command (leading + sign).  These will occur even in console mode,
// to keep the character from continuing an action started before a console
// switch.  Button commands include the kenum as a parameter, so multiple
// downs can be matched with ups
//
	if (!down)
	{
		// Baker: we only want to trigger -alias if appropriate
		//        but we ALWAYS want to exit if the key is up
		if (wasgamekey)
		{
#ifdef SUPPORTS_KEYBIND_FLUSH
			kb = Key_GetBinding(key);
#else // oldway ..
			kb = keybindings[key];
#endif // !SUPPORTS_KEYBIND_FLUSH
			if (kb && kb[0] == '+')
			{
				c_snprintf2 (cmd, "-%s %d\n", kb + 1, key);
				Cbuf_AddText (cmd);
			}
#ifdef PLATFORM_OSX // Crusty Mac
			if (keyshift[key] != key)
#else
			if (!in_keymap.value && keyshift[key] != key) // International keyboard. Are we still doing this?  I didn't know we were?
#endif
			{
#ifdef SUPPORTS_KEYBIND_FLUSH
				kb = Key_GetBinding(keyshift[key]); // International keyboard. Are we still doing this?  I didn't know we were?
#else // oldway ...
				kb = keybindings[keyshift[key]];
#endif // !SUPPORTS_KEYBIND_FLUSH
				if (kb && kb[0] == '+')
				{
					c_snprintf2 (cmd, "-%s %d\n", kb + 1, key);
					Cbuf_AddText (cmd);
				}
			}
		}
		return 0;
	}

//
// during demo playback, most keys bring up the main menu
//
// Baker:  Quake was intended to bring up the menu with keys during the intro.
// so the user knew what to do.  But if someone does "playdemo" that isn't the intro.
// So we want this behavior ONLY when startdemos are in action.  If startdemos are
// not in action, cls.demonum == -1

	if (cls.demonum >= 0) // We are in startdemos intro.  Bring up menu for keys.
	{
		if (cls.demoplayback && down && consolekeys[key] && key_dest == key_game)
		{
			M_ToggleMenu_f (NULL);
			return 0;
		}
	}

//
// if not a consolekey, send to the interpreter no matter what mode is
//
#pragma message ("Baker: I'm not so sure about this")
#pragma message ("Baker: What value is running aliases in the menu and such or when no map running")
#pragma message ("Baker: This is used by keygamedown allowing releases at any time")

	// Baker: See you can get this "right".
	// This really probably should say key_dest == game && !console1.forcedup
	if ( (key_dest == key_menu && menubound[key]) ||
		(key_dest == key_console && !consolekeys[key]) ||
			(key_dest == key_game && ( !console1.forcedup || !consolekeys[key] ) ) )
	{
#ifdef SUPPORTS_KEYBIND_FLUSH
		kb = Key_GetBinding (key);
#else // oldway ...
		kb = keybindings[key];
#endif // !SUPPORTS_KEYBIND_FLUSH
	// I think this should be (kb && kb[0])

		if (kb)
		{
			// Baker: if we are here, the key is down
			//        and if it is retrigger a bind
			//        it must be allowed to trigger the -bind
			//
			keygamedown[key] = true; // Let it be untriggered anytime

			if (kb[0] == '+')
			{	// button commands add keynum as a parm
				//c_snprintf2 (cmd, "%s %d" NEWLINE, kb, key);
				//Cbuf_AddTextLinef (cmd);
				Cbuf_AddTextLinef ("%s %d", kb, key);
			}
			else
			{
				Cbuf_AddTextLine (kb);
			}
		}
		return 0;
	}

#pragma message ("Baker: At this point the key must be down")
	// Baker: I think this next line is unreachable!
	if (!down)
		return 0;		// other systems only care about key down events

	// USA keymap
#ifdef PLATFORM_OSX // Crusty Mac
	if (keydown[K_SHIFT])
#else
	if (!special && !in_keymap.value && keydown[K_SHIFT]) // in_keymap 0 only, right?
#endif // ! PLATFORM_OSX Crusty Mac
		key = keyshift[key];

	// Let CTRL + (key) combos through to the console/menu, even if we are instructed to filter.
	if (special && Key_Ctrl_Down () && in_range(SPACE_CHAR_32, key, MAX_ASCII_PRINTABLE_126))
		special = 0;

	switch (key_dest)
	{
	case key_message:
		if (special)  return key_message; // Don't do it.  Let KeyEventEx do it.
		Key_Message (key);
		break;

	case key_menu:
		// If we are binding a key, we should send the scancode.

		if (special && !m_keys_bind_grab) return key_menu;
		M_Keydown (key);
		break;

	case key_game:
		// This bastard falls through?  Should it?  Yes, it's in the original Quake and FitzQuake 0.85 too!
		// Baker: My testing shows that this doesn't happen.  Possibly ever.  Have not tested the rude disconnect scenario, not going to bother.
		// Whatever the circumstance, it's scarce to non-existent, at least in Mark V.


	case key_console:
		if (special) return key_console;  // Don't do it.  Let KeyEventEx do it.


//		Con_PrintLinef ("Before:");
//		Undo_Dump (&console1.undo_buffer);
		Key_Console (key);
//		Con_PrintLinef ("After:");
//		Undo_Dump (&console1.undo_buffer);
		break;

	default:
		System_Error ("Bad key_dest");
	}
	return 0;
}


/*
===================
Key_ClearStates
===================
*/
void Key_Release_Keys (cvar_t* var)
{
   int      i;

	for (i = 0 ; i < KEYMAP_COUNT_512 ; i++)
	{
		if (keydown[i])
#ifdef PLATFORM_OSX // Crusty Mac
			Key_Event (i, false, 0 /* not special*/ );
#else
			Key_Event_Ex (NO_WINDOW_NULL, i, false, ASCII_0, UNICODE_0, CORE_SHIFTBITS_UNREAD_NEG1);
#endif // ! PLATFORM_OSX Crusty Mac
	}
}

// Baker: This function is separate because there are situations where we
// want to release the mouse and we lose the events as a result.
// However, we don't want to release the keyboard.  An example, is opening the
// console in windowed mode.  However, if we release the mouse we aren't
// getting button up messages so let us release the mouse buttons now.
void Key_Release_Mouse_Buttons (void)
{
	int i; for (i = 0 ; i < INPUT_NUM_MOUSE_BUTTONS ; i ++) {
		if (keydown[K_MOUSE1 + i])
#ifdef PLATFORM_OSX // Crusty Mac
			Key_Event (i, false, 0 /* not special*/ );
#else
			Key_Event_Ex (NO_WINDOW_NULL, K_MOUSE1 + i, false, ASCII_0, UNICODE_0, CORE_SHIFTBITS_UNREAD_NEG1);
#endif // !PLATFORM_OSX Crusty Mac
	}
}


void Key_SetDest (keydest_e newdest)
{
	if (key_dest == newdest)
		return; // No change

#ifdef PLATFORM_WINDOWS
#ifndef CORE_SDL
	if (key_dest == key_game || newdest == key_game) {
		// A switch to or away from using scancodes
		if (in_keymap.value) // 1005
			Shell_Input_ResetDeadKeys ();

	}
#endif // !CORE_SDL
#endif // PLATFORM_WINDOWS


	if (key_dest == key_console || console1.forcedup)
	{
		// Changed away from console
		Con_Exit ();
	}
	else if (key_dest == key_menu)
	{
		// Changed away from menu
		M_Exit (); // Sets m_state = m_none, but Menu has right to do other things and be notified.
	}
	key_dest = newdest;
}