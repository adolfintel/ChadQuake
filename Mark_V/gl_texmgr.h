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

#ifndef __GL_TEXMGR_H__
#define __GL_TEXMGR_H__

//gl_texmgr.h -- fitzquake's texture manager. manages opengl texture images

#define TEXPREF_NONE			0x0000
#define TEXPREF_MIPMAP			0x0001	// generate mipmaps
// TEXPREF_NEAREST and TEXPREF_LINEAR aren't supposed to be ORed with TEX_MIPMAP
#define TEXPREF_LINEAR			0x0002	// force linear
#define TEXPREF_NEAREST			0x0004	// force nearest
#define TEXPREF_ALPHA			0x0008	// allow alpha
#define TEXPREF_PAD				0x0010	// allow padding
#define TEXPREF_PERSIST			0x0020	// never free
#define TEXPREF_OVERWRITE		0x0040	// overwrite existing same-name texture
#define TEXPREF_NOPICMIP		0x0080	// always load full-sized
#define TEXPREF_FULLBRIGHT		0x0100	// use fullbright mask palette
#define TEXPREF_NOBRIGHT		0x0200	// use nobright mask palette
#define TEXPREF_CONCHARS		0x0400	// use conchars palette
#define TEXPREF_WARPIMAGE		0x0800	// resize this texture when warpimagesize changes
#define TEXPREF_FLOODFILL		0x1000	// skin, part of MH floodfill fix
#define TEXPREF_BLENDED			0x2000	// Don't gamma me bro.  Cheating for lightning gun for now.
#define TEXPREF_BLOOMSCREEN		0x4000	// resize this texture when screensize changes


enum srcformat_e
{
	SRC_INDEXED,
	SRC_LIGHTMAP,
	SRC_RGBA,
	SRC_INDEXED_WITH_PALETTE,
};



typedef uintptr_t src_offset_t;

typedef struct gltexture_s {
//managed by texture manager
	unsigned int		texnum;
	struct gltexture_s	*next;
	qmodel_t			*owner;
// Baker
	int		            bsp_texnum; // for bsp
	cbool				is_mirror; // For debugging
#if 0
	char				paste_texture_name[MAX_QPATH_64]; //relative filepath to data source, or "" if source is in memory
	unsigned int		paste_texture_width; //size of image in source data
	unsigned int		paste_texture_height; //size of image in source data
	unsigned int		paste_texture_crc; //size of image in source data
#endif

//managed by image loading
	char				name[MAX_QPATH_64];
	unsigned int		width; //size of image as it exists in opengl
	unsigned int		height; //size of image as it exists in opengl
	unsigned int		flags;
	char				source_file[MAX_QPATH_64]; //relative filepath to data source, or "" if source is in memory
	src_offset_t		source_offset; //byte offset into file, or memory address
	src_offset_t		source_palette_offset;
	enum srcformat_e	source_format; //format of pixel data (indexed, lightmap, or rgba)
	unsigned int		source_width; //size of image in source data
	unsigned int		source_height; //size of image in source data
	unsigned short		source_crc; //generated by source data before modifications
	int					skinnum;
	int					shirt; //0-13 shirt color, or -1 if never colormapped
	int					pants; //0-13 pants color, or -1 if never colormapped
//used for rendering
	int					visframe; //matches r_framecount if texture was bound this frame
} gltexture_t;

extern gltexture_t *notexture;
extern gltexture_t *nulltexture;
extern gltexture_t *whitetexture; // For missing skins like FVF without the pak

extern unsigned int d_8to24table_fbright[PALETTE_COLORS_256];
extern unsigned int d_8to24table_fbright_alpha[PALETTE_COLORS_256]; // fbright pal but 255 has no alpha
extern unsigned int d_8to24table_nobright[PALETTE_COLORS_256];
extern unsigned int d_8to24table_conchars[PALETTE_COLORS_256];
extern unsigned int d_8to24table_shirt[PALETTE_COLORS_256];
extern unsigned int d_8to24table_pants[PALETTE_COLORS_256];


// TEXTURE MANAGER

float TexMgr_FrameUsage (void);
gltexture_t *TexMgr_FindTexture (qmodel_t *owner, const char *name);
gltexture_t *TexMgr_NewTexture (void);
gltexture_t *TexMgr_FreeTexture (gltexture_t *kill);
void TexMgr_FreeTextures (unsigned int flags, unsigned int mask);
void TexMgr_FreeTexturesForOwner (qmodel_t *owner);
void TexMgr_NewGame (void);
void TexMgr_Init (void);

#ifdef GLQUAKE_TEXTUREGAMMA_SUPPORT
void TexMgr_Gamma_Execute (float newvalue); // Clamped non-zero value
#endif // GLQUAKE_TEXTUREGAMMA_SUPPORT

// IMAGE LOADING
gltexture_t *TexMgr_LoadImage_SetPal (qmodel_t *owner, int bsp_texnum, const char *description_name, int width, int height, enum srcformat_e format,
									  void *data, byte *palette_data, const char *source_file_qpath, src_offset_t source_offset, src_offset_t source_palette_offset, unsigned flags);

gltexture_t *TexMgr_LoadImage (qmodel_t *owner, int bsp_texnum, const char *description_name, int width, int height, enum srcformat_e format,
							   void *data, const char *source_file_qpath, src_offset_t source_offset, unsigned flags);
void TexMgr_ReloadImage (gltexture_t *glt, int shirt, int pants);
void TexMgr_ReloadImages (cbool generate_new_texslot);
void TexMgr_ReloadNobrightImages (void);

int TexMgr_Pad(int s);
int TexMgr_SafeTextureSize (int s);
int TexMgr_PadConditional (int s);

void TexMgr_ReplaceImage_RGBA (gltexture_t *texture, unsigned *data, int width, int height);
void TexMgr_TextureMode_f (cvar_t *var);

#define TEXMODE_GL_NEAREST_0						0
#define TEXMODE_GL_NEAREST_MIPMAP_NEAREST_1			1
#define TEXMODE_GL_NEAREST_MIPMAP_LINEAR_2			2
#define TEXMODE_GL_LINEAR_MIPMAP_LINEAR_5			5

// TEXTURE BINDING & TEXTURE UNIT SWITCHING

void GL_DisableMultitexture (void); //selects texture unit 0
void GL_EnableMultitexture (void); //selects texture unit 1
void GL_Bind (gltexture_t *texture);

#define MAX_COLORMAP_SKINS_1024 1024
extern gltexture_t *playertextures[MAX_COLORMAP_SKINS_1024];

cbool TexMgr_Clipboard_Set (gltexture_t *glt);
const char *TexMgr_TextureModes_ListExport (void); // Baker
void TexMgr_R_SetupView_RecalcWarpImageSize (void);
void TexMgr_R_SetupView_InitUnderwaterWarpTexture (void);



#endif // ! __GL_TEXMGR_H__

