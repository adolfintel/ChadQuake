/*
Copyright (C) 2011-2014 Baker

*/
// color.c -- core functions


#include "core.h"
#include "color.h" // courtesy


const color4 color4_gray = { 0.25, 0.25, 0.25, 0 };
const color4 color4_black = { 0,0,0, 0 };
const color4 color4_white = { 1,1,1,0};
const color4 color4_red = { 1,0,0, 0 };
const color4 color4_green = { 0,1,0,0};
const color4 color4_blue = { 0,0,1,0};
const color4 color4_yellow = { 1,1,0,0};
const color4 color4_purple = { 1,0,1,0};


///////////////////////////////////////////////////////////////////////////////
//  COLOR: Basic function setup
///////////////////////////////////////////////////////////////////////////////

unsigned Color_From_Bytes (byte red, byte green, byte blue, byte alpha)
{
	return ((unsigned)red + ((unsigned)green << 8) + ((unsigned)blue << 16) + ((unsigned)alpha << 24));
}



void Color_RGBA_To_Bytes (unsigned color, byte *red, byte *green, byte *blue, byte *alpha)
{
	*red				= (color      )  & 0xFF;
	*green				= (color >> 8 )  & 0xFF;
	*blue				= (color >> 16)  & 0xFF;
	*alpha				= (color >> 24)  & 0xFF;
}


#define COLOR_PERCENT_TO_BYTE(_f) ( (_f) == 1.0f? 255 : floor( (_f) * 256.0f ) )
unsigned Color_RGBA_From_Percents (float redPercent, float greenPercent, float bluePercent, float alphaPercent)
{
	byte red	= (byte)COLOR_PERCENT_TO_BYTE(redPercent) ; //  multiplies x 256 except for 1 which becomes 255
	byte green	= (byte)COLOR_PERCENT_TO_BYTE(greenPercent);
	byte blue	= (byte)COLOR_PERCENT_TO_BYTE(bluePercent);
	byte alpha	= (byte)COLOR_PERCENT_TO_BYTE(alphaPercent);

	return Color_From_Bytes (red, green, blue, alpha);
}

unsigned Color_For_Gamma_Table (unsigned color, byte gammatable_256[])
{
	byte red, green, blue, alpha;
	Color_RGBA_To_Bytes (color, &red, &green, &blue, &alpha);
	red   = gammatable_256[red], green = gammatable_256[green], blue  = gammatable_256[blue];
	return Color_From_Bytes (red, green, blue, alpha);
}


#if 0
void Color_Blend_In_Place (byte *result, const byte *dst, const byte *src)
{
	// http://stackoverflow.com/questions/12011081/alpha-blending-2-rgba-colors-in-c
/*	void Color_Blend (unsigned char result[4], unsigned char fg[4], unsigned char bg[4])
    unsigned int alpha = fg[3] + 1;
    unsigned int inv_alpha = 256 - fg[3];
    result[0] = (unsigned char)((alpha * fg[0] + inv_alpha * bg[0]) >> 8);
    result[1] = (unsigned char)((alpha * fg[1] + inv_alpha * bg[1]) >> 8);
    result[2] = (unsigned char)((alpha * fg[2] + inv_alpha * bg[2]) >> 8);
    result[3] = 0xff;
*/

}


unsigned Color_Blend (unsigned dst, unsigned src)
{
// Hmmm.  This func takes (byte *), depends on endianness.  Seems dodgy to me.  Ever tested?
	Color_Blend_In_Place (&dst, &dst, &src);
	return dst; // modified by the above
}
#endif

const char *Color4_To_String (const float *mycolor)
{
	static char colorstring[64];
	c_snprintfc (colorstring, sizeof(colorstring), "r: %1.3f g: %1.3f b: %1.3f a: %1.3f",
		mycolor[0], mycolor[1], mycolor[2], mycolor[3] );
	return colorstring;
}



///////////////////////////////////////////////////////////////////////////////
//  COLOR: HTML Color
///////////////////////////////////////////////////////////////////////////////


typedef struct
{
	const char	*name;
	const char	*hex;
	unsigned	rgba; // With alpha = 1?
	float		color4f[RGBA_4]; // Need space for unused alpha component.
} html_colors_t;


static html_colors_t HTML_Colors[] =
{
#define ITEMDEF(_namestring, _hexstring ) { _namestring, _hexstring },
#include "color_html_colors.enum.h"  // { "AliceBlue", "#F0F8FF"}, etc.
	{ NULL }
};


