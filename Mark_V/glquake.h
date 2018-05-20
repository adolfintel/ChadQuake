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
// glquake.h

#ifndef __GLQUAKE_H__
#define __GLQUAKE_H__

//cmd void GL_Info_f (void);

///////////////////////////////////////////////////////////////////////////////
//  OPENGL: General FitzQuake drawing
///////////////////////////////////////////////////////////////////////////////

extern cbool envmap; // don't draw viewmodel during this

#define MAX_CROSSHAIRS_25 (MAX_EFFECTIVE_WEAPON_COUNT_25)

extern gltexture_t *char_texture; // conchars
extern gltexture_t *crosshair_weapon_textures[MAX_CROSSHAIRS_25]; // crosshair
extern int crosshair_weapon_textures_found;

extern gltexture_t *crosshair_default_texture;
extern float load_subdivide_size; //johnfitz -- remember what subdivide_size value was when this map was loaded



// Multitexture
extern cbool mtexenabled;
#pragma message ("Baker: This mtexenabled is used by GL_DrawAliasFrame and I don't think it should be")

//johnfitz -- polygon offset
#define OFFSET_BMODEL 1
#define OFFSET_NONE 0
#define OFFSET_DECAL -1
#define OFFSET_FOG -2
#define OFFSET_SHOWTRIS -3

// gl_rmain.c - used by showtris and (DECAL ->) R_DrawSpriteModel for oriented sprites
void GL_PolygonOffset (int);


///////////////////////////////////////////////////////////////////////////////
//  Rendering stats
///////////////////////////////////////////////////////////////////////////////

//johnfitz -- rendering statistics
extern int rs_brushpolys, rs_aliaspolys, rs_skypolys, rs_particles, rs_fogpolys;
extern int rs_dynamiclightmaps, rs_brushpasses, rs_aliaspasses, rs_skypasses;
extern float rs_megatexels;

///////////////////////////////////////////////////////////////////////////////
//  Lightmaps
///////////////////////////////////////////////////////////////////////////////

#define BLOCKLITE_BYTES_3	3
#define LIGHTMAPS_BYTES_4	4

#define LIGHTMAPS_BLOCK_SIZE	(LIGHTMAPS_BYTES_4 * LIGHTMAPS_BLOCK_WIDTH * LIGHTMAPS_BLOCK_HEIGHT)
#define BLOCKLITE_BLOCK_SIZE	(BLOCKLITE_BYTES_3 * LIGHTMAPS_BLOCK_WIDTH * LIGHTMAPS_BLOCK_HEIGHT)

typedef struct glRect_s 
{
	unsigned char l, t, w, h;
} glRect_t;

typedef struct
{
	cbool		modified;
	glRect_t	rectchange;
	int			allocated[LIGHTMAPS_BLOCK_WIDTH];
	byte		lightmaps[LIGHTMAPS_BLOCK_SIZE];
	byte		stainmaps[BLOCKLITE_BLOCK_SIZE];
	glpoly_t	*polys;
	gltexture_t	*texture;
} lightmapinfo_t;

extern lightmapinfo_t lightmap[MAX_FITZQUAKE_LIGHTMAPS];


///////////////////////////////////////////////////////////////////////////////
//  FOG
//  johnfitz -- fog functions called from outside gl_fog.c
///////////////////////////////////////////////////////////////////////////////

void Fog_ParseServerMessage (void);
float *Fog_GetColor (float *startdist, float *enddist);
float Fog_GetDensity (void);
void Fog_EnableGFog (void);
void Fog_DisableGFog (void);
void Fog_StartAdditive (void);
void Fog_StopAdditive (void);
void Fog_SetupFrame (void);
void Fog_NewMap (void);
void Fog_Init (void);
void Fog_Update (float density, float red, float green, float blue, float time); // For Nehahra 

///////////////////////////////////////////////////////////////////////////////
//  Rendering and Frame Setup
///////////////////////////////////////////////////////////////////////////////

