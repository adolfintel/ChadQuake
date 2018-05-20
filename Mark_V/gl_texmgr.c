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

//gl_texmgr.c -- fitzquake's texture manager. manages opengl texture images



#include "quakedef.h"

const int	gl_solid_format = RGB_3;
const int	gl_alpha_format = RGBA_4;

//ericw -- workaround for preventing TexMgr_FreeTexture during TexMgr_ReloadImages
static cbool in_reload_images;


#define	MAX_GLTEXTURES	2048
static int numgltextures;
static gltexture_t	*active_gltextures, *free_gltextures;
gltexture_t		*notexture, *nulltexture, *whitetexture; // whitetexture for missing skins

unsigned int d_8to24table_fbright[PALETTE_COLORS_256];
unsigned int d_8to24table_nobright[PALETTE_COLORS_256];
unsigned int d_8to24table_conchars[PALETTE_COLORS_256];
unsigned int d_8to24table_shirt[PALETTE_COLORS_256];
unsigned int d_8to24table_pants[PALETTE_COLORS_256];
unsigned int d_8to24table_fbright_alpha[PALETTE_COLORS_256]; // fbright pal but 255 has no alpha


void TexMgr_Regamma (float gamma_level);

/*
================================================================================

	COMMANDS

================================================================================
*/

typedef struct
{
	int	magfilter;
	int minfilter;
	const char  *name;
} glmode_t;
static glmode_t glmodes[] = {
	{GL_NEAREST, GL_NEAREST,				"GL_NEAREST"},					// 0
	{GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST,	"GL_NEAREST_MIPMAP_NEAREST"},	// 1 (crunchy?)
	{GL_NEAREST, GL_NEAREST_MIPMAP_LINEAR,	"GL_NEAREST_MIPMAP_LINEAR"},	// 2
	{GL_LINEAR,  GL_LINEAR,					"GL_LINEAR"},					// 3
	{GL_LINEAR,  GL_LINEAR_MIPMAP_NEAREST,	"GL_LINEAR_MIPMAP_NEAREST"},	// 4
	{GL_LINEAR,  GL_LINEAR_MIPMAP_LINEAR,	"GL_LINEAR_MIPMAP_LINEAR"},		// 5  (default)
};
#define NUM_GLMODES (int)(sizeof(glmodes)/sizeof(glmodes[0]))
int glmode_idx = NUM_GLMODES - 1; /* trilinear */

const char *TexMgr_TextureModes_ListExport (void)
{
	static char returnbuf[32];

	static int last = -1; // Top of list.
	// We want first entry >= this
	int		wanted = CLAMP(0, last + 1, NUM_GLMODES );  // Baker: bounds check

	int i;

	for (i = wanted; i < NUM_GLMODES ; i++)
	{

		if (i >= wanted) // Baker: i must be >=want due to way I setup this function
		{
			c_strlcpy (returnbuf, glmodes[i].name);
			String_Edit_To_Lower_Case (returnbuf); // Baker: avoid hassles with uppercase keynames

			last = i;
//			Con_PrintLinef ("Added %s", returnbuf);
			return returnbuf;
		}
	}

	// Not found, reset to top of list and return NULL
	last = -1;
	return NULL;
}


/*
===============
TexMgr_DescribeTextureModes_f -- report available texturemodes
===============
*/
void TexMgr_DescribeTextureModes_f (void)
{
	int i;

	for (i=0; i < NUM_GLMODES; i++)
		Con_SafePrintLinef ("   %2d: %s", i + 1, glmodes[i].name);

	Con_PrintLinef ("%d modes", i);
}

/*
===============
TexMgr_SetFilterModes
===============
*/
static void TexMgr_SetFilterModes (gltexture_t *glt)
{
	GL_Bind (glt);

	if (glt->flags & TEXPREF_NEAREST)
	{
		eglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		eglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	else if (glt->flags & TEXPREF_LINEAR)
	{
		eglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		eglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	else if (glt->flags & TEXPREF_MIPMAP)
	{
		eglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glmodes[glmode_idx].magfilter);
		eglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glmodes[glmode_idx].minfilter);
	}
	else
	{
		eglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glmodes[glmode_idx].magfilter);
		eglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glmodes[glmode_idx].magfilter);
	}

	eglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_texture_anisotropy.value);
}

/*
===============
TexMgr_TextureMode_f -- called when gl_texturemode changes
===============
*/
void TexMgr_TextureMode_f (cvar_t *var)
{
	static int recursion_blocker;
	int n, newidx;

	if (recursion_blocker)
		return; // We are setting ourselves.

	recursion_blocker ++;

	// glmode_idx is the thing we set
	for (newidx = -1, n = 0; n < NUM_GLMODES; n ++) {
		if (String_Does_Match_Caseless (glmodes[n].name, var->string)) {
			newidx = n;
			break;
		}
	}

	glmode_idx = (newidx == -1) ? 0 /* it was invalid set default */ : newidx;

	// Apply it to the textures
	{
		gltexture_t	*glt;
		for (glt = active_gltextures; glt; glt = glt->next)
			TexMgr_SetFilterModes (glt);
	}

	Sbar_Changed (); // sbar graphics need to be redrawn with new filter mode.  Baker: Or at least they used to.  I think I corrected that.

	// Make the name one of the constant values
	Cvar_SetQuick (&gl_texturemode, glmodes[glmode_idx].name); // Set cvar string manually

	recursion_blocker --;
}

#if 0
void TexMgr_TextureMode_f (lparse_t *line)
{
	gltexture_t	*glt;
	const char *arg;
	int i;

	switch (line->count)
	{
	case 1:
		Con_PrintLinef (QUOTEDSTR("gl_texturemode") " is " QUOTED_S, glmodes[glmode_idx].name);
		break;
	case 2:
		arg = line->args[1];
		if (arg[0] == 'G' || arg[0] == 'g')
		{
			for (i=0; i<NUM_GLMODES; i++)
				if (!strcasecmp (glmodes[i].name, arg))
				{
					glmode_idx = i;
					goto stuff;
				}
			Con_PrintLinef (QUOTED_S " is not a valid texturemode", arg);
			return;
		}
		else if (arg[0] >= '0' && arg[0] <= '9')
		{
			i = atoi(arg);
			if (i > NUM_GLMODES || i < 1)
			{
				Con_PrintLinef (QUOTED_S " is not a valid texturemode", arg);
				return;
			}
			glmode_idx = i - 1;
		}
		else
			Con_PrintLinef (QUOTED_S " is not a valid texturemode", arg);

stuff:
		for (glt = active_gltextures; glt; glt=glt->next)
			TexMgr_SetFilterModes (glt);

		Sbar_Changed (); //sbar graphics need to be redrawn with new filter mode

		//FIXME: warpimages need to be redrawn, too.

		break;
	default:
		Con_SafePrintLinef ("usage: gl_texturemode <mode>");
		break;
	}
}
#endif

/*
===============
TexMgr_Anisotropy_f -- called when gl_texture_anisotropy changes

FIXME: this is getting called twice (becuase of the recursive Cvar_SetValue call)
===============
*/
void TexMgr_Anisotropy_f (cvar_t *var)
{
	gltexture_t	*glt;

	Cvar_SetValueQuick (&gl_texture_anisotropy, CLAMP (1.0, gl_texture_anisotropy.value, renderer.gl_max_anisotropy));

	for (glt=active_gltextures; glt; glt=glt->next)
		TexMgr_SetFilterModes (glt);
}

/*
===============
TexMgr_Imagelist_f -- report loaded textures
===============
*/
enum {SHOW_ALL, SHOW_UNREPLACED_BSP};
static void TexMgr_Imagelist_Run (int textures_to_show)
{
	float mb;
	float texels = 0;
	gltexture_t	*glt;
	int txcount;

	for (txcount = 0, glt = active_gltextures; glt; glt=glt->next)
	{
		cbool replaced = glt->owner && glt->source_format == SRC_RGBA; // Only skybox is RGBA and skybox has has no owner

		if (textures_to_show == SHOW_UNREPLACED_BSP)
		{
			cbool isbsp = glt->owner && glt->owner->type == mod_brush;
			cbool isworld = (glt->owner == cl.worldmodel);
			const char *texturename = String_Skip_Char (glt->name, ':'); // Skip the colon

			if (String_Does_Start_With (texturename, "lightmap"))
				continue; // Don't want those!

			if (String_Does_Start_With (texturename, "sky"))
				continue; // Don't want those!

			if (String_Does_Match (texturename, "trigger"))
				continue; // Don't want those!

			if (String_Does_Match (texturename, "clip"))
				continue; // Don't want those!

			if (!(isbsp && isworld && !replaced)) // Isn't worldmodel or isn't replaced
				continue; // We only want unreplaced world model textures ...
		}

		txcount ++;
		Con_SafePrintLinef ("   %4d x%4d %s%s", glt->width, glt->height, glt->name, replaced ? " (R)" : "" );
		if (glt->flags & TEXPREF_MIPMAP)
			texels += glt->width * glt->height * 4.0f / 3.0f;
		else texels += (glt->width * glt->height);
	}
	mb = texels * (vid.desktop.bpp / 8.0f) / 0x100000;
	Con_PrintLinef ("%d textures %d pixels %1.1f megabytes", txcount /*numgltextures*/, (int)texels, mb);
}


void TexMgr_Imagelist_f (lparse_t *line)
{
	if (line->count == 2 && !strcmp(line->args[1], "unreplaced") && cl.worldmodel )
	{
		Con_PrintLinef ("imagelist unreplaced bsp textures only:" NEWLINE);
		TexMgr_Imagelist_Run (SHOW_UNREPLACED_BSP);
	}
	else TexMgr_Imagelist_Run (SHOW_ALL);
}

