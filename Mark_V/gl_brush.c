#ifdef GLQUAKE // GLQUAKE specific

/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
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
// r_brush.c: brush model rendering. renamed from r_surf.c



#include "quakedef.h"

static void R_BuildLightMap (msurface_t *surf);



void CalcTexCoords (float *verts, float *s, float *t, int renderfx)
{
	float scroll;
	if (renderfx & SURF_DRAWENVMAP)
	{
		vec3_t		dir;
		float		length;

		VectorSubtract (verts, r_origin, dir);
		dir[2] *= 3;	// flatten the sphere

		length = VectorLength (dir);
		length = 6*63/length;

		dir[0] *= length;
		dir[1] *= length;

		*s = (dir[0]) * (1.0/256);
		*t = (dir[1]) * (1.0/256);
		return;
	}

	// We want this to scroll 1 texture length every X seconds in increments of 256

	// Seconds range.  10 secconds.

#define SCROLL_CYCLE_SECONDS 5
	scroll = ((int)cl.ctime % SCROLL_CYCLE_SECONDS) * SCROLL_CYCLE_SECONDS;	// (whole number); // Calc floor.  Will be an integer
	scroll = cl.ctime - scroll;			// Scroll is a number between 0 and 10 (but not 10)
	scroll = (1.0f / SCROLL_CYCLE_SECONDS) * scroll;

	*s = verts[3];
	*t = verts[4];

	if (renderfx & SURF_SCROLLX)
		*s = *s + scroll;
	if (renderfx & SURF_SCROLLY)
		*t = *t + scroll;
}


/*
================
DrawGLPoly
================
*/
void DrawGLPoly (glpoly_t *p, int renderfx)
{
	float	*v;
	int		i;

	if (renderfx & (SURF_DRAWENVMAP | SURF_SCROLLX | SURF_SCROLLY))
	{
		eglBegin (GL_POLYGON);
		v = p->verts[0];
		for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
		{
			float s0, t0;
			CalcTexCoords (v, &s0, &t0, renderfx);
			eglTexCoord2f (s0, t0);
			eglVertex3fv (v);
		}
		eglEnd ();
		return;
	}

	if ((renderfx & SURF_DRAWTURB) && (renderfx & SURF_DRAWTELE)==0 && gl_waterripple.value) // liquid, not a tele and with water ripple
	{
		eglBegin (GL_POLYGON);
		v = p->verts[0];
		for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
		{
			vec3_t		newverts;
			newverts[0] = v[0];
			newverts[1] = v[1];
			newverts[2] = v[2] + gl_waterripple.value * sin(v[0] * 0.05 + cl.ctime * 3) * sin(v[2] * 0.05 + cl.ctime * 3);

			eglTexCoord2f (v[3], v[4]);
			eglVertex3fv (newverts);
		}
		eglEnd ();
		return;
	}

	eglBegin (GL_POLYGON);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
	{
		eglTexCoord2f (v[3], v[4]);
		eglVertex3fv (v);
	}
	eglEnd ();
}

/*
================
DrawGLTriangleFan -- johnfitz -- like DrawGLPoly but for r_showtris
================
*/
void DrawGLTriangleFan (glpoly_t *p, int renderfx)
{
	float	*v;
	int		i;

	if ((renderfx & SURF_DRAWTURB) && (renderfx & SURF_DRAWTELE)==0 && gl_waterripple.value)
	{
		eglBegin (GL_TRIANGLE_FAN);
		v = p->verts[0];
		for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
		{
			vec3_t		newverts;
			newverts[0] = v[0];
			newverts[1] = v[1];
			newverts[2] = v[2] + gl_waterripple.value * sin(v[0] * 0.05 + cl.ctime * 3) * sin(v[2] * 0.05 + cl.ctime * 3);

			eglVertex3fv (newverts);

		}
		eglEnd ();
		return;
	}

	eglBegin (GL_TRIANGLE_FAN);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
	{
		eglVertex3fv (v);
	}
	eglEnd ();
}

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
================
R_DrawSequentialPoly -- johnfitz -- rewritten
================
*/
// Ony R_DrawBrushModel calls me.
#define ACTIVE_MIRROR(_s) (frame.has_mirror && (Flag_Check ((_s)->flags, SURF_DRAWMIRROR) && frame.mirror_plane && (_s)->plane && !memcmp (frame.mirror_plane, (_s)->plane, sizeof(frame.mirror_plane[0]))))
#define DRAW_ACTIVE_MIRROR (active_mirror && frame.in_mirror_overlay)

