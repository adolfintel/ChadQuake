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
// r_surf.c: surface-related refresh code

#include "quakedef.h" // Baker: mods = stains, moved dynamic lights working on ents.  That's it.






drawsurf_t	r_drawsurf;

int				lightleft /* qasm */, sourcesstep /* qasm */, blocksize  /* qasm */, sourcetstep /* qasm */;
int				lightdelta /* qasm */, lightdeltastep /* qasm */;
int				lightright /* qasm */, lightleftstep /* qasm */, lightrightstep /* qasm */, blockdivshift  /* qasm */;
unsigned		blockdivmask  /* qasm */;
void			*prowdestbase /* qasm */;
unsigned char	*pbasesource /* qasm */;
int				surfrowbytes /* qasm */;	// used by ASM files
unsigned		*r_lightptr /* qasm */;
int				r_stepback /* qasm */;
int				r_lightwidth /* qasm */;
int				r_numhblocks, r_numvblocks /* qasm */;
unsigned char	*r_source, *r_sourcemax /* qasm */;

void R_DrawSurfaceBlock8_mip0 (void);
void R_DrawSurfaceBlock8_mip1 (void);
void R_DrawSurfaceBlock8_mip2 (void);
void R_DrawSurfaceBlock8_mip3 (void);

static void	(*surfmiptable[4])(void) = {
	R_DrawSurfaceBlock8_mip0,
	R_DrawSurfaceBlock8_mip1,
	R_DrawSurfaceBlock8_mip2,
	R_DrawSurfaceBlock8_mip3
};

unsigned blocklights[18*18];


typedef unsigned char stmap;
stmap stainmaps[MAX_FITZQUAKE_LIGHTMAPS*LIGHTMAPS_BLOCK_WIDTH*LIGHTMAPS_BLOCK_HEIGHT];	//added to lightmap for added (hopefully) speed.
int			allocated[MAX_FITZQUAKE_LIGHTMAPS][LIGHTMAPS_BLOCK_WIDTH];

//radius, x y z, a
void R_StainSurf (msurface_t *surf, float *parms)
{
	int			sd, td;
	float		dist, rad, minlight;
	vec3_t		impact, local;
	int			s, t;
	int			i;
#if 0
	int			smax, tmax;
#endif
	float amm;
	int lim;
	mtexinfo_t	*tex;

	stmap *stainbase;

	lim = 255 - (r_stains.value*255);

#if 0
	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
#endif
	tex = surf->texinfo;

	stainbase = stainmaps + surf->lightmaptexturenum*LIGHTMAPS_BLOCK_WIDTH*LIGHTMAPS_BLOCK_HEIGHT;
	stainbase += (surf->light_t * LIGHTMAPS_BLOCK_WIDTH + surf->light_s);

	rad = *parms;
	dist = DotProduct ((parms+1), surf->plane->normal) - surf->plane->dist;
	rad -= fabs(dist);
	minlight = 0;
	if (rad < minlight)	//not hit
		return;
	minlight = rad - minlight;

	for (i=0 ; i<3 ; i++)
	{
		impact[i] = (parms+1)[i] - surf->plane->normal[i]*dist;
	}

	local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3];
	local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3];

	local[0] -= surf->texturemins[0];
	local[1] -= surf->texturemins[1];

	for (t = 0 ; t < surf->tmax ; t++)
	{
		td = local[1] - t*16;
		if (td < 0)
			td = -td;
		for (s=0 ; s < surf->smax ; s++)
		{
			sd = local[0] - s*16;
			if (sd < 0)
				sd = -sd;
			if (sd > td)
				dist = sd + (td>>1);
			else
				dist = td + (sd>>1);
			if (dist < minlight)
			{
				amm = stainbase[(s)] - (dist - rad)*parms[4];
				stainbase[(s)] = CLAMP(lim, amm, 255);

				surf->stained = true;
			}
		}
		stainbase += LIGHTMAPS_BLOCK_WIDTH;
	}

	if (surf->stained)
	{
		for (i = 0; i < 4; i++)
			if (surf->cachespots[i])
				surf->cachespots[i]->dlight = -1;
	}
}

