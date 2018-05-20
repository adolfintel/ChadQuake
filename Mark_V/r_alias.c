#ifndef GLQUAKE // WinQuake Software renderer

/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2010 MH - Frame Interpolation
Copyright (C) 2009-2014 Baker and others

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_alias.c: routines for setting up to draw alias models

#include "quakedef.h"


#define LIGHT_MIN	5		// lowest light value we'll allow, to avoid the
							//  need for inner-loop light clamping

mtriangle_t		*ptriangles;
affinetridesc_t	r_affinetridesc /* qasm */;

void *			acolormap /* qasm */;	// FIXME: should go away

// this is now only needed so that we can link with r_aliasa.s
// we could alternatively just remove r_aliasa.s from our project as we don't need it anymore
trivertx_t		*r_apverts /* qasm */;

// TODO: these probably will go away with optimized rasterization
mdl_t				*pmdl;
vec3_t				r_plightvec /* qasm */;
int					r_ambientlight /* qasm */;
float				r_shadelight /* qasm */;
aliashdr_t			*paliashdr;
finalvert_t			*pfinalverts;
auxvert_t			*pauxverts;
static float		ziscale;
static qmodel_t		*pmodel;

static vec3_t		alias_forward, alias_right, alias_up;

static maliasskindesc_t	*pskindesc;

int				r_amodels_drawn;
int				a_skinwidth;
int				r_anumverts /* qasm */;

float	aliastransform[3][4] /* qasm */;

typedef struct {
	int	index0;
	int	index1;
} aedge_t;

static aedge_t	aedges[12] = {
{0, 1}, {1, 2}, {2, 3}, {3, 0},
{4, 5}, {5, 6}, {6, 7}, {7, 4},
{0, 5}, {1, 4}, {2, 7}, {3, 6}
};



float	r_avertexnormals[NUMVERTEXNORMALS_162][3] /* qasm */ = {
#include "anorms.h"
};


typedef struct aliaslerp_s
{
	float v[3];

	byte lastlightnormal;
	byte currlightnormal;
	float blend;
} aliaslerp_t;

finalvert_t		*r_finalverts;
auxvert_t		*r_auxverts;
aliaslerp_t		*r_aliaslerpverts;
aliaslerp_t		*lerpverts;


void R_AliasTransformAndProjectFinalVerts (finalvert_t *fv, stvert_t *pstverts);
void R_AliasSetUpTransform (int trivial_accept);
void R_AliasTransformVector (vec3_t in, vec3_t out);
void R_AliasTransformFinalVert (finalvert_t *fv, auxvert_t *av, trivertx_t *pverts, stvert_t *pstverts);
void R_AliasProjectFinalVert (finalvert_t *fv, auxvert_t *av);


void R_AliasTransformAndProjectFinalVerts_C (finalvert_t *fv, stvert_t *pstverts);
void R_AliasTransformFinalVertMH (finalvert_t *fv, auxvert_t *av, aliaslerp_t *pverts, stvert_t *pstverts);


int r_maxaliasverts = 0;

void R_CheckAliasVerts (int numverts)
{
	if (numverts > r_maxaliasverts) r_maxaliasverts = numverts;
}


void R_FinalizeAliasVerts (void)
{
	// called every map change - these go on the hunk so they get cleared between maps
	// because alias models go in the cache r_maxaliasverts can only grow, never shrink
	r_aliaslerpverts = (aliaslerp_t *) Hunk_Alloc (r_maxaliasverts * sizeof (aliaslerp_t));
	r_auxverts = (auxvert_t *) Hunk_Alloc (r_maxaliasverts * sizeof (auxvert_t));
	r_finalverts = (finalvert_t *) Hunk_Alloc ((r_maxaliasverts + ((CACHE_SIZE - 1) / sizeof (finalvert_t)) + 1) * sizeof (finalvert_t));

	Con_DPrintLinef ("%d alias verts", r_maxaliasverts);
}



