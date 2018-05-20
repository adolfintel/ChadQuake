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
// render.h

#ifndef __RENDER_H__
#define __RENDER_H__

// refresh.h -- public interface to refresh functions

#define	TOP_RANGE		16			// soldier uniform colors
#define	BOTTOM_RANGE	96

//=============================================================================

typedef struct efrag_s
{
	struct mleaf_s		*leaf;
	struct efrag_s		*leafnext;
	struct entity_s		*entity;
	struct efrag_s		*entnext;
} efrag_t;

#ifdef WINQUAKE_STIPPLE_WATERALPHA
byte r_foundtranswater, r_wateralphapass; // Manoel Kasimier - translucent water
#endif // WINQUAKE_STIPPLE_WATERALPHA

//johnfitz -- for lerping
#define LERP_MOVESTEP	(1<<0) //this is a MOVETYPE_STEP entity, enable movement lerp
#define LERP_RESETANIM	(1<<1) //disable anim lerping until next anim frame
#define LERP_RESETANIM2	(1<<2) //set this and previous flag to disable anim lerping for two anim frames
#define LERP_RESETMOVE	(1<<3) //disable movement lerping until next origin/angles change
#define LERP_FINISH		(1<<4) //use lerpfinish time from server update instead of assuming interval of 0.1
//johnfitz

typedef struct entity_s
{
	cbool					forcelink;		// model changed
	glmatrix				gl_matrix;
//	vec3_t					modelorg;
	int						update_type;

	entity_state_t			baseline;		// to fill in defaults in updates

	double					msgtime;		// time of last update
	vec3_t					msg_origins[2];	// last two updates (0 is newest)
	vec3_t					origin;
	vec3_t					msg_angles[2];	// last two updates (0 is newest)
	vec3_t					angles;
	struct qmodel_s			*model;			// NULL = no model
#ifdef GLQUAKE // Baker: Eliminate in future
	struct gltexture_s		*coloredskin;
	int						colormap;
	cbool					is_static_entity;
//	int						static_mirror_numsurfs;
	//struct mplane_s		*mirror_plane;
#else
	byte					*colormap;
#endif // GLQUAKE vs. WinQuake


	int						modelindex; // Too useful
#ifdef GLQUAKE_SUPPORTS_QMB
	cbool					is_fake_frame0;
#endif // GLQUAKE_SUPPORTS_QMB

	struct efrag_s			*efrag;			// linked list of efrags
	int						frame;
	float					syncbase;		// for client-side animations

	int						effects;		// light, particles, etc
	int						skinnum;		// for Alias models
	int						visframe;		// last frame this entity was
											//  found in an active leaf

#ifdef GLQUAKE // Baker: Eliminate in future
	int						dlightframe;	// dynamic lighting
	int						dlightbits;
#endif // GLQUAKE vs. WinQuake

// FIXME: could turn these into a union
	int						trivial_accept;
	struct mnode_s			*topnode;		// for bmodels, first world node that splits bmodel, or NULL if not split
#ifdef SUPPORTS_NEHAHRA
	// nehahra support
	float					smokepuff_time;
	vec3_t					trail_origin;
	cbool					traildrawn;

#endif // SUPPORTS_NEHAHRA


	byte					alpha;			//johnfitz -- alpha
	byte					lerpflags;		//johnfitz -- lerping
	float					lerpstart;		//johnfitz -- animation lerping
	float					lerptime;		//johnfitz -- animation lerping
	float					lerpfinish;		//johnfitz -- lerping -- server sent us a more accurate interval, use it instead of 0.1
	short					previouspose;	//johnfitz -- animation lerping
	short					currentpose;	//johnfitz -- animation lerping
//	short					futurepose;		//johnfitz -- animation lerping
	float					movelerpstart;	//johnfitz -- transform lerping
	vec3_t					previousorigin;	//johnfitz -- transform lerping
	vec3_t					currentorigin;	//johnfitz -- transform lerping
	vec3_t					previousangles;	//johnfitz -- transform lerping
	vec3_t					currentangles;	//johnfitz -- transform lerping
} entity_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	vrect_t		vrect;					// subwindow in video for refresh

