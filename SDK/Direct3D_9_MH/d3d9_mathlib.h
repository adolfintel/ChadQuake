/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/


#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

#pragma warning(disable : 4244)     // MIPS ... Baker added

#define DEG2RAD(a) (((a) * M_PI) / 180.0)
#define RAD2DEG(a) (((a) * 180.0) / M_PI)

// common utility funcs
__inline float SafeSqrt (float in)
{
	if (in < 0.00001f)
		return 0;
	else return sqrt (in);
}


__inline float *float2 (float a, float b)
{
	static float f[2];

	f[0] = a;
	f[1] = b;

	return f;
}


__inline float *float3 (float a, float b, float c)
{
	static float f[3];

	f[0] = a;
	f[1] = b;
	f[2] = c;

	return f;
}


__inline float *float4 (float a, float b, float c, float d)
{
	static float f[4];

	f[0] = a;
	f[1] = b;
	f[2] = c;
	f[3] = d;

	return f;
}


// vector functions from directq
__inline void Vector2Madf (float *out, float *vec, float scale, float *add)
{
	out[0] = vec[0] * scale + add[0];
	out[1] = vec[1] * scale + add[1];
}


__inline void Vector2Mad (float *out, float *vec, float *scale, float *add)
{
	out[0] = vec[0] * scale[0] + add[0];
	out[1] = vec[1] * scale[1] + add[1];
}


__inline void Vector3Madf (float *out, float *vec, float scale, float *add)
{
	out[0] = vec[0] * scale + add[0];
	out[1] = vec[1] * scale + add[1];
	out[2] = vec[2] * scale + add[2];
}


__inline void Vector3Mad (float *out, float *vec, float *scale, float *add)
{
	out[0] = vec[0] * scale[0] + add[0];
	out[1] = vec[1] * scale[1] + add[1];
	out[2] = vec[2] * scale[2] + add[2];
}


__inline void Vector4Madf (float *out, float *vec, float scale, float *add)
{
	out[0] = vec[0] * scale + add[0];
	out[1] = vec[1] * scale + add[1];
	out[2] = vec[2] * scale + add[2];
	out[3] = vec[3] * scale + add[3];
}


__inline void Vector4Mad (float *out, float *vec, float *scale, float *add)
{
	out[0] = vec[0] * scale[0] + add[0];
	out[1] = vec[1] * scale[1] + add[1];
	out[2] = vec[2] * scale[2] + add[2];
	out[3] = vec[3] * scale[3] + add[3];
}


__inline void Vector2Scalef (float *dst, float *vec, float scale)
{
	dst[0] = vec[0] * scale;
	dst[1] = vec[1] * scale;
}


__inline void Vector2Scale (float *dst, float *vec, float *scale)
{
	dst[0] = vec[0] * scale[0];
	dst[1] = vec[1] * scale[1];
}


__inline void Vector3Scalef (float *dst, float *vec, float scale)
{
	dst[0] = vec[0] * scale;
	dst[1] = vec[1] * scale;
	dst[2] = vec[2] * scale;
}


__inline void Vector3Scale (float *dst, float *vec, float *scale)
{
	dst[0] = vec[0] * scale[0];
	dst[1] = vec[1] * scale[1];
	dst[2] = vec[2] * scale[2];
}


__inline void Vector4Scalef (float *dst, float *vec, float scale)
{
	dst[0] = vec[0] * scale;
	dst[1] = vec[1] * scale;
	dst[2] = vec[2] * scale;
	dst[3] = vec[3] * scale;
}


__inline void Vector4Scale (float *dst, float *vec, float *scale)
{
	dst[0] = vec[0] * scale[0];
	dst[1] = vec[1] * scale[1];
	dst[2] = vec[2] * scale[2];
	dst[3] = vec[3] * scale[3];
}


__inline void Vector2Recipf (float *dst, float *vec, float scale)
{
	dst[0] = vec[0] / scale;
	dst[1] = vec[1] / scale;
}


__inline void Vector2Recip (float *dst, float *vec, float *scale)
{
	dst[0] = vec[0] / scale[0];
	dst[1] = vec[1] / scale[1];
}


__inline void Vector3Recipf (float *dst, float *vec, float scale)
{
	dst[0] = vec[0] / scale;
	dst[1] = vec[1] / scale;
	dst[2] = vec[2] / scale;
}


__inline void Vector3Recip (float *dst, float *vec, float *scale)
{
	dst[0] = vec[0] / scale[0];
	dst[1] = vec[1] / scale[1];
	dst[2] = vec[2] / scale[2];
}


__inline void Vector4Recipf (float *dst, float *vec, float scale)
{
	dst[0] = vec[0] / scale;
	dst[1] = vec[1] / scale;
	dst[2] = vec[2] / scale;
	dst[3] = vec[3] / scale;
}


