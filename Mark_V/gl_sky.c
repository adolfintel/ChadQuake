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
//gl_sky.c



#include "quakedef.h"

#define	MAX_CLIP_VERTS 64

extern	qmodel_t	*loadmodel;
extern	int	rs_skypolys; //for r_speeds readout
extern	int rs_skypasses; //for r_speeds readout
float	skyflatcolor[3];
float	skymins[2][SKYBOX_SIDES_COUNT_6], skymaxs[2][SKYBOX_SIDES_COUNT_6];

char	skybox_name[MAX_QPATH_64]; //name of current skybox, or "" if no skybox

gltexture_t	*skybox_textures[6];
gltexture_t	*solidskytexture, *alphaskytexture;



int skytexorder[SKYBOX_SIDES_COUNT_6] = {0, 2, 1, 3, 4, 5}; //for skybox

vec3_t	skyclip[SKYBOX_SIDES_COUNT_6] = {
	{1,1,0},
	{1,-1,0},
	{0,-1,1},
	{0,1,1},
	{1,0,1},
	{-1,0,1}
};

int	st_to_vec[SKYBOX_SIDES_COUNT_6][3] =
{
	{3,-1,2},
	{-3,1,2},
	{1,3,2},
	{-1,-3,2},
 	{-2,-1,3},		// straight up
 	{2,-1,-3}		// straight down
};

int	vec_to_st[SKYBOX_SIDES_COUNT_6][3] =
{
	{-2,3,1},
	{2,3,-1},
	{1,3,2},
	{-1,3,-2},
	{-2,-1,3},
	{-2,1,-3}
};

//==============================================================================
//
//  INIT
//
//==============================================================================

/*
=============
Sky_LoadTexture

A sky texture is 256*128, with the left side being a masked overlay
==============
*/
void Sky_LoadTexture (texture_t *mt)
{
	char		texturename[MAX_QPATH_64];
	int			i, j, p, r, g, b, count;
	byte		*src;
	static byte	front_data[128*128]; //FIXME: Hunk_Alloc
	static byte	back_data[128*128]; //FIXME: Hunk_Alloc
	unsigned	*rgba;

	// Baker: Warn.
	if (mt->width != 256 || mt->height != 128)
		Con_WarningLinef ("Standard sky texture %s expected to be 256 x 128 but is %d by %d ", mt->name, mt->width, mt->height);

	src = (byte *)mt + mt->offset0;

// extract back layer and upload
	for (i=0 ; i<128 ; i++)
		for (j=0 ; j<128 ; j++)
			back_data[(i*128) + j] = src[i*256 + j + 128];

	c_snprintf2 (texturename, "%s:%s_back", loadmodel->name, mt->name);
	solidskytexture = TexMgr_LoadImage (loadmodel, -1 /*Fitz is reloading from memory*/, texturename, 128, 128, SRC_INDEXED, back_data, "", (src_offset_t)back_data, TEXPREF_NONE);

// extract front layer and upload
	for (i=0 ; i<128 ; i++)
		for (j=0 ; j<128 ; j++)
		{
			front_data[(i*128) + j] = src[i*256 + j];
			if (front_data[(i*128) + j] == 0)
				front_data[(i*128) + j] = 255;
		}

	c_snprintf2 (texturename, "%s:%s_front", loadmodel->name, mt->name);
	alphaskytexture = TexMgr_LoadImage (loadmodel, -1 /*Fitz is reloading from memory*/, texturename, 128, 128, SRC_INDEXED, front_data, "", (src_offset_t)front_data, TEXPREF_ALPHA);

// calculate r_fastsky color based on average of all opaque foreground colors
	r = g = b = count = 0;
	for (i=0 ; i<128 ; i++)
		for (j=0 ; j<128 ; j++)
		{
			p = src[i*256 + j];
			if (p != 0)
			{
				rgba = &vid.d_8to24table[p];
				r += ((byte *)rgba)[0];
				g += ((byte *)rgba)[1];
				b += ((byte *)rgba)[2];
				count++;
			}
		}
	skyflatcolor[0] = (float)r/(count*255);
	skyflatcolor[1] = (float)g/(count*255);
	skyflatcolor[2] = (float)b/(count*255);
}

///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////


