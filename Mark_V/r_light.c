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
// r_light.c

#include "quakedef.h" // Baker: Mods = basically zero.  It's just lights.

int d_lightstylevalue[256];	// 8.8 fraction of base light value


/*
==================
R_AnimateLight
==================
*/
void R_AnimateLight (void)
{
	int			i,j,k;

//
// light animations
// 'm' is normal light, 'a' is no light, 'z' is double bright
	i = (int)(cl.ctime * 10);
	for (j = 0 ; j < MAX_LIGHTSTYLES ; j++)
	{
		if (!cl.lightstyle[j].length)
		{
			d_lightstylevalue[j] = 256;
			continue;
		}
		//johnfitz -- r_flatlightstyles
		if (r_flatlightstyles.value == 2)
			k = cl.lightstyle[j].peak - 'a';
		else if (r_flatlightstyles.value == 1)
			k = cl.lightstyle[j].average - 'a';
		else
		{
			k = i % cl.lightstyle[j].length;
			k = cl.lightstyle[j].map[k] - 'a';
		}
		d_lightstylevalue[j] = k * 22;
	}
}


#ifdef GLQUAKE_RENDERER_SUPPORT
/*
=============================================================================

DYNAMIC LIGHTS BLEND RENDERING (gl_flashblend 1)

=============================================================================
*/

void AddLightBlend (float r, float g, float b, float a2)
{
	float	a;

	v_blend[3] = a = v_blend[3] + a2*(1-v_blend[3]);

	a2 = a2/a;

	v_blend[0] = v_blend[1]*(1-a2) + r*a2;
	v_blend[1] = v_blend[1]*(1-a2) + g*a2;
	v_blend[2] = v_blend[2]*(1-a2) + b*a2;
}

static float	bubble_sin[17], bubble_cos[17];

void R_Init_FlashBlend_Bubble (void)
{
	int	i;
	float	a;

	for (i = 16 ; i >= 0 ; i --)
	{
		a = i/16.0 * M_PI * 2;
		bubble_sin[i] = sin(a);
		bubble_cos[i] = cos(a);
	}
}

void R_RenderDlight (dlight_t *light)
{
	int		i, j;
	vec3_t	v;
	float	rad;

	rad = light->radius * 0.35;

	VectorSubtract (light->origin, r_origin, v);
	if (VectorLength (v) < rad)
	{	// view is inside the dlight
		AddLightBlend (1, 0.5, 0, light->radius * 0.0003);
		return;
	}

	eglBegin (GL_TRIANGLE_FAN);
	eglColor3f (0.2,0.1,0.0);
	for (i=0 ; i<3 ; i++)
		v[i] = light->origin[i] - vpn[i]*rad;
	eglVertex3fv (v);
	eglColor3f (0,0,0);
	for (i=16 ; i>=0 ; i--)
	{
		for (j=0 ; j<3 ; j++)
			v[j] = light->origin[j] + vright[j] * bubble_cos[i] * rad + vup[j] * bubble_sin[i] * rad;
		eglVertex3fv (v);
	}
	eglEnd ();
}

/*
=============
R_RenderDlights_Flashblend
=============
*/
void R_RenderDlights_Flashblend (void)
{
	int		i;
	dlight_t	*l;

	if (!gl_flashblend.value)
		return;

	eglDepthMask (0);
	eglDisable (GL_TEXTURE_2D);
	eglShadeModel (GL_SMOOTH);
	eglEnable (GL_BLEND);
	eglBlendFunc (GL_ONE, GL_ONE);

	l = cl.dlights;
	for (i=0 ; i<MAX_FITZQUAKE_DLIGHTS ; i++, l++)
	{
		if (l->die < cl.time || !l->radius)
			continue;
		R_RenderDlight (l);
	}

	eglColor3f (1,1,1);
	eglDisable (GL_BLEND);
	eglEnable (GL_TEXTURE_2D);
	eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	eglDepthMask (1);
}
#endif // GLQUAKE_RENDERER_SUPPORT