/*
===============
TexMgr_Imagedump_f -- dump all current textures to TGA files
===============
*/
void TexMgr_Imagedump_f (void)
{
	char image_qpath[MAX_OSPATH], tempname[MAX_OSPATH], dirname[MAX_OSPATH];
	gltexture_t	*glt;
	unsigned *buffer;
	char *c;

	{
		Con_PrintLinef ("This can be slow if there are many textures ..." NEWLINE "especially writing png files ...");
		SCR_UpdateScreen (); // Force!
	}

	//create directory
	FS_FullPath_From_QPath (dirname, "imagedump");
	File_Mkdir (dirname);

	//loop through textures
	for (glt = active_gltextures; glt; glt=glt->next)
	{
		c_strlcpy (tempname, glt->name);
		while ( (c = strchr(tempname, ':')) ) *c = '_';
		while ( (c = strchr(tempname, '/')) ) *c = '_';
		while ( (c = strchr(tempname, '*')) ) *c = '_';
		c_snprintf1 (image_qpath, "imagedump/%s", tempname);

		if (glt->width < 4 || glt->height < 4)
			continue; // Too small for PNG, don't bother.

		GL_Bind (glt);
		if (glt->flags & TEXPREF_ALPHA)
		{
			buffer = (unsigned *) malloc(glt->width*glt->height*RGBA_4);
			eglGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
			Image_Save_PNG_QPath (image_qpath, buffer, glt->width, glt->height);
			//Image_Save_TGA_QPath (image_qpath, buffer, glt->width, glt->height, true);
		}
		else
		{
			buffer = (unsigned *) malloc(glt->width*glt->height*RGBA_4);
			eglGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
			Image_Save_PNG_QPath (image_qpath, buffer, glt->width, glt->height);
			//Image_Save_TGA_QPath (image_qpath, buffer, glt->width, glt->height, true);

		}
		free (buffer);
	}

	Recent_File_Set_FullPath (dirname);

	Con_PrintLinef ("dumped %d textures to %s", numgltextures, dirname);
}

/*
===============
TexMgr_FrameUsage -- report texture memory usage for this frame
===============
*/
float TexMgr_FrameUsage (void)
{
	float mb;
	float texels = 0;
	gltexture_t	*glt;

	for (glt=active_gltextures; glt; glt=glt->next)
	{
		if (glt->visframe == cl.r_framecount)
		{
			if (glt->flags & TEXPREF_MIPMAP)
				texels += glt->width * glt->height * 4.0f / 3.0f;
			else
				texels += (glt->width * glt->height);
		}
	}
	mb = texels * ( vid.desktop.bpp / 8.0f) / 0x100000;
	return mb;
}

/*
================================================================================

	TEXTURE MANAGER

================================================================================
*/

/*
================
TexMgr_FindTexture
================
*/
gltexture_t *TexMgr_FindTexture (qmodel_t *owner, const char *name)
{
	gltexture_t	*glt;

	if (name)
	{
		for (glt=active_gltextures; glt; glt=glt->next)
		{
			if (!strcmp (glt->name, name))
				if (glt->owner == owner)
				return glt;
		}
	}

	return NULL;
}

/*
================
TexMgr_NewTexture
================
*/
gltexture_t *TexMgr_NewTexture (void)
{
	gltexture_t *glt;

	if (numgltextures == MAX_GLTEXTURES)
		System_Error("numgltextures == MAX_GLTEXTURES");

	glt = free_gltextures;
	free_gltextures = glt->next;
	glt->next = active_gltextures;
	active_gltextures = glt;

	eglGenTextures(1, &glt->texnum);
	numgltextures++;
	return glt;
}

/*
================
TexMgr_FreeTexture
================
*/
gltexture_t* TexMgr_FreeTexture (gltexture_t *kill)
{
	gltexture_t *glt;

	if (in_reload_images) // Baker: Does this happen?
		return kill; // Pow!!!  We do not want to kill the slot for reload images, we are reusing the slot.


	if (kill == NULL)
	{
		Con_PrintLinef ("TexMgr_FreeTexture: NULL texture");
		return NULL;
	}

	if (active_gltextures == kill)
	{
		active_gltextures = kill->next;
		kill->next = free_gltextures;
		free_gltextures = kill;

		eglDeleteTextures(1, &kill->texnum);
		numgltextures--;
		return NULL;
	}

	for (glt = active_gltextures; glt; glt = glt->next)
	{
		if (glt->next == kill)
		{
			glt->next = kill->next;
			kill->next = free_gltextures;
			free_gltextures = kill;

			eglDeleteTextures(1, &kill->texnum);
			numgltextures--;
			return NULL;
		}
	}

	Con_PrintLinef ("TexMgr_FreeTexture: not found");
	return NULL;
}

/*
================
TexMgr_FreeTextures

compares each bit in "flags" to the one in glt->flags only if that bit is active in "mask"
================
*/
void TexMgr_FreeTextures (unsigned int flags, unsigned int mask)
{
	gltexture_t *glt, *next;

	for (glt = active_gltextures; glt; glt = next)
	{
		next = glt->next;
		if ((glt->flags & mask) == (flags & mask))
			TexMgr_FreeTexture (glt);
	}
}

/*
================
TexMgr_FreeTexturesForOwner
================
*/
void TexMgr_FreeTexturesForOwner (qmodel_t *owner)
{
	gltexture_t *glt, *next;

	for (glt = active_gltextures; glt; glt = next)
	{
		next = glt->next;
		if (glt && glt->owner == owner)
			TexMgr_FreeTexture (glt);
	}
}

/*
================================================================================

	INIT

================================================================================
*/



 // Set back to 0 if changed

float texmgr_texturegamma_exec = 0; // Set back to 0 if changed
float texmgr_texturegamma_current = -1.0;

#ifdef GLQUAKE_TEXTUREGAMMA_SUPPORT
void TexMgr_Gamma_Execute (float newvalue) // Don't give us a 0
{
//    return;
//	if (newvalue == vid_texturegamma_effective)			// NEVER IGNORE SOMEONE CALLING US.
//		return; // Same.  Nothing to do.
	if (whitetexture == NULL)
		Host_Error ("TexMgr_Gamma_Execute before TexMgr_Init");

	if (newvalue != CLAMP(VID_MIN_POSSIBLE_GAMMA, newvalue, VID_MAX_POSSIBLE_GAMMA))
		Host_Error ("Texture gamma value must be pre-clamped.");

	if (texmgr_texturegamma_current == newvalue) {
		Con_SafePrintLinef ("No gamma change required");
	}

	texmgr_texturegamma_exec = newvalue;

	TexMgr_Regamma (newvalue);
	TexMgr_ReloadImages (false); // false = same slot use
//	Cbuf_AddTextLine ("texreload");
	Con_SafePrintLinef ("Texture gamma applied %g", texmgr_texturegamma_current ? texmgr_texturegamma_current : 1);
}

#endif // GLQUAKE_TEXTUREGAMMA_SUPPORT



/*
=================
TexMgr_LoadPalette -- johnfitz -- was VID_SetPalette, moved here, renamed, rewritten
=================
*/
static byte texture_gammatable_256[GAMMA_UNITS_256];


byte disk_pal_768[PALETTE_SIZE_768];
byte gamma_pal_768[PALETTE_SIZE_768];


void TexMgr_Regamma (float gamma_level)
{
	static int hitcount;
	unsigned alpha_zero_mask = COLOR_RGBA(255,255,255,ALPHA_FULL_TRANSPARENT_0);	// Alpha remover?
	unsigned black = COLOR_RGBA(0,0,0,ALPHA_SOLID_255);
	byte *src, *dst;
	int i, mark = 0 , mark_end = 0;

	hitcount ++;

	if (gamma_level == -1) {
		// Means use most recent gamma
		gamma_level = texmgr_texturegamma_current;
		if (gamma_level == -1) {
			// Must be initialization
			gamma_level = vid_hardwaregamma.value ? 1 : CLAMP(VID_MIN_POSSIBLE_GAMMA, vid_gamma.value, VID_MAX_POSSIBLE_GAMMA);
		}
	}

	memcpy(gamma_pal_768, disk_pal_768, sizeof(gamma_pal_768));  texmgr_texturegamma_current = 0; // Base

	if (gamma_level == 1)
		gamma_level = 0; // Default request may as well be no request.

#ifdef GLQUAKE_TEXTUREGAMMA_SUPPORT
	if (gamma_level) {
		// If texture gamma is wanted, build and apply the gamma table.
		byte *pal_edit;
		texmgr_texturegamma_current = gamma_level;
		Image_Build_Gamma_Table (texmgr_texturegamma_current, 1.0 /*contrast_level*/, texture_gammatable_256);
		for (pal_edit = gamma_pal_768, i = 0 ; i < PALETTE_COLORS_256 ; i ++, pal_edit += RGB_3) {
			pal_edit[0] = texture_gammatable_256[pal_edit[0]],
			pal_edit[1] = texture_gammatable_256[pal_edit[1]],
			pal_edit[2] = texture_gammatable_256[pal_edit[2]];
		}
		gamma_level = 0;
	}
#endif // GLQUAKE_TEXTUREGAMMA_SUPPORT

	// Now do the other tables.

	//standard palette, 255 is transparent
	dst = (byte *)vid.d_8to24table;
	src = gamma_pal_768;
	for (i = 0; i < PALETTE_COLORS_256; i++)
	{
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = ALPHA_SOLID_255;
	}
	vid.d_8to24table[255] &= alpha_zero_mask; // *(int *)mask;

	//fullbright palette, 0-223 are black (for additive blending)
	src = gamma_pal_768 + 224 * RGB_3;
	dst = (byte *)(d_8to24table_fbright) + 224 * RGBA_4;
	for (i = 224; i < PALETTE_COLORS_256; i++)
	{
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = ALPHA_SOLID_255;
	}
	for (i=0; i<224; i++)
		d_8to24table_fbright[i] = black; // *(int *)black;

	//nobright palette, 224-255 are black (for additive blending)
	dst = (byte *)d_8to24table_nobright;
	src = gamma_pal_768;
	for (i = 0; i < PALETTE_COLORS_256; i++)
	{
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = ALPHA_SOLID_255;
	}
	for (i = 224; i < PALETTE_COLORS_256; i++)
		d_8to24table_nobright[i] = black; // *(int *)black;

// Baker: Why do this?  Here is why ...
// 255 is mask color for alpha textures (prefixed "{")
// Now even though 255 pink is a silly color to use in a texture
// no one says you can't do it.  So we need to support that.

// Change 1.  We can't have nobright 255 have alpha.  This should be harmless
// since FitzQuake kills the alpha for color 255 in the regular palette
// d_8to24table.
	d_8to24table_nobright[255] = ALPHA_FULL_TRANSPARENT_0; // Alpha of zero.

// Change 2.  If fullbright pink legitimately is intentionally or
// unintentionally in a map texture, we still want to draw it correctly.
// Even though the usage is extremely uncommon (in old GLQuake days a texture
// might convert some pixels in TexMex to this color and mapper might not know.)
// Either way Quake let's this color be in map textures so we must create
// a separate color conversion for alpha intended textures with the single
// exception of making color 255 black with no alpha.

	memcpy (d_8to24table_fbright_alpha, d_8to24table_fbright, sizeof(d_8to24table_fbright_alpha));
	d_8to24table_fbright_alpha[255] = ALPHA_FULL_TRANSPARENT_0; // Black with no alpha.

	//conchars palette, 0 and 255 are transparent
	memcpy(d_8to24table_conchars, vid.d_8to24table, sizeof(d_8to24table_conchars)); // PALETTE_COLORS_256 * 4);
	d_8to24table_conchars[0] &= alpha_zero_mask; // *(int *)mask;
}