//combination of R_AddDynamicLights and R_MarkLights
void R_Q1BSP_StainNode (mnode_t *node, float *parms)
{
	mplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	unsigned int i;

	if (node->contents < 0)
		return;

	splitplane = node->plane;
	dist = DotProduct ((parms+1), splitplane->normal) - splitplane->dist;

	if (dist > (*parms))
	{
		R_Q1BSP_StainNode (node->children[0], parms);
		return;
	}
	if (dist < (-*parms))
	{
		R_Q1BSP_StainNode (node->children[1], parms);
		return;
	}

// mark the polygons
	surf = cl.worldmodel->surfaces + node->firstsurface;
	for (i = 0 ; i < node->numsurfaces ; i++, surf++)
	{
		if (surf->flags & SURF_DRAWTILED)
			continue;
		R_StainSurf(surf, parms);
	}

	R_Q1BSP_StainNode (node->children[0], parms);
	R_Q1BSP_StainNode (node->children[1], parms);
}

void Stain_AddStain(const vec3_t org, float tint, float radius)
{
	entity_t *pe;
	int i;
	float parms[5];

	if (r_stains.value <= 0 || !cl.worldmodel)
		return;
	parms[0] = radius;
	parms[1] = org[0];
	parms[2] = org[1];
	parms[3] = org[2];
	parms[4] = -tint * .50;

	R_Q1BSP_StainNode(cl.worldmodel->nodes+cl.worldmodel->hulls[0].firstclipnode, parms);

	//now stain bsp models other than world.
	for (i=1 ; i< cl.num_entities ; i++)	//0 is world...
	{
		pe = &cl_entities[i];
		if (pe->model && pe->model->surfaces == cl.worldmodel->surfaces)
		{
			parms[1] = org[0] - pe->origin[0];
			parms[2] = org[1] - pe->origin[1];
			parms[3] = org[2] - pe->origin[2];
			R_Q1BSP_StainNode(pe->model->nodes+pe->model->hulls[0].firstclipnode, parms);
		}
	}
}

void Stains_WipeStains_NewMap(void)
{
	memset(stainmaps, 255, sizeof(stainmaps));
}

void Stain_Change_f (cvar_t* var)
{
	Stain_FrameSetup_LessenStains (true /* erase! */);
}

void Stain_FrameSetup_LessenStains (cbool erase_stains)
{
	int i, j;
	msurface_t	*surf;
#if 0
	int			smax, tmax;
#endif
	int			s, t;
	stmap *stain;
	int stride;
	int amount, limit;

	static float time;

	if (erase_stains)
	{
		if (!cl.worldmodel)
			return;

			goto erase_go;
	}


	if (!r_stains.value)
	{
		return;
	}

	time += host_frametime_;

	if (time < r_stains_fadetime.value)
		return;

	time-=r_stains_fadetime.value;

erase_go:
	amount = r_stains_fadeamount.value;

	if (erase_stains)
		amount = 255;

	limit = 255 - amount;

	surf = cl.worldmodel->surfaces;

	for (i=0 ; i<cl.worldmodel->numsurfaces ; i++, surf++)
	{
		if (surf->stained)
		{
			for (j = 0; j < 4; j++)
				if (surf->cachespots[j])
					surf->cachespots[j]->dlight = -1;

#if 0
			smax = (surf->extents[0]>>4)+1;
			tmax = (surf->extents[1]>>4)+1;
#endif

			stain = stainmaps + surf->lightmaptexturenum*LIGHTMAPS_BLOCK_WIDTH*LIGHTMAPS_BLOCK_HEIGHT;
			stain += (surf->light_t * LIGHTMAPS_BLOCK_WIDTH + surf->light_s);

			stride = (LIGHTMAPS_BLOCK_WIDTH - surf->smax);

			surf->stained = false;

			for (t = 0 ; t < surf->tmax ; t++, stain+=stride)
			{
				for (s=0 ; s < surf->smax ; s++)
				{
					if (*stain < limit)
					{
						*stain += amount;
						surf->stained=true;
					}
					else	//reset to 255.
						*stain = 255;

					stain++;
				}
			}
		}
	}

}