/*
================
R_AliasCheckBBox
================
*/
cbool R_AliasCheckBBox (void)
{
	int					i, flags, frame, numv;
	aliashdr_t			*pahdr;
	float				zi, basepts[8][3], v0, v1, frac;
	finalvert_t			*pv0, *pv1, viewpts[16];
	auxvert_t			*pa0, *pa1, viewaux[16];
	maliasframedesc_t	*pframedesc;
	cbool			zclipped, zfullyclipped;
	unsigned			anyclip, allclip;
	int					minz;

// expand, rotate, and translate points into worldspace

	currententity->trivial_accept = 0;
	pmodel = currententity->model;
	pahdr = Mod_Extradata (pmodel);
	pmdl = (mdl_t *)((byte *)pahdr + pahdr->model);

	R_AliasSetUpTransform (currententity->trivial_accept);

// construct the base bounding box for this frame
	frame = currententity->frame;
// TODO: don't repeat this check when drawing?
	if ((frame >= pmdl->numframes) || (frame < 0))
	{
		Con_DPrintLinef ("No such frame %d %s", frame,
			pmodel->name);
		frame = 0;
	}

	pframedesc = &pahdr->frames[frame];

// x worldspace coordinates
	basepts[0][0] = basepts[1][0] = basepts[2][0] = basepts[3][0] = (float)pframedesc->bboxmin.v[0]; // qbism has some min max stuff going on here.  We don't.
	basepts[4][0] = basepts[5][0] = basepts[6][0] = basepts[7][0] = (float)pframedesc->bboxmax.v[0];

// y worldspace coordinates
	basepts[0][1] = basepts[3][1] = basepts[5][1] = basepts[6][1] = (float)pframedesc->bboxmin.v[1];
	basepts[1][1] = basepts[2][1] = basepts[4][1] = basepts[7][1] = (float)pframedesc->bboxmax.v[1];

// z worldspace coordinates
	basepts[0][2] = basepts[1][2] = basepts[4][2] = basepts[5][2] = (float)pframedesc->bboxmin.v[2];
	basepts[2][2] = basepts[3][2] = basepts[6][2] = basepts[7][2] = (float)pframedesc->bboxmax.v[2];


	zclipped = false;
	zfullyclipped = true;

	minz = 9999;
	for (i=0; i<8 ; i++)
	{
		R_AliasTransformVector  (&basepts[i][0], &viewaux[i].fv[0]);

		if (viewaux[i].fv[2] < ALIAS_Z_CLIP_PLANE)
		{
		// we must clip points that are closer than the near clip plane
			viewpts[i].flags = ALIAS_Z_CLIP;
			zclipped = true;
		}
		else
		{
			if (viewaux[i].fv[2] < minz)
				minz = viewaux[i].fv[2];
			viewpts[i].flags = 0;
			zfullyclipped = false;
		}
	}


	if (zfullyclipped)
	{
		return false;	// everything was near-z-clipped
	}

	numv = 8;

	if (zclipped)
	{
	// organize points by edges, use edges to get new points (possible trivial
	// reject)
		for (i=0 ; i<12 ; i++)
		{
		// edge endpoints
			pv0 = &viewpts[aedges[i].index0];
			pv1 = &viewpts[aedges[i].index1];
			pa0 = &viewaux[aedges[i].index0];
			pa1 = &viewaux[aedges[i].index1];

		// if one end is clipped and the other isn't, make a new point
			if (pv0->flags ^ pv1->flags)
			{
				frac = (ALIAS_Z_CLIP_PLANE - pa0->fv[2]) /
						(pa1->fv[2] - pa0->fv[2]);
				viewaux[numv].fv[0] = pa0->fv[0] +
						(pa1->fv[0] - pa0->fv[0]) * frac;
				viewaux[numv].fv[1] = pa0->fv[1] +
						(pa1->fv[1] - pa0->fv[1]) * frac;
				viewaux[numv].fv[2] = ALIAS_Z_CLIP_PLANE;
				viewpts[numv].flags = 0;
				numv++;
			}
		}
	}

// project the vertices that remain after clipping
	anyclip = 0;
	allclip = ALIAS_XY_CLIP_MASK;

// TODO: probably should do this loop in ASM, especially if we use floats
	for (i=0 ; i<numv ; i++)
	{
	// we don't need to bother with vertices that were z-clipped
		if (viewpts[i].flags & ALIAS_Z_CLIP)
			continue;

		zi = 1.0 / viewaux[i].fv[2];

	// FIXME: do with chop mode in ASM, or convert to float
		v0 = (viewaux[i].fv[0] * xscale * zi) + xcenter;
		v1 = (viewaux[i].fv[1] * yscale * zi) + ycenter;

		flags = 0;

		if (v0 < r_refdef.fvrectx)
			flags |= ALIAS_LEFT_CLIP;

		if (v1 < r_refdef.fvrecty)
			flags |= ALIAS_TOP_CLIP;

		if (v0 > r_refdef.fvrectright)
			flags |= ALIAS_RIGHT_CLIP;

		if (v1 > r_refdef.fvrectbottom)
			flags |= ALIAS_BOTTOM_CLIP;

		anyclip |= flags;
		allclip &= flags;
	}

	if (allclip)
		return false;	// trivial reject off one side

	// MH never trivial accept as it breaks the render with interpolation
	currententity->trivial_accept = 0;
	return true;
#if 0 // 2015 April 11
	currententity->trivial_accept = !anyclip & !zclipped;

	if (currententity->trivial_accept)
	{
		if (minz > (r_aliastransition + (pmdl->size * r_resfudge)))
		{
			currententity->trivial_accept |= 2;
		}
	}

	// MH never trivial accept as it breaks the render with interpolation
#if 1 // Test
	//if (r_lerpmodels.value) // Baker
		currententity->trivial_accept = 0;
#endif

	return true;
#endif // End 2015 April 11
}


