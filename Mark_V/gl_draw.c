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

// draw.c -- 2d drawing



#include "quakedef.h"


//extern unsigned char d_15to8table[65536]; //johnfitz -- never used



qpic_t		*draw_disc;
qpic_t		*draw_backtile;

gltexture_t *char_texture; //johnfitz
gltexture_t *crosshair_weapon_textures[MAX_CROSSHAIRS_25]; // gfx/crosshairs/crosshair1.tga
int			crosshair_weapon_textures_found;

gltexture_t *crosshair_default_texture; // gfx/crosshairs/crosshair1.tga

qpic_t		*pic_ovr, *pic_ins; //johnfitz -- new cursor handling
qpic_t		*pic_nul; //johnfitz -- for missing gfx, don't crash

//johnfitz -- new pics
byte pic_ovr_data[8][8] =
{
	{255,255,255,255,255,255,255,255},
	{255, 15, 15, 15, 15, 15, 15,255},
	{255, 15, 15, 15, 15, 15, 15,  2},
	{255, 15, 15, 15, 15, 15, 15,  2},
	{255, 15, 15, 15, 15, 15, 15,  2},
	{255, 15, 15, 15, 15, 15, 15,  2},
	{255, 15, 15, 15, 15, 15, 15,  2},
	{255,255,  2,  2,  2,  2,  2,  2},
};

byte pic_ins_data[9][8] =
{
	{ 15, 15,255,255,255,255,255,255},
	{ 15, 15,  2,255,255,255,255,255},
	{ 15, 15,  2,255,255,255,255,255},
	{ 15, 15,  2,255,255,255,255,255},
	{ 15, 15,  2,255,255,255,255,255},
	{ 15, 15,  2,255,255,255,255,255},
	{ 15, 15,  2,255,255,255,255,255},
	{ 15, 15,  2,255,255,255,255,255},
	{255,  2,  2,255,255,255,255,255},
};

byte pic_nul_data[8][8] =
{
	{252,252,252,252,  0,  0,  0,  0},
	{252,252,252,252,  0,  0,  0,  0},
	{252,252,252,252,  0,  0,  0,  0},
	{252,252,252,252,  0,  0,  0,  0},
	{  0,  0,  0,  0,252,252,252,252},
	{  0,  0,  0,  0,252,252,252,252},
	{  0,  0,  0,  0,252,252,252,252},
	{  0,  0,  0,  0,252,252,252,252},
};

byte pic_stipple_data[8][8] =
{
	{255,  0,  0,  0,255,  0,  0,  0},
	{  0,  0,255,  0,  0,  0,255,  0},
	{255,  0,  0,  0,255,  0,  0,  0},
	{  0,  0,255,  0,  0,  0,255,  0},
	{255,  0,  0,  0,255,  0,  0,  0},
	{  0,  0,255,  0,  0,  0,255,  0},
	{255,  0,  0,  0,255,  0,  0,  0},
	{  0,  0,255,  0,  0,  0,255,  0},
};

byte pic_crosshair_data[8][8] =
{
	{255,255,255,255,255,255,255,255},
	{255,255,255,  8,  9,255,255,255},
	{255,255,255,  6,  8,  2,255,255},
	{255,  6,  8,  8,  6,  8,  8,255},
	{255,255,  2,  8,  8,  2,  2,  2},
	{255,255,255,  7,  8,  2,255,255},
	{255,255,255,255,  2,  2,255,255},
	{255,255,255,255,255,255,255,255},
};
//johnfitz

typedef struct
{
	gltexture_t *gltexture;
	float		sl, tl, sh, th;
} glpic_t;

canvastype currentcanvas = CANVAS_NONE; //johnfitz -- for Draw_SetCanvas

/* Baker: I moved this into draw.c
typedef struct
{
	// Current drawing
	int x;			// x offset from clx (not supported in WinQuake right now)
	int y;			// y offset from cly (not supported in WinQuake right now)
	int width;
	int height;
	float scale_x;
	float scale_y;
} canvas_rect_t;
*/

canvas_rect_t canvas;


//==============================================================================
//
//  PIC CACHING
//
//==============================================================================

typedef struct cachepic_s
{
	char		name[MAX_QPATH_64];
	qpic_t		pic;
	byte		padding[32];	// for appended glpic
} cachepic_t;

#define	MAX_CACHED_PICS		128
cachepic_t	menu_cachepics[MAX_CACHED_PICS];
int			menu_numcachepics;

byte		menuplyr_pixels[4096];

//  scrap allocation
//  Allocate all the little status bar obejcts into a single texture
//  to crutch up stupid hardware / drivers

#define	MAX_SCRAPS		2
#define	BLOCK_WIDTH		256
#define	BLOCK_HEIGHT	256

