/*
Copyright (C) 2013-2014 Baker

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
// math_general.h -- basic


#ifndef __MATH_GENERAL_H__
#define __MATH_GENERAL_H__

#define DEGREES_TO_RADIANS(__ANGLE)		((__ANGLE) / 180.0 * M_PI)	 // 1 degrees =  0.0174532925 radians
#define RADIANS_TO_DEGREES(__RADIANS)	((__RADIANS) * (180.0 / M_PI)) // 1 radians = 57.2957795    degrees

#define RANDOM_FLOAT ((float)rand()/(float)RAND_MAX)
#define RANDOM_INT
      
int random_int (int low, int high);

cbool is_pow2 (unsigned int x);
//unsigned int NextPowerOfTwo (unsigned int v);
unsigned int roundup_pow2 (unsigned int x);

#define roundup_16(n) (((n) + 15) & ~15)
#define roundup_8(n) (((n) + 7) & ~7)
int hex_char_to_int (char ch);

int math_sign (int x);

#define Math_KiloBytesDouble(_size) (_size / 1024.0)

#define ONE_DIV_16 (1/16.0f) // Commonly needed in calculations here so made it a constant.  Is necessary?


float Math_Largest_Multiple_For_Screen (float grainsize, cbool rotated_90, int wanted_width_, int wanted_height_, int client_width, int client_height, reply int *offset_x_out, reply int *offset_y_out, reply int *new_width_out, reply int *new_height_out);


double Cycle_Circular_In_X_Seconds (double raw_clocksecs, double cycle_in_x_seconds, double low, double high);
double angle_maybe_wrap (double angle, reply cbool *did_wrap_out);



// The low should be 1 and the high should be zero to signal uninitialized
#define RANGE_MIN_START 1
#define RANGE_MAX_START 0
cbool range_extend_d (double value, required double *lo, required double *hi);
cbool range_extend_f (float value, required float *lo, required float *hi);


#define cosd(a)					\
	(	a == 0    ?  1 :		\
		a == 90   ?  0 :		\
		a == 180  ? -1 :		\
		a == 270  ?  0 :		\
		a == -90  ?  0 :		\
	cos(a * M_PI / 180.0) )

#define sind(a)					\
	(	a == 0    ?  0 :		\
		a == 90   ?  1 :		\
		a == 180  ?  0 :		\
		a == 270  ? -1 :		\
		a == -90  ? -1 :		\
	sin(a * M_PI / 180.0) )

#define isin0(x) (0)					// No, not insane.  You can mark an future list with this.  Beats a comment.
#define isin1(x,a) ((x) == (a))			// No, not insane.  What if you expect more, but can only remember one or only know of 1 at the moment.
#define isin2(x,a,b) ((x) == (a) || (x) == (b))
#define isin3(x,a,b,c) ((x) == (a) || (x) == (b) || (x) == (c))
#define isin4(x,a,b,c,d) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d))
#define isin5(x,a,b,c,d,e) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e))
#define isin6(x,a,b,c,d,e,f) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f))
#define isin7(x,a,b,c,d,e,f,g) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g))
#define isin8(x,a,b,c,d,e,f,g,h) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h))
#define isin9(x,a,b,c,d,e,f,g,h,i) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i))
#define isin10(x,a,b,c,d,e,f,g,h,i,j) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i) || (x) == (j))
#define isin11(x,a,b,c,d,e,f,g,h,i,j,k) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i) || (x) == (j) || (x) == (k))
#define isin12(x,a,b,c,d,e,f,g,h,i,j,k,l) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i) || (x) == (j) || (x) == (k) || (x) == (l))
#define isin13(x,a,b,c,d,e,f,g,h,i,j,k,l,m) ((x) == (a) || (x) == (b) || (x) == (c) || (x) == (d) || (x) == (e) || (x) == (f) || (x) == (g) || (x) == (h) || (x) == (i) || (x) == (j) || (x) == (k) || (x) == (l) || (x) == (m))

#define choose4(x,a,b,c,d) ((x) == (a) ? a :\
							(x) == (b) ? b :\
							(x) == (c) ? c :\
							(x) == (d) ? d :\
							0 )

int aligned_size (int size, int alignsize);


#define RECTRB_REPLY(rectrb) &(rectrb).left, &(rectrb).top, &(rectrb).width, &(rectrb).height, &(rectrb).right, &(rectrb).bottom
#define RECT_REPLY(rect) &(rect).left, &(rect).top, &(rect).width, &(rect).height
#define RECT_SEND(rect) (rect).left, (rect).top, (rect).width, (rect).height
#define RECT_SEND_MULT(rect,factor) ((rect).left * factor), ((rect).top * factor), ((rect).width * factor), ((rect).height * factor)

//#define PRECT_SEND(prect) (prect)->left, (prect)->top, (prect)->width, (prect)->height  Haven't had to use this yet
#define RECT_SEND_LTRB(rect) (rect).left, (rect).top, (rect).left + (rect).width - 1, (rect).top + (rect).height - 1

#define RECT_SEND_LRBT(rect) (rect).left, (rect).left + (rect).width - 1, (rect).top + (rect).height - 1, (rect).top


#endif // ! __MATH_GENERAL_H__



