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
// r_misc.c

#include "quakedef.h" // Baker: mods = nothing

/*
===============
R_CheckVariables
===============
*/
void R_CheckVariables (void)
{
	static float	oldbright;

	if (r_fullbright.value != oldbright)
	{
		oldbright = r_fullbright.value;
		D_FlushCaches ();	// so all lighting changes
	}
}


/*
================
R_LineGraph

Only called by R_DisplayTime
================
*/
void R_LineGraph (int x, int y, int h)
{
	int		i;
	byte	*dest;
	int		s;

// FIXME: should be disabled on no-buffer adapters, or should be in the driver

	x += r_refdef.vrect.x;
	y += r_refdef.vrect.y;

	dest = vid.buffer + vid.rowbytes*y + x;

	s = sw_r_graphheight.value;

	if (h>s)
		h = s;

	for (i=0 ; i<h ; i++, dest -= vid.rowbytes*2)
	{
		dest[0] = 0xff;
		*(dest-vid.rowbytes) = 0x30;
	}
	for ( ; i<s ; i++, dest -= vid.rowbytes*2)
	{
		dest[0] = 0x30;
		*(dest-vid.rowbytes) = 0x30;
	}
}

/*
==============
R_TimeGraph

Performance monitoring tool
==============
*/
#define	MAX_TIMINGS		100
extern float mouse_x, mouse_y;
void R_TimeGraph (void)
{
	static	int		timex;
	int		a;
	float	r_time2;
	static byte	r_timings[MAX_TIMINGS];
	int		x;

	r_time2 = System_DoubleTime ();

	a = (r_time2-r_time1)/0.01;
//a = fabs(mouse_y * 0.05);
//a = (int)((r_refdef.vieworg[2] + 1024)/1)%(int)r_graphheight.value;
//a = fabs(velocity[0])/20;
//a = ((int)fabs(origin[0])/8)%20;
//a = (cl.idealpitch + 30)/5;
	r_timings[timex] = a;
	a = timex;

	if (r_refdef.vrect.width <= MAX_TIMINGS)
		x = r_refdef.vrect.width-1;
	else
		x = r_refdef.vrect.width -
			(r_refdef.vrect.width - MAX_TIMINGS)/2;
	do
	{
		R_LineGraph (x, r_refdef.vrect.height-2, r_timings[a]);
		if (x==0)
			break;		// screen too small to hold entire thing
		x--;
		a--;
		if (a == -1)
			a = MAX_TIMINGS-1;
	} while (a != timex);

	timex = (timex+1)%MAX_TIMINGS;
}


/////////////////////////////////////////////////
/////////////////////////////////////////////////
/////////////////////////////////////////////////
/////////////////////////////////////////////////
/////////////////////////////////////////////////
/////////////////////////////////////////////////
/////////////////////////////////////////////////

/*
==================
R_InitTextures
==================
*/
void R_InitTextures (void)
{
	int		x,y, m;
	byte	*dest;

// create a simple checkerboard texture for the default
	r_notexture_mip = Hunk_AllocName (sizeof(texture_t) + 16*16+8*8+4*4+2*2, "notexture");

	r_notexture_mip->width = r_notexture_mip->height = 16;
	r_notexture_mip->offsets[0] = sizeof(texture_t);
	r_notexture_mip->offsets[1] = r_notexture_mip->offsets[0] + 16*16;
	r_notexture_mip->offsets[2] = r_notexture_mip->offsets[1] + 8*8;
	r_notexture_mip->offsets[3] = r_notexture_mip->offsets[2] + 4*4;

	for (m=0 ; m<4 ; m++)
	{
		dest = (byte *)r_notexture_mip + r_notexture_mip->offsets[m];
		for (y=0 ; y< (16>>m) ; y++)
		{
			for (x=0 ; x< (16>>m) ; x++)
			{
				if (  (y< (8>>m) ) ^ (x< (8>>m) ) )
					*dest++ = 0;
				else
					*dest++ = 0xff;
			}
		}
	}
}



/*
===============
R_Init
===============
*/

void R_Init_Local (void)
{
	int		dummy;

// get stack position so we can guess if we are going to overflow
	r_stack_start = (byte *)&dummy;

	R_InitTurb ();

	Cmd_AddCommands (R_Init_Local);

	Cvar_SetValueQuick (&sw_r_maxedges, 100000); //NUMSTACKEDGES    /// ODDBALL
	Cvar_SetValueQuick (&sw_r_maxsurfs, 100000); //NUMSTACKSURFACES  /// ODDBALL

	view_clipplanes[0].leftedge = true;
	view_clipplanes[1].rightedge = true;
	view_clipplanes[1].leftedge = view_clipplanes[2].leftedge =
		view_clipplanes[3].leftedge = false;
	view_clipplanes[0].rightedge = view_clipplanes[2].rightedge =
		view_clipplanes[3].rightedge = false;

	r_refdef.xOrigin = XCENTERING;
	r_refdef.yOrigin = YCENTERING;

	R_InitTextures ();


// TODO: collect 386-specific code in one place
#if	id386
	System_MakeCodeWriteable ((long)R_EdgeCodeStart, (long)R_EdgeCodeEnd - (long)R_EdgeCodeStart);
#endif	// id386

	D_Init ();
}

