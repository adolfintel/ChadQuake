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
// console.h

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

//
// console
//

#define MAX_CHAT_SIZE_45 45  // The ProQuake server limit is 64, though.  Might even be in standard Quake.
extern char chat_buffer[MAX_CHAT_SIZE_45];


#define		CONSOLE_TEXTSIZE		(1024*256)	// Baker: Was 65536 in FitzQuake 0.85
#define		CONSOLE_MINSIZE			16384		//johnfitz -- old default, now the minimum size
#define		CONSOLE_CURSOR_SPEED	4
#define		CONSOLE_MAX_CMDLINE_256	MAX_CMD_256 // 256
#define		CONSOLE_NOTIFY_TIMES	4			// Baker: con_notifylines controls # of rows
#define		CONSOLE_MINIMUM_PCT_10	0.10
#define		CONSOLE_MAX_USER_PCT_90	0.90

typedef struct
{
	cbool	initialized;

	char		*text;				// Text allocation (formerly con_text)
	int			buffer_size;		// johnfitz -- user can now override default (formerly con_buffersize)
	int			buffer_rows;		// buffer (formerly con_totallines)
	int			buffer_columns;		// defaults to 78, text width of a row (formerly con_linewidth)


	float		con_times[CONSOLE_NOTIFY_TIMES];	// realtime time the line was generated
										// for transparent notify lines
// Current display
	cbool	forcedup;				// If no map loaded
	float		visible_pct;			// 0 to 1 (replaces scr_con_current)
	float		wanted_pct;				// 0 to 1 (replaces scr_conlines ... destination percent)
	int			visible_lines;			// 0 to glheight (replaces scr_con_current)
	int			visible_lines_conscale; // Same as above except in conscale metric (i.e. *  * vid.conheight / glheight)
	int			backscroll;				// Formerly (con_backscroll)
	float		user_pct;				// Formerly (scr_con_size) defaults to 50% Percent of console to display if running world (50% default, user can adjust with CTRL+UP/CTRL+DOWN

// Current positioning
	int			cursor_row;				// Formerly (con_current)
	int			cursor_column;			// Formerly (con_x)

	undo_t		undo_buffer;
} console_t;

extern console_t console1;

extern cbool key_inpartial;
extern int key_completetype;
extern char *key_partial_start;
extern char *key_partial_end;
ssize_t Con_AutoComplete (char *text, size_t s_size, ssize_t cursor, cbool force_completion, cbool *in_completion,
					   int *complete_type, const char **match_start, const char **match_end, cbool do_reverse);



void Con_DrawCharacter (int cx, int line, int num);

void Con_CheckResize (void);
void Con_Init (void);
void Con_DrawConsole (float pct, cbool drawinput);
int Con_Printf (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));
#define Con_PrintContf Con_Printf


int Con_PrintLinef (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));
int Con_PrintLine (void); // Just prints a newline


//int  Con_Warning (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2))); //johnfitz (Retired due to total extinction of use)
int Con_WarningLinef (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2))); //johnfitz


int Con_DPrintf (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));
#define Con_DPrintContf Con_DPrintf


int  Con_DPrintLinef (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));
int  Con_DPrintLineLevel5f (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2))); // Loaded dprint with 5 char prefix like "MP3_:"

int  Con_DPrintLinef_Net (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));
int  Con_DPrintLinef_Files (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));
int  Con_DPrintLinef_Model (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));
int  Con_DPrintLinef_System (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));
int  Con_DWarningLine (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2))); //johnfitz

int  Con_VerbosePrintLinef (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2))); //johnfitz




//int Con_DebugLog (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2))); // static now.
int Con_DebugLogLine (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));

int Con_SafePrintf (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));
#define Con_SafePrintContf Con_SafePrintf

int Con_SafePrintLinef (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));
void Con_SafePrintLine (void);

void Con_DrawNotify (void);
void Con_ClearNotify (void);
void Con_ToggleConsole_f (lparse_t *unused);
void Con_Exit (void); // Console closed

void Con_NotifyBox (const char *text);	// during startup for sound / cd warnings


const char *Con_Quakebar (int len);
void Con_TabComplete (cbool force_zero_length_completion, cbool reverse);
void Con_LogCenterPrint (const char *str);


#endif // ! __CONSOLE_H__

