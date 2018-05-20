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
// math_general.c

#include "core.h"
#include "math_general.h" // Courtesy include

// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static unsigned int sNextPowerOfTwo (unsigned int v) // compute the next highest power of 2 of 32-bit v
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}


cbool is_pow2 (unsigned int x)
{
  return ((x != 0) && ((x & (~x + 1)) == x));
}


int random_int (int low, int high)
{
	int range = high - low + 1;
	int rando = rand() % range + low;
	return rando;
	//float f = RANDOM_FLOAT; // 0 to 1.

}


unsigned int roundup_pow2 (unsigned int x)
{
	if (is_pow2(x))
		return x;

	return sNextPowerOfTwo(x);
}

// ch is a char variable holding a hexadecimal digit, some systems have digittoint
int hex_char_to_int (char ch)
{
	if (!isxdigit(ch)) { log_fatal ("%c is not a valid hex digit", ch); return 0; }
	
	// is decimal ? decimal  : lowercase hex eval
	return ( isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10  );
}


//Change from integral to step size like 1/16 make it a float.  0 means none.
float Math_Largest_Multiple_For_Screen (float grainsize, cbool rotated_90, int wanted_width_, int wanted_height_, int client_width, int client_height, reply int *offset_x_out, reply int *offset_y_out, reply int *new_width_out, reply int *new_height_out)
{
	int wanted_width			=  !rotated_90 ? wanted_width_  :wanted_height_;
	int wanted_height			=  !rotated_90 ? wanted_height_ : wanted_width_;
	float factor_x				=  (float)client_width  / wanted_width;
	float factor_y				=  (float)client_height / wanted_height;
	const float _final_factor	=  c_min (factor_x, factor_y);
	const int count				= grainsize ? _final_factor / grainsize : 0;
	const float final_factor	= grainsize ? count * grainsize : _final_factor;
	
	int new_width, new_height, offset_x, offset_y;

	if (!final_factor)
		log_fatal ("Largest_Multiple_For_Screen fail.  Available is %d x %d.  Wanted is %d x %d", client_width, client_height, wanted_width, wanted_height);

	new_width = ceil(wanted_width * final_factor); // This needs ceil rounded
	new_height = ceil(wanted_height * final_factor); // This needs ceil rounded

	offset_x = ((client_width  - new_width)  / 2.0);  //crint
	offset_y = ((client_height - new_height)  / 2.0);

	NOT_MISSING_ASSIGN (offset_x_out, offset_x);
	NOT_MISSING_ASSIGN (offset_y_out, offset_y);
	NOT_MISSING_ASSIGN (new_width_out, new_width);
	NOT_MISSING_ASSIGN (new_height_out, new_height);

	return final_factor;
}

//
// Alternators.  These need thorough fucking documentation.
//

// This calculates how far we are into a cycle.
// For example.  If the cycle period is 5 seconds.
// 1 second = 20% or 0.20
// 4 second = 80% or 0.20

static double Cycle_Fraction (double clocksecs, double cycle_period_seconds)
{
	if (cycle_period_seconds)
	{
		//		unsigned in_cycle_base = cyclems ? host.time_milliseconds % cyclems : 0; // Avoid divide by 0 potential
		//		unsigned in_cycle = host.time_milliseconds - in_cycle_base;
		int in_cycle = (int)(clocksecs * 1000) % (int)(cycle_period_seconds * 1000); // Avoid divide by 0 potential
		double in_cycle_frac = (double)in_cycle / (double)cycle_period_seconds;
		return (in_cycle_frac / 1000.0f);
	}

	return 0.0f;
}

// The return value circles the center.  It orbits like a circle so it has a bounce-like behavior.
// Think of Quake weapon bobbing.
double Cycle_Circular_In_X_Seconds (double raw_clocksecs, double cycle_in_x_seconds, double low, double high)
{
	double frac_0_to_1 = Cycle_Fraction (raw_clocksecs, cycle_in_x_seconds); // Will go from 0 to 1 in X seconds.

	double frac_angle_mod	= sin(DEGREES_TO_RADIANS(frac_0_to_1 * 360) ); // Will vary from -1 to 1.
	double cur_value		= (frac_angle_mod + 1) / 2; // Converts -1 to 1 range to 0 to 1 range.

	double range = high - low;
	return low +  cur_value * range;
}


double angle_maybe_wrap (double angle, reply cbool *did_wrap_out)
{
	// - 90
	// 270
	// 721
	// -370 --> -10 --> 350

	if (angle >= 360) {
		double wrapped_angle = fmod(angle, 360.0);
//		logd ("Wrapped angle %g to %g", angle, wrapped_angle);

		NOT_MISSING_ASSIGN(did_wrap_out, true);
		return wrapped_angle;
	}


	// -1 becomes 359.  So does -361 or -721
	if (angle < 0) {
		double wrapped_angle = 360 - fmod(-angle, 360.0);
		if (wrapped_angle >= 360)
			wrapped_angle = 0;
//		logd ("Wrapped angle %g to %g", angle, wrapped_angle);
		NOT_MISSING_ASSIGN(did_wrap_out, true);
		return wrapped_angle;
	}

	return angle; // No effect.  Do not set angle wrap flag.
}

cbool range_extend_d (double value, required double *lo, required double *hi)
{
	// Special condition where hi is 0 and lo is greater than hi means uninitialized lo and hi
	if (*hi == 0 && *lo > *hi ) {
		*hi = value, *lo = value;
		return true;
	}
	else {
		cbool changed = false;

		if (value < *lo) *lo = value, changed = true;  // Considitional
		if (value > *hi) *hi = value, changed = true;
		return changed;
	}
}

cbool range_extend_f (float value, required float *lo, required float *hi)
{
	// Special condition where hi is 0 and lo is greater than hi means uninitialized lo and hi
	if (*hi == 0 && *lo > *hi ) {
		*hi = value, *lo = value;
		return true;
	}
	else {
		cbool changed = false;

		if (value < *lo) *lo = value, changed = true;  // Considitional
		if (value > *hi) *hi = value, changed = true;
		return changed;
	}
}

int aligned_size (int size, int alignsize)
{
	int overage = size % alignsize;
	int out = size + (overage ? alignsize - overage : 0);
	return out;
}

int math_sign (int x)
{
	if (!x) return 0;
	if (x > 0) return 1;
	return -1;
}
