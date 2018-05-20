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

//r_alias.c -- alias model rendering



#include "quakedef.h"



//up to 16 color translated skins
gltexture_t *playertextures[MAX_COLORMAP_SKINS_1024];



float r_avertexnormals[NUMVERTEXNORMALS_162][3] = {
	#include "anorms.h"
};

vec3_t	shadevector;

extern vec3_t	lightcolor; //johnfitz -- replaces "float shadelight" for lit support

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT_16 16
float	r_avertexnormal_dots[SHADEDOT_QUANT_16][256] =
	#include "anorm_dots.h"
;

extern vec3_t lightspot;

float	*shadedots = r_avertexnormal_dots[0];

float	entalpha; //johnfitz

cbool	overbright; //johnfitz

cbool shading = true; //johnfitz -- if false, disable vertex shading for various reasons (fullbright, r_lightmap, showtris, etc)

/*
=============
GL_DrawAliasFrame_QMB_Flame_And_Shadows -- johnfitz -- rewritten to support colored light, lerping, entalpha, multitexture, and r_drawflat.  Baker: Modified
=============
*/


void GL_DrawAliasFrame_QMB_Flame_And_Shadows (aliashdr_t *paliashdr, lerpdata_t lerpdata, int commands_offset, cbool truncate_flame, float mdl_entalpha)
{
	int				*commands = (int *)((byte *)paliashdr + commands_offset); //paliashdr->commands); // Not always!
    trivertx_t 		*verts2, *verts1 = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	float			blend, iblend;
	cbool			lerping = (lerpdata.pose1 != lerpdata.pose2);
	int				count;
	float			u, v;
	float 			vertcolor[4];

	vertcolor[3] = mdl_entalpha; //never changes, so there's no need to put this inside the loop	

	if (lerping) 
	{
		verts2  = verts1;
		verts1 += lerpdata.pose1 * paliashdr->poseverts;
		verts2 += lerpdata.pose2 * paliashdr->poseverts;
		blend = lerpdata.blend;
		iblend = 1.0f - blend;
	}
	else // poses the same means either 1. the entity has paused its animation, or 2. r_lerpmodels is disabled
	{
		verts1  = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
		verts2  = verts1; // avoid bogus compiler warning
		verts1 += lerpdata.pose1 * paliashdr->poseverts;
		blend = iblend = 0; // avoid bogus compiler warning
	}

	while (1)
	{
		// get the vertex count and primitive type
		count = *commands++;
		if (!count)
			break;		// done

		if (count < 0)
		{
			count = -count;
			eglBegin (GL_TRIANGLE_FAN);
		}
		else
			eglBegin (GL_TRIANGLE_STRIP);

		do
		{
			u = ((float *)commands)[0];
			v = ((float *)commands)[1];

			if (truncate_flame) {
				verts1 = verts1;
				if (verts1->v[2] > 100) {
					commands += 2;			
					if (lerping)
					{
						verts1++;
						verts2++;
					} else verts1++;
					continue;
				}
			}

			if (mtexenabled)
			{
				renderer.GL_MTexCoord2fFunc (renderer.TEXTURE0, u, v);
				renderer.GL_MTexCoord2fFunc (renderer.TEXTURE1, u, v);
			}
			else
				eglTexCoord2f (u, v);

			commands += 2;

			if (shading)
			{
				if (r_drawflat_cheatsafe)
				{
					srand(count * (unsigned int)(src_offset_t) commands);
					eglColor3f (rand()%256/255.0, rand()%256/255.0, rand()%256/255.0);
				}
				else if (lerping)
				{
					vertcolor[0] = (shadedots[verts1->lightnormalindex]*iblend + shadedots[verts2->lightnormalindex]*blend) * lightcolor[0];
					vertcolor[1] = (shadedots[verts1->lightnormalindex]*iblend + shadedots[verts2->lightnormalindex]*blend) * lightcolor[1];
					vertcolor[2] = (shadedots[verts1->lightnormalindex]*iblend + shadedots[verts2->lightnormalindex]*blend) * lightcolor[2];
					eglColor4fv (vertcolor);
				}
				else
				{
					vertcolor[0] = shadedots[verts1->lightnormalindex] * lightcolor[0];
					vertcolor[1] = shadedots[verts1->lightnormalindex] * lightcolor[1];
					vertcolor[2] = shadedots[verts1->lightnormalindex] * lightcolor[2];
					eglColor4fv (vertcolor);
				}
			}

			if (lerping)
			{
				eglVertex3f (verts1->v[0]*iblend + verts2->v[0]*blend,
							verts1->v[1]*iblend + verts2->v[1]*blend,
							verts1->v[2]*iblend + verts2->v[2]*blend);
				verts1++;
				verts2++;
			}
			else
			{
				eglVertex3f (verts1->v[0], verts1->v[1], verts1->v[2]);
				verts1++;
			}
		} while (--count);

		eglEnd ();
	}

	rs_aliaspasses += paliashdr->numtris;
}

