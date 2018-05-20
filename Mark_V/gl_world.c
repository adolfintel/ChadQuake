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
// r_world.c: world model rendering



#include "quakedef.h"


//extern glpoly_t	*lightmap_polys[MAX_FITZQUAKE_LIGHTMAPS];

byte *SV_FatPVS (vec3_t org, qmodel_t *worldmodel);
extern byte mod_novis[MAX_MAP_LEAFS/8];
int vis_changed; //if true, force pvs to be refreshed

#define ACTIVE_MIRROR(_s) (frame.has_mirror && (Flag_Check ((_s)->flags, SURF_DRAWMIRROR) && frame.mirror_plane && (_s)->plane && !memcmp (frame.mirror_plane, (_s)->plane, sizeof(frame.mirror_plane[0]))))
#define DRAW_ACTIVE_MIRROR (active_mirror && frame.in_mirror_overlay)


//==============================================================================
//
// SETUP CHAINS
//
//==============================================================================

/*
===============
R_MarkSurfaces -- johnfitz -- mark surfaces based on PVS and rebuild texture chains
===============
*/
void R_MarkSurfaces (void)
{
	byte		*vis;
	mleaf_t		*leaf;
	mnode_t		*node;
	msurface_t	*surf, **mark;
	int			 i, j;

	// Do we concern ourselves with this for mirrors?
	// clear lightmap chains
//	if (!frame.in_mirror_draw) {
	for (i = 0; i < MAX_FITZQUAKE_LIGHTMAPS; i++)
		lightmap[i].polys = NULL;
//	}

// mirror:  5. (DONE) If there is a mirror.  We don't update the framecount or set the old leaf or recalculate vis or mark visible leafs
// This only happens AFTER we've been here once to notice (possibly below)
	if (frame.has_mirror)
		goto skip_for_mirror; // This is cute.


	// check this leaf for water portals
	// TODO: loop through all water surfs and use distance to leaf cullbox

	for (i = 0, mark = cl.r_viewleaf->firstmarksurface; i < cl.r_viewleaf->nummarksurfaces; i++, mark++)
		if ((*mark)->flags & SURF_DRAWTURB)
			frame.nearwaterportal = true;

	// choose vis data
	if (r_novis.value || cl.r_viewleaf->contents == CONTENTS_SOLID || cl.r_viewleaf->contents == CONTENTS_SKY)
		vis = &mod_novis[0];
	else if (frame.nearwaterportal)
		vis = SV_FatPVS (r_origin, cl.worldmodel);
	else
		vis = Mod_LeafPVS (cl.r_viewleaf, cl.worldmodel);

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

	// if surface chains don't need regenerating, just add static entities and return
	if (cl.r_oldviewleaf == cl.r_viewleaf && !vis_changed && !frame.nearwaterportal)
	{
		leaf = &cl.worldmodel->leafs[1];
		for (i=0 ; i<cl.worldmodel->numleafs ; i++, leaf++)
			if (vis[i>>3] & (1<<(i&7)))
				if (leaf->efrags)
					R_StoreEfrags (&leaf->efrags);
		// Vis didn't change.  Get out.
		return;
	}

	//Con_PrintLinef ("New vis: %x (key? %d", cl.r_viewleaf, vis);
	vis_changed = false;
	cl.r_visframecount++;
	cl.r_oldviewleaf = cl.r_viewleaf;

	// iterate through leaves, marking surfaces
	leaf = &cl.worldmodel->leafs[1];

	for (i = 0 ; i < cl.worldmodel->numleafs ; i++, leaf++)
	{
		if (vis[i >> 3] & (1 << (i & 7))) // >> 3 = divide by 8
		{
			if (gl_oldskyleaf.value || leaf->contents != CONTENTS_SKY) {
				for (j = 0, mark = leaf->firstmarksurface; j < leaf->nummarksurfaces; j++, mark++) {
					(*mark)->visframe = cl.r_visframecount;
					if ((*mark)->xtraleafs_count) {
						// Irony.
						msurface_t *surf = (*mark), **mark2;
//MARKSURFACES						
						int j2, k2;
						for (j2 = 0; j2 < surf->xtraleafs_count; j2++) {
							mleaf_t	*leafx = surf->xtraleafs[j2];
							for (k2 = 0, mark2 = leafx->firstmarksurface; k2 < leafx->nummarksurfaces; k2++, mark2++) {
								(*mark2)->visframe = cl.r_visframecount;
							}

							// add static models
							if (leafx->efrags)
								R_StoreEfrags (&leafx->efrags);
						}
					}
				}
			}

			// add static models
			if (leaf->efrags)
				R_StoreEfrags (&leaf->efrags);
		}
	}


skip_for_mirror:


	// set all chains to null
	for (i = 0 ; i < cl.worldmodel->numtextures ; i++)
		if (cl.worldmodel->textures[i])
 			cl.worldmodel->textures[i]->texturechain = NULL;

	// rebuild chains

#if 1
	//iterate through surfaces one node at a time to rebuild chains
	//need to do it this way if we want to work with tyrann's skip removal tool
	//becuase his tool doesn't actually remove the surfaces from the bsp surfaces lump
	//nor does it remove references to them in each leaf's marksurfaces list
	for (i = 0, node = cl.worldmodel->nodes ; i < cl.worldmodel->numnodes ; i++, node++)
		for (j = 0, surf = &cl.worldmodel->surfaces[node->firstsurface] ; (unsigned int)j < node->numsurfaces ; j++, surf++)
			if (surf->visframe == cl.r_visframecount)
			{
				surf->texturechain = surf->texinfo->texture->texturechain;
				surf->texinfo->texture->texturechain = surf;
			}
#else
	//the old way
	surf = &cl.worldmodel->surfaces[cl.worldmodel->firstmodelsurface];
	for (i = 0 ; i < cl.worldmodel->nummodelsurfaces ; i++, surf++)
	{
		if (surf->visframe == cl.r_visframecount)
		{
			surf->texturechain = surf->texinfo->texture->texturechain;
			surf->texinfo->texture->texturechain = surf;
		}
	}
#endif
}

