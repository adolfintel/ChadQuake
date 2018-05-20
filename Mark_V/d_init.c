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
// d_init.c: rasterization driver initialization

#include "quakedef.h" // Baker: Modifications = none of any significance


#define NUM_MIPS	4


surfcache_t		*d_initial_rover;
cbool		d_roverwrapped;
int				d_minmip;
float			d_scalemip[NUM_MIPS-1];

static float	basemip[NUM_MIPS-1] = {1.0, 0.5*0.8, 0.25*0.8};

extern int			d_aflatcolor;

void (*d_drawspans) (espan_t *pspan);


/*
===============
D_Init
===============
*/
void D_Init (void)
{
	Cmd_AddCommands (D_Init);

	r_aliasuvscale = 1.0;
}



/*
===============
D_SetupFrame
===============
*/
void D_SetupFrame (void)
{
	int		i;

	if (r_dowarp)
	{
		d_viewbuffer = r_warpbuffer;
#ifdef WINQUAKE_DEBUG
		d_viewbufferstart = d_viewbuffer;
		d_viewbufferend = d_viewbufferstart + WARP_HEIGHT * WARP_WIDTH; // Baker * sizeof pixel t which is 1
#endif // WINQUAKE_DEBUG
	}
	else
	{
		d_viewbuffer = (void *)(byte *)vid.buffer;
#ifdef WINQUAKE_DEBUG
		d_viewbufferstart = d_viewbuffer; // Baker what about negative rowbytes?
		d_viewbufferend = d_viewbufferstart + vid.surfcachesize;
#endif // WINQUAKE_DEBUG
	}

	if (r_dowarp)
		screenwidth = WARP_WIDTH;
	else
		screenwidth = vid.rowbytes;

	d_roverwrapped = false;
	d_initial_rover = sc_rover;

	d_minmip = sw_d_mipcap.value;
	if (d_minmip > 3)
		d_minmip = 3;
	else if (d_minmip < 0)
		d_minmip = 0;

	for (i = 0 ; i < (NUM_MIPS - 1) ; i++)
		d_scalemip[i] = basemip[i] * sw_d_mipscale.value;

#if	id386
	if (sw_d_subdiv16.value) // Baker: This defaults to 1
		d_drawspans = D_DrawSpans16;
	else
		d_drawspans = D_DrawSpans8;
#else
		d_drawspans = D_DrawSpans8;
#endif

	d_aflatcolor = 0;
}

#if 0
/*
===============
D_UpdateRects
===============
*/
void D_UpdateRects (vrect_t *prect)
{

// the software driver draws these directly to the vid buffer

	UNUSED(prect);
}
#endif // 0

#endif // !GLQUAKE - WinQuake Software renderer