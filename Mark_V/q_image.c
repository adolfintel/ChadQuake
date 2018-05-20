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
//q_image.c -- image loading

#include "quakedef.h"
#include "q_image.h"


///////////////////////////////////////////////////////////////////////////////
//  QUAKE_IMAGE: LOCATE MEDIA
///////////////////////////////////////////////////////////////////////////////

unsigned *Image_Load (const char *qpath_file_url, int *width, int *height)
{
	return Image_Load_Limited (qpath_file_url, width, height, NULL /* no media owner path */);
}

unsigned *Image_Load_Limited (const char *qpath_file_url, int *width, int *height, const char *media_owner_path)
{
	char loadfilename[MAX_OSPATH];
	FILE	*f;

	c_snprintf1 (loadfilename, "%s.tga", qpath_file_url);
	COM_FOpenFile_Limited (loadfilename, &f, media_owner_path);
	if (f) {
		unsigned *Image_Load_TGA_FileHandle (FILE *fin, int *width, int *height, malloc_fn_t Malloc_Fn, const char *description);
		return Image_Load_TGA_FileHandle (f, width, height, Hunk_Alloc, loadfilename);
	}


	c_snprintf1 (loadfilename, "%s.pcx", qpath_file_url);
	COM_FOpenFile_Limited (loadfilename, &f, media_owner_path);
	if (f) {
		unsigned *Image_Load_PCX_FileHandle (FILE *fin, int *width, int *height, size_t filelen, malloc_fn_t Malloc_Fn, const char *description);
		return Image_Load_PCX_FileHandle (f, width, height, com_filesize, Hunk_Alloc, loadfilename);
	}

	return NULL;
}

cbool Image_Save_TGA_QPath (const char *qpath_file_url, unsigned *pixels_rgba, int width, int height, cbool upsidedown)
{
	// Note: We are never actually saving TGA, but the old code *always* saved them upside down.
	char *file_url = (char *)qpath_to_url (qpath_file_url);

	File_URL_Edit_Force_Extension (file_url, ".tga", CORE_STRINGS_VA_ROTATING_BUFFER_BUFSIZ_1024);

	//Image_Save_File (file_url, pixels_rgba, width, height); <--- these functions cannot handle RGB
	//Image_Save_Auto (file_url, pixels_rgba, width, height); <--- but the BPP_32 below means that is never the case anyway.
	//																Plus remember we are never writing TGA anyway!

	//return Image_Save_TGA (file_url, pixels_rgba, width, height, BPP_32 /*bitsperpixel*/, upsidedown);
	return !!Image_Save_File (file_url, pixels_rgba, width, height); // Converts from pointer to boolean!  NULL becomes 0.
}


cbool Image_Save_PNG_QPath (const char *qpath_file_url, unsigned *pixels_rgba, int width, int height)
{
	char *file_url = (char *)qpath_to_url (qpath_file_url);
	File_URL_Edit_Force_Extension (file_url, ".png", CORE_STRINGS_VA_ROTATING_BUFFER_BUFSIZ_1024);
	//return Image_Save_PNG (file_url, pixels_rgba, width, height);
	return !!Image_Save_File (file_url, pixels_rgba, width, height);
}



// Puts result on hunk, caller manages marking hunk, etc.
// Only supports TGA
byte *Image_Load_Convert_RGBA_To_Palette (const char *name, int *width, int *height, byte palette_768[])
{
    unsigned *Image_Load_TGA_FileHandle (FILE *fin, int *width, int *height, malloc_fn_t Malloc_Fn, const char *description);
	char loadfilename[MAX_OSPATH];

	FILE		*f;
	unsigned	*data;
	byte		*out, *dst;
	unsigned	*src;
	int			fwidth, fheight;
	int			x, y;

	c_snprintf1 (loadfilename, "%s.tga", name);
	COM_FOpenFile_Limited (loadfilename, &f, NULL);

	if (!f)
		return NULL;

	data = Image_Load_TGA_FileHandle (f, &fwidth, &fheight, Hunk_Alloc, loadfilename);
	if (!data)
		return NULL;

//	Con_PrintLinef ("Converting skybox to palette ...");

	src = (unsigned *)data;
	dst = out = (byte *)Hunk_Alloc (fwidth * fheight);
	for (x = 0; x < fwidth; x ++)
		for (y = 0; y < fheight; y ++, src ++, dst++)
			*dst = Palette_Best_Color_Index_For_Pixel (*src, palette_768, 255 /* Baker: 255 renders white */);

	*width = fwidth;
	*height = fheight;
	FS_fclose(f);

	return out;
}


