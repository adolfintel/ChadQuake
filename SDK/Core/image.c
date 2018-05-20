/*
Copyright (C) 2012-2014 Baker

*/
// image.c -- image manipulation and loading

#define CORE_LOCAL

#include "core.h"
#include "image.h"

#ifdef IMAGE_JPEG_SUPPORT
#include "jpeglib.h"
#endif // IMAGE_JPEG_SUPPORT

#include "lodepng.h"

#if 1 // TARGA
	#define TGA_UNCOMPRESSED_2	2
	#define TGA_RLE_10			10

	typedef struct _targaheader_t_s
	{
		unsigned char 	id_length, colormap_type, image_type;
		unsigned short	colormap_index, colormap_length;
		unsigned char	colormap_size;
		unsigned short	x_origin, y_origin, width, height;
		unsigned char	pixel_size, attributes;
	} targaheader_t;

	#define TARGAHEADERSIZE_18 18 // size on disk - (not size of struct, which can include padding)

	targaheader_t targa_header;
#endif // TARGA

static void *image_load_fn_for_enum (image_format_e image_format)
{
	switch (image_format) {
	case image_format_jpg: 		return (void *)Image_Load_JPG_Memory_Alloc;
	case image_format_png: 		return (void *)Image_Load_PNG_Memory_Alloc;
	case image_format_tga: 		return (void *)Image_Load_TGA_Memory_Alloc;
	default:					break;
	}
	log_debug ("image_format unknown");
	return NULL;
}

static void *image_save_fn_for_enum (image_format_e image_format)
{
	switch (image_format) {
	case image_format_jpg: 		return (void *)Image_Save_JPG_Memory_Alloc;
	case image_format_png: 		return (void *)Image_Save_PNG_Memory_Alloc;
	case image_format_tga: 		return (void *)Image_Save_TGA_Memory_Alloc;
	default:					break;
	}
	log_debug ("image_format unknown");
	return NULL;
}

static const char *image_type_string_for_enum (image_format_e image_format)
{
	switch (image_format) {
	case image_format_jpg: 		return "jpg";
	case image_format_png: 		return "png";
	case image_format_tga: 		return "tga";
	default:					break;
	}
	log_debug ("image_format unknown");
	return "UNKNOWN type";
}

image_format_e File_URL_Image_Format (const char *path_to_file)
{
	const char *extension = File_URL_GetExtension (path_to_file);

	if (!strcasecmp (".jpg", extension)) return image_format_jpg;
	if (!strcasecmp (".jpeg", extension)) return image_format_jpg;
	if (!strcasecmp (".png", extension)) return image_format_png;
	if (!strcasecmp (".tga", extension)) return image_format_tga;
	if (!strcasecmp (".$best$", extension)) return image_format_best; // SPECIAL
//	if (!strcasecmp (".pcx", extension)) return image_format_pcx;
	log_debug ("image_format unknown");
	return image_format_invalid_0;
}

//#define is_in2(x, y1, y2) (((x) == (y1)) || ((x) == (y2)))
image_format_e Image_Format_From_Data (const void *data, size_t data_length)
{
	const byte *charz = data;
	if (data_length > 1 + 3 - 1 && !memcmp (&charz[1], "PNG",  3)) return image_format_png;
	if (data_length > 6 + 4 - 1 && !memcmp (&charz[6], "JFIF", 4)) return image_format_jpg;

	// TGA.  More tricksie.  We can only guess, but it will be an accurate guess.
	if (data_length > TARGAHEADERSIZE_18) do {
		int w = charz[13] * 256 + charz[12];
		int h = charz[15] * 256 + charz[14];
		int bits_per_pixel = charz[16];
		size_t expected_size = w * h * (bits_per_pixel / 8) + TARGAHEADERSIZE_18;
		int tga_type = charz[2];

		if (!isin2(tga_type, TGA_UNCOMPRESSED_2, TGA_RLE_10))	break;	// We can only load type 2 and type 10.
		if (!isin2(bits_per_pixel, BPP_24, BPP_32))				break;	// We can only load 24 or 32 bits per pixel TGA.
		if (data_length != expected_size)						break;	// Wrong size.  Give it tolerance.  Why?  Rogue padding?
		return image_format_tga;
	} while (0); // Evile breakable if statement

	return image_format_invalid_0; // don't know for sure
}


///////////////////////////////////////////////////////////////////////////////
//  STANDARDIZED FORMAT READERS
///////////////////////////////////////////////////////////////////////////////
typedef unsigned *(load_fn_t) (const byte *, size_t, int *, int *, const char *);

unsigned *Image_Load_TGA_Memory_Alloc (const byte *tga_data, size_t tga_data_length, required int *width, required int *height, const char *description)
{
//	const char 		*fn_name = "Image_Load_TGA_Memory_Alloc";
	targaheader_t	targa_header = {0};

	MILE 			*m = mopen (tga_data, tga_data_length);

	int				columns, rows, numPixels;
	byte			*pixbuf;
	int				row, column;
	byte			*targa_rgba;
	int				realrow; //johnfitz -- fix for upside-down targas
	cbool			upside_down; //johnfitz -- fix for upside-down targas

	if (!m) // This is nigh impossible.
		return log_debug (SPRINTSFUNC "%s couldn't open data pointer", __func__, description), NULL; // Tricksie returns NULL because , NULL!

	targa_header.id_length 			= mgetc(m);
	targa_header.colormap_type 		= mgetc(m);
	targa_header.image_type 		= mgetc(m);

	targa_header.colormap_index 	= mgetLittleShort(m);
	targa_header.colormap_length 	= mgetLittleShort(m);
	targa_header.colormap_size 		= mgetc(m);
	targa_header.x_origin 			= mgetLittleShort(m);
	targa_header.y_origin 			= mgetLittleShort(m);
	targa_header.width 				= mgetLittleShort(m);
	targa_header.height 			= mgetLittleShort(m);
	targa_header.pixel_size 		= mgetc(m);
	targa_header.attributes 		= mgetc(m);

	if (targa_header.image_type != TGA_UNCOMPRESSED_2 && targa_header.image_type != TGA_RLE_10)
		return log_debug (SPRINTSFUNC "%s is not a type 2 or type 10 targa", __func__, description), NULL; // Tricksie returns NULL because , NULL!

	if (targa_header.colormap_type != 0 || (targa_header.pixel_size != BPP_32 && targa_header.pixel_size != BPP_24))
		return log_debug (SPRINTSFUNC "%s is not a 24bit or 32bit targa", __func__, description), NULL; // Tricksie returns NULL because , NULL!

	columns 						= targa_header.width;
	rows 							= targa_header.height;
	numPixels 						= columns * rows;
	upside_down 					= !(targa_header.attributes & 0x20); //johnfitz -- fix for upside-down targas

	targa_rgba 						= (byte *)malloc (numPixels * RGBA_4);

	if (targa_header.id_length != 0)
		mseek (m, targa_header.id_length, SEEK_CUR);  // skip TARGA image comment

	if (targa_header.image_type == TGA_UNCOMPRESSED_2) // Uncompressed, RGB images
	{
		for (row = rows - 1; row >= 0; row --) {
			realrow = upside_down ? row : rows - 1 - row;  //johnfitz -- fix for upside-down targas
			pixbuf 	= targa_rgba + realrow * columns * RGBA_4;

			for (column = 0; column < columns; column++ ) {
				byte red, green, blue, alphabyte;
				switch (targa_header.pixel_size) {
				case 24:
					// NOTE: read and write are not same sequence, so that's why.
					blue = mgetc (m), green = mgetc (m), red = mgetc (m);
					*pixbuf++ = red, *pixbuf++ = green, *pixbuf++ = blue, *pixbuf++ = 255;
					break;

				case 32:
					blue = mgetc (m), green = mgetc (m), red = mgetc (m), alphabyte = mgetc (m);
					*pixbuf++ = red, *pixbuf++ = green, *pixbuf++ = blue, *pixbuf++ = alphabyte;
				}
			} // End column
		} // End row
	} // End type 2
	else if (targa_header.image_type == TGA_RLE_10) // Runlength encoded RGB images
	{
		byte red, green, blue, alphabyte, packetHeader, packetSize, j;
		for (row = rows - 1; row >= 0; row--) {
			realrow = upside_down ? row : rows - 1 - row;  //johnfitz -- fix for upside-down targas
			pixbuf = targa_rgba + realrow * columns * RGBA_4;
			for (column = 0; column < columns; /* nada*/) {
				packetHeader = mgetc(m);
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80) // run-length packet
				{
					switch (targa_header.pixel_size)
					{
					case BPP_24:
						blue = mgetc (m), green = mgetc (m), red = mgetc (m), alphabyte = 255;
						break;

					case BPP_32:
						blue = mgetc (m), green = mgetc (m), red = mgetc (m), alphabyte = mgetc (m);
						break;

					default: /* avoid compiler warnings */
						blue = red = green = alphabyte = 0;
					}

					for (j = 0; j < packetSize; j++) {
						*pixbuf++ = red, *pixbuf++ = green, *pixbuf++ = blue, *pixbuf++ = alphabyte;
						column++;
						if (column == columns) // run spans across rows
						{
							column = 0;
							if (row > 0)
								row--;
							else
								goto breakOut;

							realrow = upside_down ? row : rows - 1 - row;  //johnfitz -- fix for upside-down targas
							pixbuf = targa_rgba + realrow * columns * RGBA_4;
						}
					} // For j
				} // End run-length packet
				else // non run-length packet
				{
					for (j = 0; j <packetSize; j++) {
						switch (targa_header.pixel_size) {
						case 24:
							blue = mgetc (m), green = mgetc (m), red = mgetc (m);
							*pixbuf++ = red, *pixbuf++ = green, *pixbuf++ = blue, *pixbuf++ = 255;
							break;

						case 32:
							blue = mgetc (m), green = mgetc (m), red = mgetc (m), alphabyte = mgetc (m);
							*pixbuf++ = red, *pixbuf++ = green, *pixbuf++ = blue, *pixbuf++ = alphabyte;
							break;

						default: /* avoid compiler warnings */
							blue = red = green = alphabyte = 0;
						}

						column++;
						if (column == columns) // pixel packet run spans across rows
						{
							column = 0;
							if (row > 0)
								row --;
							else
								goto breakOut;

							realrow = upside_down ? row : rows - 1 - row; //johnfitz -- fix for upside-down targas
							pixbuf = targa_rgba + realrow * columns * RGBA_4;
						}
					} // End for
				}  // End non-RLE packet
			} // End for column
			breakOut:;
		} // End for row
	} // End run-length encoded

	mclose (m);

	*width 		= (int)targa_header.width;
	*height 	= (int)targa_header.height;
	return (unsigned *)targa_rgba;
}


