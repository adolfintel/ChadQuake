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
// image.h -- image manipulation and loading

#ifndef __IMAGE_H__
#define __IMAGE_H__


/*
** Allocations in Core SHALL be made with C allocators calloc, free, etc.
** 
** image.c is part of Core.  
** Functions may provide a method for caller to supply their own allocator.
**
*/

// This keeps things real clear!  And communicates the return type and input quite well
// Rule: indexed ---> byte *data		
// Rule: rgba/bgra--> unsigned *rgba, unsigned *bgra
// Rule: unknwon ---> void *data


#define CORE_LOCAL
#include "core.h"
#include "environment.h"

#define BPP_32 32
#define BPP_24 24

#define PALETTE_COLORS_256 256
#define PALETTE_SIZE_768 768 // RGB Palette Size
#define GAMMA_UNITS_256 256

#define RGBA_4 4 // Used to indicate a 4 x is bytes per pixel
#define RGB_3  3 // Used to indicate a 3 x is bytes per pixel 


#include <stdio.h> // fopen, etc.
#include <stdlib.h> // malloc, etc.
#include <string.h> // mem
//#include "lodepng.h"

///////////////////////////////////////////////////////////////////////////////
//  IMAGE: OPERATIONS
///////////////////////////////////////////////////////////////////////////////

// Since save is going to be popular, here is how it works.
// Caller supplies the file name.  If Image_Save_File works, it returns the
// file name written, otherwise it returns NULL.  So is kind of like a boolean.
// You can use ! to check for failure.  Reads the extension to determine format
// to write.  Note that far greater powers exist like save to memory or 
// probably write to file offset.

const char *Image_Save_File (const char *path_to_file, const unsigned *rgba, int width, int height);

unsigned *Image_Load_JPG_Memory_Alloc (const byte *jpg_data, size_t jpg_data_length, required int *width, required int *height, const char *description);
unsigned *Image_Load_PNG_Memory_Alloc (const byte *png_data, size_t png_data_length, required int *width, required int *height, const char *description);
unsigned *Image_Load_TGA_Memory_Alloc (const byte *tga_data, size_t tga_data_length, required int *width, required int *height, const char *description);

 
void *Image_Save_JPG_Memory_Alloc (const unsigned *rgba, int width, int height, required size_t *png_length, const char *description);
void *Image_Save_PNG_Memory_Alloc (const unsigned *rgba, int width, int height, required size_t *png_length, const char *description);
void *Image_Save_TGA_Memory_Alloc (const unsigned *rgba, int width, int height, required size_t *tga_length, const char *description);

// Uh excuse me?  I have no body?  Image_Save_File superceded it.
//cbool Image_Save_Auto (const char *path_to_file, const unsigned *rgba, int width, int height);

unsigned *Image_Load_Memory_Alloc (image_format_e image_format, const void *data, size_t len, required int *width, required int *height, const char *description);
unsigned *Image_Load_File_Offset_Alloc (const char *path_to_file, image_format_e image_format, size_t offset_into_file, size_t len, required int *width, required int *height);
unsigned *Image_Load_File_Alloc (const char *path_to_file, required int *width, required int *height);
unsigned *Bundle_Image_Load_Auto_Alloc (const char *path_to_file, int *width, int *height);
unsigned *Image_Base64Decode_Alloc (const char *s, required int *width, required int *height);

cbool Image_Best_Format (unsigned *rgba, int width, int height, reply double *pct_jpg, reply double *pct_png);

void *Image_Save_Memory_Alloc (image_format_e image_format, const unsigned *rgba, int width, int height, required size_t *out_length, const char *description, reply image_format_e *image_format_used);

char *Image_Base64Encode_Alloc (image_format_e image_format, const unsigned *rgba, int width, int height);

// Loads from a blob and will be a type of image (not rgba at this point)
unsigned *Image_Load_File_Offset_Alloc (const char *path_to_file, image_format_e blob_type, size_t offset_into_file, size_t len, int *width, int *height);
#if 0
unsigned *Bundle_Image_Load_Auto_Alloc (const char *path_to_file, int *width, int *height);
#endif // at this time



///////////////////////////////////////////////////////////////////////////////
//  IMAGE: OPERATIONS
///////////////////////////////////////////////////////////////////////////////

void Image_Flip_Buffer (void *buffer, int columns, int rows, int bytes_per_pixel);
void Image_Flip_RedGreen (void *rgba, size_t numbytes);
void Image_Flip_RedGreen_Alpha_255 (void *rgba, size_t numbytes);


///////////////////////////////////////////////////////////////////////////////
//  IMAGE: GAMMA
///////////////////////////////////////////////////////////////////////////////