//#ifdef SHADOW_VOLUMES
typedef struct lerpedvert_s
{
	float position[3];	// point3
	float color[4];		// color4
	float texcoord[2];	// clampf s, t
} lerpedvert_t;

#define MAX_LERPED_VERTS_3984		MAXALIASVERTS_3984	// Baker: Right?
#define MAX_LERPED_INDEXES_12000	12000

lerpedvert_t r_lerped_verts[MAX_LERPED_VERTS_3984];
unsigned int r_lerped_indexes[MAX_LERPED_INDEXES_12000];

int r_num_lerped_verts;
int r_num_lerped_indexes;

// Baker:	For the moment, nuking true lightpoint knowing it is easy to add back in later.
//			My first goal is to get this functional

void GL_LerpVerts (aliashdr_t *paliashdr, lerpdata_t lerpdata, int commands_offset, vec3_t mdl_scale, vec3_t mdl_scale_origin, float mdl_entalpha)
{
    int				*commands = (int *)((byte *)paliashdr + commands_offset); //paliashdr->commands); // Not always!
	trivertx_t 		*verts2, *verts1 = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	float			blend, iblend;
	cbool			lerping = (lerpdata.pose1 != lerpdata.pose2);
	lerpedvert_t	*lv  = r_lerped_verts;
	unsigned int	*ndx = r_lerped_indexes;

	r_num_lerped_verts = 0;
	r_num_lerped_indexes = 0;


	if (lerping) 
	{
		verts2  = verts1;
		verts1 += lerpdata.pose1 * paliashdr->poseverts;
		verts2 += lerpdata.pose2 * paliashdr->poseverts;
		blend = lerpdata.blend;
		iblend = 1.0f - blend;
	}
	else // poses the same means either 1. the entity has paused its animation, or 2. r_lerpmodels is disabled
	{
		verts1  = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
		verts2  = verts1; // avoid bogus compiler warning
		verts1 += lerpdata.pose1 * paliashdr->poseverts;
		blend = iblend = 0; // avoid bogus compiler warning
	}

	while (1)
	{
		// get the vertex count and primitive type
		int count = *commands++;
		int i;

		if (!count)
			break;		// done

		if (count < 0) // GL_TRIANGLE_FAN
		{
			for (i = 2, count = -count; i < count; i++, ndx += 3, r_num_lerped_indexes += 3)
			{
				ndx[0] = r_num_lerped_verts + 0;
				ndx[1] = r_num_lerped_verts + (i - 1);
				ndx[2] = r_num_lerped_verts + i;
			}
		}
		else // GL_TRIANGLE_STRIP
		{
			for (i = 2; i < count; i++, ndx += 3, r_num_lerped_indexes += 3)
			{
				ndx[0] = r_num_lerped_verts + i - 2;
				ndx[1] = r_num_lerped_verts + ((i & 1) ? i : (i - 1));
				ndx[2] = r_num_lerped_verts + ((i & 1) ? (i - 1) : i);
			}
		}

		do
		{
			lv->texcoord[0] = ((float *)commands)[0];
			lv->texcoord[1] = ((float *)commands)[1];
			commands += 2;

			if (r_drawflat_cheatsafe)
			{
				srand(count * (unsigned int)(src_offset_t) commands);
				lv->color[0] = (rand () % 256) / 255.0f;
				lv->color[1] = (rand () % 256) / 255.0f;
				lv->color[2] = (rand () % 256) / 255.0f;
			}
			else if (lerping)
			{
				lv->color[0] = (shadedots[verts1->lightnormalindex]*iblend + shadedots[verts2->lightnormalindex]*blend) * lightcolor[0];
				lv->color[1] = (shadedots[verts1->lightnormalindex]*iblend + shadedots[verts2->lightnormalindex]*blend) * lightcolor[1];
				lv->color[2] = (shadedots[verts1->lightnormalindex]*iblend + shadedots[verts2->lightnormalindex]*blend) * lightcolor[2];
			}
			else
			{
				lv->color[0] = shadedots[verts1->lightnormalindex] * lightcolor[0];
				lv->color[1] = shadedots[verts1->lightnormalindex] * lightcolor[1];
				lv->color[2] = shadedots[verts1->lightnormalindex] * lightcolor[2];
			}

			lv->color[3] = mdl_entalpha;

			if (lerping)
			{
				lv->position[0] = verts1->v[0] * iblend + verts2->v[0] * blend;
				lv->position[1] = verts1->v[1] * iblend + verts2->v[1] * blend;
				lv->position[2] = verts1->v[2] * iblend + verts2->v[2] * blend;
				verts1++;
				verts2++;
			}
			else
			{
				lv->position[0] = verts1->v[0];
				lv->position[1] = verts1->v[1];
				lv->position[2] = verts1->v[2];
				verts1++;
			}

			lv->position[0] = lv->position[0] * mdl_scale[0] + mdl_scale_origin[0];
			lv->position[1] = lv->position[1] * mdl_scale[1] + mdl_scale_origin[1];
			lv->position[2] = lv->position[2] * mdl_scale[2] + mdl_scale_origin[2];

			r_num_lerped_verts++;
			lv++;
		} while (--count);
	}
}

