/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 Baker

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
//q_image.h -- image loading

#ifndef __Q_IMAGE_H__
#define __Q_IMAGE_H__

///////////////////////////////////////////////////////////////////////////////
//  QUAKE_IMAGE: LOCATE MEDIA
///////////////////////////////////////////////////////////////////////////////

unsigned *Image_Load (const char *qpath_file_url, int *width, int *height);
unsigned *Image_Load_Limited (const char *qpath_file_url, int *width, int *height, const char *media_owner_path);
cbool Image_Save_TGA_QPath (const char *qpath_file_url, unsigned *pixels_rgba, int width, int height, cbool upsidedown);
cbool Image_Save_PNG_QPath (const char *qpath_file_url, unsigned *pixels_rgba, int width, int height);

// Baker: used for skybox loading in WinQuake
byte *Image_Load_Convert_RGBA_To_Palette (const char *name, int *width, int *height, byte palette_rgb_256[]);


#endif // ! __Q_IMAGE_H__