/*
================
R_BackFaceCull -- johnfitz -- returns true if the surface is facing away from vieworg
================
*/
cbool R_BackFaceCull (msurface_t *surf)
{
	double dot;

	switch (surf->plane->type)
	{
	case PLANE_X_0:
		dot = r_refdef.vieworg[0] - surf->plane->dist;
		break;
	case PLANE_Y_1:
		dot = r_refdef.vieworg[1] - surf->plane->dist;
		break;
	case PLANE_Z_2:
		dot = r_refdef.vieworg[2] - surf->plane->dist;
		break;
	default:
		dot = DotProduct (r_refdef.vieworg, surf->plane->normal) - surf->plane->dist;
		break;
	}

	if ((dot < 0) ^ !!(surf->flags & SURF_PLANEBACK))
		return true;

	return false;
}

/*
================
R_CullSurfaces -- johnfitz
================
*/
void R_CullSurfaces (void)
{
	msurface_t *s;
	int i;

	if (!r_drawworld_cheatsafe)
		return;

	s = &cl.worldmodel->surfaces[cl.worldmodel->firstmodelsurface];
	for (i=0 ; i<cl.worldmodel->nummodelsurfaces ; i++, s++)
	{
		if (s->visframe == cl.r_visframecount)
		{
			if (R_CullBox(s->mins, s->maxs) || R_BackFaceCull (s))
				s->culled = true;
			else
			{
				s->culled = false;
				rs_brushpolys++; //count wpolys here
				if (s->texinfo->texture->warpimage)
					s->texinfo->texture->update_warp = true;
			}
		}
	}
}