int			scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte		scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT]; //johnfitz -- removed *4 after BLOCK_HEIGHT
cbool	scrap_dirty;
gltexture_t	*scrap_textures[MAX_SCRAPS]; //johnfitz


/*
================
Scrap_AllocBlock

returns an index into scrap_texnums[] and the position inside it
================
*/
int Scrap_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		texnum;

	for (texnum = 0 ; texnum < MAX_SCRAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i = 0 ; i < BLOCK_WIDTH - w ; i++)
		{
			best2 = 0;

			for (j=0 ; j < w ; j++)
			{
				if (scrap_allocated[texnum][i+j] >= best)
					break;
				if (scrap_allocated[texnum][i+j] > best2)
					best2 = scrap_allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i = 0 ; i < w ; i++)
			scrap_allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	System_Error ("Scrap_AllocBlock: full"); //johnfitz -- correct function name
	return 0; //johnfitz -- shut up compiler
}

/*
================
Scrap_Upload -- johnfitz -- now uses TexMgr
================
*/
void Scrap_Upload (void)
{
	char name[8];
	int	i;

	for (i=0; i<MAX_SCRAPS; i++)
	{
		c_snprintf1 (name, "scrap%d", i);
		scrap_textures[i] = TexMgr_LoadImage (NULL, -1 /*not bsp texture*/, name, BLOCK_WIDTH, BLOCK_HEIGHT, SRC_INDEXED, scrap_texels[i],
			"", (src_offset_t)scrap_texels[i], TEXPREF_ALPHA | /* Baker: Crisp -> */ TEXPREF_NEAREST | TEXPREF_OVERWRITE | TEXPREF_NOPICMIP);
	}

	scrap_dirty = false;
}


#ifdef SUPPORTS_LEVELS_MENU_HACK
#include "mark_v_lmp.h" // gfx/levels.lmp (levels_lmp) and gfx/demos.lmp (demos_lmp)

int levels_pic_size = sizeof(levels_lmp);
int demos_pic_size = sizeof(demos_lmp);

extern int normal_menu;
extern int normal_help;
extern int normal_backtile;
#endif // SUPPORTS_LEVELS_MENU_HACK

/*
================
Draw_PicFromWad
================
*/
qpic_t *Draw_PicFromWad (const char *name)
{
	qpic_t	*p;
	glpic_t	gl;
	src_offset_t offset; //johnfitz

	p = (qpic_t *)W_GetLumpName (name);
	if (!p) return pic_nul; //johnfitz

#ifdef SUPPORTS_LEVELS_MENU_HACK
	if (!strcmp (name, "backtile"))
	{
		FILE *f;
		if (COM_FOpenFile (WADFILENAME, &f))
		{
			if (!strstr(com_filepath, "/id1/"))
				normal_backtile = 0; // No!
			else
				normal_backtile = -1; // Undetermined
			FS_fclose (f);
		}
	}
#endif

	do
	{
		// Try the external texture which will never go to the scrap
		if (gl_external_textures.value)
		{
			int mark = Hunk_LowMark ();
			char current_filename[MAX_OSPATH];
			char limit_path[MAX_OSPATH];
			int fwidth, fheight;
			unsigned *data;

			c_snprintf1 (current_filename, "/gfx/%s", name);
			c_strlcpy (limit_path, com_filepath);

			data = Image_Load_Limited (current_filename, &fwidth, &fheight, limit_path);//, mod->loadinfo.searchpath);
			
			if (data)
			{
				gl.gltexture = TexMgr_LoadImage (NULL, -1 /*not bsp texture*/, current_filename, fwidth, fheight, SRC_RGBA, data, current_filename,
											  0, TEXPREF_ALPHA | /* Baker: Crisp -> */ TEXPREF_NEAREST | TEXPREF_PAD | TEXPREF_NOPICMIP); //johnfitz -- TexMgr
				gl.sl = gl.tl = 0;
				gl.sh = gl.th = 1;
				Hunk_FreeToLowMark (mark); // Should be no files to close, LoadImage closes them.

#ifdef SUPPORTS_LEVELS_MENU_HACK
				if (normal_backtile == -1)
					normal_backtile = 0;
#endif

				break; // Done!  Get out and return the data!
			}
			// No external data ...
		}

	// load little ones into the scrap
	if (p->width < 64 && p->height < 64)
	{
		int		x = 0, y = 0;
		int		i, j, k;
		int		texnum;

		texnum = Scrap_AllocBlock (p->width, p->height, &x, &y);
		scrap_dirty = true;
		k = 0;
		for ( i =0 ; i < p->height ; i++)
		{
			for (j = 0 ; j < p->width ; j++, k++)
			{
#if 0
				scrap_texels[texnum][(y + i) * BLOCK_WIDTH + x + j] = p->data[k];
#else
				int calc = (y + i) * BLOCK_WIDTH + x + j;
				if (calc < 0)
					calc = calc;
				scrap_texels[texnum][calc] = p->data[k];
#endif
			}
		}
		gl.gltexture = scrap_textures[texnum]; //johnfitz -- changed to an array
		//johnfitz -- no longer go from 0.01 to 0.99
		gl.sl = x/(float)BLOCK_WIDTH;
		gl.sh = (x+p->width)/(float)BLOCK_WIDTH;
		gl.tl = y/(float)BLOCK_WIDTH;
		gl.th = (y+p->height)/(float)BLOCK_WIDTH;
	}
	else
	{
		char texturename[64]; //johnfitz
		c_snprintf2 (texturename, "%s:%s", WADFILENAME, name); //johnfitz

		offset = (src_offset_t)p - (src_offset_t)wad_base + sizeof(int)*2; //johnfitz

		gl.gltexture = TexMgr_LoadImage (NULL, -1 /*not bsp texture*/, texturename, p->width, p->height, SRC_INDEXED, p->data, WADFILENAME,
										  offset, TEXPREF_ALPHA | /* Baker: Crisp -> */ TEXPREF_NEAREST | TEXPREF_PAD | TEXPREF_NOPICMIP); //johnfitz -- TexMgr
		gl.sl = 0;
		gl.sh = (float)p->width/(float)TexMgr_PadConditional(p->width); //johnfitz
		gl.tl = 0;
		gl.th = (float)p->height/(float)TexMgr_PadConditional(p->height); //johnfitz
	}
	} while (0);
	memcpy (p->data, &gl, sizeof(glpic_t));

#ifdef SUPPORTS_LEVELS_MENU_HACK
				if (normal_backtile == -1)
					normal_backtile = 1;
#endif

	return p;
}