__inline void Vector4Recip (float *dst, float *vec, float *scale)
{
	dst[0] = vec[0] / scale[0];
	dst[1] = vec[1] / scale[1];
	dst[2] = vec[2] / scale[2];
	dst[3] = vec[3] / scale[3];
}


__inline void Vector2Copy (float *dst, float *src)
{
	dst[0] = src[0];
	dst[1] = src[1];
}


__inline void Vector3Copy (float *dst, float *src)
{
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
}


__inline void Vector4Copy (float *dst, float *src)
{
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	dst[3] = src[3];
}


__inline void Vector2Addf (float *dst, float *vec1, float add)
{
	dst[0] = vec1[0] + add;
	dst[1] = vec1[1] + add;
}


__inline void Vector2Add (float *dst, float *vec1, float *vec2)
{
	dst[0] = vec1[0] + vec2[0];
	dst[1] = vec1[1] + vec2[1];
}


__inline void Vector2Subtractf (float *dst, float *vec1, float sub)
{
	dst[0] = vec1[0] - sub;
	dst[1] = vec1[1] - sub;
}


__inline void Vector2Subtract (float *dst, float *vec1, float *vec2)
{
	dst[0] = vec1[0] - vec2[0];
	dst[1] = vec1[1] - vec2[1];
}


__inline void Vector3Addf (float *dst, float *vec1, float add)
{
	dst[0] = vec1[0] + add;
	dst[1] = vec1[1] + add;
	dst[2] = vec1[2] + add;
}


__inline void Vector3Add (float *dst, float *vec1, float *vec2)
{
	dst[0] = vec1[0] + vec2[0];
	dst[1] = vec1[1] + vec2[1];
	dst[2] = vec1[2] + vec2[2];
}


__inline void Vector3Subtractf (float *dst, float *vec1, float sub)
{
	dst[0] = vec1[0] - sub;
	dst[1] = vec1[1] - sub;
	dst[2] = vec1[2] - sub;
}


__inline void Vector3Subtract (float *dst, float *vec1, float *vec2)
{
	dst[0] = vec1[0] - vec2[0];
	dst[1] = vec1[1] - vec2[1];
	dst[2] = vec1[2] - vec2[2];
}


__inline void Vector4Addf (float *dst, float *vec1, float add)
{
	dst[0] = vec1[0] + add;
	dst[1] = vec1[1] + add;
	dst[2] = vec1[2] + add;
	dst[3] = vec1[3] + add;
}


__inline void Vector4Add (float *dst, float *vec1, float *vec2)
{
	dst[0] = vec1[0] + vec2[0];
	dst[1] = vec1[1] + vec2[1];
	dst[2] = vec1[2] + vec2[2];
	dst[3] = vec1[3] + vec2[3];
}


__inline void Vector4Subtractf (float *dst, float *vec1, float sub)
{
	dst[0] = vec1[0] - sub;
	dst[1] = vec1[1] - sub;
	dst[2] = vec1[2] - sub;
	dst[3] = vec1[3] - sub;
}


__inline void Vector4Subtract (float *dst, float *vec1, float *vec2)
{
	dst[0] = vec1[0] - vec2[0];
	dst[1] = vec1[1] - vec2[1];
	dst[2] = vec1[2] - vec2[2];
	dst[3] = vec1[3] - vec2[3];
}


__inline float Vector2Dot (const float *x, const float *y)
{
	return ((double) x[0] * (double) y[0]) + ((double) x[1] * (double) y[1]);
}


__inline float Vector3Dot (const float *x, const float *y)
{
	return ((double) x[0] * (double) y[0]) + ((double) x[1] * (double) y[1]) + ((double) x[2] * (double) y[2]);
}


__inline float Vector4Dot (const float *x, const float *y)
{
	return ((double) x[0] * (double) y[0]) + ((double) x[1] * (double) y[1]) + ((double) x[2] * (double) y[2]) + ((double) x[3] * (double) y[3]);
}


__inline void Vector2Lerpf (float *dst, float *l1, float *l2, float b)
{
	dst[0] = l1[0] + (l2[0] - l1[0]) * b;
	dst[1] = l1[1] + (l2[1] - l1[1]) * b;
}


__inline void Vector3Lerpf (float *dst, float *l1, float *l2, float b)
{
	dst[0] = l1[0] + (l2[0] - l1[0]) * b;
	dst[1] = l1[1] + (l2[1] - l1[1]) * b;
	dst[2] = l1[2] + (l2[2] - l1[2]) * b;
}


__inline void Vector4Lerpf (float *dst, float *l1, float *l2, float b)
{
	dst[0] = l1[0] + (l2[0] - l1[0]) * b;
	dst[1] = l1[1] + (l2[1] - l1[1]) * b;
	dst[2] = l1[2] + (l2[2] - l1[2]) * b;
	dst[3] = l1[3] + (l2[3] - l1[3]) * b;
}


