
#ifndef __COLOR_H__
#define __COLOR_H__

///////////////////////////////////////////////////////////////////////////////
//  COLOR: Basic function setup
///////////////////////////////////////////////////////////////////////////////

//typedef	float color4f_t[4];
typedef union {
	struct {
		float red;
		float green;
		float blue;
		float alpha;
	};
	float c4[4];
} color4;

#define COLOR4_SEND(c) (c).red, (c).green, (c).blue, (c).alpha

typedef unsigned color4byte;
//void Color4_From_UnsignedColor (float v4[], color4byte rgba);
color4 Color4_From_UnsignedColor (color4byte rgba);

unsigned Color_From_Bytes (byte red, byte green, byte blue, byte alpha);
void Color_To_Bytes (unsigned color, byte *red, byte *green, byte *blue, byte *alpha);
unsigned Color_For_Gamma_Table (unsigned color, byte gammatable_256[]);



const char *Color4_To_String (const float *mycolor);

//typedef enum
#define coloru_black	0x00000000 // enum
extern const color4 color4_gray; 
extern const color4 color4_black; 
extern const color4 color4_white; 
extern const color4 color4_red; 
extern const color4 color4_green; 
extern const color4 color4_blue; 
extern const color4 color4_yellow; 
extern const color4 color4_black; 
extern const color4 color4_purple; 

//#define ucolor_white = (~0);

/*
#define COLOR_BYTES_FOR_RGBA(_color, _red, _green, _blue, _alpha) \
	byte _red	= (_color      )  & 0xFF; \
	byte _green	= (_color >> 8 )  & 0xFF; \
	byte _blue	= (_color >> 16)  & 0xFF; \
	byte _alpha	= (_color >> 24)  & 0xFF
*/

///////////////////////////////////////////////////////////////////////////////
//  COLOR: HTML Color
///////////////////////////////////////////////////////////////////////////////

void HTML_Color_Clampf (const char *s, float v4[]); // Sets a color 4 float from a string
//unsigned HTML_Color_RGBA (const char *s); // returns a color 4 byte from a string
const char *Color_To_HTML_String (unsigned rgba); // Returns string from static buffer

unsigned Color_From_HTML_String (const char *s);

#define COLOR_RGBA(red, green, blue, alpha) ((unsigned)red + ((unsigned)green << 8) + ((unsigned)blue << 16) + ((unsigned)alpha << 24))
#define ALPHA_SOLID_255 255
#define ALPHA_FULL_TRANSPARENT_0 0


#endif // ! __COLOR_H__