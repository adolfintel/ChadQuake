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
// enumbits.h -- enumerated lists and bit functions


#ifndef __ENUMBITS_H__
#define __ENUMBITS_H__

#include "environment.h"

#define INT32_BITCOUNT_32 32

///////////////////////////////////////////////////////////////////////////////
//  CORE: General functions
///////////////////////////////////////////////////////////////////////////////

//Note: This expression can xor a single bit: number ^= 1 << x;

//unsigned c_bitxor (unsigned x, unsigned y);  // Use ^ operator in C
void c_swapf (float* a, float* b);
void c_swapi (int *a, int *b);
void c_swapb (unsigned char *a, unsigned char *b);
void c_swapd (double *a, double *b);
void c_swapptr (void *a, void *b);

byte clampf_to_byte (float f);
float byte_to_clampf (byte b);


///////////////////////////////////////////////////////////////////////////////
//  KEY/VALUE: Baker - Slightly easier to make constants/enumeration tables
///////////////////////////////////////////////////////////////////////////////

// This is here because it relates to enums
typedef struct
{
	const char *keystring;
	int value;
} keyvalue_t;

#define KEYVALUE(x) { #x , x }


const char *KeyValue_GetKeyString (keyvalue_t table[], int val); // Returns null if not found
keyvalue_t *KeyValue_GetEntry (keyvalue_t table[], const char *keystring); // Returns null if not found

///////////////////////////////////////////////////////////////////////////////
//  BITS:  Easier access and storage of bits into bytes (8 bits obviously)
///////////////////////////////////////////////////////////////////////////////

//#define BITS_SIZE_32(_numbits)				( (_numbits + 7 )   >> 5 ) 
//#define BITS_TEST_32(_bitarray, _bitnum)	( _bitarray[_bitnum >> 5] &  (1U << (_bitnum & 31)) )
//#define BITS_SET_32(_bitarray, _bitnum)		( _bitarray[_bitnum >> 5] != (1U << (_bitnum & 31)) )
//#define BITS_CLEAR_32(_bitarray, _bitnum)	( _bitarray[_bitnum >> 5] &= ~(byte)(1U << (_bitnum & 31)) )

//#define BITS_SIZE(_numbits)					( (_numbits + 7 )   >> 3 ) 
//#define BITS_TEST(_bitarray, _bitnum)		( _bitarray[_bitnum >> 3] &  (1U << (_bitnum & 7)) )
//#define BITS_SET(_bitarray, _bitnum)		( _bitarray[_bitnum >> 3] != (1U << (_bitnum & 7)) )
//#define BITS_CLEAR(_bitarray, _bitnum)		( _bitarray[_bitnum >> 3] &= ~(byte)(1U << (_bitnum & 7)) )

#define BYTE_BITS_8
#define BYTES_FOR_NUMBER_OF_BITS(NumberOfBits)	( (NumberOfBits + 7) / 8) 
#define VFLAGS_CHECK(ByteArrayName, BitNumber)	( ByteArrayName[(BitNumber) / 8] &   (byte)(1U << ((BitNumber) & 7)) )
#define VFLAGS_SET(ByteArrayName, BitNumber)	( ByteArrayName[(BitNumber) / 8] |=  (byte)(1U << ((BitNumber) & 7)) )
#define VFLAGS_UNSET(ByteArrayName, BitNumber)	( ByteArrayName[(BitNumber) / 8] &= ~(byte)(1U << ((BitNumber) & 7)) )


// 25 flags example
// byte my_flags_container[BITS_SIZE(25)];	// should be 4
// if (BITS_TEST (my_flags_container, 6))	// should see if bit #6 is set
// BITS_SET (my_flags_container, 6)			// should set the bit
// BITS_REMOVE (my_flags_container, 6)		// should remove

/*
keyvalue_t input_state_text [] =
{
	KEYVALUE (input_none),
	KEYVALUE (input_have_keyboard),
	KEYVALUE (input_have_mouse_keyboard),
NULL, 0 };  // NULL termination
*/

//void Bits_Print (byte *, size_t len, keyvalue_t table[]); // Never written as far as I can tell
//void Bits_Print32 (byte *, size_t len, keyvalue_t table[]);

#define Flag_Add(_x, _flag) ((_x) | ((_flag) )) // 
#define Flag_Remove(_x, _flag) ((_x) & (~ (_flag) )) // 
#define Flag_Check(_x, _flag)  (!!((_x) & (_flag))) // 
#define Flag_Check_Strict(_x, _flag)  (((_x) & (_flag)) == _flag) // 


#endif	// ! __ENUMBITS_H__