/*
==================
Sky_LoadSkyBox
==================
*/
const char	*suf[SKYBOX_SIDES_COUNT_6] = {"rt", "bk", "lf", "ft", "up", "dn"};
void Sky_LoadSkyBox (const char *name)
{
	int			i, mark, width, height;
	char		filename[MAX_OSPATH];
	unsigned	*data;
	cbool nonefound = true;

	if (strcmp(skybox_name, name) == 0)
		return; //no change

	//purge old textures
	for (i = 0; i < SKYBOX_SIDES_COUNT_6; i++)
	{
		if (skybox_textures[i] && skybox_textures[i] != notexture)
			TexMgr_FreeTexture (skybox_textures[i]);
		skybox_textures[i] = NULL;
	}

	//turn off skybox if sky is set to ""
	if (name[0] == 0)
	{
		// If there is a user sky, load that now, otherwise clear
		if (!cl_sky.string[0])
		{
			// No user sky box, so clear the skybox
			skybox_name[0] = 0;
			return;
		}

		// User has a skybox, load that and continue ...
		name = cl_sky.string;
	}


	//load textures
	for (i = 0; i < SKYBOX_SIDES_COUNT_6; i++)
	{
		mark = Hunk_LowMark ();
		c_snprintf2 (filename, "gfx/env/%s%s", name, suf[i]);
		data = Image_Load (filename, &width, &height);
		if (data)
		{
			skybox_textures[i] = TexMgr_LoadImage (cl.worldmodel, -1, filename, width, height, SRC_RGBA, data, filename, 0, TEXPREF_NONE);
			nonefound = false;
		}
		else
		{
			Con_PrintLinef ("Couldn't load %s", filename);
			skybox_textures[i] = notexture;
		}
		Hunk_FreeToLowMark (mark);
	}

	if (nonefound) // go back to scrolling sky if skybox is totally missing
	{
		for (i = 0; i < SKYBOX_SIDES_COUNT_6; i++)
		{
			if (skybox_textures[i] && skybox_textures[i] != notexture)
				TexMgr_FreeTexture (skybox_textures[i]);
			skybox_textures[i] = NULL;
		}
		skybox_name[0] = 0;
			if (name == cl_sky.string)
				Cvar_SetQuick (&cl_sky, ""); // If the user chose it, nuke it
		return;
	}

	c_strlcpy (skybox_name, name);
}