unsigned *Image_Load_JPG_Memory_Alloc (const byte *jpg_data, size_t jpg_data_length, required int *width, required int *height, const char *description)
{
	byte *data = NULL;

#ifdef IMAGE_JPEG_SUPPORT
	void jpeg_mem_src (j_decompress_ptr cinfo, void* buffer, size_t nbytes);

	int	i, row_stride;
	byte *scanline, *p;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

#ifdef _DEBUG
	size_t checko = sizeof(struct jpeg_decompress_struct);
#endif

	cinfo.err = jpeg_std_error (&jerr);
	jpeg_create_decompress (&cinfo);
	jpeg_mem_src (&cinfo, (void *)jpg_data, jpg_data_length);
	jpeg_read_header (&cinfo, true);
	jpeg_start_decompress (&cinfo);

	data = malloc (cinfo.image_width * cinfo.image_height * 4);
	row_stride = cinfo.output_width * cinfo.output_components;
	scanline = malloc (row_stride);

	p = data;
	while (cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines (&cinfo, &scanline, 1);

		// convert the image to RGBA
		switch (cinfo.output_components)
		{
		// RGB images
		case 3:
			for (i=0 ; i<row_stride ; )
			{
				*p++ = scanline[i++];
				*p++ = scanline[i++];
				*p++ = scanline[i++];
				*p++ = 255;
			}
			break;

		// greyscale images (default to it, just in case)
		case 1:
		default:
			for (i=0 ; i<row_stride ; i++)
			{
				*p++ = scanline[i];
				*p++ = scanline[i];
				*p++ = scanline[i];
				*p++ = 255;
			}
			break;
		}
	}

	*width = cinfo.image_width;
	*height = cinfo.image_height;

	jpeg_finish_decompress (&cinfo);
	jpeg_destroy_decompress (&cinfo);
	free (scanline);

#endif // IMAGE_JPEG_SUPPORT
	return (unsigned *)data;
}

// Decode from memory
unsigned *Image_Load_PNG_Memory_Alloc (const byte *png_data, size_t png_data_length, required int *width, required int *height, const char *description)
{
	byte *image = NULL;
	unsigned uwidth, uheight, error;

	error = lodepng_decode32(&image, &uwidth, &uheight, png_data, png_data_length);
	if(error) { log_fatal("error %u: %s", error, lodepng_error_text(error)); return NULL; }

	*width = (int)uwidth; // Width and height for an image function, the return must be required.  Otherwise, how would you use the image?
	*height = (int)uheight;
	return (unsigned *)image;
}


///////////////////////////////////////////////////////////////////////////////
//  STANDARDIZED FORMAT WRITERS
///////////////////////////////////////////////////////////////////////////////

typedef byte *(save_fn_t) (const unsigned *, int, int, size_t *, const char *);


void *Image_Save_JPG_Memory_Alloc (const unsigned *rgba, int width, int height, required size_t *jpeg_length, const char *description)
{
	size_t mem_length = 0; byte *mem = NULL;

#ifdef IMAGE_JPEG_SUPPORT
	void jpeg_mem_dest (j_compress_ptr cinfo, void **buffer, size_t *nbytes);

	byte	*scanline;

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	byte *rgb = NULL;
	{
		int pixels = width * height;
		int n;
		byte *src = (byte *)rgba;
		rgb = calloc (pixels, 3);
		for (n = 0; n < pixels; n ++) {
			rgb[n * 3 + 0] = src[n * 4 + 0];
			rgb[n * 3 + 1] = src[n * 4 + 1];
			rgb[n * 3 + 2] = src[n * 4 + 2];
		}
	}


	cinfo.err = jpeg_std_error (&jerr);
	jpeg_create_compress (&cinfo);
	jpeg_mem_dest (&cinfo, (void **) &mem, &mem_length);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults (&cinfo);
	jpeg_set_quality (&cinfo, 85, true);
	jpeg_start_compress (&cinfo, true);

	while ((int)cinfo.next_scanline < height)
	{
		scanline = &rgb[cinfo.next_scanline * width * RGB_3];
		jpeg_write_scanlines (&cinfo, &scanline, 1);
	}

	jpeg_finish_compress (&cinfo);
	jpeg_destroy_compress (&cinfo);

	*jpeg_length = mem_length;
#endif // IMAGE_JPEG_SUPPORT
	return mem;
}

void *Image_Save_PNG_Memory_Alloc (const unsigned *rgba, int width, int height, required size_t *png_length, const char *description)
{
	const char *fn_name = "Image_Save_PNG_Memory_Alloc";
	byte *png_out = NULL;

	unsigned error = lodepng_encode32 (&png_out, png_length, (byte *)rgba, width, height);

	if (error)
		return log_debug ("%s: Error saving image to memory '%s'", fn_name, description), NULL; // Tricksie null return

	return png_out;
}


void *Image_Save_TGA_Memory_Alloc (const unsigned *rgba, int width, int height, required size_t *tga_length, const char *description)
{
	const char *fn_name = "Image_Save_TGA_Memory_Alloc";
	byte *tga_out = NULL;

	const int rgba_length = width * height * RGBA_4;

	int		n;
	byte	header[TARGAHEADERSIZE_18] = {0};
	byte	*data = (byte *)rgba;
	MILE	*m = mopen (NULL, 0); // Dest open

	header[2]	= 2;				// uncompressed type
	header[12]	= width & 255;		// Write littleShort
	header[13]	= width >> 8;
	header[14]	= height & 255;		// Write littleShort
	header[15]	= height >> 8;
	header[16]	= RGBA_4 * 8;		// pixel size
	header[17]  = 0x20;				// upside-down attribute (TGA is like row ascending like OpenGL)

	mwrite (&header, 1, TARGAHEADERSIZE_18, m);

	// swap red and blue bytes.  Uh as written it would screw with the data!
	for (n = 0; n < rgba_length; n += RGBA_4) {
		mputc (data[n + 2], m); // BGRA
		mputc (data[n + 1], m);
		mputc (data[n + 0], m);
		mputc (data[n + 3], m);
	}

	mclose_transfer (m, &tga_out, tga_length);

	// Concerns.  RGBA.  What if width isn't multiple of 4?  Fucked TGA?
	return tga_out;
}

///////////////////////////////////////////////////////////////////////////////
//  STANDARDIZED AUTO FORMAT READERS, WRITERS
///////////////////////////////////////////////////////////////////////////////


// Loads from a blob
unsigned *Image_Load_Memory_Alloc (image_format_e image_format, const void *data, size_t len, required int *width, required int *height, const char *description)
{
	load_fn_t *load_fn = image_load_fn_for_enum(image_format);

	return load_fn && data && len > 0 ? load_fn(data, len, width, height, description) : NULL;
}



unsigned *Image_Load_File_Offset_Alloc (const char *path_to_file, image_format_e image_format, size_t offset_into_file, size_t len, required int *width, required int *height)
{
	byte *mem = NULL;
	if (len > 0 && File_Exists (path_to_file) && (mem = File_To_Memory_Offset_Alloc (path_to_file, NULL, offset_into_file, len)) != NULL) {
		unsigned *rgba = Image_Load_Memory_Alloc (image_format, mem, len, width, height, path_to_file);
		freenull (mem);
		return rgba;
	}

	log_debug ("File not found or couldn't load data: %s", path_to_file);
	return NULL;
}

#if 0
unsigned *Bundle_Image_Load_Auto_Alloc (const char *path_to_file, int *width, int *height)
{
	image_format_e image_format = File_URL_Image_Format (path_to_file);
	size_t mem_length; const void *mem = Bundle_File_Memory_Pointer (path_to_file, &mem_length);
	if (mem) // No, we do NOT free a bundle memory pointer.
		return Image_Load_Memory_Alloc (image_format, mem, mem_length, width, height, path_to_file);

	log_debug ("Bundle file not found: '%s'", path_to_file);
	return NULL;
}
#endif


unsigned *Image_Load_File_Alloc (const char *path_to_file, required int *width, required int *height)
{
	image_format_e image_format = File_URL_Image_Format (path_to_file);
	size_t len = File_Length (path_to_file);
	return Image_Load_File_Offset_Alloc (path_to_file, image_format, 0, len, width, height);
}



//	unsigned *Image_Base64Decode_Alloc (const char *s, image_format_e image_format, required int *width, required int *height)
//	{
//		size_t data_length = 0;
//		byte *data = base64_decode_a (s, &data_length);
//		if (data) {
//			unsigned *rgba = Image_Load_Memory_Alloc (image_format, data, data_length, width, height, "base64 decode");
//			freenull (data);
//			return rgba;
//		}
//		log_debug ("Couldn't decode base64 string");
//		return NULL;
//	}

unsigned *Image_Base64Decode_Alloc (const char *s, required int *width, required int *height)
{
	size_t data_length = 0;
	byte *data = base64_decode_a (s, &data_length);
	if (data) {
		image_format_e image_format = Image_Format_From_Data (data, data_length);
		//alert ("Auto detect data type is %s", image_type_string_for_enum (image_format));
		if (image_format > image_format_invalid_0) {
			unsigned *rgba = Image_Load_Memory_Alloc (image_format, data, data_length, width, height, "base64 decode");
			freenull (data);
			return rgba;
		}

		freenull (data);
		log_debug ("Couldn't determine base 64 file format");
		return NULL;
	}
	log_debug ("Couldn't decode base64 string");
	return NULL;
}