static void R_DrawBrushModel_DrawSequentialPoly (entity_t *ent, msurface_t *s)
{
	glpoly_t	*p;
	texture_t	*t;
	float		*v;
	float		entalpha;
	int			i;
	cbool		active_mirror = ACTIVE_MIRROR(s); //frame.has_mirror ? false : (Flag_Check (s->flags, SURF_DRAWMIRROR) && !memcmp (frame.mirror_plane, s->plane, sizeof(frame.mirror_plane[0])));

	if (active_mirror) // && !DRAW_ACTIVE_MIRROR)
		return;  // We are an active mirror, we never get drawn in this manner when active.                        // We are an active mirror, but we are not in mirror overlay draw.

	//if (frame.in_mirror_draw && s == ent->model->mirror_only_surface) {
	//	// We are in a mirror draw, do not draw mirrors matching the plane
	//	//frame.mirror_plane
	//	if (ent->model->mirror_plane
	//	if (memcmp (frame.mirror_plane, ent->model->mirror_plane, sizeof(frame.mirror_plane[0])));
	//}


	t = R_TextureAnimation (s->texinfo->texture, currententity->frame);
	entalpha = ENTALPHA_DECODE(currententity->alpha);

// drawflat
	if (r_drawflat_cheatsafe)
	{
		if ((s->flags & SURF_DRAWTURB) && frame.oldwater)
		{
			for (p = s->polys->next; p; p = p->next)
			{
				srand((unsigned int) (uintptr_t) p);
				eglColor3f (rand()%256/255.0, rand()%256/255.0, rand()%256/255.0);
				DrawGLPoly (p, s->flags);
				rs_brushpasses++;
			}
			return;
		}

		srand((unsigned int) (uintptr_t) s->polys);
		eglColor3f (rand()%256/255.0, rand()%256/255.0, rand()%256/255.0);
		DrawGLPoly (s->polys, 0); // Drawflat doesn't do texture
		rs_brushpasses++;
		return;
	}

// fullbright
	if ((r_fullbright_cheatsafe) && !(s->flags & SURF_DRAWTILED))
	{
		if (entalpha < 1)
		{
			eglDepthMask(GL_FALSE);
			eglEnable(GL_BLEND);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			eglColor4f(1, 1, 1, entalpha);
		}
		if (s->flags & SURF_DRAWFENCE)
			eglEnable (GL_ALPHA_TEST); // Flip on alpha test

		GL_Bind (t->gltexture);
		DrawGLPoly (s->polys, s->flags);
		rs_brushpasses++;
			if (s->flags & SURF_DRAWFENCE)
				eglDisable (GL_ALPHA_TEST); // Flip alpha test back off

		if (entalpha < 1)
		{
			eglDepthMask(GL_TRUE);
			eglDisable(GL_BLEND);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			eglColor3f(1, 1, 1);
		}
		goto fullbrights;
	}

// r_lightmap
	if (r_lightmap_cheatsafe)
	{
		if (s->flags & SURF_DRAWTILED)
		{
			eglDisable (GL_TEXTURE_2D);
			DrawGLPoly (s->polys, 0); // Lightmap doesn't do texture
			eglEnable (GL_TEXTURE_2D);
			rs_brushpasses++;
			return;
		}

		R_RenderDynamicLightmaps (s);
		GL_Bind (lightmap[s->lightmaptexturenum].texture);
		if (!gl_overbright.value)
		{
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			eglColor3f(0.5, 0.5, 0.5);
		}
		eglBegin (GL_POLYGON);
		v = s->polys->verts[0];
		for (i = 0 ; i < s->polys->numverts ; i++, v+= VERTEXSIZE)
		{
			eglTexCoord2f (v[5], v[6]);
			eglVertex3fv (v);
		}
		eglEnd ();
		if (!gl_overbright.value)
		{
			eglColor3f(1,1,1);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		}
		rs_brushpasses++;
		return;
	}

// sky poly -- skip it, already handled in gl_sky.c
	if (s->flags & SURF_DRAWSKY)
		return;

// water poly
	if (s->flags & SURF_DRAWTURB)
	{
		if (currententity->alpha == ENTALPHA_DEFAULT && (s->flags & SURF_DRAWTELE)== false)
			entalpha = frame.wateralpha;

		if (entalpha < 1)
		{
			eglDepthMask(GL_FALSE);
			eglEnable(GL_BLEND);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			eglColor4f(1, 1, 1, entalpha);
		}
		if (frame.oldwater)
		{
			GL_Bind (s->texinfo->texture->gltexture);
			for (p = s->polys->next; p; p = p->next)
			{
				DrawWaterPoly (p, s->flags & SURF_DRAWTELE);
				rs_brushpasses++;
			}
			rs_brushpasses++;
		}
		else
		{
			GL_Bind (s->texinfo->texture->warpimage);
			s->texinfo->texture->update_warp = true; // FIXME: one frame too late!
			DrawGLPoly (s->polys, s->flags);
			rs_brushpasses++;
		}
		if (entalpha < 1)
		{
			eglDepthMask(GL_TRUE);
			eglDisable(GL_BLEND);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			eglColor3f(1, 1, 1);
		}
		return;
	}

// missing texture
	if (s->flags & SURF_NOTEXTURE)
	{
		if (entalpha < 1)
		{
			eglDepthMask(GL_FALSE);
			eglEnable(GL_BLEND);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			eglColor4f(1, 1, 1, entalpha);
		}
		GL_Bind (t->gltexture);
		DrawGLPoly (s->polys, 0); // I'm thinking texturefx on missing texture would be bad
		rs_brushpasses++;
		if (entalpha < 1)
		{
			eglDepthMask(GL_TRUE);
			eglDisable(GL_BLEND);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			eglColor3f(1, 1, 1);
		}
		return;
	}

// lightmapped poly
	if (entalpha < 1)
	{
		eglDepthMask(GL_FALSE);
		eglEnable(GL_BLEND);
		eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		eglColor4f(1, 1, 1, entalpha);
	}
	else
		eglColor3f(1, 1, 1);

	if (s->flags & SURF_DRAWFENCE)
		eglEnable (GL_ALPHA_TEST); // Flip on alpha test

	if (gl_overbright.value)
	{
		if (renderer.gl_texture_env_combine && renderer.gl_mtexable) //case 1: texture and lightmap in one pass, overbright using texture combiners
		{
			GL_DisableMultitexture(); // selects TEXTURE0
			GL_Bind (t->gltexture);
#if 1 // Spike workaround
#define GL_SOURCE0_ALPHA_ARB 0x8588
#define GL_SOURCE1_ALPHA_ARB 0x8589
#endif 
			eglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PRIMARY_COLOR);
			eglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
			eglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);

			GL_EnableMultitexture(); // selects TEXTURE1
			GL_Bind (lightmap[s->lightmaptexturenum].texture);
			R_RenderDynamicLightmaps (s);
			eglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			eglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			
			eglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
			eglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
			eglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);
			eglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 2.0f);
			eglBegin(GL_POLYGON);
			v = s->polys->verts[0];
			for (i=0 ; i<s->polys->numverts ; i++, v+= VERTEXSIZE)
			{
				if (s->flags & (SURF_DRAWENVMAP | SURF_SCROLLX | SURF_SCROLLY))
				{
					float s0, t0;
					CalcTexCoords (v, &s0, &t0, s->flags);
					renderer.GL_MTexCoord2fFunc (renderer.TEXTURE0, s0, t0);
				}
				else renderer.GL_MTexCoord2fFunc (renderer.TEXTURE0, v[3], v[4]);

				renderer.GL_MTexCoord2fFunc (renderer.TEXTURE1, v[5], v[6]);
				eglVertex3fv (v);
			}
			eglEnd ();
			eglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0f);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			GL_DisableMultitexture ();
			rs_brushpasses++;
		}
		else if (entalpha < 1) //case 2: can't do multipass if entity has alpha, so just draw the texture
		{
			GL_Bind (t->gltexture);
			DrawGLPoly (s->polys, s->flags);
			rs_brushpasses++;
		}
		else //case 3: texture in one pass, lightmap in second pass using 2x modulation blend func, fog in third pass
		{
			//first pass -- texture with no fog
			Fog_DisableGFog ();
			GL_Bind (t->gltexture);
			DrawGLPoly (s->polys, s->flags);
			Fog_EnableGFog ();
			rs_brushpasses++;

			//second pass -- lightmap with black fog, modulate blended
			R_RenderDynamicLightmaps (s);
			GL_Bind (lightmap[s->lightmaptexturenum].texture);
			eglDepthMask (GL_FALSE);
			eglEnable (GL_BLEND);
			eglBlendFunc(GL_DST_COLOR, GL_SRC_COLOR); //2x modulate
			Fog_StartAdditive ();
			eglBegin (GL_POLYGON);
			v = s->polys->verts[0];
			for (i=0 ; i<s->polys->numverts ; i++, v+= VERTEXSIZE)
			{
				eglTexCoord2f (v[5], v[6]);
				eglVertex3fv (v);
			}
			eglEnd ();
			Fog_StopAdditive ();
			rs_brushpasses++;

			//third pass -- black geo with normal fog, additive blended
			if (Fog_GetDensity() > 0)
			{
				eglBlendFunc(GL_ONE, GL_ONE); //add
				eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				eglColor3f(0,0,0);
				DrawGLPoly (s->polys, s->flags);
				eglColor3f(1,1,1);
				eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
				rs_brushpasses++;
			}

			eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			eglDisable (GL_BLEND);
			eglDepthMask (GL_TRUE);
		}
	}
	else
	{
		if (renderer.gl_mtexable) //case 4: texture and lightmap in one pass, regular modulation
		{
			GL_DisableMultitexture(); // selects TEXTURE0
			GL_Bind (t->gltexture);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			GL_EnableMultitexture(); // selects TEXTURE1
			GL_Bind (lightmap[s->lightmaptexturenum].texture);
			R_RenderDynamicLightmaps (s);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			eglBegin(GL_POLYGON);
			v = s->polys->verts[0];
			for (i=0 ; i<s->polys->numverts ; i++, v+= VERTEXSIZE)
			{
				if (s->flags & (SURF_DRAWENVMAP | SURF_SCROLLX | SURF_SCROLLY))
				{
					float s0, t0;
					CalcTexCoords (v, &s0, &t0, s->flags);
					renderer.GL_MTexCoord2fFunc (renderer.TEXTURE0, s0, t0);
				}
				else renderer.GL_MTexCoord2fFunc (renderer.TEXTURE0, v[3], v[4]);

				renderer.GL_MTexCoord2fFunc (renderer.TEXTURE1, v[5], v[6]);
				eglVertex3fv (v);
			}
			eglEnd ();
			GL_DisableMultitexture ();
			rs_brushpasses++;
		}
		else if (entalpha < 1) //case 5: can't do multipass if entity has alpha, so just draw the texture
		{
			GL_Bind (t->gltexture);
			DrawGLPoly (s->polys, s->flags);
			rs_brushpasses++;
		}
		else //case 6: texture in one pass, lightmap in a second pass, fog in third pass
		{
			//first pass -- texture with no fog
			Fog_DisableGFog ();
			GL_Bind (t->gltexture);
			DrawGLPoly (s->polys, s->flags);
			Fog_EnableGFog ();
			rs_brushpasses++;

			//second pass -- lightmap with black fog, modulate blended
			R_RenderDynamicLightmaps (s);
			GL_Bind (lightmap[s->lightmaptexturenum].texture);
			eglDepthMask (GL_FALSE);
			eglEnable (GL_BLEND);
			eglBlendFunc (GL_ZERO, GL_SRC_COLOR); //modulate
			Fog_StartAdditive ();
			eglBegin (GL_POLYGON);
			v = s->polys->verts[0];
			for (i=0 ; i<s->polys->numverts ; i++, v+= VERTEXSIZE)
			{
				eglTexCoord2f (v[5], v[6]);
				eglVertex3fv (v);
			}
			eglEnd ();
			Fog_StopAdditive ();
			rs_brushpasses++;

			//third pass -- black geo with normal fog, additive blended
			if (Fog_GetDensity() > 0)
			{
				eglBlendFunc(GL_ONE, GL_ONE); //add
				eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				eglColor3f(0,0,0);
				DrawGLPoly (s->polys, s->flags);
				eglColor3f(1,1,1);
				eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
				rs_brushpasses++;
			}

			eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			eglDisable (GL_BLEND);
			eglDepthMask (GL_TRUE);

		}
	}
	if (entalpha < 1)
	{
		eglDepthMask(GL_TRUE);
		eglDisable(GL_BLEND);
		eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		eglColor3f(1, 1, 1);
	}

	if (s->flags & SURF_DRAWFENCE)
		eglDisable (GL_ALPHA_TEST); // Flip alpha test back off