/*
================
Draw_CachePic
================
*/
qpic_t	*Draw_CachePic (const char *path)
{
	cachepic_t	*pic;
	int			i;
	qpic_t		*dat;
	glpic_t		gl;

	for (pic=menu_cachepics, i=0 ; i<menu_numcachepics ; pic++, i++)
	{
		if (!strcmp (path, pic->name))
			return &pic->pic;
	}
	if (menu_numcachepics == MAX_CACHED_PICS)
		System_Error ("menu_numcachepics == MAX_CACHED_PICS");
	menu_numcachepics++;
	c_strlcpy (pic->name, path);
//
// load the pic from disk
//

#ifdef SUPPORTS_LEVELS_MENU_HACK
	if (!strcmp ("gfx/levels.lmp", path))
	{
		//int levels_pic_size = sizeof(levels_lmp);
		//int demos_pic_size = sizeof(demos_lmp);
		dat = (qpic_t *)levels_lmp;
	}
	else
	if (!strcmp ("gfx/demos.lmp", path))
	{
		dat = (qpic_t *)demos_lmp;
	}
	else
#endif // SUPPORTS_LEVELS_MENU_HACK
		dat = (qpic_t *)COM_LoadTempFile (path);

	if (!dat)
		System_Error ("Draw_CachePic: failed to load %s", path);

	SwapPic (dat);

	// HACK HACK HACK --- we need to keep the bytes for
	// the translatable player picture just for the menu
	// configuration dialog
	if (!strcmp (path, "gfx/menuplyr.lmp"))
		memcpy (menuplyr_pixels, dat->data, dat->width*dat->height);

	pic->pic.width = dat->width;
	pic->pic.height = dat->height;

#ifdef SUPPORTS_LEVELS_MENU_HACK
	if (!strcmp (path, "gfx/sp_menu.lmp"))
	{
		if (!strstr(com_filepath, "/id1/"))
			normal_menu = 0; // No!
		else
			normal_menu = -1; // Undetermined
	}
	else if (!strcmp (path, "gfx/help0.lmp"))
	{
		if (!strstr(com_filepath, "/id1/"))
			normal_help = 0; // No!
		else
			normal_help = -1; // Undetermined
	}
#endif // SUPPORTS_LEVELS_MENU_HACK

	do
	{
		// Check for external data
		if (gl_external_textures.value)
		{
			int mark = Hunk_LowMark ();
			char current_filename[MAX_OSPATH];
			char limit_path[MAX_OSPATH];
			int fwidth, fheight;
			unsigned *data;

			File_URL_Copy_StripExtension (current_filename, path,  sizeof(current_filename));
			c_strlcpy (limit_path, com_filepath);

			data = Image_Load_Limited (current_filename, &fwidth, &fheight, limit_path);
			if (data)
			{
				gl.gltexture = TexMgr_LoadImage (NULL, -1 /*not bsp texture*/, path, fwidth, fheight, SRC_RGBA, data, current_filename /* was path*/,
											  0, TEXPREF_ALPHA | /* Baker: Crisp -> */ TEXPREF_NEAREST | TEXPREF_PAD | TEXPREF_NOPICMIP); //johnfitz -- TexMgr
				gl.sl = gl.tl = 0;
				gl.sh = gl.th = 1;
				Hunk_FreeToLowMark (mark); // Should be no files to close, LoadImage closes them.

#ifdef SUPPORTS_LEVELS_MENU_HACK
				if (normal_menu == -1)
					normal_menu = 0;
				if (normal_help == -1)
					normal_menu = 0;
#endif // SUPPORTS_LEVELS_MENU_HACK

				break; // Done!  Get out and return the data!
			}
			// No external data ...
		}

#ifdef SUPPORTS_LEVELS_MENU_HACK
		if (!strcmp ("gfx/levels.lmp", path))

		gl.gltexture = TexMgr_LoadImage (NULL, -1 /*not bsp texture*/, path, dat->width, dat->height, SRC_INDEXED, dat->data, "",
									  (src_offset_t)levels_lmp + sizeof(int)*2, TEXPREF_ALPHA | /* Baker: Crisp -> */ TEXPREF_NEAREST | TEXPREF_PAD | TEXPREF_NOPICMIP); //johnfitz -- TexMgr
		else if (!strcmp ("gfx/demos.lmp", path))
		gl.gltexture = TexMgr_LoadImage (NULL, -1 /*not bsp texture*/, path, dat->width, dat->height, SRC_INDEXED, dat->data, "",
									  (src_offset_t)demos_lmp + sizeof(int)*2, TEXPREF_ALPHA | /* Baker: Crisp -> */ TEXPREF_NEAREST | TEXPREF_PAD | TEXPREF_NOPICMIP); //johnfitz -- TexMgr
		else
#endif // SUPPORTS_LEVELS_MENU_HACK
		gl.gltexture = TexMgr_LoadImage (NULL, -1 /*not bsp texture*/, path, dat->width, dat->height, SRC_INDEXED, dat->data, path,
								  sizeof(int)*2, TEXPREF_ALPHA | /* Baker: Crisp -> */ TEXPREF_NEAREST | TEXPREF_PAD | TEXPREF_NOPICMIP); //johnfitz -- TexMgr



		gl.sl = gl.tl = 0;
		gl.sh = (float)dat->width/(float)TexMgr_PadConditional(dat->width); //johnfitz
		gl.th = (float)dat->height/(float)TexMgr_PadConditional(dat->height); //johnfitz

#ifdef SUPPORTS_LEVELS_MENU_HACK
		if (normal_menu == -1)
			normal_menu = 1;
		if (normal_help == -1)
			normal_help = 1;
#endif // SUPPORTS_LEVELS_MENU_HACK

	} while (0);
	memcpy (pic->pic.data, &gl, sizeof(glpic_t));

	return &pic->pic;
}