void Image_Build_Gamma_Table (float g, float c, byte gammatable_256[]);
void Image_Apply_Gamma_To_RGBA_Pixels (unsigned *rgba, int width, int height, byte gammatable_256[]);

///////////////////////////////////////////////////////////////////////////////
//  PALETTE REPLACEMENT
///////////////////////////////////////////////////////////////////////////////

// 256 color functions
void Image_Quick_Palette_256_Free (void);
void Image_Quick_Palette_256_Alloc (int black_index);
byte Palette_Best_Color_Index_For_Pixel (unsigned mypixel, byte palette768[], int numpalcolors);
void Image_Convert_Palette_To_Palette (byte *pixels_indexed, int len, byte old_palette_768[], byte new_palette_768[] );
byte *Image_Buffer_RGBA_To_Palette_Alpha_Threshold_Alloc (unsigned *rgba, int width, int height, byte palette_768[], int alpha_threshold);

// RGB functions
extern unsigned *fastpal_rgb4;
void Image_Quick_Palette_RGBA_Free (void);
void Image_Quick_Palette_RGBA_Alloc (void);


///////////////////////////////////////////////////////////////////////////////
//  IMAGE: Query functions (has alpha, etc.)
///////////////////////////////////////////////////////////////////////////////

cbool Image_Has_Alpha_Channel (const unsigned *rgba, int width, int height);



///////////////////////////////////////////////////////////////////////////////
//  IMAGE: Resample (speed issues, reliability, quality?)
///////////////////////////////////////////////////////////////////////////////

unsigned *Image_Bilinear_Resize_Alloc (const unsigned *rgba, int w, int h, int new_width, int new_height);


///////////////////////////////////////////////////////////////////////////////
//  DRAW
///////////////////////////////////////////////////////////////////////////////

typedef enum {
	paste_type_invalid,
	paste_type_fill,
	paste_type_fill_xor,
	paste_type_normal,
	paste_type_masked,
	paste_type_masked_fill,
	paste_type_masked_anti_fill,
	MAX_PASTE_TYPES
} paste_type_e;


#define IMGDRAWPARMS \
	int pixel_bytes,													/* BOTH     */ \
	void *dst, unsigned dst_rowbytes, int dst_x, int dst_y,				/* DST		*/ \
	const void *src, unsigned src_rowbytes, int src_x, int src_y,		/* SRC		*/ \
	int paste_width, int paste_height,									/* OP SIZE	*/ \
	unsigned forecolor, unsigned backcolor, byte alpha_threshold		/* OP INFO	*/

typedef void (*Image_Draw_fn) (IMGDRAWPARMS);

extern Image_Draw_fn paste_type_fn[MAX_PASTE_TYPES];


void Image_Draw_Rect						(IMGDRAWPARMS);	// Draws a rect.
void Image_Draw_Rect_XOR					(IMGDRAWPARMS);	// Inversion
void Image_Draw_Paste						(IMGDRAWPARMS);	// Past
void Image_Draw_Paste_Alpha_Mask			(IMGDRAWPARMS);	// Paste where alpha pass
void Image_Draw_Paste_Alpha_Mask_Color		(IMGDRAWPARMS);	// Fill color where paste source alpha pass (for text)
void Image_Draw_Paste_Alpha_Mask_Color_Anti	(IMGDRAWPARMS);	// Fill backcolor where paste source alpha pass, forecolor for fail (for selection)

//void Image_Fill_Rect_XOR					(IMGDRAWPARMS);


//void Image_Paste (int dst_x, int dst_y, int src_x, int src_y, int paste_width, int paste_height, void *dst, int dst_width, int dst_height, const void *src, int src_width, int src_height, int pixel_bytes);

// Version 2:  May replace
//void Image_Paste2 (int dst_x, int dst_y, int src_x, int src_y, int paste_width, int paste_height, void *dst, int dst_width, int dst_height, unsigned dst_rowbytes, const void *src, int src_width, int src_height, unsigned src_rowbytes, int pixel_bytes);
void Image_Paste2 (void *dst, unsigned dst_rowbytes, int dst_x, int dst_y,
				   const void *src, unsigned src_rowbytes, int src_x, int src_y,
				   int paste_width, int paste_height, int pixel_bytes);


//void Image_Paste2_Alpha_Mask_RGBA (byte alpha_threshold, int dst_x, int dst_y, int src_x, int src_y, int paste_width, int paste_height,
//				   void *dst, int dst_width, int dst_height, unsigned dst_rowbytes,
//				   const void *src, int src_width, int src_height, unsigned src_rowbytes);

void Image_Paste2_Alpha_Mask_RGBA (void *dst, unsigned dst_rowbytes, int dst_x, int dst_y,
				   const void *src, unsigned src_rowbytes, int src_x, int src_y, int paste_width, int paste_height, int pixel_bytes, byte alpha_threshold);