/*
===============
R_NewGame -- johnfitz -- handle a game switch
===============
*/
void R_NewGame (void)
{
	// Nothing at the moment

}


/*
===============
R_NewMap_Local
===============
*/

void R_NewMap_Local (void)
{

	R_BuildLightmaps(); // Calls Stains_WipeStains_NewMap

#ifdef WINQUAKE_SOFTWARE_SKYBOX
	R_InitSkyBox (); // Manoel Kasimier - skyboxes // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
#endif // WINQUAKE_SOFTWARE_SKYBOX

	R_FinalizeAliasVerts ();

	r_cnumsurfs = sw_r_maxsurfs.value;

	if (r_cnumsurfs <= MINSURFACES)
		r_cnumsurfs = MINSURFACES;

	if (r_cnumsurfs > NUMSTACKSURFACES)
	{
		surfaces = Hunk_AllocName (r_cnumsurfs * sizeof(surf_t), "surfaces");
		surface_p = surfaces;
		surf_max = &surfaces[r_cnumsurfs];
		r_surfsonstack = false;
	// surface 0 doesn't really exist; it's just a dummy because index 0
	// is used to indicate no edge attached to surface
		surfaces--;
#if id386
		R_SurfacePatch ();
#endif // id386
	}
	else
	{
		r_surfsonstack = true;
	}

	r_maxedgesseen = 0;
	r_maxsurfsseen = 0;

	r_numallocatededges = sw_r_maxedges.value;

	if (r_numallocatededges < MINEDGES)
		r_numallocatededges = MINEDGES;

	if (r_numallocatededges <= NUMSTACKEDGES)
	{
		auxedges = NULL;
	}
	else
	{
		auxedges = Hunk_AllocName (r_numallocatededges * sizeof(edge_t),
				"edges");
	}

	r_dowarpold = false;
	r_viewchanged = false;

#ifdef PASSAGES
	CreatePassages ();
#endif

	r_framecount = cl.r_framecount = 1; //johnfitz -- paranoid?



}


/*
=============
R_PrintTimes
=============
*/
void R_PrintTimes (void)
{
	double	r_time2;
	double	ms;

	r_time2 = System_DoubleTime ();

	ms = 1000* (r_time2 - r_time1);

	Con_PrintLinef ("%5.1f ms %3d/%3d/%3d poly %3d surf",
		ms, c_faceclip, r_polycount, r_drawnpolycount, c_surf);
	c_surf = 0;
}


/*
=============
R_PrintDSpeeds
=============
*/
void R_PrintDSpeeds (void)
{
	double	ms, dp_time, r_time2, rw_time, db_time, se_time, de_time, dv_time;

	r_time2 = System_DoubleTime ();

	dp_time = (dp_time2 - dp_time1) * 1000;
	rw_time = (rw_time2 - rw_time1) * 1000;
	db_time = (db_time2 - db_time1) * 1000;
	se_time = (se_time2 - se_time1) * 1000;
	de_time = (de_time2 - de_time1) * 1000;
	dv_time = (dv_time2 - dv_time1) * 1000;
	ms = (r_time2 - r_time1) * 1000;

	Con_PrintLinef ("%3d %4.1fp %3dw %4.1fb %3ds %4.1fe %4.1fv",
				(int)ms, dp_time, (int)rw_time, db_time, (int)se_time, de_time,
				 dv_time);
}


/*
=============
R_PrintAliasStats
=============
*/
void R_PrintAliasStats (void)
{
	Con_PrintLinef ("%3d polygon model drawn", r_amodels_drawn);
}


/*
===================
R_TransformFrustum
===================
*/
void R_TransformFrustum (void)
{
	int		i;
	vec3_t	v, v2;

	for (i=0 ; i<4 ; i++)
	{
		v[0] = screenedge[i].normal[2];
		v[1] = -screenedge[i].normal[0];
		v[2] = screenedge[i].normal[1];

		v2[0] = v[1]*vright[0] + v[2]*vup[0] + v[0]*vpn[0];
		v2[1] = v[1]*vright[1] + v[2]*vup[1] + v[0]*vpn[1];
		v2[2] = v[1]*vright[2] + v[2]*vup[2] + v[0]*vpn[2];

		VectorCopy (v2, view_clipplanes[i].normal);

		view_clipplanes[i].dist = DotProduct (modelorg, v2);
	}
}