/*
================
R_AliasTransformVector
================
*/
void R_AliasTransformVector (vec3_t in, vec3_t out)
{
	out[0] = DotProduct(in, aliastransform[0]) + aliastransform[0][3];
	out[1] = DotProduct(in, aliastransform[1]) + aliastransform[1][3];
	out[2] = DotProduct(in, aliastransform[2]) + aliastransform[2][3];
}


/*
================
R_AliasPreparePoints

General clipped case
================
*/
void R_AliasPreparePoints (void)
{
	int			i;
	stvert_t	*pstverts;
	finalvert_t	*fv;
	auxvert_t	*av;
	mtriangle_t	*ptri;
	finalvert_t	*pfv[3];

	pstverts = (stvert_t *)((byte *)paliashdr + paliashdr->stverts);
	r_anumverts = pmdl->numverts;
 	fv = pfinalverts;
	av = pauxverts;

	lerpverts = r_aliaslerpverts;

	for (i = 0; i < r_anumverts; i++, fv++, av++, lerpverts++, pstverts++)
	{
		R_AliasTransformFinalVertMH (fv, av, lerpverts, pstverts);

		if (av->fv[2] < ALIAS_Z_CLIP_PLANE)
			fv->flags |= ALIAS_Z_CLIP;
		else
		{
			R_AliasProjectFinalVert (fv, av);

			if (fv->v[0] < r_refdef.aliasvrect.x)
				fv->flags |= ALIAS_LEFT_CLIP;

			if (fv->v[1] < r_refdef.aliasvrect.y)
				fv->flags |= ALIAS_TOP_CLIP;

			if (fv->v[0] > r_refdef.aliasvrectright)
				fv->flags |= ALIAS_RIGHT_CLIP;

			if (fv->v[1] > r_refdef.aliasvrectbottom)
				fv->flags |= ALIAS_BOTTOM_CLIP;
		}
	}

//
// clip and draw all triangles
//
	r_affinetridesc.numtriangles = 1;

	ptri = (mtriangle_t *)((byte *)paliashdr + paliashdr->triangles);
	for (i=0 ; i<pmdl->numtris ; i++, ptri++)
	{
		pfv[0] = &pfinalverts[ptri->vertindex[0]];
		pfv[1] = &pfinalverts[ptri->vertindex[1]];
		pfv[2] = &pfinalverts[ptri->vertindex[2]];

		if ( pfv[0]->flags & pfv[1]->flags & pfv[2]->flags & (ALIAS_XY_CLIP_MASK | ALIAS_Z_CLIP) )
			continue;		// completely clipped

		if ( ! ( (pfv[0]->flags | pfv[1]->flags | pfv[2]->flags) &
			(ALIAS_XY_CLIP_MASK | ALIAS_Z_CLIP) ) )
		{	// totally unclipped
			r_affinetridesc.pfinalverts = pfinalverts;
			r_affinetridesc.ptriangles = ptri;
			D_PolysetDraw ();
		}
		else
		{	// partially clipped
			R_AliasClipTriangle (ptri);
		}
	}
}


