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
// r_main.c

#include "quakedef.h" // Baker: mods = none really, just boilerplate for MH model interpolation but just the function call, not the meat (like the actual code is in a different file)


//define	PASSAGES

void		*colormap  /* qasm */;
vec3_t		viewlightvec;
alight_t	r_viewlighting = {128, 192, viewlightvec};
float		r_time1;
int			r_numallocatededges;

float		r_aliasuvscale = 1.0;
int			r_outofsurfaces;
int			r_outofedges;

cbool		r_dowarp, r_dowarpold, r_viewchanged;

int			numbtofpolys;
btofpoly_t	*pbtofpolys;
mvertex_t	*r_pcurrentvertbase;

int			c_surf;
int			r_maxsurfsseen, r_maxedgesseen, r_cnumsurfs;
cbool		r_surfsonstack;
int			r_clipflags;

byte		*r_stack_start;
byte		*r_warpbuffer;




//
// view origin
//

vec3_t base_vup, base_vpn, base_vright;

//
// screen size info
//

float		xcenter /* qasm */, ycenter /* qasm */;
float		xscale /* qasm */, yscale /* qasm */;
float		xscaleinv, yscaleinv;
float		xscaleshrink, yscaleshrink;
float		aliasxscale, aliasyscale, aliasxcenter /* qasm */, aliasycenter  /* qasm */;


float	pixelAspect;
//float	screenAspect;
float	verticalFieldOfView;
float	xOrigin, yOrigin;

mplane_t	screenedge[4];

//
// refresh flags
//

// Baker: ASM uses r_framecount
int		r_framecount = 1  /* qasm */;	// Baker: was "=1" with comment "so frame counts initialized to 0 don't match"
int		r_polycount;
int		r_drawnpolycount;


#define		VIEWMODNAME_LENGTH	256
char		viewmodname[VIEWMODNAME_LENGTH+1];
int			modcount;

int			*pfrustum_indexes[4];
int			r_frustum_indexes[4*6];

//mleaf_t		*r_viewleaf, *r_oldviewleaf;

texture_t	*r_notexture_mip;

float		r_aliastransition, r_resfudge;


float	dp_time1, dp_time2, db_time1, db_time2, rw_time1, rw_time2;
float	se_time1, se_time2, de_time1, de_time2, dv_time1, dv_time2;

void R_MarkLeaves (void);




#if 0 // Baker: We aren't using these as of now


void R_CullModelForEntityGetMins (entity_t *e, vec3_t mymins, vec3_t mymaxs)
{

	if (e->angles[0] || e->angles[2]) //pitch or roll
	{
		VectorCopy (e->model->rmins, mymins);
		VectorCopy (e->model->rmaxs, mymaxs);
	}
	else if (e->angles[1]) //yaw
	{
		VectorCopy (e->model->ymins, mymins);
		VectorCopy (e->model->ymaxs, mymaxs);
	}
	else //no rotation
	{
		VectorCopy (e->model->mins, mymins);
		VectorCopy (e->model->maxs, mymaxs);
	}

}
#endif

//==============================================================================
//
// SETUP FRAME
//
//==============================================================================

int SignbitsForPlane (mplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}

/*
===============
TurnVector -- johnfitz

turn forward towards side on the plane defined by forward and side
if angle = 90, the result will be equal to side
assumes side and forward are perpendicular, and normalized
to turn away from side, use a negative angle
===============
*/

void TurnVector (vec3_t out, const vec3_t forward, const vec3_t side, float angle)
{
	float scale_forward, scale_side;

	scale_forward = cos (Degree_To_Radians (angle) );
	scale_side = sin (Degree_To_Radians (angle) );

	out[0] = scale_forward*forward[0] + scale_side*side[0];
	out[1] = scale_forward*forward[1] + scale_side*side[1];
	out[2] = scale_forward*forward[2] + scale_side*side[2];
}