__inline void Vector2Lerp (float *dst, float *l1, float *l2, float *b)
{
	dst[0] = l1[0] + (l2[0] - l1[0]) * b[0];
	dst[1] = l1[1] + (l2[1] - l1[1]) * b[1];
}


__inline void Vector3Lerp (float *dst, float *l1, float *l2, float *b)
{
	dst[0] = l1[0] + (l2[0] - l1[0]) * b[0];
	dst[1] = l1[1] + (l2[1] - l1[1]) * b[1];
	dst[2] = l1[2] + (l2[2] - l1[2]) * b[2];
}


__inline void Vector4Lerp (float *dst, float *l1, float *l2, float *b)
{
	dst[0] = l1[0] + (l2[0] - l1[0]) * b[0];
	dst[1] = l1[1] + (l2[1] - l1[1]) * b[1];
	dst[2] = l1[2] + (l2[2] - l1[2]) * b[2];
	dst[3] = l1[3] + (l2[3] - l1[3]) * b[3];
}


__inline void Vector2Set (float *vec, float x, float y)
{
	vec[0] = x;
	vec[1] = y;
}


__inline void Vector3Set (float *vec, float x, float y, float z)
{
	vec[0] = x;
	vec[1] = y;
	vec[2] = z;
}


__inline void Vector4Set (float *vec, float x, float y, float z, float w)
{
	vec[0] = x;
	vec[1] = y;
	vec[2] = z;
	vec[3] = w;
}


__inline void Vector2Clear (float *vec)
{
	vec[0] = vec[1] = 0.0f;
}


__inline void Vector3Clear (float *vec)
{
	vec[0] = vec[1] = vec[2] = 0.0f;
}


__inline void Vector4Clear (float *vec)
{
	vec[0] = vec[1] = vec[2] = vec[3] = 0.0f;
}


__inline void Vector2Clamp (float *vec, float clmp)
{
	if (vec[0] > clmp) vec[0] = clmp;
	if (vec[1] > clmp) vec[1] = clmp;
}


__inline void Vector3Clamp (float *vec, float clmp)
{
	if (vec[0] > clmp) vec[0] = clmp;
	if (vec[1] > clmp) vec[1] = clmp;
	if (vec[2] > clmp) vec[2] = clmp;
}


__inline void Vector4Clamp (float *vec, float clmp)
{
	if (vec[0] > clmp) vec[0] = clmp;
	if (vec[1] > clmp) vec[1] = clmp;
	if (vec[2] > clmp) vec[2] = clmp;
	if (vec[3] > clmp) vec[3] = clmp;
}


__inline void Vector2Cross (float *cross, float *v1, float *v2)
{
	// System_Error ("Just what do you think you're doing, Dave?");
}


__inline void Vector3Cross (float *cross, float *v1, float *v2)
{
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}


__inline void Vector4Cross (float *cross, float *v1, float *v2)
{
	// System_Error ("Just what do you think you're doing, Dave?");
}


__inline float Vector2Length (float *v)
{
	return SafeSqrt (Vector2Dot (v, v));
}


__inline float Vector3Length (float *v)
{
	return SafeSqrt (Vector3Dot (v, v));
}


__inline float Vector4Length (float *v)
{
	return SafeSqrt (Vector4Dot (v, v));
}


__inline float Vector2Normalize (float *v)
{
	float length = Vector2Dot (v, v);

	if ((length = SafeSqrt (length)) > 0)
	{
		float ilength = 1 / length;

		v[0] *= ilength;
		v[1] *= ilength;
	}

	return length;
}


__inline float Vector3Normalize (float *v)
{
	float length = Vector3Dot (v, v);

	if ((length = SafeSqrt (length)) > 0)
	{
		float ilength = 1 / length;

		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;
}


__inline float Vector4Normalize (float *v)
{
	float length = Vector4Dot (v, v);

	if ((length = SafeSqrt (length)) > 0)
	{
		float ilength = 1 / length;

		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
		v[3] *= ilength;
	}

	return length;
}


__inline int Vector2Compare (float *v1, float *v2)
{
	if (v1[0] != v2[0]) return 0;
	if (v1[1] != v2[1]) return 0;

	return 1;
}


__inline int Vector3Compare (float *v1, float *v2)
{
	if (v1[0] != v2[0]) return 0;
	if (v1[1] != v2[1]) return 0;
	if (v1[2] != v2[2]) return 0;

	return 1;
}


__inline int Vector4Compare (float *v1, float *v2)
{
	if (v1[0] != v2[0]) return 0;
	if (v1[1] != v2[1]) return 0;
	if (v1[2] != v2[2]) return 0;
	if (v1[3] != v2[3]) return 0;

	return 1;
}