/*
================
R_AliasSetUpTransform
================
*/
void R_AliasSetUpTransform (int trivial_accept)
{
	int				i;
	float			rotationmatrix[3][4], t2matrix[3][4];
	static float	tmatrix[3][4];
	static float	viewmatrix[3][4];
	vec3_t			angles;

// TODO: should really be stored with the entity instead of being reconstructed
// TODO: should use a look-up table
// TODO: could cache lazily, stored in the entity

	angles[ROLL] = currententity->angles[ROLL];
	angles[PITCH] = -currententity->angles[PITCH];
	angles[YAW] = currententity->angles[YAW];
	AngleVectors (angles, alias_forward, alias_right, alias_up);

	tmatrix[0][0] = pmdl->scale[0];
	tmatrix[1][1] = pmdl->scale[1];
	tmatrix[2][2] = pmdl->scale[2];

	tmatrix[0][3] = pmdl->scale_origin[0];
	tmatrix[1][3] = pmdl->scale_origin[1];
	tmatrix[2][3] = pmdl->scale_origin[2];

#if 1 // Baker: To take out the FOV and substitute 90 (or user setting) for the viewmodel
	if (currententity == &cl.viewent_gun) {
		if (r_viewmodel_fov.value)
		{
	//		float scale = 1.0f / tan( DEG2RAD ((double)scr_fov.value / 2.0f) ) * (double)r_viewmodelfov.value / 90.0f; // Reverse out fov and do fov we want
			float scale = 1.0f / tan (Degree_To_Radians (scr_fov.value / 2.0f) ) * r_viewmodel_fov.value / 90.0f; // Reverse out fov and do fov we want

			if (r_viewmodel_size.value)
				scale *= CLAMP (0.5, r_viewmodel_size.value, 2);

			tmatrix[0][3] *= scale;
			tmatrix[0][0] *= scale;
		}
		

	}
#endif

// TODO: can do this with simple matrix rearrangement

	for (i=0 ; i<3 ; i++)
	{
		t2matrix[i][0] = alias_forward[i];
		t2matrix[i][1] = -alias_right[i];
		t2matrix[i][2] = alias_up[i];
	}

	t2matrix[0][3] = -modelorg[0];
	t2matrix[1][3] = -modelorg[1];
	t2matrix[2][3] = -modelorg[2];

// FIXME: can do more efficiently than full concatenation
	R_ConcatTransforms (t2matrix, tmatrix, rotationmatrix);

// TODO: should be global, set when vright, etc., set
	VectorCopy (vright, viewmatrix[0]);
	VectorCopy (vup, viewmatrix[1]);
	VectorInverse (viewmatrix[1]);
	VectorCopy (vpn, viewmatrix[2]);

//	viewmatrix[0][3] = 0;
//	viewmatrix[1][3] = 0;
//	viewmatrix[2][3] = 0;

	R_ConcatTransforms (viewmatrix, rotationmatrix, aliastransform);

// do the scaling up of x and y to screen coordinates as part of the transform
// for the unclipped case (it would mess up clipping in the clipped case).
// Also scale down z, so 1/z is scaled 31 bits for free, and scale down x and y
// correspondingly so the projected x and y come out right
// FIXME: make this work for clipped case too?
	if (trivial_accept)
	{
		for (i=0 ; i<4 ; i++)
		{
			aliastransform[0][i] *= aliasxscale *
					(1.0 / ((float)0x8000 * 0x10000));
			aliastransform[1][i] *= aliasyscale *
					(1.0 / ((float)0x8000 * 0x10000));
			aliastransform[2][i] *= 1.0 / ((float)0x8000 * 0x10000);

		}
	}
}