extern int gl_warpimagesize; //johnfitz -- for water warp
extern cbool r_drawflat_cheatsafe, r_fullbright_cheatsafe, r_lightmap_cheatsafe, r_drawworld_cheatsafe; //johnfitz

void R_MarkSurfaces (void);
void R_CullSurfaces (void);
cbool R_CullBox (vec3_t emins, vec3_t emaxs);
cbool R_CullModelForEntity (entity_t *e);
void R_RotateForEntity (vec3_t origin, vec3_t angles);

void R_SetupView_UpdateWarpTextures (void); // R_WATERWARP

void GL_WarpScreen (void); // R_WATERWARP

void R_DrawWorld (void);
void R_DrawAliasModel (entity_t *e);
void R_DrawBrushModel (entity_t *e);
void R_DrawSpriteModel (entity_t *e);
void R_DrawTextureChains_Water (cbool alphapass);

void R_UploadLightmaps_Modified (void);
void GL_BuildLightmaps_Upload_All_NewMap (void);

void GL_SubdivideSurface (msurface_t *fa);
void R_RenderDynamicLightmaps (msurface_t *fa);

void R_DrawTextureChains_ShowTris (void);
void R_DrawBrushModel_ShowTris (entity_t *e);
void R_DrawAliasModel_ShowTris (entity_t *e);
void R_DrawParticles_ShowTris (void);

void GL_DrawAliasShadow (entity_t *e);

void RB_ShadowVolumeBegin (void);	// Baker: Clear the stencil buffer if applicable  MH Shadow Volumes
void RB_ShadowVolumeFinish (void);	// Baker: Render  MH Shadow Volumes


void DrawGLPoly (glpoly_t *p, int renderfx);
void DrawGLTriangleFan (glpoly_t *p, int renderfx);
void DrawWaterPoly (glpoly_t *p, cbool isteleporter);
void GL_MakeAliasModelDisplayLists (qmodel_t *m, aliashdr_t *hdr);

void R_VisChanged (cvar_t *var);
void R_SetClearColor_f (cvar_t *var);

cbool R_SkinTextureChanged (entity_t *cur_ent); // Baker: Colored dead bodies and colormapped ents
gltexture_t *R_TranslateNewModelSkinColormap (entity_t *cur_ent);


cbool GL_Mirrors_Is_TextureName_Mirror (const char *txname);
// Baker: For drawing a simple texture, like a crosshair texture.
// Crosshair texture must be named /gfx/crosshair.tga
void Draw_GLTexture (gltexture_t *tx, float x0, float y0, float x1, float y1);

enum { FILLED_POLYGON, OUTLINED_POLYGON };
Point3D R_EmitSurfaceHighlight (entity_t *enty, msurface_t *surf, rgba4_t color, int style);

///////////////////////////////////////////////////////////////////////////////
//  Bloom -- Baker
///////////////////////////////////////////////////////////////////////////////

void GL_Bloom_Init_Once (void);
void GL_Bloom_RecalcImageSize (void); // Screensize changes, similar to TexMgr_R_SetupView_RecalcWarpImageSize
void GL_BloomBlend (void);

///////////////////////////////////////////////////////////////////////////////
//  Mirrors -- Baker
///////////////////////////////////////////////////////////////////////////////

void R_Mirror (void);
void R_DrawTextureChains_Multitexture_Mirrors (void);

#define Q_GLDEPTHRANGE_MIN_0_0 0
#define Q_GLDEPTHRANGE_MAX_0_5 0.5
#define Q_GLDEPTHRANGE_MIN_MIRROR_0_5 0.5
#define Q_GLDEPTHRANGE_MAX_MIRROR_1_0 1

#define Q_GLDEPTHRANGE_GUN_0_125 0.125 // See if "ok", made this binary friendly. (0.125 = 1 / 8)



#endif // ! __GLQUAKE_H__