/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLights
=============
*/
void R_MarkLights (dlight_t *light, int num, mnode_t *node)
{
	mplane_t	*splitplane;
	msurface_t	*surf;
	vec3_t		impact;
	float		dist, l, maxdist;
	unsigned int i;
	int			j, s, t;

	while (1)
	{
		if (node->contents < 0)
			return;

		splitplane = node->plane;

		if (splitplane->type < 3)
			dist = light->transformed[splitplane->type] - splitplane->dist;
		else
		dist = DotProduct (light->transformed, splitplane->normal) - splitplane->dist;

		if (dist > light->radius)
		{
			node = node->children[0];
				continue;
		}

		if (dist < -light->radius)
		{
			node = node->children[1];
				continue;
		}

		break;
	}

	maxdist = light->radius*light->radius;
	// mark the polygons
	surf = cl.worldmodel->surfaces + node->firstsurface;
	for (i = 0 ; i < node->numsurfaces ; i++, surf++)
	{
		if (surf->flags & SURF_DRAWTILED) // no lights on these
			continue;

		for (j=0 ; j<3 ; j++)
			impact[j] = light->transformed[j] - surf->plane->normal[j]*dist;

		// clamp center of light to corner and check brightness
		l = DotProduct (impact, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3] - surf->texturemins[0];

		s = l + 0.5;
		s = CLAMP (0, s, surf->extents[0]);
		s = l - s;
		l = DotProduct (impact, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3] - surf->texturemins[1];
		t = l + 0.5;
		t = CLAMP (0, t, surf->extents[1]);
		t = l - t;

		// compare to minimum light
		if ((s*s+t*t+dist*dist) < maxdist)
		{
			if (surf->dlightframe != cl.r_framecount) // not dynamic until now
			{
	         memset (surf->dlightbits, 0, sizeof (surf->dlightbits));
				surf->dlightframe = cl.r_framecount;
			}

			// mark the surf and ent for this dlight
      		surf->dlightbits[num >> 5] |= 1U << (num & 31);
		}
	}

	if (node->children[0]->contents >= 0) R_MarkLights (light, num, node->children[0]);
	if (node->children[1]->contents >= 0) R_MarkLights (light, num, node->children[1]);
}

/*
=============
R_PushDlights
=============
*/
void R_TransformDLight (glmatrix *m, float *transformed, float *origin)
{
	if (m)
	{
		// transformed
		transformed[0] = origin[0] * m->m16[0] + origin[1] * m->m16[4] + origin[2] * m->m16[8] + m->m16[12];
		transformed[1] = origin[0] * m->m16[1] + origin[1] * m->m16[5] + origin[2] * m->m16[9] + m->m16[13];
		transformed[2] = origin[0] * m->m16[2] + origin[1] * m->m16[6] + origin[2] * m->m16[10] + m->m16[14];
	}
	else
	{
		// untransformed
		transformed[0] = origin[0];
		transformed[1] = origin[1];
		transformed[2] = origin[2];
	}

}

void R_PushDlights (entity_t *ent)
{
	int		i;
	dlight_t	*l = cl.dlights;
	glmatrix 	*m = ent ? &ent->gl_matrix : NULL;
	mnode_t		*headnode;

	if (ent)
		headnode = ent->model->nodes + ent->model->hulls[0].firstclipnode;
	else headnode = cl.worldmodel->nodes;

	for (i=0 ; i<MAX_FITZQUAKE_DLIGHTS ; i++, l++)
	{
		if (l->die < cl.time || (l->radius <= 0))
			continue;

	// move the light back to the same space as the model

		R_TransformDLight (m, l->transformed, l->origin);
      	R_MarkLights (l, i, headnode);
	}
}

#ifdef GLQUAKE_RENDERER_SUPPORT
void R_PushDlights_World (void)
{
	if (gl_flashblend.value) // Non-flashblend dlights
		return;

	R_PushDlights (NULL);
}
#endif // GLQUAKE_RENDERER_SUPPORT

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

#ifdef GLQUAKE_RENDERER_SUPPORT
mplane_t		*lightplane;
vec3_t			lightspot;
vec3_t			lightcolor; //johnfitz -- lit support via lordhavoc