#if	!id386

/*
================
TransformVector
================
*/
void TransformVector (vec3_t in, vec3_t out)
{
	out[0] = DotProduct(in,vright);
	out[1] = DotProduct(in,vup);
	out[2] = DotProduct(in,vpn);
}

#endif


/*
================
R_TransformPlane
================
*/
// Nobody calls this?
#if 0
void R_TransformPlane (mplane_t *p, float *normal, float *dist)
{
	float	d;

	d = DotProduct (r_origin, p->normal);
	*dist = p->dist - d;
// TODO: when we have rotating entities, this will need to use the view matrix
	TransformVector (p->normal, normal);
}
#endif


/*
===============
R_SetUpFrustumIndexes
===============
*/
static void R_SetUpFrustumIndexes (void)
{
	int		i, j, *pindex;

	pindex = r_frustum_indexes;

	for (i=0 ; i<4 ; i++)
	{
		for (j=0 ; j<3 ; j++)
		{
			if (view_clipplanes[i].normal[j] < 0)
			{
				pindex[j] = j;
				pindex[j+3] = j+3;
			}
			else
			{
				pindex[j] = j+3;
				pindex[j+3] = j;
			}
		}

	// FIXME: do just once at start
		pfrustum_indexes[i] = pindex;
		pindex += 6;
	}
}


/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
	int				edgecount;
	vrect_t			vrect;
	float			w, h;

// don't allow cheats in multiplayer
	if (cl.maxclients > 1)
	{
		Cvar_SetValueQuick (&sw_r_draworder, 0);
		Cvar_SetValueQuick (&r_fullbright, 0);
		Cvar_SetValueQuick (&sw_r_ambient, 0);
		Cvar_SetValueQuick (&r_drawflat, 0);
	}

	Stain_FrameSetup_LessenStains (false);

	if (sw_r_numsurfs.value)
	{
		if ((surface_p - surfaces) > r_maxsurfsseen)
			r_maxsurfsseen = (int) (surface_p - surfaces);

		Con_PrintLinef ("Used %d of %d surfs; %d max", (int)(surface_p - surfaces),
				(int)(surf_max - surfaces), r_maxsurfsseen);
	}

	if (sw_r_numedges.value)
	{
		edgecount = (int) (edge_p - r_edges);

		if (edgecount > r_maxedgesseen)
			r_maxedgesseen = edgecount;

		Con_PrintLinef ("Used %d of %d edges; %d max", edgecount,
			r_numallocatededges, r_maxedgesseen);
	}

	r_refdef.ambientlight = sw_r_ambient.value;

	if (r_refdef.ambientlight < 0)
		r_refdef.ambientlight = 0;

	if (!sv.active)
		sw_r_draworder.value = 0;	// don't let cheaters look behind walls

	R_CheckVariables ();

	R_AnimateLight ();

	r_framecount = cl.r_framecount ++;

	numbtofpolys = 0;

// build the transformation matrix for the given view angles
	VectorCopy (r_refdef.vieworg, modelorg);
	VectorCopy (r_refdef.vieworg, r_origin);

	AngleVectors (r_refdef.viewangles, vpn, vright, vup); // Baker

#if 0 // Baker: We aren't using this right now
	R_SetFrustum (r_refdef.fov_x, r_refdef.fov_y);
#endif

// current viewleaf
	cl.r_oldviewleaf = cl.r_viewleaf;
	cl.r_viewleaf = Mod_PointInLeaf (r_origin, cl.worldmodel);

	r_dowarpold = r_dowarp;
	r_dowarp = (r_waterwarp.value > 0) && (cl.r_viewleaf->contents <= CONTENTS_WATER);

	if ((r_dowarp != r_dowarpold) || r_viewchanged /* || lcd_x.value*/)
	{
		if (r_dowarp)
		{
			if ((clwidth <= WARP_WIDTH) &&
				(clheight <= WARP_HEIGHT))
			{
				vrect.x = 0;
				vrect.y = 0;
				vrect.width = clwidth;
				vrect.height = clheight;

				R_ViewChanged (&vrect, sb_lines, vid.aspect);
			}
			else
			{
				w = clwidth;
				h = clheight;

				if (w > WARP_WIDTH)
				{
					h *= (float)WARP_WIDTH / w;
					w = WARP_WIDTH;
				}

				if (h > WARP_HEIGHT)
				{
					h = WARP_HEIGHT;
					w *= (float)WARP_HEIGHT / h;
				}

				vrect.x = 0;
				vrect.y = 0;
				vrect.width = (int)w;
				vrect.height = (int)h;

				R_ViewChanged (&vrect,
					(int)((float)sb_lines * (h/(float)clwidth)),
							   vid.aspect * (h / w) *
						 ((float)clwidth / (float)clheight));
			}
		}
		else
		{
			vrect.x = 0;
			vrect.y = 0;
			vrect.width = clwidth;
			vrect.height = clheight;

			R_ViewChanged (&vrect, sb_lines, vid.aspect);
		}

		r_viewchanged = false;
	}

