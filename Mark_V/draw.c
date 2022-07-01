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

// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"


typedef struct {
	vrect_t	rect;
	int		width;
	int		height;
	byte	*ptexbytes;
	int		rowbytes;
} rectdesc_t;

static rectdesc_t	r_rectdesc;

byte		*draw_chars;				// 8*8 graphic characters
qpic_t		*draw_disc;
qpic_t		*draw_backtile;


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
	cache_user_t	cache;
} cachepic_t;

#define	MAX_CACHED_PICS		128
cachepic_t	menu_cachepics[MAX_CACHED_PICS];
int			menu_numcachepics;



#include "mark_v_lmp.h" // gfx/levels.lmp (levels_lmp) and gfx/demos.lmp (demos_lmp)

int levels_pic_size = sizeof(levels_lmp);

extern int normal_menu;
extern int normal_help;
extern int normal_backtile;

int demos_pic_size = sizeof(demos_lmp);

/*
================
Draw_PicFromWad
================
*/
qpic_t	*Draw_PicFromWad (const char *name)
{
	return W_GetLumpName (name);
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

	for (pic=menu_cachepics, i=0 ; i<menu_numcachepics ; pic++, i++)
	{
		if (!strcmp (path, pic->name))
			break;
	}

	if (i == menu_numcachepics)
	{
		if (menu_numcachepics == MAX_CACHED_PICS)
			System_Error ("menu_numcachepics == MAX_CACHED_PICS");
		menu_numcachepics++;
		c_strlcpy (pic->name, path);
	}

	dat = Cache_Check (&pic->cache);

	if (dat)
		return dat;

//
// load the pic from disk
//

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
	{
		COM_LoadCacheFile (path, &pic->cache);
		dat = (qpic_t *)pic->cache.data;
	}

	if (!dat)
		System_Error ("Draw_CachePic: failed to load %s", path);

	SwapPic (dat);
#ifdef SUPPORTS_LEVELS_MENU_HACK
	if (!strcmp (path, "gfx/sp_menu.lmp"))
	{
		if (!strstr(com_filepath, "/id1/"))
			normal_menu = 0; // No!
		else
			normal_menu = 1; // No external textures in WinQuake so determined!
	}
	else if (!strcmp (path, "gfx/help0.lmp"))
	{
		if (!strstr(com_filepath, "/id1/"))
			normal_help = 0; // No!
		else
			normal_help = 1; // No external textures in WinQuake so determined!
	}
#endif // SUPPORTS_LEVELS_MENU_HACK

	return dat;
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
	draw_chars = W_GetLumpName ("conchars");
	draw_disc = W_GetLumpName ("disc");
	draw_backtile = W_GetLumpName ("backtile");

	r_rectdesc.width = draw_backtile->width;
	r_rectdesc.height = draw_backtile->height;
	r_rectdesc.ptexbytes = draw_backtile->data;
	r_rectdesc.rowbytes = draw_backtile->width;
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
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Character (int _x, int _y, int num)
{
	int x = _x + canvas.x; // Baker: Canvas
	int y = _y + canvas.y; // Baker: Canvas

	byte			*dest;
	byte			*source;

	int				drawline;
	int				row, col;

	num &= 255;

	if (num == SPACE_CHAR_32)
		return; // Don't waste time

	if (y <= -8)
		return;			// totally off screen

#if 1 // was #ifdef PARANOID -- Baker: 4.32 we need this for autoid
	if (y > clheight - 8 || x < 0 || x > clwidth - 8)
	{
		//System_Error  ("Con_DrawCharacter: (%d, %d)", x, y);
		Con_DPrintLinef ("Con_DrawCharacter: (%d, %d)", x, y);
		return;
	}
#endif

#ifdef PARANOID // Baker: I would enable but I don't see how it could happen, haha!  Why isn't it num unsigned char anyways?
		if (num < 0 || num > 255)
			System_Error ("Con_DrawCharacter: char %d", num);
#endif

	row = num / 16; // >> 4;
	col = num & 15;
	source = draw_chars + (row << 10) + (col * 8 /*<< 3*/ ); // Don't touch this one because actually doing a bitshift here.

	if (y < 0)
	{	// clipped
		drawline = 8 + y;
		source -= 128*y;
		y = 0;
	}
	else
	{
		drawline = 8;
	}


	dest = vid.buffer + y * vid.rowbytes + x; // Formerly vid.conbuffer and/or vid.conrowbytes

	while (drawline--)
	{
		if (source[0])
			dest[0] = source[0];
		if (source[1])
			dest[1] = source[1];
		if (source[2])
			dest[2] = source[2];
		if (source[3])
			dest[3] = source[3];
		if (source[4])
			dest[4] = source[4];
		if (source[5])
			dest[5] = source[5];
		if (source[6])
			dest[6] = source[6];
		if (source[7])
			dest[7] = source[7];
		source += 128;
		dest += vid.rowbytes; // Formerly vid.conbuffer and/or vid.conrowbytes
	}

}

/*
================
Draw_String
================
*/
void Draw_String (int x, int y, const char *str)
{
	if (y <= -8)
		return;			// totally off screen

	while (*str)
	{
		if (*str != SPACE_CHAR_32) //don't waste verts on spaces
		Draw_Character (x, y, *str);
		str++;
		x += 8;
	}
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
Draw_Pic
=============
*/
void Draw_Pic (int _x, int _y, qpic_t *pic)
{
	int x = _x + canvas.x; // Baker: Canvas
	int y = _y + canvas.y; // Baker: Canvas

	byte			*dest, *source;
	int				v;

	if (x < 0 || x + pic->width > clwidth || y < 0 || y + pic->height > clheight)
		System_Error ("Draw_Pic: bad coordinates");

	source = pic->data;


	dest = vid.buffer + y * vid.rowbytes + x;

	for (v=0 ; v<pic->height ; v++)
	{
		memcpy (dest, source, pic->width);
		dest += vid.rowbytes;
		source += pic->width;
	}

}

/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic (int _x, int _y, qpic_t *pic)
{
	int x = _x + canvas.x; // Baker: Canvas
	int y = _y + canvas.y; // Baker: Canvas

	byte	*dest, *source, tbyte;
	int				v, u;

	if (x < 0 || (x + pic->width) > clwidth || y < 0 || (y + pic->height) > clheight)
		System_Error ("Draw_TransPic: bad coordinates");

	source = pic->data;


	dest = vid.buffer + y * vid.rowbytes + x;

	if (pic->width & 7)
	{	// general
		for (v=0 ; v<pic->height ; v++)
		{
			for (u=0 ; u<pic->width ; u++)
				if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
					dest[u] = tbyte;

			dest += vid.rowbytes;
			source += pic->width;
		}
	}
	else
	{	// unwound
		for (v=0 ; v<pic->height ; v++)
		{
			for (u=0 ; u<pic->width ; u+=8)
			{
				if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
					dest[u] = tbyte;
				if ( (tbyte=source[u+1]) != TRANSPARENT_COLOR)
					dest[u+1] = tbyte;
				if ( (tbyte=source[u+2]) != TRANSPARENT_COLOR)
					dest[u+2] = tbyte;
				if ( (tbyte=source[u+3]) != TRANSPARENT_COLOR)
					dest[u+3] = tbyte;
				if ( (tbyte=source[u+4]) != TRANSPARENT_COLOR)
					dest[u+4] = tbyte;
				if ( (tbyte=source[u+5]) != TRANSPARENT_COLOR)
					dest[u+5] = tbyte;
				if ( (tbyte=source[u+6]) != TRANSPARENT_COLOR)
					dest[u+6] = tbyte;
				if ( (tbyte=source[u+7]) != TRANSPARENT_COLOR)
					dest[u+7] = tbyte;
			}
			dest += vid.rowbytes;
			source += pic->width;
		}
	}

}


/*
=============
Draw_TransPicTranslate
=============
*/
void Draw_TransPicTranslate (int _x, int _y, qpic_t *pic, byte *translation)
{
	int x = _x + canvas.x; // Baker: Canvas
	int y = _y + canvas.y; // Baker: Canvas

	byte	*dest, *source, tbyte;

	int				v, u;

	if (x < 0 || (x + pic->width) > clwidth || y < 0 || (y + pic->height) > clheight)
		System_Error ("Draw_TransPic: bad coordinates");

	source = pic->data;


	dest = vid.buffer + y * vid.rowbytes + x;

	if (pic->width & 7)
	{	// general
		for (v=0 ; v<pic->height ; v++)
		{
			for (u=0 ; u<pic->width ; u++)
				if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
					dest[u] = translation[tbyte];

			dest += vid.rowbytes;
			source += pic->width;
		}
	}
	else
	{	// unwound
		for (v=0 ; v<pic->height ; v++)
		{
			for (u=0 ; u<pic->width ; u+=8)
			{
				if ( (tbyte=source[u]) != TRANSPARENT_COLOR)
					dest[u] = translation[tbyte];
				if ( (tbyte=source[u+1]) != TRANSPARENT_COLOR)
					dest[u+1] = translation[tbyte];
				if ( (tbyte=source[u+2]) != TRANSPARENT_COLOR)
					dest[u+2] = translation[tbyte];
				if ( (tbyte=source[u+3]) != TRANSPARENT_COLOR)
					dest[u+3] = translation[tbyte];
				if ( (tbyte=source[u+4]) != TRANSPARENT_COLOR)
					dest[u+4] = translation[tbyte];
				if ( (tbyte=source[u+5]) != TRANSPARENT_COLOR)
					dest[u+5] = translation[tbyte];
				if ( (tbyte=source[u+6]) != TRANSPARENT_COLOR)
					dest[u+6] = translation[tbyte];
				if ( (tbyte=source[u+7]) != TRANSPARENT_COLOR)
					dest[u+7] = translation[tbyte];
			}
			dest += vid.rowbytes;
			source += pic->width;
		}
	}

}


/*
================
Draw_ConsoleBackground -- johnfitz -- rewritten
================
*/
void Draw_ConsoleBackground (void)
{
	int lines =  console1.visible_lines_conscale;
	int				x, y, v;
	byte			*src, *dest;
	int				f, fstep;
	qpic_t			*conback = Draw_CachePic ("gfx/conback.lmp");

	Draw_SetCanvas (CANVAS_CONSOLE); //in case this is called from weird places
// draw the pic

	dest = vid.buffer; // Formerly vid.conbuffer and/or vid.conrowbytes

	for (y = 0 ; y < lines ; y++, dest += vid.rowbytes) // Formerly vid.conbuffer and/or vid.conrowbytes
	{
		v = (vid.conheight - lines + y)*200/vid.conheight;
		src = conback->data + v*320;
		if (vid.conwidth == 320)
			memcpy (dest, src, vid.conwidth);
		else
		{
			f = 0;
			fstep = 320*0x10000/vid.conwidth;
			for (x=0 ; x < vid.conwidth ; x += 4)
			{
				dest[x] = src[f>>16]; // true bitshift
				f += fstep;
				dest[x+1] = src[f>>16];  // true bitshift
				f += fstep;
				dest[x+2] = src[f>>16]; // true bitshift
				f += fstep;
				dest[x+3] = src[f>>16]; // true bitshift
				f += fstep;
			}
		}
	}

}

/*
==============
R_DrawRect8
==============
*/
void R_DrawRect8 (vrect_t *prect, int rowbytes, byte *psrc, int transparent)
{
	byte	t, *pdest;
	int		i, j, srcdelta, destdelta;

	pdest = vid.buffer + (prect->y * vid.rowbytes) + prect->x;

	srcdelta = rowbytes - prect->width;
	destdelta = vid.rowbytes - prect->width;

	if (transparent)
	{
		for (i=0 ; i<prect->height ; i++)
		{
			for (j=0 ; j<prect->width ; j++)
			{
				t = *psrc;
				if (t != TRANSPARENT_COLOR)
				{
					*pdest = t;
				}

				psrc++;
				pdest++;
			}

			psrc += srcdelta;
			pdest += destdelta;
		}
	}
	else
	{
		for (i=0 ; i<prect->height ; i++)
		{
			memcpy (pdest, psrc, prect->width);
			psrc += rowbytes;
			pdest += vid.rowbytes;
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
void Draw_TileClear (int _x, int _y, int w, int h)
{
	int x = _x + canvas.x; // Baker: Canvas
	int y = _y + canvas.y; // Baker: Canvas

	int				width, height, tileoffsetx, tileoffsety;
	byte			*psrc;
	vrect_t			vr;

	r_rectdesc.rect.x = x;
	r_rectdesc.rect.y = y;
	r_rectdesc.rect.width = w;
	r_rectdesc.rect.height = h;

	vr.y = r_rectdesc.rect.y;
	height = r_rectdesc.rect.height;

	tileoffsety = vr.y % r_rectdesc.height;

	while (height > 0)
	{
		vr.x = r_rectdesc.rect.x;
		width = r_rectdesc.rect.width;

		if (tileoffsety != 0)
			vr.height = r_rectdesc.height - tileoffsety;
		else
			vr.height = r_rectdesc.height;

		if (vr.height > height)
			vr.height = height;

		tileoffsetx = vr.x % r_rectdesc.width;

		while (width > 0)
		{
			if (tileoffsetx != 0)
				vr.width = r_rectdesc.width - tileoffsetx;
			else
				vr.width = r_rectdesc.width;

			if (vr.width > width)
				vr.width = width;

			psrc = r_rectdesc.ptexbytes + (tileoffsety * r_rectdesc.rowbytes) + tileoffsetx;


			R_DrawRect8 (&vr, r_rectdesc.rowbytes, psrc, 0);

			vr.x += vr.width;
			width -= vr.width;
			tileoffsetx = 0;	// only the left tile can be left-clipped
		}

		vr.y += vr.height;
		height -= vr.height;
		tileoffsety = 0;		// only the top tile can be top-clipped
	}
}

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int _x, int _y, int w, int h, int c, float alpha)
{
	// Clamping?
	// Baker: Alpha is ignored at this time
	int x = _x + canvas.x; // Baker: Canvas
	int y = _y + canvas.y; // Baker: Canvas
#if 0
	int x = CLAMP(0, _x + canvas.x, vid.width - 1)
	int y = CLAMP(0, _y + canvas.y, vid.height - 1)
	int w = CLAMP(0, _w, vid.width - x);  // draw at 350 w 100 vidwidth 400 so 399 is max.  w = 400-1-350=49
	int h = CLAMP(0, _h, vid.height - y);
#endif
	byte			*dest;
	int				u, v;

	dest = vid.buffer + y*vid.rowbytes + x;
	for (v = 0 ; v < h ; v++, dest += vid.rowbytes)
		for (u = 0 ; u < w ; u++)
			dest[u] = c;
}

/*
================
Draw_FadeScreen
================
*/
void Draw_FadeScreen (void)
{
	int			x,y;
	byte		*pbuf;

	Draw_SetCanvas (CANVAS_DEFAULT);
	S_ExtraUpdate ();

	for (y = 0 ; y < clheight ; y++)
	{
		int	t;

		pbuf = (byte *)(vid.buffer + vid.rowbytes * y);
		t = (y & 1) << 1;

		for (x=0 ; x < clwidth ; x++)
		{
			if ((x & 3) != t)
				pbuf[x] = 0;
		}
	}

	S_ExtraUpdate ();

#ifdef WINQUAKE_RENDERER_SUPPORT
	winquake_scr_copyeverything = 1;
#endif // WINQUAKE_RENDERER_SUPPORT

	Sbar_Changed();
}

/*
================
Draw_SetCanvas -- Baker -- support various canvas types
================
*/
void Draw_SetCanvas (canvastype newcanvas)
{
	int		lines;
//	float s;

	if (newcanvas == currentcanvas)
		return;

	currentcanvas = newcanvas;

	switch(newcanvas)
	{
	case CANVAS_DEFAULT:
		canvas.x = 0, canvas.y = 0;
		canvas.width = clwidth, canvas.height = clheight;
		canvas.scale_x = 1, canvas.scale_y = 1;
		break;
	case CANVAS_DEFAULT_CONSCALE:
		canvas.x = 0, canvas.y = 0;
		canvas.width = clwidth, canvas.height = clheight;
		canvas.scale_x = 1, canvas.scale_y = 1;
		break;
	case CANVAS_CONSOLE:
		lines = vid.conheight - (console1.visible_lines_conscale);
		canvas.x = 0, canvas.y = - lines;
		canvas.width = clwidth, canvas.height = clheight;
		canvas.scale_x = 1, canvas.scale_y = 1;
		break;
	case CANVAS_SCOREBOARD2:
		// Fall through.
	case CANVAS_MENU:
		canvas.x = (clwidth - 320) /2 /*>> 1*/ , canvas.y = (newcanvas == CANVAS_SCOREBOARD2)? 4 : (clheight - 200) / 2 /*>> 1*/;
		//if (scr_winquake.value) canvas.y = 8; // Just 1 char down.  Well ... we'll find out won't we?
		canvas.width = 320, canvas.height = 200; // Will these be ignored or what?  Do we set this to vid.conwidth or what?
		canvas.scale_x = 1, canvas.scale_y = 1;
		break;
	case CANVAS_MENU_INTERMISSION_TEXT: // Increased size a bit.
		canvas.x = (clwidth - 320) /2 /*>> 1*/ , canvas.y = (clheight - 200) / 2; // >> 1  = Divide by 2 right?
		canvas.width = 320, canvas.height = 240;
		canvas.scale_x = 1, canvas.scale_y = 1;
		break;
	case CANVAS_SBAR:
		if (cl.gametype == GAME_DEATHMATCH && !scr_sbarcentered.value)
			canvas.x = 0;
		else canvas.x = (clwidth - 320) / 2; // >> 1  = Divide by 2 right?  

		canvas.y = clheight - 48;
		canvas.width = 320, canvas.height = 48;
		canvas.scale_x = 1, canvas.scale_y = 1;
		break;
	case CANVAS_CROSSHAIR: //0,0 is center of viewport
		// Baker: This canvas is a bit different than the others
		canvas.x = scr_vrect.x + scr_vrect.width / 2;
		canvas.y = scr_vrect.y + scr_vrect.height / 2;
		canvas.width = scr_vrect.width, canvas.height = scr_vrect.height;
		canvas.scale_x = 1, canvas.scale_y = 1;
		break;
	case CANVAS_BOTTOMLEFT: //used by devstats
		canvas.x = 0, canvas.y = clheight - 200;
		canvas.width = 320, canvas.height = 200;
		canvas.scale_x = 1, canvas.scale_y = 1;
		break;
/* Baker: Deactivated at this time.  We aren't drawing there.
	case CANVAS_BOTTOMRIGHT: //used by fps
		s = (float)clwidth/vid.conwidth; //use console scale
		glOrtho (0, 320, 200, 0, -99999, 99999);
		glViewport (clx + clwidth - 320 * s, cly, 320 * s, 200 * s);
		break;
*/
	case CANVAS_TOPRIGHT: //used by disc / ram / turtle, sw uses it
		// Baker canvas
		canvas.x = clwidth - 320 , canvas.y = 0;
		canvas.width = 320, canvas.height = 200;
		canvas.scale_x = 1, canvas.scale_y = 1;
		break;

	case CANVAS_TOPLEFT: //used by devstats
		canvas.x = 0, canvas.y = 0;
		canvas.width = 320, canvas.height = 200;
		canvas.scale_x = 1, canvas.scale_y = 1;
		break;

	default:
		System_Error ("Draw_SetCanvas: bad canvas type");
	}

}

/*
================
GL_Set2D -- Baker: Emulate Fitz
================
*/
void Draw_Set2D (void)
{
	currentcanvas = CANVAS_INVALID;
	Draw_SetCanvas (CANVAS_DEFAULT);

}

#endif // !GLQUAKE - WinQuake Software renderer