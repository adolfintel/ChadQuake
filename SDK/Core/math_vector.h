/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
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

#ifndef __MATH_VECTOR_H__
#define __MATH_VECTOR_H__

// mathlib.h
//#include "environment.h"
//#include <math.h>


enum q_pitch_e
{
	Q_PITCH = 0, // up / down
	Q_YAW   = 1, // left / right
	Q_ROLL  = 2, // fall over
};

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];



#define M_PI_DIV_180 (M_PI / 180.0)
#define Degree_To_Radians(a) ((a) * M_PI) / 180.0F
#define Degree_To_RadiansEx(a) ( (a) * M_PI_DIV_180 )




void PerpendicularVector( vec3_t dst, const vec3_t src );
void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal );

#define DotProduct(x,y) ((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define VectorSubtract(a,b,c) {(c)[0]=(a)[0]-(b)[0];(c)[1]=(a)[1]-(b)[1];(c)[2]=(a)[2]-(b)[2];}
#define VectorAdd(a,b,c) {(c)[0]=(a)[0]+(b)[0];(c)[1]=(a)[1]+(b)[1];(c)[2]=(a)[2]+(b)[2];}
#define VectorCopy(a,b) {(b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2];}
#define VectorClear(a)		((a)[0] = (a)[1] = (a)[2] = 0)
#define VectorNegate(a, b)	((b)[0] = -(a)[0], (b)[1] = -(a)[1], (b)[2] = -(a)[2])

//johnfitz -- courtesy of lordhavoc
// QuakeSpasm: To avoid strict aliasing violations, use a float/int union instead of type punning.
#define VectorNormalizeFast(_v)\
{\
	union { float f; int i; } _y, _number;\
	_number.f = DotProduct(_v, _v);\
	if (_number.f != 0.0)\
	{\
		_y.i = 0x5f3759df - (_number.i >> 1);\
		_y.f = _y.f * (1.5f - (_number.f * 0.5f * _y.f * _y.f));\
		VectorScale(_v, _y.f, _v);\
	}\
}

void TurnVector (vec3_t out, const vec3_t forward, const vec3_t side, float angle); //johnfitz (MISSING)
void VectorAngles (const vec3_t forward, vec3_t angles); //johnfitz

void VectorMA (const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc);

float _DotProduct (const vec3_t v1, const vec3_t v2);
void _VectorSubtract (const vec3_t veca, const vec3_t vecb, vec3_t out);
void _VectorAdd (const vec3_t veca, const vec3_t vecb, vec3_t out);
void _VectorCopy (const vec3_t in, vec3_t out);

cbool VectorCompare (const vec3_t v1, const vec3_t v2); // Return true on match
float VectorLength (const vec3_t v);
//void CrossProduct (const vec3_t v1, const vec3_t v2, vec3_t cross);
void VectorCrossProduct (const vec3_t v1, const vec3_t v2, vec3_t cross); // returns cross
float VectorNormalize (vec3_t v); // returns vector length, changes v
void VectorInverse (vec3_t v);
void VectorScale (const vec3_t in, const vec_t scale, vec3_t out);
void AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up); // writes out forward, right, up
void VectorToAngles (const vec3_t vec, vec3_t ang);

#define VectorSet(v, x, y, z) ((v)[0] = (x), (v)[1] = (y), (v)[2] = (z))
#define VectorAddFloat(a, b, c)				((c)[0] = (a)[0] + (b), (c)[1] = (a)[1] + (b), (c)[2] = (a)[2] + (b))

float DistanceBetween2Points (const vec3_t v1, const vec3_t v2);

void VectorAverage (const vec3_t v1, const vec3_t v2, vec3_t out);
void VectorExtendLimits (const vec3_t newvalue, vec3_t minlimit, vec3_t maxlimit); // Changes minlimit and maxlimit


int ParseFloats(const char *s, float *f, int *f_size);
cbool PointInCube(vec3_t point, vec3_t cube_mins, vec3_t cube_maxs);

// Rogues .. VectorSet4 is usually to define a color
#define VectorSet4(dest, x, y, z, w)			((dest)[0] = (x), (dest)[1] = (y), (dest)[2] = (z), (dest)[3] = (w))
#define VectorsMatch4(a, b)						((a)[0] == (b)[0] && (a)[1] == (b)[1] && (a)[2] == (b)[2] && (a)[3] == (b)[3])
#define VectorsDiffer2(a, b)					((a)[0] != (b)[0] || (a)[1] != (b)[1])
#define VectorCopy2(source, dest)				((dest)[0] = (source)[0], (dest)[1] = (source)[1])
#define VectorCopy4(source, dest)				((dest)[0] = (source)[0], (dest)[1] = (source)[1], (dest)[2] = (source)[2], (dest)[3] = (source)[3] )

// OpenGL oriented so float is ok.

// For these kind of unions, I think it is important to have the struct part first.
// Because arrays cannot be assigned by equal statements, but a struct can be!
typedef struct
{
	union
	{
		struct
		{
			float pitch;
			float yaw;
			float roll;
		};
		vec3_t vec3;
	};
} orientation3d;

typedef struct
{
	union
	{
		struct
		{
			float x;
			float y;
			float z;
		};
		vec3_t vec3;
	};
} position3d;//, Direction3D;

typedef struct
{
	union
	{
		struct
		{
			float width;
			float depth;
			float height;
		};
		vec3_t vec3;
	};
} size3d;

typedef struct {
	union
	{
		struct
		{
			float x;
			float y;
			float z;
		};
		vec3_t position3;
	};
	union
	{
		struct
		{
			float pitch;	// Right vector is immune to pitch
			float roll;		// Forward vector is immune to roll
			float yaw;		// Up vector is immune to yaw
		};
		vec3_t orientation3;

	};
	union
	{
		struct
		{
			float width;
			float depth;
			float height;
		};
		vec3_t size3;
	};
} location3d;

void ZAngleVectorsFast (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
void ZAngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
void ZAngleVectorsGL (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up); // Gosh I'm a fuck up.

double _cosd(double degrees);
double _sind(double degrees);
void cossinf (float angle, float *mycos, float *mysin);

char *_angles4(const vec3_t angles, const vec3_t forward, const vec3_t right, const vec3_t up);
char *_angles (const vec3_t a);

void vectoangles (const vec3_t vec, vec3_t ang);

#endif	// ! __MATH_VECTOR_H__