/*
=============
RecursiveLightPoint -- johnfitz -- replaced entire function for lit support via lordhavoc
=============
*/
int RecursiveLightPoint (vec3_t color, mnode_t *node, vec3_t start, vec3_t end)
{
	float		front, back, frac;
	vec3_t		mid;

loc0:
	if (node->contents < 0)
		return false;		// didn't hit anything

// calculate mid point
	if (node->plane->type < 3)
	{
		front = start[node->plane->type] - node->plane->dist;
		back = end[node->plane->type] - node->plane->dist;
	}
	else
	{
		front = DotProduct(start, node->plane->normal) - node->plane->dist;
		back = DotProduct(end, node->plane->normal) - node->plane->dist;
	}

	// LordHavoc: optimized recursion
	if ((back < 0) == (front < 0))
//		return RecursiveLightPoint (color, node->children[front < 0], start, end);
	{
		node = node->children[front < 0];
		goto loc0;
	}

	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;

// go down front side
	if (RecursiveLightPoint (color, node->children[front < 0], start, mid))
		return true;	// hit something
	else
	{
		unsigned int i;
		int ds, dt;
		msurface_t	*surf;
// check for impact on this node
		VectorCopy (mid, lightspot);
		lightplane = node->plane;

		surf = cl.worldmodel->surfaces + node->firstsurface;
		for (i = 0 ; i < node->numsurfaces ; i++, surf++)
		{
			if (surf->flags & SURF_DRAWTILED)
				continue;	// no lightmaps
#if 1
				ds = (int) ((double) DoublePrecisionDotProduct (mid, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3]);
				dt = (int) ((double) DoublePrecisionDotProduct (mid, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3]);		
#else
				ds = (int) ((float) DotProduct (mid, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3]);
				dt = (int) ((float) DotProduct (mid, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3]);
#endif
	
			if (ds < surf->texturemins[0] || dt < surf->texturemins[1])
				continue;
	
			ds -= surf->texturemins[0];
			dt -= surf->texturemins[1];
	
			if ( ds > surf->extents[0] || dt > surf->extents[1] )
				continue;
	
			if (surf->samples)
			{
				// LordHavoc: enhanced to interpolate lighting
				byte *lightmap;
				int maps, line3, dsfrac = ds & 15, dtfrac = dt & 15, r00 = 0, g00 = 0, b00 = 0, r01 = 0, g01 = 0, b01 = 0, r10 = 0, g10 = 0, b10 = 0, r11 = 0, g11 = 0, b11 = 0;
				float scale;
				
				line3 = surf->smax *3;

				lightmap = surf->samples + ( (dt>>4) * surf->smax + (ds>>4) )* 3;  // LordHavoc: *3 for color
				for (maps = 0;maps < MAXLIGHTMAPS && surf->styles[maps] != 255;maps++)
				{
					scale = (float) d_lightstylevalue[surf->styles[maps]] * 1.0 / 256.0;
					r00 += (float) lightmap[      0] * scale;g00 += (float) lightmap[      1] * scale;b00 += (float) lightmap[2] * scale;
					r01 += (float) lightmap[      3] * scale;g01 += (float) lightmap[      4] * scale;b01 += (float) lightmap[5] * scale;
					r10 += (float) lightmap[line3+0] * scale;g10 += (float) lightmap[line3+1] * scale;b10 += (float) lightmap[line3+2] * scale;
					r11 += (float) lightmap[line3+3] * scale;g11 += (float) lightmap[line3+4] * scale;b11 += (float) lightmap[line3+5] * scale;
					lightmap += surf->smax * surf->tmax * 3; // LordHavoc: *3 for colored lighting
				}
					
				color[0] += (float) ((int) ((((((((r11-r10) * dsfrac) >> 4) + r10)-((((r01-r00) * dsfrac) >> 4) + r00)) * dtfrac) >> 4) + ((((r01-r00) * dsfrac) >> 4) + r00)));
				color[1] += (float) ((int) ((((((((g11-g10) * dsfrac) >> 4) + g10)-((((g01-g00) * dsfrac) >> 4) + g00)) * dtfrac) >> 4) + ((((g01-g00) * dsfrac) >> 4) + g00)));
				color[2] += (float) ((int) ((((((((b11-b10) * dsfrac) >> 4) + b10)-((((b01-b00) * dsfrac) >> 4) + b00)) * dtfrac) >> 4) + ((((b01-b00) * dsfrac) >> 4) + b00)));
			}
			return true; // success
		}

	// go down back side
		return RecursiveLightPoint (color, node->children[front >= 0], mid, end);
	}
}
#endif // GLQUAKE_RENDERER_SUPPORT