void *Image_Save_Memory_Alloc (image_format_e image_format, const unsigned *rgba, int width, int height, required size_t *out_length, const char *description, reply image_format_e *image_format_used)
{
	size_t data_length = 0; byte *data = NULL;
	if (rgba) {
		// SPECIAL
		if (image_format == image_format_best) while (1) /* evile breakable if*/ {
			size_t data_jpg_length = 0; byte *data_jpg = Image_Save_JPG_Memory_Alloc (rgba, width, height, &data_jpg_length, description);
			size_t data_png_length = 0; byte *data_png = Image_Save_PNG_Memory_Alloc (rgba, width, height, &data_png_length, description);
			size_t total = width * height * RGBA_4;
			// Usually if a jpg is less than 10% size, it is going to beat PNG.
			// However, this runs into a potential fine-tuning problem.
			// (i.e. in what situations does our arbitrary number fail us)

			double jpg_pct = data_jpg ? (double) data_jpg_length / total : 2.0;
			double png_pct = data_png ? (double) data_png_length / total : 2.0;

			if (!data_jpg)	log_debug ("Could encode data to jpg");
			if (!data_png) 	log_debug ("Could encode data to png");

			if (data_jpg && (!data_png || data_jpg_length < data_png_length) ) {
				freenull (data_png);
				data_length = data_jpg_length, data = data_jpg;
				NOT_MISSING_ASSIGN (image_format_used, image_format_jpg);
				break;
			}

			if (!data_png)
				break; // Extra weird situation

			freenull (data_jpg);
			data_length = data_png_length, data = data_png;
			NOT_MISSING_ASSIGN (image_format_used, image_format_png);
			break; // Remember we are in a while 1.
		}
		else
		{
			// Clearly specified format
			save_fn_t *save_fn = image_save_fn_for_enum(image_format);
			data = save_fn ? save_fn (rgba, width, height, &data_length, "save auto") : NULL;
		}
	}
	else log_debug ("no rgba data");

	*out_length = data_length;
	return data;
}

//	image_format_e Image_Best_Format (unsigned *rgba, int width, int height, reply double *pct_jpg, reply double *pct_png)
//	{
//		if (rgba) {
//			size_t total = width * height * RGBA_4;
//			size_t data_jpg_length = 0; byte *data_jpg = Image_Save_Memory_Alloc (image_format_jpg, rgba, width, height, &data_length, path_to_file);
//			size_t data_png_length = 0; byte *data_png = Image_Save_Memory_Alloc (image_format_png, rgba, width, height, &data_length, path_to_file);
//			double jpg_pct = data_jpg ? (double) data_jpg_length / total : 2.0;
//			double jpg_png = data_png ? (double) data_jpg_length / total : 2.0;
//
//			if (!data_jpg)	log_debug ("Could encode data to jpg");
//			if (!data_png) 	log_debug ("Could encode data to png");
//
//			freenull (data_jpg);
//			freenull (data_png);
//
//			if (jpg_pct == 2 && jpg_png == 2)
//				return image_format_invalid_0; // Throw hands up in the air.
//
//			NOT_MISSING_ASSIGN (pct_jpg, jpg_pct);
//			NOT_MISSING_ASSIGN (pct_png, png_pct);
//			return pct_jpg < pct_png ? image_format_png : image_format_jpg;
//		}
//
//		log_debug ("Passed NULL data");
//		return image_format_invalid_0;
//	}
//

const char *Image_Save_File (const char *path_to_file, const unsigned *rgba, int width, int height)
{
	image_format_e image_format = File_URL_Image_Format (path_to_file);
	if (rgba) {
		image_format_e image_format_out = 0;
		size_t data_length = 0; byte *data = Image_Save_Memory_Alloc (image_format, rgba, width, height, &data_length, path_to_file, &image_format_out);
		if (data) {
			cbool ret;
			static char filename[MAX_OSPATH];
			if (image_format == image_format_best) {
				// We got a reply format, play with the filename
				const char *extension = image_format_out == image_format_jpg ? ".jpg" : ".png";
				c_strlcpy (filename, path_to_file);
				File_URL_Edit_Change_Extension (filename, extension, sizeof(filename));
				path_to_file = filename; // Tricksie rewire
			}
			ret = File_Memory_To_File (path_to_file, data, data_length); // Write to disk
			freenull (data);
			if (ret)
				return path_to_file;
			return NULL;
		}
	}
	else log_debug ("no rgba data");

	return NULL;
}