// returns a texture number and the position inside it
int SWAllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		texnum;

	if (!w || !h)
		System_Error ("AllocBlock: bad size");

	for (texnum=0 ; texnum<MAX_FITZQUAKE_LIGHTMAPS ; texnum++)
	{
		best = LIGHTMAPS_BLOCK_HEIGHT;

		for (i=0 ; i<LIGHTMAPS_BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (allocated[texnum][i+j] >= best)
					break;
				if (allocated[texnum][i+j] > best2)
					best2 = allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > LIGHTMAPS_BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	System_Error ("AllocBlock: full");
	return 0;
}

void R_CreateSurfaceLightmap (msurface_t *surf)
{
	// store these out so that we don't have to recalculate them every time
#if 0 // Baker: I do this in model loading now
	surf->smax = (surf->extents[0] >> 4) + 1;
	surf->tmax = (surf->extents[1] >> 4) + 1;
#endif

	if (surf->flags & (SURF_DRAWSKY|SURF_DRAWTURB))
		return;
	if (surf->texinfo->flags & (TEX_SPECIAL))
		return;


	//smax = (surf->extents[0]>>4)+1;
	//tmax = (surf->extents[1]>>4)+1;

	surf->lightmaptexturenum = SWAllocBlock (surf->smax, surf->tmax, &surf->light_s, &surf->light_t);
}

void R_BuildLightmaps(void)
{
	int i;
	msurface_t	*surf;

	memset(allocated, 0, sizeof(allocated));

	Stains_WipeStains_NewMap();

	surf = cl.worldmodel->surfaces;
	for (i = 0 ; i < cl.worldmodel->numsurfaces ; i++, surf++)
	{
//		if ( cl.worldmodel->surfaces[i].flags & SURF_DRAWSKY )
//			P_EmitSkyEffectTris(cl.worldmodel, &cl.worldmodel->surfaces[i]);
		R_CreateSurfaceLightmap(surf);
	}
}
//qbism ftestain end


/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights (void)
{
	msurface_t *surf;
	int			lnum;
	int			sd, td;
	float		dist, rad, minlight;
	vec3_t		impact, local;
	int			s, t;
	int			i;
#if 0 // Baker: Use smax, tmax
	int			smax, tmax;
#endif
	mtexinfo_t	*tex;

	surf = r_drawsurf.surf;

#if 0 // Baker: Use smax, tmax
	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
#endif
	tex = surf->texinfo;

	for (lnum=0 ; lnum < MAX_FITZQUAKE_DLIGHTS ; lnum++)
	{
		if (!(surf->dlightbits[lnum >> 5] & (1 << (lnum & 31))))
			continue;		// not lit by this light

		rad = cl.dlights[lnum].radius;
		dist = DotProduct (cl.dlights[lnum].transformed /*origin*/, surf->plane->normal) -
				surf->plane->dist;
		rad -= fabs(dist);
		minlight = cl.dlights[lnum].minlight;
		if (rad < minlight)
			continue;
		minlight = rad - minlight;

		for (i=0 ; i<3 ; i++)
		{
			impact[i] = cl.dlights[lnum].transformed[i] /*origin[i]*/ -
					surf->plane->normal[i]*dist;
		}

		local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];

		for (t = 0 ; t < surf->tmax ; t++)
		{
			td = local[1] - t*16;
			if (td < 0)
				td = -td;
			for (s=0 ; s < surf->smax ; s++)
			{
				sd = local[0] - s*16;
				if (sd < 0)
					sd = -sd;
				if (sd > td)
					dist = sd + (td>>1);
				else
					dist = td + (sd>>1);
				if (dist < minlight)
#ifdef QUAKE2
				{
					unsigned temp;
					temp = (rad - dist)*256;
					i = t * surf->smax + s;
					if (!cl.dlights[lnum].dark)
						blocklights[i] += temp;
					else
					{
						if (blocklights[i] > temp)
							blocklights[i] -= temp;
						else
							blocklights[i] = 0;
					}
				}
#else
					blocklights[t * surf->smax + s] += (rad - dist)*256;
#endif
			}
		}
	}
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the 8.8 format in blocklights
===============
*/
void R_BuildLightMap (void)
{
#if 0
	int			smax, tmax;
#endif
	int			t;
	int			i, size;
	byte		*lightmap;
	unsigned	scale;
	int			maps;
	msurface_t	*surf;

	surf = r_drawsurf.surf;

#if 0
	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
#endif

	size = surf->smax * surf->tmax;
	lightmap = surf->samples;

	if (r_fullbright.value || !cl.worldmodel->lightdata)
	{
		for (i = 0 ; i < size ; i++)
			blocklights[i] = 0;
		return;
	}

// clear to ambient
	for (i = 0 ; i < size ; i++)
		blocklights[i] = r_refdef.ambientlight<<8;


// add all the lightmaps
	if (lightmap)
	{
		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			maps++)
		{
			scale = r_drawsurf.lightadj[maps];	// 8.8 fraction
			for (i=0 ; i<size ; i++)
				blocklights[i] += lightmap[i] * scale;
			lightmap += size;	// skip to next lightmap
		}
	}

// add all the dynamic lights
	if (surf->dlightframe == cl.r_framecount)
		R_AddDynamicLights ();

//qbism ftestains begin
	if (surf->stained)
	{
		stmap *stain;
		int sstride;
		int x, y;

		stain = stainmaps + surf->lightmaptexturenum*LIGHTMAPS_BLOCK_WIDTH*LIGHTMAPS_BLOCK_HEIGHT;
		stain += (surf->light_t * LIGHTMAPS_BLOCK_WIDTH + surf->light_s);

		sstride = LIGHTMAPS_BLOCK_WIDTH - surf->smax;

		i = 0;

		for (x = 0; x < surf->tmax; x++, stain+=sstride)
		{
			for (y = 0; y < surf->smax; y++, i++, stain++)
			{
				t = (255*256*256-127*256-(int)blocklights[i]*(*stain)) >> (16 - VID_CBITS);

				if (t < (1 << 6))
					t = (1 << 6);

				blocklights[i] = t;
			}
		}
		return;
	}
//qbism ftestains end

// bound, invert, and shift
	for (i = 0 ; i < size ; i++)
	{
		t = (255*256 - (int)blocklights[i]) >> (8 - VID_CBITS);

		if (t < (1 << 6))
			t = (1 << 6);

		blocklights[i] = t;
	}
}