fullbrights:
	if (gl_fullbrights.value && t->fullbright)
	{
		eglDepthMask (GL_FALSE);
		eglEnable (GL_BLEND);
		eglBlendFunc (GL_ONE, GL_ONE);
		eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		eglColor3f (entalpha, entalpha, entalpha);
		GL_Bind (t->fullbright);
		Fog_StartAdditive ();
		DrawGLPoly (s->polys, s->flags);
		Fog_StopAdditive ();
		eglColor3f(1, 1, 1);
		eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		eglDisable (GL_BLEND);
		eglDepthMask (GL_TRUE);
		rs_brushpasses++;
	}
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel (entity_t *e)
{
	int			i;
	msurface_t	*psurf;
	float		dot;
	mplane_t	*pplane;
	qmodel_t		*clmodel;

	if (e->model->mirror_only_surface)
		return; // Only the mirror draws, that does not occur here.

	if (R_CullModelForEntity(e))
		return;


	currententity = e;
	clmodel = e->model;

	VectorSubtract (r_refdef.vieworg, e->origin, modelorg);
	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (e->angles, forward, right, up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);
	}

	psurf = &clmodel->surfaces[clmodel->firstmodelsurface];

// calculate dynamic lighting for bmodel if it's not an
// instanced model (like a healthbox ... external brush models supply their own lighting)
	if (clmodel->firstmodelsurface != 0 && !gl_flashblend.value)
	{
		// calculate entity local space for dlight transforms
		Mat4_Identity_Set (&e->gl_matrix); // GL_IdentityMatrix

		// don't need to negate angles[0] as it's not going through the extra negation in R_RotateForEntity
		if (e->angles[2]) Mat4_Rotate (&e->gl_matrix, -e->angles[2], 1, 0, 0);  // GL_RotateMatrix
		if (e->angles[0]) Mat4_Rotate (&e->gl_matrix, -e->angles[0], 0, 1, 0);  // GL_RotateMatrix
		if (e->angles[1]) Mat4_Rotate (&e->gl_matrix, -e->angles[1], 0, 0, 1);  // GL_RotateMatrix

		Mat4_Translate (&e->gl_matrix, -e->origin[0], -e->origin[1], -e->origin[2]); // GL_TranslateMatrix
		R_PushDlights (e);
	}

    eglPushMatrix ();
	e->angles[0] = -e->angles[0];	// stupid quake bug
	if (gl_zfix.value)
	{
		e->origin[0] -= DIST_EPSILON;
		e->origin[1] -= DIST_EPSILON;
		e->origin[2] -= DIST_EPSILON;
	}
	R_RotateForEntity (e->origin, e->angles);
	if (gl_zfix.value)
	{
		e->origin[0] += DIST_EPSILON;
		e->origin[1] += DIST_EPSILON;
		e->origin[2] += DIST_EPSILON;
	}
	e->angles[0] = -e->angles[0];	// stupid quake bug

	//
	// draw it
	//
	if (r_drawflat_cheatsafe) //johnfitz
		eglDisable(GL_TEXTURE_2D);

	for (i=0 ; i<clmodel->nummodelsurfaces ; i++, psurf++)
	{
// HERE?  
		pplane = psurf->plane;
		dot = DotProduct (modelorg, pplane->normal) - pplane->dist;
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			R_DrawBrushModel_DrawSequentialPoly (e, psurf);
			rs_brushpolys++;
		}
	}

	if (r_drawflat_cheatsafe) //johnfitz
		eglEnable(GL_TEXTURE_2D);

	GL_DisableMultitexture(); // selects TEXTURE0

	eglPopMatrix ();
}