int R_AliasLightVert (aliaslerp_t *pverts)
{
	float	temp;
	float	lightcos, *plightnormal;

	temp = r_ambientlight;

	plightnormal = r_avertexnormals[pverts->currlightnormal];
	lightcos = DotProduct (plightnormal, r_plightvec) * pverts->blend;

	if (lightcos < 0)
	{
		temp += (int) (r_shadelight * lightcos);

		// clamp; because we limited the minimum ambient and shading light, we
		// don't have to clamp low light, just bright
		if (temp < 0) temp = 0;
	}

	plightnormal = r_avertexnormals[pverts->lastlightnormal];
	lightcos = DotProduct (plightnormal, r_plightvec) * (1.0f - pverts->blend);

	if (lightcos < 0)
	{
		temp += (int) (r_shadelight * lightcos);

		// clamp; because we limited the minimum ambient and shading light, we
		// don't have to clamp low light, just bright
		if (temp < 0) temp = 0;
	}

	return (int) temp;
}

/*
================
R_AliasTransformFinalVertMH
================
*/
void R_AliasTransformFinalVertMH (finalvert_t *fv, auxvert_t *av, aliaslerp_t *pverts, stvert_t *pstverts) {
	av->fv[0] = DotProduct (pverts->v, aliastransform[0]) + aliastransform[0][3];
	av->fv[1] = DotProduct (pverts->v, aliastransform[1]) + aliastransform[1][3];
	av->fv[2] = DotProduct (pverts->v, aliastransform[2]) + aliastransform[2][3];

	fv->v[2] = pstverts->s;
	fv->v[3] = pstverts->t;

	fv->flags = pstverts->onseam;

	// lighting
	fv->v[4] = R_AliasLightVert (pverts);
}

/*
================
R_AliasTransformFinalVert
================
*/
void R_AliasTransformFinalVert (finalvert_t *fv, auxvert_t *av,
	trivertx_t *pverts, stvert_t *pstverts)
{
	int		temp;
	float	lightcos, *plightnormal;

	av->fv[0] = DotProduct(pverts->v, aliastransform[0]) +
		aliastransform[0][3];
	av->fv[1] = DotProduct(pverts->v, aliastransform[1]) +
		aliastransform[1][3];
	av->fv[2] = DotProduct(pverts->v, aliastransform[2]) +
		aliastransform[2][3];

	fv->v[2] = pstverts->s;
	fv->v[3] = pstverts->t;

	fv->flags = pstverts->onseam;

// lighting
	plightnormal = r_avertexnormals[pverts->lightnormalindex];
	lightcos = DotProduct (plightnormal, r_plightvec);
	temp = r_ambientlight;

	if (lightcos < 0)
	{
		temp += (int)(r_shadelight * lightcos);

	// clamp; because we limited the minimum ambient and shading light, we
	// don't have to clamp low light, just bright
		if (temp < 0)
			temp = 0;
	}

	fv->v[4] = temp;
}


#if	!id386