// no err handling
char *Image_Base64Encode_Alloc (image_format_e image_format, const unsigned *rgba, int width, int height)
{
	size_t data_length = 0; byte *data = Image_Save_Memory_Alloc (image_format, rgba, width, height, &data_length, "base64encode", NULL);

	if (data) {
		char *base64_string = base64_encode_a (data, data_length, NULL);
		freenull (data);
		return base64_string;
	}

	log_debug ("Image base64 save error");
	return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//  STANDARDIZED INTERFACES
///////////////////////////////////////////////////////////////////////////////

Image_Draw_fn paste_type_fn[MAX_PASTE_TYPES] = {
	NULL,
	Image_Draw_Rect,
	Image_Draw_Rect_XOR,
	Image_Draw_Paste,
	Image_Draw_Paste_Alpha_Mask,
	Image_Draw_Paste_Alpha_Mask_Color,
	Image_Draw_Paste_Alpha_Mask_Color_Anti,
};

// Draws a rect.
void Image_Draw_Rect						(IMGDRAWPARMS)
{
	Image_Rect_Fill3 (dst, dst_rowbytes, dst_x, dst_y, paste_width, paste_height, pixel_bytes, forecolor);

}



// Inversion
//void Image_Draw_Rect_XOR					(IMGDRAWPARMS)

// Paste solid unconditionally
void Image_Draw_Paste						(IMGDRAWPARMS)
{
	Image_Paste2 (dst, dst_rowbytes, dst_x, dst_y, src, src_rowbytes, src_x, src_y, paste_width, paste_height, pixel_bytes);

}

// Paste where source alpha pass (like text)
void Image_Draw_Paste_Alpha_Mask			(IMGDRAWPARMS)
{
	Image_Paste2_Alpha_Mask_RGBA (dst, dst_rowbytes, dst_x, dst_y, src, src_rowbytes, src_x, src_y, paste_width, paste_height, pixel_bytes, alpha_threshold);
}

// Fill color where paste source alpha pass (for text)
void Image_Draw_Paste_Alpha_Mask_Color		(IMGDRAWPARMS)
{
	Image_Paste2_Alpha_Mask_Color_RGBA (dst, dst_rowbytes, dst_x, dst_y, src, src_rowbytes, src_x, src_y, paste_width, paste_height, pixel_bytes, forecolor, alpha_threshold);

}

// Fill backcolor where paste source alpha pass, forecolor for fail (for selection)
void Image_Draw_Paste_Alpha_Mask_Color_Anti	(IMGDRAWPARMS)
{
	Image_Paste2_Alpha_Mask_Anti_Rect2_RGBA (dst, dst_rowbytes, dst_x, dst_y, src, src_rowbytes, src_x, src_y, paste_width, paste_height, pixel_bytes, forecolor, backcolor, alpha_threshold);

}



///////////////////////////////////////////////////////////////////////////////
//  IMAGE: OPERATIONS
///////////////////////////////////////////////////////////////////////////////

void Image_Flip_RedGreen (void *rgba, size_t numbytes)
{
	byte	*byte_rep = (byte *)rgba;
	byte	temp;

	// RGBA to BGRA so clipboard will take it
	size_t i; for (i = 0 ; i < numbytes ; i += RGBA_4)
	{
		temp = byte_rep[i + 0];

		byte_rep[i + 0] = byte_rep[i + 2];
		byte_rep[i + 2] = temp;
	}
}

void Image_Flip_RedGreen_Alpha_255 (void *rgba, size_t numbytes)
{
	byte	*byte_rep = (byte *)rgba;
	byte	temp;

	// RGBA to BGRA so clipboard will take it
	size_t i; for (i = 0 ; i < numbytes ; i += RGBA_4)
	{
		temp = byte_rep[i + 0];

		byte_rep[i + 0] = byte_rep[i + 2];
		byte_rep[i + 2] = temp;
		byte_rep[i + 3] = 255;
	}
}


void Image_Flip_Buffer (void *pels, int columns, int rows, int bytes_per_pixel)
{
	byte	*buffer = (byte *)pels;
	int		bufsize = columns * bytes_per_pixel; // bufsize = widthBytes;
	byte*	tb1 = malloc (bufsize); // Flip buffer
	byte*	tb2 = malloc (bufsize); // Flip buffer2
	int		i, offset1, offset2;

	for (i = 0; i < (rows + 1) / 2; i ++)
	{
		offset1 = i * bufsize;
		offset2 = ((rows - 1) - i) * bufsize;

		memcpy(tb1,				buffer + offset1, bufsize);
		memcpy(tb2,				buffer + offset2, bufsize);
		memcpy(buffer+offset1,	tb2,			  bufsize);
		memcpy(buffer+offset2,	tb1,			  bufsize);
	}

	free (tb1);
	free (tb2);
	return;
}

///////////////////////////////////////////////////////////////////////////////
//  IMAGE: MISCELLANEOUS
///////////////////////////////////////////////////////////////////////////////

// Endangered?
int Image_Power_Of_Two_Size (int n)
{
	int upsized_pow2;
	for (upsized_pow2 = 1 ; upsized_pow2 < n ; upsized_pow2 <<= 1) //  <<= 1 is a bitwise double, same as "times two"
		;

	logd ("Upsized %d to %d", n, upsized_pow2);
	return upsized_pow2;
}


///////////////////////////////////////////////////////////////////////////////
//  IMAGE: GAMMA
///////////////////////////////////////////////////////////////////////////////

void Image_Build_Gamma_Table (float g, float col, byte gammatable_256[])
{
	int	i, inf;

	g = CLAMP(0.3, g, 3);
	col = CLAMP(1, col, 3);

	if (g == 1 && col == 1)
	{
		for (i = 0 ; i < 256 ; i++)
			gammatable_256[i] = i;
		return;
	}

	for (i = 0 ; i < 256 ; i++)
	{
		inf = 255 * pow((i + 0.5) / 255.5 * col, g) + 0.5;
		gammatable_256[i] = CLAMP(0, inf, 255);
	}
}

// See also unsigned Color_For_Gamma_Table (unsigned color, byte gammatable_256[])
void Image_Apply_Gamma_To_RGBA_Pixels (unsigned *rgba, int width, int height, byte gammatable_256[])
{
	byte* data = (byte *)rgba;
	int imagesize = width * height;
	int i;
	for (i = 0 ; i < imagesize ; i++)
	{
		data[4 * i + 0] = gammatable_256[data[4 * i + 0]];
		data[4 * i + 1] = gammatable_256[data[4 * i + 1]];
		data[4 * i + 2] = gammatable_256[data[4 * i + 2]];
	}
}


///////////////////////////////////////////////////////////////////////////////
//  PALETTE REPLACEMENT
///////////////////////////////////////////////////////////////////////////////

static byte *fastpal;
static byte	 fastblack;

void Image_Quick_Palette_256_Free (void)
{
	free (fastpal);
	fastpal = NULL;
}

void Image_Quick_Palette_256_Alloc (int black_index)
{
	// Check to see if it was already allocated
	if (fastpal)
	{
		logd ("Quick palette wasn't closed. Freeing ...");
		Image_Quick_Palette_256_Free ();
	}

	// Baker: Allocate a buffer for all possible RGB combinations, calloc zeros the memory
	fastpal = (byte*)calloc (256 * 256 * 256, 1);
	fastblack = (byte)black_index;
}


byte Palette_Best_Color_Index_For_Pixel (unsigned mypixel, byte palette768[], int numpalcolors)
{
	static byte red, green, blue; //, alpha;
	static int red_distance, green_distance, blue_distance;
	static int color_distance, best_color_index;
	static unsigned best_color_distance;
	static byte *current_palette_index;
	static int i;

	unsigned rgbpix = mypixel & 0xFFFFFF; // Remove alpha from the equation

	byte fasteval = fastpal ? fastpal[rgbpix] : 0;

	if (fasteval)
	{
		if (mypixel == 0)
			return fastblack;

		return fasteval;
	}

	red		= (mypixel      )  & 0xFF;
	green	= (mypixel >> 8 )  & 0xFF;
	blue	= (mypixel >> 16)  & 0xFF;
//	alpha	= (mypixel >> 24)  & 0xFF;

	best_color_index		=  -1;
	best_color_distance		=  0 - 1; // Should be a big number
	current_palette_index	= palette768;

	for (i = 0; i < numpalcolors; i++, current_palette_index += 3 )
	{
		red_distance	= red   - current_palette_index[0];
		green_distance	= green - current_palette_index[1];
		blue_distance	= blue  - current_palette_index[2];
		color_distance	= SQUARED(red_distance) + SQUARED(green_distance) + SQUARED(blue_distance); // Could sqrt this but no reason to do so

		if ((unsigned)color_distance < best_color_distance)
		{
			best_color_distance	= color_distance;
			best_color_index		= i;

			if (best_color_distance == 0)
				break; // Can't beat an exact match
		}
	}

	if (fastpal)
		fastpal[rgbpix] = (byte)best_color_index;

	return (byte)best_color_index;
}

void Image_Convert_Palette_To_Palette (byte *pixels_indexed, int len, byte old_palette_768[], byte new_palette_768[] )
{
	byte	newpal[256]; // For each HL color, we will fill this in
	int i, j;
	unsigned mycolor;
	byte	qpal;

	for (i = 0, j = 0; i < 256; i++, j+=3)
	{
		mycolor = ((unsigned)old_palette_768[j + 0] << 0)
				+ ((unsigned)old_palette_768[j + 1] << 8)
				+ ((unsigned)old_palette_768[j + 2] << 16);

		newpal[i] = qpal = Palette_Best_Color_Index_For_Pixel (mycolor, new_palette_768, 224);
	}

	// Baker: I think Half-Life color #255 is special like Quake
	newpal[255] = 255;

	// Now convert the pixels
	for (i = 0; i < len; i ++)
	{
		pixels_indexed[i] = newpal[pixels_indexed[i]];
	}
}

byte *Image_Buffer_RGBA_To_Palette_Alpha_Threshold_Alloc (unsigned *pixels_rgba, int width, int height, byte palette_768[], int alpha_threshold)
{
	int size = width * height;
	byte *buf = malloc (size);
	byte *dst;
	unsigned *src;
	int i;
	unsigned mypixel;
	byte red, green, blue, alpha;

	for (i = 0, src = (unsigned *) pixels_rgba, dst = buf; i < size; i++, dst++, src++)
	{
		mypixel = *src;
		red		= (mypixel      )  & 0xFF;
		green	= (mypixel >> 8 )  & 0xFF;
		blue	= (mypixel >> 16)  & 0xFF;
		alpha	= (mypixel >> 24)  & 0xFF;

		if (alpha <= alpha_threshold)
			*dst = 255; // Quake transparent color
		else
			*dst = Palette_Best_Color_Index_For_Pixel (*src, palette_768, 255); // First 255 colors, not 256
	}
	return buf;
}



unsigned *fastpal_rgb4;
void Image_Quick_Palette_RGBA_Free (void)
{
	free (fastpal_rgb4);
	fastpal_rgb4 = NULL;
}

void Image_Quick_Palette_RGBA_Alloc (void)
{
	// Check to see if it was already allocated
	if (fastpal_rgb4)
	{
		log_debug ("Quick palette wasn't closed. Freeing ..."); // Because could be a mistake in calling application.
		Image_Quick_Palette_RGBA_Free ();
	}

	// Baker: Allocate a buffer for all possible RGB combinations, calloc zeros the memory
	fastpal_rgb4 = (unsigned *)calloc (256 * 256 * 256, RGBA_4);
}


//#if 0 // Obsolete but don't delete

///////////////////////////////////////////////////////////////////////////////
//  IMAGE: TGA with bgra and rgba support
///////////////////////////////////////////////////////////////////////////////

	static int targa_fgetLittleShort (FILE *f)
	{
	    //int fgetLittleShort (FILE *f); //<--- returns EOF (-1) on failure.  We don't
		byte	b1, b2;

		b1 = fgetc(f);
		b2 = fgetc(f);

		return (short)(b1 + b2*256);
	}

	/* unused
	static int targa_fgetLittleLong (FILE *f)
	{
		byte	b1, b2, b3, b4;

		b1 = fgetc(f);
		b2 = fgetc(f);
		b3 = fgetc(f);
		b4 = fgetc(f);

		return b1 + (b2<<8) + (b3<<16) + (b4<<24);
	}
	*/

	cbool Image_Save_TGA (const char *path_to_file, const void *pels, int width, int height, int bits_per_pixel, cbool upsidedown)
	{
		int		i, size, temp, bytes;
		byte	header[TARGAHEADERSIZE_18];
		byte	*data = (byte *)pels;
		FILE *f;

		f = core_fopen_write (path_to_file, "wb");

		if (!f)
			return false;

		memset (&header, 0, TARGAHEADERSIZE_18);
		header[2] = 2; // uncompressed type
		header[12] = width & 255;  // Write littleShort
		header[13] = width >> 8;
		header[14] = height & 255; // Write littleShort
		header[15] = height >> 8;
		header[16] = bits_per_pixel; // pixel size
		if (upsidedown)
			header[17] = 0x20; //upside-down attribute // Why?

		// swap red and blue bytes
		bytes = bits_per_pixel / 8;
		size = width * height * bytes;

		for (i = 0; i < size; i += bytes)
		{
			temp = data[i];
			data[i] = data[i+2];
			data[i+2] = temp;
		}

		fwrite (&header, 1, TARGAHEADERSIZE_18, f);
		fwrite (data, 1, size, f);

		core_fclose (f);

		return true;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  IMAGE: BUFFERED READ (STATIC)
	///////////////////////////////////////////////////////////////////////////////


	typedef struct stdio_buffer_s
	{
		FILE *f;
		unsigned char buffer[1024];
		int size;
		int pos;
	} stdio_buffer_t;

	static stdio_buffer_t *Buf_Alloc(FILE *f)
	{
		stdio_buffer_t *buf = calloc (1, sizeof(stdio_buffer_t));
		buf->f = f;
		return buf;
	}

	static void Buf_Free(stdio_buffer_t *buf)
	{
		free (buf);
	}

	static inline int Buf_GetC(stdio_buffer_t *buf)
	{
		if (buf->pos >= buf->size)
		{
			buf->size = fread(buf->buffer, 1, sizeof(buf->buffer), buf->f);
			buf->pos = 0;

			if (buf->size == 0)
				return EOF;
		}

		return buf->buffer[buf->pos++];
	}


	unsigned *Image_Load_TGA_FileHandle (FILE *fin, int *width, int *height, malloc_fn_t Malloc_Fn, const char *description)
	{
		int				columns, rows, numPixels;
		byte			*pixbuf;
		int				row, column;
		byte			*targa_rgba;
		int				realrow; //johnfitz -- fix for upside-down targas
		cbool			upside_down; //johnfitz -- fix for upside-down targas
		stdio_buffer_t	*buf;

		targa_header.id_length = fgetc(fin);
		targa_header.colormap_type = fgetc(fin);
		targa_header.image_type = fgetc(fin);

		targa_header.colormap_index = targa_fgetLittleShort(fin);
		targa_header.colormap_length = targa_fgetLittleShort(fin);
		targa_header.colormap_size = fgetc(fin);
		targa_header.x_origin = targa_fgetLittleShort(fin);
		targa_header.y_origin = targa_fgetLittleShort(fin);
		targa_header.width = targa_fgetLittleShort(fin);
		targa_header.height = targa_fgetLittleShort(fin);
		targa_header.pixel_size = fgetc(fin);
		targa_header.attributes = fgetc(fin);

		if (targa_header.image_type!= TGA_UNCOMPRESSED_2 && targa_header.image_type!= TGA_RLE_10)
			log_fatal ("Image_LoadTGA: %s is not a type 2 or type 10 targa", description);

		if (targa_header.colormap_type !=0 || (targa_header.pixel_size!= BPP_32 && targa_header.pixel_size!= BPP_24))
			log_fatal ("Image_LoadTGA: %s is not a 24-bit or 32-bit targa", description);

		columns = targa_header.width;
		rows = targa_header.height;
		numPixels = columns * rows;
		upside_down = !(targa_header.attributes & 0x20); //johnfitz -- fix for upside-down targas

		targa_rgba = (byte *)Malloc_Fn (numPixels * RGBA_4);

		if (targa_header.id_length != 0)
			fseek (fin, targa_header.id_length, SEEK_CUR);  // skip TARGA image comment

		buf = Buf_Alloc(fin);

		if (targa_header.image_type == TGA_UNCOMPRESSED_2) // Uncompressed, RGB images
		{
			for(row=rows-1; row>=0; row--)
			{
				//johnfitz -- fix for upside-down targas
				realrow = upside_down ? row : rows - 1 - row;
				pixbuf = targa_rgba + realrow * columns * RGBA_4;
				//johnfitz
				for(column=0; column<columns; column++)
				{
					unsigned char red,green,blue,alphabyte;
					switch (targa_header.pixel_size)
					{
					case BPP_24:
						blue = Buf_GetC(buf);
						green = Buf_GetC(buf);
						red = Buf_GetC(buf);
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = 255;
						break;
					case BPP_32:
						blue = Buf_GetC(buf);
						green = Buf_GetC(buf);
						red = Buf_GetC(buf);
						alphabyte = Buf_GetC(buf);
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alphabyte;
						break;
					}
				}
			}
		}
		else if (targa_header.image_type == TGA_RLE_10) // Runlength encoded RGB images
		{
			unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;
			for(row = rows - 1; row >= 0; row--)
			{
				//johnfitz -- fix for upside-down targas
				realrow = upside_down ? row : rows - 1 - row;
				pixbuf = targa_rgba + realrow * columns * RGBA_4;
				//johnfitz
				for(column = 0; column < columns; )
				{
					packetHeader=Buf_GetC(buf);
					packetSize = 1 + (packetHeader & 0x7f);
					if (packetHeader & 0x80) // run-length packet
					{
						switch (targa_header.pixel_size)
						{
						case BPP_24:
							blue = Buf_GetC(buf);
							green = Buf_GetC(buf);
							red = Buf_GetC(buf);
							alphabyte = 255;
							break;
						case BPP_32:
							blue = Buf_GetC(buf);
							green = Buf_GetC(buf);
							red = Buf_GetC(buf);
							alphabyte = Buf_GetC(buf);
							break;
						default: /* avoid compiler warnings */
							blue = red = green = alphabyte = 0;
						}

						for(j = 0; j < packetSize; j++)
						{
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alphabyte;
							column++;
							if (column == columns) // run spans across rows
							{
								column=0;
								if (row>0)
									row--;
								else
									goto breakOut;
								//johnfitz -- fix for upside-down targas
								realrow = upside_down ? row : rows - 1 - row;
								pixbuf = targa_rgba + realrow * columns * RGBA_4;
								//johnfitz
							}
						}
					}
					else // non run-length packet
					{
						for(j = 0; j < packetSize; j++)
						{
							switch (targa_header.pixel_size)
							{
							case BPP_24:
								blue = Buf_GetC(buf);
								green = Buf_GetC(buf);
								red = Buf_GetC(buf);
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = 255;
								break;
							case BPP_32:
								blue = Buf_GetC(buf);
								green = Buf_GetC(buf);
								red = Buf_GetC(buf);
								alphabyte = Buf_GetC(buf);
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = alphabyte;
								break;
							default: /* avoid compiler warnings */
								blue = red = green = alphabyte = 0;
							}
							column++;
							if (column == columns) // pixel packet run spans across rows
							{
								column = 0;
								if (row > 0)
									row--;
								else
									goto breakOut;
								//johnfitz -- fix for upside-down targas
								realrow = upside_down ? row : rows - 1 - row;
								pixbuf = targa_rgba + realrow * columns * RGBA_4;
								//johnfitz
							}
						}
					}
				}
				breakOut:;
			}
		}

		Buf_Free(buf);
		core_fclose(fin);

		*width = (int)(targa_header.width);
		*height = (int)(targa_header.height);
		return (unsigned *)targa_rgba;
	}


	///////////////////////////////////////////////////////////////////////////////
	//  IMAGE: PCX (READ ONLY)
	///////////////////////////////////////////////////////////////////////////////

	typedef struct
	{
		char			signature;
		char			version;
		char			encoding;
		char			bits_per_pixel;
		unsigned short	xmin,ymin,xmax,ymax;
		unsigned short	hdpi,vdpi;
		byte			colortable[48];
		char			reserved;
		char			color_planes;
		unsigned short	bytes_per_line;
		unsigned short	palette_type;
		char			filler[58];
	} pcxheader_t;


	unsigned *Image_Load_PCX_FileHandle (FILE *fin, int *width, int *height, size_t filelen, malloc_fn_t Malloc_Fn, const char *description)
	{
		pcxheader_t	pcx;
		int			x, y, w, h, readbyte, runlength, start;
		byte		*p, *data;
		byte		palette[768];
		stdio_buffer_t  *buf;

		start = ftell (fin); //save start of file (since we might be inside a pak file, SEEK_SET might not be the start of the pcx)

		fread (&pcx, sizeof(pcx), 1, fin);
		pcx.xmin = (unsigned short)pcx.xmin;
		pcx.ymin = (unsigned short)pcx.ymin;
		pcx.xmax = (unsigned short)pcx.xmax;
		pcx.ymax = (unsigned short)pcx.ymax;
		pcx.bytes_per_line = (unsigned short)pcx.bytes_per_line;

		if (pcx.signature != 0x0A)
			log_fatal ("'%s' is not a valid PCX file", description);

		if (pcx.version != 5)
			log_fatal ("'%s' is version %d, should be 5", description, pcx.version);

		if (pcx.encoding != 1 || pcx.bits_per_pixel != 8 || pcx.color_planes != 1)
			log_fatal ("'%s' has wrong encoding or bit depth", description);

		w = pcx.xmax - pcx.xmin + 1;
		h = pcx.ymax - pcx.ymin + 1;

		data = (byte *)Malloc_Fn((w * h + 1) * RGBA_4); //+1 to allow reading padding byte on last line

		//load palette
		fseek (fin, start + filelen - 768, SEEK_SET);
		fread (palette, 1, 768, fin);

		//back to start of image data
		fseek (fin, start + sizeof(pcx), SEEK_SET);

		buf = Buf_Alloc(fin);

		for (y=0; y<h; y++)
		{
			p = data + y * w * RGBA_4;

			for (x=0; x < pcx.bytes_per_line; ) //read the extra padding byte if necessary
			{
				readbyte = Buf_GetC(buf);

				if(readbyte >= 0xC0)
				{
					runlength = readbyte & 0x3F;
					readbyte = Buf_GetC(buf);
				}
				else
					runlength = 1;

				while(runlength--)
				{
					p[0] = palette[readbyte*3];
					p[1] = palette[readbyte*3+1];
					p[2] = palette[readbyte*3+2];
					p[3] = 255;
					p += 4;
					x++;
				}
			}
		}

		Buf_Free(buf);
		core_fclose(fin);

		*width = w;
		*height = h;
		return (unsigned *)data;
	}

//#endif // OBSOLETE, BUT DON'T DELETE


cbool Image_Has_Alpha_Channel (const unsigned *rgba, int width, int height)
{
	int pels = width * height;
	int i;
	cbool found_black = false;
	cbool found_gray  = false;
	cbool found_white = false;
	byte *data;

	for (i = 0, data = (byte *) rgba; i < pels; i ++, data += RGBA_4)
	{
		switch (data[3])
		{
			case 0x00: found_black = true; break;
			case 0xFF: found_white = true; break;
			default  : found_gray  = true; return true; // Found gray, get out!
		}

		if (found_black && found_white) return true;
	}

	return false;
}

// Safety is on you!
unsigned Image_Point_Get_Color (const void *pels, unsigned rowbytes, int pixel_bytes, int x, int y)
{
	byte *data_rep = (byte *)pels;
	int row_offset = rowbytes * y + x * pixel_bytes;

	data_rep = &data_rep[row_offset];

	switch (pixel_bytes)
	{
	case RGBA_4:	return (unsigned) ((unsigned *)data_rep)[0];
	case 2:			return (unsigned) ((unsigned short *)data_rep)[0];
	case 1:			return (unsigned) ((byte *)data_rep)[0];
	default:		break;
	}

	log_fatal ("Invalid pixel_bytes");
	return 0;
}

unsigned Image_Point_Set_Color (const void *pels, unsigned rowbytes, int pixel_bytes, int x, int y, unsigned fillcolor)
{
	byte *data_rep = (byte *)pels;
	int row_offset = rowbytes * y + x * pixel_bytes;

	data_rep = &data_rep[row_offset];

	switch (pixel_bytes)
	{
	case RGBA_4:	return (unsigned) (((unsigned *)data_rep)[0] = (unsigned) fillcolor);
	case 2:			return (unsigned) (((unsigned short *)data_rep)[0] = (unsigned short) fillcolor);
	case 1:			return (unsigned) (((byte *)data_rep)[0] = (byte) fillcolor);
	default:		break;
	}

	log_fatal ("Invalid pixel_bytes");
	return 0;
}


unsigned *Image_Bilinear_Resize_Alloc (const unsigned *rgba, int width, int height, int new_width, int new_height)
{
    unsigned *temp_o = calloc (sizeof(unsigned), new_width * new_height);
    int a, b, col, d, x, y, index, i, j ;
    float x_ratio = ((float)(width-1))/new_width ;
    float y_ratio = ((float)(height-1))/new_height ;
    float x_diff, y_diff, blue, red, green ;
    int offset = 0 ;
    for (i=0;i<new_height;i++) {
        for (j=0;j<new_width;j++) {
            x = (int)(x_ratio * j) ;
            y = (int)(y_ratio * i) ;
            x_diff = (x_ratio * j) - x ;
            y_diff = (y_ratio * i) - y ;
            index = (y*width+x) ;
            a = rgba[index] ;
            b = rgba[index+1] ;
            col = rgba[index+width] ;
            d = rgba[index+width+1] ;

            // blue element
            // Yb = Ab(1-width)(1-height) + Bb(width)(1-height) + Cb(height)(1-width) + Db(wh)
            blue = (a&0xff)*(1-x_diff)*(1-y_diff) + (b&0xff)*(x_diff)*(1-y_diff) +
                   (col&0xff)*(y_diff)*(1-x_diff)   + (d&0xff)*(x_diff*y_diff);

            // green element
            // Yg = Ag(1-width)(1-height) + Bg(width)(1-height) + Cg(height)(1-width) + Dg(wh)
            green = ((a>>8)&0xff)*(1-x_diff)*(1-y_diff) + ((b>>8)&0xff)*(x_diff)*(1-y_diff) +
                    ((col>>8)&0xff)*(y_diff)*(1-x_diff)   + ((d>>8)&0xff)*(x_diff*y_diff);

            // red element
            // Yr = Ar(1-width)(1-height) + Br(width)(1-height) + Cr(height)(1-width) + Dr(wh)
            red = ((a>>16)&0xff)*(1-x_diff)*(1-y_diff) + ((b>>16)&0xff)*(x_diff)*(1-y_diff) +
                  ((col>>16)&0xff)*(y_diff)*(1-x_diff)   + ((d>>16)&0xff)*(x_diff*y_diff);

            temp_o[offset++] =
                    0xff000000 | // hardcode alpha
                    ((((int)red)<<16)&0xff0000) |
                    ((((int)green)<<8)&0xff00) |
                    ((int)blue) ;
        }
    }
    return temp_o ;
}


/*
// Literal, if you want to clamp src/dst coords and width, do it in another function
void Image_Paste (int dst_x, int dst_y, int src_x, int src_y, int paste_width, int paste_height, void *dst, int dst_width, int dst_height, const void *src, int src_width, int src_height, int pixel_bytes)
{
	int r;
	byte *bdst = (byte *)dst;
	byte *bsrc = (byte *)src;

	int dst_rowbytes = dst_width * pixel_bytes;
	int src_rowbytes = src_width * pixel_bytes;
	int paste_rowbytes = paste_width * pixel_bytes; // Amount to copy per row

	int dst_offset = dst_y * dst_rowbytes + dst_x * pixel_bytes;
	int src_offset = src_y * src_rowbytes + src_x * pixel_bytes;

	for (r = 0; r < paste_height; r++, dst_offset += dst_rowbytes, src_offset += src_rowbytes)
		memcpy (&bdst[dst_offset], &bsrc[src_offset], paste_rowbytes);
}
*/

// Literal, if you want to clamp src/dst coords and width, do it in another function
void Image_Paste2 (void *dst, unsigned dst_rowbytes, int dst_x, int dst_y,
				   const void *src, unsigned src_rowbytes, int src_x, int src_y, int paste_width, int paste_height, int pixel_bytes)
{
	int r;
	byte *bdst = (byte *)dst;
	byte *bsrc = (byte *)src;

	size_t paste_rowbytes = paste_width * pixel_bytes; // Amount to copy per row

	int dst_offset = dst_y * dst_rowbytes + dst_x * pixel_bytes;
	int src_offset = src_y * src_rowbytes + src_x * pixel_bytes;

	for (r = 0; r < paste_height; r++, dst_offset += dst_rowbytes, src_offset += src_rowbytes)
		memcpy (&bdst[dst_offset], &bsrc[src_offset], paste_rowbytes);
}

//void Image_Paste2_Alpha_Mask_RGBA (byte alpha_threshold, int dst_x, int dst_y, int src_x, int src_y, int paste_width, int paste_height,
//				   void *dst, int dst_width, int dst_height, unsigned dst_rowbytes,
//				   const void *src, int src_width, int src_height, unsigned src_rowbytes)

void Image_Draw_Rect_XOR (
	int pixel_bytes,														/* BOTH     */ \
	void *dst, unsigned dst_rowbytes, int dst_x, int dst_y,					/* DST		*/ \
	const void *unused_0, unsigned unused_1, int unused_2, int unused_3,	/* SRC		*/ \
	int paste_width, int paste_height,										/* OP SIZE	*/ \
	unsigned unused_4, unsigned unused_5, byte unused_6						/* OP INFO	*/
	)

{


	byte *bdst = (byte *)dst;
	byte *pdst;

	int dst_offset = dst_y * dst_rowbytes + dst_x * RGBA_4;

	int r, c;

	for (r = 0; r < paste_height; r++, dst_offset += dst_rowbytes) {
		for (c = 0, pdst = &bdst[dst_offset]; c < paste_width; c++, pdst += RGBA_4) {
			// Take the non-alpha bits and invert them.
				pdst[0] = ~pdst[0];
				pdst[1] = ~pdst[1];
//				pdst[2] = ~pdst[2]
//				pdst[3] = 255;
		}
	}

}

void Image_Paste2_Alpha_Mask_RGBA (void *dst, unsigned dst_rowbytes, int dst_x, int dst_y,
				   const void *src, unsigned src_rowbytes, int src_x, int src_y, int paste_width, int paste_height, int pixel_bytes, byte alpha_threshold)

{
	int r, c;
	byte *bsrc = (byte *)src;
	byte *bdst = (byte *)dst;
	byte *psrc, *pdst;

	size_t paste_rowbytes = paste_width * RGBA_4; // Amount to copy per row

	int dst_offset = dst_y * dst_rowbytes + dst_x * RGBA_4;
	int src_offset = src_y * src_rowbytes + src_x * RGBA_4;

//	for (r = 0; r < paste_height; r++, dst_offset += dst_rowbytes, src_offset += src_rowbytes)
//		memcpy (&bdst[dst_offset], &bsrc[src_offset], paste_rowbytes);
	for (r = 0; r < paste_height; r++, dst_offset += dst_rowbytes, src_offset += src_rowbytes)
		for (c = 0, pdst = &bdst[dst_offset], psrc = &bsrc[src_offset]; c < paste_width; c++, psrc += RGBA_4, pdst += RGBA_4)
			if (psrc[3] >= alpha_threshold)
			{
				memcpy (pdst, psrc, RGBA_4 - 1);
//				pdst[0] = 255;
//				pdst[1] = 255;
//				pdst[2] = 255;
				pdst[3] = 255;
			}
}



void Image_Paste2_Alpha_Mask_Color_RGBA (void *dst, unsigned dst_rowbytes, int dst_x, int dst_y,
				   const void *src, unsigned src_rowbytes, int src_x, int src_y, int paste_width, int paste_height, int pixel_bytes, unsigned fill_color_alpha, byte alpha_threshold)

{
	int r, c;
	byte *bsrc = (byte *)src;
	byte *bdst = (byte *)dst;
	byte *psrc, *pdst;

	size_t paste_rowbytes = paste_width * RGBA_4; // Amount to copy per row

	int dst_offset = dst_y * dst_rowbytes + dst_x * RGBA_4;
	int src_offset = src_y * src_rowbytes + src_x * RGBA_4;

//	for (r = 0; r < paste_height; r++, dst_offset += dst_rowbytes, src_offset += src_rowbytes)
//		memcpy (&bdst[dst_offset], &bsrc[src_offset], paste_rowbytes);
	for (r = 0; r < paste_height; r++, dst_offset += dst_rowbytes, src_offset += src_rowbytes)
		for (c = 0, pdst = &bdst[dst_offset], psrc = &bsrc[src_offset]; c < paste_width; c++, psrc += RGBA_4, pdst += RGBA_4)
			if (psrc[3] >= alpha_threshold)
				memcpy (pdst, &fill_color_alpha, sizeof(fill_color_alpha) );
}

void Image_Paste2_Alpha_Mask_Anti_Rect2_RGBA (void *dst, unsigned dst_rowbytes, int dst_x, int dst_y,
				   const void *src, unsigned src_rowbytes, int src_x, int src_y, int paste_width, int paste_height,
				   int pixel_bytes, unsigned forecolor, unsigned backcolor, byte alpha_threshold)

{
	int r, c;
	byte *bsrc = (byte *)src;
	byte *bdst = (byte *)dst;
	byte *psrc, *pdst;

	size_t paste_rowbytes = paste_width * RGBA_4; // Amount to copy per row

	int dst_offset = dst_y * dst_rowbytes + dst_x * RGBA_4;
	int src_offset = src_y * src_rowbytes + src_x * RGBA_4;

//	for (r = 0; r < paste_height; r++, dst_offset += dst_rowbytes, src_offset += src_rowbytes)
//		memcpy (&bdst[dst_offset], &bsrc[src_offset], paste_rowbytes);
	for (r = 0; r < paste_height; r++, dst_offset += dst_rowbytes, src_offset += src_rowbytes)
		for (c = 0, pdst = &bdst[dst_offset], psrc = &bsrc[src_offset]; c < paste_width; c++, psrc += RGBA_4, pdst += RGBA_4)
			if (psrc[3] >= alpha_threshold)
				memcpy (pdst, &forecolor, sizeof(forecolor) );
			else memcpy (pdst, &backcolor, sizeof(backcolor) );
}


/* Retired ...
void Image_Rect_Fill (unsigned fillcolor, int x, int y, int paint_width, int paint_height, void *pels, int pix_width, int pix_height, int pixel_bytes)
{
	byte *buf = (byte *)pels;
	int rowbytes = pix_width * pixel_bytes;
	int startoffset = y * rowbytes + x * pixel_bytes;
	int i, j, bufoffset;

	// Non-byte pixels must fill the first row immediately
	switch (pixel_bytes)
	{
	case 2: // Short
		for (i = 0; i < paint_width; i ++)
			((short *)buf)[y * pix_width + x + i] = (short)fillcolor; // Fill first row
		break;

	case RGBA_4: // 4, unsigned
		for (i = 0; i < paint_width; i ++)
			((unsigned *)buf)[y * pix_width + x + i] = fillcolor; // Fill first row
		break;

	default: // Hopefully default is 1
		break;
	}

	for (j = 0, bufoffset = startoffset; j < paint_height; y ++, j++, bufoffset += rowbytes)
	{
		// Single byte pixels: memset.  multi-byte pixels: skip first row we already filled, then memcpy it
		if (pixel_bytes == 1) memset (&buf[bufoffset], fillcolor, paint_width); // byte pixels just memset
		else if (j > 0) memcpy (&buf[bufoffset], &buf[startoffset], paint_width * pixel_bytes);
	}

}
*/




/*
void Image_Rect_Fill2 (unsigned fillcolor, int x, int y, int paint_width, int paint_height, void *pels, int pix_width, int pix_height, unsigned pix_rowbytes, int pixel_bytes)
{
	byte *buf = (byte *)pels;
//	int rowbytes = pix_width * pixel_bytes;
	int startoffset = y * pix_rowbytes + x * pixel_bytes;
	int i, j, bufoffset;

	// Non-byte pixels must fill the first row immediately
	switch (pixel_bytes)
	{
	case 2: // Short
		for (i = 0; i < paint_width; i ++)
			((short *)buf)[y * pix_width + x + i] = (short)fillcolor; // Fill first row
		break;

	case RGBA_4: // 4, unsigned
		for (i = 0; i < paint_width; i ++)
			((unsigned *)buf)[y * pix_width + x + i] = fillcolor; // Fill first row
		break;

	default: // Hopefully default is 1
		break;
	}

	for (j = 0, bufoffset = startoffset; j < paint_height; y ++, j++, bufoffset += pix_rowbytes)
	{
		// Single byte pixels: memset.  multi-byte pixels: skip first row we already filled, then memcpy it
		if (pixel_bytes == 1) memset (&buf[bufoffset], fillcolor, paint_width); // byte pixels just memset
		else if (j > 0) memcpy (&buf[bufoffset], &buf[startoffset], paint_width * pixel_bytes);
	}

}
*/

#if 1111111
//#pragma message ("We know ultimately OpenGL drawing functions should be float, but keep any API open to double.")
//#pragma message ("The problem with float is that is it 24-bit int and cannot represent even signed int, but double can.")
//#pragma message ("But most graphics hardware is specifically GPU float friendly (but not double friendly).")
//#pragma message ("Ultimately we need to use float for graphics, not double.")


static void Image_Draw_Scan_Line (void *pels, int width, int height, unsigned rowbytes, int pixel_bytes, double x0, double x1, double y, unsigned fill_color)
{
	if (x0 > x1)
		c_swapd (&x0, &x1);
	else
		x0 = x0;

	// Shitty ...
	Image_Rect_Fill3 (pels, rowbytes, c_rint(x0), c_rint(y), c_rint(x1 - x0 + 1), 1, pixel_bytes, fill_color);
}

#define OVERDRAW_1 0

//#define crint()
static void Image_Triangle_Bottom_Fill_Limits (void *pels, int width, int height, unsigned rowbytes, int pixel_bytes, int lx, int ly, int lw, int lh, double x1, double y1, double _x2, double y2, double _x3, double y3, unsigned fill_color)
{
	double x2 = (_x3 > _x2) ? _x3 : _x2;
	double x3 = (_x3 > _x2) ? _x2 : _x3;

	double invslope1 = (x2 - x1) / (y2 - y1);
    double invslope2 = (x3 - x1) / (y3 - y1);

    double curx1 = x1;
    double curx2 = x1;

	int scanline_y;

	for (scanline_y = y1; scanline_y < y2; scanline_y ++, curx1 += invslope1, curx2 += invslope2)
	{
		// If scanline hasn't entered scope, continue ...
		// If scanline exited scope, break;
		double draw_x1 = curx1 - OVERDRAW_1;
		double draw_x2 = curx2 + OVERDRAW_1;

		Image_Draw_Scan_Line (pels, width, height, rowbytes, pixel_bytes, draw_x1, draw_x2, scanline_y, fill_color + 0xFF);
	}
}

// Wider at the bottom, a point at the top.
static void Image_Triangle_Top_Fill_Limits (void *pels, int width, int height, unsigned rowbytes, int pixel_bytes, int lx, int ly, int lw, int lh, double _x1, double y1, double _x2, double y2, double x3, double y3, unsigned fill_color)
{
	double x1 = (_x1 > _x2) ? _x2 : _x1;
	double x2 = (_x1 > _x2) ? _x1 : _x2;
	double invslope1 = (x3 - x1) / (y3 - y1);
    double invslope2 = (x3 - x2) / (y3 - y2);

    double curx1 = x3;
    double curx2 = x3;

	int scanline_y;

	for (scanline_y = y3; scanline_y >= y1; scanline_y --, curx1 -= invslope1, curx2 -= invslope2)
	{
		// If scanline hasn't entered scope, continue ...
		// If scanline exited scope, break;
		double draw_x1 = curx1 - OVERDRAW_1;
		double draw_x2 = curx2 + OVERDRAW_1;
		Image_Draw_Scan_Line (pels, width, height, rowbytes, pixel_bytes, draw_x1, draw_x2, scanline_y, fill_color);
	}
}

// Note: The limits don't work, I didn't code them.
// Which is fine because we get seams and we otherwise aren't ready for prime time and we need to use floats instead of doubles anyway.
void Image_Triangle_Fill_Limits (void *pels, int width, int height, unsigned rowbytes, int pixel_bytes, int lx, int ly, int lw, int lh, double x1, double y1, double x2, double y2, double x3, double y3, unsigned fill_color)
{
    if (y2 > y3)
		{ c_swapd (&x2, &x3); c_swapd (&y2, &y3); }
    if (y1 > y2)
		{ c_swapd (&x1, &x2); c_swapd (&y1, &y2); }
    if (y2 > y3)  // Again ...
		{ c_swapd (&x2, &x3); c_swapd (&y2, &y3); }

    // here we know that v1.y <= v2.y <= v3.y
    // check for trivial case of bottom-flat triangle
    if (y2 == y3)
        Image_Triangle_Bottom_Fill_Limits(pels, width, height, rowbytes, pixel_bytes, lx, ly, lw, lh, x1, y1, x2, y2, x3, y3, fill_color);
    // check for trivial case of top-flat triangle
    else if (y1 == y2)
        Image_Triangle_Top_Fill_Limits (pels, width, height, rowbytes, pixel_bytes, lx, ly, lw, lh, x1, y1, x2, y2, x3, y3, fill_color);
    else
    {
        // general case - split the triangle in a topflat and bottom-flat one
        double x4 = x1 + ((y2 - y1) / (y3 - y1) * (x3 - x1)); // Should this be doubley?
		//int x4 = x1 + (( (double)y2 - y1) / ((double)y3 - y1) * ((double)x3 - x1)); // Should this be doubley?
		double y4 = y2;

        Image_Triangle_Bottom_Fill_Limits (pels, width, height, rowbytes, pixel_bytes, lx, ly, lw, lh, x1, y1, x2, y2, x4, y4, fill_color);
        Image_Triangle_Top_Fill_Limits (pels, width, height, rowbytes, pixel_bytes, lx, ly, lw, lh, x2, y2, x4, y4, x3, y3, fill_color);
    }
}

void Image_Triangle_Fan_Fill_Limits (void *pels, int width, int height, unsigned rowbytes, int pixel_bytes, int lx, int ly, int lw, int lh, int num_points, double *dpoints, unsigned fill_color)
{
	int n;

	if (num_points < 3)
		log_fatal ("Triangle Fan without at least 3 points");
	else
	{
		double fixed_x = dpoints[0], fixed_y = dpoints[1];
		for (n = 1; n < num_points - 1; n ++)
		{
			double x1 = dpoints[ 2 * (n + 0) + 0],  y1 = dpoints[2 * (n + 0) + 1];
			double x2 = dpoints[ 2 * (n + 1) + 0],  y2 = dpoints[2 * (n + 1) + 1];
			Image_Triangle_Fill_Limits (pels, width, height, rowbytes, pixel_bytes,
				lx, ly, lw, lh, fixed_x, fixed_y,
				x1, y1, x2, y2, fill_color );//+ 100 * n);
		}
	}
}


void Image_Regular_Polygon_Fill_Limits (void *pels, int width, int height, unsigned rowbytes, int pixel_bytes, int lx, int ly, int lw, int lh, int num_sides, double center_x, double center_y, double radius, unsigned fill_color)
{
	double *dpoints = Polygon_Regular_Make_DPoints_Alloc (num_sides, center_x, center_y, radius);
	int n;
	for (n = 0; n < num_sides; n ++)
		logd ("%f, %f", dpoints[n*2 +0], dpoints[n*2 +1]);
	Image_Triangle_Fan_Fill_Limits (pels, width, height, rowbytes, pixel_bytes, lx, ly, lw, lh, num_sides, dpoints, fill_color);

	free (dpoints);
}

#if 0


	if (0) {
#if 0
		int x1 = 320, y1 = 0;
		int x2 = 0,	y2 = 480;
		int x3 = 640, y3 = 480;
/*#else
		int x1 = 5, y1 = 240;
		int x2 = 635,	y2 = 5;
		int x3 = 477, y3 = 475;
*/
#else
		int x1 = 320, y1 = 480;
		int x2 = 0,	y2 = 0;
		int x3 = 640, y3 = 0;


#endif
		crect_t r = {0};
		cbitmap_t *bm = &doc.cb_backbuffer;
		unsigned fill_color = 0xFF00;
		Rect_Expand_Triangle (&r, x1, y1, x2, y2, x3, y3, NULL);


//		Bitmap_Fill_Rect (&doc.cb_backbuffer, 0xFF, r.left, r.top, r.width, r.height);
#if 0
		Bitmap_Fill_Triangle (&doc.cb_backbuffer, 0x0000FF00, x1, y1, x2, y2, x3, y3);
#else
	Image_Triangle_Fill_Limits (bm->rgba_a, bm->width, bm->height, bm->rowbytes, RGBA_4, 0, 0, bm->width, bm->height,
		x1, y1, x2, y2, x3, y3, fill_color);
#endif
		Bitmap_Clipboard_Copy (&doc.cb_backbuffer);
	}
	else
	{

		cbitmap_t *bm = &doc.cb_backbuffer;
		unsigned fill_color = 0xFF00;

		Image_Regular_Polygon_Fill_Limits (bm->rgba_a, bm->width, bm->height, bm->rowbytes, RGBA_4,
			0, 0, bm->width, bm->height,
			5, 450, 240, 100, 0xff00);

//		int points[] = { 20, 20, 578, 11, 603, 470, 40, 380 };
//			Image_Triangle_Fan_Fill_Limits (bm->rgba_a, bm->width, bm->height, bm->rowbytes, RGBA_4, 0, 0, bm->width, bm->height, 4, points, fill_color);


	}

//	{
//		c
//	Image_Triangle_Fill_Limits (bm->rgba_a, bm->width, bm->height, bm->rowbytes, RGBA_4, 0, 0, bm->width, bm->height,
//		x1, y1, x2, y2, x3, y3, fill_color);
#endif
#endif

void Image_Rect_Fill3 (void *pels, unsigned rowbytes, int x, int y, int paint_width, int paint_height, int pixel_bytes, unsigned fill_color)
{
	byte *buf = (byte *)pels;
//	int rowbytes = pix_width * pixel_bytes;
	int startoffset = y * rowbytes + x * pixel_bytes;
	int i, j, bufoffset;
	int pix_width = rowbytes / pixel_bytes; // Because we reinterpret the buffer as short or unsigned for first row paint
	// Non-byte pixels must fill the first row immediately

#ifdef _DEBUG
	if (paint_width < 0 || paint_height < 0)
		alert ("Negative size");
#endif

	switch (pixel_bytes)
	{
	case 2: // Short
		for (i = 0; i < paint_width; i ++)
			((short *)buf)[y * pix_width + x + i] = (short)fill_color; // Fill first row
		break;

	case RGBA_4: // 4, unsigned
		for (i = 0; i < paint_width; i ++)
			((unsigned *)buf)[y * pix_width + x + i] = fill_color; // Fill first row
		break;

	default: // Hopefully default is 1
		break;
	}

	for (j = 0, bufoffset = startoffset; j < paint_height; y ++, j++, bufoffset += rowbytes)
	{
		// Single byte pixels: memset.  multi-byte pixels: skip first row we already filled, then memcpy it
		if (pixel_bytes == 1) memset (&buf[bufoffset], fill_color, paint_width); // byte pixels just memset
		else if (j > 0) memcpy (&buf[bufoffset], &buf[startoffset], paint_width * pixel_bytes);
	}

}

//void Image_Rect_Fill (unsigned fillcolor, int x, int y, int paint_width, int paint_height, void *pels, int pix_width, int pix_height, int pixel_bytes)
//void Image_Rect_Fill3 (void *pels, unsigned rowbytes, unsigned fillcolor, int x, int y, int paint_width, int paint_height, int pixel_bytes)

void sImage_Format_Buffer (byte *buf, int w, int h, int pixel_bytes, unsigned fillcolor)
{
//	if (pixel_bytes == RGBA_4) Image_Rect_Fill (fillcolor,0,0,w,h,buf,w,h,pixel_bytes);  // unsigned
	if (pixel_bytes == RGBA_4) Image_Rect_Fill3 (buf,w*h*pixel_bytes,0,0,w,h,pixel_bytes,fillcolor);  // unsigned
	else if (pixel_bytes == 1) memset (buf, fillcolor, w * h);
	else log_fatal ("Invalid pixel_bytes"); // We can actually handle 2 above using same text as if (pixel_bytes == RGBA_4)
}

void *Image_Enlarge_Canvas_Alloc (const void *pels, int width, int height, int pixel_bytes, int new_width, int new_height, unsigned fillcolor, cbool centered)
{
	byte *temp_o = calloc (pixel_bytes, new_width * new_height);

	if (fillcolor != 0) sImage_Format_Buffer (temp_o, new_width, new_height, pixel_bytes, fillcolor);

	if (centered)
	{
		int center_x = c_rint ((new_width - width) / 2.0);
		int center_y = c_rint ((new_height - height) / 2.0);
//		Image_Paste (center_x,center_y,0,0,width,height,temp_o,new_width,new_height,pels,width,height,pixel_bytes);
//void Image_Paste2 (void *dst, unsigned dst_rowbytes, int dst_x, int dst_y,
//				   const void *src, unsigned src_rowbytes, int src_x, int src_y, int paste_width, int paste_height, int pixel_bytes)
		Image_Paste2 (temp_o, new_width * pixel_bytes, center_x, center_y, pels, width * pixel_bytes, 0, 0, width, height, pixel_bytes);
	}
//	else Image_Paste (0,0,0,0,width,height,temp_o,new_width,new_height,pels,width,height,pixel_bytes);
//void Image_Paste2 (void *dst, unsigned dst_rowbytes, int dst_x, int dst_y,
//				   const void *src, unsigned src_rowbytes, int src_x, int src_y, int paste_width, int paste_height, int pixel_bytes)
	else Image_Paste2 (temp_o, new_width * pixel_bytes, 0, 0, pels, width * pixel_bytes, 0, 0, width, height, pixel_bytes);
	return (void *)temp_o;
}



// Literal, if you want to clamp src/dst coords and width, do it in another function
void *Image_Crop_Alloc (int pixel_bytes, void *src, int src_width, int src_height, unsigned src_row_bytes, int src_x, int src_y, int src_copy_width, int src_copy_height)
{
	int new_width = src_copy_width, new_height = src_copy_height;
	unsigned new_rowbytes = new_width * pixel_bytes;
	byte *new_image_o = calloc (new_rowbytes * new_height, 1);

	Image_Paste2 (new_image_o, new_rowbytes, 0, 0, src, src_row_bytes, src_x, src_y, new_width, new_height, pixel_bytes);
//	Image_Paste (0, 0, x, y, new_width, new_height, temp_o, new_width, new_height, pels, width, height, pixel_bytes);
//void Image_Paste2 (void *dst, unsigned dst_rowbytes, int dst_x, int dst_y,
//				   const void *src, unsigned src_rowbytes, int src_x, int src_y, int paste_width, int paste_height, int pixel_bytes)
	//Image_Paste2 (temp_o, new_width * pixel_bytes, 0, 0, pels, width * pixel_bytes, x, y, new_width, new_height, pixel_bytes);
	return (void *)new_image_o;
}

///////////////////////////////////////////////////////////////////////////////
//  IMAGE: Create / Free
///////////////////////////////////////////////////////////////////////////////


// Literal, if you want to clamp src/dst coords and width, do it in another function
void *Image_Create_Alloc (int width, int height, int pixel_bytes, reply size_t *bufsize, unsigned fillcolor)
{
	size_t sz = pixel_bytes * width * height;
    byte *temp_o = calloc (sz, 1);

	if (fillcolor)
		sImage_Format_Buffer (temp_o, width, height, pixel_bytes, fillcolor);

	NOT_MISSING_ASSIGN(bufsize, sz);

	return (void *)temp_o;
}

// Literal, if you want to clamp src/dst coords and width, do it in another function
void *Image_Free (void * buf)
{
	free (buf);
	return NULL;
}

//NO use clipboard methods
/*
unsigned *Image_Clipboard_Load_Alloc (int *outwidth, int *outheight)
{
	return System_Clipboard_Get_Image_RGBA_Alloc (outwidth, outheight);
}

cbool Image_Clipboard_Copy (const unsigned *rgba, int width, int height)
{
	return System_Clipboard_Set_Image_RGBA (rgba, width, height);
}
*/


unsigned *Image_MipMapW (unsigned *rgba, int width, int height)
{
	int		i, pixels;
	byte	*out, *in;

	out = in = (byte *)rgba;
	pixels = (width * height) / 2; // >> 1; // divide by 2

	for (i = 0; i < pixels; i++, out+= RGBA_4, in += RGBA_4 * 2)
	{
		out[0] = (in[0] + in[4]) / 2; // >> 1; // divide by 2
		out[1] = (in[1] + in[5]) / 2; // >> 1;
		out[2] = (in[2] + in[6]) / 2; // >> 1;
		out[3] = (in[3] + in[7]) / 2; // >> 1;
	}

	return rgba;
}



unsigned *Image_MipMapH (unsigned *rgba, int width, int height)
{
	int rowbytes = width * RGBA_4; // <<= 2;  // Multiply by 4
	int		i, j;
	byte	*out, *in;

	out = in = (byte *)rgba;
	height /= 2; // >>= 1; // Divide by 2


	for (i = 0; i < height; i++, in += rowbytes)
	{
		for (j = 0; j < rowbytes; j += RGBA_4, out += RGBA_4, in += RGBA_4)
		{
			out[0] = (in[0] + in[rowbytes + 0]) / 2; // Divide by 2
			out[1] = (in[1] + in[rowbytes + 1]) / 2;
			out[2] = (in[2] + in[rowbytes + 2]) / 2;
			out[3] = (in[3] + in[rowbytes + 3]) / 2;
		}
	}

	return rgba;
}


void *Image_Copy_Alloc (void *pels, int width, int height, int pixelbytes)
{
	return core_memdup (pels, width * height * pixelbytes);
}

