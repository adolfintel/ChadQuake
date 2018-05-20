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
// d_modech.c: called when mode has just changed

#include "quakedef.h" // Baker: Modifications = avoid giant particles in larger resolutions


int	d_vrectx /* qasm */, d_vrecty /* qasm */, d_vrectright_particle /* qasm */, d_vrectbottom_particle /* qasm */;

int	d_y_aspect_shift /* qasm */, d_pix_min /* qasm */, d_pix_max /* qasm */, d_pix_shift /* qasm */;

int		d_scantable[MAXHEIGHT] /* qasm */;
short	*zspantable[MAXHEIGHT] /* qasm */;

/*
================
D_Patch
================
*/
void D_Patch (void)
{
// Baker: SW-ASM Killed Feb 7 2016 
#if 0
#if id386

	static cbool protectset8 = false;

	if (!protectset8)
	{
		System_MakeCodeWriteable ((int)D_PolysetAff8Start,
						     (int)D_PolysetAff8End - (int)D_PolysetAff8Start);
		protectset8 = true;
	}

#endif	// id386
#endif // END SW-ASM Killed
}


/*
================
D_ViewChanged
================
*/
void D_ViewChanged (void)
{
	int rowbytes;

	if (r_dowarp)
		rowbytes = WARP_WIDTH;
	else
		rowbytes = vid.rowbytes;

	scale_for_mip = xscale;
	if (yscale > xscale)
		scale_for_mip = yscale;

	d_zrowbytes = clwidth * 2;
	d_zwidth = clwidth;

#if 1 // Baker: Particles starting to get gigantic at 1360 x 768
	if (r_refdef.vrect.width > 1024)
	{
		d_pix_min = r_refdef.vrect.width / 480;
		if (d_pix_min < 1)
			d_pix_min = 1;

		d_pix_max = (int)((float)r_refdef.vrect.width / (480.0 / 4.0) + 0.5);
		d_pix_shift = 8 - (int)((float)r_refdef.vrect.width / 480.0 + 0.5);
	}
	else
#endif
	{ // Baker: Original
		d_pix_min = r_refdef.vrect.width / 320;
		if (d_pix_min < 1)
			d_pix_min = 1;

		d_pix_max = (int)((float)r_refdef.vrect.width / (320.0 / 4.0) + 0.5);
		d_pix_shift = 8 - (int)((float)r_refdef.vrect.width / 320.0 + 0.5);
	}
	if (d_pix_max < 1)
		d_pix_max = 1;

	if (pixelAspect > 1.4)
		d_y_aspect_shift = 1;
	else
		d_y_aspect_shift = 0;

	d_vrectx = r_refdef.vrect.x;
	d_vrecty = r_refdef.vrect.y;
	d_vrectright_particle = r_refdef.vrectright - d_pix_max;
	d_vrectbottom_particle =
			r_refdef.vrectbottom - (d_pix_max << d_y_aspect_shift);

	{
		int		i;

		for (i = 0 ; i < clheight; i++)
		{
			d_scantable[i] = i * rowbytes;
			zspantable[i] = d_pzbuffer + i * d_zwidth;
		}
	}

	D_Patch ();
}

#endif // !GLQUAKE - WinQuake Software renderer