/*
================
R_BuildLightmapChains -- johnfitz -- used for r_lightmap 1
================
*/
void R_BuildLightmapChains (void)
{
	msurface_t *s;
	int i;

	// clear lightmap chains (already done in r_marksurfaces, but clearing them here to be safe becuase of r_stereo)
	for (i=0; i < MAX_FITZQUAKE_LIGHTMAPS; i++)
		lightmap[i].polys = NULL;

	// now rebuild them
	s = &cl.worldmodel->surfaces[cl.worldmodel->firstmodelsurface];
	for (i=0 ; i<cl.worldmodel->nummodelsurfaces ; i++, s++)
		if (s->visframe == cl.r_visframecount && !R_CullBox(s->mins, s->maxs) && !R_BackFaceCull (s))
			R_RenderDynamicLightmaps (s);
}

//==============================================================================
//
// DRAW CHAINS
//
//==============================================================================

/*
================
R_DrawTextureChains_ShowTris -- johnfitz
================
*/
void R_DrawTextureChains_ShowTris (void)
{
	int			i;
	msurface_t	*s;
	texture_t	*t;
	glpoly_t	*p;

	for (i=0 ; i<cl.worldmodel->numtextures ; i++)
	{
		t = cl.worldmodel->textures[i];
		if (!t)
			continue;

		if (frame.oldwater && t->texturechain && (t->texturechain->flags & SURF_DRAWTURB))
		{
			for (s = t->texturechain; s; s = s->texturechain)
				if (!s->culled || frame.in_mirror_draw)  // Hope for the best
					for (p = s->polys->next; p; p = p->next)
					{
						DrawGLTriangleFan (p, s->flags);
					}
		}		
		else
		{
			for (s = t->texturechain; s; s = s->texturechain)
				if (!s->culled || frame.in_mirror_draw) // Hope for the best
				{
					DrawGLTriangleFan (s->polys, s->flags);
				}
		}
	}
}

/*
================
R_DrawTextureChains_Drawflat -- johnfitz
================
*/
void R_DrawTextureChains_Drawflat (void)
{
	int			i;
	msurface_t	*s;
	texture_t	*t;
	glpoly_t	*p;

	for (i=0 ; i<cl.worldmodel->numtextures ; i++)
	{
		t = cl.worldmodel->textures[i];
		if (!t)
			continue;

		if (frame.oldwater && t->texturechain && (t->texturechain->flags & SURF_DRAWTURB))
		{
			for (s = t->texturechain; s; s = s->texturechain)
				if (!s->culled || frame.in_mirror_draw) {
					cbool active_mirror = ACTIVE_MIRROR(s);
					if (active_mirror && !DRAW_ACTIVE_MIRROR)
						continue; 
					for (p = s->polys->next; p; p = p->next)
					{
						srand((unsigned int) (uintptr_t) p);
						eglColor3f (rand()%256/255.0, rand()%256/255.0, rand()%256/255.0);
						DrawGLPoly (p, 0);
						rs_brushpasses++;
					}
				}
		}
		else
		{
			for (s = t->texturechain; s; s = s->texturechain)
				if (!s->culled || frame.in_mirror_draw) {
					cbool active_mirror = ACTIVE_MIRROR(s);
					if (active_mirror && !DRAW_ACTIVE_MIRROR)
						continue; 

					srand((unsigned int) (uintptr_t) s->polys);
					eglColor3f (rand()%256/255.0, rand()%256/255.0, rand()%256/255.0);
					DrawGLPoly (s->polys, 0);
					rs_brushpasses++;
				}
		}
	}
	eglColor3f (1,1,1);
	srand ((int) (cl.ctime * 1000));
}