void TexMgr_LoadPalette (void)
{
	FILE *f;

	Con_DPrintLinef ("Gamma level %g", texmgr_texturegamma_exec);
	COM_FOpenFile ("gfx/palette.lmp", &f);
	if (!f)
		System_Error ("Couldn't load gfx/palette.lmp"); // Keep System_Error.  That would be a tier #1 mess.

	fread (disk_pal_768, 1, sizeof(disk_pal_768), f);
	FS_fclose(f);

	TexMgr_Regamma (-1);
}

/*
================
TexMgr_NewGame
================
*/
void TexMgr_NewGame (void)
{
	TexMgr_FreeTextures (0, TEXPREF_PERSIST); //deletes all textures where TEXPREF_PERSIST is unset
	TexMgr_LoadPalette ();
}

/*
=============
TexMgr_RecalcWarpImageSize -- called during init, and after a vid_restart

choose safe warpimage size and resize existing warpimage textures
=============
*/
void TexMgr_R_SetupView_RecalcWarpImageSize (void)
{
	int	mark;
	gltexture_t *glt;
	byte *dummy;

	if (vid.direct3d == 8) // DIRECT3D8_WRAPPER
		return;

	//
	// find the new correct size
	//
	//oldsize = gl_warpimagesize;

	gl_warpimagesize = TexMgr_SafeTextureSize (512); // Baker: This determines the largest that the texture can be.

	// Baker: This finds a power of 2 square smaller than the current width and height
	while (gl_warpimagesize > clwidth)
		gl_warpimagesize >>= 1; // Divide by 2
	while (gl_warpimagesize > clheight)
		gl_warpimagesize >>= 1; // Divide by 2.


	// ericw -- removed early exit if (gl_warpimagesize == oldsize).
	// after vid_restart TexMgr_ReloadImage reloads textures
	// to tx->source_width/source_height, which might not match oldsize.
	// fixes: https://sourceforge.net/p/quakespasm/bugs/13/

	//
	// resize the textures in opengl
	//

	mark = Hunk_LowMark();
	dummy = (byte *) Hunk_AllocName (gl_warpimagesize * gl_warpimagesize * RGBA_4, "warpbuf");

	for (glt=active_gltextures; glt; glt=glt->next)
	{
		if (glt->flags & TEXPREF_WARPIMAGE)
		{
			GL_Bind (glt);
			eglTexImage2D (GL_TEXTURE_2D, 0, gl_solid_format, gl_warpimagesize, gl_warpimagesize, 0, GL_RGBA, GL_UNSIGNED_BYTE, dummy);
			glt->width = glt->height = gl_warpimagesize;
		}
	}

	Hunk_FreeToLowMark (mark);
	
}


gltexture_t *r_underwaterwarptexture;

void TexMgr_R_SetupView_InitUnderwaterWarpTexture (void)
{
	extern byte *hunk_base;

	if (renderer.gl_texture_non_power_of_two && !r_waterwarp_downscale.value)
	{
		// let them choose if they don't want this
		vid.maxwarpwidth = vid.screen.width;
		vid.maxwarpheight = vid.screen.height;
	}
	else
	{
		// called at startup and whenever the video mode changes
		for (vid.maxwarpwidth = 1; vid.maxwarpwidth < vid.screen.width; vid.maxwarpwidth <<= 1);
		for (vid.maxwarpheight = 1; vid.maxwarpheight < vid.screen.height; vid.maxwarpheight <<= 1);
	
		// take a power of 2 down from the screen res so that we can maintain perf if warping
		vid.maxwarpwidth >>= 1;
		vid.maxwarpheight >>= 1;
	}

	Con_DPrintLinef ("Waterwrap texture size is %d, %d", vid.maxwarpwidth, vid.maxwarpheight);

	// note - OpenGL allows specifying NULL data to create an empty texture, but FitzQuake's texmgr doesn't.  So we need to do hunk
	// shenanigans to work around it; if you  modify the texmgr to allow for this, you could just specify NULL data and get rid of
	// this.  in order to minimise the code impact i didn't bother.  using SRC_INDEXED so that risk of overflowing the hunk is minimised.
	r_underwaterwarptexture = TexMgr_LoadImage (NULL, -1 /*not bsp texture*/, "r_waterwarp2", vid.maxwarpwidth, vid.maxwarpheight, SRC_INDEXED, hunk_base, "", (unsigned) hunk_base, TEXPREF_NOPICMIP | TEXPREF_PERSIST | TEXPREF_LINEAR | TEXPREF_WARPIMAGE);

	// the d3d wrapper will need to respecify this as a rendertarget texture so do it now rather than having to check and do it at runtime
	GL_Bind (r_underwaterwarptexture);
	eglCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, vid.maxwarpwidth, vid.maxwarpheight);
}


void TexMgr_Warp_Downscale_f (cvar_t *var)
{
	vid.warp_stale = true;

}


void TexMgr_GreyScale_f (cvar_t *var)
{
	static float lastgreyscale = 0.0; // Depends on a default of 0

	if (gl_greyscale.value == lastgreyscale)
		return;

	lastgreyscale = gl_greyscale.value;

	TexMgr_ReloadImages (0 /* don't generate a slot */);
	vid.warp_stale = true;
}

/*
================
TexMgr_Init

must be called before any texture loading
================
*/
void TexMgr_Init (void)
{
	int i;
	static byte notexture_data[16] = {159,91,83,255,0,0,0,255,0,0,0,255,159,91,83,255}; //black and pink checker
	static byte nulltexture_data[16] = {127,191,255,255,0,0,0,255,0,0,0,255,127,191,255,255}; //black and blue checker
	static byte whitetexture_data[16] = {255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255}; //black and blue checker
	extern texture_t *r_notexture_mip, *r_notexture_mip2;

	// init texture list
	free_gltextures = (gltexture_t *) Hunk_AllocName (MAX_GLTEXTURES * sizeof(gltexture_t), "gltextures");
	active_gltextures = NULL;
	for (i = 0; i < MAX_GLTEXTURES-1; i ++)
		free_gltextures[i].next = &free_gltextures[i+1];
	free_gltextures[i].next = NULL;
	numgltextures = 0;

	// palette
	TexMgr_LoadPalette ();


	Cmd_AddCommands (TexMgr_Init);

	// load notexture images
	notexture = TexMgr_LoadImage (NULL, -1, "notexture", 2, 2, SRC_RGBA, notexture_data, "", (src_offset_t)notexture_data, TEXPREF_NEAREST | TEXPREF_PERSIST | TEXPREF_NOPICMIP);
	nulltexture = TexMgr_LoadImage (NULL, -1, "nulltexture", 2, 2, SRC_RGBA, nulltexture_data, "", (src_offset_t)nulltexture_data, TEXPREF_NEAREST | TEXPREF_PERSIST | TEXPREF_NOPICMIP);
	whitetexture = TexMgr_LoadImage (NULL, -1, "whitetexture", 2, 2, SRC_RGBA, whitetexture_data, "", (src_offset_t)whitetexture_data, TEXPREF_NEAREST | TEXPREF_PERSIST | TEXPREF_NOPICMIP);

	//have to assign these here becuase Mod_Init is called before TexMgr_Init
	r_notexture_mip->gltexture = r_notexture_mip2->gltexture = notexture;

	//set safe size for warpimages
	gl_warpimagesize = 0;
	TexMgr_R_SetupView_RecalcWarpImageSize (); // This must be here.
	TexMgr_R_SetupView_InitUnderwaterWarpTexture (); // This must be here  // UNDERWATER_WARP

	GL_Bloom_Init_Once (); 

	vid.warp_stale = true; // <--- means warp needs recalculated.
}

/*
================================================================================

	IMAGE LOADING

================================================================================
*/

/*
================
TexMgr_Pad -- return smallest power of two greater than or equal to s
================
*/
// Baker
// Endangered:  Use Image_Power_Of_Two_Size for consistency instead
int TexMgr_Pad (int s)
{
	int i;
	for (i = 1; i < s; i<<=1)
		;
	return i;
}

/*
===============
TexMgr_SafeTextureSize -- return a size with hardware and user prefs in mind
===============
*/
int TexMgr_SafeTextureSize (int s)
{
	s = TexMgr_Pad(s);
	if (renderer.gl_max_texture_size > 0)
		s = c_min(TexMgr_Pad(renderer.gl_max_texture_size), s);
	else s = c_min(1024, s);
	return s;
}

/*
================
TexMgr_PadConditional -- only pad if a texture of that size would be padded. (used for tex coords)
================
*/
int TexMgr_PadConditional (int s)
{
	if (s < TexMgr_SafeTextureSize(s))
		return TexMgr_Pad(s);
	else
		return s;
}