#ifndef GLQUAKE
	vrect_t		aliasvrect;				// scaled Alias version
	int			vrectright, vrectbottom;// right & bottom screen coords
	int			aliasvrectright;
	int			aliasvrectbottom;		// scaled Alias versions
	float		vrectrightedge;			// rightmost right edge we care about, for use in edge list
	float		fvrectx, fvrecty;		// for floating-point compares
	float		fvrectx_adj;			// left and top edges, for clamping
	float		fvrecty_adj;
	int			vrect_x_adj_shift20;	// (vrect.x + 0.5 - epsilon) << 20
	int			vrectright_adj_shift20;	// (vrectright + 0.5 - epsilon) << 20
	float		fvrectright_adj;
	float		fvrectbottom_adj; 		// right and bottom edges, for clamping
	float		fvrectright;			// rightmost edge, for Alias clamping
	float		fvrectbottom;			// bottommost edge, for Alias clamping
	float		horizontalFieldOfView;	// at Z = 1.0, this many X is visible
										// 2.0 = 90 degrees
	float		xOrigin;				// should probably always be 0.5
	float		yOrigin;				// between be around 0.3 to 0.5
#endif

	vec3_t		vieworg;
	vec3_t		viewangles;

	float		fov_x, fov_y;

#ifndef GLQUAKE
	int			ambientlight;
#endif
} refdef_t;


//
// refresh
//




void R_Init (void);
void R_InitEfrags (void);
void R_RenderView (void);		// must set r_refdef first

#ifdef GLQUAKE_FLASH_BLENDS
void R_Init_FlashBlend_Bubble (void);
#endif // GLQUAKE_FLASH_BLENDS

void R_CheckEfrags (void); //johnfitz
void R_AddEfrags (entity_t *ent);
void R_RemoveEfrags (entity_t *ent);

void R_NewMap (void);
void R_Level_Key_Alpha_SkyFog_Changed (cvar_t *var);

// particles

typedef enum trail_type_s
{
	ROCKET_TRAIL_0,
	GRENADE_TRAIL_1,
	BLOOD_TRAIL_2,
	TRACER1_SCRAG_TRAIL_3,
	SLIGHT_ZOM_BLOOD_TRAIL_4,
	TRACER2_HELLKNIGHT_TRAIL_5,
	VOOR_TRAIL_6,
#ifdef GLQUAKE_SUPPORTS_QMB
	LAVA_TRAIL_7,
	BUBBLE_TRAIL_8,
#endif // GLQUAKE_SUPPORTS_QMB
	NEHAHRA_SMOKE_9
} trail_type_e;


/*  No ... these are static ....
void Classic_InitParticles (void); // void R_InitParticles
void Classic_ClearParticles (void); // R_ClearParticles
void Classic_ParseParticleEffect (void); // R_ParseParticleEffect 
void Classic_RunParticleEffect (vec3_t org, const vec3_t dir, int color, int count); // R_RunParticleEffect 
void Classic_AnyTrail (vec3_t start, vec3_t end, vec3_t *trail_origin, trail_type_e type); // R_AnyTrail 
void Classic_EntityParticles (entity_t *ent); // R_EntityParticles 
void Classic_BlobExplosion (vec3_t org); // R_BlobExplosion 
void Classic_ParticleExplosion (vec3_t org); // R_ParticleExplosion 
void Classic_ColorMappedExplosion (vec3_t org, int colorStart, int colorLength); // R_ParticleExplosion2 
void Classic_LavaSplash (vec3_t org); // R_LavaSplash 
void Classic_TeleportSplash (vec3_t org); // R_TeleportSplash 
*/

void R_InitParticles (void); 
void R_ClearParticles (void);
void R_ParseParticleEffect (void);
void R_RunParticleEffect (vec3_t org, const vec3_t dir, int color, int count, te_ef_effect_e te_ef_effect);
void R_AnyTrail (entity_t *ent, vec3_t start, vec3_t end, vec3_t *trail_origin, trail_type_e type);
void R_EntityParticles (entity_t *ent);
void R_BlobExplosion (vec3_t org);
void R_ParticleExplosion (vec3_t org);
void R_ColorMappedExplosion (vec3_t org, int colorStart, int colorLength); // ParticleExplosion2
void R_LavaSplash (vec3_t org);
void R_TeleportSplash (vec3_t org);




// by joe: the last 2 are own creations, they handle color mapped explosions
typedef enum 
{
	lt_invalid = -1, // In case GCC gets stupid and tries to make unsigned
	lt_default = 0,
	lt_muzzleflash,
	lt_explosion,
	lt_rocket,
	lt_red,
	lt_blue,
	lt_redblue,
	NUM_DLIGHTTYPES_7,
	lt_explosion2,
	lt_explosion3
} dlighttype_e;


extern cbool qmb_is_available;

#ifdef GLQUAKE_SUPPORTS_QMB
extern	float			ExploColor[3];	// joe: for color mapped explosions
const char *QMB_InitParticles_Error (void);   // Returns pointer to reason
void QMB_ClearParticles (void);
void QMB_DrawParticles (void);
void QMB_RunParticles (void);