/*
================
Draw_MakePic -- johnfitz -- generate pics from internal data
================
*/
qpic_t *Draw_MakePic (const char *name, int width, int height, byte *data)
{
	int flags = TEXPREF_NEAREST | TEXPREF_ALPHA | TEXPREF_PERSIST | TEXPREF_NOPICMIP | TEXPREF_PAD;
	qpic_t		*pic;
	glpic_t		gl;

	pic = (qpic_t *) Hunk_Alloc (sizeof(qpic_t) - 4 /*4 = sizeof qpic_t data, aka member size of data*/ + sizeof (glpic_t));
	pic->width = width;
	pic->height = height;

	gl.gltexture = TexMgr_LoadImage (NULL, -1 /*not bsp texture*/, name, width, height, SRC_INDEXED, data, "", (src_offset_t)data, flags);
	gl.sl = 0;
	gl.sh = (float)width/(float)TexMgr_PadConditional(width);
	gl.tl = 0;
	gl.th = (float)height/(float)TexMgr_PadConditional(height);
	memcpy (pic->data, &gl, sizeof(glpic_t));

	return pic;
}

//==============================================================================
//
//  INIT
//
//==============================================================================

/*
===============
Draw_LoadPics -- johnfitz
===============
*/
void Draw_LoadPics (void)
{
	do
	{
		// Check for external data
		if (gl_external_textures.value)
		{
			int mark = Hunk_LowMark ();
			char current_filename[MAX_OSPATH] = "/gfx/conchars";
			char limit_path[MAX_OSPATH];
			int fwidth, fheight;
			unsigned *data;

			c_strlcpy (limit_path, com_filepath);
			data = Image_Load_Limited (current_filename, &fwidth, &fheight, limit_path);
			if (data)
			{
				char_texture = TexMgr_LoadImage (NULL, -1 /*not bsp texture*/, current_filename, fwidth, fheight, SRC_RGBA, data, current_filename,
											  0, TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_NOPICMIP); //johnfitz -- TexMgr
				Hunk_FreeToLowMark (mark); // Should be no files to close, LoadImage closes them.
				break; // Done!  Get out and return the data!
			}
		}

		{
			byte		*data = (byte *)W_GetLumpName ("conchars");
	src_offset_t	offset;

			if (!data)
				System_Error ("Draw_LoadPics: couldn't load conchars");

		offset = (src_offset_t)data - (src_offset_t)wad_base;
		char_texture = TexMgr_LoadImage (NULL, -1 /*not bsp texture*/, WADFILENAME":conchars", 128, 128, SRC_INDEXED, data,
		WADFILENAME, offset, TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_NOPICMIP | TEXPREF_CONCHARS);
		}

	} while (0);

// RELOAD:  CONCHARS IS HERE
	draw_disc = Draw_PicFromWad ("disc");
	draw_backtile = Draw_PicFromWad ("backtile");

	crosshair_weapon_textures_found = 0;
	crosshair_default_texture = NULL;
	{
		int n; for (n = 0; n < MAX_CROSSHAIRS_25; n ++ ) {
			crosshair_weapon_textures[n] = NULL; // We may not have one
		}
	}

	// Crosshair
	if (gl_external_textures.value)
	{
		int mark = Hunk_LowMark ();
		const char *crosshair_base = "/gfx/crosshairs/weapon_";
		char current_filename[MAX_QPATH_64];
		char limit_path[MAX_OSPATH];
		int fwidth, fheight;
		unsigned *data;
		int n;

		//c_strlcpy (limit_path, com_filepath);
		c_strlcpy (current_filename, "/gfx/crosshairs/default");
		data = Image_Load_Limited (current_filename, &fwidth, &fheight, NULL); //limit_path);//, mod->loadinfo.searchpath);
		if (data) {
			crosshair_default_texture = TexMgr_LoadImage (NULL, -1 /*not bsp texture*/, current_filename, fwidth, fheight, SRC_RGBA, data, current_filename,
										  0, TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_NOPICMIP); //johnfitz -- TexMgr
			Hunk_FreeToLowMark (mark); // Should be no files to close, LoadImage closes them.
		}


		for (n = 0; n < MAX_CROSSHAIRS_25; n ++) {
			c_snprintf2 (current_filename, "%s%d", crosshair_base, n + 1);
			data = Image_Load_Limited (current_filename, &fwidth, &fheight, NULL); //limit_path);//, mod->loadinfo.searchpath);
			if (data) {
				crosshair_weapon_textures[n] = TexMgr_LoadImage (NULL, -1 /*not bsp texture*/, current_filename, fwidth, fheight, SRC_RGBA, data, current_filename,
											  0, TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_NOPICMIP); //johnfitz -- TexMgr
				Hunk_FreeToLowMark (mark); // Should be no files to close, LoadImage closes them.
				crosshair_weapon_textures_found ++;
			}
		}

		
	}

}

