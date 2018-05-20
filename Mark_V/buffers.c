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
// buffers.c - buffers

#include "quakedef.h"

///////////////////////////////////////////////////////////////////////////////
//  Buffers: cmd.c, network and server use these functions to fill buffers
///////////////////////////////////////////////////////////////////////////////

void SZ_Alloc (sizebuf_t *buf, int startsize)
{
	if (startsize < 256)
		startsize = 256;
	buf->data = (byte *) Hunk_AllocName (startsize, "sizebuf");
	buf->maxsize = startsize;
	buf->cursize = 0;
}


void SZ_Free (sizebuf_t *buf)
{
//      Z_Free (buf->data);
//      buf->data = NULL;
//      buf->maxsize = 0;
	buf->cursize = 0;
}

void SZ_Clear (sizebuf_t *buf)
{
	buf->cursize = 0;
}


/*
typedef struct sizebuf_s
{
	cbool	allowoverflow;	// if false, do a System_Error
	cbool	overflowed;		// set to true if the buffer size failed
	byte	*data;
	int		maxsize;
	int		cursize;
} sizebuf_t;
*/

void *SZ_GetSpace (sizebuf_t *buf, int length)
{
	void    *data;

	if (buf->cursize + length > buf->maxsize)
	{
		if (!buf->allowoverflow)
			System_Error ("SZ_GetSpace: overflow without allowoverflow set");

		if (length > buf->maxsize)
			System_Error ("SZ_GetSpace: %d is > full buffer size", length);

		buf->overflowed = true;
		Con_PrintLinef ("SZ_GetSpace: overflow");
		SZ_Clear (buf);
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;

	return data;
}

void SZ_Write (sizebuf_t *buf, const void *data, int length)
{
	memcpy (SZ_GetSpace(buf,length), data, length);
}

void SZ_Print (sizebuf_t *buf, const char *data)
{
	int len = strlen(data) + 1;

	if (buf->data[buf->cursize-1])
	{	/* no trailing 0 */
		memcpy ((byte *)SZ_GetSpace(buf, len),data,len);
	}
	else
	{	/* write over trailing 0 */
		memcpy ((byte *)SZ_GetSpace(buf, len-1)-1,data,len);
	}
}