/*
================
R_AliasTransformAndProjectFinalVerts
================
*/
void R_AliasTransformAndProjectFinalVerts (finalvert_t *fv, stvert_t *pstverts)
{
	int			i, temp;
	float		lightcos, *plightnormal, zi;
	trivertx_t	*pverts;

	pverts = r_apverts;

	for (i=0 ; i<r_anumverts ; i++, fv++, pverts++, pstverts++)
	{
	// transform and project
		zi = 1.0 / (DotProduct(pverts->v, aliastransform[2]) +
				aliastransform[2][3]);

	// x, y, and z are scaled down by 1/2**31 in the transform, so 1/z is
	// scaled up by 1/2**31, and the scaling cancels out for x and y in the
	// projection
		fv->v[5] = zi;

		fv->v[0] = ((DotProduct(pverts->v, aliastransform[0]) +
				aliastransform[0][3]) * zi) + aliasxcenter;
		fv->v[1] = ((DotProduct(pverts->v, aliastransform[1]) +
				aliastransform[1][3]) * zi) + aliasycenter;

		fv->v[2] = pstverts->s;
		fv->v[3] = pstverts->t;
		fv->flags = pstverts->onseam;

	// lighting
		plightnormal = r_avertexnormals[pverts->lightnormalindex];
		lightcos = DotProduct (plightnormal, r_plightvec);
		temp = r_ambientlight;

		if (lightcos < 0)
		{
			temp += (int)(r_shadelight * lightcos);

		// clamp; because we limited the minimum ambient and shading light, we
		// don't have to clamp low light, just bright
			if (temp < 0)
				temp = 0;
		}

		fv->v[4] = temp;
	}
}

#endif


/*
================
R_AliasTransformAndProjectFinalVerts
================
*/
void R_AliasTransformAndProjectFinalVerts_C (finalvert_t *fv, stvert_t *pstverts)
{
	int			i;
	float		zi;
	aliaslerp_t *pverts;

	pverts = r_aliaslerpverts;

	for (i = 0; i < r_anumverts; i++, fv++, pverts++, pstverts++)
	{
		// transform and project
		zi = 1.0 / (DotProduct (pverts->v, aliastransform[2]) + aliastransform[2][3]);

		// x, y, and z are scaled down by 1/2**31 in the transform, so 1/z is
		// scaled up by 1/2**31, and the scaling cancels out for x and y in the
		// projection
		fv->v[5] = zi;

		fv->v[0] = ((DotProduct (pverts->v, aliastransform[0]) + aliastransform[0][3]) * zi) + aliasxcenter;
		fv->v[1] = ((DotProduct (pverts->v, aliastransform[1]) + aliastransform[1][3]) * zi) + aliasycenter;

		fv->v[2] = pstverts->s;
		fv->v[3] = pstverts->t;
		fv->flags = pstverts->onseam;

		// lighting
		fv->v[4] = R_AliasLightVert (pverts);
	}
}



/*
================
R_AliasProjectFinalVert
================
*/
void R_AliasProjectFinalVert (finalvert_t *fv, auxvert_t *av)
{
	float	zi;

// project points
	zi = 1.0 / av->fv[2];

	fv->v[5] = zi * ziscale;

	fv->v[0] = (av->fv[0] * aliasxscale * zi) + aliasxcenter;
	fv->v[1] = (av->fv[1] * aliasyscale * zi) + aliasycenter;
}


/*
================
R_AliasPrepareUnclippedPoints
================
*/
void R_AliasPrepareUnclippedPoints (void)
{
	stvert_t	*pstverts;
	finalvert_t	*fv;

	pstverts = (stvert_t *)((byte *)paliashdr + paliashdr->stverts);
	r_anumverts = pmdl->numverts;
// FIXME: just use pfinalverts directly?
	fv = pfinalverts;
	
	R_AliasTransformAndProjectFinalVerts_C (fv, pstverts);

	if (r_affinetridesc.drawtype)
		D_PolysetDrawFinalVerts (fv, r_anumverts);

	r_affinetridesc.pfinalverts = pfinalverts;
	r_affinetridesc.ptriangles = (mtriangle_t *)
		((byte *)paliashdr + paliashdr->triangles);
	r_affinetridesc.numtriangles = pmdl->numtris;

	D_PolysetDraw ();
}