/*
================
Sky_FrameSetup

Baker: Evaluate the sky and make preparations for drawing
================
*/
void Sky_FrameSetup (void)
{
	int			i;
	msurface_t	*s;
	texture_t	*t;

	frame.skyfog = level.is_skyfog > 0 ? level.skyfog : CLAMP(0.0, gl_skyfog.value, 1.0);

	// Baker: Direct3D doesn't have stencil at this time, but we no longer check for
	// vid.direct3d as I have it simple keep 0 for stencilbits in initialization now
	//if (!renderer.gl_stencilbits || vid.direct3d == 9 /*temp disable hack*/)
	if (!renderer.gl_stencilbits DIRECT3D9_STENCIL_DISABLE_BLOCK_OR)
	{
		Sky_DrawSky (); //johnfitz
		return;
	}

	if (!r_drawworld_cheatsafe)
		return;

	for (i = 0 ; i < cl.worldmodel->numtextures ; i++)
	{
		t = cl.worldmodel->textures[i];

		if (!t || !t->texturechain || !(t->texturechain->flags & SURF_DRAWSKY))
			continue;

		for (s = t->texturechain; s; s = s->texturechain)
			if (!s->culled || frame.in_mirror_draw) // Sky frame setup occurs once per frame. Mirror scan completed first.
			{
				frame.has_sky = true;
				return;
			}
	}

	// Check for sky entities
	{
		entity_t	*e;
		msurface_t	*s;
		int			i,j;
		float		dot;

		if (!r_drawentities.value)
			return;

		for (i = 0 ; i < cl.numvisedicts ; i++)
		{
			e = cl.visedicts[i];

			if (e->model->type != mod_brush)
				continue;

			if (R_CullModelForEntity(e))
				continue;

			if (e->alpha == ENTALPHA_ZERO)
				continue;

			for (j = 0, s = &e->model->surfaces[e->model->firstmodelsurface];
				j < e->model->nummodelsurfaces ; j++, s++)
			{
				if (s->flags & SURF_DRAWSKY)
				{
					dot = DotProduct (modelorg, s->plane->normal) - s->plane->dist;
					if (((s->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
						(!(s->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
					{
						frame.has_sky = true;
						return;
					}
				} // End of if SURF_DRAWSKY
			} // End of for each surface loop
		} // End of for each vis edict loop
	} // End of entity check
}

void Sky_Stencil_Draw (void)
{
	int			i;
	msurface_t	*s;
	texture_t	*t;

	// Baker: Direct3D doesn't have stencil at this time, but we no longer check for
	// vid.direct3d as I have it simple keep 0 for stencilbits in initialization now

// Baker: No sky to draw
	if (!level.sky /*|| !frame.has_sky*/)
		return;

	//if (!renderer.gl_stencilbits || vid.direct3d = =9 /*temp disable hack*/)
	if (!renderer.gl_stencilbits DIRECT3D9_STENCIL_DISABLE_BLOCK_OR )
	{
		return;
	}

// Baker: Where drawn (doesn't z-fail), replace with 1
// in the stencil buffer.
	eglClearStencil (0); // Sky must clear to 0
	eglClear(GL_STENCIL_BUFFER_BIT);
	eglStencilFunc (GL_ALWAYS, 1, ~0 );
	eglStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);
	eglEnable (GL_STENCIL_TEST);

// Baker: A no draw pass of the sky brushes, does not even
// write to depth buffer (maybe it should?) that writes
// our stencil overlay.

	eglColorMask (0,0,0,0);
//	eglDepthMask (0);  // Don't write depth to buffer
	eglDisable (GL_TEXTURE_2D);
	for (i=0 ; i<cl.worldmodel->numtextures ; i++)
	{
		t = cl.worldmodel->textures[i];

		if (!t || !t->texturechain || !(t->texturechain->flags & SURF_DRAWSKY))
			continue;

		for (s = t->texturechain; s; s = s->texturechain)
//			if (!s->culled)
			{
				DrawGLPoly (s->polys, 0); // Not here.
				rs_brushpasses++;
			}
	}
	eglEnable (GL_TEXTURE_2D);
	eglColorMask (1,1,1,1);
//	eglDepthMask (1);

// Baker: Keep any pixels where stencil wasn't drawn
// for this drawing pass.
	eglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
	eglStencilFunc( GL_EQUAL, 1, ~0 );

// Baker: Now draw the stencil
	Sky_DrawSky ();

// Turn it off
	eglDisable (GL_STENCIL_TEST);
}

//==============================================================================
//
//  PROCESS SKY SURFS
//
//==============================================================================

/*
=================
Sky_ProjectPoly

update sky bounds
=================
*/
void Sky_ProjectPoly (int nump, vec3_t vecs)
{
	int		i,j;
	vec3_t	v, av;
	float	s, t, dv;
	int		axis;
	float	*vp;

	// decide which face it maps to
	VectorCopy (vec3_origin, v);
	for (i=0, vp=vecs ; i<nump ; i++, vp+=3)
	{
		VectorAdd (vp, v, v);
	}
	av[0] = fabs(v[0]);
	av[1] = fabs(v[1]);
	av[2] = fabs(v[2]);
	if (av[0] > av[1] && av[0] > av[2])
	{
		if (v[0] < 0)
			axis = 1;
		else
			axis = 0;
	}
	else if (av[1] > av[2] && av[1] > av[0])
	{
		if (v[1] < 0)
			axis = 3;
		else
			axis = 2;
	}
	else
	{
		if (v[2] < 0)
			axis = 5;
		else
			axis = 4;
	}

	// project new texture coords
	for (i=0 ; i<nump ; i++, vecs+=3)
	{
		j = vec_to_st[axis][2];
		if (j > 0)
			dv = vecs[j - 1];
		else
			dv = -vecs[-j - 1];

		j = vec_to_st[axis][0];
		if (j < 0)
			s = -vecs[-j -1] / dv;
		else
			s = vecs[j-1] / dv;
		j = vec_to_st[axis][1];
		if (j < 0)
			t = -vecs[-j -1] / dv;
		else
			t = vecs[j-1] / dv;

		if (s < skymins[0][axis])
			skymins[0][axis] = s;
		if (t < skymins[1][axis])
			skymins[1][axis] = t;
		if (s > skymaxs[0][axis])
			skymaxs[0][axis] = s;
		if (t > skymaxs[1][axis])
			skymaxs[1][axis] = t;
	}
}

/*
=================
Sky_ClipPoly
=================
*/
void Sky_ClipPoly (int nump, vec3_t vecs, int stage)
{
	float	*norm;
	float	*v;
	cbool	front, back;
	float	d, e;
	float	dists[MAX_CLIP_VERTS];
	int		sides[MAX_CLIP_VERTS];
	vec3_t	newv[2][MAX_CLIP_VERTS];
	int		newc[2];
	int		i, j;

	if (nump > MAX_CLIP_VERTS-2)
		Host_Error ("Sky_ClipPoly: MAX_CLIP_VERTS");
	if (stage == 6) // fully clipped
	{
		Sky_ProjectPoly (nump, vecs);
		return;
	}

	front = back = false;
	norm = skyclip[stage];
	for (i=0, v = vecs ; i<nump ; i++, v+=3)
	{
		d = DotProduct (v, norm);
		if (d > ON_EPSILON)
		{
			front = true;
			sides[i] = SIDE_FRONT;
		}
		else if (d < ON_EPSILON)
		{
			back = true;
			sides[i] = SIDE_BACK;
		}
		else
			sides[i] = SIDE_ON;
		dists[i] = d;
	}

	if (!front || !back)
	{	// not clipped
		Sky_ClipPoly (nump, vecs, stage+1);
		return;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy (vecs, (vecs+(i*3)) );
	newc[0] = newc[1] = 0;

	for (i=0, v = vecs ; i<nump ; i++, v+=3)
	{
		switch (sides[i])
		{
		case SIDE_FRONT:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			break;
		case SIDE_BACK:
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;
		case SIDE_ON:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		d = dists[i] / (dists[i] - dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{
			e = v[j] + d*(v[j+3] - v[j]);
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// continue
	Sky_ClipPoly (newc[0], newv[0][0], stage+1);
	Sky_ClipPoly (newc[1], newv[1][0], stage+1);
}

/*
================
Sky_ProcessPoly
================
*/
void Sky_ProcessPoly (glpoly_t	*p)
{
	int			i;
	vec3_t		verts[MAX_CLIP_VERTS];

	//draw it
	DrawGLPoly(p, 0);
	rs_brushpasses++;

	//update sky bounds
	if (!gl_fastsky.value)
	{
		for (i=0 ; i<p->numverts ; i++)
			VectorSubtract (p->verts[i], r_origin, verts[i]);
		Sky_ClipPoly (p->numverts, verts[0], 0);
	}
}

/*
================
Sky_ProcessTextureChains -- handles sky polys in world model
================
*/
void Sky_ProcessTextureChains (void)
{
	int			i;
	msurface_t	*s;
	texture_t	*t;

	if (!r_drawworld_cheatsafe)
		return;

	for (i=0 ; i<cl.worldmodel->numtextures ; i++)
	{
		t = cl.worldmodel->textures[i];

		if (!t || !t->texturechain || !(t->texturechain->flags & SURF_DRAWSKY))
			continue;

		for (s = t->texturechain; s; s = s->texturechain)
//			if (!!s->culled)
				Sky_ProcessPoly (s->polys);
	}
}

/*
================
Sky_ProcessEntities -- handles sky polys on brush models
================
*/
void Sky_ProcessEntities (void)
{
	entity_t	*e;
	msurface_t	*s;
	glpoly_t	*p;
	int			i,j,k,mark;
	float		dot;
	cbool	rotated;
	vec3_t		temp, forward, right, up;

	if (!r_drawentities.value)
		return;

	for (i = 0 ; i < cl.numvisedicts ; i++)
	{
		e = cl.visedicts[i];

		if (e->model->type != mod_brush)
			continue;

		if (R_CullModelForEntity(e))
			continue;

		if (e->alpha == ENTALPHA_ZERO)
			continue;

		VectorSubtract (r_refdef.vieworg, e->origin, modelorg);
		if (e->angles[0] || e->angles[1] || e->angles[2])
		{
			rotated = true;
			AngleVectors (e->angles, forward, right, up);
			VectorCopy (modelorg, temp);
			modelorg[0] = DotProduct (temp, forward);
			modelorg[1] = -DotProduct (temp, right);
			modelorg[2] = DotProduct (temp, up);
		}
		else
			rotated = false;

		s = &e->model->surfaces[e->model->firstmodelsurface];

		for (j = 0 ; j < e->model->nummodelsurfaces ; j++, s++)
		{
			if (s->flags & SURF_DRAWSKY)
			{
				dot = DotProduct (modelorg, s->plane->normal) - s->plane->dist;
				if (((s->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
					(!(s->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
				{
					//copy the polygon and translate manually, since Sky_ProcessPoly needs it to be in world space
					mark = Hunk_LowMark();
					p = (glpoly_t *) Hunk_Alloc (sizeof(*s->polys)); //FIXME: don't allocate for each poly
					p->numverts = s->polys->numverts;
					for (k=0; k<p->numverts; k++)
					{
						if (rotated)
						{
							p->verts[k][0] = e->origin[0] + s->polys->verts[k][0] * forward[0]
														  - s->polys->verts[k][1] * right[0]
														  + s->polys->verts[k][2] * up[0];
							p->verts[k][1] = e->origin[1] + s->polys->verts[k][0] * forward[1]
														  - s->polys->verts[k][1] * right[1]
														  + s->polys->verts[k][2] * up[1];
							p->verts[k][2] = e->origin[2] + s->polys->verts[k][0] * forward[2]
														  - s->polys->verts[k][1] * right[2]
														  + s->polys->verts[k][2] * up[2];
						}
						else
							VectorAdd(s->polys->verts[k], e->origin, p->verts[k]);
					}
					Sky_ProcessPoly (p);
					Hunk_FreeToLowMark (mark);
				}
			}
		}
	}
}

//==============================================================================
//
//  RENDER SKYBOX
//
//==============================================================================

/*
==============
Sky_EmitSkyBoxVertex
==============
*/
void Sky_EmitSkyBoxVertex (float s, float t, int axis)
{
	vec3_t		v, b;
	int			j, k;
	float		w, h;

	b[0] = s * gl_farclip.value / sqrt(3.0);
	b[1] = t * gl_farclip.value / sqrt(3.0);
	b[2] = gl_farclip.value / sqrt(3.0);

	for (j=0 ; j<3 ; j++)
	{
		k = st_to_vec[axis][j];
		if (k < 0)
			v[j] = -b[-k - 1];
		else
			v[j] = b[k - 1];
		v[j] += r_origin[j];
	}

	// convert from range [-1,1] to [0,1]
	s = (s+1)*0.5;
	t = (t+1)*0.5;

	// avoid bilerp seam
	w = skybox_textures[skytexorder[axis]]->width;
	h = skybox_textures[skytexorder[axis]]->height;
	s = s * (w-1)/w + 0.5/w;
	t = t * (h-1)/h + 0.5/h;

	t = 1.0 - t;
	eglTexCoord2f (s, t);
	eglVertex3fv (v);
}

/*
==============
Sky_DrawSkyBox

FIXME: eliminate cracks by adding an extra vert on tjuncs
==============
*/
void Sky_DrawSkyBox (void)
{
	int		i;

	for (i = 0; i< SKYBOX_SIDES_COUNT_6; i++)
	{
		if (skymins[0][i] >= skymaxs[0][i] || skymins[1][i] >= skymaxs[1][i])
			continue;

		GL_Bind (skybox_textures[skytexorder[i]]);

#if 1 //FIXME: this is to avoid tjunctions until i can do it the right way
		skymins[0][i] = -1;
		skymins[1][i] = -1;
		skymaxs[0][i] = 1;
		skymaxs[1][i] = 1;
#endif

		eglBegin (GL_QUADS);
		Sky_EmitSkyBoxVertex (skymins[0][i], skymins[1][i], i);
		Sky_EmitSkyBoxVertex (skymins[0][i], skymaxs[1][i], i);
		Sky_EmitSkyBoxVertex (skymaxs[0][i], skymaxs[1][i], i);
		Sky_EmitSkyBoxVertex (skymaxs[0][i], skymins[1][i], i);
		eglEnd ();

		rs_skypolys++;
		rs_skypasses++;

		if (Fog_GetDensity() > 0 && frame.skyfog > 0)
		{
			float *c;

			c = Fog_GetColor(NULL, NULL);
			eglEnable (GL_BLEND);
			eglDisable (GL_TEXTURE_2D);
			eglColor4f (c[0],c[1],c[2], frame.skyfog);

			eglBegin (GL_QUADS);
			Sky_EmitSkyBoxVertex (skymins[0][i], skymins[1][i], i);
			Sky_EmitSkyBoxVertex (skymins[0][i], skymaxs[1][i], i);
			Sky_EmitSkyBoxVertex (skymaxs[0][i], skymaxs[1][i], i);
			Sky_EmitSkyBoxVertex (skymaxs[0][i], skymins[1][i], i);
			eglEnd ();

			eglColor3f (1, 1, 1);
			eglEnable (GL_TEXTURE_2D);
			eglDisable (GL_BLEND);

			rs_skypasses++;
		}
	}
}

//==============================================================================
//
//  RENDER CLOUDS
//
//==============================================================================

/*
==============
Sky_SetBoxVert
==============
*/
void Sky_SetBoxVert (float s, float t, int axis, vec3_t v)
{
	vec3_t		b;
	int			j, k;

	b[0] = s * gl_farclip.value / sqrt(3.0);
	b[1] = t * gl_farclip.value / sqrt(3.0);
	b[2] = gl_farclip.value / sqrt(3.0);

	for (j=0 ; j<3 ; j++)
	{
		k = st_to_vec[axis][j];
		if (k < 0)
			v[j] = -b[-k - 1];
		else
			v[j] = b[k - 1];
		v[j] += r_origin[j];
	}
}

/*
=============
Sky_GetTexCoord
=============
*/
void Sky_GetTexCoord (const vec3_t v, float speed, float *s, float *t)
{
	vec3_t	dir;
	float	length, scroll;

	VectorSubtract (v, r_origin, dir);
	dir[2] *= 3;	// flatten the sphere

	length = dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2];
	length = sqrt (length);
	length = 6*63/length;

	scroll = cl.ctime * speed;
	scroll -= (int)scroll & ~127;

	*s = (scroll + dir[0] * length) * (1.0/128);
	*t = (scroll + dir[1] * length) * (1.0/128);
}

#if 0

/*
===============
Sky_DrawFaceQuad
===============
*/
void Sky_FitzQuake_DrawFaceQuad (glpoly_t *p)
{
	float	s, t;
	float	*v;
	int		i;

	if (renderer.gl_mtexable && gl_skyalpha.value >= 1.0)
	{
		GL_Bind (solidskytexture);
		GL_EnableMultitexture();
		GL_Bind (alphaskytexture);
		eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

		eglBegin (GL_QUADS);
		for (i=0, v=p->verts[0] ; i<4 ; i++, v+=VERTEXSIZE)
		{
			Sky_GetTexCoord (v, 8, &s, &t);
			renderer.GL_MTexCoord2fFunc (renderer.TEXTURE0, s, t);
			Sky_GetTexCoord (v, 16, &s, &t);
			renderer.GL_MTexCoord2fFunc (renderer.TEXTURE1, s, t);
			eglVertex3fv (v);
		}
		eglEnd ();

		GL_DisableMultitexture();

		rs_skypolys++;
		rs_skypasses++;
	}
	else
	{
		GL_Bind (solidskytexture);

		if (gl_skyalpha.value < 1.0)
			eglColor3f (1, 1, 1);

		eglBegin (GL_QUADS);
		for (i=0, v=p->verts[0] ; i<4 ; i++, v+=VERTEXSIZE)
		{
			Sky_GetTexCoord (v, 8, &s, &t);
			eglTexCoord2f (s, t);
			eglVertex3fv (v);
		}

		eglEnd ();


		GL_Bind (alphaskytexture);
		eglEnable (GL_BLEND);

		if (gl_skyalpha.value < 1.0)
			eglColor4f (1, 1, 1, gl_skyalpha.value);

		eglBegin (GL_QUADS);
		for (i=0, v=p->verts[0] ; i<4 ; i++, v+=VERTEXSIZE)
		{
			Sky_GetTexCoord (v, 16, &s, &t);
			eglTexCoord2f (s, t);
			eglVertex3fv (v);
		}
		eglEnd ();

		eglDisable (GL_BLEND);

		rs_skypolys++;
		rs_skypasses += 2;
	}

	if (Fog_GetDensity() > 0 && frame.skyfog > 0)
	{
		float *c;

		c = Fog_GetColor(NULL, NULL);
		eglEnable (GL_BLEND);
		eglDisable (GL_TEXTURE_2D);
		eglColor4f (c[0],c[1],c[2], frame.skyfog);

		eglBegin (GL_QUADS);
		for (i=0, v=p->verts[0] ; i<4 ; i++, v+=VERTEXSIZE)
			eglVertex3fv (v);
		eglEnd ();

		eglColor3f (1, 1, 1);
		eglEnable (GL_TEXTURE_2D);
		eglDisable (GL_BLEND);

		rs_skypasses++;
	}
}

/*
==============
Sky_DrawFace
==============
*/

void Sky_FitzQuake_DrawFace (int axis)
{
	glpoly_t	*p;
	vec3_t		verts[4];
	int			i, j, start;
	float		di,qi,dj,qj;
	vec3_t		vup, vright, temp, temp2;

	Sky_SetBoxVert(-1.0, -1.0, axis, verts[0]);
	Sky_SetBoxVert(-1.0,  1.0, axis, verts[1]);
	Sky_SetBoxVert(1.0,   1.0, axis, verts[2]);
	Sky_SetBoxVert(1.0,  -1.0, axis, verts[3]);

	start = Hunk_LowMark ();
	p = (glpoly_t *) Hunk_Alloc(sizeof(glpoly_t));

	VectorSubtract(verts[2],verts[3],vup);
	VectorSubtract(verts[2],verts[1],vright);

	di = c_max((int)gl_sky_quality.value, 1);
	qi = 1.0 / di;
	dj = (axis < 4) ? di*2 : di; //subdivide vertically more than horizontally on skybox sides
	qj = 1.0 / dj;

	for (i=0; i<di; i++)
	{
		for (j=0; j<dj; j++)
		{
			if (i*qi < skymins[0][axis]/2+0.5 - qi || i*qi > skymaxs[0][axis]/2+0.5 ||
				j*qj < skymins[1][axis]/2+0.5 - qj || j*qj > skymaxs[1][axis]/2+0.5)
				continue;

			//if (i&1 ^ j&1) continue; //checkerboard test
			VectorScale (vright, qi*i, temp);
			VectorScale (vup, qj*j, temp2);
			VectorAdd(temp,temp2,temp);
			VectorAdd(verts[0],temp,p->verts[0]);

			VectorScale (vup, qj, temp);
			VectorAdd (p->verts[0],temp,p->verts[1]);

			VectorScale (vright, qi, temp);
			VectorAdd (p->verts[1],temp,p->verts[2]);

			VectorAdd (p->verts[0],temp,p->verts[3]);

			Sky_FitzQuake_DrawFaceQuad (p);
		}
	}
	Hunk_FreeToLowMark (start);
}
#endif // SKY

// declared this way so that we can pass glVertex3fv as a Sky_DrawFunc
void APIENTRY Sky_MH_DrawMultitexture (const float *v)
{
	float s, t;

	Sky_GetTexCoord (v, 8, &s, &t);
	renderer.GL_MTexCoord2fFunc (renderer.TEXTURE0, s, t);
	Sky_GetTexCoord (v, 16, &s, &t);
	renderer.GL_MTexCoord2fFunc (renderer.TEXTURE1, s, t);
	eglVertex3fv (v);
}


void APIENTRY Sky_MH_DrawSolidLayer (const float *v)
{
	float s, t;

	Sky_GetTexCoord (v, 8, &s, &t);
	eglTexCoord2f (s, t);
	eglVertex3fv (v);
}


void APIENTRY Sky_MH_DrawAlphaLayer (const float *v)
{
	float s, t;

	Sky_GetTexCoord (v, 16, &s, &t);
	eglTexCoord2f (s, t);
	eglVertex3fv (v);
}


void Sky_MH_DrawPass (int axis, void (APIENTRY *Sky_DrawFunc) (const float *))
{
	int i, j;
	vec3_t temp, temp2, vec[4];

	vec3_t		verts[4];
	vec3_t		vup, vright;

	float di = c_max ((int) gl_sky_quality.value, 1);
	float qi = 1.0 / di;
	float dj = (axis < 4) ? di * 2 : di; // subdivide vertically more than horizontally on skybox sides
	float qj = 1.0 / dj;

	Sky_SetBoxVert (-1.0, -1.0, axis, verts[0]);
	Sky_SetBoxVert (-1.0,  1.0, axis, verts[1]);
	Sky_SetBoxVert ( 1.0,  1.0, axis, verts[2]);
	Sky_SetBoxVert ( 1.0, -1.0, axis, verts[3]);

	VectorSubtract (verts[2], verts[3], vup);
	VectorSubtract (verts[2], verts[1], vright);

	eglBegin (GL_QUADS);

	for (i = 0; i < di; i++)
	{
		for (j = 0; j < dj; j++)
		{
			if (i * qi < skymins[0][axis] / 2 + 0.5 - qi || i * qi > skymaxs[0][axis] / 2 + 0.5 ||
				j * qj < skymins[1][axis] / 2 + 0.5 - qj || j * qj > skymaxs[1][axis] / 2 + 0.5)
				continue;

			//if ((i & 1) ^ (j & 1)) continue; //checkerboard test
			VectorScale (vright, qi * i, temp);
			VectorScale (vup, qj * j, temp2);
			VectorAdd (temp, temp2, temp);

			VectorAdd (verts[0], temp, vec[0]);

			VectorScale (vup, qj, temp);
			VectorAdd (vec[0], temp, vec[1]);

			VectorScale (vright, qi, temp);
			VectorAdd (vec[1], temp, vec[2]);

			VectorAdd (vec[0], temp, vec[3]);

			Sky_DrawFunc (vec[0]);
			Sky_DrawFunc (vec[1]);
			Sky_DrawFunc (vec[2]);
			Sky_DrawFunc (vec[3]);

			rs_skypolys++;
		}
	}

	eglEnd ();
	rs_skypasses++;
}


void Sky_MH_DrawFace (int axis)
{
	// now we set up and do batched drawing of the recorded quads
	if (renderer.gl_mtexable && gl_skyalpha.value >= 1.0)
	{
		GL_Bind (solidskytexture);
		GL_EnableMultitexture ();
		GL_Bind (alphaskytexture);
		eglTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

		Sky_MH_DrawPass (axis, Sky_MH_DrawMultitexture);

		GL_DisableMultitexture ();
	}
	else
	{
		GL_Bind (solidskytexture);

		if (gl_skyalpha.value < 1.0)
			eglColor3f (1, 1, 1);

		Sky_MH_DrawPass (axis, Sky_MH_DrawSolidLayer);

		GL_Bind (alphaskytexture);
		eglEnable (GL_BLEND);

		if (gl_skyalpha.value < 1.0)
			eglColor4f (1, 1, 1, gl_skyalpha.value);

		Sky_MH_DrawPass (axis, Sky_MH_DrawAlphaLayer);

		eglDisable (GL_BLEND);
	}

	if (Fog_GetDensity() > 0 && gl_skyfog.value > 0)
	{
		float *c;

		c = Fog_GetColor(NULL, NULL);
		eglEnable (GL_BLEND);
		eglDisable (GL_TEXTURE_2D);
		eglColor4f (c[0],c[1],c[2], CLAMP(0.0, gl_skyfog.value, 1.0));

		Sky_MH_DrawPass (axis, eglVertex3fv);

		eglColor3f (1, 1, 1);
		eglEnable (GL_TEXTURE_2D);
		eglDisable (GL_BLEND);
	}
}

/*
==============
Sky_DrawSkyLayers

draws the old-style scrolling cloud layers
==============
*/
void Sky_DrawSkyLayers (void)
{
	int i;

	if (gl_skyalpha.value < 1.0)
		eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	for (i = 0; i < SKYBOX_SIDES_COUNT_6; i++)
		if (skymins[0][i] < skymaxs[0][i] && skymins[1][i] < skymaxs[1][i])
			Sky_MH_DrawFace (i); // Sky_FitzQuake_DrawFace

	if (gl_skyalpha.value < 1.0)
		eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

/*
==============
Sky_DrawSky

called once per frame before drawing anything else
==============
*/
void Sky_DrawSky (void)
{
	int			i;

	// Baker: If no sky to draw, exit
	if (!level.sky)
		return;

//#ifndef DIRECT3D8_WRAPPER // Baker: DX8 no stencil
// Don't enable this since I don't think we are checking.
// But would be nice to check, wouldn't it?
// 	if (!frame.has_sky)
//		return;
//#endif // DIRECT3D8_WRAPPER // No DX8 stencil

	//in these special render modes, the sky faces are handled in the normal world/brush renderer
	if (r_drawflat_cheatsafe || r_lightmap_cheatsafe )
		return;

	//
	// reset sky bounds
	//
	for (i = 0; i < SKYBOX_SIDES_COUNT_6; i++)
	{
		skymins[0][i] = skymins[1][i] = 9999;
		skymaxs[0][i] = skymaxs[1][i] = -9999;
	}

	//
	// process world and bmodels: draw flat-shaded sky surfs, and update skybounds
	//

	Fog_DisableGFog ();
	eglDisable (GL_TEXTURE_2D);
	if (Fog_GetDensity() > 0)
		eglColor3fv (Fog_GetColor(NULL, NULL));
	else
		eglColor3fv (skyflatcolor);
	Sky_ProcessTextureChains ();
	Sky_ProcessEntities ();
	eglColor3f (1, 1, 1);
	eglEnable (GL_TEXTURE_2D);

	//
	// render slow sky: cloud layers or skybox
	//
	if (!gl_fastsky.value && !(Fog_GetDensity() > 0 && frame.skyfog >= 1))
	{
		eglDepthFunc(GL_GEQUAL);
		eglDepthMask(0);

		if (skybox_name[0])
			Sky_DrawSkyBox ();
		else
			Sky_DrawSkyLayers();

		eglDepthMask(1);
		eglDepthFunc(GL_LEQUAL);
	}

	Fog_EnableGFog ();
}

#endif // GLQUAKE specific