/*
================
R_DrawTextureChains_Glow -- johnfitz
================
*/
void R_DrawTextureChains_Glow (void)
{
	int			i;
	msurface_t	*s;
	texture_t	*t;
	gltexture_t	*glt;
	cbool	bound;

	for (i=0 ; i<cl.worldmodel->numtextures ; i++)
	{
		t = cl.worldmodel->textures[i];

		if (!t || !t->texturechain || !(glt = R_TextureAnimation(t,0)->fullbright))
			continue;

		bound = false;

		for (s = t->texturechain; s; s = s->texturechain)
			if (!s->culled || frame.in_mirror_draw) // Glow
			{
				// If we are in a mirror frame, we only draw mirrors in overlay.
				cbool active_mirror = ACTIVE_MIRROR(s);
				if (active_mirror && !DRAW_ACTIVE_MIRROR) // Mirrors only draw in overlay
					continue; 

				if (!bound) //only bind once we are sure we need this texture
				{
					GL_Bind (glt);
					bound = true;
				}
				DrawGLPoly (s->polys, s->flags);
				rs_brushpasses++;
			}
	}
}

extern cbool alpha_known;
extern cbool surf_underwater;
extern cbool surf_abovewater;

/*
================
R_DrawTextureChains_Multitexture -- johnfitz
================
*/
static void R_DrawTextureChains_Multitexture_Tex (texture_t *t) //is_mirror_pass)
{
	int			j;
	msurface_t	*s;
	float		*v;
	cbool		bound = false;

	for (s = t->texturechain; s; s = s->texturechain)
	{
		// If we aren't culled *OR* we are in mirror draw, continue ...
		// Remember, we don't use culling for the mirror pass because FOV alone is not enough to determine the true frustum.
		// And determining the true frustum in a mirror scene would be very, very hard to calculate.
#if 1 // Baker: Dec 2
		if (!level.water_vis_known)
		{
			if (s->flags & SURF_UNDERWATER)
				frame.has_underwater = true;
			else
				frame.has_abovewater = true;
		}		
#endif

		if (!s->culled || frame.in_mirror_draw) {
			cbool active_mirror = ACTIVE_MIRROR(s);
#if 0
// Debugging
			cbool surf_mirro = Flag_Check (s->flags, SURF_DRAWMIRROR);
			cbool planematch = frame.mirror_plane && s->plane;
			int mirrorcmp  = frame.mirror_plane && s->plane ? memcmp (frame.mirror_plane, s->plane, sizeof(frame.mirror_plane[0])) : 0;
//cbool test = (frame.has_mirror && (Flag_Check ((_s)->flags, SURF_DRAWMIRROR) && frame.mirror_plane && (_s)->plane && !memcmp (frame.mirror_plane, (_s)->plane, sizeof(frame.mirror_plane[0]))))
//#define DRAW_ACTIVE_MIRROR (active_mirror && frame.in_mirror_overlay)
			if (frame.in_mirror_draw && surf_mirro)
				continue;
#endif 

			if (active_mirror && !DRAW_ACTIVE_MIRROR) // Mirrors only draw in overlay
				continue; 
			if (!bound) //only bind once we are sure we need this texture
			{
				GL_Bind ((R_TextureAnimation(t,0))->gltexture);
				GL_EnableMultitexture(); // selects TEXTURE1
				bound = true;
			}
#if 0
			if (!level.water_vis_known)
			{
				if (s->flags & SURF_UNDERWATER)
					frame.has_underwater = true;
				else
					frame.has_abovewater = true;
			}
#endif
			if (s->flags & SURF_DRAWFENCE)
			{
				GL_DisableMultitexture(); // selects TEXTURE0
				eglEnable (GL_ALPHA_TEST); // Flip alpha test back on
				GL_EnableMultitexture(); // selects TEXTURE1
			}

			R_RenderDynamicLightmaps (s);
			GL_Bind (lightmap[s->lightmaptexturenum].texture);

			if (active_mirror) {
				// An active mirror is fullbright.
				GL_DisableMultitexture(); 
				eglBegin(GL_POLYGON);
				v = s->polys->verts[0];
				for (j = 0 ; j < s->polys->numverts ; j++, v+= VERTEXSIZE) {
					eglTexCoord2f (v[3], v[4]);
					eglVertex3fv (v);
				}
				eglEnd ();
				GL_EnableMultitexture(); // Restore state
			}
			else {
				eglBegin(GL_POLYGON);
				v = s->polys->verts[0];
				for (j = 0 ; j < s->polys->numverts ; j++, v+= VERTEXSIZE)
				{
					renderer.GL_MTexCoord2fFunc (renderer.TEXTURE0, v[3], v[4]);
					renderer.GL_MTexCoord2fFunc (renderer.TEXTURE1, v[5], v[6]);
					eglVertex3fv (v);
				}
				eglEnd ();
			}
			if (s->flags & SURF_DRAWFENCE)
			{
				GL_DisableMultitexture(); // selects TEXTURE0
				eglDisable (GL_ALPHA_TEST); // Flip alpha test back off
				GL_EnableMultitexture(); // selects TEXTURE1
			}

			rs_brushpasses++;
		} // End if			
	} // End for
	GL_DisableMultitexture(); // selects TEXTURE0
}