/*
=================
R_DrawBrushModel_ShowTris -- johnfitz
=================
*/
void R_DrawBrushModel_ShowTris (entity_t *e)
{
	int			i;
	msurface_t	*psurf;
	float		dot;
	mplane_t	*pplane;
	qmodel_t		*clmodel;
	glpoly_t	*p;

	if (R_CullModelForEntity(e))
		return;

	currententity = e;
	clmodel = e->model;

	VectorSubtract (r_refdef.vieworg, e->origin, modelorg);
	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (e->angles, forward, right, up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);
	}

	psurf = &clmodel->surfaces[clmodel->firstmodelsurface];

    eglPushMatrix ();
	e->angles[0] = -e->angles[0];	// stupid quake bug
	R_RotateForEntity (e->origin, e->angles);
	e->angles[0] = -e->angles[0];	// stupid quake bug

	//
	// draw it
	//
	for (i=0 ; i<clmodel->nummodelsurfaces ; i++, psurf++)
	{
		pplane = psurf->plane;
		dot = DotProduct (modelorg, pplane->normal) - pplane->dist;
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if ((psurf->flags & SURF_DRAWTURB) && frame.oldwater)
				for (p = psurf->polys->next; p; p = p->next)
					DrawGLTriangleFan (p, psurf->flags);
			else DrawGLTriangleFan (psurf->polys, psurf->flags);
		}
	}

	eglPopMatrix ();
}

/*
=============================================================

	LIGHTMAPS

=============================================================
*/



lightmapinfo_t lightmap[MAX_FITZQUAKE_LIGHTMAPS];
int last_lightmap_allocated; // Quakespasm / ericw

static unsigned	blocklights[BLOCKLITE_BLOCK_SIZE]; //johnfitz -- was 18*18, added lit support (*3) and loosened surface extents maximum (LIGHTMAPS_BLOCK_WIDTH*LIGHTMAPS_BLOCK_HEIGHT)

