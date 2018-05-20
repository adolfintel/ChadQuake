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
// gl_rmain.c

#include "quakedef.h"

///////////////////////////////////////////////////////////////////////////////
//  Mirrors -- Baker
///////////////////////////////////////////////////////////////////////////////

float r_modelview_matrix[16];			// world_matrix[16];	// GL_MODELVIEW_MATRIX stored off.
float r_projection_matrix[16];

///////////////////////////////////////////////////////////////////////////////
//  Rendering stats
///////////////////////////////////////////////////////////////////////////////

//johnfitz -- rendering statistics
int rs_brushpolys, rs_aliaspolys, rs_skypolys, rs_particles, rs_fogpolys;
int rs_dynamiclightmaps, rs_brushpasses, rs_aliaspasses, rs_skypasses;
float rs_megatexels;

cbool	envmap;	// true during envmap command capture




cbool r_drawflat_cheatsafe, r_fullbright_cheatsafe, r_lightmap_cheatsafe, r_drawworld_cheatsafe; //johnfitz

/*
=================
R_CullBox -- johnfitz -- replaced with new function from lordhavoc

Returns true if the box is completely outside the frustum
=================
*/
cbool R_CullBox (vec3_t emins, vec3_t emaxs)
{
	int i;
	mplane_t *p;

	if (frame.in_mirror_draw) 
		return false; // Simply impossible to apply frustum culling?

	for (i = 0;i < 4;i++)
	{
		p = r_frustum + i;
		switch(p->signbits)
		{
		default:
		case 0:
			if (p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2] < p->dist)
				return true;
			break;
		case 1:
			if (p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2] < p->dist)
				return true;
			break;
		case 2:
			if (p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2] < p->dist)
				return true;
			break;
		case 3:
			if (p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2] < p->dist)
				return true;
			break;
		case 4:
			if (p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2] < p->dist)
				return true;
			break;
		case 5:
			if (p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2] < p->dist)
				return true;
			break;
		case 6:
			if (p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2] < p->dist)
				return true;
			break;
		case 7:
			if (p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2] < p->dist)
				return true;
			break;
		}
	}
	return false;
}
/*
===============
R_CullModelForEntity -- johnfitz -- uses correct bounds based on rotation
===============
*/
cbool R_CullModelForEntity (entity_t *e)
{
	vec3_t mins, maxs;

	if (e->angles[0] || e->angles[2]) //pitch or roll
	{
		VectorAdd (e->origin, e->model->rmins, mins);
		VectorAdd (e->origin, e->model->rmaxs, maxs);
	}
	else if (e->angles[1]) //yaw
	{
		VectorAdd (e->origin, e->model->ymins, mins);
		VectorAdd (e->origin, e->model->ymaxs, maxs);
	}
	else //no rotation
	{
		VectorAdd (e->origin, e->model->mins, mins);
		VectorAdd (e->origin, e->model->maxs, maxs);
	}

	return R_CullBox (mins, maxs);
}

/*
===============
R_RotateForEntity -- johnfitz -- modified to take origin and angles instead of pointer to entity
===============
*/
void R_RotateForEntity (vec3_t origin, vec3_t angles)
{
	eglTranslatef (origin[0],  origin[1],  origin[2]);
	eglRotatef (angles[1],  0, 0, 1);
	eglRotatef (-angles[0],  0, 1, 0);
	eglRotatef (angles[2],  1, 0, 0);
}

/*
=============
GL_PolygonOffset -- johnfitz

negative offset moves polygon closer to camera
=============
*/
void GL_PolygonOffset (int offset)
{
	if (offset > 0)
	{
		eglEnable (GL_POLYGON_OFFSET_FILL);
		eglEnable (GL_POLYGON_OFFSET_LINE);
		eglPolygonOffset(1, offset);
	}
	else if (offset < 0)
	{
		eglEnable (GL_POLYGON_OFFSET_FILL);
		eglEnable (GL_POLYGON_OFFSET_LINE);
		eglPolygonOffset(-1, offset);
	}
	else
	{
		eglDisable (GL_POLYGON_OFFSET_FILL);
		eglDisable (GL_POLYGON_OFFSET_LINE);
	}
}

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

	scale_forward = cos (Degree_To_Radians( angle ));
	scale_side = sin (Degree_To_Radians( angle ));

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

	if (r_lockfrustum.value)
		return;		// Do not update!

	if (gl_stereo.value)
		fovx += 10; //silly hack so that polygons don't drop out becuase of stereo skew

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

/*
=============
GL_SetFrustum -- johnfitz -- written to replace MYgluPerspective
=============
*/

#define NEARCLIP 1
float frustum_skew = 0.0; //used by r_stereo
void GL_SetFrustum(float fovx, float fovy)
{
	float xmax, ymax;
	xmax = NEARCLIP * tan( fovx * M_PI / 360.0 );
	ymax = NEARCLIP * tan( fovy * M_PI / 360.0 );
	eglFrustum(-xmax + frustum_skew, xmax + frustum_skew, -ymax, ymax, NEARCLIP, gl_farclip.value);
}

/*
=============
R_SetupGL
=============
*/
// We are never called by mirror drawing
static void R_RenderScene_SetupScene_SetupGL (void)
{
#if 0 // Baker:  For testing
	vrect_t area =  {glx + r_refdef.vrect.x, gly + glheight - r_refdef.vrect.y - r_refdef.vrect.height, r_refdef.vrect.width, r_refdef.vrect.height};
#endif

	//johnfitz -- rewrote this section
	eglMatrixMode(GL_PROJECTION);
		eglLoadIdentity ();

		if (frame.do_glwarp)
		{
			// this will go down into sbar territory so refresh it
			eglViewport (0, 0, vid.maxwarpwidth, vid.maxwarpheight);
			Sbar_Changed ();
		}
		else
			eglViewport (clx + r_refdef.vrect.x,
					cly + clheight - r_refdef.vrect.y - r_refdef.vrect.height,
					r_refdef.vrect.width,
					r_refdef.vrect.height);
		//johnfitz

		GL_SetFrustum (r_fovx, r_fovy); //johnfitz -- use r_fov* vars

	eglGetFloatv (GL_PROJECTION_MATRIX, r_projection_matrix); // Store off the model view matrix.  (r_world_matrix)

	eglMatrixMode(GL_MODELVIEW);
		eglLoadIdentity ();

		eglRotatef (-90,  1, 0, 0);	    // put Z going up
		eglRotatef (90,  0, 0, 1);	    // put Z going up
		eglRotatef (-r_refdef.viewangles[2],  1, 0, 0);
		eglRotatef (-r_refdef.viewangles[0],  0, 1, 0);
		eglRotatef (-r_refdef.viewangles[1],  0, 0, 1);
		eglTranslatef (-r_refdef.vieworg[0],  -r_refdef.vieworg[1],  -r_refdef.vieworg[2]);

	eglGetFloatv (GL_MODELVIEW_MATRIX, r_modelview_matrix); // Store off the model view matrix.  (r_world_matrix)

	//
	// set drawing parms
	//
	if (gl_cull.value)
		eglEnable(GL_CULL_FACE);
	else
		eglDisable(GL_CULL_FACE);

	// Make sure these guys are properly set in mirrors
	eglDisable(GL_BLEND);
	eglDisable(GL_ALPHA_TEST);
	eglEnable(GL_DEPTH_TEST);
}

/*
=============
R_Clear -- johnfitz -- rewritten and gutted
=============
*/
// Only R_SetupView
static void R_SetupView_R_Clear (void)
{
	unsigned int clearbits;
	
	clearbits = GL_DEPTH_BUFFER_BIT;
	if (gl_clear.value || renderer.isIntelVideo  || (sv.active && sv_player->v.movetype == MOVETYPE_NOCLIP) )  // Baker: -1 means never clear color
		// Baker: gl_clear -1 never clears unless movetype noclip, allows me to simulate behavior on non-Intel cards
		if (gl_clear.value != - 1 || (sv.active && sv_player->v.movetype == MOVETYPE_NOCLIP))
			clearbits |= GL_COLOR_BUFFER_BIT; //intel video workarounds from Baker
//	if (renderer.gl_stencilbits) // Clear buffer upon any function needing to use it.  Per frame is irrelevant.
#if 1 // We should probably always do this
	clearbits |= GL_STENCIL_BUFFER_BIT;
#endif // We should probably always do this

	eglClear (clearbits);


	eglDepthRange (Q_GLDEPTHRANGE_MIN_0_0, Q_GLDEPTHRANGE_MAX_0_5);


}