void R_DrawTextureChains_Multitexture (void)
{
	int			i;
	texture_t	*t;

	for (i = 0; i < cl.worldmodel->numtextures; i++) {
		t = cl.worldmodel->textures[i];

		if (!t || !t->texturechain || t->texturechain->flags & (SURF_DRAWTILED | SURF_NOTEXTURE)) // Make a draw mirror pass using flags.
			continue;

		R_DrawTextureChains_Multitexture_Tex (t);
	}
}


/*
================
R_DrawTextureChains_Multitexture -- johnfitz
================
*/
// Mirrors.
// sv_novis?  To test for weirdities.
// 1) You can have unlimited mirrors on the exact same plane.  Same plane = all facing the same direction with all the same angles.
// 2) You can have mirrors at different angles, but they must not be able to see each other.  Nor can there be any point that can see both.
//            NOR can there be anywhere in the map, a place that can see any part of both mirrors.  
//					If there is a point P with that can draw lines to any part of 2 different mirror planes, you have a problem.
//					Remember, func_wall is not a real wall.  It does not block visibility.
// 3) Moving mirror entity is probably not a good idea because VIS is for unmoving points.  Expect problems except in a simple box room.                                                           
// 4) Ideally, there would be a function in the mapping tools during VIS generation to single out mirror VIS locations and expand them.
//                    But currently such does not exist.  So might want to make sure mirror doesn't show different VIS leaf
//                                          turning a wall into a func_wall (which doesn't block VIS) might alleviate problems.
void R_DrawTextureChains_Multitexture_Mirrors (void)
{
	int			i;
	texture_t	*t;

	for (i = 0; i < cl.worldmodel->numtextures; i++) {
		const char *txname = cl.worldmodel->textures[i]->name;
		t = cl.worldmodel->textures[i];

		if (!t || !t->texturechain || t->texturechain->flags & (SURF_DRAWTILED | SURF_NOTEXTURE)) // Make a draw mirror pass using flags.
			continue;

// Let Multitexture catch this
		if ( !Flag_Check (t->texturechain->flags, SURF_DRAWMIRROR) )
			continue;
// Culled by who and when?  I don't want to draw mirrors that can't be seen.
		
		R_DrawTextureChains_Multitexture_Tex (t);
	}


}

/*
================
R_DrawTextureChains_NoTexture -- johnfitz

draws surfs whose textures were missing from the BSP
================
*/
void R_DrawTextureChains_NoTexture (void)
{
	int			i;
	msurface_t	*s;
	texture_t	*t;
	cbool	bound;

	for (i=0 ; i<cl.worldmodel->numtextures ; i++)
	{
		t = cl.worldmodel->textures[i];

		if (!t || !t->texturechain || !(t->texturechain->flags & SURF_NOTEXTURE))
			continue;

		bound = false;

		for (s = t->texturechain; s; s = s->texturechain)
			if (!s->culled || frame.in_mirror_draw)
			{
				cbool active_mirror = ACTIVE_MIRROR(s);
				if (active_mirror && !DRAW_ACTIVE_MIRROR) // Mirrors only draw in overlay
					continue; 

				if (!bound) //only bind once we are sure we need this texture
				{
					GL_Bind (t->gltexture);
					bound = true;
				}
				DrawGLPoly (s->polys, 0);
				rs_brushpasses++;
			}
	}
}

