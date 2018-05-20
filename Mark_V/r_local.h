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
// r_local.h

#ifndef __R_LOCAL_H__
#define __R_LOCAL_H__


// r_local.h -- private refresh defs

#ifdef WINQUAKE_RENDERER_SUPPORT

#include "r_shared.h"


#define BMODEL_FULLY_CLIPPED	0x10 // value returned by R_BmodelCheckBBox ()
									 //  if bbox is trivially rejected

//===========================================================================
// viewmodel lighting

typedef struct {
	int			ambientlight;
	int			shadelight;
	float		*plightvec;
} alight_t;

//===========================================================================
// clipped bmodel edges

typedef struct bedge_s
{
	mvertex_t		*v[2];
	struct bedge_s	*pnext;
} bedge_t;

typedef struct {
	float	fv[3];		// viewspace x, y
} auxvert_t;


#define XCENTERING	(1.0 / 2.0)
#define YCENTERING	(1.0 / 2.0)



//===========================================================================

#define	DIST_NOT_SET	98765

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct clipplane_s
{
	vec3_t		normal;
	float		dist;
	struct		clipplane_s	*next;
	byte		leftedge;
	byte		rightedge;
	byte		reserved[2];
} clipplane_t;

extern	clipplane_t	view_clipplanes[4];

//=============================================================================

void R_RenderWorld (void);

//=============================================================================

extern byte		*r_stack_start;

extern	mplane_t	screenedge[4];



extern	vec3_t	r_entorigin;

//extern	float	screenAspect;
extern	float	verticalFieldOfView;
extern	float	xOrigin, yOrigin;



//=============================================================================

//void R_ClearPolyList (void);
//void R_DrawPolyList (void);

//
// current entity info
//
extern	cbool		insubmodel;
extern	vec3_t			r_worldmodelorg;



void R_DrawSpriteModel (entity_t *e);
void R_RenderFace (msurface_t *fa, int clipflags);
//void R_RenderPoly (msurface_t *fa, int clipflags);
void R_RenderBmodelFace (bedge_t *pedges, msurface_t *psurf);
void R_TransformPlane (mplane_t *p, float *normal, float *dist);
void R_TransformFrustum (void);
void R_SetSkyFrame (void);
void R_DrawSurfaceBlock16 (void);
void R_DrawSurfaceBlock8 (void);

void R_WinQuake_Generate_Alpha50_Map (byte my_alpha50map[]); //qb: 50% / 50% alpha
cbool VID_WinQuake_AllocBuffers_D_InitCaches (int width, int height); // Alloc Buffers.

#if	id386

void R_DrawSurfaceBlock8_mip0 (void);
void R_DrawSurfaceBlock8_mip1 (void);
void R_DrawSurfaceBlock8_mip2 (void);
void R_DrawSurfaceBlock8_mip3 (void);

#endif


void R_Surf8Patch (void); // Baker: asm
//void R_Surf16Patch (void); // Baker: in asm, but not used (16 bit pixels)
void R_DrawSubmodelPolygons (qmodel_t *pmodel, int clipflags);
void R_DrawSolidClippedSubmodelPolygons (qmodel_t *pmodel);

void R_AddPolygonEdges (emitpoint_t *pverts, int numverts, int miplevel);
surf_t *R_GetSurf (void);
void R_AliasDrawModelMH (alight_t *plighting);
void R_AliasDrawModel (alight_t *plighting);
void R_CheckAliasVerts (int numverts);
void R_FinalizeAliasVerts (void);

void R_BeginEdgeFrame (void);
void R_ScanEdges (void);
void D_DrawSurfaces (void);
void R_InsertNewEdges (edge_t *edgestoadd, edge_t *edgelist);
void R_StepActiveU (edge_t *pedge);
void R_RemoveEdges (edge_t *pedge);

extern void R_Surf8Start (void);
extern void R_Surf8End (void);
extern void R_Surf16Start (void);
extern void R_Surf16End (void);
extern void R_EdgeCodeStart (void);
extern void R_EdgeCodeEnd (void);

extern void R_RotateBmodel (void);

extern int	c_faceclip;
extern int	r_polycount;

extern int		*pfrustum_indexes[4];

// !!! if this is changed, it must be changed in asm_draw.h too !!!
#define	NEAR_CLIP	0.01