/*
===============
R_SetupScene -- johnfitz -- this is the stuff that needs to be done once per eye in stereo mode
===============
*/
static void R_RenderScene_SetupScene (void)
{
	Stain_FrameSetup_LessenStains (false);
	cl.r_framecount++;
	R_PushDlights_World (); // temp
	R_AnimateLight ();
	R_RenderScene_SetupScene_SetupGL ();
}

/*
===============
R_SetupView -- johnfitz -- this is the stuff that needs to be done once per frame, even in stereo mode
===============
*/
// Is not called by mirrors
void R_SetupView (void)
{

	Fog_SetupFrame (); //johnfitz

// Setup per frame variables
	frame.oldwater = (gl_oldwater.value || (vid.direct3d == 8) );

// build the transformation matrix for the given view angles
	VectorCopy (r_refdef.vieworg, r_origin);
	AngleVectors (r_refdef.viewangles, vpn, vright, vup);

// current viewleaf
	cl.r_oldviewleaf = cl.r_viewleaf;

	if (r_lockpvs.value == 0) { // Don't update if PVS is locked
		if (chase_active.value && chase_mode) // Camera position isn't visibility leaf!
			cl.r_viewleaf = Mod_PointInLeaf (nonchase_origin, cl.worldmodel);
		else
			cl.r_viewleaf = Mod_PointInLeaf (r_origin, cl.worldmodel);
	}

	if (frame.in_mirror_draw == false) {
		// This is pure once per frame stuff.  We do not recalcuate for the mirror pass.

		View_SetContentsColor (cl.r_viewleaf->contents);
		View_CalcBlend ();
	}

	//johnfitz -- calculate r_fovx and r_fovy here
	r_fovx = r_refdef.fov_x;
	r_fovy = r_refdef.fov_y;

	
	//r_dowarp = false; // UNDERWATER_WARP.  Our frame structure should have already been cleared so we don't need to reset.
	
	if (r_waterwarp.value)
	{
		int contents = cl.r_viewleaf->contents; // Baker was: Mod_PointInLeaf (r_origin, cl.worldmodel)->contents;
		if (contents == CONTENTS_WATER || contents == CONTENTS_SLIME || contents == CONTENTS_LAVA)
		{
			if (r_waterwarp.value <= 1 && vid.direct3d !=8 ) // dx8 has no CopyTexSubImage, therefore cannot do WinQuake R_WATERWARP
				frame.do_glwarp = true;
			else {	
				//variance is a percentage of width, where width = 2 * tan(fov / 2) otherwise the effect is too dramatic at high FOV and too subtle at low FOV.  what a mess!
				r_fovx = atan(tan(Degree_To_Radians(r_refdef.fov_x) / 2) * (0.97 + sin(cl.ctime * 1.5) * 0.03)) * 2 / M_PI_DIV_180;
				r_fovy = atan(tan(Degree_To_Radians(r_refdef.fov_y) / 2) * (1.03 - sin(cl.ctime * 1.5) * 0.03)) * 2 / M_PI_DIV_180;
			}
		}
	}
	//johnfitz

	if (frame.in_mirror_draw == false /* because if true we already did all of this for the frame ... well mostly */) {
		R_SetFrustum (r_fovx, r_fovy); //johnfitz -- use r_fov* vars
		R_MarkSurfaces (); //johnfitz -- create texture chains from PVS
		R_CullSurfaces (); //johnfitz -- do after R_SetFrustum and R_MarkSurfaces

		if (vid.warp_stale == true)
		{
			TexMgr_R_SetupView_RecalcWarpImageSize ();
			TexMgr_R_SetupView_InitUnderwaterWarpTexture ();
			GL_Bloom_RecalcImageSize ();
			vid.warp_stale = false;
		}

		R_SetupView_UpdateWarpTextures (); //johnfitz -- do this before R_Clear

		R_SetupView_R_Clear ();

		//johnfitz -- cheat-protect some draw modes
		r_drawflat_cheatsafe = r_fullbright_cheatsafe = r_lightmap_cheatsafe = false;

		// Baker: Strangely, FitzQuake 0.85 doesn't need to do this here to make sure that
		// a fullbright map in multiplayer is fullbrighted like it should be.
		// But for reasons beyond explanation, Mark V needs this.  The only factor is
		// cl.maxlients.

		if (!cl.worldmodel->lightdata)
			r_fullbright_cheatsafe = true;

		r_drawworld_cheatsafe = true;
		if (cl.maxclients == 1 || cls.demoplayback)
		{
			if (!gl_drawworld.value) r_drawworld_cheatsafe = false;

			if (r_drawflat.value) r_drawflat_cheatsafe = true;
			else if (r_fullbright.value || !cl.worldmodel->lightdata) r_fullbright_cheatsafe = true;
			else if (gl_lightmap.value) r_lightmap_cheatsafe = true;
		}
		//johnfitz
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
enum model_draw_type_e {
	model_draw_type_brush = 1,
	model_draw_type_not_brush = 2,
	model_draw_type_any = 3,
};

static void R_DrawEntitiesOnList (enum model_draw_type_e model_draw_type, cbool alphapass) //johnfitz -- added parameter
{
	int		i;

	if (!r_drawentities.value)
		return;

	//johnfitz -- sprites are not a special case
	for (i=0 ; i < cl.numvisedicts ; i++)
	{
		currententity = cl.visedicts[i];

		if (model_draw_type == model_draw_type_brush && currententity->model->type != mod_brush)
			continue;

		if (model_draw_type == model_draw_type_not_brush && currententity->model->type == mod_brush)
			continue;

		//johnfitz -- if alphapass is true, draw only alpha entites this time
		//if alphapass is false, draw only nonalpha entities this time
		if ((ENTALPHA_DECODE(currententity->alpha) < 1 && !alphapass) ||
			(ENTALPHA_DECODE(currententity->alpha) == 1 && alphapass))
			continue;

		// Checks if a flame model and if QMB replaces we do not draw here
		if (qmb_is_available /* this must happen even if qmb is off */ && QMB_FlameModelSetState(currententity))
			continue;

		//johnfitz -- chasecam
		if (currententity == &cl_entities[cl.viewentity_player] && chase_active.value)
			currententity->angles[0] *= 0.3;
		//johnfitz


		
#if 1 // Baker: Rare.  Need to investigate relink entities and Nehahra support.  MODEL_CRUTCH
		if  (currententity->model == NULL && currententity->modelindex)
			currententity->model = cl.model_precache[currententity->baseline.modelindex];
#endif

		switch (currententity->model->type)
		{
			case mod_alias:
#ifdef SUPPORTS_NEHAHRA
				if (nehahra_active && !strcmp("progs/null.mdl", currententity->model->name))
					break; // Baker: TO DO: Something should have filtered this out.  No, it is carrying an effect or something.
#endif // SUPPORTS_NEHAHRA
					R_DrawAliasModel (currententity);
				break;
			case mod_brush:
					R_DrawBrushModel (currententity);
				break;
			case mod_sprite:
					R_DrawSpriteModel (currententity);
				break;
		}
	}
}



void R_DrawPlayerModel (entity_t *ent) //Baker --- for mirrors
{
	if (!r_drawentities.value)
		return;

	if (ent->coloredskin == NULL || R_SkinTextureChanged (ent))
		ent->coloredskin = R_TranslateNewModelSkinColormap (ent);

	currententity = ent;

// Baker: Is this working with alpha?  I have no reason to think it doesn't but there is a comment I left here
// Dec 2016: I'm pretty sure that it is.  Nothing undesirable has come up in testing.

#if 1 // Baker: Rare.  Need to investigate relink entities and Nehahra support.  E1M5 occasional intermission crash due to player.mdl not being resolved.  MODEL_CRUTCH
		if (currententity->model == NULL && currententity->modelindex)
			currententity->model = cl.model_precache[currententity->baseline.modelindex];
#endif 		

	switch (currententity->model->type)
	{
		case mod_alias:
				R_DrawAliasModel (currententity);
			break;
		case mod_brush:
				R_DrawBrushModel (currententity);
			break;
		case mod_sprite:
				R_DrawSpriteModel (currententity);
			break;
	}
}



/*
=============
R_DrawViewModel -- johnfitz -- gutted
=============
*/
void R_DrawViewModel (void)
{
	if (!r_drawviewmodel.value)
		return;

 	if (chase_active.value)
		return;

	if (!r_drawentities.value)
		return;

#ifdef GLQUAKE_RENDERER_SUPPORT
	if (envmap)
		return;
#endif // GLQUAKE_RENDERER_SUPPORT

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

	// hack the depth range to prevent view model from poking into walls
	eglDepthRange (Q_GLDEPTHRANGE_MIN_0_0, Q_GLDEPTHRANGE_GUN_0_125);
	eglEnable (GL_DEPTH_TEST);
	eglEnable (GL_CULL_FACE);
	R_DrawAliasModel (currententity);
	eglDepthRange (Q_GLDEPTHRANGE_MIN_0_0, Q_GLDEPTHRANGE_MAX_0_5);
}

/*
================
R_EmitWirePoint -- johnfitz -- draws a wireframe cross shape for point entities
================
*/
void R_EmitWirePoint (vec3_t origin)
{
	const int size = 8;

	eglBegin (GL_LINES);
	eglVertex3f (origin[0]-size, origin[1], origin[2]);
	eglVertex3f (origin[0]+size, origin[1], origin[2]);
	eglVertex3f (origin[0], origin[1]-size, origin[2]);
	eglVertex3f (origin[0], origin[1]+size, origin[2]);
	eglVertex3f (origin[0], origin[1], origin[2]-size);
	eglVertex3f (origin[0], origin[1], origin[2]+size);
	eglEnd ();
}

/*
================
R_EmitWireBox -- johnfitz -- draws one axis aligned bounding box
================
*/
void R_EmitWireBox (vec3_t mins, vec3_t maxs)
{
	eglBegin (GL_QUAD_STRIP);
	eglVertex3f (mins[0], mins[1], mins[2]);
	eglVertex3f (mins[0], mins[1], maxs[2]);
	eglVertex3f (maxs[0], mins[1], mins[2]);
	eglVertex3f (maxs[0], mins[1], maxs[2]);
	eglVertex3f (maxs[0], maxs[1], mins[2]);
	eglVertex3f (maxs[0], maxs[1], maxs[2]);
	eglVertex3f (mins[0], maxs[1], mins[2]);
	eglVertex3f (mins[0], maxs[1], maxs[2]);
	eglVertex3f (mins[0], mins[1], mins[2]);
	eglVertex3f (mins[0], mins[1], maxs[2]);
	eglEnd ();
}



static void R_EmitBox_Faces (const vec3_t mins, const vec3_t maxs)
{
	eglBegin (GL_QUADS);

    // Front Face
    eglVertex3f (mins[0], mins[1], maxs[2]);
    eglVertex3f (maxs[0], mins[1], maxs[2]);
    eglVertex3f (maxs[0], maxs[1], maxs[2]);
    eglVertex3f (mins[0], maxs[1], maxs[2]);
    // Back Face
    eglVertex3f (mins[0], mins[1], mins[2]);
    eglVertex3f (mins[0], maxs[1], mins[2]);
    eglVertex3f (maxs[0], maxs[1], mins[2]);
    eglVertex3f (maxs[0], mins[1], mins[2]);
    // Top Face
    eglVertex3f (mins[0], maxs[1], mins[2]);
    eglVertex3f (mins[0], maxs[1], maxs[2]);
    eglVertex3f (maxs[0], maxs[1], maxs[2]);
    eglVertex3f (maxs[0], maxs[1], mins[2]);
    // Bottom Face
    eglVertex3f (mins[0], mins[1], mins[2]);
    eglVertex3f (maxs[0], mins[1], mins[2]);
    eglVertex3f (maxs[0], mins[1], maxs[2]);
    eglVertex3f (mins[0], mins[1], maxs[2]);
    // Right face
    eglVertex3f (maxs[0], mins[1], mins[2]);
    eglVertex3f (maxs[0], maxs[1], mins[2]);
    eglVertex3f (maxs[0], maxs[1], maxs[2]);
    eglVertex3f (maxs[0], mins[1], maxs[2]);
    // Left Face
    eglVertex3f (mins[0], mins[1], mins[2]);
    eglVertex3f (mins[0], mins[1], maxs[2]);
    eglVertex3f (mins[0], maxs[1], maxs[2]);
    eglVertex3f (mins[0], maxs[1], mins[2]);

	eglEnd ();
}

Point3D R_EmitSurfaceHighlight (entity_t* enty, msurface_t* surf, rgba4_t color, int style)
{
	Point3D center;
	float *verts = surf->polys->verts[0];

	vec3_t mins = { 99999,  99999,  99999};
	vec3_t maxs = {-99999, -99999, -99999};
	int i;

	if (enty)
	{
		eglPushMatrix ();
		R_RotateForEntity (enty->origin, enty->angles);
	}

	if (style == OUTLINED_POLYGON)	// Set to lines
		eglPolygonMode	(GL_FRONT_AND_BACK, GL_LINE);

	eglDisable (GL_TEXTURE_2D);
	eglEnable (GL_POLYGON_OFFSET_FILL);
	eglDisable (GL_CULL_FACE);
	eglColor4f (color[0], color[1], color[2], color[3]);
	eglDisable (GL_DEPTH_TEST);
	eglEnable (GL_BLEND);

	eglBegin (GL_POLYGON);

	// Draw polygon while collecting information for the center.
	for (i = 0 ; i < surf->polys->numverts ; i++, verts+= VERTEXSIZE)
	{
		VectorExtendLimits (verts, mins, maxs);
		eglVertex3fv (verts);
	}
	eglEnd ();

	eglEnable (GL_TEXTURE_2D);
	eglDisable (GL_POLYGON_OFFSET_FILL);
	eglEnable (GL_CULL_FACE);
	eglColor4f (1, 1, 1, 1);
	eglEnable (GL_DEPTH_TEST);
	eglDisable (GL_BLEND);

	if (style == OUTLINED_POLYGON)	// Set to lines
		eglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

	if (enty)
		eglPopMatrix ();

	// Calculate the center
	VectorAverage (mins, maxs, center.vec3);

	return center;

}

static void R_EmitBox_CalcCoordsMinsMaxs (const vec3_t centerpoint, const vec3_t xyz_mins, const vec3_t xyz_maxs, const float ExpandSize, vec3_t mins, vec3_t maxs)
{	// Calculate mins and maxs
	int		calculation_type = !centerpoint ? 0 : (!xyz_mins ? 2 : 1);

	switch (calculation_type)
	{
	case 0:		// No centerpoint so these represent absolute coordinates
				VectorCopy (xyz_mins, mins);
				VectorCopy (xyz_maxs, maxs);
				break;

				// We have a centerpoint so the other 2 coordinates are relative to the center
	case 1:		VectorAdd (centerpoint, xyz_mins, mins);
				VectorAdd (centerpoint, xyz_maxs, maxs);
				break;

	case 2:		// No mins or maxs (point type entity)
				VectorCopy (centerpoint, mins);
				VectorCopy (centerpoint, maxs);
				break;
	}

	// Optionally, adjust the size a bit like pull the sides back just a little or expand it
	if (ExpandSize)		{ VectorAddFloat (mins,  -ExpandSize, mins);	VectorAddFloat (maxs, ExpandSize, maxs); }

}

vec3_t currentbox_mins, currentbox_maxs;
void R_EmitBox (const vec3_t myColor, const vec3_t centerpoint, const vec3_t xyz_mins, const vec3_t xyz_maxs, const int bTopMost, const int bLines, const float Expandsize)
{
	float	pointentity_size = 8.0f;
	float	smallster = 0.10f;
	vec3_t	mins, maxs;

	// Calculate box size based on what we received (i.e., if we have a centerpoint use it otherwise it is absolute coords)
	R_EmitBox_CalcCoordsMinsMaxs (centerpoint, xyz_mins, xyz_maxs, Expandsize, /* outputs -> */ mins, maxs);

	// Setup to draw

	eglDisable			(GL_TEXTURE_2D);
	eglEnable			(GL_POLYGON_OFFSET_FILL);
	eglDisable			(GL_CULL_FACE);
	eglColor4fv		(myColor);

	if (myColor[3])	// If no alpha then it is solid
	{
		eglEnable		(GL_BLEND);
		eglDepthMask	(GL_FALSE);
	}

	if (bTopMost)	// Draw topmost if specified
		eglDisable		(GL_DEPTH_TEST);

	if (bLines)		// Set to lines
		eglPolygonMode	(GL_FRONT_AND_BACK, GL_LINE);

	if (bLines)
		R_EmitWireBox (mins, maxs);
	else
		R_EmitBox_Faces (mins, maxs);

	//	eglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);		// This is a GL_SetupState default
	eglEnable			(GL_TEXTURE_2D);
	eglDisable			(GL_POLYGON_OFFSET_FILL);
	eglEnable			(GL_CULL_FACE);
	eglColor4f			(1, 1, 1, 1);

	if (myColor[3])	// If no alpha then it is solid
	{
		eglDisable		(GL_BLEND);
		eglDepthMask	(GL_TRUE);
	}

	if (bTopMost)	// Draw topmost if specified
		eglEnable		(GL_DEPTH_TEST);

	if (bLines)		// Set to lines
		eglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

	VectorCopy (mins, currentbox_mins);
	VectorCopy (maxs, currentbox_maxs);
}




void R_EmitCaption_Text_Get_Columns_And_Rows (const char *myText, int *return_columns, int *return_rows)
{
	int i, maxcolumn = 0, numrows = 1, curcolumn = 0;

	for (i=0 ; i < (int)strlen(myText) ; i++)
	{
		if (myText[i] == '\b')
			continue; // Bronzing toggle ... we ignore this

		if (myText[i] == '\n')
		{
			numrows ++;
			curcolumn = 0;
			continue;
		}

		curcolumn ++;
		if (curcolumn > maxcolumn)
			(maxcolumn = curcolumn);
	}

	*return_columns = maxcolumn;
	*return_rows = numrows;

}

typedef struct fRect_s {
	float	left, top, right, bottom;
} fRect_t;


void R_EmitCaption (vec3_t location, const char *caption, cbool backgrounded)
{
	int			string_length = strlen(caption);
	float		charwidth	= 8;
	float		charheight  = 8;

	vec3_t		point;
	vec3_t		right, up;

	int			string_columns, string_rows;
	float		string_width, string_height; // pixels
	fRect_t		captionbox;
	int			i, x, y;
	cbool	bronze = false;

	// Step 1: Detemine Rows and columns
	R_EmitCaption_Text_Get_Columns_And_Rows (caption, &string_columns, &string_rows);

	string_width = (float)string_columns * charwidth;
	string_height = (float)string_rows * charheight;

	// Step 2: Calculate bounds of rectangle

	captionbox.top	  = -(float)string_height/2;
	captionbox.bottom =  (float)string_height/2;
	captionbox.left   = -(float)string_width /2;
	captionbox.right  =  (float)string_width /2;

	// Copy the view origin up and right angles
	VectorCopy (vup, up);
	VectorCopy (vright, right);

	if (backgrounded)
	{
		// Draw box
		eglDisable (GL_TEXTURE_2D);

		eglEnable (GL_POLYGON_OFFSET_FILL);
		eglDisable (GL_CULL_FACE);

		eglDepthRange (Q_GLDEPTHRANGE_GUN_0_125, Q_GLDEPTHRANGE_MAX_0_5 - 0.0125);  // 0.3 to 0.5 ish
		eglColor4f (0,0,0,1);
		eglBegin (GL_QUADS);

		// top left
		VectorMA (location, captionbox.top-2, up, point);
		VectorMA (point, captionbox.left+4, right, point);
		eglVertex3fv (point);

		// top, right
		VectorMA (location, captionbox.top-2, up, point);
		VectorMA (point, captionbox.right+12, right, point);
		eglVertex3fv (point);

		// bottom right
		VectorMA (location, captionbox.bottom+1, up, point);
		VectorMA (point, captionbox.right+12, right, point);
		eglVertex3fv (point);

		// bottom, left
		VectorMA (location, captionbox.bottom+1, up, point);
		VectorMA (point, captionbox.left+4, right, point);
		eglVertex3fv (point);

		eglEnd ();
		eglEnable (GL_TEXTURE_2D);
		eglColor4f (1,1,1,1);

		eglDisable (GL_POLYGON_OFFSET_FILL);
		eglEnable (GL_CULL_FACE);
#pragma message ("Baker: Depth range here ...")
		eglDepthRange (Q_GLDEPTHRANGE_MIN_0_0, Q_GLDEPTHRANGE_MAX_0_5);
	}

	GL_Bind		(char_texture);
	eglDisable	(GL_CULL_FACE);
	eglEnable	(GL_ALPHA_TEST);

	eglDepthRange (Q_GLDEPTHRANGE_GUN_0_125, Q_GLDEPTHRANGE_MAX_0_5 - 0.25);

	for (i = 0, x = 0, y =0; i < string_length; i++)
	{
		// Deal with the string

		if (caption[i] == '\n')
		{
			// Reset the row, ignore and carry on
			x = 0;
			y ++;
			continue;
		}

		if (caption[i] == '\b')
		{
			// Toggle the bronzing and carry on
			bronze = !bronze;
			continue;
		}

		// New character to print
		x++;
		{
			fRect_t		charcoords;
			float		s, t;
			int			charcode  = caption[i] | (bronze * 128);	// "ascii" ish character code, but Quake character code
			int			charrow   = charcode >> 4; // Fancy way of dividing by 16
			int			charcol   = charcode & 15; // column = mod (16-1)

			// Calculate s, t texture coords from character set image

			s = charcol * 0.0625f; // 0.0625 = 1/16th
			t = charrow * 0.0625f;

			// Calculate coords

			// Baker: These calc look a bit "clustered" :(
			charcoords.top	  =	 captionbox.top + ((string_rows - 1 - y)/(float)string_rows)*(captionbox.bottom - captionbox.top);
			charcoords.bottom =  captionbox.top + (((string_rows - 1 - y)+1)/(float)string_rows)*(captionbox.bottom - captionbox.top);
			charcoords.left   =  captionbox.left + (x/(float)string_columns)*(captionbox.right - captionbox.left);
			charcoords.right  =  captionbox.left + ((x+1)/(float)string_columns)*(captionbox.right - captionbox.left);

			// Draw box
			eglBegin (GL_QUADS);

			// top left
			eglTexCoord2f (s, t + 0.0625);
			VectorMA (location, charcoords.top, up, point);
			VectorMA (point, charcoords.left, right, point);
			eglVertex3fv (point);

			// top, right
			eglTexCoord2f (s + 0.0625, t + 0.0625);
			VectorMA (location, charcoords.top, up, point);
			VectorMA (point, charcoords.right, right, point);
			eglVertex3fv (point);

			// bottom right
			eglTexCoord2f (s + 0.0625, t);
			VectorMA (location, charcoords.bottom, up, point);
			VectorMA (point, charcoords.right, right, point);
			eglVertex3fv (point);

			// bottom, left
			eglTexCoord2f (s, t);
			VectorMA (location, charcoords.bottom, up, point);
			VectorMA (point, charcoords.left, right, point);
			eglVertex3fv (point);

			eglEnd ();
		}
	}

	eglEnable (GL_CULL_FACE);
	eglDisable (GL_ALPHA_TEST);
	eglDepthRange (Q_GLDEPTHRANGE_MIN_0_0, Q_GLDEPTHRANGE_MAX_0_5);
}

/*
================
R_ShowBoundingBoxes -- johnfitz

draw bounding boxes -- the server-side boxes, not the renderer cullboxes
================
*/
void R_ShowBoundingBoxes (void)
{
	extern		edict_t *sv_player;
	vec3_t		mins,maxs;
	edict_t		*ed;
	int			i;

	if (!gl_showbboxes.value || !sv.active || cl.maxclients > 1 || !r_drawentities.value)
		return;

	eglDisable (GL_DEPTH_TEST);
	eglPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	GL_PolygonOffset (OFFSET_SHOWTRIS);
	eglDisable (GL_TEXTURE_2D);
	eglDisable (GL_CULL_FACE);
	eglColor3f (1,1,1);

	for (i = 0, ed = NEXT_EDICT(sv.edicts) ; i < sv.num_edicts ; i++, ed = NEXT_EDICT(ed))
	{
		if (ed == sv_player)
			continue; //don't draw player's own bbox

//		if (r_showbboxes.value != 2)
//			if (!SV_VisibleToClient (sv_player, ed, sv.worldmodel))
//				continue; //don't draw if not in pvs

		if (ed->v.mins[0] == ed->v.maxs[0] && ed->v.mins[1] == ed->v.maxs[1] && ed->v.mins[2] == ed->v.maxs[2])
		{
			//point entity
			R_EmitWirePoint (ed->v.origin);
		}
		else
		{
			//box entity
			VectorAdd (ed->v.mins, ed->v.origin, mins);
			VectorAdd (ed->v.maxs, ed->v.origin, maxs);
			R_EmitWireBox (mins, maxs);
		}
	}

	eglColor3f (1,1,1);
	eglEnable (GL_TEXTURE_2D);
	eglEnable (GL_CULL_FACE);
	eglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	GL_PolygonOffset (OFFSET_NONE);
	eglEnable (GL_DEPTH_TEST);

	Sbar_Changed (); //so we don't get dots collecting on the statusbar
}

/*
================
R_ShowTris -- johnfitz
================
*/
void R_ShowTris (void)
{
	int i;

	if (gl_showtris.value < 1 || gl_showtris.value > 2 || cl.maxclients > 1)
		return;

	if (gl_showtris.value == 1)
		eglDisable (GL_DEPTH_TEST);
	eglPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	GL_PolygonOffset (OFFSET_SHOWTRIS);
	eglDisable (GL_TEXTURE_2D);
	eglColor3f (1,1,1);
//	eglEnable (GL_BLEND);
//	eglBlendFunc (GL_ONE, GL_ONE);

	if (gl_drawworld.value)
	{
		R_DrawTextureChains_ShowTris ();
	}

	if (r_drawentities.value)
	{
		for (i=0 ; i < cl.numvisedicts ; i++)
		{
			currententity = cl.visedicts[i];

			if (currententity == &cl_entities[cl.viewentity_player]) // chasecam
				currententity->angles[0] *= 0.3;

			switch (currententity->model->type)
			{
			case mod_brush:
				R_DrawBrushModel_ShowTris (currententity);
				break;
			case mod_alias:
				R_DrawAliasModel_ShowTris (currententity);
				break;
			case mod_sprite:
				R_DrawSpriteModel (currententity);
				break;
			default:
				break;
			}
		}

		// viewmodel
		currententity = &cl.viewent_gun;
		if (r_drawviewmodel.value
			&& !chase_active.value
			&& !envmap
			&& cl.stats[STAT_HEALTH] > 0
			&& !((cl.items & IT_INVISIBILITY) && !r_viewmodel_ring.value)
			&& currententity->model
			&& currententity->model->type == mod_alias)
		{
			eglDepthRange (Q_GLDEPTHRANGE_MIN_0_0, Q_GLDEPTHRANGE_GUN_0_125);
			R_DrawAliasModel_ShowTris (currententity);
			eglDepthRange (Q_GLDEPTHRANGE_MIN_0_0, Q_GLDEPTHRANGE_MAX_0_5);
		}
	}

	if (gl_particles.value)
	{
		R_DrawParticles_ShowTris ();
	}

//	eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	eglDisable (GL_BLEND);
	eglColor3f (1,1,1);
	eglEnable (GL_TEXTURE_2D);
	eglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	GL_PolygonOffset (OFFSET_NONE);
	if (gl_showtris.value == 1)
		eglEnable (GL_DEPTH_TEST);

	Sbar_Changed (); //so we don't get dots collecting on the statusbar
}

/*
================
R_DrawShadows
================
*/

void R_DrawShadows (void)
{
	int i;

	if (!gl_shadows.value || !r_drawentities.value || r_drawflat_cheatsafe || r_lightmap_cheatsafe) // 3 = SHADOW VOLUMES
		return;

	if (gl_shadows.value >= 3) // SHADOW_VOLUME means we don't do this.
		return;

	//if (renderer.gl_stencilbits && vid.direct3d != 9 /*temp disable hack*/ )
	if (renderer.gl_stencilbits DIRECT3D9_STENCIL_DISABLE_BLOCK_AND)
	{
		eglClearStencil (0); // Regular stencil shadows clear to 0
		eglClear(GL_STENCIL_BUFFER_BIT);
		eglStencilFunc(GL_EQUAL, 0, ~0); // Stencil Sky Fix - 2015 April 13 Per Spike
		eglStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
		eglEnable(GL_STENCIL_TEST);
	}

	for (i = 0 ; i < cl.numvisedicts ; i++)
	{
		currententity = cl.visedicts[i];

#if 1 // Baker: Rare.  Need to investigate relink entities and Nehahra support.  E1M5 occasional intermission crash due to player.mdl not being resolved.  MODEL_CRUTCH
		if (currententity->model == NULL && currententity->modelindex)
			currententity->model = cl.model_precache[currententity->baseline.modelindex];
#endif 		

		if (currententity->model->type != mod_alias)
			continue;

		if (currententity == &cl.viewent_gun)
			return;

		GL_DrawAliasShadow (currententity);
	}
	
	//if (renderer.gl_stencilbits && vid.direct3d != 9 /*temp disable hack*/)
	if (renderer.gl_stencilbits DIRECT3D9_STENCIL_DISABLE_BLOCK_AND)
	{
		eglDisable(GL_STENCIL_TEST);
	}

}


cbool R_MirrorScan_SetMirrorAlpha (void)
{
	int			i;
	texture_t	*t;
	float		alpha_possible;

	frame.mirroralpha = 1;  //frame.has_mirror = false; // This is already set, we memset 0 it.

	if (!renderer.gl_stencilbits) // For reasons unclear to me, Direct3D 8 crashes.  In theory Direct3D 8 should work, just with sky problems.  But it infinite loops.  Update: MH said Direct3D 8 wrapper doesn't support
		return false; // Or should we force the skybox to false?

	if (!level.mirror)
		return false; // Can't!  No need to think about it.
		
	if (cl.r_viewleaf->contents == CONTENTS_SOLID) // Pulsar reported
		return false; // in solid brush ... too bad, so sad ...

	level.is_mirroralpha > 0 ? level.mirroralpha : CLAMP(0, gl_mirroralpha.value, 1.0);
	alpha_possible = level.is_mirroralpha > 0 ? level.mirroralpha : CLAMP(0, gl_mirroralpha.value, 1.0);

	if (alpha_possible == 1)
		return false;	// Can't because we aren't using it.

	for (i = 0; i < cl.worldmodel->numtextures; i++) {
		msurface_t	*s;
		t = cl.worldmodel->textures[i];

		if (!t || !t->texturechain || t->texturechain->flags & (SURF_DRAWTILED | SURF_NOTEXTURE)) // Make a draw mirror pass using flags.
			continue;
		
		if (!Flag_Check (t->texturechain->flags, SURF_DRAWMIRROR) )
			continue;

		// Scan for mirrors.
		for (s = t->texturechain; s; s = s->texturechain) {
			if (s->culled)
				continue;

			// Mirror found!
			frame.has_mirror = true;
			frame.mirroralpha = alpha_possible; // level.is_mirroralpha > 0 ? level.mirroralpha : CLAMP(0, gl_mirroralpha.value, 1.0);
			frame.mirror_plane = s->plane;
			return true;
		}				

	}

	// Extra work to do here.
	
	if (!r_drawentities.value)
		return false;

	for (i = 0 ; i < cl.numvisedicts; i++) {
		entity_t *ent = cl.visedicts[i];
		if (!ent->model || ent->model->type != mod_brush || !ent->model->mirror_only_surface)
			continue; // No mirrors.

		// Weakness.  We don't actually know if it is facing towards us.
		// Mirror found!  It's on an entity
		frame.has_mirror = true;
		frame.mirroralpha = alpha_possible; // level.is_mirroralpha > 0 ? level.mirroralpha : CLAMP(0, gl_mirroralpha.value, 1.0);
		frame.mirror_plane = ent->model->mirror_plane;
		return true;
	}

	return false;
}

/*
================
R_RenderScene
================
*/

void R_RenderScene (void)
{
	R_RenderScene_SetupScene (); //johnfitz -- this does everything that should be done once per call to RenderScene

	Fog_EnableGFog (); //johnfitz

	if (R_MirrorScan_SetMirrorAlpha ())
		R_Mirror (); // Draw plus enforce depth

	Sky_FrameSetup (); // Baker ... This will draw the sky if no stencil

	R_DrawWorld ();	// This uploads changed lightmaps.

	R_SetLiquidAlpha ();

	S_ExtraUpdate (); // don't let sound get messed up if going slow

	R_DrawShadows (); //johnfitz -- render entity shadows  

	R_DrawEntitiesOnList (model_draw_type_brush, false); //johnfitz -- false means this is the pass for nonalpha entities	

	Sky_Stencil_Draw (); // Baker: This will draw the sky if there is stencil.  // Baker: Shadow volumes glClearStencil (0) + glClear (It does)

	RB_ShadowVolumeBegin(); // Baker: Shadow volumes glClearStencil (0) + glClear	
	
	R_DrawEntitiesOnList (model_draw_type_not_brush, false); //johnfitz -- false means this is the pass for nonalpha entities			

	//R_DrawEntitiesOnList (true /* bmodels */, false); //johnfitz -- false means this is the pass for nonalpha entities	

	R_DrawTextureChains_Water (false); //Baker -- solid warp surfaces here

	R_DrawEntitiesOnList (model_draw_type_any,  true); //johnfitz -- true means this is the pass for alpha entities

	R_DrawTextureChains_Water (true); //johnfitz -- drawn here since they might have transparency

	R_RenderDlights_Flashblend (); //triangle fan dlights -- johnfitz -- moved after water
	
	R_DrawParticles ();

	Fog_DisableGFog (); //johnfitz
	

	// Baker: TexturePointer ignores depth testing so needs to be here
	// before drawing view model but after the world (otherwise the
	// world draws over it).

	TexturePointer_Think ();

	R_DrawViewModel (); //johnfitz -- moved here from R_RenderView

	R_ShowTris (); //johnfitz

	R_ShowBoundingBoxes (); //johnfitz

	RB_ShadowVolumeFinish (); // MH SHADOW_VOLUMES  -- this disturbs the depth buffer bad enough it must be at the end.

	GL_BloomBlend ();  // Blooms

}

#pragma message ("Baker: Does fog and mirrors play nice together?  I bet it does") // They do

void R_Mirror_RenderScene_Partial (void)
{
// Baker: The commented out lines display the differences from the main rendering
//	R_SetupScene (); //johnfitz -- this does everything that should be done once per call to RenderScene

//	Fog_EnableGFog (); //johnfitz
//	Sky_FrameSetup (); // Baker
	
//	if (1 && vid.direct3d != 9 /*temp disable hack*/) // we affect next line
if (1 DIRECT3D9_STENCIL_DISABLE_BLOCK_AND) // The effect of this is that it will block Direct3D 9 from drawing the stencil sky
	Sky_Stencil_Draw (); // Baker, R_RenderScene has this later in the draw.
	R_DrawWorld ();	// This uploads changed lightmaps.

	S_ExtraUpdate (); // don't let sound get messed up if going slow

	R_DrawShadows (); //johnfitz -- render entity shadows

	R_DrawEntitiesOnList (model_draw_type_any, false); //johnfitz -- false means this is the pass for nonalpha entities

	R_DrawPlayerModel (&cl_entities[cl.viewentity_player]); //johnfitz -- moved here from R_RenderView

	R_DrawTextureChains_Water (false); //Baker -- solid warp surfaces here

	R_DrawEntitiesOnList (model_draw_type_any, true); //johnfitz -- true means this is the pass for alpha entities

	R_DrawTextureChains_Water (true); //johnfitz -- drawn here since they might have transparency

	R_RenderDlights_Flashblend (); //triangle fan dlights -- johnfitz -- moved after water

	R_DrawParticles ();  // They are oriented.

	// Other things we don't do in mirror pass
	// Fog, Texturepointer, Viewmodel, ShowTris, ShowBB, R_Mirror 

}


/*
=============
R_Mirror
=============
*/
// Do all the entities that aren't culled, but only do the mirror surfaces.  Capabilities already set.
static void R_Mirror_Entity_Surfaces (void)
{
	// Mirror entity hardening belt.
	if (r_drawentities.value) {
		int i;  for (i = 0 ; i < cl.numvisedicts ; i++) {
			entity_t *ent = cl.visedicts[i];
//			if (!ent->static_mirror_numsurfs)
//				continue; // No mirror surfaces.

			//if (!ent->model || ent->model->type != mod_brush) // Can't happen
			if (!ent->model || ent->model->type != mod_brush || !ent->model->mirror_only_surface)
				continue;

			if (memcmp (frame.mirror_plane, ent->model->mirror_plane, sizeof(frame.mirror_plane[0])))
				continue; // Not right mirror plane.  Two mirrors in frame.
			
			//R_DrawBrushModel (ent);
			if (R_CullModelForEntity(ent) == false /*not culled*/) {
				// Draw it.  We want to have a little bit of control over this.
				qmodel_t	*clmodel = ent->model;
				msurface_t	*surf = clmodel->mirror_only_surface;   //&clmodel->surfaces[clmodel->firstmodelsurface];
				int j;
				
				// We are operating on the idea that origin and angles must be zero.
				// No transation or anything else.  We could just use vieworg.
				VectorSubtract (r_refdef.vieworg, ent->origin, modelorg); // Shouldn't need to do this technically, we should be at zero!
				for (j = 0 ; j < clmodel->nummodelsurfaces; j++, surf++) {
					
						
					if (surf == clmodel->mirror_only_surface) {
						// Normally
						float dot = DotProduct (modelorg, clmodel->mirror_plane->normal  /*pplane->normal*/) - clmodel->mirror_plane->dist /*pplane->dist*/;
						if (       (  Flag_Check(surf->flags, SURF_PLANEBACK) &&  dot < -BACKFACE_EPSILON )
								|| ( !Flag_Check(surf->flags, SURF_PLANEBACK) &&  dot >  BACKFACE_EPSILON )      ) {
							//glpoly_t	*polys = surf->polys;
							texture_t	*texture = surf->texinfo->texture; // R_TextureAnimation (, ent->frame);  // We don't animate
							
							GL_Bind (texture->gltexture);
							DrawGLPoly (surf->polys, 0); // I'm thinking texturefx on missing texture would be bad
							rs_brushpolys++; // Won't kill us
						}
					}
					// 
				} // For each surface
			}
		} // For loop
	}


}

float DotProductEx (const vec3_t vorg, const mplane_t *pplane)
{
	float dot;
	switch (pplane->type)
	{
	case PLANE_X_0:
		dot = vorg[0] - pplane->dist;
		break;
	case PLANE_Y_1:
		dot = vorg[1] - pplane->dist;
		break;
	case PLANE_Z_2:
		dot = vorg[2] - pplane->dist;
		break;
	default:
		//dot = DotProduct (r_refdef.vieworg, surf->plane->normal) - surf->plane->dist;
		dot = DotProduct (vorg, pplane->normal) - pplane->dist;
		break;
	}
	return dot;
}

#if 0 // Security camera mock test
void PlayNoise_f (void)
{
	Cbuf_AddText ("play misc/menu2.wav");

}
#endif

void R_Mirror (void)
{
	if (!frame.has_mirror || frame.mirroralpha == 1.0) {
//		Con_SafePrintLinef ("No mirror %d", cl.r_framecount); //RANDOM_FLOAT * 100);
//		Test indicates that it works right in determining whether to do any mirror stuff.  r_mirroralpha 1 = never, and if no mirror in frustum we don't either.
		return;
	}

	// PREPARE FOR MIRROR SURFACE DRAW
	{
		// Are we stencilling?  I don't actually think we are.  I think we are depth testing.  //eglDisable (GL_STENCIL_TEST);
		// I think the stencilling is just for the sky.		
		eglDepthMask(GL_FALSE);
		eglColor4f (1,0,0,1);
		eglEnable (GL_BLEND);
		eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		GL_DisableMultitexture ();
		eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
#if 1		
		eglColorMask(0, 0, 0, 0); // DEPTH ONLY.  BLOCK ALL COLOR WRITING!
#endif
		frame.in_mirror_overlay = true;
	}

	// Depth only pass.	
	
	// Hardening phase.  Phase 1
	if (gl_drawmirror_world.value >= 1)			R_DrawTextureChains_Multitexture_Mirrors ();
	if (gl_drawmirror_entities.value >= 1)		R_Mirror_Entity_Surfaces ();

	{
		// In mirror textures draw = false
		eglColorMask(1, 1, 1, 1);
		GL_DisableMultitexture ();
		eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		eglColor4f (1, 1, 1, 1);
		eglDisable (GL_BLEND);
		eglDepthMask(GL_TRUE);
		eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		frame.in_mirror_overlay = false;
	}

//	Con_SafePrintLinef ("Mirror %d", cl.r_framecount) ;//RANDOM_FLOAT * 100);

	eglGetFloatv (GL_PROJECTION_MATRIX, frame.original_view.projection);	// Do these now.  Because gun kick or other stuff may have changed them a little.
	eglGetFloatv (GL_MODELVIEW_MATRIX, frame.original_view.modelview);		//					Maybe???
#define MEMCPY_SAVE(x, y) memcpy(x, y, sizeof(y))  //; if (sizeof(x) != sizeof(y)) System_Error ("Size match fail")
#define MEMCPY_RESTORE(x, y) memcpy(y, x, sizeof(x)) //; if (sizeof(x) != sizeof(y)) System_Error ("Size match fail")
	MEMCPY_SAVE (frame.original_view.r_frustum,	r_frustum						);
	MEMCPY_SAVE (frame.original_view.vpn,			vpn							);
	MEMCPY_SAVE (frame.original_view.vright,		vright						);
	MEMCPY_SAVE (frame.original_view.vup,			vup							);
	MEMCPY_SAVE (frame.original_view.vieworg,		r_refdef.vieworg			);
	MEMCPY_SAVE (frame.original_view.viewangles,	r_refdef.viewangles			);
	MEMCPY_SAVE (frame.original_view.r_origin,		r_origin					);
	
	memcpy (&frame.mirror_view, &frame.original_view, sizeof(frame.original_view));

	// Calculate the things we need.
	// 
	{
		float dot;

//Con_PrintLinef ("Current org: %3.2f %3.2f %3.2f", r_refdef.vieworg[0], r_refdef.vieworg[1], r_refdef.vieworg[2] );

		// This ... seems to make no difference
		//dot = DotProduct (r_refdef.vieworg, frame.mirror_plane->normal) - frame.mirror_plane->dist;
		dot = DotProductEx (r_refdef.vieworg, frame.mirror_plane); // - frame.mirror_plane->dist;
		
		VectorMA (r_refdef.vieworg, -2 * dot, frame.mirror_plane->normal, r_refdef.vieworg);
//Con_PrintLinef ("mir: %3.2f %3.2f %3.2f", r_refdef.vieworg[0], r_refdef.vieworg[1], r_refdef.vieworg[2] );

		dot = DotProduct (/*frame.mirror_view.*/ vpn, frame.mirror_plane->normal);
		//dot = DotProductEx (vpn, frame.mirror_plane);
		VectorMA (/*frame.mirror_view.*/ vpn, -2 * dot, frame.mirror_plane->normal, /*frame.mirror_view.*/ vpn);

//Con_Printf ("Current angles: %3.2f %3.2f %3.2f", r_refdef.viewangles[0], r_refdef.viewangles[1], r_refdef.viewangles[2] );
		/// r_refdef.viewangles[0] = -asin (/*frame.mirror_view.*/ vpn[2])/M_PI*180;  orig

// This is pretty much all fuct.
// It's like it is 180 degrees off.  And somehow super sensitive to the mouse?  The slope isn't even much?
#if 0
		r_refdef.viewangles[0] = -asin (/*frame.mirror_view.*/ vpn[2])/M_PI*180;
		r_refdef.viewangles[1] = atan2 (/*frame.mirror_view.*/ vpn[1], /*frame.mirror_view.*/ vpn[0])/M_PI*180;

#else
	// There is something I need to do convert between degrees and euler angles, but can't remember what it is.
	// Under certain conditions I need a 180 degree twist or maybe a full roll angle 180?
	// Plane type 4 - signbits pitch 0 yaw 0.866618 roll .499999

		r_refdef.viewangles[0] = -asin (/*frame.mirror_view.*/ vpn[2])/M_PI*180;
		r_refdef.viewangles[1] = atan2 (/*frame.mirror_view.*/ vpn[1], /*frame.mirror_view.*/ vpn[0])/M_PI*180;
#endif
		//r_refdef.viewangles[2] = -r_refdef.viewangles[2];  original
		//..
		r_refdef.viewangles[2] = -r_refdef.viewangles[2];

		
		//{
		//	float dot;
		//	switch (pplane->type)
		//	{
		//	case PLANE_X_0:
		//		dot = vorg[0] - pplane->dist;
		//		break;
		//	case PLANE_Y_1:
		//		dot = vorg[1] - pplane->dist;
		//		break;
		//	case PLANE_Z_2:
		//		dot = vorg[2] - pplane->dist;
		//		break;
		//	default:
		//		//dot = DotProduct (r_refdef.vieworg, surf->plane->normal) - surf->plane->dist;
		//		dot = DotProduct (vorg, pplane->normal) - pplane->dist;
		//		break;
		//	}
		//	return dot;
		//}
		//

//		switch {


//Con_PrintLinef ("mir: %3.2f %3.2f %3.2f", r_refdef.viewangles[0], r_refdef.viewangles[1], r_refdef.viewangles[2] );
#if 0 // Security camera mock test
		switch ((int)pr_saved1.value) {
		case 0:
			VectorSet (r_refdef.vieworg, -322, -772, -125);
			VectorSet (r_refdef.viewangles, 77,304 ,0);
		
			break;

//Camera: (631 18 -61) 22 311 0
//Viewpos: (631 18 -83) 22 311 0
//Camera: (1055 288 -248) 42 317 0
//Viewpos: (1055 288 -271) 42 317 0
//]copy

		case 1:
//			VectorSet (r_refdef.vieworg, -209, -167, -136);
//			VectorSet (r_refdef.viewangles, 64, 322, 0);
			VectorSet (r_refdef.vieworg, 631, 18, -83);
			VectorSet (r_refdef.viewangles, 22, 311, 0);
			break;

		case 2:
//			VectorSet (r_refdef.vieworg, 2145, -25, -78);
//			VectorSet (r_refdef.viewangles, 27, 219, 0);
			VectorSet (r_refdef.vieworg, 1055, 288, -248);
			VectorSet (r_refdef.viewangles, 42, 317, 0);
			break;
		}
		

#endif // SECURITY CAMERA MOCK

		// These too ...
		AngleVectors (r_refdef.viewangles, vpn, vright, vup); // Split it back out?  Must we negate these?
		VectorCopy (r_refdef.vieworg, r_origin);
	}


	// PROPER BEHAVIOR:  Activate the stencil buffer
	//					 Draw the mirror texture with the stencil operation that says solid where we are
	//					 Render the scene
	//					 Turn the stencil buffer off
	//					 Ironically, we do none of the above but instead use the stencil on the sky for drawing which would be required anyway.


	// SET PROJECTION MATRIX
	eglMatrixMode(GL_PROJECTION);
		switch (!!frame.mirror_plane->normal[2]) {
			case 1:		eglScalef ( 1, -1, 1); break;
			default:	eglScalef (-1,  1, 1); break;
			}
		eglGetFloatv (GL_PROJECTION_MATRIX, frame.mirror_view.projection);

	// MODELVIEW MATRIX
	eglMatrixMode(GL_MODELVIEW);
		eglLoadIdentity ();

		eglRotatef (-90,  1, 0, 0);	    // put Z going up
		eglRotatef (90,  0, 0, 1);	    // put Z going up

		eglRotatef (-r_refdef.viewangles[2],  1, 0, 0);
		eglRotatef (-r_refdef.viewangles[0],  0, 1, 0);
		eglRotatef (-r_refdef.viewangles[1],  0, 0, 1);
		eglTranslatef (-r_refdef.vieworg[0],  -r_refdef.vieworg[1],  -r_refdef.vieworg[2]);

		eglGetFloatv (GL_MODELVIEW_MATRIX, frame.mirror_view.modelview);

	// CAPABILITIES																	// Normally
	{
		eglCullFace(GL_FRONT);															// eglCullFace(GL_BACK);
		eglDepthRange (Q_GLDEPTHRANGE_MIN_MIRROR_0_5, Q_GLDEPTHRANGE_MAX_MIRROR_1_0);	// eglDepthRange (Q_GLDEPTHRANGE_MIN_0_0, Q_GLDEPTHRANGE_MAX_0_5);
		eglDepthFunc (GL_LEQUAL); // This is the normal.

		frame.in_mirror_draw = true;	// ONLY AT THIS POINT ARE WE IN MIRROR DRAW, ALL VARIABLES AND GL CAPS HAVE BEEN SET
	}

#if 0
	// I would need to do more than I feel like doing.
	R_SetFrustum (179.999, 179.999); // Need a maximum fov view because mirror has wider view range
	R_MarkSurfaces (); //johnfitz -- create texture chains from PVS
	R_CullSurfaces (); //johnfitz -- do after R_SetFrustum and R_MarkSurfaces
#else
	R_SetFrustum (360, 360); // Need a maximum fov view because mirror has wider view range
#endif

	// Reverso phase.  Phase 2
	if (gl_drawmirror_world.value >= 2) {			
		R_Mirror_RenderScene_Partial ();
		R_DrawParticles ();  // They are oriented.
	}

	{
		eglDepthRange (Q_GLDEPTHRANGE_MIN_0_0, Q_GLDEPTHRANGE_MAX_0_5);
		eglDepthFunc (GL_LEQUAL);
		eglCullFace(GL_BACK);
		frame.in_mirror_draw = false;
	}

	// RESTORE
	eglMatrixMode(GL_PROJECTION); // Set to projection matrix
	eglLoadMatrixf (frame.original_view.projection);
	eglMatrixMode(GL_MODELVIEW);
	eglLoadMatrixf (frame.original_view.modelview);


	MEMCPY_RESTORE (frame.original_view.r_frustum,	r_frustum					);
	MEMCPY_RESTORE (frame.original_view.vpn,		vpn							);
	MEMCPY_RESTORE (frame.original_view.vright,		vright						);
	MEMCPY_RESTORE (frame.original_view.vup,		vup							);
	MEMCPY_RESTORE (frame.original_view.vieworg,	r_refdef.vieworg			);
	MEMCPY_RESTORE (frame.original_view.viewangles,	r_refdef.viewangles			);
	MEMCPY_RESTORE (frame.original_view.r_origin,	r_origin					);
		r_fovx = r_refdef.fov_x;		// Part of restore.  Particularly for underwater.
		r_fovy = r_refdef.fov_y;


	// Doesn't look like we fixed the mirror scaling


	// PREPARE FOR MIRROR SURFACE DRAW
	{
		// Are we stencilling?  I don't actually think we are.  I think we are depth testing.  //eglDisable (GL_STENCIL_TEST);
		// I think the stencilling is just for the sky.		
		//eglDepthMask(GL_FALSE);
		eglColor4f (1,1,1, frame.mirroralpha);
		eglEnable (GL_BLEND);
		eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		GL_DisableMultitexture ();
		eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		frame.in_mirror_overlay = true;
	}
	
	if (gl_drawmirror_world.value >= 3)		R_DrawTextureChains_Multitexture_Mirrors ();
	if (gl_drawmirror_entities.value >= 3)	R_Mirror_Entity_Surfaces ();

	//R_DrawTextureChains_Multitexture_Mirrors (); // Hit it again.
	{
		// In mirror textures draw = false
		GL_DisableMultitexture ();
		eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		eglColor4f (1, 1, 1, 1);
		eglDisable (GL_BLEND);
		eglDepthMask(GL_TRUE);
		eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		frame.in_mirror_overlay = false;
	}



}


/*
================
R_RenderView
================
*/
void R_RenderView (void)
{
	double	time1, time2;

	if (gl_norefresh.value)
		return;

	if (!cl.worldmodel)
		System_Error ("R_RenderView: NULL worldmodel");

	time1 = 0; /* avoid compiler warning */
	if (r_speeds.value)
	{
		eglFinish ();
		time1 = System_DoubleTime ();

		//johnfitz -- rendering statistics
		rs_brushpolys = rs_aliaspolys = rs_skypolys = rs_particles = rs_fogpolys = rs_megatexels =
		rs_dynamiclightmaps = rs_aliaspasses = rs_skypasses = rs_brushpasses = 0;
	}
	else if (gl_finish.value)
		eglFinish ();

	// Baker: Reset frame information
	memset (&frame, 0, sizeof(frame));  frame.qmb = qmb_is_available && qmb_active.value;  // Mirror-in-scene will join this.

	R_SetupView (); //johnfitz -- this does everything that should be done once per frame

	//johnfitz -- stereo rendering -- full of hacky goodness
	if (gl_stereo.value)
	{
		float eyesep = CLAMP(-8.0f, gl_stereo.value, 8.0f);
		float fdepth = CLAMP(32.0f, gl_stereodepth.value, 1024.0f);

		AngleVectors (r_refdef.viewangles, vpn, vright, vup);

		//render left eye (red)
		eglColorMask(1, 0, 0, 1);
		VectorMA (r_refdef.vieworg, -0.5f * eyesep, vright, r_refdef.vieworg);
		frustum_skew = 0.5 * eyesep * NEARCLIP / fdepth;
		srand((int) (cl.ctime * 1000)); //sync random stuff between eyes

		R_RenderScene ();

		//render right eye (cyan)
		eglClear (GL_DEPTH_BUFFER_BIT);
		eglColorMask(0, 1, 1, 1);
		VectorMA (r_refdef.vieworg, 1.0f * eyesep, vright, r_refdef.vieworg);
		frustum_skew = -frustum_skew;
		srand((int) (cl.ctime * 1000)); //sync random stuff between eyes

		R_RenderScene ();

		//restore
		eglColorMask(1, 1, 1, 1);
		VectorMA (r_refdef.vieworg, -0.5f * eyesep, vright, r_refdef.vieworg);
		frustum_skew = 0.0f;
	}
	else
	{
		R_RenderScene ();
	}
	//johnfitz

	if (frame.do_glwarp) // UNDERWATER_WARP
		GL_WarpScreen ();

	//johnfitz -- modified r_speeds output
	time2 = System_DoubleTime ();
	if (r_speeds.value == 2)
		Con_PrintLinef ("%3d ms  %4d/%4d wpoly %4d/%4d epoly %3d lmap %4d/%4d sky %1.1f mtex",
					(int)((time2-time1)*1000),
					rs_brushpolys,
					rs_brushpasses,
					rs_aliaspolys,
					rs_aliaspasses,
					rs_dynamiclightmaps,
					rs_skypolys,
					rs_skypasses,
					TexMgr_FrameUsage ());
	else if (r_speeds.value)
		Con_PrintLinef ("%3d ms  %4d wpoly %4d epoly %3d lmap",
					(int)((time2-time1)*1000),
					rs_brushpolys,
					rs_aliaspolys,
					rs_dynamiclightmaps);
	//johnfitz
}

#endif // GLQUAKE specific