/*
===============
Draw_NewGame -- johnfitz
===============
*/
void Draw_NewGame (void)
{
	cachepic_t	*pic;
	int			i;

	// empty scrap and reallocate gltextures
	memset(&scrap_allocated, 0, sizeof(scrap_allocated));
	memset(&scrap_texels, 255, sizeof(scrap_texels));
	Scrap_Upload (); //creates 2 empty gltextures

	// reload wad pics
	W_LoadWadFile (); //johnfitz -- filename is now hard-coded for honesty
	Draw_LoadPics ();
	SCR_LoadPics ();
	Sbar_LoadPics ();

	// empty lmp cache
	for (pic = menu_cachepics, i = 0; i < menu_numcachepics; pic++, i++)
		pic->name[0] = 0;
	menu_numcachepics = 0;
}

/*
===============
Draw_Init -- johnfitz -- rewritten
===============
*/
void Draw_Init (void)
{
	TexMgr_Init (); //johnfitz

	// clear scrap and allocate gltextures
	memset(&scrap_allocated, 0, sizeof(scrap_allocated));
	memset(&scrap_texels, 255, sizeof(scrap_texels));
	Scrap_Upload (); //creates 2 empty textures

	// create internal pics
	pic_ins = Draw_MakePic ("ins", 8, 9, &pic_ins_data[0][0]);
	pic_ovr = Draw_MakePic ("ovr", 8, 8, &pic_ovr_data[0][0]);
	pic_nul = Draw_MakePic ("nul", 8, 8, &pic_nul_data[0][0]);

	// load game pics
	Draw_LoadPics ();
}

//==============================================================================
//
//  2D DRAWING
//
//==============================================================================

