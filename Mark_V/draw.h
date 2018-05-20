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
// draw.h

#ifndef __DRAW_H__
#define __DRAW_H__

// draw.h -- these are the only functions outside the refresh allowed
// to touch the vid buffer

extern	qpic_t		*draw_disc;	// also used on sbar

void Draw_Init (void);
void Draw_Character (int x, int y, int num);
void Draw_Pic (int x, int y, qpic_t *pic);
void Draw_TransPic (int x, int y, qpic_t *pic);
void Draw_Fill (int x, int y, int w, int h, int c, float alpha);
void Draw_TileClear (int x, int y, int w, int h);

void Draw_FadeScreen (void);
void Draw_String (int x, int y, const char *str);
void Draw_StringEx (int x, int y, const char *str);
void Draw_NewGame (void);

qpic_t *Draw_PicFromWad (const char *name);
qpic_t *Draw_CachePic (const char *path);

void Draw_ConsoleBackground (void); //johnfitz -- removed parameter int lines

#ifdef GLQUAKE_RENDERER_SUPPORT
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, int top, int bottom); //johnfitz -- more parameters
#endif // GLQUAKE_RENDERER_SUPPORT


#ifdef WINQUAKE_RENDERER_SUPPORT
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, byte *translation);
#endif // WINQUAKE_RENDERER_SUPPORT

//johnfitz -- stuff for 2d drawing control
typedef enum
{
	CANVAS_NONE,
	CANVAS_DEFAULT,
	CANVAS_DEFAULT_CONSCALE,	// Baker: For centerprint except intermission
	CANVAS_CONSOLE,
	CANVAS_SCOREBOARD2,			// WinQuake multiplayer scoreboard.  320 width (menu), but starts at top of screen + 4.  Uses menu scaling.
	CANVAS_MENU,
	CANVAS_MENU_INTERMISSION_TEXT,
	CANVAS_SBAR,
	CANVAS_WARPIMAGE,
	CANVAS_CROSSHAIR,
	CANVAS_BOTTOMLEFT,
	CANVAS_BOTTOMRIGHT,
	CANVAS_TOPRIGHT,
	CANVAS_TOPLEFT,
	CANVAS_INVALID = -1
} canvastype;
//johnfitz

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

extern canvas_rect_t canvas;

void Draw_Set2D (void);
void Draw_SetCanvas (canvastype newcanvas); //johnfitz


#endif // ! __DRAW_H__