/*
================
R_RenderDynamicLightmaps
called during rendering
================
*/
void R_RenderDynamicLightmaps (msurface_t *fa)
{
	int			maps;

	if (fa->flags & SURF_DRAWTILED) //johnfitz -- not a lightmapped surface
		return;

#if 1 // This is only for non-multitexture dynamic lights
	// add to lightmap chain
	fa->polys->chain = lightmap[fa->lightmaptexturenum].polys;
	lightmap[fa->lightmaptexturenum].polys = fa->polys;
#endif

	// check for lightmap modification
	for (maps=0; maps < MAXLIGHTMAPS && fa->styles[maps] != 255; maps++)
	{
		if (d_lightstylevalue[fa->styles[maps]] != fa->cached_light[maps])
		{
			goto dynamic;
		}
	}

	if (fa->dlightframe == cl.r_framecount	// dynamic this frame
		|| fa->cached_dlight)			// dynamic previously
	{
dynamic:
		if (gl_dynamic.value)
		{
			lightmap[fa->lightmaptexturenum].modified = true;
			if (fa->light_t < lightmap[fa->lightmaptexturenum].rectchange.t)
			{
				if (lightmap[fa->lightmaptexturenum].rectchange.h)
					lightmap[fa->lightmaptexturenum].rectchange.h += lightmap[fa->lightmaptexturenum].rectchange.t - fa->light_t;
				lightmap[fa->lightmaptexturenum].rectchange.t = fa->light_t;
			}
			if (fa->light_s < lightmap[fa->lightmaptexturenum].rectchange.l)
			{
				if (lightmap[fa->lightmaptexturenum].rectchange.w)
					lightmap[fa->lightmaptexturenum].rectchange.w += lightmap[fa->lightmaptexturenum].rectchange.l - fa->light_s;
				lightmap[fa->lightmaptexturenum].rectchange.l = fa->light_s;
			}

			if ((lightmap[fa->lightmaptexturenum].rectchange.w + lightmap[fa->lightmaptexturenum].rectchange.l) < (fa->light_s + fa->smax))
				lightmap[fa->lightmaptexturenum].rectchange.w = (fa->light_s-lightmap[fa->lightmaptexturenum].rectchange.l) + fa->smax;

			if ((lightmap[fa->lightmaptexturenum].rectchange.h + lightmap[fa->lightmaptexturenum].rectchange.t) < (fa->light_t + fa->tmax))
				lightmap[fa->lightmaptexturenum].rectchange.h = (fa->light_t-lightmap[fa->lightmaptexturenum].rectchange.t) + fa->tmax;

			R_BuildLightMap (fa);
		}
	}
}

/*
========================
AllocBlock -- returns a texture number and the position inside it
========================
*/

static int AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		texnum;

//	for (texnum=0 ; texnum<MAX_FITZQUAKE_LIGHTMAPS ; texnum++)
	for (texnum = last_lightmap_allocated ; texnum < MAX_FITZQUAKE_LIGHTMAPS ; texnum++, last_lightmap_allocated++) // Quakespasm / ericw
	{
		best = LIGHTMAPS_BLOCK_HEIGHT;

		for (i=0 ; i<LIGHTMAPS_BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (lightmap[texnum].allocated[i+j] >= best)
					break;
				if (lightmap[texnum].allocated[i+j] > best2)
					best2 = lightmap[texnum].allocated[i+j];
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
			lightmap[texnum].allocated[*x + i] = best + h;

		return texnum;
	}

	Host_Error ("AllocBlock: full");
	return 0;
}



/*
========================
GL_CreateSurfaceLightmap
========================
*/
static void GL_CreateSurfaceLightmap (msurface_t *surf)
{
	// store these out so that we don't have to recalculate them every time
#if 0 // Baker: I do this in model loading now
	surf->smax = (surf->extents[0] >> 4) + 1;
	surf->tmax = (surf->extents[1] >> 4) + 1;
#endif

	if (surf->smax > LIGHTMAPS_BLOCK_WIDTH)
		Host_Error ("GL_CreateSurfaceLightmap: smax = %d > LIGHTMAPS_BLOCK_WIDTH (%d)", surf->smax, LIGHTMAPS_BLOCK_WIDTH);
	if (surf->tmax > LIGHTMAPS_BLOCK_HEIGHT)
		Host_Error ("GL_CreateSurfaceLightmap: tmax = %d > LIGHTMAPS_BLOCK_HEIGHT (%d)", surf->tmax, LIGHTMAPS_BLOCK_HEIGHT);

	surf->lightmaptexturenum = AllocBlock (surf->smax, surf->tmax, &surf->light_s, &surf->light_t);

	R_BuildLightMap (surf);
}