/*
================
TexMgr_MipMapW
================
*/
static unsigned *TexMgr_MipMapW (unsigned *data, int width, int height)
{
	int		i, size;
	byte	*out, *in;

	out = in = (byte *)data;
	size = (width*height)>>1;

	for (i=0; i<size; i++, out+=4, in+=8)
	{
		out[0] = (in[0] + in[4])>>1;
		out[1] = (in[1] + in[5])>>1;
		out[2] = (in[2] + in[6])>>1;
		out[3] = (in[3] + in[7])>>1;
	}

	return data;
}

/*
================
TexMgr_MipMapH
================
*/
static unsigned *TexMgr_MipMapH (unsigned *data, int width, int height)
{
	int		i, j;
	byte	*out, *in;

	out = in = (byte *)data;
	height >>=1;
	width <<=2;

	for (i=0; i<height; i++, in+=width)
	{
		for (j=0; j<width; j+=4, out += RGBA_4, in += RGBA_4)
		{
			out[0] = (in[0] + in[width+0])>>1;
			out[1] = (in[1] + in[width+1])>>1;
			out[2] = (in[2] + in[width+2])>>1;
			out[3] = (in[3] + in[width+3])>>1;
		}
	}

	return data;
}


/*
================
TexMgr_ResampleTextureForce
================
*/
// No one calls us!  Jan 2017
//static unsigned *TexMgr_ResampleTextureForce (unsigned *in, int inwidth, int inheight, int outwidth, int outheight, cbool alpha)
//{
//	byte *nwpx, *nepx, *swpx, *sepx, *dest;
//	unsigned xfrac, yfrac, x, y, modx, mody, imodx, imody, injump, outjump;
//	unsigned *out;
//	int i, j;
//
//	out = Hunk_Alloc(outwidth*outheight*RGBA_4);
//
//	xfrac = ((inwidth-1) << 16) / (outwidth-1);
//	yfrac = ((inheight-1) << 16) / (outheight-1);
//	y = outjump = 0;
//
//	for (i=0; i<outheight; i++)
//	{
//		mody = (y>>8) & 0xFF;
//		imody = 256 - mody;
//		injump = (y>>16) * inwidth;
//		x = 0;
//
//		for (j=0; j<outwidth; j++)
//		{
//			modx = (x>>8) & 0xFF;
//			imodx = 256 - modx;
//
//			nwpx = (byte *)(in + (x>>16) + injump);
//			nepx = nwpx + 4;
//			swpx = nwpx + inwidth*4;
//			sepx = swpx + 4;
//
//			dest = (byte *)(out + outjump + j);
//
//			dest[0] = (nwpx[0]*imodx*imody + nepx[0]*modx*imody + swpx[0]*imodx*mody + sepx[0]*modx*mody)>>16;
//			dest[1] = (nwpx[1]*imodx*imody + nepx[1]*modx*imody + swpx[1]*imodx*mody + sepx[1]*modx*mody)>>16;
//			dest[2] = (nwpx[2]*imodx*imody + nepx[2]*modx*imody + swpx[2]*imodx*mody + sepx[2]*modx*mody)>>16;
//			if (alpha)
//				dest[3] = (nwpx[3]*imodx*imody + nepx[3]*modx*imody + swpx[3]*imodx*mody + sepx[3]*modx*mody)>>16;
//			else
//				dest[3] = ALPHA_SOLID_255;
//
//			x += xfrac;
//		}
//		outjump += outwidth;
//		y += yfrac;
//	}
//
//	return out;
//}


/*
================
TexMgr_ResampleTexture -- bilinear resample
================
*/
static unsigned *TexMgr_LoadImage32_ResampleTexture_NPOT (unsigned *in, int inwidth, int inheight, cbool alpha)
{
	byte *nwpx, *nepx, *swpx, *sepx, *dest;
	unsigned xfrac, yfrac, x, y, modx, mody, imodx, imody, injump, outjump;
	unsigned *out;
	int i, j, outwidth, outheight;

	if (inwidth == TexMgr_Pad(inwidth) && inheight == TexMgr_Pad(inheight))
		return in;

	outwidth = TexMgr_Pad(inwidth);
	outheight = TexMgr_Pad(inheight);
	out = (unsigned *)Hunk_Alloc(outwidth*outheight*RGBA_4);

	xfrac = ((inwidth-1) << 16) / (outwidth-1);
	yfrac = ((inheight-1) << 16) / (outheight-1);
	y = outjump = 0;

	for (i=0; i<outheight; i++)
	{
		mody = (y>>8) & 0xFF;
		imody = 256 - mody;
		injump = (y>>16) * inwidth;
		x = 0;

		for (j=0; j<outwidth; j++)
		{
			modx = (x>>8) & 0xFF;
			imodx = 256 - modx;

			nwpx = (byte *)(in + (x>>16) + injump);
			nepx = nwpx + 4;
			swpx = nwpx + inwidth*4;
			sepx = swpx + 4;

			dest = (byte *)(out + outjump + j);

			dest[0] = (nwpx[0]*imodx*imody + nepx[0]*modx*imody + swpx[0]*imodx*mody + sepx[0]*modx*mody)>>16;
			dest[1] = (nwpx[1]*imodx*imody + nepx[1]*modx*imody + swpx[1]*imodx*mody + sepx[1]*modx*mody)>>16;
			dest[2] = (nwpx[2]*imodx*imody + nepx[2]*modx*imody + swpx[2]*imodx*mody + sepx[2]*modx*mody)>>16;
			if (alpha)
				dest[3] = (nwpx[3]*imodx*imody + nepx[3]*modx*imody + swpx[3]*imodx*mody + sepx[3]*modx*mody)>>16;
			else
				dest[3] = 255;

			x += xfrac;
		}
		outjump += outwidth;
		y += yfrac;
	}

	return out;
}

/*
===============
TexMgr_AlphaEdgeFix

eliminate pink edges on sprites, etc.
operates in place on 32bit data
===============
*/
static void TexMgr_AlphaEdgeFix (byte *data, int width, int height)
{
	int i, j, n = 0, b, c[3] = {0,0,0},
		lastrow, thisrow, nextrow,
		lastpix, thispix, nextpix;
	byte *dest = data;

	for (i=0; i<height; i++)
	{
		lastrow = width * 4 * ((i == 0) ? height-1 : i-1);
		thisrow = width * 4 * i;
		nextrow = width * 4 * ((i == height-1) ? 0 : i+1);

		for (j=0; j<width; j++, dest+=4)
		{
			if (dest[3]) //not transparent
				continue;

			lastpix = 4 * ((j == 0) ? width-1 : j-1);
			thispix = 4 * j;
			nextpix = 4 * ((j == width-1) ? 0 : j+1);

			b = lastrow + lastpix; if (data[b+3]) {c[0] += data[b]; c[1] += data[b+1]; c[2] += data[b+2]; n++;}
			b = thisrow + lastpix; if (data[b+3]) {c[0] += data[b]; c[1] += data[b+1]; c[2] += data[b+2]; n++;}
			b = nextrow + lastpix; if (data[b+3]) {c[0] += data[b]; c[1] += data[b+1]; c[2] += data[b+2]; n++;}
			b = lastrow + thispix; if (data[b+3]) {c[0] += data[b]; c[1] += data[b+1]; c[2] += data[b+2]; n++;}
			b = nextrow + thispix; if (data[b+3]) {c[0] += data[b]; c[1] += data[b+1]; c[2] += data[b+2]; n++;}
			b = lastrow + nextpix; if (data[b+3]) {c[0] += data[b]; c[1] += data[b+1]; c[2] += data[b+2]; n++;}
			b = thisrow + nextpix; if (data[b+3]) {c[0] += data[b]; c[1] += data[b+1]; c[2] += data[b+2]; n++;}
			b = nextrow + nextpix; if (data[b+3]) {c[0] += data[b]; c[1] += data[b+1]; c[2] += data[b+2]; n++;}

			//average all non-transparent neighbors
			if (n)
			{
				dest[0] = (byte)(c[0]/n);
				dest[1] = (byte)(c[1]/n);
				dest[2] = (byte)(c[2]/n);

				n = c[0] = c[1] = c[2] = 0;
			}
		}
	}
}

/*
===============
TexMgr_PadEdgeFixW -- special case of AlphaEdgeFix for textures that only need it because they were padded

operates in place on 32bit data, and expects unpadded height and width values
===============
*/
static void TexMgr_PadEdgeFixW (byte *data, int width, int height)
{
	byte *src, *dst;
	int i, padw, padh;

	padw = TexMgr_PadConditional(width);
	padh = TexMgr_PadConditional(height);

	//copy last full column to first empty column, leaving alpha byte at zero
	src = data + (width - 1) * 4;
	for (i=0; i<padh; i++)
	{
		src[4] = src[0];
		src[5] = src[1];
		src[6] = src[2];
		src += padw * 4;
	}

	//copy first full column to last empty column, leaving alpha byte at zero
	src = data;
	dst = data + (padw - 1) * 4;
	for (i=0; i<padh; i++)
	{
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		src += padw * 4;
		dst += padw * 4;
	}
}

/*
===============
TexMgr_PadEdgeFixH -- special case of AlphaEdgeFix for textures that only need it because they were padded

operates in place on 32bit data, and expects unpadded height and width values
===============
*/
static void TexMgr_PadEdgeFixH (byte *data, int width, int height)
{
	byte *src, *dst;
	int i, padw, padh;

	padw = TexMgr_PadConditional(width);
	padh = TexMgr_PadConditional(height);

	//copy last full row to first empty row, leaving alpha byte at zero
	dst = data + height * padw * 4;
	src = dst - padw * 4;
	for (i=0; i<padw; i++)
	{
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		src += 4;
		dst += 4;
	}

	//copy first full row to last empty row, leaving alpha byte at zero
	dst = data + (padh - 1) * padw * 4;
	src = data;
	for (i=0; i<padw; i++)
	{
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		src += 4;
		dst += 4;
	}
}