/*
================
R_DrawTextureChains_TextureOnly -- johnfitz
================
*/
void R_DrawTextureChains_TextureOnly (void)
{
	int			i;
	msurface_t	*s;
	texture_t	*t;
	cbool	bound;

	for (i=0 ; i<cl.worldmodel->numtextures ; i++)
	{
		t = cl.worldmodel->textures[i];

		if (!t || !t->texturechain || t->texturechain->flags & (SURF_DRAWTURB | SURF_DRAWSKY))
			continue;

		bound = false;

		for (s = t->texturechain; s; s = s->texturechain)
			if (!s->culled || frame.in_mirror_draw)
			{
				cbool active_mirror = ACTIVE_MIRROR(s);
				if (active_mirror && !DRAW_ACTIVE_MIRROR)
					continue; 

				if (!bound) //only bind once we are sure we need this texture
				{
					GL_Bind ((R_TextureAnimation(t,0))->gltexture);
					bound = true;
				}

				if (!level.water_vis_known)
				{
					if (s->flags & SURF_UNDERWATER)
						frame.has_underwater = true;
					else
						frame.has_abovewater = true;
				}

				if (s->flags & SURF_DRAWFENCE)
					eglEnable (GL_ALPHA_TEST); // Flip alpha test back on
				R_RenderDynamicLightmaps (s); //adds to lightmap chain
				DrawGLPoly (s->polys, s->flags);

				if (s->flags & SURF_DRAWFENCE)
					eglDisable (GL_ALPHA_TEST); // Flip alpha test back off
				rs_brushpasses++;
			}
	}
}

/*
================
R_DrawTextureChains_Water -- johnfitz
================
*/
void R_DrawTextureChains_Water (cbool alphapass)
{
	int			i;
	msurface_t	*s;
	texture_t	*t;
	glpoly_t	*p;
	cbool	bound;
	float		texture_alpha;

	if (r_drawflat_cheatsafe || r_lightmap_cheatsafe || !r_drawworld_cheatsafe)
		return;
/*
	if (frame_wateralpha < 1.0)
	{
		// Baker: Yikes ... we will have to turn this off and on as we draw teleporters.  Or create a new chain (NO).
		eglDepthMask(GL_FALSE);
		eglEnable (GL_BLEND);
		eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		eglColor4f (1,1,1, frame_wateralpha);
	}
*/
	if (frame.oldwater)
	{
		for (i = 0 ; i<cl.worldmodel->numtextures ; i++)
		{
			t = cl.worldmodel->textures[i];
			if (!t || !t->texturechain || !(t->texturechain->flags & SURF_DRAWTURB))
				continue;
			bound = false;
			for (s = t->texturechain; s; s = s->texturechain)
				if (!s->culled || frame.in_mirror_draw) // By definition, water cannot be mirror.
				{
					if (!bound) //only bind once we are sure we need this texture
					{
						// Determine texture alpha
						if		(s->flags & SURF_DRAWTELE)		texture_alpha = 1;
						else if (s->flags & SURF_DRAWLAVA)		texture_alpha = frame.lavaalpha;
						else if (s->flags & SURF_DRAWSLIME)		texture_alpha = frame.slimealpha;
						else									texture_alpha = frame.wateralpha;

						if (!((texture_alpha < 1) ==  alphapass))
							continue;

						GL_Bind (t->gltexture);
						bound = true;

						// Set capabilities here
						if (texture_alpha < 1)
						{
							eglDepthMask(GL_FALSE);
							eglEnable (GL_BLEND);
							eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
							eglColor4f (1,1,1, texture_alpha);
						}
						else
						{
							eglDepthMask(GL_TRUE);
							eglDisable (GL_BLEND);
							eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
							eglColor3f (1,1,1);
						}
					}

					for (p = s->polys->next; p; p = p->next)
					{
						DrawWaterPoly (p, (s->flags & SURF_DRAWTELE));
						rs_brushpasses++;
					}
				}
		}
	}
	else
	{
		for (i=0 ; i<cl.worldmodel->numtextures ; i++)
		{
			t = cl.worldmodel->textures[i];
			if (!t || !t->texturechain || !(t->texturechain->flags & SURF_DRAWTURB))
				continue;
			bound = false;
			for (s = t->texturechain; s; s = s->texturechain)
				if (!s->culled || frame.in_mirror_draw) // By definition, water cannot be mirror
				{
					if (!bound) //only bind once we are sure we need this texture
					{
						// Determine texture alpha
						if		(s->flags & SURF_DRAWTELE)		texture_alpha = 1;
						else if (s->flags & SURF_DRAWLAVA)		texture_alpha = frame.lavaalpha;
						else if (s->flags & SURF_DRAWSLIME)		texture_alpha = frame.slimealpha;
						else									texture_alpha = frame.wateralpha;

						if (!((texture_alpha < 1) ==  alphapass))
							continue;

						GL_Bind (t->warpimage);
						bound = true;

						// Set capabilities here
						if (texture_alpha < 1)
						{
							eglDepthMask(GL_FALSE);
							eglEnable (GL_BLEND);
							eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
							eglColor4f (1,1,1, texture_alpha);
						}
						else
						{
							eglDepthMask(GL_TRUE);
							eglDisable (GL_BLEND);
							eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
							eglColor3f (1,1,1);
						}
					}
					DrawGLPoly (s->polys, s->flags);
					rs_brushpasses++;
				}
		}
	}

	eglDepthMask(GL_TRUE);
	eglDisable (GL_BLEND);
	eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	eglColor3f (1,1,1);

}