/*
================
BuildSurfaceDisplayList -- called at level load time
================
*/
static void BuildSurfaceDisplayList (qmodel_t *curmodel, msurface_t *fa)
{
	medge_t		*pedges = curmodel->edges;

// reconstruct the polygon
	int			lnumverts = fa->numedges;
	glpoly_t	*poly = (glpoly_t *) Hunk_AllocName (sizeof(glpoly_t) + (lnumverts-4) * VERTEXSIZE*sizeof(float), "lm_polys");

	medge_t 	*r_pedge;

	int			i, lindex;
	float		*vec, s, t;
	//
	// draw texture
	//
	poly->next = fa->polys;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i = 0 ; i < lnumverts ; i++)
	{
		lindex = curmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = curmodel->vertexes[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = curmodel->vertexes[r_pedge->v[1]].position;
		}

		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->texture->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->texture->height;

		VectorCopy (vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		//
		// lightmap texture coordinates
		//
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s * 16;
		s += 8;
		s /= LIGHTMAPS_BLOCK_WIDTH * 16; //fa->texinfo->texture->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t*16;
		t += 8;
		t /= LIGHTMAPS_BLOCK_HEIGHT*16; //fa->texinfo->texture->height;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}

	poly->numverts = lnumverts;
}



/*
==================
GL_BuildLightmaps -- called at level load time

Builds the lightmap texture
with all the surfaces from all brush models
==================
*/

void GL_BuildLightmaps_Upload_All_NewMap (void)
{
	char	lightmap_txname[16];
	int		i, j;

	for (i = 0; i < MAX_FITZQUAKE_LIGHTMAPS; i ++) {
		memset (lightmap[i].allocated, 0, sizeof(lightmap[i].allocated));
#if 1 // Clear more things.  Prevents DX8 dynamic light on map change, as far as I can tell.
		lightmap[i].modified = 0;
		lightmap[i].rectchange.l = 0;
		lightmap[i].rectchange.h = 0;
		lightmap[i].rectchange.t = 0;
		lightmap[i].rectchange.w = 0;
		lightmap[i].polys = NULL;
		lightmap[i].texture = NULL;
#endif
	}

	last_lightmap_allocated = 0; // Quakespasm / ericw

	cl.r_framecount = 1; // no dlightcache

	//johnfitz -- null out array (the gltexture objects themselves were already freed by Mod_ClearAll)
	for (i=0; i < MAX_FITZQUAKE_LIGHTMAPS; i++)
		lightmap[i].texture = NULL;
	//johnfitz

	for (j=1 ; j<MAX_FITZQUAKE_MODELS ; j++)
	{
		if (!cl.model_precache[j])
			break;

		if (cl.model_precache[j]->name[0] == '*')
			continue;

		for (i=0 ; i<cl.model_precache[j]->numsurfaces ; i++)
		{
			//johnfitz -- rewritten to use SURF_DRAWTILED instead of the sky/water flags
			if (cl.model_precache[j]->surfaces[i].flags & SURF_DRAWTILED)
				continue;
			GL_CreateSurfaceLightmap (cl.model_precache[j]->surfaces + i);
			BuildSurfaceDisplayList (cl.model_precache[j], cl.model_precache[j]->surfaces + i);
			//johnfitz
		}
	}

	//
	// upload all lightmaps that were filled
	//
	for (i=0; i<MAX_FITZQUAKE_LIGHTMAPS; i++)
	{
		if (!lightmap[i].allocated[0])
			break;		// no more used

		lightmap[i].modified = false;
		lightmap[i].rectchange.l = LIGHTMAPS_BLOCK_WIDTH;
		lightmap[i].rectchange.t = LIGHTMAPS_BLOCK_HEIGHT;
		lightmap[i].rectchange.w = 0;
		lightmap[i].rectchange.h = 0;

		//johnfitz -- use texture manager
		c_snprintf1 (lightmap_txname, "lightmap%03d", i);
		lightmap[i].texture = TexMgr_LoadImage (cl.worldmodel, -1, lightmap_txname, LIGHTMAPS_BLOCK_WIDTH, LIGHTMAPS_BLOCK_HEIGHT,
			 SRC_LIGHTMAP, lightmap[i].lightmaps, "", (src_offset_t)lightmap[i].lightmaps, TEXPREF_LINEAR | TEXPREF_NOPICMIP);
		//johnfitz
	}

	//johnfitz -- warn about exceeding old limits
	if (i >= MAX_WINQUAKE_LIGHTMAPS)
		Con_DWarningLine ("%d lightmaps exceeds standard limit of %d.", i, MAX_WINQUAKE_LIGHTMAPS); // 64

	//johnfitz
}

/*
===============
R_AddDynamicLights
===============
*/
static void R_AddDynamicLights (msurface_t *surf)
{
	int			lnum;
	int			sd, td;
	float		dist, rad, minlight;
	vec3_t		impact, local;
	int			s, t;
	int			i;
	mtexinfo_t	*tex;
	//johnfitz -- lit support via lordhavoc
	float		cred, cgreen, cblue, brightness;
	unsigned	*bl;
	//johnfitz

	tex = surf->texinfo;

	for (lnum=0 ; lnum<MAX_FITZQUAKE_DLIGHTS ; lnum++)
	{
		if ( !(surf->dlightbits[lnum >> 5] & (1U << (lnum & 31))))
			continue;		// not lit by this light

		rad = cl.dlights[lnum].radius;
		dist = DotProduct (cl.dlights[lnum].transformed, surf->plane->normal) -
				surf->plane->dist;
		rad -= fabs(dist);
		minlight = cl.dlights[lnum].minlight;
		if (rad < minlight)
			continue;
		minlight = rad - minlight;

		for (i=0 ; i<3 ; i++)
		{
			impact[i] = cl.dlights[lnum].transformed[i] -
					surf->plane->normal[i]*dist;
		}

		local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];

		//johnfitz -- lit support via lordhavoc
		bl = blocklights;
		cred = cl.dlights[lnum].color.vec3[0] * 256.0f;
		cgreen = cl.dlights[lnum].color.vec3[1] * 256.0f;
		cblue = cl.dlights[lnum].color.vec3[2] * 256.0f;
		//johnfitz
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
				//johnfitz -- lit support via lordhavoc
				{
					brightness = rad - dist;
					bl[0] += (int) (brightness * cred);
					bl[1] += (int) (brightness * cgreen);
					bl[2] += (int) (brightness * cblue);
				}
				bl += 3;
				//johnfitz
			}
		}
	}
}

/*
===============
R_BuildLightMap -- johnfitz -- revised for lit support via lordhavoc

Combine and scale multiple lightmaps into the 8.8 format in blocklights
===============
*/

static void R_BuildLightMap (msurface_t *surf)
{
	int			size = surf->smax * surf->tmax;
	int			block_size = size * BLOCKLITE_BYTES_3;
	surf->cached_dlight = (surf->dlightframe == cl.r_framecount);

	if (cl.worldmodel->lightdata)
	{
		byte		*mylightmap = surf->samples;
	// clear to no light
		memset (&blocklights[0], 0, BLOCKLITE_BLOCK_SIZE /*block_size * sizeof (unsigned int)*/); //johnfitz -- lit support via lordhavoc
	// add all the lightmaps for the surface
		if (mylightmap)
		{
			int			maps, i;
			unsigned	scale;

			for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ; maps++)
			{
				unsigned	*blocklight_fill = blocklights;

				surf->cached_light[maps] = scale =  d_lightstylevalue[surf->styles[maps]];	// 8.8 fraction

				for ( i = 0 ; i < block_size; i++)
					*blocklight_fill++ += *mylightmap++ * scale;
			}
		} // End lightmap
	// add all the dynamic lights
		if (surf->dlightframe == cl.r_framecount)
			R_AddDynamicLights (surf);
	}
	else
	{
	// set to full bright if no light data
		memset (&blocklights[0], 255, block_size * sizeof (unsigned int)); //johnfitz -- lit support via lordhavoc
	}