// This function prototype needs to match GL_DrawAliasFrame_QMB_Flame_And_Shadows
// So we can transparently reroute QMB flame to here
void GL_DrawAliasFrame_New (aliashdr_t *paliashdr, lerpdata_t lerpdata, int unused_int, cbool unused_cbool, float unused_float)
{
	int		*commands;
	int		count;
	lerpedvert_t *lv;

	commands = (int *)((byte *)paliashdr + paliashdr->commands);
	lv = r_lerped_verts;

	while (1)
	{
		// get the vertex count and primitive type
		count = *commands++;

		if (!count)
			break;		// done

		if (count < 0)
		{
			count = -count;
			eglBegin (GL_TRIANGLE_FAN);
		}
		else eglBegin (GL_TRIANGLE_STRIP);

		do
		{
			if (mtexenabled)
			{
				renderer.GL_MTexCoord2fFunc (renderer.TEXTURE0, lv->texcoord[0], lv->texcoord[1]);
				renderer.GL_MTexCoord2fFunc (renderer.TEXTURE1, lv->texcoord[0], lv->texcoord[1]);
			}
			else eglTexCoord2fv (lv->texcoord);

			if (shading)
				eglColor4fv (lv->color);

			eglVertex3fv (lv->position);

			commands += 2;
			lv++;
		} while (--count);

		eglEnd ();
	}

	rs_aliaspasses += paliashdr->numtris;
}


typedef struct edgeDef_s
{
	int		i2;
	int		facing;
} edgeDef_t;

#define	MAX_EDGE_DEFS_32	32

static	edgeDef_t	edgeDefs[MAX_LERPED_VERTS_3984][MAX_EDGE_DEFS_32];
static	int			numEdgeDefs[MAX_LERPED_VERTS_3984];
static	int			facing[MAX_LERPED_INDEXES_12000 / 3];

void R_AddEdgeDef (int i1, int i2, int facing)
{
	int	c = numEdgeDefs[i1];

	if (c == MAX_EDGE_DEFS_32)
	{
		// overflow
		return;
	}

	edgeDefs[i1][c].i2 = i2;
	edgeDefs[i1][c].facing = facing;

	numEdgeDefs[i1]++;
}

void R_RenderShadowEdges (void)
{
	int		i;
	int		c, c2;
	int		j, k;
	int		i2;
	int		hit[2];

	// an edge is NOT a silhouette edge if its face doesn't face the light,
	// or if it has a reverse paired edge that also faces the light.
	// A well behaved polyhedron would have exactly two faces for each edge,
	// but lots of models have dangling edges or overfanned edges
	eglBegin (GL_QUADS);

	for (i = 0; i < r_num_lerped_verts; i++)
	{
		c = numEdgeDefs[i];

		for (j = 0; j < c; j++)
		{
			if (!edgeDefs[i][j].facing)
				continue;

			hit[0] = 0;
			hit[1] = 0;

			i2 = edgeDefs[i][j].i2;
			c2 = numEdgeDefs[i2];

			for (k = 0; k < c2; k++)
			{
				if (edgeDefs[i2][k].i2 == i)
				{
					hit[edgeDefs[i2][k].facing]++;
				}
			}

			// if it doesn't share the edge with another front facing
			// triangle, it is a sil edge
			if (hit[1] == 0)
			{
				eglVertex3fv (r_lerped_verts[i + r_num_lerped_verts].position);
				eglVertex3fv (r_lerped_verts[i2 + r_num_lerped_verts].position);
				eglVertex3fv (r_lerped_verts[i2].position);
				eglVertex3fv (r_lerped_verts[i].position);
			}
		}
	}

	eglEnd ();
}


