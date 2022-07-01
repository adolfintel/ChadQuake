#ifndef GLQUAKE // WinQuake Software renderer

/*
Copyright (C) 1996-1997 Id Software, Inc.
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
// d_clear: clears a specified rectangle to the specified color

#include "quakedef.h" // Baker: No modications of importance


/*
================
D_FillRect
================
*/
void D_FillRect (vrect_t *rect, int color)
{
	int				rx, ry, rwidth, rheight;
	unsigned char	*dest;
	unsigned		*ldest;

	rx = rect->x;
	ry = rect->y;
	rwidth = rect->width;
	rheight = rect->height;

	if (rx < 0)
	{
		rwidth += rx;
		rx = 0;
	}
	if (ry < 0)
	{
		rheight += ry;
		ry = 0;
	}
// Baker: These appear to clamp to canvas but since offset from
// vidbuffer, this must use screen width/height
// We could adjust the vrect
	if (rx + rwidth > clwidth)
		rwidth = clwidth - rx;
	if (ry + rheight > clheight)
		rheight = clheight - rx;

	if (rwidth < 1 || rheight < 1)
		return;

	dest = ((byte *)vid.buffer + ry*vid.rowbytes + rx);

	if (((rwidth & 0x03) == 0) && (((long)dest & 0x03) == 0))
	{
	// faster aligned dword clear
		ldest = (unsigned *)dest;
		color += color << 16;

		rwidth >>= 2;
		color += color << 8;

		for (ry=0 ; ry<rheight ; ry++)
		{
			for (rx=0 ; rx<rwidth ; rx++)
				ldest[rx] = color;
			ldest = (unsigned *)((byte*)ldest + vid.rowbytes);
		}
	}
	else
	{
	// slower byte-by-byte clear for unaligned cases
		for (ry=0 ; ry<rheight ; ry++)
		{
			for (rx=0 ; rx<rwidth ; rx++)
				dest[rx] = color;
			dest += vid.rowbytes;
		}
	}
}

#endif // !GLQUAKE - WinQuake Software renderer