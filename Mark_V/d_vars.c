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
// r_vars.c: global refresh variables


#include	"quakedef.h" // Baker: mods = none that matter

#if	!id386

// all global and static refresh variables are collected in a contiguous block
// to avoid cache conflicts.

//-------------------------------------------------------
// global refresh variables
//-------------------------------------------------------

// FIXME: make into one big structure, like cl or sv
// FIXME: do separately for refresh engine and driver

float	d_sdivzstepu /* qasm */, d_tdivzstepu /* qasm */, d_zistepu /* qasm */;
float	d_sdivzstepv /* qasm */, d_tdivzstepv /* qasm */, d_zistepv /* qasm */;
float	d_sdivzorigin /* qasm */, d_tdivzorigin /* qasm */, d_ziorigin;

fixed16_t	sadjust /* qasm */, tadjust /* qasm */, bbextents  /* qasm */, bbextentt /* qasm */;

pixel_t			*cacheblock  /* qasm */;
int				cachewidth  /* qasm */;
pixel_t			*d_viewbuffer /* qasm */;
#ifdef WINQUAKE_DEBUG
void*			d_viewbufferstart;
void*			d_viewbufferend;
#endif // WINQUAKE_DEBUG
short			*d_pzbuffer /* qasm */;
unsigned int	d_zrowbytes /* qasm */;
unsigned int	d_zwidth /* qasm */;

#endif	// !id386

#endif // !GLQUAKE - WinQuake Software renderer