/*
================
Draw_CharacterQuad -- johnfitz -- seperate function to spit out verts
================
*/
void Draw_CharacterQuad (int x, int y, char num)
{
	int				row, col;
	float			frow, fcol, size;

	row = num>>4;
	col = num&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	eglTexCoord2f (fcol, frow);
	eglVertex2f (x, y);
	eglTexCoord2f (fcol + size, frow);
	eglVertex2f (x+8, y);
	eglTexCoord2f (fcol + size, frow + size);
	eglVertex2f (x+8, y+8);
	eglTexCoord2f (fcol, frow + size);
	eglVertex2f (x, y+8);
}

void Draw_GLTexture (gltexture_t *tx, float x0, float y0, float x1, float y1)
{
//	int				row, col;
//	float			frow, fcol, size;



	GL_Bind (tx);
	eglTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	eglDisable (GL_ALPHA_TEST);
	eglEnable (GL_BLEND);
	eglColor4f (1,1,1,0.5);

	eglBegin (GL_QUADS);
	eglTexCoord2f (0,0);
	eglVertex2f (x0, y0);
	eglTexCoord2f (1,0);
	eglVertex2f (x1, y0);
	eglTexCoord2f (1,1);
	eglVertex2f (x1, y1);
	eglTexCoord2f (0,1);
	eglVertex2f (x0, y1);
	eglEnd ();

	eglDisable (GL_BLEND);
	eglEnable (GL_ALPHA_TEST);
	eglTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	eglColor4f (1,1,1,1);
}


/*
================
Draw_Character -- johnfitz -- modified to call Draw_CharacterQuad
================
*/
void Draw_Character (int x, int y, int num)
{
	if (y <= -8)
		return;			// totally off screen

	num &= 255;

	if (num == 32)
		return; //don't waste verts on spaces

	GL_Bind (char_texture);
	eglBegin (GL_QUADS);

	Draw_CharacterQuad (x, y, (char) num);

	eglEnd ();
}

/*
================
Draw_String -- johnfitz -- modified to call Draw_CharacterQuad
================
*/
void Draw_String (int x, int y, const char *str)
{
	if (y <= -8)
		return;			// totally off screen

	GL_Bind (char_texture);
	eglBegin (GL_QUADS);

	while (*str)
	{
		if (*str != 32) //don't waste verts on spaces
			Draw_CharacterQuad (x, y, *str);
		str++;
		x += 8;
	}

	eglEnd ();
}

/*
================
Draw_StringEx -- Baker: Recognizes /b bronze markup
================
*/
void Draw_StringEx (int x, int y, const char *str)
{
	int col = 0;
	cbool bronze = false;

	if (y <= -8)
		return;			// totally off screen

	while (*str)
	{
		if (*str == '\b')
			bronze = !bronze; // Toggle the bronzing and don't draw
		else Draw_Character ( x + col++ * 8, y, *str | bronze * 128);
		str++;
	}
}


/*
=============
Draw_Pic -- johnfitz -- modified
=============
*/
void Draw_Pic (int x, int y, qpic_t *pic)
{
	glpic_t			*gl;

	if (scrap_dirty)
		Scrap_Upload ();
	gl = (glpic_t *)pic->data;
	GL_Bind (gl->gltexture);

	eglBegin (GL_QUADS);
	eglTexCoord2f (gl->sl, gl->tl);
	eglVertex2f (x, y);
	eglTexCoord2f (gl->sh, gl->tl);
	eglVertex2f (x+pic->width, y);
	eglTexCoord2f (gl->sh, gl->th);
	eglVertex2f (x+pic->width, y+pic->height);
	eglTexCoord2f (gl->sl, gl->th);
	eglVertex2f (x, y+pic->height);
	eglEnd ();
}

/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic (int x, int y, qpic_t *pic)
{
// Baker: This function is unnecessary for GL, but keeps the GLQuake/WinQuake
// code similar.
	Draw_Pic (x, y, pic);
}


/*
=============
Draw_TransPicTranslate -- johnfitz -- rewritten to use texmgr to do translation

Only used for the player color selection menu
=============
*/
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, int top, int bottom)
{
	static int oldtop = -2;
	static int oldbottom = -2;

	if (top != oldtop || bottom != oldbottom)
	{
		glpic_t *p = (glpic_t *)pic->data;
		gltexture_t *glt = p->gltexture;
		oldtop = top;
		oldbottom = bottom;
		TexMgr_ReloadImage (glt, top, bottom);
	}
	Draw_Pic (x, y, pic);
}