/*
================
TexMgr_8to32
================
*/
unsigned *TexMgr_8to32 (byte *in, int pixels, unsigned int *usepal)
{
	int i;
	unsigned *out, *data;

	out = data = (unsigned *) Hunk_Alloc(pixels * RGBA_4);

	for (i=0 ; i<pixels ; i++)
		*out++ = usepal[*in++];

	return data;
}

/*
================
TexMgr_PadImageW -- return image with width padded up to power-of-two dimentions
================
*/
static byte *TexMgr_PadImageW (byte *in, int width, int height, byte padbyte)
{
	int i, j, outwidth;
	byte *out, *data;

	if (width == TexMgr_Pad(width))
		return in;

	outwidth = TexMgr_Pad(width);

	out = data = (byte *) Hunk_Alloc(outwidth*height);

	for (i=0; i<height; i++)
	{
		for (j=0; j<width; j++)
			*out++ = *in++;
		for (   ; j<outwidth; j++)
			*out++ = padbyte;
	}

	return data;
}

/*
================
TexMgr_PadImageH -- return image with height padded up to power-of-two dimentions
================
*/
static byte *TexMgr_PadImageH (byte *in, int width, int height, byte padbyte)
{
	int i, srcpix, dstpix;
	byte *data, *out;

	if (height == TexMgr_Pad(height))
		return in;

	srcpix = width * height;
	dstpix = width * TexMgr_Pad(height);

	out = data = (byte *) Hunk_Alloc(dstpix);

	for (i=0; i<srcpix; i++)
		*out++ = *in++;
	for (   ; i<dstpix; i++)
		*out++ = padbyte;

	return data;
}

byte quakePalette [] =
{
      0,   0,   0,
     15,  15,  15,
     31,  31,  31,
     47,  47,  47,
     63,  63,  63,
     75,  75,  75,
     91,  91,  91,
    107, 107, 107,
    123, 123, 123,
    139, 139, 139,
    155, 155, 155,
    171, 171, 171,
    187, 187, 187,
    203, 203, 203,
    219, 219, 219,
    235, 235, 235,
     15,  11,   7,
     23,  15,  11,
     31,  23,  11,
     39,  27,  15,
     47,  35,  19,
     55,  43,  23,
     63,  47,  23,
     75,  55,  27,
     83,  59,  27,
     91,  67,  31,
     99,  75,  31,
    107,  83,  31,
    115,  87,  31,
    123,  95,  35,
    131, 103,  35,
    143, 111,  35,
     11,  11,  15,
     19,  19,  27,
     27,  27,  39,
     39,  39,  51,
     47,  47,  63,
     55,  55,  75,
     63,  63,  87,
     71,  71, 103,
     79,  79, 115,
     91,  91, 127,
     99,  99, 139,
	107, 107, 151,
    115, 115, 163,
    123, 123, 175,
    131, 131, 187,
    139, 139, 203,
      0,   0,   0,
      7,   7,   0,
     11,  11,   0,
     19,  19,   0,
     27,  27,   0,
     35,  35,   0,
     43,  43,   7,
     47,  47,   7,
     55,  55,   7,
     63,  63,   7,
     71,  71,   7,
     75,  75,  11,
     83,  83,  11,
     91,  91,  11,
     99,  99,  11,
    107, 107,  15,
      7,   0,   0,
     15,   0,   0,
     23,   0,   0,
     31,   0,   0,
     39,   0,   0,
     47,   0,   0,
     55,   0,   0,
     63,   0,   0,
     71,   0,   0,
     79,   0,   0,
     87,   0,   0,
     95,   0,   0,
    103,   0,   0,
    111,   0,   0,
    119,   0,   0,
    127,   0,   0,
     19,  19,   0,
     27,  27,   0,
     35,  35,   0,
     47,  43,   0,
     55,  47,   0,
     67,  55,   0,
     75,  59,   7,
     87,  67,   7,
     95,  71,   7,
    107,  75,  11,
    119,  83,  15,
    131,  87,  19,
    139,  91,  19,
    151,  95,  27,
    163,  99,  31,
    175, 103,  35,
     35,  19,   7,
     47,  23,  11,
     59,  31,  15,
     75,  35,  19,
     87,  43,  23,
     99,  47,  31,
    115,  55,  35,
    127,  59,  43,
    143,  67,  51,
    159,  79,  51,
    175,  99,  47,
    191, 119,  47,
    207, 143,  43,
    223, 171,  39,
    239, 203,  31,
    255, 243,  27,
     11,   7,   0,
     27,  19,   0,
     43,  35,  15,
     55,  43,  19,
     71,  51,  27,
     83,  55,  35,
     99,  63,  43,
    111,  71,  51,
    127,  83,  63,
    139,  95,  71,
    155, 107,  83,
    167, 123,  95,
    183, 135, 107,
    195, 147, 123,
    211, 163, 139,
    227, 179, 151,
    171, 139, 163,
    159, 127, 151,
    147, 115, 135,
    139, 103, 123,
    127,  91, 111,
    119,  83,  99,
    107,  75,  87,
     95,  63,  75,
     87,  55,  67,
     75,  47,  55,
     67,  39,  47,
     55,  31,  35,
     43,  23,  27,
     35,  19,  19,
     23,  11,  11,
     15,   7,   7,
    187, 115, 159,
    175, 107, 143,
    163,  95, 131,
    151,  87, 119,
    139,  79, 107,
    127,  75,  95,
    115,  67,  83,
    107,  59,  75,
     95,  51,  63,
     83,  43,  55,
     71,  35,  43,
     59,  31,  35,
     47,  23,  27,
     35,  19,  19,
     23,  11,  11,
     15,   7,   7,
    219, 195, 187,
    203, 179, 167,
    191, 163, 155,
    175, 151, 139,
    163, 135, 123,
    151, 123, 111,
    135, 111,  95,
    123,  99,  83,
    107,  87,  71,
     95,  75,  59,
     83,  63,  51,
     67,  51,  39,
     55,  43,  31,
     39,  31,  23,
     27,  19,  15,
     15,  11,   7,
    111, 131, 123,
    103, 123, 111,
     95, 115, 103,
     87, 107,  95,
     79,  99,  87,
     71,  91,  79,
     63,  83,  71,
     55,  75,  63,
     47,  67,  55,
     43,  59,  47,
     35,  51,  39,
     31,  43,  31,
     23,  35,  23,
     15,  27,  19,
     11,  19,  11,
      7,  11,   7,
    255, 243,  27,
    239, 223,  23,
    219, 203,  19,
    203, 183,  15,
    187, 167,  15,
    171, 151,  11,
    155, 131,   7,
    139, 115,   7,
    123,  99,   7,
    107,  83,   0,
     91,  71,   0,
     75,  55,   0,
     59,  43,   0,
     43,  31,   0,
     27,  15,   0,
     11,   7,   0,
      0,   0, 255,
     11,  11, 239,
     19,  19, 223,
     27,  27, 207,
     35,  35, 191,
     43,  43, 175,
     47,  47, 159,
     47,  47, 143,
     47,  47, 127,
     47,  47, 111,
     47,  47,  95,
     43,  43,  79,
     35,  35,  63,
     27,  27,  47,
     19,  19,  31,
     11,  11,  15,
     43,   0,   0,
     59,   0,   0,
     75,   7,   0,
     95,   7,   0,
    111,  15,   0,
    127,  23,   7,
    147,  31,   7,
    163,  39,  11,
    183,  51,  15,
    195,  75,  27,
    207,  99,  43,
    219, 127,  59,
    227, 151,  79,
    231, 171,  95,
    239, 191, 119,
    247, 211, 139,
    167, 123,  59,
    183, 155,  55,
    199, 195,  55,
    231, 227,  87,
    127, 191, 255,
    171, 231, 255,
    215, 255, 255,
    103,   0,   0,
    139,   0,   0,
    179,   0,   0,
    215,   0,   0,
    255,   0,   0,
    255, 243, 147,
    255, 247, 199,
    255, 255, 255,
    159,  91,  83,
};

#define SQUARED(x) ((x)*(x))
void Pixel_Apply_Best_Palette_Index (byte *red, byte *green, byte *blue, const byte *myPalette, const int* myPaletteNumColors)
{
	int				bestColorIndex		=  -1;
	int				bestColorDistance	=  -1;
	const byte*		currentPaletteIndex	= myPalette;
	int				i;

	for (i = 0; i < *myPaletteNumColors; i++, currentPaletteIndex += 3 )
	{
		const int		redDistance		= *red   - currentPaletteIndex[0];
		const int		greenDistance	= *green - currentPaletteIndex[1];
		const int		blueDistance	= *blue  - currentPaletteIndex[2];
		const int		ColorDistance	= SQUARED(redDistance) + SQUARED(greenDistance) + SQUARED(blueDistance); // Could sqrt this but no reason to do so

		if (bestColorDistance == -1 || ColorDistance < bestColorDistance)
		{
			bestColorDistance	= ColorDistance;
			bestColorIndex		= i;
		}
	}

	*red	= myPalette[bestColorIndex * 3 + 0];
	*green	= myPalette[bestColorIndex * 3 + 1];
	*blue	= myPalette[bestColorIndex * 3 + 2];

}



/*
================
TexMgr_LoadImage32 -- handles 32bit source data
================
*/
#pragma message ("Baker: Finish texgamma implementation and stagger it")
#pragma message ("Baker: Also be sure to process the warp images")
#pragma message ("Baker: And exclude the lightmap")

