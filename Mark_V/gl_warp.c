#ifdef GLQUAKE // GLQUAKE specific

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
//gl_warp.c -- warping animation support



#include "quakedef.h"





int gl_warpimagesize;

float load_subdivide_size; //johnfitz -- remember what subdivide_size value was when this map was loaded

float	turbsin[] =
{
	#include "gl_warp_sin.h"
};

#define WARPCALC(s,t) ((s + turbsin[(int)((t*2)+(cl.ctime*(128.0/M_PI))) & 255]) * (1.0/64)) //johnfitz -- correct warp
#define WARPCALC2(s,t) ((s + turbsin[(int)((t*0.125+cl.ctime)*(128.0/M_PI)) & 255]) * (1.0/64)) //johnfitz -- old warp


//==============================================================================
//
//  OLD-STYLE WATER
//
//==============================================================================

extern	qmodel_t	*loadmodel;

msurface_t	*warpface;

void BoundPoly (int numverts, float *verts, vec3_t mins, vec3_t maxs)
{
	int		i, j;
	float	*v;

	mins[0] = mins[1] = mins[2] = 9999;
	maxs[0] = maxs[1] = maxs[2] = -9999;
	v = verts;
	for (i=0 ; i<numverts ; i++)
		for (j=0 ; j<3 ; j++, v++)
		{
			if (*v < mins[j])
				mins[j] = *v;
			if (*v > maxs[j])
				maxs[j] = *v;
		}
}