void QMB_ParseParticleEffect (void);
void QMB_RunParticleEffect (const vec3_t org, const vec3_t dir, int color, int count, te_ef_effect_e te_ef_effect);
void QMB_AnyTrail (const vec3_t start, const vec3_t end, vec3_t *trail_origin, trail_type_e type);
void QMB_EntityParticles (entity_t *ent);
void QMB_BlobExplosion (const vec3_t org);
void QMB_ParticleExplosion (const vec3_t org);
void QMB_ColorMappedExplosion (const vec3_t org, int colorStart, int colorLength); // ParticleExplosion2
void QMB_LavaSplash (const vec3_t org);
void QMB_TeleportSplash (const vec3_t org);
void QMB_LightningBeam (const vec3_t start, const vec3_t end);
void QMB_LaserFire (const vec3_t start, const vec3_t end);

cbool QMB_FlameModelSetState (entity_t *ent);
void QMB_StaticBubble (entity_t *ent, const vec3_t origin);
void QMB_ShamblerCharge (const vec3_t org);
void QMB_MissileFire (const vec3_t org, const vec3_t start, const vec3_t end);

cbool QMB_MaybeInsertEffect (entity_t *ent, vec3_t oldorg, int entnum);
cbool QMB_Effects_Evaluate (int i, entity_t *ent, vec3_t oldorg);
Point3D QMB_GetDlightColor (dlighttype_e colornum, dlighttype_e def, cbool random);

//extern byte qmb_flame0_mdl[];
//xtern const size_t qmb_flame0_mdl_size;

#endif //  GLQUAKE_SUPPORTS_QMB


void R_PushDlights (entity_t *ent);

cbool Clasic_Effects_Evaluate (int i, entity_t *ent, vec3_t oldorg);
void DLight_Add (int keyx, vec3_t originx, float radiusx, float minlightx, double dietimex, float redx, float greenx, float bluex);

void Stains_WipeStains_NewMap (void);
void Stain_FrameSetup_LessenStains(cbool erase_stains);
void Stain_AddStain(const vec3_t origin, float tint, float in_radius);
void Stain_Change_f (cvar_t *var);



#ifdef GLQUAKE_CAPTIONS_AND_HIGHLIGHTS
void R_EmitCaption (vec3_t location, const char *caption, cbool backgrounded);
void R_EmitBox (const vec3_t myColor, const vec3_t centerpoint, const vec3_t xyz_mins, const vec3_t xyz_maxs, const int bTopMost, const int bLines, const float Expandsize);
#endif // GLQUAKE_CAPTIONS_AND_HIGHLIGHTS


#ifdef WINQUAKE_RENDERER_SUPPORT
int	D_SurfaceCacheForRes (int width, int height);
void D_FlushCaches (void);
void D_InitCaches (void *buffer, int size);
void R_SetVrect (vrect_t *pvrect, vrect_t *pvrectin, int lineadj);
void R_ViewChanged (vrect_t *pvrect, int lineadj, float aspect); // called whenever r_refdef or vid change

// surface cache related
extern cbool	r_cache_thrash;	// set if thrashing the surface cache
#endif // WINQUAKE_RENDERER_SUPPORT


typedef enum
{
	pt_static, pt_grav, pt_slowgrav, pt_fire, pt_explode, pt_explode2, pt_blob, pt_blob2
} ptype_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct particle_s
{
// driver-usable fields
	vec3_t		org;
	float		color;
// drivers never touch the following fields
	struct particle_s	*next;
	vec3_t		vel;
	float		ramp;
	float		die;
	ptype_t		type;
} particle_t;




#define BACKFACE_EPSILON	0.01

extern float r_fovx, r_fovy;

// Original home was model.h  DO NOT MODIFY.  /* qasm */ ASM requires unmodified.
typedef struct mplane_s
{
	vec3_t	normal;
	float	dist;
	byte	type;			// for texture axis selection and fast side tests
	byte	signbits;		// signx + signy<<1 + signz<<1
	byte	pad[2];
} mplane_t;
// Original home was model.h

