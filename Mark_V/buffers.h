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
// buffers.h

#ifndef __BUFFERS_H__
#define __BUFFERS_H__

///////////////////////////////////////////////////////////////////////////////
//  Buffers: cmd.c, network and server use these functions to fill buffers
///////////////////////////////////////////////////////////////////////////////

// Baker: The sole use of allowoverflow seems to be in sv_main.c for 
// sent client messages and will drop the client if it occurs, instead
// of doing a Host_Error
typedef struct sizebuf_s
{
	cbool	allowoverflow;	// if false, do a System_Error
	cbool	overflowed;		// set to true if the buffer size failed
	byte	*data;
	int		maxsize;
	int		cursize;
} sizebuf_t;

void SZ_Alloc (sizebuf_t *buf, int startsize); // called 5 times
void SZ_Free (sizebuf_t *buf); // not called in code
void SZ_Clear (sizebuf_t *buf); // called a lot
void *SZ_GetSpace (sizebuf_t *buf, int length);
void SZ_Write (sizebuf_t *buf, const void *data, int length);


// Baker: SZ_Print is almost exclusively used by cmd.c
void SZ_Print (sizebuf_t *buf, const char *data);	// strcats onto the sizebuf

#endif // __BUFFERS_H__