void SubdividePolygon (int numverts, float *verts)
{
	int		i, j, k;
	vec3_t	mins, maxs;
	float	m;
	float	*v;
	vec3_t	front[64], back[64];
	int		f, b;
	float	dist[64];
	float	frac;
	glpoly_t	*poly;
	float	s, t;

	if (numverts > 60)
		Host_Error ("numverts = %d", numverts);

	BoundPoly (numverts, verts, mins, maxs);

	for (i=0 ; i<3 ; i++)
	{
		m = (mins[i] + maxs[i]) * 0.5;
		m = gl_subdivide_size.value * floor (m/gl_subdivide_size.value + 0.5);
		if (maxs[i] - m < 8)
			continue;
		if (m - mins[i] < 8)
			continue;

		// cut it
		v = verts + i;
		for (j=0 ; j<numverts ; j++, v+= 3)
			dist[j] = *v - m;

		// wrap cases
		dist[j] = dist[0];
		v-=i;
		VectorCopy (verts, v);

		f = b = 0;
		v = verts;
		for (j=0 ; j<numverts ; j++, v+= 3)
		{
			if (dist[j] >= 0)
			{
				VectorCopy (v, front[f]);
				f++;
			}
			if (dist[j] <= 0)
			{
				VectorCopy (v, back[b]);
				b++;
			}
			if (dist[j] == 0 || dist[j+1] == 0)
				continue;
			if ( (dist[j] > 0) != (dist[j+1] > 0) )
			{
				// clip point
				frac = dist[j] / (dist[j] - dist[j+1]);
				for (k=0 ; k<3 ; k++)
					front[f][k] = back[b][k] = v[k] + frac*(v[3+k] - v[k]);
				f++;
				b++;
			}
		}

		SubdividePolygon (f, front[0]);
		SubdividePolygon (b, back[0]);
		return;
	}

	poly = (glpoly_t *)Hunk_Alloc (sizeof(glpoly_t) + (numverts-4) * VERTEXSIZE*sizeof(float));
	poly->next = warpface->polys->next;
	warpface->polys->next = poly;
	poly->numverts = numverts;
	for (i=0 ; i<numverts ; i++, verts+= 3)
	{
		VectorCopy (verts, poly->verts[i]);
		s = DotProduct (verts, warpface->texinfo->vecs[0]);
		t = DotProduct (verts, warpface->texinfo->vecs[1]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;
	}
}

/*
================
GL_SubdivideSurface
================
*/
void GL_SubdivideSurface (msurface_t *fa)
{
	vec3_t	verts[64];
	int		i;

	warpface = fa;

	//the first poly in the chain is the undivided poly for newwater rendering.
	//grab the verts from that.
	for (i = 0; i < fa->polys->numverts; i++)
		VectorCopy (fa->polys->verts[i], verts[i]);

	SubdividePolygon (fa->polys->numverts, verts[0]);
}

/*
================
DrawWaterPoly -- johnfitz
================
*/
void DrawWaterPoly (glpoly_t *p, cbool isteleporter)
{
	float	*v;
	int		i;

	if (load_subdivide_size > 48)
	{
		eglBegin (GL_POLYGON);
		v = p->verts[0];
		for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
		{
			eglTexCoord2f (WARPCALC2(v[3],v[4]), WARPCALC2(v[4],v[3]));
			if (gl_waterripple.value && isteleporter == false)
			{
				vec3_t		newverts;
				newverts[0] = v[0];
				newverts[1] = v[1];
				newverts[2] = v[2] + gl_waterripple.value * sin(v[0] * 0.05 + cl.ctime * 3) * sin(v[2] * 0.05 + cl.ctime * 3);


				eglVertex3fv (newverts);
			}
			else
				eglVertex3fv (v);
		}
		eglEnd ();
	}
	else
	{
		eglBegin (GL_POLYGON);
		v = p->verts[0];
		for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
		{
			eglTexCoord2f (WARPCALC(v[3],v[4]), WARPCALC(v[4],v[3]));
			if (gl_waterripple.value && isteleporter == false)
			{
				vec3_t		newverts;
				newverts[0] = v[0];
				newverts[1] = v[1];
				newverts[2] = v[2] + gl_waterripple.value * sin(v[0] * 0.05 + cl.ctime * 3) * sin(v[2] * 0.05 + cl.ctime * 3);

				eglVertex3fv (newverts);
			}
			else
				eglVertex3fv (v);
		}
		eglEnd ();
	}
}

//==============================================================================
//
//  RENDER-TO-FRAMEBUFFER WATER
//
//==============================================================================

/*
=============
R_UpdateWarpTextures -- johnfitz -- each frame, update warping textures
=============
*/
void R_SetupView_UpdateWarpTextures (void)
{
	texture_t *tx;
	int i;
	float x, y, x2, warptess;

	if (frame.oldwater || cl.paused || r_drawflat_cheatsafe || r_lightmap_cheatsafe)
		return;

	warptess = 128.0/CLAMP (3.0, floor(gl_waterquality.value), 64.0);

	for (i=0; i<cl.worldmodel->numtextures; i++)
	{
		if (!(tx = cl.worldmodel->textures[i]))
			continue;

		if (!tx->update_warp)
			continue;

		//render warp
		Draw_SetCanvas (CANVAS_WARPIMAGE);
		GL_Bind (tx->gltexture);
		for (x=0.0; x<128.0; x=x2)
		{
			x2 = x + warptess;
			eglBegin (GL_TRIANGLE_STRIP);
			for (y=0.0; y<128.01; y+=warptess) // .01 for rounding errors
			{
				eglTexCoord2f (WARPCALC(x,y), WARPCALC(y,x));
				eglVertex2f (x,y);
				eglTexCoord2f (WARPCALC(x2,y), WARPCALC(y,x2));
				eglVertex2f (x2,y);
			}
			eglEnd();
		}

		//copy to texture
		GL_Bind (tx->warpimage);
		eglCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, clx, cly + clheight - gl_warpimagesize, gl_warpimagesize, gl_warpimagesize);

		tx->update_warp = false;
	}

	//if warp render went down into sbar territory, we need to be sure to refresh it next frame
	if (gl_warpimagesize + sb_lines > clheight)
		Sbar_Changed ();

	//if viewsize is less than 100, we need to redraw the frame around the viewport
	glquake_scr_tileclear_updates = 0;
}