void RB_ShadowTessEnd (lerpdata_t *lerpdata)
{
	int		i;
	int		numTris;
	vec3_t	lightDir = {400, 0, 400}; // this mimics the approximate angle of classic GLQuake shadows

	// SHADOW_VOLUME
	if (lerpdata->angles[0] || lerpdata->angles[1] || lerpdata->angles[2])
	{
		vec3_t forward, right, up, temp;

		AngleVectors (lerpdata->angles, forward, right, up);
		VectorCopy (lightDir, temp);

		lightDir[0] = DotProduct (temp, forward);
		lightDir[1] = -DotProduct (temp, right);
		lightDir[2] = DotProduct (temp, up);
	}

	VectorNormalize (lightDir);

	// project vertexes away from light direction
	for (i = 0; i < r_num_lerped_verts; i++)
		VectorMA (r_lerped_verts[i].position, -512, lightDir, r_lerped_verts[r_num_lerped_verts + i].position);

	// decide which triangles face the light
	memset (numEdgeDefs, 0, 4 * r_num_lerped_verts);

	numTris = r_num_lerped_indexes / 3;

	for (i = 0; i < numTris; i++ )
	{
		int		i1, i2, i3;
		vec3_t	d1, d2, normal;
		float	*v1, *v2, *v3;
		float	d;

		i1 = r_lerped_indexes[i * 3 + 0];
		i2 = r_lerped_indexes[i * 3 + 1];
		i3 = r_lerped_indexes[i * 3 + 2];

		v1 = r_lerped_verts[i1].position;
		v2 = r_lerped_verts[i2].position;
		v3 = r_lerped_verts[i3].position;

		VectorSubtract (v2, v1, d1);
		VectorSubtract (v3, v1, d2);
		VectorCrossProduct (d1, d2, normal);

		d = DotProduct (normal, lightDir);

		if (d > 0)
			facing[i] = 1;
		else facing[i] = 0;

		// create the edges
		R_AddEdgeDef (i1, i2, facing[i]);
		R_AddEdgeDef (i2, i3, facing[i]);
		R_AddEdgeDef (i3, i1, facing[i]);
	}

	// draw the silhouette edges
	eglEnable (GL_CULL_FACE);
	eglColorMask (GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	eglDepthMask (GL_FALSE);

	eglEnable (GL_STENCIL_TEST);
	eglStencilFunc (GL_ALWAYS, 1, 255);

	// because fitzquake flipped the front-face and cull, we need to do the same here
	// we could use separate stencil here for a slightly more sensible one-pass draw which would be easier on the CPU
	eglCullFace (GL_FRONT);
	eglStencilOp (GL_KEEP, GL_KEEP, GL_INCR);

	R_RenderShadowEdges ();

	eglCullFace (GL_BACK);
	eglStencilOp (GL_KEEP, GL_KEEP, GL_DECR);

	R_RenderShadowEdges ();

	// reenable writing to the color buffer
	eglColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	eglDepthMask (GL_TRUE);
	eglDisable (GL_STENCIL_TEST);
	eglCullFace (GL_BACK);
}

/*
=================
RB_ShadowFinish

Darken everything that is in a shadow volume.
We have to delay this until everything has been shadowed,
because otherwise shadows from different body parts would
overlap and double darken.
=================
*/
void RB_ShadowVolumeBegin (void)
{
	if (gl_shadows.value < 3 || !r_drawentities.value || r_drawflat_cheatsafe || r_lightmap_cheatsafe) // 3 = SHADOW VOLUMES
		return;

	// Baker: SHADOW_VOLUME.  
	eglClearStencil (0); // Clear to 1.  And get out!
	eglClear(GL_STENCIL_BUFFER_BIT);
	frame.in_shadow_volume_draw = true;
}

void RB_ShadowVolumeFinish (void)
{
	// SHADOW_VOLUME
	eglEnable (GL_STENCIL_TEST);
	eglStencilFunc (GL_NOTEQUAL, 0, 255);

	eglDisable (GL_CULL_FACE);
	eglDisable (GL_TEXTURE_2D);

	eglLoadIdentity ();

	eglColor3f (0.6f, 0.6f, 0.6f);
	eglEnable (GL_BLEND);
	eglBlendFunc (GL_DST_COLOR, GL_ZERO);
	eglDepthMask (1);

	eglBegin (GL_QUADS);
	eglVertex3f (-100, 100, -10);
	eglVertex3f (100, 100, -10);
	eglVertex3f (100, -100, -10);
	eglVertex3f (-100, -100, -10);
	eglEnd ();

	eglColor4f (1, 1, 1, 1);
	eglDisable (GL_STENCIL_TEST);
	eglEnable (GL_CULL_FACE);
	eglEnable (GL_TEXTURE_2D);
	eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	eglDisable (GL_BLEND);
	frame.in_shadow_volume_draw = false;
}


/*
=================
R_SetupAliasLighting -- johnfitz -- broken out from R_DrawAliasModel and rewritten
=================
*/
void R_SetupAliasLighting (entity_t	*e)
{
	vec3_t		dist;
	float		add;
	int			i;

	R_LightPoint (e->origin);

	//add dlights
	for (i=0 ; i<MAX_FITZQUAKE_DLIGHTS ; i++)
	{
		if (cl.dlights[i].die >= cl.time)
		{
			VectorSubtract (currententity->origin, cl.dlights[i].origin, dist);
			add = cl.dlights[i].radius - VectorLength(dist);
			if (add > 0)
				VectorMA (lightcolor, add, cl.dlights[i].color.vec3, lightcolor);
		}
	}

	// minimum light value on gun (24)
	if (e == &cl.viewent_gun)
	{
		add = 72.0f - (lightcolor[0] + lightcolor[1] + lightcolor[2]);
		if (add > 0.0f)
		{
			lightcolor[0] += add / 3.0f;
			lightcolor[1] += add / 3.0f;
			lightcolor[2] += add / 3.0f;
		}
	}

	// minimum light value on players (8)
	if (currententity > cl_entities && currententity <= cl_entities + cl.maxclients)
	{
		add = 24.0f - (lightcolor[0] + lightcolor[1] + lightcolor[2]);
		if (add > 0.0f)
		{
			lightcolor[0] += add / 3.0f;
			lightcolor[1] += add / 3.0f;
			lightcolor[2] += add / 3.0f;
		}
	}

	// clamp lighting so it doesn't overbright as much (96)
	if (overbright)
	{
		add = 288.0f / (lightcolor[0] + lightcolor[1] + lightcolor[2]);
		if (add < 1.0f)
			VectorScale(lightcolor, add, lightcolor);
	}

	//hack up the brightness when fullbrights but no overbrights (256)
	if (gl_fullbrights.value && !gl_overbright_models.value)
		if (e->model->modelflags & MOD_FBRIGHTHACK)
		{
			lightcolor[0] = 256.0f;
			lightcolor[1] = 256.0f;
			lightcolor[2] = 256.0f;
		}

	shadedots = r_avertexnormal_dots[((int)(e->angles[1] * (SHADEDOT_QUANT_16 / 360.0))) & (SHADEDOT_QUANT_16 - 1)];
	VectorScale(lightcolor, 1.0f / 200.0f, lightcolor);
}

/*
=================
R_DrawAliasModel -- johnfitz -- almost completely rewritten
=================
*/


typedef void (*GL_DrawAliasFrame_Fn_t) (aliashdr_t *paliashdr, lerpdata_t lerpdata, int unused_int, cbool unused_cbool, float unused_float);

void R_DrawAliasModel (entity_t *e)
{
	aliashdr_t				*paliashdr = (aliashdr_t *)Mod_Extradata (e->model);
	GL_DrawAliasFrame_Fn_t	GL_DrawAliasFrame =	GL_DrawAliasFrame_New;
	int						anim, skinnum;
	gltexture_t				*tx, *fb = NULL;
	lerpdata_t				lerpdata;
	vec3_t					mdl_scale;
	vec3_t					mdl_scale_origin;
	
	int						command_offset = paliashdr->commands;
	cbool					did_set_texture_matrix = false;
	cbool					combine_pass_engine_issue_dx9 = 0;

	//
	// setup pose/lerp data -- do it first so we don't miss updates due to culling
	//
	// paliashdr = (aliashdr_t *)Mod_Extradata (e->model);  // moved up about 12 lines
	R_SetupAliasFrame (paliashdr, e->frame, &lerpdata);
	R_SetupEntityTransform (e, &lerpdata);

	//
	// cull it
	//
	if (e != &cl.viewent_gun && R_CullModelForEntity(e))
		return;

	//
	// transform it
	//
    eglPushMatrix ();
	R_RotateForEntity (lerpdata.origin, lerpdata.angles);

	// Special handling of view model to keep FOV from altering look.  Pretty good.  Not perfect but rather close.
	if (e == &cl.viewent_gun && r_viewmodel_fov.value)
	{
		float scale = 1.0f / tan (Degree_To_Radians (scr_fov.value / 2.0f) ) * r_viewmodel_fov.value / 90.0f; // Reverse out fov and do fov we want

		if (r_viewmodel_size.value)
			scale *= CLAMP (0.5, r_viewmodel_size.value, 2);

		VectorSet (mdl_scale_origin,	paliashdr->scale_origin[0] * scale, paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
		VectorSet (mdl_scale,			paliashdr->scale[0] * scale, paliashdr->scale[1], paliashdr->scale[2]);
	}
	else
	{
		VectorSet (mdl_scale_origin,	paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
		VectorSet (mdl_scale,			paliashdr->scale[0], paliashdr->scale[1], paliashdr->scale[2]);
	}

	if (e->is_fake_frame0) {
		// Relic.  QMB flame.  It needs the old draw to chop the verts.
		eglTranslatef (mdl_scale_origin[0], mdl_scale_origin[1], mdl_scale_origin[2]);
		eglScalef	  (mdl_scale[0], mdl_scale[1], mdl_scale[2]);
		GL_DrawAliasFrame = GL_DrawAliasFrame_QMB_Flame_And_Shadows; // Rewire
	}

	//
	// random stuff
	//
	if (gl_smoothmodels.value && !r_drawflat_cheatsafe)
		eglShadeModel (GL_SMOOTH);
	if (gl_affinemodels.value)
		eglHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	overbright = gl_overbright_models.value;
	shading = true;

	//
	// set up for alpha blending
	//
	if (r_drawflat_cheatsafe || r_lightmap_cheatsafe) //no alpha in drawflat or lightmap mode
		entalpha = 1;
	else
		entalpha = ENTALPHA_DECODE(e->alpha);
	if (entalpha == 0)
	{
		tx = NULL; // Let's us know tx was never bound at all. (nor assigned!)
		goto cleanup;
	}

	if (entalpha < 1)
	{
#if 0
		// I cannot recall why I had done this.  If I recall the rocket launcher looked dumb.
		// If it does become a problem, we can use Spike's draw with no effect trick (only writes to depth buffer)
		// and then draw with depth must equal as a last resort.
		// Testing is sadly not showing this is needed --- I wish I knew the issue.
		if (e != &cl.viewent_gun /* if entity isn't the player's gun don't depth mask why?*/) // Baker: If we don't do this, looks real stupid			e == &cl.viewent_gun 
			eglDepthMask(GL_FALSE);
#endif
		// cl.viewent_gun is gun.  cl.viewentity_player is  player.
		// 
		eglEnable(GL_BLEND);
	}

	//
	// set up lighting
	//
	rs_aliaspolys += paliashdr->numtris;
	R_SetupAliasLighting (e);

	//
	// set up textures
	//
	anim = (int)(cl.ctime*10) & 3;
	
	skinnum = e->skinnum;
	if ((skinnum >= paliashdr->numskins) || (skinnum < 0))
	{
		Con_DPrintLinef ("R_DrawAliasModel: no such skin # %d", skinnum);
		skinnum = 0;
	}
	tx = paliashdr->gltextures[skinnum][anim];
	fb = paliashdr->fbtextures[skinnum][anim];	

	if (e->colormap && !gl_nocolors.value && e->coloredskin)
		tx = e->coloredskin;
	if (!gl_fullbrights.value)
		fb = NULL;

// Here we go
	if (entalpha < 1 && vid.direct3d == 8 && !renderer.gl_texture_env_combine) {
		fb = NULL; // Works fine now!  Oct 22 2016 - formerly overbright = false, but that didn't honor alpha.  This affects mainly Direct3D
		overbright = false; // Added because of .alpha was not very transparent
	}

	if (!tx)
		tx = whitetexture; //Baker: Part of missing skins fix

// Baker: MH's tip to use texture matrix
	GL_DisableMultitexture();

	if (tx->flags & TEXPREF_ALPHA) // EF_ALPHA_MASKED_MDL
		eglEnable (GL_ALPHA_TEST);

	//
	// Baker: Command set improvisation for DX8
	//

	if (vid.direct3d == 8) {
		if (tx->source_format != SRC_RGBA)
			command_offset = paliashdr->commands_d3d8_no_external_skins;
		// DX8 must not hit the next block because it has no texture mode support
	}
	else if (tx->source_format != SRC_RGBA) // DX8 must not do this. No texture matrix support.
	{
		float width_scale = (float) tx->source_width / tx->width;
		float height_scale = (float) tx->source_height / tx->height;
		eglMatrixMode (GL_TEXTURE);
		eglLoadIdentity ();
		eglScalef (width_scale, height_scale, 1.0f);
		if (fb)
		{
			float fbwidth_scale = (float) fb->source_width / fb->width;
			float fbheight_scale = (float) fb->source_height / fb->height;
			GL_EnableMultitexture();
			eglMatrixMode (GL_TEXTURE);
			eglLoadIdentity ();
			eglScalef (fbwidth_scale, fbheight_scale, 1.0f);
			GL_DisableMultitexture();
		}
		eglMatrixMode (GL_MODELVIEW);
		did_set_texture_matrix = true;
	}

	// pre-lerp all verts so that we can reuse the data over multiple passes
	GL_LerpVerts (paliashdr, lerpdata, command_offset, mdl_scale, mdl_scale_origin, entalpha);

	//
	// draw it
	//
	if (r_drawflat_cheatsafe)
	{
		eglDisable (GL_TEXTURE_2D);
		GL_DrawAliasFrame (paliashdr, lerpdata, command_offset, e->is_fake_frame0, entalpha);
		eglEnable (GL_TEXTURE_2D);
		srand((int) (cl.ctime * 1000)); //restore randomness
	}
	else if (r_fullbright_cheatsafe)
	{
		GL_Bind (tx);
		shading = false;
		eglColor4f(1,1,1,entalpha);
		GL_DrawAliasFrame (paliashdr, lerpdata, command_offset, e->is_fake_frame0, entalpha);
		if (fb)
		{
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			GL_Bind(fb);
			eglEnable(GL_BLEND);
			eglBlendFunc (GL_ONE, GL_ONE);
			eglDepthMask(GL_FALSE);
			eglColor3f(entalpha,entalpha,entalpha);
			Fog_StartAdditive ();
			GL_DrawAliasFrame (paliashdr, lerpdata, command_offset, e->is_fake_frame0, entalpha);
			Fog_StopAdditive ();
			eglDepthMask(GL_TRUE);
			eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			eglDisable(GL_BLEND);
		}
	}
	else if (r_lightmap_cheatsafe)
	{
		eglDisable (GL_TEXTURE_2D);
		shading = false;
		eglColor3f(1,1,1);
		GL_DrawAliasFrame (paliashdr, lerpdata, command_offset, e->is_fake_frame0, entalpha);
		eglEnable (GL_TEXTURE_2D);
	}
	else if (overbright)
	{
		if  (renderer.gl_texture_env_combine && renderer.gl_mtexable && renderer.gl_texture_env_add && fb) //case 1: everything in one pass
		{
			GL_Bind (tx);
			eglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			eglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			eglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
			eglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PRIMARY_COLOR);
			eglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 2.0f);
			GL_EnableMultitexture(); // selects TEXTURE1
			GL_Bind (fb);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
			eglEnable(GL_BLEND);
			GL_DrawAliasFrame (paliashdr, lerpdata, command_offset, e->is_fake_frame0, entalpha);
			eglDisable(GL_BLEND);
			GL_DisableMultitexture();
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		}
		else if (renderer.gl_texture_env_combine) //case 2: overbright in one pass, then fullbright pass
		{
		// first pass
			GL_Bind(tx);
			eglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			eglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			eglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
			eglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PRIMARY_COLOR);
			eglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 2.0f);
			GL_DrawAliasFrame (paliashdr, lerpdata, command_offset, e->is_fake_frame0, entalpha);
			eglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0f);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		// second pass
			if (fb)
			{
				eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				GL_Bind(fb);
				eglEnable(GL_BLEND);
				eglBlendFunc (GL_ONE, GL_ONE);
				eglDepthMask(GL_FALSE);
				shading = false;
				eglColor3f(entalpha,entalpha,entalpha);
				Fog_StartAdditive ();
				GL_DrawAliasFrame (paliashdr, lerpdata, command_offset, e->is_fake_frame0, entalpha);
				Fog_StopAdditive ();
				eglDepthMask(GL_TRUE);
				eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				eglDisable(GL_BLEND);
				eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			}
		}
		else //case 3: overbright in two passes, then fullbright pass
		{

		// first pass
			GL_Bind(tx);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			GL_DrawAliasFrame (paliashdr, lerpdata, command_offset, e->is_fake_frame0, entalpha);
		// second pass -- additive with black fog, to double the object colors but not the fog color
			eglEnable(GL_BLEND);
			eglBlendFunc (GL_ONE, GL_ONE);
			eglDepthMask(GL_FALSE);
			Fog_StartAdditive ();
			GL_DrawAliasFrame (paliashdr, lerpdata, command_offset, e->is_fake_frame0, entalpha);
			Fog_StopAdditive ();
			eglDepthMask(GL_TRUE);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			eglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			eglDisable(GL_BLEND);
		// third pass
			if (fb)
			{
				eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				GL_Bind(fb);
				eglEnable(GL_BLEND);
				eglBlendFunc (GL_ONE, GL_ONE);
				eglDepthMask(GL_FALSE);
				shading = false;
				eglColor3f(entalpha,entalpha,entalpha);
				Fog_StartAdditive ();
				GL_DrawAliasFrame (paliashdr, lerpdata, command_offset, e->is_fake_frame0, entalpha);
				Fog_StopAdditive ();
				eglDepthMask(GL_TRUE);
				eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				eglDisable(GL_BLEND);
				eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			}
		}
	}
	else
	{
		if (renderer.gl_mtexable && renderer.gl_texture_env_add && fb) //case 4: fullbright mask using multitexture
		{
			GL_DisableMultitexture(); // selects TEXTURE0
			GL_Bind (tx);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			GL_EnableMultitexture(); // selects TEXTURE1
			GL_Bind (fb);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
			eglEnable(GL_BLEND);
			GL_DrawAliasFrame (paliashdr, lerpdata, command_offset, e->is_fake_frame0, entalpha);
			eglDisable(GL_BLEND);
			GL_DisableMultitexture();
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		}
		else //case 5: fullbright mask without multitexture
		{
		// first pass
			GL_Bind(tx);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			GL_DrawAliasFrame (paliashdr, lerpdata, command_offset, e->is_fake_frame0, entalpha);
		// second pass
			if (fb)
			{
				GL_Bind(fb);
				eglEnable(GL_BLEND);
				eglBlendFunc (GL_ONE, GL_ONE);
				eglDepthMask(GL_FALSE);
				shading = false;
				eglColor3f(entalpha,entalpha,entalpha);
				Fog_StartAdditive ();
				GL_DrawAliasFrame (paliashdr, lerpdata, command_offset, e->is_fake_frame0, entalpha);
				Fog_StopAdditive ();
				eglDepthMask(GL_TRUE);
				eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				eglDisable(GL_BLEND);
			}
		}
	}