static void TexMgr_LoadImage32 (gltexture_t *glt, unsigned *data)
{
	int	internalformat,	miplevel, mipwidth, mipheight, picmip;

	// resample up
	if (!renderer.gl_texture_non_power_of_two)
	{
		data = TexMgr_LoadImage32_ResampleTexture_NPOT (data, glt->width, glt->height, glt->flags & TEXPREF_ALPHA);
		glt->width = TexMgr_Pad(glt->width);
		glt->height = TexMgr_Pad(glt->height);
	}

	// mipmap down
	picmip = 0;
	mipwidth = TexMgr_SafeTextureSize (glt->width >> picmip);
	mipheight = TexMgr_SafeTextureSize (glt->height >> picmip);
	while ((int) glt->width > mipwidth)
	{
		TexMgr_MipMapW (data, glt->width, glt->height);
		glt->width >>= 1;
		if (Flag_Check (glt->flags, TEXPREF_ALPHA) )
			TexMgr_AlphaEdgeFix ((byte *)data, glt->width, glt->height);
	}
	while ((int) glt->height > mipheight)
	{
		TexMgr_MipMapH (data, glt->width, glt->height);
		glt->height >>= 1;
		if (Flag_Check (glt->flags, TEXPREF_ALPHA) )
			TexMgr_AlphaEdgeFix ((byte *)data, glt->width, glt->height);
	}

	{ // BEGIN BLOCK
		unsigned *myData = data;
		int pixelcount = glt->width * glt->height;
		cbool madecopy = false;
		int i;

	// What if lightmap?  Well, we are in the 32 bits zone here.
		//if (glt->flags & TEXPREF_WARPIMAGE) // Wrong place to do this, as this is not a real image.  And it's WAY too late at this point.
		// Remember a warp image supplies data we shall not write to.  Sigh.			
		if (texmgr_texturegamma_current && !Flag_Check(glt->flags, TEXPREF_WARPIMAGE) && !Flag_Check(glt->flags, TEXPREF_BLENDED) && isin2(glt->source_format, SRC_RGBA, SRC_INDEXED_WITH_PALETTE)) {
			for (i = 0; i < pixelcount; i++) {
				// Baker: Not that we care but this isn't Big Endian friendly.
				byte *pixel = (byte*)&myData[i];
				pixel[0] = texture_gammatable_256[pixel[0]];
				pixel[1] = texture_gammatable_256[pixel[1]];
				pixel[2] = texture_gammatable_256[pixel[2]];
			}
		}

		if ( (gl_greyscale.value == 2 && glt->source_format == SRC_RGBA) || gl_greyscale.value == 1)
		{
			float favg;
			byte	average;
			myData = malloc (pixelcount * RGBA_4);
			memcpy (myData, data, pixelcount * RGBA_4);
			madecopy = true;
			//	{
			// !(glt->flags & TEXPREF_PERSIST)
			// Baker: TEXPREF_PERSIST cannot be overwritten, trying to attempt so would be an access violation.
			for (i = 0; i < pixelcount; i++)
			{
				byte *pixel = (byte*)&myData[i];
				if (gl_greyscale.value == 2)
				{
					static const int numNoFullBrightColors = 224;
					Pixel_Apply_Best_Palette_Index (&pixel[0], &pixel[1], &pixel[2], quakePalette, &numNoFullBrightColors);
				}
				else
				{
					favg = 0.299f * pixel[0] /* red */ + 0.587f * pixel[1] /*green*/ + 0.114f * pixel[2] /*blue*/;
					favg = CLAMP(0, favg, 255);
					average = (byte)(favg + 0.5f);
					pixel[0] = pixel[1] = pixel[2] = average;
//					if (pixel[3] != 255 && pixel[3] !=0)
//						i=i;
				}
			}
			// END GREYSCALE
		}

		// upload
		GL_Bind (glt);
		internalformat = (glt->flags & TEXPREF_ALPHA) ? gl_alpha_format : gl_solid_format;
		eglTexImage2D (GL_TEXTURE_2D, 0, internalformat, glt->width, glt->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, myData);

		// upload mipmaps
		if (Flag_Check(glt->flags, TEXPREF_MIPMAP) && vid.direct3d != 9) // MH says DX9 NO MIPMAP doesn't need to mipmap.
		{
			mipwidth = glt->width;
			mipheight = glt->height;

			for (miplevel=1; mipwidth > 1 || mipheight > 1; miplevel++)
			{
				if (mipwidth > 1)
				{
					TexMgr_MipMapW (myData, mipwidth, mipheight);
					mipwidth >>= 1;
				}
				if (mipheight > 1)
				{
					TexMgr_MipMapH (myData, mipwidth, mipheight);
					mipheight >>= 1;
				}
				eglTexImage2D (GL_TEXTURE_2D, miplevel, internalformat, mipwidth, mipheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, myData);
			}
		}

		if (madecopy)
			free (myData);
	} // END BLOCK

	// set filter modes
	TexMgr_SetFilterModes (glt);
}

/*
================
TexMgr_LoadImage8 -- handles 8bit source data, then passes it to LoadImage32
================
*/
static void TexMgr_LoadImage8 (gltexture_t *glt, byte *data, byte *palette_data)
{

	cbool padw = false, padh = false;
	byte padbyte; // Formerly byte
	unsigned int *usepal;
	int i;
	unsigned int custom_palette [257]; // +1 for black index of 256

	// Baker: This must be here for texture reload
	// HACK HACK HACK -- taken from tomazquake
#define SHOT1SID_WIDTH_32 32
#define SHOT1SID_HEIGHT_32 32
	if (strstr(glt->name, "shot1sid") &&
		glt->width == SHOT1SID_WIDTH_32 && glt->height == SHOT1SID_HEIGHT_32 &&
		CRC_Block(data, 1024) == 65393)
	{
		// This texture in b_shell1.bsp has some of the first 32 pixels painted white.
		// They are invisible in software, but look really ugly in GL. So we just copy
		// 32 pixels from the bottom to make it look nice.
		memcpy (data, data + SHOT1SID_WIDTH_32 * 31, SHOT1SID_WIDTH_32);
	}

	// detect false alpha cases
	if ((glt->flags  & TEXPREF_ALPHA) && !(glt->flags & TEXPREF_CONCHARS))
	{
		for (i = 0; i < (int) (glt->width * glt->height); i++)
			if (data[i] == 255) //transparent index
				break;
		if (i == (int) (glt->width * glt->height))
			glt->flags -= TEXPREF_ALPHA;
	}

	// choose palette and padbyte
	if (glt->source_format == SRC_INDEXED_WITH_PALETTE)
	{
		int k;
		byte *palette = palette_data;

		for (k = 0; k < PALETTE_COLORS_256; k ++, palette += RGB_3 /* RGB is +=3 */)
		{
			byte	red = palette[0];
			byte	green = palette[1];
			byte	blue = palette[2];
			byte	alpha = 255;

			custom_palette[k] = ((unsigned)red + ((unsigned)green << 8) + ((unsigned)blue << 16) + ((unsigned)alpha << 24));
		}

		custom_palette[256] = 0 + ((unsigned)255 << 24); // Solid black with full alpha

		if (glt->flags & TEXPREF_ALPHA)
			custom_palette[255] = 0;

		usepal = custom_palette;

		// Baker: This looks wrong but we are using the last index
		// of "custom_palette" that has 257 entries.  It's ok.
		// This is for a special case of alpha textures  where
		// fullbright pink is acceptable -- which is color 255.
		// so I had to make an entry beyond the normal range.
		// To allow fullbright pink to NOT be masked.  I wish I could
		// remember the specific example where I spotted this either
		// with a map or a monster or a weapon. Baker -1 for bad notes.
		// And even though I vaguely recall, I know this is right
		// because I remember discovering the issue and being very
		// disappointed before I fixed it.
#pragma message ("Fix me!  Was I worn out or what?  This only applies to Half-Life maps, what was the issue I was trying to fix.")
#pragma message ("I'm sure I was trying to solve a legitimate problem, but what was problem and where because this won't work right.")
		padbyte = (byte) 256;
	}
	else
	if (glt->flags & TEXPREF_FULLBRIGHT)
	{
// If an alpha intended image, use color conversion where 255 has no alpha
		if (glt->flags & TEXPREF_ALPHA)
			usepal = d_8to24table_fbright_alpha;
		else
			usepal = d_8to24table_fbright;
		padbyte = 0;
	}
	else if (glt->flags & TEXPREF_NOBRIGHT && gl_fullbrights.value)
	{
		usepal = d_8to24table_nobright;
		padbyte = 0;
	}
	else if (glt->flags & TEXPREF_CONCHARS)
	{
		usepal = d_8to24table_conchars;
		padbyte = 0;
	}
	else
	{
		usepal = vid.d_8to24table;
		padbyte = 255;
	}

	// pad each dimension, but only if it's not going to be downsampled later
	if (glt->flags & TEXPREF_PAD)
	{
		if ((int) glt->width < TexMgr_SafeTextureSize(glt->width))
		{
			data = TexMgr_PadImageW (data, glt->width, glt->height, padbyte);
			glt->width = TexMgr_Pad(glt->width);
			padw = true;
		}
		if ((int) glt->height < TexMgr_SafeTextureSize(glt->height))
		{
			data = TexMgr_PadImageH (data, glt->width, glt->height, padbyte);
			glt->height = TexMgr_Pad(glt->height);
			padh = true;
		}
	}

	// convert to 32bit
	data = (byte *)TexMgr_8to32(data, glt->width * glt->height, usepal);

	// fix edges
	if (glt->flags & TEXPREF_ALPHA)
		TexMgr_AlphaEdgeFix (data, glt->width, glt->height);
	else
	{
		if (padw)
			TexMgr_PadEdgeFixW (data, glt->source_width, glt->source_height);
		if (padh)
			TexMgr_PadEdgeFixH (data, glt->source_width, glt->source_height);
	}

	// upload it
	TexMgr_LoadImage32 (glt, (unsigned *)data);
}

/*
================
TexMgr_LoadLightmap -- handles lightmap data
================
*/
static void TexMgr_LoadLightmap (gltexture_t *glt, byte *data)
{
	// upload it
	GL_Bind (glt);

	eglTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, glt->width, glt->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	// set filter modes
	TexMgr_SetFilterModes (glt);
}

/*
================
TexMgr_LoadImage -- the one entry point for loading all textures
================
*/
gltexture_t *TexMgr_LoadImage (qmodel_t *owner, int bsp_texnum, const char *name, int width, int height, enum srcformat_e format,
							   void *data, const char *source_file, src_offset_t source_offset, unsigned flags)
{
	return TexMgr_LoadImage_SetPal (owner, bsp_texnum, name, width, height, format, data, NULL, source_file, source_offset, 0, flags);

}