/*
===============
R_DrawSurface
===============
*/
void R_DrawSurface (void)
{
	unsigned char	*basetptr;
	int				smax, tmax, twidth;
	int				u;
	int				soffset, basetoffset, texwidth;
	int				horzblockstep;
	unsigned char	*pcolumndest;
	void			(*pblockdrawer)(void);
	texture_t		*mt;

// calculate the lightings
	R_BuildLightMap ();

	surfrowbytes = r_drawsurf.rowbytes;

	mt = r_drawsurf.texture;

	r_source = (byte *)mt + mt->offsets[r_drawsurf.surfmip];

// the fractional light values should range from 0 to (VID_GRADES - 1) << 16
// from a source range of 0 - 255

	texwidth = mt->width >> r_drawsurf.surfmip;

	blocksize = 16 >> r_drawsurf.surfmip;
	blockdivshift = 4 - r_drawsurf.surfmip;
	blockdivmask = (1 << blockdivshift) - 1;

#if 0
	r_lightwidth = (r_drawsurf.surf->extents[0]>>4)+1;
#else
	r_lightwidth = r_drawsurf.surf->smax;
#endif

	r_numhblocks = r_drawsurf.surfwidth >> blockdivshift;
	r_numvblocks = r_drawsurf.surfheight >> blockdivshift;

//==============================

	pblockdrawer = surfmiptable[r_drawsurf.surfmip];
// TODO: only needs to be set when there is a display settings change
	horzblockstep = blocksize;

	smax = mt->width >> r_drawsurf.surfmip;
	twidth = texwidth;
	tmax = mt->height >> r_drawsurf.surfmip;
	sourcetstep = texwidth;
	r_stepback = tmax * twidth;

	r_sourcemax = r_source + (tmax * smax);

	soffset = r_drawsurf.surf->texturemins[0];
	basetoffset = r_drawsurf.surf->texturemins[1];

// << 16 components are to guarantee positive values for %
	soffset = ((soffset >> r_drawsurf.surfmip) + (smax << 16)) % smax;
	basetptr = &r_source[((((basetoffset >> r_drawsurf.surfmip)
		+  (tmax << 16)) % tmax) * twidth)];

	pcolumndest = r_drawsurf.surfdat;

	for (u=0 ; u<r_numhblocks; u++)
	{
		r_lightptr = blocklights + u;

		prowdestbase = pcolumndest;

		pbasesource = basetptr + soffset;

		(*pblockdrawer)();

		soffset = soffset + blocksize;
		if (soffset >= smax)
			soffset = 0;

		pcolumndest += horzblockstep;
	}
}