/*
===============
R_AliasSetupSkin
===============
*/
void R_AliasSetupSkin (void)
{
	int					skinnum;
	int					i, numskins;
	maliasskingroup_t	*paliasskingroup;
	float				*pskinintervals, fullskininterval;
	float				skintargettime, skintime;

	skinnum = currententity->skinnum;
	if ((skinnum >= pmdl->numskins) || (skinnum < 0))
	{
		Con_DPrintLinef ("R_AliasSetupSkin: no such skin # %d", skinnum);
		skinnum = 0;
	}

	pskindesc = ((maliasskindesc_t *)
		 ((byte *)paliashdr + paliashdr->skindesc)) + skinnum;
	a_skinwidth = pmdl->skinwidth;

	if (pskindesc->type == ALIAS_SKIN_GROUP)
	{
		paliasskingroup = (maliasskingroup_t *)((byte *)paliashdr +
			pskindesc->skin);
		pskinintervals = (float *)
			((byte *)paliashdr + paliasskingroup->intervals);
		numskins = paliasskingroup->numskins;
		fullskininterval = pskinintervals[numskins-1];

		skintime = cl.ctime + currententity->syncbase;

	// when loading in Mod_LoadAliasSkinGroup, we guaranteed all interval
	// values are positive, so we don't have to worry about division by 0
		skintargettime = skintime -
			 ((int)(skintime / fullskininterval)) * fullskininterval;

		for (i=0 ; i<(numskins-1) ; i++)
		{
			if (pskinintervals[i] > skintargettime)
				break;
		}

		pskindesc = &paliasskingroup->skindescs[i];
	}

	r_affinetridesc.pskindesc = pskindesc;
	r_affinetridesc.pskin = (void *)((byte *)paliashdr + pskindesc->skin);
	r_affinetridesc.skinwidth = a_skinwidth;
	r_affinetridesc.seamfixupX16 =  (a_skinwidth >> 1) << 16;
	r_affinetridesc.skinheight = pmdl->skinheight;
}

/*
================
R_AliasSetupLighting
================
*/
void R_AliasSetupLighting (alight_t *plighting)
{

// guarantee that no vertex will ever be lit below LIGHT_MIN, so we don't have
// to clamp off the bottom
	r_ambientlight = plighting->ambientlight;

	if (r_ambientlight < LIGHT_MIN)
		r_ambientlight = LIGHT_MIN;

	r_ambientlight = (255 - r_ambientlight) << VID_CBITS;

	if (r_ambientlight < LIGHT_MIN)
		r_ambientlight = LIGHT_MIN;

	r_shadelight = plighting->shadelight;

	if (r_shadelight < 0)
		r_shadelight = 0;

	r_shadelight *= VID_GRADES;

// rotate the lighting vector into the model's frame of reference
	r_plightvec[0] = DotProduct (plighting->plightvec, alias_forward);
	r_plightvec[1] = -DotProduct (plighting->plightvec, alias_right);
	r_plightvec[2] = DotProduct (plighting->plightvec, alias_up);
}


void R_BoundPoseSingle (entity_t *ent, mdl_t *m, lerpdata_t *lerpdata)
{
	if (lerpdata->pose2 < 0) lerpdata->pose2 = 0;
	if (lerpdata->pose2 >= m->numframes) lerpdata->pose2 = m->numframes - 1;

	if (lerpdata->pose1 < 0) lerpdata->pose1 = 0;
	if (lerpdata->pose1 >= m->numframes) lerpdata->pose1 = m->numframes - 1;
}