cleanup:
	if (tx->flags & TEXPREF_ALPHA) // EF_ALPHA_MASKED_MDL
		eglDisable (GL_ALPHA_TEST);

	eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	eglHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	eglShadeModel (GL_FLAT);
	eglDepthMask(GL_TRUE);
	eglDisable(GL_BLEND);
	eglColor3f(1,1,1);

	//
	// Shadow volumes - This excludes DX8 which does not have stencil.
	// 
	
	do {
		if (!frame.in_shadow_volume_draw)			break; // We don't have shadows activated
		if (entalpha < 1)							break; // Alpha entities don't get shadows
		if (e == &cl.viewent_gun)					break; // Gun does not get shadow
		if (e->model->modelflags & MOD_NOSHADOW)	break; // Model indicates that it does not shadow (flame, lightning bolt or something)
		
		// SHADOW_VOLUME
		RB_ShadowTessEnd (&lerpdata); 
	} while (0);

	eglPopMatrix ();


	switch (did_set_texture_matrix) {
	case false:
		GL_DisableMultitexture (); 
		break;

	case true:
		if (fb) {
			GL_EnableMultitexture ();
			eglMatrixMode (GL_TEXTURE);
			eglLoadIdentity ();
		}
		
		GL_DisableMultitexture ();
		eglMatrixMode (GL_TEXTURE);
		eglLoadIdentity ();
		
		eglMatrixMode (GL_MODELVIEW);
	} 
}