/*
================
Draw_ConsoleBackground -- johnfitz -- rewritten
================
*/
void Draw_ConsoleBackground (void)
{
	qpic_t *pic;
	float alpha;

	pic = Draw_CachePic ("gfx/conback.lmp");
	pic->width = vid.conwidth;
	pic->height = vid.conheight;

	alpha = (console1.forcedup) ? 1.0 : gl_conalpha.value;

	Draw_SetCanvas (CANVAS_CONSOLE); //in case this is called from weird places

	if (alpha > 0.0)
	{
		if (alpha < 1.0)
		{
			eglEnable (GL_BLEND);
			eglColor4f (1,1,1,alpha);
			eglDisable (GL_ALPHA_TEST);
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}

		Draw_Pic (0, 0, pic);

		if (alpha < 1.0)
		{
			eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			eglEnable (GL_ALPHA_TEST);
			eglDisable (GL_BLEND);
			eglColor4f (1,1,1,1);
		}
	}
}


/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h)
{
	glpic_t	*gl;

	gl = (glpic_t *)draw_backtile->data;

	eglColor3f (1,1,1);
	GL_Bind (gl->gltexture);
	eglBegin (GL_QUADS);
	eglTexCoord2f (x/64.0, y/64.0);
	eglVertex2f (x, y);
	eglTexCoord2f ( (x+w)/64.0, y/64.0);
	eglVertex2f (x+w, y);
	eglTexCoord2f ( (x+w)/64.0, (y+h)/64.0);
	eglVertex2f (x+w, y+h);
	eglTexCoord2f ( x/64.0, (y+h)/64.0 );
	eglVertex2f (x, y+h);
	eglEnd ();
}

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c, float alpha)
{
	byte *pal = (byte *)vid.d_8to24table; //johnfitz -- use d_8to24table instead of host_basepal

	eglDisable (GL_TEXTURE_2D);
	eglEnable (GL_BLEND); //johnfitz -- for alpha
	eglDisable (GL_ALPHA_TEST); //johnfitz -- for alpha
	eglColor4f (pal[c*4]/255.0, pal[c*4+1]/255.0, pal[c*4+2]/255.0, alpha); //johnfitz -- added alpha

	eglBegin (GL_QUADS);
	eglVertex2f (x,y);
	eglVertex2f (x+w, y);
	eglVertex2f (x+w, y+h);
	eglVertex2f (x, y+h);
	eglEnd ();

	eglColor3f (1,1,1);
	eglDisable (GL_BLEND); //johnfitz -- for alpha
	eglEnable (GL_ALPHA_TEST); //johnfitz -- for alpha
	eglEnable (GL_TEXTURE_2D);
}

/*
================
Draw_FadeScreen -- johnfitz -- revised
================
*/
void Draw_FadeScreen (void)
{
	Draw_SetCanvas (CANVAS_DEFAULT);

	eglEnable (GL_BLEND);
	eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Baker: ha!  This was "random"
	eglDisable (GL_ALPHA_TEST);
	eglDisable (GL_TEXTURE_2D);

	eglColor4f (0, 0, 0, 0.5);

	eglBegin (GL_QUADS);

	eglVertex2f (0,0);
	eglVertex2f (clwidth, 0);
	eglVertex2f (clwidth, clheight);
	eglVertex2f (0, clheight);

	eglEnd ();
	eglColor4f (1,1,1,1);
	eglEnable (GL_TEXTURE_2D);
	eglEnable (GL_ALPHA_TEST);
	eglDisable (GL_BLEND);

	Sbar_Changed();
}