/*
===============
R_SetFrustum -- johnfitz -- rewritten
===============
*/
void R_SetFrustum (float fovx, float fovy)
{
	int		i;

//	if (r_lockfrustum.value)
//		return;		// Do not update!

	TurnVector(r_frustum[0].normal, vpn, vright, fovx/2 - 90); //left plane
	TurnVector(r_frustum[1].normal, vpn, vright, 90 - fovx/2); //right plane
	TurnVector(r_frustum[2].normal, vpn, vup, 90 - fovy/2); //bottom plane
	TurnVector(r_frustum[3].normal, vpn, vup, fovy/2 - 90); //top plane

	for (i=0 ; i<4 ; i++)
	{
		r_frustum[i].type = PLANE_ANYZ_5;
		r_frustum[i].dist = DotProduct (r_origin, r_frustum[i].normal); //FIXME: shouldn't this always be zero?
		r_frustum[i].signbits = SignbitsForPlane (&r_frustum[i]);
	}
}

void CreatePassages (void);
void SetVisibilityByPassages (void);





/*
===============
R_SetVrect
===============
*/
void R_SetVrect (vrect_t *pvrectin, vrect_t *pvrect, int lineadj)
{
	int		h;
	float	size;

	size = scr_viewsize.value > 100.0 ? 100.0 : scr_viewsize.value;
	if (cl.intermission)
	{
		size = 100.0;
		lineadj = 0;
	}
	size /= 100.0;

	h = pvrectin->height - lineadj;
	pvrect->width = pvrectin->width * size;
	if (pvrect->width < 96)
	{
		size = 96.0 / pvrectin->width;
		pvrect->width = 96;	// min for icons
	}
	pvrect->width &= ~7;
	pvrect->height = pvrectin->height * size;
	if (pvrect->height > pvrectin->height - lineadj)
		pvrect->height = pvrectin->height - lineadj;

	pvrect->height &= ~1;

	pvrect->x = (pvrectin->width - pvrect->width)/2;
	pvrect->y = (h - pvrect->height)/2;


}