//johnfitz -- values for shadow matrix
#define SHADOW_SKEW_X -0.7 //skew along x axis. -0.7 to mimic glquake shadows
#define SHADOW_SKEW_Y 0 //skew along y axis. 0 to mimic glquake shadows
#define SHADOW_VSCALE 0 //0=completely flat
#define SHADOW_HEIGHT 0.1 //how far above the floor to render the shadow
//johnfitz

/*
=============
GL_DrawAliasShadow -- johnfitz -- rewritten

TODO: orient shadow onto "lightplane" (a global mplane_t*)
=============
*/
void GL_DrawAliasShadow (entity_t *e)
{
	float	shadowmatrix[16] = {1,				0,				0,				0,
								0,				1,				0,				0,
								SHADOW_SKEW_X,	SHADOW_SKEW_Y,	SHADOW_VSCALE,	0,
								0,				0,				SHADOW_HEIGHT,	1};
	float		lheight;
	aliashdr_t	*paliashdr;
	lerpdata_t	lerpdata;

//	if (R_CullModelForEntity(e))  // Baker: Just because entity is culled doesn't mean shadow is!
//		return;

	if (e == &cl.viewent_gun || e->model->modelflags & MOD_NOSHADOW)
		return;

	entalpha = ENTALPHA_DECODE(e->alpha);
	if (entalpha == 0) return;

	paliashdr = (aliashdr_t *)Mod_Extradata (e->model);
	R_SetupAliasFrame (paliashdr, e->frame, &lerpdata);
	R_SetupEntityTransform (e, &lerpdata);
	R_LightPoint (e->origin);
	lheight = currententity->origin[2] - lightspot[2];

// set up matrix
    eglPushMatrix ();
	eglTranslatef (lerpdata.origin[0],  lerpdata.origin[1],  lerpdata.origin[2]);
	eglTranslatef (0,0,-lheight);
	eglMultMatrixf (shadowmatrix);
	eglTranslatef (0,0,lheight);
	eglRotatef (lerpdata.angles[1],  0, 0, 1);
	eglRotatef (-lerpdata.angles[0],  0, 1, 0);
	eglRotatef (lerpdata.angles[2],  1, 0, 0);
	eglTranslatef (paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
	eglScalef (paliashdr->scale[0], paliashdr->scale[1], paliashdr->scale[2]);

// draw it
	eglDepthMask(GL_FALSE);
	eglEnable (GL_BLEND);
	GL_DisableMultitexture ();
	eglDisable (GL_TEXTURE_2D);
	shading = false;
	eglColor4f(0,0,0, gl_shadows.value > 1 ? entalpha * 1 : entalpha * 0.5);
	GL_DrawAliasFrame_QMB_Flame_And_Shadows (paliashdr, lerpdata, paliashdr->commands, false, entalpha /*I think entalpha*/);
	eglEnable (GL_TEXTURE_2D);
	eglDisable (GL_BLEND);
	eglDepthMask(GL_TRUE);


//clean up
	eglPopMatrix ();


}

/*
=================
R_DrawAliasModel_ShowTris -- johnfitz
=================
*/
void R_DrawAliasModel_ShowTris (entity_t *e)
{
	aliashdr_t	*paliashdr;
	lerpdata_t	lerpdata;

	if (R_CullModelForEntity(e))
		return;

	paliashdr = (aliashdr_t *)Mod_Extradata (e->model);
	R_SetupAliasFrame (paliashdr, e->frame, &lerpdata);
	R_SetupEntityTransform (e, &lerpdata);

    eglPushMatrix ();
	R_RotateForEntity (lerpdata.origin,lerpdata.angles);
	eglTranslatef (paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
	eglScalef (paliashdr->scale[0], paliashdr->scale[1], paliashdr->scale[2]);

	shading = false;
	eglColor3f(1,1,1);
	GL_DrawAliasFrame_QMB_Flame_And_Shadows (paliashdr, lerpdata, paliashdr->commands, e->is_fake_frame0, 1 /*entalpha*/);

	eglPopMatrix ();
}

#endif // GLQUAKE specific