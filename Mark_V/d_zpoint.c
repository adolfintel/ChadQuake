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
// d_zpoint.c: software driver module for drawing z-buffered points

#include "quakedef.h" // Baker: mods = none



/*
=====================
D_DrawZPoint
=====================
*/
void D_DrawZPoint (void)
{
	byte	*pdest;
	short	*pz;
	int		izi;
	
	pz = d_pzbuffer + (d_zwidth * r_zpointdesc.v) + r_zpointdesc.u;
	pdest = d_viewbuffer + d_scantable[r_zpointdesc.v] + r_zpointdesc.u;
	izi = (int)(r_zpointdesc.zi * 0x8000);

	if (*pz <= izi)
	{
		*pz = izi;
		*pdest = r_zpointdesc.color;
	}
}

#endif // !GLQUAKE - WinQuake Software renderer