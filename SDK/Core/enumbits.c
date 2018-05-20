/*
Copyright (C) 2011-2014 Baker

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
// enumbits.c -- enumerated lists and bit functions

#define CORE_LOCAL
#include "core.h"
#include "enumbits.h"

///////////////////////////////////////////////////////////////////////////////
//  KEY/VALUE: Baker - Slightly easier to make constants/enumeration tables
///////////////////////////////////////////////////////////////////////////////

/*
typedef struct
{
	const char *keystring;
	int value;
} keyvalue_t;

#define KEYVALUE(x) { #x , x }
*/


const char *KeyValue_GetKeyString (keyvalue_t table[], int val)
{
	keyvalue_t* cur;

	for (cur = &table[0]; cur->keystring != NULL; cur++)
	{
		if (cur->value == val)
			return cur->keystring;
	}

	return NULL;
}

keyvalue_t *KeyValue_GetEntry (keyvalue_t table[], const char* keystring)
{
	int i;

	for (i = 0; table[i].keystring; i++)
	{
		keyvalue_t* entry = &table[i];
		if (strcasecmp(entry->keystring, keystring) == 0 )
			return entry;
	}
	return NULL; // Not found
}

///////////////////////////////////////////////////////////////////////////////
//  CORE: General functions
///////////////////////////////////////////////////////////////////////////////



// Use ^ operator in C
//unsigned c_bitxor (unsigned x, unsigned y)
//{
//    unsigned a = x & y;
//    /unsigned b = ~x & ~y;
//    unsigned z = ~a & ~b;
//    return z;
//}



void c_swapd (double *a, double *b)
{
	double c = *a;
	*a = *b;
	*b = c;
}

void c_swapf (float *a, float *b)
{
	float c = *a;
	*a = *b;
	*b = c;
}

void c_swapptr (void *_a, void *_b)
{
	void **a = _a, **b = _b;
	void *c = *a;
	*a = *b;
	*b = c;
}


void c_swapb (unsigned char *a, unsigned char *b)
{
	unsigned char c = *a;
	*a = *b;
	*b = c;
}

void c_swapi (int *a, int *b)
{
	int c = *a;
	*a = *b;
	*b = c;
}

float byte_to_clampf (byte b)
{
	static float byte_to_float[256];

	if (b == 0)		
		return 0;

	if (byte_to_float[b])
		return b; // Shortcut

	// Beyond this point, save for shortcut and return
	if (b == 255)
		return (byte_to_float[b] = 1); 
	
	// Rounding?
	return (byte_to_float[b] = b / 255.0f); 
}

byte clampf_to_byte (float f)
{
	if (f >= 1)	return 255;
	if (f <= 0)	return 0;

	return (byte)f * 255.0;
}