// This version plugs in color4 where it passes.
void Image_Paste2_Alpha_Mask_Color_RGBA (void *dst, unsigned dst_rowbytes, int dst_x, int dst_y,
				   const void *src, unsigned src_rowbytes, int src_x, int src_y, int paste_width, int paste_height, int pixel_bytes, unsigned fill_color_alpha, byte alpha_threshold);


void Image_Paste2_Alpha_Mask_Anti_Rect2_RGBA (void *dst, unsigned dst_rowbytes, int dst_x, int dst_y,
				   const void *src, unsigned src_rowbytes, int src_x, int src_y, int paste_width, int paste_height,
				   int pixel_bytes, unsigned forecolor, unsigned backcolor, byte alpha_threshold);


//void *Image_Crop_Alloc (const void *pels, int width, int height, int pixel_bytes, int x, int y, int new_width, int new_height);
void *Image_Crop_Alloc (int pixel_bytes, void *src, int src_width, int src_height, unsigned src_row_bytes, int src_x, int src_y, int src_copy_width, int src_copy_height);
void *Image_Enlarge_Canvas_Alloc (const void *pels, int width, int height, int pixel_bytes, int new_width, int new_height, unsigned fillcolor, cbool centered);

//void Image_Rect_Fill (unsigned fillcolor, int x, int y, int paint_width, int paint_height, void *pels, int pix_width, int pix_height, int pixel_bytes);

//void Image_Rect_Fill2 (unsigned fillcolor, int x, int y, int paint_width, int paint_height, void *pels, int pix_width, int pix_height, unsigned pix_rowbytes, int pixel_bytes);

void Image_Rect_Fill3 (void *pels, unsigned rowbytes, int x, int y, int paint_width, int paint_height, int pixel_bytes, unsigned fillcolor);

unsigned Image_Point_Get_Color (const void *pels, unsigned rowbytes, int pixel_bytes, int x, int y);
unsigned Image_Point_Set_Color (const void *pels, unsigned rowbytes, int pixel_bytes, int x, int y, unsigned fillcolor);




///////////////////////////////////////////////////////////////////////////////
//  IMAGE: Create / Free
///////////////////////////////////////////////////////////////////////////////

void *Image_Create_Alloc (int width, int height, int pixel_bytes, reply size_t *bufsize, unsigned fillcolor);
void *Image_Free (void * buf);

// No, use Clipboard_ methods
//unsigned *Image_Clipboard_Load_Alloc (int *outwidth, int *outheight);
//cbool Image_Clipboard_Copy (const unsigned *rgba, int width, int height);

//void Image_Triangle_Fill_Limits (void *pels, int width, int height, unsigned rowbytes, int pixel_bytes, int lx, int ly, int lw, int lh, int x1, int y1, int x2, int y2, int x3, int y3, unsigned fill_color);
//void Image_Triangle_Fan_Fill_Limits (void *pels, int width, int height, unsigned rowbytes, int pixel_bytes, int lx, int ly, int lw, int lh, int num_points, int *points, unsigned fill_color);
void Image_Triangle_Fan_Fill_Limits (void *pels, int width, int height, unsigned rowbytes, int pixel_bytes, int lx, int ly, int lw, int lh, int num_points, double *dpoints, unsigned fill_color);
void Image_Triangle_Fill_Limits (void *pels, int width, int height, unsigned rowbytes, int pixel_bytes, int lx, int ly, int lw, int lh, double x1, double y1, double x2, double y2, double x3, double y3, unsigned fill_color);
void Image_Regular_Polygon_Fill_Limits (void *pels, int width, int height, unsigned rowbytes, int pixel_bytes, int lx, int ly, int lw, int lh, int num_sides, double center_x, double center_y, double radius, unsigned fill_color);


unsigned *Image_MipMapH (unsigned *rgba, int width, int height);
unsigned *Image_MipMapW (unsigned *rgba, int width, int height);
void *Image_Copy_Alloc (void *pels, int width, int height, int pixelbytes);
image_format_e Image_Format_From_Data (const void *data, size_t data_length);


///////////////////////////////////////////////////////////////////////////////
//  IMAGE: Color Functions
///////////////////////////////////////////////////////////////////////////////

//unsigned HTML_Color_RGBA (const char *s); // AliceBlue or #ff0000 to rgba
//const char* HTML_Color_From_RGBA (unsigned rgba); // rgba to color string (loses alpha)


///////////////////////////////////////////////////////////////////////////////
//  IMAGE: MISCELLANEOUS
///////////////////////////////////////////////////////////////////////////////

int Image_Power_Of_Two_Size (int myInteger);


#endif	// ! __IMAGE_H__