void R_BoundPoseGroup (entity_t *ent, maliasgroup_t *m, lerpdata_t *lerpdata)
{
// Baker: I do not believe this function is needed any longer?
	return;
#if 0 // 11 April 2015
#if 0
	if (1)
	{

	if (strstr(ent->model->name, "flame2"))
	{
		lerpdata->pose1 -= 6;
		lerpdata->pose2 -= 6;
		return;
	}
#endif

	if (lerpdata->pose2 < 0) 
		lerpdata->pose2 = 0;
	if (lerpdata->pose2 >= m->numframes) 
		lerpdata->pose2 = m->numframes - 1;

	if (lerpdata->pose1 < 0) 
		lerpdata->pose1 = 0;
	if (lerpdata->pose1 >= m->numframes) 
		lerpdata->pose1 = m->numframes - 1;
#endif // April 11
}



/*
=================
R_AliasSetupFrameMH

set currverts and lastverts and blend between the two frames
=================
*/
void R_AliasSetupFrameMH (entity_t *ent)
{
	maliasgroup_t	*paliasgroup;
	int				frame;
	lerpdata_t		lerpdata;
	float			blend;

	frame = ent->frame;

	if ((frame >= pmdl->numframes) || (frame < 0))
	{
		Con_DPrintLinef ("R_AliasSetupFrameMH: no such frame %d", frame);
		frame = 0;
	}

	paliasgroup = (maliasgroup_t *) ((byte *) paliashdr + paliashdr->frames[frame].frame);

	R_SetupAliasFrame (paliashdr, frame, &lerpdata);

	if (lerpdata.pose1 != lerpdata.pose2)
		blend = lerpdata.blend;
	else blend = 0;

	{
		trivertx_t		*currverts;
		trivertx_t		*lastverts;
		int				i;


		if (paliashdr->frames[frame].type == ALIAS_SINGLE)
		{
			R_BoundPoseSingle (ent, pmdl, &lerpdata);

			currverts = (trivertx_t *) ((byte *) paliashdr + paliashdr->frames[lerpdata.pose2].frame);
			lastverts = (trivertx_t *) ((byte *) paliashdr + paliashdr->frames[lerpdata.pose1].frame);
		}
		else
		{
			if (1) // For static models like torches with no server ent
			{
				lerpdata.pose1 -= paliashdr->frames[frame].firstpose;
				lerpdata.pose2 -= paliashdr->frames[frame].firstpose;
			}
//			else R_BoundPoseGroup (ent, paliasgroup, &lerpdata);
			currverts = (trivertx_t *) ((byte *) paliashdr + paliasgroup->frames[lerpdata.pose2].frame);
			lastverts = (trivertx_t *) ((byte *) paliashdr + paliasgroup->frames[lerpdata.pose1].frame);
		}

		lerpverts = r_aliaslerpverts;

		// perform the lerp
		for (i = 0; i < pmdl->numverts; i++, currverts++, lastverts++, lerpverts++)
		{
			lerpverts->v[0] = currverts->v[0] * blend + lastverts->v[0] * (1.0f - blend);
			lerpverts->v[1] = currverts->v[1] * blend + lastverts->v[1] * (1.0f - blend);
			lerpverts->v[2] = currverts->v[2] * blend + lastverts->v[2] * (1.0f - blend);

			lerpverts->currlightnormal = currverts->lightnormalindex;
			lerpverts->lastlightnormal = lastverts->lightnormalindex;
			lerpverts->blend = blend;
		}
	}
}



/*
================
R_AliasDrawModelMH
================
*/
void R_AliasDrawModelMH (alight_t *plighting)
{
	r_amodels_drawn++;

	// cache align
	pfinalverts = (finalvert_t *)(((long) &r_finalverts[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));
	pauxverts = &r_auxverts[0];

	paliashdr = (aliashdr_t *) Mod_Extradata (currententity->model);
	pmdl = (mdl_t *) ((byte *) paliashdr + paliashdr->model);

	R_AliasSetupSkin ();
	R_AliasSetUpTransform (0);
	R_AliasSetupLighting (plighting);
	R_AliasSetupFrameMH (currententity);

	if (!currententity->colormap)
		System_Error ("R_AliasDrawModel: !currententity->colormap");

	r_affinetridesc.drawtype = (currententity->trivial_accept == 3);

	if (r_affinetridesc.drawtype)
	{
		D_PolysetUpdateTables ();		// FIXME: precalc...
	}
	else
	{
// Baker: SW-ASM Killed Feb 7 2016 with no loss of frames per second /// #if id386
//		//D_Aff8Patch (currententity->colormap);
//#endif
	}

	acolormap = currententity->colormap;

	if (currententity != &cl.viewent_gun)
		ziscale = (float) 0x8000 * (float) 0x10000;
	else
		ziscale = (float) 0x8000 * (float) 0x10000 * 3.0;

	if (currententity->trivial_accept)
		R_AliasPrepareUnclippedPoints ();
	else
		R_AliasPreparePoints ();
}

#endif // !GLQUAKE - WinQuake Software renderer

