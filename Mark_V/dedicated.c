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
// dedicated_win.c

#include "quakedef.h"

void Dedicated_Local_Print (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2))); // send text to the console

// Kill this?
#if !defined(_WIN32) || defined(CORE_SDL)
void Dedicated_Local_Print (const char *fmt, ...)
{

}
#endif

int Dedicated_Printf (const char *fmt, ...)
{
	if (isDedicated)
	{
		VA_EXPAND (text, MAXPRINTMSG_4096, fmt);

		// JPG 1.05 - translate to plain text
		if (pq_dedicated_dequake.value)
			COM_DeQuake_String (text);

		Dedicated_Local_Print ("%s", text);

		// JPG 3.00 - rcon (64 doesn't mean anything special, but we need some extra space because NET_MAXMESSAGE == RCON_BUFF_SIZE)
		if (rcon_active  && (rcon_message.cursize < rcon_message.maxsize - (int)strlen(text) - 64))
		{
			rcon_message.cursize--;
			MSG_WriteString(&rcon_message, text);
		}		
	}
	return 0; // I guess.  Success
}

int Dedicated_PrintLinef (const char *fmt, ...)
{
	VA_EXPAND_NEWLINE (text, MAXPRINTMSG_4096, fmt);
	return Dedicated_Printf ("%s", text); // newline baked into the text.

}