typedef struct {
//	vec3_t		mirrorscalef;		// No, that's taken into account in the matrix.
	vec3_t		vieworg;
	vec3_t		viewangles;
	mplane_t	r_frustum[4];
	vec3_t		vpn, vright, vup;
	float		r_fovx, r_fovy;		// No idea on how to calculate this for mirrors and I'm not certain it is possible either.
									// Because a mirror isn't a single point, so it isn't like a camera.  Therefore for culling
									// purposes it is useless because the view of a mirror is not the same shape as a camera view.
									// Plus we can have multiple mirrors on the same plane.
									// Hence fov culling is incompatible with mirrors, according to geometry.  Some other calculation
									// might work, but it would be complex and need to calculate the bounds of all mirror surfaces
									// and then backtrack on a plane and determine a focal point to calculate the frustum.
									// Maybe a level 100 3D math genious could do this.
	
	float		projection[16];
	float		modelview[16];
	vec3_t		r_origin;
} view_store_t;
// Grab bag of variables we have to store for mirrors.  Grrr.


typedef struct
{
	cbool oldwater;				// Doesn't apply to WinQuake, just GL and the Direct3D version forces to 1 every frame.
	cbool nearwaterportal; 		// GL + WinQuake
	cbool has_underwater; 		// GL + WinQuake
	cbool has_abovewater; 		// GL + WinQuake
	cbool has_sky; 				// Baker: Could work for WinQuake but not relevant.
	cbool has_mirror; 			// Baker: Would be real hard to make it work for WinQuake.
	cbool do_glwarp;			// GL R_WATERWARP

	cbool qmb;					// QMB

// Baker: Direct3D wrapper doesn't have stencil, so will have to have the mirror some other way.
// Baker: Will have to draw the sky the traditional way
	float skyfog;

	float liquid_alpha; 		// WinQuake:  A per surface indicator of how much alpha to apply for current liquid texture.

	float wateralpha; 			// GL + WinQuake, alpha for the frame
	float slimealpha; 			// GL + WinQuake, alpha for the frame
	float lavaalpha; 			// GL + WinQuake, alpha for the frame
	float mirroralpha; 			// Baker: Don't see this working in WinQuake easily, unless I add a stencil buffer to WinQuake
	
	cbool			in_mirror_draw;
	cbool			in_mirror_overlay;
	mplane_t		*mirror_plane;
	view_store_t	original_view;
	view_store_t	mirror_view;
	cbool			in_shadow_volume_draw;
	
} frame_render_t; // erased every frame, right?  Yes.  gl_rmain.c and r_main.c



#ifdef WINQUAKE_RENDERER_SUPPORT
extern float winquake_surface_liquid_alpha;
#endif // WINQUAKE_RENDERER_SUPPORT

// r_surf.c and gl_brush.c
#define	LIGHTMAPS_BLOCK_WIDTH		128
#define	LIGHTMAPS_BLOCK_HEIGHT		128

// r_common.c
extern vec3_t	modelorg;
extern entity_t	*currententity;
extern refdef_t	r_refdef;
extern vec3_t	r_origin, vpn, vright, vup;

extern frame_render_t frame;

void R_Init_Local (void); // Software or GL intialization where not common (R_Init)
void R_NewMap_Local (void); // Same except for new map
void R_SetLiquidAlpha (void);
// texture_t *R_TextureAnimation (texture_t *base, int frame); Baker had to put in model.h due to texture_t :(

// r_efrag.c
void R_StoreEfrags (efrag_t **ppefrag);

// r_part.c
//cmd void R_ReadPointFile_f (void);
//void R_InitParticles (void);  moved up
void CL_RunParticles (void);
void R_DrawParticles (void);
//void R_ClearParticles (void); // moved up

// r_light.c
extern int d_lightstylevalue[256]; // 8.8 fraction of base light value

void R_AnimateLight (void);
int R_LightPoint (vec3_t p);

#ifdef GLQUAKE_RENDERER_SUPPORT
void R_PushDlights_World (void);
void R_RenderDlights_Flashblend (void);
#endif // GLQUAKE_RENDERER_SUPPORT

// r_misc.c and gl_rmisc.c
void R_NewGame (void);

// r_sky.c and gl_sky.c
#define SKYBOX_SIDES_COUNT_6 6
void Sky_LoadSkyBox (const char *name);
void Sky_NewMap (void);
void Sky_Init (void);
void Sky_DrawSky (void);
//void Sky_LoadTexture (struct texture_s *mt); // Baker: Had to move to model.h to avoid Mac compiler error
//cmd void Sky_SkyCommand_f (void);

extern char skybox_name[MAX_QPATH_64];

#ifdef GLQUAKE_RENDERER_SUPPORT
// Baker: Stencil drawing method
void Sky_FrameSetup (void);
void Sky_Stencil_Draw (void);
#endif // GLQUAKE_RENDERER_SUPPORT


#endif // __RENDER_H__