/*
================
Draw_SetCanvas -- johnfitz -- support various canvas types
================
*/
void Draw_SetCanvas (canvastype newcanvas)
{
	int		lines;
	float s;
	float y, ybot;

	if (newcanvas == currentcanvas)
		return;

	currentcanvas = newcanvas;

	eglMatrixMode(GL_PROJECTION);
    eglLoadIdentity ();

//if (newcanvas == CANVAS_MENU_INTERMISSION_TEXT ) newcanvas = CANVAS_MENU;
	switch(newcanvas)
	{
	case CANVAS_DEFAULT:
		eglOrtho (0, clwidth, clheight, 0, -99999, 99999);
		eglViewport (clx, cly, clwidth, clheight);
		break;
	case CANVAS_DEFAULT_CONSCALE:
		eglOrtho (0, vid.conwidth, vid.conheight, 0, -99999, 99999);
		eglViewport (clx, cly, clwidth, clheight);
		break;
	case CANVAS_CONSOLE:
		lines = vid.conheight - (console1.visible_lines_conscale);
		eglOrtho (0, vid.conwidth, vid.conheight + lines, lines, -99999, 99999);
		eglViewport (clx, cly, clwidth, clheight);
		break;
	case CANVAS_SCOREBOARD2:
		y=0; // Fall through .. 

	case CANVAS_MENU:
		// HERE AUTO
		s = c_min ((float)clwidth / 320.0, (float)clheight / 200.0);
		//s = CLAMP (1.0, gl_menuscale.value, s);
		s = CLAMP (1.0, vid.menu_scale /*gl_menuscale.value*/, s);
		eglOrtho (0, 320, 200, 0, -99999, 99999);
		
		y = (newcanvas ==  CANVAS_SCOREBOARD2) ? 4 : cly + (clheight - 200*s) / 2;
		ybot = (newcanvas ==  CANVAS_SCOREBOARD2) ? cly + (clheight - 200*s) : cly + (clheight - 200*s) / 2;
		//eglViewport (clx + (clwidth - 320*s) / 2, ybot, 320*s, 200*s);
		eglViewport (clx + (clwidth - 320*s) / 2, ybot, 320*s, 200*s);
		break;
	case CANVAS_MENU_INTERMISSION_TEXT: // Increased size a bit.
		// HERE AUTO
		s = c_min ((float)clwidth / 320.0, (float)clheight / 200.0);
		//s = CLAMP (1.0, gl_menuscale.value, s);
		s = CLAMP (1.0, vid.menu_scale /*gl_menuscale.value*/, s);
		{
			int x_width  = 320*s, x_offset = (clwidth - x_width) / 2;
			int y_height_scaled = c_min (300, clheight / s); // Canvas size is 300 (if we can get it) or available area, whichever is lower.
			
			eglOrtho (0, 320 /* x_width /s */ , y_height_scaled, 0, -99999, 99999);
			eglViewport (clx + x_offset, cly + (clheight - y_height_scaled *s) / 2, x_width, y_height_scaled * s);
		}
		break;
	case CANVAS_SBAR:
		// HERE AUTO
		//s = CLAMP (1.0, gl_sbarscale.value, (float)clwidth / 320.0);
		s = CLAMP (1.0, vid.sbar_scale /*gl_sbarscale.value*/, (float)clwidth / 320.0);
		if (cl.gametype == GAME_DEATHMATCH && !scr_sbarcentered.value)
		{
			eglOrtho (0, clwidth / s, 48, 0, -99999, 99999);
			eglViewport (clx, cly, clwidth, 48*s);
		}
		else
		{
			eglOrtho (0, 320, 48, 0, -99999, 99999);
			eglViewport (clx + (clwidth - 320*s) / 2, cly, 320*s, 48*s);
		}
		break;
	case CANVAS_WARPIMAGE:
		eglOrtho (0, 128, 0, 128, -99999, 99999);
		eglViewport (clx, cly + clheight - gl_warpimagesize, gl_warpimagesize, gl_warpimagesize);
		break;
	case CANVAS_CROSSHAIR: //0,0 is center of viewport
		s = CLAMP (0.10, gl_crosshairscale.value, 10.0);
		eglOrtho (scr_vrect.width/-2.0f/s, scr_vrect.width/2.0f/s, scr_vrect.height/2.0f/s, scr_vrect.height/-2.0f/s, -99999, 99999);
		eglViewport (clx + scr_vrect.x, clheight - scr_vrect.y - scr_vrect.height, scr_vrect.width & ~1, scr_vrect.height & ~1);
		break;
	case CANVAS_BOTTOMLEFT: //used by devstats
		s = (float)clwidth/vid.conwidth; //use console scale
		eglOrtho (0, 320, 200, 0, -99999, 99999);
		eglViewport (clx, cly, 320 * s, 200 * s);
		break;
/* Baker: Deactivated at this time.  We aren't drawing there.
	case CANVAS_BOTTOMRIGHT: //used by fps
		s = (float)clwidth/vid.conwidth; //use console scale
		eglOrtho (0, 320, 200, 0, -99999, 99999);
		eglViewport (clx + clwidth - 320 * s, cly, 320 * s, 200 * s);
		break;
*/
	case CANVAS_TOPRIGHT: //used by disc
		s = 1;
		eglOrtho (0, 320, 200, 0, -99999, 99999);
		eglViewport (clx + clwidth-320 * s, cly + clheight-200 * s, 320 * s, 200 * s);
		break;

	case CANVAS_TOPLEFT: //used by disc
		s = 1;
		eglOrtho (0, 320, 200, 0, -99999, 99999);
		eglViewport (clx, cly + clheight-200 * s, 320 * s, 200 * s);
		break;

	default:
		System_Error ("Draw_SetCanvas: bad canvas type");
	}

	eglMatrixMode(GL_MODELVIEW);
    eglLoadIdentity ();
}

/*
================
GL_Set2D -- johnfitz -- rewritten
================
*/
void Draw_Set2D (void)
{
	currentcanvas = CANVAS_INVALID;
	Draw_SetCanvas (CANVAS_DEFAULT);

	eglDisable (GL_DEPTH_TEST);
	eglDisable (GL_CULL_FACE);
	eglDisable (GL_BLEND);
	eglEnable (GL_ALPHA_TEST);
	eglColor4f (1,1,1,1);
}

#endif // GLQUAKE specific