gltexture_t *TexMgr_LoadImage_SetPal (qmodel_t *owner, int bsp_texnum, const char *name, int width, int height, enum srcformat_e format,
							   void *data, byte *palette_data, const char *source_file, src_offset_t source_offset, src_offset_t source_palette_offset, unsigned flags)
{

	unsigned short crc;
	gltexture_t *glt;
	int mark;

	if (isDedicated)
		return NULL; // Baker: Can this really happen or is this just safety?

	// cache check
	switch (format)
	{
		case SRC_INDEXED_WITH_PALETTE:  // To do: Baker ... CRC the 32 bit data instead ...
			crc = CRC_Block(data, width * height);
			break;
		case SRC_INDEXED:
			crc = CRC_Block(data, width * height);
			break;
		case SRC_LIGHTMAP:
		case SRC_RGBA:
			crc = CRC_Block(data, width * height * RGBA_4);
			break;
		default: /* not reachable but avoids compiler warnings */
			crc = 0;
	}
	if ((flags & TEXPREF_OVERWRITE) && (glt = TexMgr_FindTexture (owner, name)))
	{
		if (glt->source_crc == crc)
			return glt;
	}
	else
		glt = TexMgr_NewTexture ();

	// copy data
	glt->owner = owner;
	c_strlcpy (glt->name, name);

	glt->width = width;
	glt->height = height;
	glt->flags = flags;
	glt->skinnum = 0;
	glt->bsp_texnum = bsp_texnum;
	glt->shirt = -1;
	glt->pants = -1;
	c_strlcpy (glt->source_file, source_file);
	glt->source_offset = source_offset;
	glt->source_palette_offset = source_palette_offset;
//	Con_PrintLinef ("%s source offset %d pal offset %d and delta %d", glt->name, source_offset, source_palette_offset, source_palette_offset - source_offset);
	glt->source_format = format;
	glt->source_width = width;
	glt->source_height = height;
	glt->source_crc = crc;

	//upload it
	mark = Hunk_LowMark();

	switch (glt->source_format)
	{
		case SRC_INDEXED:
		case SRC_INDEXED_WITH_PALETTE:
			TexMgr_LoadImage8 (glt, data, palette_data);
			break;
		case SRC_LIGHTMAP:
			TexMgr_LoadLightmap (glt, data);
			break;
		case SRC_RGBA:
			TexMgr_LoadImage32 (glt, (unsigned *)data);
			break;
	}

	Hunk_FreeToLowMark(mark);

	return glt;
}

/*
================================================================================

	COLORMAPPING AND TEXTURE RELOADING

================================================================================
*/

// , const char *cache_filename
void TexMgr_ReplaceImage_RGBA (gltexture_t* texture, unsigned *data, int width, int height)
{
#if 0
	c_strlcpy (texture->paste_texture_name, cache_filename, sizeof(paste_texture_name) );
	texture->paste_texture_width		= width;
	texture->	paste_texture_height	= height;
	texture->crc = CRC_Block(data, width * height * RGBA_4);
#else
	texture->source_format	= SRC_RGBA;
	texture->source_width	= texture->width  = width;
	texture->source_height	= texture->height = height;

	texture->source_crc		= CRC_Block(data, width * height * RGBA_4);

	if (texture->bsp_texnum != -1)
	{
		texture_t* maptexture = texture->owner->textures[texture->bsp_texnum];
#pragma message ("This isn't a fix, we must properly delete it?")
		maptexture->fullbright = NULL; // Nuke fullbright texture
	}

#endif

//
// upload it
//
	switch (texture->source_format)
	{
		case SRC_RGBA:
			TexMgr_LoadImage32 (texture, (unsigned *)data);
			break;
		default:
			Con_PrintLinef ("Error");
			return;
	}

#pragma message ("Need to turn fullbright to none?")

// Warp texture?
	if (texture->flags & TEXPREF_WARPIMAGE)
		vid.warp_stale = true;
#pragma message ("Isn't this too much?  Only that texture changed")

}

/*
================
TexMgr_ReloadImage -- reloads a texture, and colormaps it if needed
================
*/
// Called mainly by Reloadimages also Draw_TransPicTranslate in menu
// and TexMgr_ReloadImage
void TexMgr_ReloadImage (gltexture_t *glt, int shirt, int pants)
{
	byte		translation[PALETTE_COLORS_256];
	byte		*src, *dst, *translated;
	void		*data = NULL;
	byte		*palette_data = NULL;
	int			mark, size, i;
	FILE		*f = NULL;

//
// get source data
//
	mark = Hunk_LowMark ();

	if (glt->source_file[0] && glt->source_offset)
	{
		//lump inside file
		FILE	*f = NULL;
		int		datasize;
		COM_FOpenFile (glt->source_file, &f);

		if (!f)
			goto invalid;

		datasize = glt->source_width * glt->source_height;
		if (glt->source_format == SRC_RGBA)
			datasize *= RGBA_4;

		data = Hunk_Alloc (datasize);

		// need SEEK_CUR for PAKs and WADs
		fseek (f, glt->source_offset, SEEK_CUR);
		fread (data, datasize, 1, f);

		if (glt->source_format == SRC_INDEXED_WITH_PALETTE)
		{
			size_t net_offset = glt->source_palette_offset - glt->source_offset - datasize;
			datasize = PALETTE_SIZE_768;
			palette_data = Hunk_Alloc (datasize);
			fseek (f, net_offset, SEEK_CUR);
			fread (palette_data, datasize, 1, f);
		}

		// we must null this so that it doesn't try to fclose twice
		FS_fclose (f);
		f = NULL;
	}
	else if (glt->source_file[0] && !glt->source_offset)
		data = Image_Load (glt->source_file, &glt->source_width, &glt->source_height); //simple file
	else if (!glt->source_file[0] && glt->source_offset)
	{
		data = (byte *) glt->source_offset; //image in memory
		if (glt->source_format == SRC_INDEXED_WITH_PALETTE)
			palette_data = (byte *) glt->source_palette_offset;
	}

	if (!data)
	{
invalid:
		Con_PrintLinef ("TexMgr_ReloadImage: invalid source for %s", glt->name);
		Hunk_FreeToLowMark(mark);
		if (f) FS_fclose (f);
		return;
	}

	glt->width = glt->source_width;
	glt->height = glt->source_height;
//
// apply shirt and pants colors
//
// if shirt and pants are -1,-1, use existing shirt and pants colors
// if existing shirt and pants colors are -1,-1, don't bother colormapping
	if (shirt > -1 && pants > -1)
	{
		if (glt->source_format == SRC_INDEXED)
		{
			glt->shirt = shirt;
			glt->pants = pants;
		}
		else Con_PrintLinef ("TexMgr_ReloadImage: can't colormap a non SRC_INDEXED texture: %s", glt->name);
	}
	if (glt->shirt > -1 && glt->pants > -1)
	{
		//create new translation table
		for (i = 0 ; i < PALETTE_COLORS_256 ; i++)
			translation[i] = i;

		shirt = glt->shirt * 16;
		if (shirt < 128)
		{
			for (i = 0 ; i < 16 ; i++)
				translation[TOP_RANGE + i] = shirt + i;
		}
		else
		{
			for (i = 0 ; i < 16 ; i++)
				translation[TOP_RANGE + i] = shirt + 15 - i;
		}

		pants = glt->pants * 16;
		if (pants < 128)
		{
			for (i = 0 ; i < 16 ; i++)
				translation[BOTTOM_RANGE+i] = pants + i;
		}
		else
		{
			for (i = 0 ; i < 16 ; i++)
				translation[BOTTOM_RANGE+i] = pants + 15 - i;
		}

		//translate texture
		size = glt->width * glt->height;
		dst = translated = (byte *) Hunk_Alloc (size);
		src = data;

		for (i=0; i<size; i++)
			*dst++ = translation[*src++];

		data = translated;
	}
	// fill the source (8-bit only)
	if ((glt->flags & TEXPREF_FLOODFILL) && glt->source_format == SRC_INDEXED)
	{
		Mod_FloodFillSkin (data, glt->source_width, glt->source_height);
	}
//
// upload it
//
	switch (glt->source_format)
	{
		case SRC_INDEXED:
		case SRC_INDEXED_WITH_PALETTE:
			TexMgr_LoadImage8 (glt, data, palette_data);
			break;
		case SRC_LIGHTMAP:
			TexMgr_LoadLightmap (glt, data);
			break;
		case SRC_RGBA:
			TexMgr_LoadImage32 (glt, (unsigned *)data);
			break;
	}

	Hunk_FreeToLowMark(mark);
	if (f) FS_fclose (f);
}


/*
================
TexMgr_ReloadImages -- reloads all texture images
================
*/
// Called by only by vid_restart, gl_greyscale.

void TexMgr_ReloadImages (cbool generate_new_texslot)
{
	gltexture_t *glt;

	int count;
	in_reload_images = true;
	for (glt = active_gltextures, count = 0; glt; glt = glt->next, count ++)
	{
		if (glt->flags & TEXPREF_WARPIMAGE) // Warp images don't reload.
			continue;
		if (generate_new_texslot)
			eglGenTextures(1, &glt->texnum);
		TexMgr_ReloadImage (glt, -1, -1);
	}
	in_reload_images = false;
}





/*
================
TexMgr_ReloadImages -- reloads all texture images
================
*/
// Called by only by vid_restart, gl_greyscale.

void TexMgr_ReloadImages_f (void)
{
	TexMgr_ReloadImages (0);
}


/*
================
TexMgr_DeleteImages -- delete them all
================
*/
void TexMgr_DeleteImages (cbool gamma_texture_only)
{
	gltexture_t *glt;

	int count;
	for (glt=active_gltextures, count = 0; glt; glt = glt->next, count ++)
	{
		eglDeleteTextures(1, &glt->texnum);
	}
}