// Baker: Tried the GL function to see if it was faster for WinQuake
// Didn't see any kind of speed improvement on timedemos and actually
// they might have been slightly slower.  Would otherwise merge the
// code if it wasn't a lot of work for no gain.

#ifdef WINQUAKE_RENDERER_SUPPORT
int RecursiveLightPoint (mnode_t *node, vec3_t start, vec3_t end)
{
	int			r, side, s, t, ds, dt, maps;
	unsigned int i;
	float		front, back, frac;
	mplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	mtexinfo_t	*tex;
	byte		*lightmap;
	unsigned	scale;

	if (node->contents < 0)
		return -1;		// didn't hit anything

// calculate mid point

// FIXME: optimize for axial
	plane = node->plane;
	front = DotProduct (start, plane->normal) - plane->dist;
	back = DotProduct (end, plane->normal) - plane->dist;
	side = front < 0;

	if ( (back < 0) == side)
		return RecursiveLightPoint (node->children[side], start, end);

	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;

// go down front side
	r = RecursiveLightPoint (node->children[side], start, mid);
	if (r >= 0)
		return r;		// hit something

	if ( (back < 0) == side )
		return -1;		// didn't hit anything

// check for impact on this node

	surf = cl.worldmodel->surfaces + node->firstsurface;
	for (i = 0 ; i < node->numsurfaces ; i++, surf++)
	{
		if (surf->flags & SURF_DRAWTILED)
			continue;	// no lightmaps

		tex = surf->texinfo;

		s = DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3];;
#if 1
		ds = (int) ((double) DoublePrecisionDotProduct (mid, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3]);
		dt = (int) ((double) DoublePrecisionDotProduct (mid, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3]);
#else
		s = DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3];;
#endif
		if (s < surf->texturemins[0] ||
		t < surf->texturemins[1])
			continue;

		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];

		if ( ds > surf->extents[0] || dt > surf->extents[1] )
			continue;

		if (!surf->samples)
			return 0;

		ds >>= 4;
		dt >>= 4;

		lightmap = surf->samples;
		r = 0;
		if (lightmap)
		{
#if 0
			lightmap += dt * ((surf->extents[0]>>4)+1) + ds;
#else
			lightmap += dt * surf->smax + ds;
#endif

			for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
				maps++)
			{
				scale = d_lightstylevalue[surf->styles[maps]];
				r += *lightmap * scale;
#if 0
				lightmap += ((surf->extents[0]>>4)+1) * ((surf->extents[1]>>4)+1);
#else
				lightmap += surf->smax * surf->tmax;
#endif
			}

			r >>= 8;
		}

		return r;
	}

// go down back side
	return RecursiveLightPoint (node->children[!side], mid, end);
}

#endif // WINQUAKE_RENDERER_SUPPORT

/*
=============
R_LightPoint
=============
*/
int R_LightPoint (vec3_t p)
{
	vec3_t		end;
#ifdef WINQUAKE_RENDERER_SUPPORT
	int			r;
#endif // WINQUAKE_RENDERER_SUPPORT

	if (!cl.worldmodel->lightdata)
	{
#ifdef GLQUAKE_RENDERER_SUPPORT
		lightcolor[0] = lightcolor[1] = lightcolor[2] = 255;
#endif // GLQUAKE_RENDERER_SUPPORT
		return 255;
	}

	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 8192;  //johnfitz -- was 2048

#ifdef GLQUAKE_RENDERER_SUPPORT
	lightcolor[0] = lightcolor[1] = lightcolor[2] = 0;
	RecursiveLightPoint (lightcolor, cl.worldmodel->nodes, p, end);
	return ((lightcolor[0] + lightcolor[1] + lightcolor[2]) * (1.0f / 3.0f));
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
	r = RecursiveLightPoint (cl.worldmodel->nodes, p, end);

	if (r == -1)
		r = 0;

	if (r < r_refdef.ambientlight)
		r = r_refdef.ambientlight;

	return r;
#endif // WINQUAKE_RENDERER_SUPPORT
}