// bound, invert, and shift
//store:
	{
		int i, j, k, h, t;
//		int r,g,b;
		unsigned	*block_lights_paint=blocklights;
		int			overbright = (gl_overbright.value) ? 1 : 0;
#if 1
		cbool	isstained		=   r_stains.value ? surf->stained : false;
		byte		*stain			=	lightmap[surf->lightmaptexturenum].stainmaps +
										   (surf->light_t * LIGHTMAPS_BLOCK_WIDTH + surf->light_s) * BLOCKLITE_BYTES_3;

#endif

		int stride = (LIGHTMAPS_BLOCK_WIDTH - surf->smax) * LIGHTMAPS_BYTES_4;
		int stain_stride = (LIGHTMAPS_BLOCK_WIDTH - surf->smax) * BLOCKLITE_BYTES_3;
		byte		*base = lightmap[surf->lightmaptexturenum].lightmaps;
		byte		*dest =  base + (surf->light_t * LIGHTMAPS_BLOCK_WIDTH + surf->light_s) * LIGHTMAPS_BYTES_4;

		for (i = 0 ; i < surf->tmax ; i++, dest += stride, stain +=stain_stride)
		{
			for (j=0 ; j< surf->smax ; j++, block_lights_paint +=BLOCKLITE_BYTES_3, dest += LIGHTMAPS_BYTES_4, stain += BLOCKLITE_BYTES_3)
			{
				for (k = 0, h = 2; k < 3; k++, h--) // b then g then r
				{
					t = block_lights_paint[k]; // Baker: Change to 2-k for BGRA format

					switch (overbright)
					{
						case 1:		t = (t >> 8);	break;
						default:	t = (t >> 7);	break;
					}

					// Baker: Fake quick integer division.
					if (isstained && stain[k])
						t = (t * (256 - stain[k])) >> 8;

					dest[k] = CLAMP (0, t, 255);

				} // End k
				dest[3] = 255;
			}  // End j

		} // End i

	} // End store
}

/*
===============
R_UploadLightmap -- johnfitz -- uploads the modified lightmap to opengl if necessary

assumes lightmap texture is already bound
===============
*/
static void R_UploadLightmap_Changed_Region (const msurface_t *surf, int in_lightmapnum)
{
	int lightmapnum = !surf ? in_lightmapnum : surf->lightmaptexturenum;

	if (!lightmap[lightmapnum].modified)
		return;

	lightmap[lightmapnum].modified = false;

	eglTexSubImage2D(
		GL_TEXTURE_2D, 0, 0,
		lightmap[lightmapnum].rectchange.t,
		LIGHTMAPS_BLOCK_WIDTH,
		lightmap[lightmapnum].rectchange.h,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		lightmap[lightmapnum].lightmaps + (lightmap[lightmapnum].rectchange.t * LIGHTMAPS_BLOCK_WIDTH * LIGHTMAPS_BYTES_4)
	);

	// Restore update region to the whole
	lightmap[lightmapnum].rectchange.l = LIGHTMAPS_BLOCK_WIDTH;
	lightmap[lightmapnum].rectchange.t = LIGHTMAPS_BLOCK_HEIGHT;
	lightmap[lightmapnum].rectchange.h = 0;
	lightmap[lightmapnum].rectchange.w = 0;
	rs_dynamiclightmaps++;
}

void R_UploadLightmaps_Modified (void)
{
	int i;

	for (i = 0; i < MAX_FITZQUAKE_LIGHTMAPS; i++)
	{
		if (!lightmap[i].modified)
			continue;

		GL_Bind (lightmap[i].texture); // Texture must be bound
		R_UploadLightmap_Changed_Region (NULL, i);
	}
}

/*
================
R_RebuildAllLightmaps -- johnfitz -- called when gl_overbright gets toggled
================
*/
void sGL_Overbright_f_R_RebuildAllLightmaps (void)
{
	int			i, j;
	qmodel_t		*mod;
	msurface_t	*fa;

	if (!cl.worldmodel) // is this the correct test?
		return;

	//for each surface in each model, rebuild lightmap with new scale
	for (i = 1; i < MAX_FITZQUAKE_MODELS; i++)
	{
		if (!(mod = cl.model_precache[i]))
			continue;
		fa = &mod->surfaces[mod->firstmodelsurface];
		for (j=0; j<mod->nummodelsurfaces; j++, fa++)
		{
			if (fa->flags & SURF_DRAWTILED)
				continue;
			R_BuildLightMap (fa);
		}
	}

	//for each lightmap, upload it
	for (i = 0; i < MAX_FITZQUAKE_LIGHTMAPS; i++)
	{
		if (!lightmap[i].allocated[0])
			break;
		GL_Bind (lightmap[i].texture);
		eglTexSubImage2D (
			GL_TEXTURE_2D, 0, 0,
			0,
			LIGHTMAPS_BLOCK_WIDTH,
			LIGHTMAPS_BLOCK_HEIGHT,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			lightmap[i].lightmaps
		);

	}
}

/*
====================
GL_Overbright_f -- johnfitz
====================
*/
void GL_Overbright_f (cvar_t *var)
{
	sGL_Overbright_f_R_RebuildAllLightmaps ();
}


/*
=============================================================

	STAINMAPS - ADAPTED FROM FTEQW

=============================================================
*/


void Stains_WipeStains_NewMap (void)
{
	int i;
	for (i = 0; i < MAX_FITZQUAKE_LIGHTMAPS; i++)
	{
		memset(lightmap[i].stainmaps, 0, sizeof(lightmap[i].stainmaps));
	}
}

void Stain_Change_f (cvar_t* var)
{
	Stain_FrameSetup_LessenStains (true /* erase! */);
}

// Fade out the stain map data lightmap[x].stainmap until decays to 255 (no stain)
// Needs to occur before
void Stain_FrameSetup_LessenStains (cbool erase_stains)
{
	static		float time;

	if (erase_stains)
	{
		if (!cl.worldmodel)
			return;

			goto erase_go;
	}

	if (!r_stains.value)
		return;

	time += host_frametime_;

	if (time < r_stains_fadetime.value)
		return;

	// Time for stain fading.  Doesn't occur every frame
	time-=r_stains_fadetime.value;

erase_go:
	{
		int			decay_factor	= r_stains_fadeamount.value;
		msurface_t	*surf = cl.worldmodel->surfaces;
		int			i;

		if (erase_stains)
			decay_factor = 999;

		for (i=0 ; i<cl.worldmodel->numsurfaces ; i++, surf++)
		{
			if (surf->stained)
			{
				int			stride			=	(LIGHTMAPS_BLOCK_WIDTH-surf->smax)*BLOCKLITE_BYTES_3;
				byte		*stain			=	lightmap[surf->lightmaptexturenum].stainmaps +
											     (surf->light_t * LIGHTMAPS_BLOCK_WIDTH + surf->light_s) * BLOCKLITE_BYTES_3;
				int			s, t;

				surf->cached_dlight=-1;		// nice hack here...
				surf->stained = false;		// Assume not stained until we know otherwise

				for (t = 0 ; t<surf->tmax ; t++, stain+=stride)
				{
					int smax_times_3 = surf->smax * BLOCKLITE_BYTES_3;
					for (s=0 ; s<smax_times_3 ; s++, stain++)
					{
						if (*stain < decay_factor)
						{
							*stain = 0;		//reset to 0
							continue;
						}

						//eventually decay to 0
						*stain -= decay_factor;
						surf->stained=true;
					}
				}
				// End of surface stained check
			}

			// Next surface

		}
	}
}