/*
================
TexMgr_ReloadNobrightImages -- reloads all texture that were loaded with the nobright palette.  called when gl_fullbrights changes
================
*/
void TexMgr_ReloadNobrightImages (void)
{
	gltexture_t *glt;

	for (glt=active_gltextures; glt; glt=glt->next)
		if (glt->flags & TEXPREF_NOBRIGHT)
			TexMgr_ReloadImage(glt, -1, -1);
}

static unsigned *TexMgr_Texture_To_RGBA_From_Source_Main (gltexture_t *glt, int *hunkclearmark, int *numbytes)
{
	int				mark = *hunkclearmark = Hunk_LowMark ();

	int				pixelbytes = glt->source_format == SRC_RGBA ? RGBA_4 : 1;
	int				datasize = glt->source_width * glt->source_height* pixelbytes;

	unsigned int	custom_palette[PALETTE_COLORS_256];
	unsigned int 	*palette_to_use = vid.d_8to24table; // Default

	void			*rawdata = NULL;
	FILE			*f = NULL;
	unsigned		*data = NULL;

//
// get source data
//
#pragma message ("Right now we can't copy a replaced texture correctly because we aren't writing a cache file")

	if (!glt->source_file[0] && glt->source_offset)
	{

		rawdata = (byte *) glt->source_offset; //image in memory
		if (glt->source_format == SRC_INDEXED_WITH_PALETTE)
		{
			int	palette_datasize = PALETTE_SIZE_768;
			int k;
			byte *palette_data = Hunk_Alloc (palette_datasize), *palette;
			palette_data = (byte *) glt->source_palette_offset;

			// And turn it into data
			for (palette = palette_data, k = 0; k < PALETTE_COLORS_256; k ++, palette += RGB_3 /* RGB is 3*/)
			{
				byte r = palette[0], g = palette[1], b = palette[2], a = 255;
				custom_palette[k] = ((unsigned)r + ((unsigned)g << 8) + ((unsigned)b << 16) + ((unsigned)a << 24));
			}

			palette_to_use = custom_palette;
		}
	}
	else
	{

		switch (glt->source_offset)
		{
		case 0:
			// simple file.  Which will not have a Quake/Half-Life palette nor be indexed.
			rawdata = Image_Load (glt->source_file, &glt->source_width, &glt->source_height);
			break;

		default:
			// Could be anything because it is in a pak or a map or who knows.
			COM_FOpenFile (glt->source_file, &f);
			if (!f)
				break;

			do
			{
				int k;

				rawdata = Hunk_Alloc (datasize);

				// need SEEK_CUR for PAKs and WADs
				fseek (f, glt->source_offset, SEEK_CUR);
				fread (rawdata, datasize, 1, f);

				if (glt->source_format == SRC_INDEXED_WITH_PALETTE)
				{
					// Read palette too
					int net_offset = glt->source_palette_offset - glt->source_offset - datasize;
					int	palette_datasize = PALETTE_SIZE_768;
					byte *palette_data = Hunk_Alloc (palette_datasize), *palette;

					fseek (f, net_offset, SEEK_CUR);
					fread (palette_data, palette_datasize, 1, f);

					// And turn it into data
					for (palette = palette_data, k = 0; k < PALETTE_COLORS_256; k ++, palette += RGB_3 /* RGB is 3*/)
					{
						byte r = palette[0], g = palette[1], b = palette[2], a = 255;
						custom_palette[k] = ((unsigned)r + ((unsigned)g << 8) + ((unsigned)b << 16) + ((unsigned)a << 24));
					}

					palette_to_use = custom_palette;
				}
				// done
			} while (0);

		}
	}

	if (f) FS_fclose (f);

	if (!rawdata)
		return NULL;

	switch (glt->source_format)
	{
		case SRC_INDEXED:
		case SRC_INDEXED_WITH_PALETTE:
			data = TexMgr_8to32 (rawdata, glt->source_width * glt->source_height, palette_to_use);
			break;

		case SRC_RGBA:
			// Nothing to do, should work as is
			data = rawdata;
			break;
	}

	// We always produce RGBA output which is 4 bytes per pixel
	*numbytes = glt->source_width * glt->source_height * RGBA_4;
	return data;
}


unsigned *TexMgr_Texture_To_RGBA_From_Source (gltexture_t *glt, int *hunkclearmark, int *bytes)
{
	FILE	*f = NULL;

	return TexMgr_Texture_To_RGBA_From_Source_Main (glt, hunkclearmark, bytes);
}

cbool TexMgr_Clipboard_Set (gltexture_t *glt)
{
	cbool ok = false;
	int mark, bytes;
	unsigned *data_rgba = TexMgr_Texture_To_RGBA_From_Source (glt, &mark, &bytes);

	if (!data_rgba)
	{
		Con_PrintLinef  ("Couldn't get texture %s source data", glt->name);
		return false;
	}

	ok = Clipboard_Set_Image (data_rgba, glt->source_width, glt->source_height);

	Hunk_FreeToLowMark (mark);

	if (ok)
		Con_PrintLinef ("Copied %s to clipboard", glt->name);
	else Con_PrintLinef ("Clipboard copy for %s failed", glt->name);

	return ok;
}

#if 0

/*
===============
TexMgr_Imagedump_f -- dump all current textures to TGA files
===============
*/
static void TexMgr_Replacement_Save_f (void)
{
	char tganame[MAX_OSPATH], tempname[MAX_OSPATH], dirname[MAX_OSPATH];
	gltexture_t	*glt;
	byte *buffer;
	char *c;

	//create directory
	FS_FullPath_From_QPath (dirname, "imagedump");
	File_Mkdir (dirname);

	//loop through textures
	for (glt=active_gltextures; glt; glt=glt->next)
	{
		c_strlcpy (tempname, glt->name, sizeof(tempname));
		while ( (c = strchr(tempname, ':')) ) *c = '_';
		while ( (c = strchr(tempname, '/')) ) *c = '_';
		while ( (c = strchr(tempname, '*')) ) *c = '_';
		c_snprintf1 (tganame, sizeof(tganame), "imagedump/%s.tga", tempname);

		GL_Bind (glt);
		if (glt->flags & TEXPREF_ALPHA)
		{
			buffer = (byte *) malloc(glt->width*glt->height*RGBA_4);
			eglGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
			Image_WriteTGA_QPath (tganame, buffer, glt->width, glt->height, BPP_32, true);
		}
		else
		{
			buffer = (byte *) malloc(glt->width*glt->height*RGB_3);
			eglGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
			Image_WriteTGA_QPath (tganame, buffer, glt->width, glt->height, BPP_24, true);
		}
		free (buffer);
	}

	Recent_File_Set_FullPath (va("%s/", dirname)`);

	Con_PrintLinef ("dumped %d textures to %s", numgltextures, dirname);
}

static void TexMgr_Replacement_Load_f (void)
{
	char tganame[MAX_OSPATH], tempname[MAX_OSPATH], dirname[MAX_OSPATH];
	gltexture_t	*glt;
	byte *buffer;
	char *c;

	//create directory
	FS_FullPath_From_QPath (dirname, "imagedump");
	File_Mkdir (dirname);

	//loop through textures
	for (glt=active_gltextures; glt; glt=glt->next)
	{
		c_strlcpy (tempname, glt->name, sizeof(tempname));
		while ( (c = strchr(tempname, ':')) ) *c = '_';
		while ( (c = strchr(tempname, '/')) ) *c = '_';
		while ( (c = strchr(tempname, '*')) ) *c = '_';
		c_snprintf1 (tganame, sizeof(tganame), "imagedump/%s.tga", tempname);

		GL_Bind (glt);
		if (glt->flags & TEXPREF_ALPHA)
		{
			buffer = (byte *) malloc(glt->width*glt->height*RGBA_4);
			eglGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
			Image_WriteTGA_QPath (tganame, buffer, glt->width, glt->height, 32, true);
		}
		else
		{
			buffer = (byte *) malloc(glt->width*glt->height*RGB_3);
			eglGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
			Image_WriteTGA_QPath (tganame, buffer, glt->width, glt->height, 24, true);
		}
		free (buffer);
	}

	Recent_File_Set_FullPath (dirname);

	Con_PrintLinef ("dumped %d textures to %s", numgltextures, dirname);
}

#endif



/*
================================================================================

	TEXTURE BINDING / TEXTURE UNIT SWITCHING

================================================================================
*/

int	currenttexture = -1; // to avoid unnecessary texture sets

cbool mtexenabled = false;

/*
================
GL_SelectTexture -- johnfitz -- rewritten
================
*/
static void GL_SelectTexture (GLenum target)
{
	static GLenum currenttarget;
	static int ct0, ct1;

	if (target == currenttarget)
		return;

	renderer.GL_SelectTextureFunc(target);

	if (target == renderer.TEXTURE0)
	{
		ct1 = currenttexture;
		currenttexture = ct0;
	}
	else //target == TEXTURE1
	{
		ct0 = currenttexture;
		currenttexture = ct1;
	}

	currenttarget = target;
}

/*
================
GL_DisableMultitexture -- selects texture unit 0
================
*/
void GL_DisableMultitexture (void)
{
	if (mtexenabled)
	{
		eglDisable(GL_TEXTURE_2D);
		GL_SelectTexture(renderer.TEXTURE0); //johnfitz -- no longer SGIS specific
		mtexenabled = false;
	}
}

/*
================
GL_EnableMultitexture -- selects texture unit 1
================
*/
void GL_EnableMultitexture(void)
{
	if (renderer.gl_mtexable)
	{
		GL_SelectTexture(renderer.TEXTURE1); //johnfitz -- no longer SGIS specific
		eglEnable(GL_TEXTURE_2D);
		mtexenabled = true;
	}
}

/*
================
GL_Bind -- johnfitz -- heavy revision
================
*/
void GL_Bind (gltexture_t *texture)
{
	if (!texture)
		texture = nulltexture;

#if 0
	if (texture->is_mirror)  // debugging
		texture = texture;
#endif

	if ((int)texture->texnum != currenttexture)
	{
		currenttexture = texture->texnum;
		eglBindTexture (GL_TEXTURE_2D, currenttexture);
		texture->visframe = cl.r_framecount;
	}
}

#endif // GLQUAKE specific