/*
===============
R_ViewChanged

Called every time the vid structure or r_refdef changes.
Guaranteed to be called before the first refresh
===============
*/
void R_ViewChanged (vrect_t *pvrect, int lineadj, float aspect)
{
	int		i;
	float	res_scale;

	r_viewchanged = true;

	R_SetVrect (pvrect, &r_refdef.vrect, lineadj);

	r_refdef.horizontalFieldOfView = 2.0 * tan (r_refdef.fov_x/360*M_PI);
	r_refdef.fvrectx = (float)r_refdef.vrect.x;
	r_refdef.fvrectx_adj = (float)r_refdef.vrect.x - 0.5;
	r_refdef.vrect_x_adj_shift20 = (r_refdef.vrect.x<<20) + (1<<19) - 1;
	r_refdef.fvrecty = (float)r_refdef.vrect.y;
	r_refdef.fvrecty_adj = (float)r_refdef.vrect.y - 0.5;
	r_refdef.vrectright = r_refdef.vrect.x + r_refdef.vrect.width;
	r_refdef.vrectright_adj_shift20 = (r_refdef.vrectright<<20) + (1<<19) - 1;
	r_refdef.fvrectright = (float)r_refdef.vrectright;
	r_refdef.fvrectright_adj = (float)r_refdef.vrectright - 0.5;
	r_refdef.vrectrightedge = (float)r_refdef.vrectright - 0.99;
	r_refdef.vrectbottom = r_refdef.vrect.y + r_refdef.vrect.height;
	r_refdef.fvrectbottom = (float)r_refdef.vrectbottom;
	r_refdef.fvrectbottom_adj = (float)r_refdef.vrectbottom - 0.5;

	r_refdef.aliasvrect.x = (int)(r_refdef.vrect.x * r_aliasuvscale);
	r_refdef.aliasvrect.y = (int)(r_refdef.vrect.y * r_aliasuvscale);
	r_refdef.aliasvrect.width = (int)(r_refdef.vrect.width * r_aliasuvscale);
	r_refdef.aliasvrect.height = (int)(r_refdef.vrect.height * r_aliasuvscale);
	r_refdef.aliasvrectright = r_refdef.aliasvrect.x + r_refdef.aliasvrect.width;
	r_refdef.aliasvrectbottom = r_refdef.aliasvrect.y + r_refdef.aliasvrect.height;

//	pixelAspect = aspect;
	xOrigin = r_refdef.xOrigin;
	yOrigin = r_refdef.yOrigin;


	pixelAspect = 1;
	verticalFieldOfView = 2.0 * tan (r_refdef.fov_y/360*M_PI);

// values for perspective projection
// if math were exact, the values would range from 0.5 to to range+0.5
// hopefully they wll be in the 0.000001 to range+.999999 and truncate
// the polygon rasterization will never render in the first row or column
// but will definately render in the [range] row and column, so adjust the
// buffer origin to get an exact edge to edge fill
	xcenter = ((float)r_refdef.vrect.width * XCENTERING) +
		r_refdef.vrect.x - 0.5;
	aliasxcenter = xcenter * r_aliasuvscale;
	ycenter = ((float)r_refdef.vrect.height * YCENTERING) +
		r_refdef.vrect.y - 0.5;
	aliasycenter = ycenter * r_aliasuvscale;

	xscale = r_refdef.vrect.width / r_refdef.horizontalFieldOfView;
	aliasxscale = xscale * r_aliasuvscale;
	xscaleinv = 1.0 / xscale;
	yscale = xscale * pixelAspect;
	yscale = (1 /*scr_fov_adapt.value*/) ? r_refdef.vrect.height / verticalFieldOfView :
	xscale * pixelAspect;
	aliasyscale = yscale * r_aliasuvscale;
	yscaleinv = 1.0 / yscale;
	xscaleshrink = (r_refdef.vrect.width-6)/r_refdef.horizontalFieldOfView;
	yscaleshrink = xscaleshrink*pixelAspect;

// left side clip
	screenedge[0].normal[0] = -1.0 / (xOrigin*r_refdef.horizontalFieldOfView);
	screenedge[0].normal[1] = 0;
	screenedge[0].normal[2] = 1;
	screenedge[0].type = PLANE_ANYZ_5;

// right side clip
	screenedge[1].normal[0] =
		1.0 / ((1.0-xOrigin)*r_refdef.horizontalFieldOfView);
	screenedge[1].normal[1] = 0;
	screenedge[1].normal[2] = 1;
	screenedge[1].type = PLANE_ANYZ_5;

// top side clip
	screenedge[2].normal[0] = 0;
	screenedge[2].normal[1] = -1.0 / (yOrigin*verticalFieldOfView);
	screenedge[2].normal[2] = 1;
	screenedge[2].type = PLANE_ANYZ_5;

// bottom side clip
	screenedge[3].normal[0] = 0;
	screenedge[3].normal[1] = 1.0 / ((1.0-yOrigin)*verticalFieldOfView);
	screenedge[3].normal[2] = 1;
	screenedge[3].type = PLANE_ANYZ_5;

	for (i=0 ; i<4 ; i++)
		VectorNormalize (screenedge[i].normal);

	res_scale = sqrt ((double)(r_refdef.vrect.width * r_refdef.vrect.height) /
		(320.0 * 152.0)) *
		(2.0 / r_refdef.horizontalFieldOfView);
	r_aliastransition = sw_r_aliastransbase.value * res_scale;
	r_resfudge = sw_r_aliastransadj.value * res_scale;


// TODO: collect 386-specific code in one place
#if	id386
	System_MakeCodeWriteable ((long)R_Surf8Start, (long)R_Surf8End - (long)R_Surf8Start);
	colormap = vid.colormap;
	R_Surf8Patch ();
#endif	// id386

	D_ViewChanged ();
}