// Fill in the stain map data; lightmap[x].stainmap
static void sStain_AddStain_StainNodeRecursive_StainSurf (msurface_t *surf, const vec3_t origin, float tint, float radius /* float *parms*/)
{
	if (surf->lightmaptexturenum < 0)		// Baker: Does this happen?
		return;


	{
		float		dist;
		float		rad = radius;
		float		minlight=0;

		const int			lim = 240;
		mtexinfo_t	*tex = surf->texinfo;
		byte		*stainbase;

		int			s,t, i, sd, td;
		float		change, amm;
		vec3_t		impact, local;

		stainbase = lightmap[surf->lightmaptexturenum].stainmaps;	// Each lightmap has own special slot with a stainmap ... pointer?
		stainbase += (surf->light_t * LIGHTMAPS_BLOCK_WIDTH + surf->light_s) * BLOCKLITE_BYTES_3;

		// Calculate impact
		dist = DotProduct (origin, surf->plane->normal) - surf->plane->dist;
		rad -= fabs(dist);
		minlight = 0;

		if (rad < minlight)	//not hit
			return;

		minlight = rad - minlight;

		for (i=0 ; i<3 ; i++)
		{
			impact[i] = origin[i] - surf->plane->normal[i]*dist;
		}

		local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];


		for (t = 0 ; t<surf->tmax ; t++, stainbase += BLOCKLITE_BYTES_3 * LIGHTMAPS_BLOCK_WIDTH /*stride*/)
		{
			td = local[1] - t*16;  if (td < 0) td = -td;

			for (s=0 ; s<surf->smax ; s++)
			{
				sd = local[0] - s*16;  if (sd < 0) sd = -sd;

				if (sd > td)
					dist = sd + (td>>1);
				else
					dist = td + (sd>>1);
				if (dist < minlight)
				{
					//amm = stainbase[s*3+0]*(rad - dist)*tint;
					amm = (rad - dist);
					change = stainbase[s*3+0] + amm * tint  /* parms[4+x] */;
					stainbase[s*3+0] = CLAMP (0, change, lim);
					stainbase[s*3+1] = CLAMP (0, change, lim);
					stainbase[s*3+2] = CLAMP (0, change, lim);

					surf->stained = true;
				}
			}

		}
	}
	// Mark it as a dynamic light so the lightmap gets updated

	if (surf->stained)
	{
		static int mystainnum;
		surf->cached_dlight=-1;
		if (!surf->stainnum)
			surf->stainnum = mystainnum++;
	}
}


//combination of R_AddDynamicLights and R_MarkLights
static void sStain_AddStain_StainNodeRecursive (mnode_t *node, const vec3_t origin, float tint, float radius)
{
	mplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	unsigned int i;

	if (node->contents < 0)
		return;

	splitplane = node->plane;
	dist = DotProduct (origin, splitplane->normal) - splitplane->dist;

	if (dist > radius)
	{
		sStain_AddStain_StainNodeRecursive (node->children[0], origin, tint, radius);
		return;
	}

	if (dist < -radius)
	{
		sStain_AddStain_StainNodeRecursive (node->children[1], origin, tint, radius);
		return;
	}

// mark the polygons
	surf = cl.worldmodel->surfaces + node->firstsurface;
	for (i = 0 ; i < node->numsurfaces ; i++, surf++)
	{
		if (surf->flags & SURF_DRAWTILED)
			continue;

		sStain_AddStain_StainNodeRecursive_StainSurf(surf, origin, tint, radius);
	}


	sStain_AddStain_StainNodeRecursive (node->children[0], origin, tint, radius);
	sStain_AddStain_StainNodeRecursive (node->children[1], origin, tint, radius);

}




// This builds the stain map
void Stain_AddStain(const vec3_t origin, float tint, float in_radius)
{
	entity_t	*pe;
	int i;
	vec3_t		adjorigin;

	// Slight randomization.  Use 80 of the radius plus or minus 20%
	float		radius = in_radius * .90 + (sin(cl.ctime*3)*.20);

	if (!cl.worldmodel || !r_stains.value)
		return;

	// Stain the world surfaces
	sStain_AddStain_StainNodeRecursive(cl.worldmodel->nodes+cl.worldmodel->hulls[0].firstclipnode, origin, tint, radius);

	//now stain bsp models other than world.

	for (i=1 ; i< cl.num_entities ; i++)	//0 is world...
	{
		pe = &cl_entities[i];
		if (pe->model && pe->model->surfaces == cl.worldmodel->surfaces)
		{
			VectorSubtract (origin, pe->origin, adjorigin);
			if (pe->angles[0] || pe->angles[1] || pe->angles[2])
			{
				vec3_t f, r, u, temp;
				AngleVectors(pe->angles, f, r, u);
				VectorCopy(adjorigin, temp);
				adjorigin[0] = DotProduct(temp, f);
				adjorigin[1] = -DotProduct(temp, r);
				adjorigin[2] = DotProduct(temp, u);
			}

			sStain_AddStain_StainNodeRecursive (pe->model->nodes+pe->model->hulls[0].firstclipnode, adjorigin, tint, radius);
		}
	}
}

#endif // GLQUAKE specific