extern int			ubasestep, errorterm, erroradjustup, erroradjustdown;

extern fixed16_t	sadjust, tadjust;
extern fixed16_t	bbextents, bbextentt;

#define MAXBVERTINDEXES	1000	// new clipped vertices when clipping bmodels
								//  to the world BSP
extern mvertex_t	*r_ptverts, *r_ptvertsmax;

extern vec3_t		sbaseaxis[3], tbaseaxis[3];
extern float		entity_rotation[3][3];

extern int r_currentkey;
extern int r_currentbkey;

typedef struct btofpoly_s
{
	int			clipflags;
	msurface_t	*psurf;
} btofpoly_t;

#define MAX_BTOFPOLYS	5000	// FIXME: tune this

extern int			numbtofpolys;
extern btofpoly_t	*pbtofpolys;

void R_InitTurb (void);
//void R_ZDrawSubmodelPolys (qmodel_t *clmodel);

//=========================================================
// Alias models
//=========================================================

//#define MAXALIASVERTS_3984 moved to model.h
#define ALIAS_Z_CLIP_PLANE	5

extern int				numverts;
extern int				a_skinwidth;
extern mtriangle_t		*ptriangles;
extern int				numtriangles;
extern aliashdr_t		*paliashdr;
extern mdl_t			*pmdl;
extern float			leftclip, topclip, rightclip, bottomclip;
extern int				r_acliptype;
extern finalvert_t		*pfinalverts;
extern auxvert_t		*pauxverts;

cbool R_AliasCheckBBox (void);

//=========================================================
// turbulence stuff

#define	AMP		8*0x10000
#define	AMP2	3
#define	SPEED	20


void R_SurfacePatch (void); // Baker: asm

extern int		r_amodels_drawn;
extern edge_t	*auxedges;
extern int		r_numallocatededges;
extern edge_t	*r_edges, *edge_p, *edge_max;

extern	edge_t	*newedges[MAXHEIGHT];
extern	edge_t	*removeedges[MAXHEIGHT];


// FIXME: make stack vars when debugging done
extern	edge_t	edge_head;
extern	edge_t	edge_tail;
extern	edge_t	edge_aftertail;
extern int r_bmodelactive;

extern float	aliasxscale, aliasyscale, aliasxcenter, aliasycenter;
extern float	r_aliastransition, r_resfudge;

extern int r_outofsurfaces;
extern int r_outofedges;

extern mvertex_t 	*r_pcurrentvertbase;
extern int			r_maxvalidedgeoffset;

void R_AliasClipTriangle (mtriangle_t *ptri);

extern float	r_time1;
extern float	dp_time1, dp_time2, db_time1, db_time2, rw_time1, rw_time2;
extern float	se_time1, se_time2, de_time1, de_time2, dv_time1, dv_time2;
extern int		r_frustum_indexes[4*6];
extern int		r_maxsurfsseen, r_maxedgesseen, r_cnumsurfs;
extern cbool	r_surfsonstack;
//extern cshift_t	cshift_water;
extern cbool	r_dowarpold, r_viewchanged;

extern vec3_t	r_emins, r_emaxs;
extern mnode_t	*r_pefragtopnode;
extern int		r_clipflags;

void R_BuildLightmaps(void); //qbism ftestain



void R_TimeGraph (void);
void R_PrintAliasStats (void);
void R_PrintTimes (void);
void R_PrintDSpeeds (void);


void R_SetupFrame (void);
void R_EmitEdge (mvertex_t *pv0, mvertex_t *pv1);
void R_ClipEdge (mvertex_t *pv0, mvertex_t *pv1, clipplane_t *clip);
void R_SplitEntityOnNode2 (mnode_t *node);


#ifdef WINQUAKE_SOFTWARE_SKYBOX
#define SKYBOX_MAX_SIZE 1024
void R_InitSkyBox (void); // Manoel Kasimier - skyboxes // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
#endif // WINQUAKE_SOFTWARE_SKYBOX


extern	struct texture_s *r_notexture_mip;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CACHE_SIZE	32		// used to align key data structures


// Baker: moved ^^ from quakedef.h because only used for winquake/software render





#endif // WINQUAKE_RENDERER_SUPPORT

#endif	// ! __R_LOCAL_H__