/*
================
R_DrawTextureChains_White -- johnfitz -- draw sky and water as white polys when r_lightmap is 1
================
*/
void R_DrawTextureChains_White (void)
{
	int			i;
	msurface_t	*s;
	texture_t	*t;

	eglDisable (GL_TEXTURE_2D);
	for (i=0 ; i<cl.worldmodel->numtextures ; i++)
	{
		t = cl.worldmodel->textures[i];

				if (!t || !t->texturechain || !(t->texturechain->flags & SURF_DRAWTILED))
			continue;

		for (s = t->texturechain; s; s = s->texturechain)
			if (!s->culled || frame.in_mirror_draw) 
			{
//				cbool active_mirror = ACTIVE_MIRROR(s);
//				if (active_mirror && !DRAW_ACTIVE_MIRROR)
//					continue; 

				DrawGLPoly (s->polys, 0); // Not here.
				rs_brushpasses++;
			}
	}
	eglEnable (GL_TEXTURE_2D);
}

/*
================
R_DrawLightmapChains -- johnfitz -- R_BlendLightmaps stripped down to almost nothing
================
*/
void R_DrawLightmapChains (void)
{
	int			i, j;
	glpoly_t	*p;
	float		*v;

	for (i=0 ; i<MAX_FITZQUAKE_LIGHTMAPS ; i++)
	{
		if (!lightmap[i].polys)
			continue;

		GL_Bind (lightmap[i].texture);
		for (p = lightmap[i].polys; p; p=p->chain)
		{
			if (!p)
				break; // WHY??????
			eglBegin (GL_POLYGON);
			v = p->verts[0];
			for (j=0 ; j<p->numverts ; j++, v+= VERTEXSIZE)
			{
				eglTexCoord2f (v[5], v[6]);
				eglVertex3fv (v);
			}
			if (!lightmap[i].polys)
				i=i;
			eglEnd ();
			rs_brushpasses++;
		}
		i = i ;
	}
	i = i ;
}