/*
===============
R_MarkLeaves
===============
*/
void R_MarkLeaves (void)
{
	byte	*vis, solid[4096]; // Note: Qbism jacked this to 16000
	mnode_t	*node;
	int		i;

	if (cl.r_oldviewleaf == cl.r_viewleaf)
		return;

//	if (r_lockpvs.value == 0)  // Don't update if PVS is locked
		if (chase_active.value && chase_mode) // Camera position isn't visibility leaf!
			cl.r_viewleaf = Mod_PointInLeaf (nonchase_origin, cl.worldmodel);
		else
			cl.r_viewleaf = Mod_PointInLeaf (r_origin, cl.worldmodel);

	cl.r_visframecount++;
	cl.r_oldviewleaf = cl.r_viewleaf;

	// Baker:
	{
		msurface_t **mark;
		for (i = 0, mark = cl.r_viewleaf->firstmarksurface; i < cl.r_viewleaf->nummarksurfaces; i++, mark++)
			if ((*mark)->flags & SURF_DRAWTURB)
				frame.nearwaterportal = true; // Baker: frame vars are reset to false each frame
	}


	if (r_novis.value)
	{
		vis = solid;
		memset (solid, 0xff, (cl.worldmodel->numleafs+7)>>3);
	}
	else
	{
		vis = Mod_LeafPVS (cl.r_viewleaf, cl.worldmodel);
	}

	if (!frame.nearwaterportal && !level.ever_been_away_from_water_portal) 
		level.ever_been_away_from_water_portal = true;

	if (frame.nearwaterportal && !level.water_vis_known)
	{

		// Baker: This is to avoid a spawn nearwaterportal situation confusing the detection which could be
		// in theory caused by a save game happening to save in such a rare place.  This is an almost impossible
		// scenario.
		if (level.ever_been_away_from_water_portal)
		{
			Con_DPrintLinef ("AUTO WATER VIS:  Level is NOT vised!");
			level.water_vis_known = true;
			level.water_vis = false;
		}
	}

	for (i = 0 ; i < cl.worldmodel->numleafs ; i++)
	{
		if (vis[i>>3] & (1<<(i&7)))
		{
			node = (mnode_t *)&cl.worldmodel->leafs[i + 1];
			do
			{
				if (node->visframe == cl.r_visframecount)
					break;
				node->visframe = cl.r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
}

//==============================================================================
//
// RENDER VIEW
//
//==============================================================================

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int			i, j;
	int			lnum;
	alight_t	lighting;
// FIXME: remove and do real lighting
	float		lightvec[3] = {-1, 0, 0};
	vec3_t		dist;
	float		add;

	if (!r_drawentities.value)
		return;

//	SortEntitiesByTransparency ();

	for (i=0 ; i<cl.numvisedicts ; i++)
	{
		currententity = cl.visedicts[i];

		// Baker 3.75 - adjusted the following from Enhanced GLQuake
		if (currententity == &cl_entities[cl.viewentity_player])
		{
			if (!chase_active.value)
				continue;	// don't draw the player
			else
				currententity->angles[0] *= 0.3;

		}

#if 1 // Baker: Rare.  Need to investigate relink entities and Nehahra support.  E1M5 occasional intermission crash due to player.mdl not being resolved.  MODEL_CRUTCH
		if (currententity->model == NULL && currententity->modelindex)
			currententity->model = cl.model_precache[currententity->baseline.modelindex];
#endif 		

		switch (currententity->model->type)
		{
		case mod_sprite:
			VectorCopy (currententity->origin, r_entorigin);
			VectorSubtract (r_origin, r_entorigin, modelorg);
			R_DrawSpriteModel (currententity);
			break;

		case mod_alias:
#ifdef SUPPORTS_NEHAHRA
			if (nehahra_active && !strcmp("progs/null.mdl", currententity->model->name))
				break; // Baker: TO DO: Something should have filtered this out.
#endif // SUPPORTS_NEHAHRA

			// Something is slightly different with paused demoplayback and r_lerpmove for the software renderer.  Haven't identified.
			if (r_lerpmove.value && !(cls.demoplayback && cl.paused & 2))
				R_SetupEntityTransform (currententity, NULL); // move lerp

			VectorCopy (currententity->origin, r_entorigin);
			VectorSubtract (r_origin, r_entorigin, modelorg);

		// see if the bounding box lets us trivially reject, also sets
		// trivial accept status

			if (R_AliasCheckBBox ())
			{
				j = R_LightPoint (currententity->origin);

				lighting.ambientlight = j;
				lighting.shadelight = j;

				lighting.plightvec = lightvec;

				for (lnum=0 ; lnum<MAX_FITZQUAKE_DLIGHTS ; lnum++)
				{
					if (cl.dlights[lnum].die >= cl.time)
					{
						VectorSubtract (currententity->origin,
									cl.dlights[lnum].origin,
									dist);
						add = cl.dlights[lnum].radius - VectorLength(dist);

						if (add > 0)
							lighting.ambientlight += add;
					}
				}

			// clamp lighting so it doesn't overbright as much
				if (lighting.ambientlight > 128)
					lighting.ambientlight = 128;
				if (lighting.ambientlight + lighting.shadelight > 192)
					lighting.shadelight = 192 - lighting.ambientlight;

				R_AliasDrawModelMH (&lighting);
			}

			break;

		default:
			break;
		}
	}
}

/*
=============
R_DrawViewModel
=============
*/
void R_DrawViewModel (void)
{
// FIXME: remove and do real lighting
	float		lightvec[3] = {-1, 0, 0};
	int			j;
	int			lnum;
	vec3_t		dist;
	float		add;
	dlight_t	*dl;

	if (!r_drawviewmodel.value)
		return;

	if (chase_active.value)
		return;

	if (!r_drawentities.value)
		return;

#ifdef GLQUAKE_ENVMAP_COMMAND
	if (envmap)
		return;
#endif // GLQUAKE_ENVMAP_COMMAND

	if (cl.stats[STAT_HEALTH] <= 0)
		return;

	currententity = &cl.viewent_gun;
	if (!currententity->model)
		return;

	//johnfitz -- this fixes a crash
	if (currententity->model->type != mod_alias)
		return;
	//johnfitz


	if (cl.items & IT_INVISIBILITY)
	{
		 if (!r_viewmodel_ring.value)
			return;

		 currententity->alpha = ENTALPHA_ENCODE(WEAPON_INVISIBILITY_ALPHA);
	} 
	else currententity->alpha = cl_entities[cl.viewentity_player].alpha; // If player is invisible so is his gun

	VectorCopy (currententity->origin, r_entorigin);
	VectorSubtract (r_origin, r_entorigin, modelorg);

	VectorNegate (vup, viewlightvec);

	j = R_LightPoint (currententity->origin);

	if (j < 24)
		j = 24;		// always give some light on gun
	r_viewlighting.ambientlight = j;
	r_viewlighting.shadelight = j;

// add dynamic lights
	for (lnum = 0 ; lnum < MAX_FITZQUAKE_DLIGHTS ; lnum++)
	{
		dl = &cl.dlights[lnum];
		if (!dl->radius)
			continue;
		if (!dl->radius)
			continue;
		if (dl->die < cl.time)
			continue;

		VectorSubtract (currententity->origin, dl->origin, dist);
		add = dl->radius - VectorLength(dist);
		if (add > 0)
			r_viewlighting.ambientlight += add;
	}

// clamp lighting so it doesn't overbright as much
	if (r_viewlighting.ambientlight > 128)
		r_viewlighting.ambientlight = 128;
	if (r_viewlighting.ambientlight + r_viewlighting.shadelight > 192)
		r_viewlighting.shadelight = 192 - r_viewlighting.ambientlight;

	r_viewlighting.plightvec = lightvec;

	R_AliasDrawModelMH (&r_viewlighting);
}


/*
=============
R_BmodelCheckBBox
=============
*/
int R_BmodelCheckBBox (qmodel_t *clmodel, float *minmaxs)
{
	int			i, *pindex, clipflags;
	vec3_t		acceptpt, rejectpt;
	double		d;

	clipflags = 0;

	if (currententity->angles[0] || currententity->angles[1]
		|| currententity->angles[2])
	{
		for (i=0 ; i<4 ; i++)
		{
			d = DotProduct (currententity->origin, view_clipplanes[i].normal);
			d -= view_clipplanes[i].dist;

			if (d <= -clmodel->radius)
				return BMODEL_FULLY_CLIPPED;

			if (d <= clmodel->radius)
				clipflags |= (1<<i);
		}
	}
	else
	{
		for (i=0 ; i<4 ; i++)
		{
		// generate accept and reject points
		// FIXME: do with fast look-ups or integer tests based on the sign bit
		// of the floating point values

			pindex = pfrustum_indexes[i];

			rejectpt[0] = minmaxs[pindex[0]];
			rejectpt[1] = minmaxs[pindex[1]];
			rejectpt[2] = minmaxs[pindex[2]];

			d = DotProduct (rejectpt, view_clipplanes[i].normal);
			d -= view_clipplanes[i].dist;

			if (d <= 0)
				return BMODEL_FULLY_CLIPPED;

			acceptpt[0] = minmaxs[pindex[3+0]];
			acceptpt[1] = minmaxs[pindex[3+1]];
			acceptpt[2] = minmaxs[pindex[3+2]];

			d = DotProduct (acceptpt, view_clipplanes[i].normal);
			d -= view_clipplanes[i].dist;

			if (d <= 0)
				clipflags |= (1<<i);
		}
	}

	return clipflags;
}


/*
=============
R_DrawBEntitiesOnList
=============
*/
void R_DrawBEntitiesOnList (void)
{
	int			i, j, clipflags;
	vec3_t		oldorigin;
	qmodel_t	*clmodel;
	float		minmaxs[6];

	if (!r_drawentities.value)
		return;

	VectorCopy (modelorg, oldorigin);
	insubmodel = true;

	for (i=0 ; i < cl.numvisedicts ; i++)
	{
		currententity = cl.visedicts[i];

#if 1 // Baker: Rare.  Need to investigate relink entities and Nehahra support.  E1M5 occasional intermission crash due to player.mdl not being resolved.  MODEL_CRUTCH
		if (currententity->model == NULL && currententity->modelindex)
			currententity->model = cl.model_precache[currententity->baseline.modelindex];
#endif 		

		if (currententity->model->type != mod_brush)
			continue;

		clmodel = currententity->model;

		// see if the bounding box lets us trivially reject, also sets
		// trivial accept status
		for (j = 0 ; j < 3 ; j++)
		{
			minmaxs[j + 0] = currententity->origin[j] + clmodel->mins[j];
			minmaxs[j + 3] = currententity->origin[j] + clmodel->maxs[j];
		}

		clipflags = R_BmodelCheckBBox (clmodel, minmaxs);

		if (clipflags == BMODEL_FULLY_CLIPPED)
			continue;

		// Bmodel and not fully clipped ...
		
		VectorCopy (currententity->origin, r_entorigin);
		VectorSubtract (r_origin, r_entorigin, modelorg);
	// FIXME: is this needed?
		VectorCopy (modelorg, r_worldmodelorg);

		r_pcurrentvertbase = clmodel->vertexes;

	// FIXME: stop transforming twice
		R_RotateBmodel ();

		// calculate dynamic lighting for bmodel if it's not an
		// instanced model
		if (clmodel->firstmodelsurface)
		{ // Baker from MH

			entity_t* e = currententity;

			// calculate entity local space for dlight transforms
			Mat4_Identity_Set (&e->gl_matrix);

			// don't need to negate angles[0] as it's not going through the extra negation in R_RotateForEntity
			if (e->angles[2]) Mat4_Rotate (&e->gl_matrix, -e->angles[2], 1, 0, 0);
			if (e->angles[0]) Mat4_Rotate (&e->gl_matrix, -e->angles[0], 0, 1, 0);
			if (e->angles[1]) Mat4_Rotate (&e->gl_matrix, -e->angles[1], 0, 0, 1);

			Mat4_Translate (&e->gl_matrix, -e->origin[0], -e->origin[1], -e->origin[2]);
			R_PushDlights (e);


		} // clmodel->firstmodelsurface 

		r_pefragtopnode = NULL;

		for (j = 0 ; j < 3 ; j++)
		{
			r_emins[j] = minmaxs[j + 0];
			r_emaxs[j] = minmaxs[j + 3];
		}

		R_SplitEntityOnNode2 (cl.worldmodel->nodes);

		if (r_pefragtopnode)
		{
			currententity->topnode = r_pefragtopnode;

			if (r_pefragtopnode->contents >= 0)
			{
				// not a leaf; has to be clipped to the world BSP
				r_clipflags = clipflags;
				R_DrawSolidClippedSubmodelPolygons (clmodel);
			}
			else
			{
				// falls entirely in one leaf, so we just put all the
				// edges in the edge list and let 1/z sorting handle
				// drawing order
				R_DrawSubmodelPolygons (clmodel, clipflags);
			}

			currententity->topnode = NULL;
		} // if r_pefragtopnode


		// put back world rotation and frustum clipping
		// FIXME: R_RotateBmodel should just work off base_vxx
		VectorCopy (base_vpn, vpn);
		VectorCopy (base_vup, vup);
		VectorCopy (base_vright, vright);
		VectorCopy (base_modelorg, modelorg);
		VectorCopy (oldorigin, modelorg);
		R_TransformFrustum ();

	} // End of for loop

	insubmodel = false;
}


/*
================
R_EdgeDrawing
================
*/
void R_EdgeDrawing (void)
{
	edge_t	ledges[NUMSTACKEDGES + ((CACHE_SIZE - 1) / sizeof(edge_t)) + 1];
	surf_t	lsurfs[NUMSTACKSURFACES + ((CACHE_SIZE - 1) / sizeof(surf_t)) + 1];

	r_edges = (auxedges) ? auxedges : (edge_t *) (((long)&ledges[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));

	if (r_surfsonstack)
	{
		surfaces =  (surf_t *) (((long)&lsurfs[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));
		surf_max = &surfaces[r_cnumsurfs];
	// surface 0 doesn't really exist; it's just a dummy because index 0
	// is used to indicate no edge attached to surface
		surfaces--;
#if id386
		R_SurfacePatch ();
#endif // id386
	}

	R_BeginEdgeFrame ();

	if (sw_r_dspeeds.value)
	{
		rw_time1 = System_DoubleTime ();
	}

	R_RenderWorld ();

	if (sw_r_dspeeds.value)
	{
		rw_time2 = System_DoubleTime ();
		db_time1 = rw_time2;
	}

	R_DrawBEntitiesOnList ();

	if (sw_r_dspeeds.value)
	{
		db_time2 = System_DoubleTime ();
		se_time1 = db_time2;
	}

	if (!sw_r_dspeeds.value)
	{
		S_ExtraUpdate ();	// don't let sound get messed up if going slow
	}

	R_ScanEdges ();
}


/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView_ (void)
{
	byte	warpbuffer[WARP_WIDTH * WARP_HEIGHT];

	r_warpbuffer = warpbuffer;

	if (sw_r_timegraph.value || r_speeds.value || sw_r_dspeeds.value)
		r_time1 = System_DoubleTime ();

	// Baker: Reset frame information
	memset (&frame, 0, sizeof(frame));  // Mirror-in-scene will join this.
	R_SetupFrame ();

#ifdef PASSAGES
	SetVisibilityByPassages ();
#else
	R_MarkLeaves ();	// done here so we know if we're in water
#endif

	R_SetLiquidAlpha ();

// make FDIV fast. This reduces timing precision after we've been running for a
// while, so we don't do it globally.  This also sets chop mode, and we do it
// here so that setup stuff like the refresh area calculations match what's
// done in screen.c
#if id386
	Sys_LowFPPrecision ();
#endif

	if (!cl_entities[0].model || !cl.worldmodel)
		System_Error ("R_RenderView: NULL worldmodel");

	if (!sw_r_dspeeds.value)
	{
		S_ExtraUpdate ();	// don't let sound get messed up if going slow
	}

#ifdef WINQUAKE_STIPPLE_WATERALPHA
	r_foundtranswater = r_wateralphapass = false; // Manoel Kasimier - translucent water
#endif // WINQUAKE_STIPPLE_WATERALPHA
	R_EdgeDrawing ();

	if (!sw_r_dspeeds.value)
	{
		S_ExtraUpdate ();	// don't let sound get messed up if going slow
	}

	if (sw_r_dspeeds.value)
	{
		se_time2 = System_DoubleTime ();  // scan edges time
		de_time1 = se_time2; // draw entities time
	}

	R_DrawEntitiesOnList ();

	if (sw_r_dspeeds.value)
	{
		de_time2 = System_DoubleTime ();  // draw entities time
		dv_time1 = de_time2;  // draw viewmodel time
	}

//	R_DrawViewModel (); moved after particles


#ifdef WINQUAKE_STIPPLE_WATERALPHA
	if (sw_r_dspeeds.value)
	{
		dv_time2 = System_DoubleTime ();
		dp_time1 = System_DoubleTime (); // stipple time
	}

	// Manoel Kasimier - translucent water - begin
	if (r_foundtranswater)
	{
		r_wateralphapass = true;
		R_EdgeDrawing ();
	}
	// Manoel Kasimier - translucent water - end

#endif // WINQUAKE_STIPPLE_WATERALPHA

	if (sw_r_dspeeds.value)
	{
		dv_time2 = System_DoubleTime ();
		dp_time1 = System_DoubleTime (); // particles time
	}

	R_DrawParticles ();
	R_DrawViewModel ();

	if (sw_r_dspeeds.value)
	{
		dv_time2 = System_DoubleTime ();
		dp_time1 = System_DoubleTime (); // dowarp time
	}

	if (r_dowarp)
		D_WarpScreen ();

	View_SetContentsColor (cl.r_viewleaf->contents);

	if (sw_r_timegraph.value)
		R_TimeGraph ();

	if (sw_r_aliasstats.value)
		R_PrintAliasStats ();

	if (r_speeds.value)
		R_PrintTimes ();

	if (sw_r_dspeeds.value)
		R_PrintDSpeeds ();

	if (sw_r_reportsurfout.value && r_outofsurfaces)
		Con_PrintLinef ("Short %d surfaces", r_outofsurfaces);

	if (sw_r_reportedgeout.value && r_outofedges)
		Con_PrintLinef ("Short roughly %d edges", r_outofedges * 2 / 3);

// back to high floating-point precision
#if id386
	Sys_HighFPPrecision ();
#endif
}

void R_RenderView (void)
{
	int		dummy;
	int		delta;

	delta = (byte *)&dummy - r_stack_start;

	if (delta < -10000 || delta > 10000)
	{
#ifdef PLATFORM_OSX

		#pragma message ("Baker: I think the delta check depends on platform specific memory address behavior")
		#pragma message ("Baker: For example, I cannot see it working on Linux")

		// Baker: For whatever reason, this happens if app is hidden on Mac, but isn't an issue
		// Yet, no point in trying to draw.
		return;

#else
		System_Error ("R_RenderView: called without enough stack");
#endif
	}

	if ( Hunk_LowMark() & 3 )
		System_Error ("Hunk is missaligned");

	if ( (long)(&dummy) & 3 )
		System_Error ("Stack is missaligned");

	if ( (long)(&r_warpbuffer) & 3 )
		System_Error ("Globals are missaligned");

	R_RenderView_ ();
}

/*
================
R_InitTurb
================
*/
void R_InitTurb (void)
{
	int		i;

	for (i=0 ; i<(SIN_BUFFER_SIZE) ; i++)
	{
		sintable[i] = AMP + sin(i*3.14159*2/CYCLE)*AMP;
		intsintable[i] = AMP2 + sin(i*3.14159*2/CYCLE)*AMP2;	// AMP2, not 20
	}
}

#endif // !GLQUAKE - WinQuake Software renderer