#if 1 // UNDERWATER WARP  R_WATERWARP
static void GL_Warp_CalcUnderwaterCoords (float x, float y, float CYCLE_X, float CYCLE_Y, float AMP_X, float AMP_Y)
{
	// vkQuake is actually 99% of the way there; all that it's missing is a sign flip and it's AMP is half what it should be.
	// this is now extremely close to consistency with mankrip's tests
	const float texX = (x - (sin (y * CYCLE_X + cl.time) * AMP_X)) * (1.0f - AMP_X * 2.0f) + AMP_X;
	const float texY = (y - (sin (x * CYCLE_Y + cl.time) * AMP_Y)) * (1.0f - AMP_Y * 2.0f) + AMP_Y;

	eglTexCoord2f (texX, texY);
	eglVertex2f (x, y);
}

extern gltexture_t *r_underwaterwarptexture;

void GL_WarpScreen (void)
{
	int x, y;

	// ripped this from vkQuake at https://github.com/Novum/vkQuake/blob/master/Shaders/screen_warp.comp
	// our x and y coords are already incoming at 0..1 range so we don't need to rescale them.
	const float aspect = (float) r_refdef.vrect.width / (float) r_refdef.vrect.height;
	const float CYCLE_X = M_PI * r_waterwarp_cycle.value; // tune or cvarize as you wish
	const float CYCLE_Y = CYCLE_X * aspect;
	const float AMP_X = 1.0f / r_waterwarp_amp.value; // tune or cvarize as you wish
	const float AMP_Y = AMP_X * aspect;

	// copy over the texture
	GL_Bind (r_underwaterwarptexture);
	eglCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, vid.maxwarpwidth, vid.maxwarpheight);

	// switch vp, ortho, mvp, etc; this should be the same viewport rect as is set in the main glViewport call for the 3D scene; if you've changed it from
	// the stock Fitz code, you should change this too to match.
	eglViewport (clx + r_refdef.vrect.x,
				cly + clheight - r_refdef.vrect.y - r_refdef.vrect.height,
				r_refdef.vrect.width,
				r_refdef.vrect.height);

	eglMatrixMode (GL_PROJECTION);
	eglLoadIdentity ();

	// bottom-left-is-origin crap
#ifdef DIRECT3D9_WRAPPER
	eglOrtho (0, 1, 1, 0, -1, 1);
#else
	eglOrtho (0, 1, 0, 1, -1, 1);
#endif

	eglMatrixMode (GL_MODELVIEW);
	eglLoadIdentity ();

	eglDisable (GL_DEPTH_TEST);
	eglDisable (GL_CULL_FACE);
	eglDisable (GL_BLEND);
	eglDisable (GL_ALPHA_TEST);
	eglColor4f (1, 1, 1, 1);

	// draw the warped view; tune or cvarize this all you wish; maybe you'll create an r_underwaterwarpquality cvar?
	// yeah, it's a lot of verts - so what?  There's far far more pixels than vertexes so vertex count isn't that big a deal here.
	for (x = 0; x < 32; x++)
	{
		eglBegin (GL_TRIANGLE_STRIP);

		for (y = 0; y <= 32; y++)
		{
			GL_Warp_CalcUnderwaterCoords ((float) x / 32.0f, (float) y / 32.0f, CYCLE_X, CYCLE_Y, AMP_X, AMP_Y);
			GL_Warp_CalcUnderwaterCoords ((float) (x + 1) / 32.0f, (float) y / 32.0f, CYCLE_X, CYCLE_Y, AMP_X, AMP_Y);
		}

		eglEnd();
	}

	// if viewsize is less than 100, we need to redraw the frame around the viewport
	glquake_scr_tileclear_updates = 0;
}
#endif // 1

#endif // GLQUAKE specific