/*
=============
R_DrawWorld -- johnfitz -- rewritten
=============
*/
void R_DrawWorld (void)
{
	R_UploadLightmaps_Modified ();
	if (!r_drawworld_cheatsafe)
		return;

	if (r_drawflat_cheatsafe)
	{
		eglDisable (GL_TEXTURE_2D);
		R_DrawTextureChains_Drawflat ();
		eglEnable (GL_TEXTURE_2D);
		return;
	}

	if (r_fullbright_cheatsafe)
	{
		R_DrawTextureChains_TextureOnly ();
		goto fullbrights;
	}

	if (r_lightmap_cheatsafe)
	{
		R_BuildLightmapChains ();
		if (!gl_overbright.value)
		{
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			eglColor3f(0.5, 0.5, 0.5);
		}
		R_DrawLightmapChains ();
		if (!gl_overbright.value)
		{
			eglColor3f(1,1,1);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		}
		R_DrawTextureChains_White ();
		return;
	}

	R_DrawTextureChains_NoTexture ();

	if (gl_overbright.value)
	{
		if (renderer.gl_texture_env_combine && renderer.gl_mtexable)
		{
			GL_EnableMultitexture ();
			eglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			eglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			eglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
			eglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);
			eglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 2.0f);
			GL_DisableMultitexture ();
			R_DrawTextureChains_Multitexture ();
			GL_EnableMultitexture ();
			eglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0f);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			GL_DisableMultitexture ();
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		}
		else
		{
			//to make fog work with multipass lightmapping, need to do one pass
			//with no fog, one modulate pass with black fog, and one additive
			//pass with black geometry and normal fog
			Fog_DisableGFog ();
			R_DrawTextureChains_TextureOnly ();
			Fog_EnableGFog ();
			eglDepthMask (GL_FALSE);
			eglEnable (GL_BLEND);
			eglBlendFunc (GL_DST_COLOR, GL_SRC_COLOR); //2x modulate
			Fog_StartAdditive ();
			R_DrawLightmapChains ();
			Fog_StopAdditive ();
			if (Fog_GetDensity() > 0)
			{
				eglBlendFunc(GL_ONE, GL_ONE); //add
				eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				eglColor3f(0,0,0);
				R_DrawTextureChains_TextureOnly ();
				eglColor3f(1,1,1);
				eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			}
			eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			eglDisable (GL_BLEND);
			eglDepthMask (GL_TRUE);
		}
	}
	else
	{
		if (renderer.gl_mtexable)
		{
			GL_EnableMultitexture ();
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			GL_DisableMultitexture ();
			R_DrawTextureChains_Multitexture ();
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		}
		else
		{
			//to make fog work with multipass lightmapping, need to do one pass
			//with no fog, one modulate pass with black fog, and one additive
			//pass with black geometry and normal fog
			Fog_DisableGFog ();
			R_DrawTextureChains_TextureOnly ();
			Fog_EnableGFog ();
			eglDepthMask (GL_FALSE);
			eglEnable (GL_BLEND);
			eglBlendFunc(GL_ZERO, GL_SRC_COLOR); //modulate
			Fog_StartAdditive ();
			R_DrawLightmapChains ();
			Fog_StopAdditive ();
			if (Fog_GetDensity() > 0)
			{
				eglBlendFunc(GL_ONE, GL_ONE); //add
				eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				eglColor3f(0,0,0);
				R_DrawTextureChains_TextureOnly ();
				eglColor3f(1,1,1);
				eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			}
			eglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			eglDisable (GL_BLEND);
			eglDepthMask (GL_TRUE);
		}
	}

fullbrights:
	if (gl_fullbrights.value) {
		eglDepthMask (GL_FALSE);
		eglEnable (GL_BLEND);
		eglBlendFunc (GL_ONE, GL_ONE);
		Fog_StartAdditive ();
		R_DrawTextureChains_Glow ();
		Fog_StopAdditive ();
		eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		eglDisable (GL_BLEND);
		eglDepthMask (GL_TRUE);
	}
}

#endif // GLQUAKE specific