// start off with just the four screen edge clip planes
	R_TransformFrustum ();

// save base values
	VectorCopy (vpn, base_vpn);
	VectorCopy (vright, base_vright);
	VectorCopy (vup, base_vup);
	VectorCopy (modelorg, base_modelorg);

	R_SetSkyFrame ();

	R_SetUpFrustumIndexes ();

	r_cache_thrash = false;

// clear frame counts
	c_faceclip = 0;
	r_polycount = 0;
	r_drawnpolycount = 0;

	r_amodels_drawn = 0;
	r_outofsurfaces = 0;
	r_outofedges = 0;

	D_SetupFrame ();
}


#ifdef WINQUAKE_QBISM_ALPHAMAP
/*
===============
BestColor - qb: from qlumpy
===============
*/
// r_misc.c?
static byte BestPalColor (int r, int g, int b, int start, int stop)
{
    int i;
    int dr, dg, db;
    int bestdistortion, distortion;
    int bestcolor_pal_idx;
    byte *pal;

    r = CLAMP (0, r, 254);
    g = CLAMP (0, g, 254);
    b = CLAMP (0, b, 254);
//
// let any color go to 0 as a last resort
//

    bestdistortion =  ( (int)r*r + (int)g*g + (int)b*b )*2; //qb: option- ( (int)r + (int)g + (int)b )*2;
    bestcolor_pal_idx = 0;

    pal = vid.basepal + start * RGB_3;
    for (i = start ; i <= stop ; i++) {
        dr = abs(r - (int)pal[0]);
        dg = abs(g - (int)pal[1]);
        db = abs(b - (int)pal[2]);
        pal += 3;
        distortion = dr*dr + dg*dg + db*db; //qb: option, more weight on value- dr + dg + db;
        if (distortion < bestdistortion)
        {
            if (!distortion)
                return i;               // perfect match

            bestdistortion = distortion;
            bestcolor_pal_idx = i;
        }
    }

	if (!in_range(0, bestcolor_pal_idx, 255))
		System_Error ("BestPalColor: %d outsize 0-255 range", bestcolor_pal_idx);
    return bestcolor_pal_idx;
}

// Move to some sort of winquake file but what one?  r_misc.c?
void R_WinQuake_Generate_Alpha50_Map (byte my_alpha50map[]) //qb: 50% / 50% alpha
{
    int color_a, color_b, r, g, b;
    byte *colmap = my_alpha50map;

    for (color_a = 0; color_a < PALETTE_COLORS_256; color_a ++) {
        for (color_b = 0 ; color_b < PALETTE_COLORS_256 ; color_b ++) {
            if (color_a == 255 || color_b == 255)
                *colmap ++ = 255;
            else {
                r = (int)(((float)vid.basepal[color_a * 3 + 0] * 0.5)  + ((float)vid.basepal[color_b * 3 + 0] * 0.5));
                g = (int)(((float)vid.basepal[color_a * 3 + 1] * 0.5)  + ((float)vid.basepal[color_b * 3 + 1] * 0.5));
                b = (int)(((float)vid.basepal[color_a * 3 + 2] * 0.5)  + ((float)vid.basepal[color_b * 3 + 2] * 0.5));
                *colmap ++ = BestPalColor(r, g, b, 0, 254); // High quality color tables get best color
            }
        }
    }
}
#endif // WINQUAKE_QBISM_ALPHAMAP

cbool VID_WinQuake_AllocBuffers_D_InitCaches (int width, int height)
{
	int		tsize, tbuffersize;

	tbuffersize = width * height * sizeof (*d_pzbuffer);
	tsize = D_SurfaceCacheForRes (width, height);
	tbuffersize += tsize;

	vid.surfcachesize = tsize;

	if (d_pzbuffer)
	{
		D_FlushCaches ();

		free (d_pzbuffer); // Formerly Hunk_FreeToHighMark (vid.highhunkmark)  but malloc is easier to debug.
		d_pzbuffer = NULL;
	}

	d_pzbuffer = malloc (tbuffersize); // Formerly vid.highhunkmark = Hunk_HighMark (); d_pzbuffer = Hunk_HighAllocName (tbuffersize, "video");
	vid.surfcache = (byte *) d_pzbuffer + width * height * sizeof (*d_pzbuffer);

	D_InitCaches (vid.surfcache, vid.surfcachesize);
	return true;
}

#endif // !GLQUAKE - WinQuake Software renderer