void HTML_Color_Clampf (const char *s, float v4[]);

static void sHTML_Colors_Init (void)
{
	html_colors_t *cur;
	int i;
	for (i = 0; (cur = &HTML_Colors[i]) && cur->name ; i++)
	{
		//System_Alert ("%d " QUOTED_S " %s", i, cur->name, cur->hex);
		cur->rgba = Color_From_HTML_String (cur->hex);
		HTML_Color_Clampf (cur->hex, cur->color4f);
		cur->color4f[3] = 1.0;
	}
}

// These are close but not perfect (no rounding, division range might need to be 255.99999 ?)
#define COLOR_BYTE_TO_UNIT_INTERVAL(_ch)		( (_ch) == 255 ? 1 : (_ch) / 255.0f )
#define UNIT_INTERVAL_TO_COLOR_BYTE_TO_(_fl)	( (_fl) >= 1.0 ? 255 : (_fl) <= 0.0 ? 0 : (_fl) / 255.0f )


static void Vec4_From_UnsignedColor (float v4[], color4byte rgba)
{
	byte red   = (rgba      )  & 0xFF;
	byte green = (rgba >> 8 )  & 0xFF;
	byte blue  = (rgba >> 16)  & 0xFF;
	byte alpha = (rgba >> 24)  & 0xFF;

	v4[0] = COLOR_BYTE_TO_UNIT_INTERVAL(red);  // byte / 256 or 1 if 255
	v4[1] = COLOR_BYTE_TO_UNIT_INTERVAL(green);
	v4[2] = COLOR_BYTE_TO_UNIT_INTERVAL(blue);
	v4[3] = COLOR_BYTE_TO_UNIT_INTERVAL(alpha);
}


color4 Color4_From_UnsignedColor (color4byte rgba)
{
	color4 out;
	Vec4_From_UnsignedColor (out.c4, rgba);
	return out;
}


// unit interval
void HTML_Color_Clampf (const char *s, float v4[])
{
	color4byte rgba = Color_From_HTML_String (s);
	Vec4_From_UnsignedColor (v4, rgba);
}

unsigned Color_From_HTML_String (const char *s)
{
	html_colors_t *cur;
	int i;

	if (s[0] == '#' && strlen (s) == 7)
	{ // HTML #rrggbb format
		int red_high	= hex_char_to_int(s[1]), red_low   = hex_char_to_int(s[2]);
		int green_high	= hex_char_to_int(s[3]), green_low = hex_char_to_int(s[4]);
		int blue_high	= hex_char_to_int(s[5]), blue_low  = hex_char_to_int(s[6]);

		byte red = red_high * 16 + red_low, green = green_high * 16 + green_low, blue = blue_high  * 16 + blue_low, alpha = 255;

		return ((unsigned)red + ((unsigned)green << 8) + ((unsigned)blue << 16) + ((unsigned)alpha << 24));
	}

	// HTML string eval like ("Teal")

	// Initialize table if it isn't, we check first entry to see rgba int is filled to determine
	if (HTML_Colors[0].rgba == 0) sHTML_Colors_Init ();

	for (i = 0; ((cur = &HTML_Colors[i])->name) ; i++)
		if (!strcasecmp (cur->name, s)) return cur->rgba;

	return (unsigned)(-1); // Error, return white I guess.
}


const char *Color_To_HTML_String (unsigned rgba)
{
	html_colors_t *cur;
	int i;

	rgba |= ((unsigned)255 << 24); // Set alpha to 1

	// Initialize table if it isn't, we check first entry to see rgba int is filled to determine
	if (HTML_Colors[0].rgba == 0)
		sHTML_Colors_Init ();

	for (i = 0; (cur = &HTML_Colors[i]) && cur->name ; i++)
		if (cur->rgba == rgba) return cur->name;

	// Didn't hit an HTML color, convert to #rrggbb string and return
	{
		static char nomatch[8];  // #ffcc33 + null term
		byte red   = (rgba      )  & 0xFF;
		byte green = (rgba >> 8 )  & 0xFF;
		byte blue  = (rgba >> 16)  & 0xFF;
		byte alpha = (rgba >> 24)  & 0xFF;

		c_snprintf3 (nomatch, "#%02x%02x%02x", red, green, blue);
		return (const char *)nomatch;
	}
}