//=============================================================================

#if	!id386

/*
================
R_DrawSurfaceBlock8_mip0
================
*/
void R_DrawSurfaceBlock8_mip0 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 4;
		lightrightstep = (r_lightptr[1] - lightright) >> 4;

		for (i = 0 ; i < 16 ; i++)
		{
			lighttemp = lightleft - lightright;
			lightstep = lighttemp >> 4;

			light = lightright;

			for (b=15; b>=0; b--)
			{
				pix = psource[b];
				prowdest[b] = ((unsigned char *)vid.colormap)
						[(light & 0xFF00) + pix];
				light += lightstep;
			}

			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_mip1
================
*/
void R_DrawSurfaceBlock8_mip1 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = prowdestbase;

	for (v = 0 ; v < r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 3;
		lightrightstep = (r_lightptr[1] - lightright) >> 3;

		for (i=0 ; i<8 ; i++)
		{
			lighttemp = lightleft - lightright;
			lightstep = lighttemp >> 3;

			light = lightright;

			for (b=7; b>=0; b--)
			{
				pix = psource[b];
				prowdest[b] = ((unsigned char *)vid.colormap)
						[(light & 0xFF00) + pix];
				light += lightstep;
			}

			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_mip2
================
*/
void R_DrawSurfaceBlock8_mip2 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 2;
		lightrightstep = (r_lightptr[1] - lightright) >> 2;

		for (i = 0 ; i < 4 ; i++)
		{
			lighttemp = lightleft - lightright;
			lightstep = lighttemp >> 2;

			light = lightright;

			for (b=3; b>=0; b--)
			{
				pix = psource[b];
				prowdest[b] = ((unsigned char *)vid.colormap)
						[(light & 0xFF00) + pix];
				light += lightstep;
			}

			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_mip3
================
*/
void R_DrawSurfaceBlock8_mip3 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 1;
		lightrightstep = (r_lightptr[1] - lightright) >> 1;

		for (i = 0 ; i < 2 ; i++)
		{
			lighttemp = lightleft - lightright;
			lightstep = lighttemp >> 1;

			light = lightright;

			for (b=1; b>=0; b--)
			{
				pix = psource[b];
				prowdest[b] = ((unsigned char *)vid.colormap)
						[(light & 0xFF00) + pix];
				light += lightstep;
			}

			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}

#endif // !id386

#endif // !GLQUAKE - WinQuake Software renderer