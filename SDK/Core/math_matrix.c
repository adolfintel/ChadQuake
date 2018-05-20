/*
Copyright (C) 2010-2012 MH
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
// math_matrix.c

#include "core.h"
#include "math_matrix.h"

#ifdef PLATFORM_WINDOWS
//#include "core_opengl.h" // Justify this please.
//#define GLM_TESTING
#endif

#include <math.h>
#include <stdlib.h> // malloc, etc.
#include <string.h> // memcpy, etc.

char *Mat4_String (glmatrix *m)
{
	return va (NEWLINE "[%.9g, %.9g, %.9g, %.9g]" NEWLINE "[%.9g, %.9g, %.9g, %.9g]" NEWLINE "[%.9g, %.9g, %.9g, %.9g]" NEWLINE "[%.9g, %.9g, %.9g, %.9g]" NEWLINE,
		m->_11, m->_12, m->_13, m->_14,
		m->_21, m->_22, m->_23, m->_24,
		m->_31, m->_32, m->_33, m->_34,
		m->_41, m->_42, m->_43, m->_44);

}

cbool Mat4_Compare (glmatrix *a, glmatrix *b)
{
	cbool ret = true;
	int n; for (n = 0; n < 16; n ++) {
		if (a->m16[n] != b->m16[n]) {
#ifdef _DEBUG
			alert ("%d us %f GL %f delta %f" NEWLINE "MATRIX A:" NEWLINE "%s" NEWLINE "MATRIX B" NEWLINE " %s", n, a->m16[n],  b->m16[n], a->m16[n] - b->m16[n], Mat4_String(a), Mat4_String(b));
#endif
			ret = false;
		}
	}
	return false;
}

void CompareFloat16 (float *a, float *b)
{
	int n; for (n = 0; n < 16; n ++) {
		if (a[n] != b[n])
			alert ("%d us %f GL %f delta %f" , n, a[n],  b[n], a[n] - b[n]);
	}
}

/*
============================================================================================================

	MATRIX OPS  (Baker: From MH)

	These happen in pace on the matrix and update it's current values

	These are D3D style matrix functions; sorry OpenGL-lovers but they're more sensible, usable
	and intuitive this way...

============================================================================================================
*/

float VectorNormalize3f (float *x, float *y, float *z)
{
	float	length, ilength;

	length = x[0] * x[0] + y[0] * y[0] + z[0] * z[0];
	length = sqrt (length);		// FIXME

	if (length)
	{
		ilength = 1 / length;

		x[0] *= ilength;
		y[0] *= ilength;
		z[0] *= ilength;
	}

	return length;
}


glmatrix *Mat4_Copy (glmatrix *dst, const glmatrix *src)
{
	memcpy (dst, src, sizeof (glmatrix));

	return dst;
}


glmatrix *Mat4_Identity_Set (glmatrix *m)
{
	m->m16[0] = m->m16[5] = m->m16[10] = m->m16[15] = 1;
	m->m16[1] = m->m16[2] = m->m16[3] = m->m16[4] = m->m16[6] = m->m16[7] = m->m16[8] = m->m16[9] = m->m16[11] = m->m16[12] = m->m16[13] = m->m16[14] = 0;

	return m;
}


glmatrix *Mat4_Multiply (glmatrix *out, const glmatrix *m1, const glmatrix *m2)
{
	int i, j;
	glmatrix tmp;

	// do it this way because either of m1 or m2 might be the same as out...
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			tmp.m4x4[i][j] = m1->m4x4[i][0] * m2->m4x4[0][j] +
							 m1->m4x4[i][1] * m2->m4x4[1][j] +
							 m1->m4x4[i][2] * m2->m4x4[2][j] +
							 m1->m4x4[i][3] * m2->m4x4[3][j];
		}
	}

	memcpy (out, &tmp, sizeof (glmatrix));

	return out;
}


glmatrix *Mat4_Translate (glmatrix *m, float x, float y, float z)
{
	glmatrix tmp;
	Mat4_Identity_Set (&tmp);

	tmp.m16[12] = x;
	tmp.m16[13] = y;
	tmp.m16[14] = z;

	Mat4_Multiply (m, &tmp, m);

	return m;
}


glmatrix *Mat4_Scale (glmatrix *m, float x, float y, float z)
{
	glmatrix tmp;
	Mat4_Identity_Set (&tmp);

	tmp.m16[0] = x;
	tmp.m16[5] = y;
	tmp.m16[10] = z;

	Mat4_Multiply (m, &tmp, m);

	return m;
}


glmatrix *Mat4_Rotate (glmatrix *m, float a, float x, float y, float z)
{
	// i prefer spaces around my operators because it makes stuff like a = b * -c clearer and easier on the eye. ;)
	glmatrix tmp;
	float c = cos (a * M_PI / 180.0);
	float s = sin (a * M_PI / 180.0);

	// http://www.opengl.org/sdk/docs/man/xhtml/glRotate.xml
	// this should normalize the vector before rotating
	VectorNormalize3f (&x, &y, &z);

	tmp.m16[0] = x * x * (1 - c) + c;
	tmp.m16[4] = x * y * (1 - c) - z * s;
	tmp.m16[8] = x * z * (1 - c) + y * s;
	tmp.m16[12] = 0;

	tmp.m16[1] = y * x * (1 - c) + z * s;
	tmp.m16[5] = y * y * (1 - c) + c;
	tmp.m16[9] = y * z * (1 - c) - x * s;
	tmp.m16[13] = 0;

	tmp.m16[2] = x * z * (1 - c) - y * s;
	tmp.m16[6] = y * z * (1 - c) + x * s;
	tmp.m16[10] = z * z * (1 - c) + c;
	tmp.m16[14] = 0;

	tmp.m16[3] = 0;
	tmp.m16[7] = 0;
	tmp.m16[11] = 0;
	tmp.m16[15] = 1;

	Mat4_Multiply (m, &tmp, m);

	return m;
}


// glFrustum
glmatrix *Mat4_Frustum (glmatrix *m, double left, double right, double bottom, double top, double znear, double zfar)
{
    // from OpenGL spec PDF:                                 can be rewritten as:
    // / 2/(r-l)     0        0       -(r+l)/(r-l)  \        / 2/(r-l)     0        0      (r+l)/(l-r)  \
    // |   0      2/(t-b)     0       -(t+b)/(t-b)  |   =>   |   0      2/(t-b)     0      (t+b)/(b-t)  |
    // |   0         0     -2/(f-n)   -(f+n)/(f-n)  |        |   0         0     2/(n-f)   (f+n)/(n-f)  |
    // \   0         0        0             1       /        \   0         0        0           1       /
    // invalid for: l=r, b=t, or n=f
	glmatrix tmp;

	c_assert (left != right); //,	"left = right");
	c_assert (bottom != top); //,  "bottom = top");
	c_assert (znear != zfar); //,  "znear = zfar");

	c_assert (left != -right); //,	"left = right");
	c_assert (bottom != -top); //,  "bottom = top");
	c_assert (znear != -zfar); //,  "znear = zfar");


	tmp.m16[0] = (2 * znear)/(right - left);
	tmp.m16[1] 	= 0;
	tmp.m16[2] 	= 0;
	tmp.m16[3] 	= 0;

	tmp.m16[4] 	= 0;
	tmp.m16[5] 	= (2 * znear)/(top - bottom);
	tmp.m16[6] 	= 0;
	tmp.m16[7] 	= 0;

	tmp.m16[8]	= (right + left)/(right - left);
	tmp.m16[9] 	= (top + bottom)/(top - bottom);
	tmp.m16[10] = -(zfar + znear)/(zfar - znear);
	tmp.m16[11] = -1;

	tmp.m16[12] = 0;
	tmp.m16[13] = 0;
	tmp.m16[14] = (-2  * zfar * znear) / (zfar - znear);
	tmp.m16[15] = 0;

	Mat4_Multiply (m, &tmp, m);

#ifdef GLM_TESTING
	{
		float check[16];
		glMatrixMode (GL_PROJECTION);
		glLoadMatrixf (m->m16);
		glFrustum (left, right, bottom, top, znear, zfar);
		glGetFloatv (GL_PROJECTION_MATRIX, check);

		CompareFloat16 (m->m16, check);
	}
#endif
	return m;
}


glmatrix *Mat4_Ortho3D (glmatrix *m, double width, double height)
{
	const double fov = 90.0;
	const double znear = 1; // 1 - 1024; // 1;
	const double zfar  = 32769; //2049; // 1 + 1024; //.5 // 2049; // 1024 spread is pow2 friendly

	double size			= znear * tan(DEGREES_TO_RADIANS(fov) / 2.0);
	double top_plane	= -size / (width / height); // We end up with a 1/1 aspect ratio.
//	double top_plane	= -size / (height/ width); // We end up with a 1/1 aspect ratio.  Still wrong with this
//	double top_plane	= -size;// / (height/ width); // We end up with a 1/1 aspect ratio.

//	Mat4_Identity_Set (m);
	Mat4_Frustum (m, -size + 0.000005, size, -top_plane + 0.000005 /*bottom*/, top_plane /*top*/, znear, zfar);
	Mat4_Translate (m, -width/2, -height/2, -width/2);
	return m;
}

glmatrix *Mat4_OrthoZero (glmatrix *m, float width, float height)
{
	return Mat4_Ortho (m, -width/2, width/2, height/2, -height/2, -4196, 4196);
	//return Mat4_Ortho(m, 0, width, height, 0, -512, 512); // znear zfar best values?
}

#define nudge 0//-0.5 // Because center of pixel is at 0.
glmatrix *Mat4_OrthoAspect (glmatrix *m, float width, float height)
{
	//Mat4_Ortho				(f->ortho, -0.5, width - 0.5, height - 0.5, -0.5, -4196, 4196); // znear zfar best values?

	return Mat4_Ortho(m, 0+nudge, width +nudge, height+nudge, 0+nudge, -4196, 4196); // znear zfar best values?
}

glmatrix *Mat4_OrthoAspectRect (glmatrix *m, float left, float top, float width, float height)
{
	float right = left + width, bottom = top + height;
	// This is wrong for non-zero left and top.  Why?
	return Mat4_Ortho(m, left+nudge, right +nudge, bottom+nudge, top+nudge, -4196, 4196); // znear zfar best values
}


glmatrix *Mat4_Ortho (glmatrix *m, double left, double right, double bottom, double top, double znear, double zfar)
{
	glmatrix tmp;
	float rml = right - left;
	float fmn = zfar - znear;
	float tmb = top - bottom;
	float _1over_rml, _1over_fmn, _1over_tmb;

	if (rml == 0.0 || fmn == 0.0 || tmb == 0.0)
	{
		// Divide by 0 error condition
		return NULL;
	}

	_1over_rml = 1.0f / rml;
	_1over_fmn = 1.0f / fmn;
	_1over_tmb = 1.0f / tmb;

	tmp.m16[0] = 2.0f * _1over_rml;
	tmp.m16[1] = 0.0f;
	tmp.m16[2] = 0.0f;
	tmp.m16[3] = 0.0f;

	tmp.m16[4] = 0.0f;
	tmp.m16[5] = 2.0f * _1over_tmb;
	tmp.m16[6] = 0.0f;
	tmp.m16[7] = 0.0f;

	tmp.m16[8] = 0.0f;
	tmp.m16[9] = 0.0f;
	tmp.m16[10] = -2.0f * _1over_fmn;
	tmp.m16[11] = 0.0f;

	tmp.m16[12] = -(right + left) *  _1over_rml;
	tmp.m16[13] = -(top + bottom) *  _1over_tmb;
	tmp.m16[14] = -(zfar + znear) * _1over_fmn;
	tmp.m16[15] = 1.0f;

	Mat4_Multiply (m, &tmp, m);

#ifdef GLM_TESTING
	{
		float check[16];
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity ();
		glOrtho (left, right, bottom, top, znear, zfar);
		glGetFloatv (GL_PROJECTION_MATRIX, check);

		CompareFloat16 (m->m16, check);
	}
#endif

	return m;
}


//fovy   - Specifies the field of view angle, in degrees, in the y direction.
//aspect - Specifies the aspect ratio that determines the field of view in the x direction.
//		 The aspect ratio is the ratio of x (width) to y (height).
//zNear  - Specifies the distance from the viewer to the near clipping plane (always positive).
//zFar   - Specifies the distance from the viewer to the far clipping plane (always positive).

// Feed it an identity matrix if you want a raw one
glmatrix *Mat4_Perspective (glmatrix *m, float fovy, float aspect, float zNear, float zFar)
{
    glmatrix tmp;

    //float radians = fovy / 2 * M_PI_DIV_180; // * __glPi / 180;
	float radians = fovy / 2 * M_PI / 180;
	float deltaZ = zFar - zNear;
	float sine = sin(radians);
	float cotangent = sine ? cos(radians) / sine : 0;

    if ((deltaZ == 0) || (sine == 0) || (aspect == 0)) {
		logd (SPRINTSFUNC "deltaz or sine or aspect = 0", __func__); // Serious?  How serious of a problem?
		return m; // Error condition
	}

	Mat4_Identity_Set (&tmp);
    tmp.m4x4[0][0] = cotangent / aspect;
    tmp.m4x4[1][1] = cotangent;
    tmp.m4x4[2][2] = -(zFar + zNear) / deltaZ;
    tmp.m4x4[2][3] = -1;
    tmp.m4x4[3][2] = -2 * zNear * zFar / deltaZ;
    tmp.m4x4[3][3] = 0;

	Mat4_Multiply (m, &tmp, m);  //xglMultMatrixf(&tmp[0][0]);
	return m;
}

// Feed it an identity matrix if you want a raw one
glmatrix *Mat4_LookAt (glmatrix *m, float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz)
{
	glmatrix tmp;
	float x[3], y[3], z[3];
	float mag;

	/* Make rotation matrix */

	/* Z vector */
	z[0] = eyex - centerx;
	z[1] = eyey - centery;
	z[2] = eyez - centerz;
	mag = sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
	if (mag) {			/* mpichler, 19950515 */
		z[0] /= mag;
		z[1] /= mag;
		z[2] /= mag;
	}

	/* Y vector */
	y[0] = upx;
	y[1] = upy;
	y[2] = upz;

	/* X vector = Y cross Z */
	x[0] = y[1] * z[2] - y[2] * z[1];
	x[1] = -y[0] * z[2] + y[2] * z[0];
	x[2] = y[0] * z[1] - y[1] * z[0];

	/* Recompute Y = Z cross X */
	y[0] = z[1] * x[2] - z[2] * x[1];
	y[1] = -z[0] * x[2] + z[2] * x[0];
	y[2] = z[0] * x[1] - z[1] * x[0];

	/* mpichler, 19950515 */
	/* cross product gives area of parallelogram, which is < 1.0 for
	 * non-perpendicular unit-length vectors; so normalize x, y here
	 */

	mag = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
	if (mag) {
		x[0] /= mag;
		x[1] /= mag;
		x[2] /= mag;
	}

	mag = sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
	if (mag) {
		y[0] /= mag;
		y[1] /= mag;
		y[2] /= mag;
	}

#define M(row,col)  tmp.m16[col * 4 + row]
	M(0, 0) = x[0];
	M(0, 1) = x[1];
	M(0, 2) = x[2];
	M(0, 3) = 0.0;
	M(1, 0) = y[0];
	M(1, 1) = y[1];
	M(1, 2) = y[2];
	M(1, 3) = 0.0;
	M(2, 0) = z[0];
	M(2, 1) = z[1];
	M(2, 2) = z[2];
	M(2, 3) = 0.0;
	M(3, 0) = 0.0;
	M(3, 1) = 0.0;
	M(3, 2) = 0.0;
	M(3, 3) = 1.0;
#undef M
	Mat4_Multiply (m, &tmp, m);  //xglMultMatrixf(&tmp[0][0]);
	Mat4_Translate (m, -eyex, -eyey, -eyez);
	return m;
#if 0
    vec3_t forward	= { centerx - eyex, centerx - eyex, centerz - eyez };
	vec3_t up		= { upx, upy, upz };
	vec3_t side;
	glmatrix tmp;

	VectorNormalize		(forward);
	VectorCrossProduct	(forward, up, side);
    VectorNormalize		(side);

    VectorCrossProduct	(side, forward, up);	// Recompute up as: up = side x forward

	Mat4_Identity_Set	(&tmp);

    tmp.m4x4[0][0] = side[0];
    tmp.m4x4[1][0] = side[1];
    tmp.m4x4[2][0] = side[2];

    tmp.m4x4[0][1] = up[0];
    tmp.m4x4[1][1] = up[1];
    tmp.m4x4[2][1] = up[2];

    tmp.m4x4[0][2] = -forward[0];
    tmp.m4x4[1][2] = -forward[1];
    tmp.m4x4[2][2] = -forward[2];

	Mat4_Multiply (m, &tmp, m);  //xglMultMatrixf(&tmp[0][0]);
	Mat4_Translate (m, -eyex, -eyey, -eyez);
	return m;
#endif
}

// Returns NULL if determinant is 0 (failure)
glmatrix *Mat4_Invert (glmatrix *m, const glmatrix *src)
{
	glmatrix tmp;
    float det;
    int i;

    tmp.m16[0]  =   src->m16[5] * src->m16[10]* src->m16[15] - src->m16[5] * src->m16[11]* src->m16[14] - src->m16[9] * src->m16[6] * src->m16[15] + src->m16[9] * src->m16[7] * src->m16[14] + src->m16[13] * src->m16[6] * src->m16[11] - src->m16[13] * src->m16[7] * src->m16[10];
    tmp.m16[4]  =  -src->m16[4] * src->m16[10]* src->m16[15] + src->m16[4] * src->m16[11]* src->m16[14] + src->m16[8] * src->m16[6] * src->m16[15] - src->m16[8] * src->m16[7] * src->m16[14] - src->m16[12] * src->m16[6] * src->m16[11] + src->m16[12] * src->m16[7] * src->m16[10];
    tmp.m16[8]  =   src->m16[4] * src->m16[9] * src->m16[15] - src->m16[4] * src->m16[11]* src->m16[13] - src->m16[8] * src->m16[5] * src->m16[15] + src->m16[8] * src->m16[7] * src->m16[13] + src->m16[12] * src->m16[5] * src->m16[11] - src->m16[12] * src->m16[7] * src->m16[9];
    tmp.m16[12] =  -src->m16[4] * src->m16[9] * src->m16[14] + src->m16[4] * src->m16[10]* src->m16[13] + src->m16[8] * src->m16[5] * src->m16[14] - src->m16[8] * src->m16[6] * src->m16[13] - src->m16[12] * src->m16[5] * src->m16[10] + src->m16[12] * src->m16[6] * src->m16[9];
    tmp.m16[1]  =  -src->m16[1] * src->m16[10]* src->m16[15] + src->m16[1] * src->m16[11]* src->m16[14] + src->m16[9] * src->m16[2] * src->m16[15] - src->m16[9] * src->m16[3] * src->m16[14] - src->m16[13] * src->m16[2] * src->m16[11] + src->m16[13] * src->m16[3] * src->m16[10];
    tmp.m16[5]  =   src->m16[0] * src->m16[10]* src->m16[15] - src->m16[0] * src->m16[11]* src->m16[14] - src->m16[8] * src->m16[2] * src->m16[15] + src->m16[8] * src->m16[3] * src->m16[14] + src->m16[12] * src->m16[2] * src->m16[11] - src->m16[12] * src->m16[3] * src->m16[10];
    tmp.m16[9]  =  -src->m16[0] * src->m16[9] * src->m16[15] + src->m16[0] * src->m16[11]* src->m16[13] + src->m16[8] * src->m16[1] * src->m16[15] - src->m16[8] * src->m16[3] * src->m16[13] - src->m16[12] * src->m16[1] * src->m16[11] + src->m16[12] * src->m16[3] * src->m16[9];
    tmp.m16[13] =   src->m16[0] * src->m16[9] * src->m16[14] - src->m16[0] * src->m16[10]* src->m16[13] - src->m16[8] * src->m16[1] * src->m16[14] + src->m16[8] * src->m16[2] * src->m16[13] + src->m16[12] * src->m16[1] * src->m16[10] - src->m16[12] * src->m16[2] * src->m16[9];
    tmp.m16[2]  =   src->m16[1] * src->m16[6] * src->m16[15] - src->m16[1] * src->m16[7] * src->m16[14] - src->m16[5] * src->m16[2] * src->m16[15] + src->m16[5] * src->m16[3] * src->m16[14] + src->m16[13] * src->m16[2] * src->m16[7]  - src->m16[13] * src->m16[3] * src->m16[6];
    tmp.m16[6]  =  -src->m16[0] * src->m16[6] * src->m16[15] + src->m16[0] * src->m16[7] * src->m16[14] + src->m16[4] * src->m16[2] * src->m16[15] - src->m16[4] * src->m16[3] * src->m16[14] - src->m16[12] * src->m16[2] * src->m16[7]  + src->m16[12] * src->m16[3] * src->m16[6];
    tmp.m16[10] =   src->m16[0] * src->m16[5] * src->m16[15] - src->m16[0] * src->m16[7] * src->m16[13] - src->m16[4] * src->m16[1] * src->m16[15] + src->m16[4] * src->m16[3] * src->m16[13] + src->m16[12] * src->m16[1] * src->m16[7]  - src->m16[12] * src->m16[3] * src->m16[5];
    tmp.m16[14] =  -src->m16[0] * src->m16[5] * src->m16[14] + src->m16[0] * src->m16[6] * src->m16[13] + src->m16[4] * src->m16[1] * src->m16[14] - src->m16[4] * src->m16[2] * src->m16[13] - src->m16[12] * src->m16[1] * src->m16[6]  + src->m16[12] * src->m16[2] * src->m16[5];
    tmp.m16[3]  =  -src->m16[1] * src->m16[6] * src->m16[11] + src->m16[1] * src->m16[7] * src->m16[10] + src->m16[5] * src->m16[2] * src->m16[11] - src->m16[5] * src->m16[3] * src->m16[10] - src->m16[9]  * src->m16[2] * src->m16[7]  + src->m16[9]  * src->m16[3] * src->m16[6];
    tmp.m16[7]  =   src->m16[0] * src->m16[6] * src->m16[11] - src->m16[0] * src->m16[7] * src->m16[10] - src->m16[4] * src->m16[2] * src->m16[11] + src->m16[4] * src->m16[3] * src->m16[10] + src->m16[8]  * src->m16[2] * src->m16[7]  - src->m16[8]  * src->m16[3] * src->m16[6];
    tmp.m16[11] =  -src->m16[0] * src->m16[5] * src->m16[11] + src->m16[0] * src->m16[7] * src->m16[9]  + src->m16[4] * src->m16[1] * src->m16[11] - src->m16[4] * src->m16[3] * src->m16[9]  - src->m16[8]  * src->m16[1] * src->m16[7]  + src->m16[8]  * src->m16[3] * src->m16[5];
    tmp.m16[15] =   src->m16[0] * src->m16[5] * src->m16[10] - src->m16[0] * src->m16[6] * src->m16[9]  - src->m16[4] * src->m16[1] * src->m16[10] + src->m16[4] * src->m16[2] * src->m16[9]  + src->m16[8]  * src->m16[1] * src->m16[6]  - src->m16[8]  * src->m16[2] * src->m16[5];

    det = src->m16[0] * tmp.m16[0] + src->m16[1] * tmp.m16[4] + src->m16[2] * tmp.m16[8] + src->m16[3] * tmp.m16[12];


	if (det == 0) {
		return NULL; // Failure
	}

	det = 1.0f / det;

	// Multiply each element of tmp matrix times 16.  Write the output to m.
	for (i = 0; i < 16; i++)
		m->m16[i] = tmp.m16[i] * det;

	return m;
}

// No fucking idea
static void sMultMatrixVecf (const glmatrix *src, const float *in /*[4]*/ ,
								float *out /*[4]*/)
{
    int n;

    for (n = 0; n < 4; n ++) {
		out[n] =
		in[0] * src->m16[0 * 4 + n] +
		in[1] * src->m16[1 * 4 + n] +
		in[2] * src->m16[2 * 4 + n] +
		in[3] * src->m16[3 * 4 + n] ;
    }
}


// Returns NULL on failure
cbool _Mat4_Project_Classic_Normal (float objx, float objy, float objz, const glmatrix *modelview, const glmatrix *projection, const int *viewport /*[4]*/,
									float *winx, float *winy, float *winz) // Outputs
{

    float in[4] = { objx, objy, objz, 1.0 };
    float out[4];

	sMultMatrixVecf (modelview, in, out);
	sMultMatrixVecf (projection, out, in);

    if (in[3] == 0.0) {
		return false;
	}

    in[0] /= in[3];
    in[1] /= in[3];
    in[2] /= in[3];

    // Map x, y and z to range 0-1
    in[0] = in[0] * 0.5f + 0.5f;
    in[1] = in[1] * 0.5f + 0.5f;
    in[2] = in[2] * 0.5f + 0.5f;

    // Map x,y to viewport
    in[0] = in[0] * viewport[2] + viewport[0];
    in[1] = in[1] * viewport[3] + viewport[1];

    if (winx) *winx = in[0];
    if (winy) *winy = in[1];
    if (winz) *winz = in[2];

#ifdef GLM_TESTING
	{
		double modelview_d[16];
		double projection_d[16];
		double winxd, winyd, winzd;
		int n;
		for (n = 0; n < 16; n ++) {
			projection_d[n] = projection->m16[n];
			modelview_d[n] = modelview->m16[n];
		}
		gluProject (objx, objy, objz, modelview_d, projection_d, viewport, &winxd, &winyd, &winzd);
		n = (in[0] == winxd && in[1] == winyd && in[2] == winzd);
	}
#endif // GLM_TESTING

	return true;
}

cbool Mat4_Project_Smart (float objx, float objy, float objz, const glmatrix *modelview, const glmatrix *projection, const int *viewport /*[4]*/, int screenheight,
									float *winx, float *winy, float *winz) // Outputs
{
	cbool ret = _Mat4_Project_Classic_Normal (objx, objy, objz, modelview, projection, viewport, winx, winy, winz);
// NO THE MATRIX ISN'T AFFECTED.  IT'S THE VIEWPORT Y, WHICH MAKES NO DIFFERENCE FOR A CENTERED WINDOW
// I appear to be very wrong on that.  It matters quite a bit
// Now very confused --- ok, Windows and OpenGL have flipped coordinates compared to each other.
	if (winy) {
		float new_winy = /*mainus.screen_descending_y ?*/ screenheight - *winy ; // ALWAYS FOR NOW: *winy;
		*winy = new_winy;
#pragma message ("Note uncentered viewports probably fail.  OpenGL is like a Mac.")
	}
	return ret;
}


cbool _Mat4_UnProject_Classic_Normal (float winx, float winy, float winz, const glmatrix *modelview, const glmatrix *projection, const int *viewport /*[4]*/,
			   float *objx, float *objy, float *objz) // Outputs
{
	glmatrix tmp;

    float in[4] = { winx, winy, winz, 1.0 };
    float out[4];

	Mat4_Multiply (&tmp, modelview, projection);

	if (!Mat4_Invert (&tmp, &tmp))
		return false;

    // Map x and y from window coordinates
    in[0] = (in[0] - viewport[0]) / viewport[2];
    in[1] = (in[1] - viewport[1]) / viewport[3];

    // Map to range -1 to 1
    in[0] = in[0] * 2 - 1;
    in[1] = in[1] * 2 - 1;
    in[2] = in[2] * 2 - 1;

    sMultMatrixVecf (&tmp, in, out);

    if (out[3] == 0.0)
		return false;

    out[0] /= out[3];
    out[1] /= out[3];
    out[2] /= out[3];

    *objx = out[0];
    *objy = out[1];
    *objz = out[2];

#ifdef GLM_TESTING
	{
		double modelview_d[16];
		double projection_d[16];
		double xd, yd, zd;
		int n;
		for (n = 0; n < 16; n ++) {
			projection_d[n] = projection->m16[n];
			modelview_d[n] = modelview->m16[n];
		}

		gluUnProject (winx, winy, winz, modelview_d, projection_d, viewport, &xd, &yd, &zd);
		n = (out[0] == xd && out[1] == yd && out[2] == zd);
	}
#endif // GLM_TESTING


    return true;
}


cbool Mat4_UnProject_Smart (float winx, float _winy, float winz, const glmatrix *modelview, const glmatrix *projection, const int *viewport /*[4]*/, int screenheight,
			   float *objx, float *objy, float *objz) // Outputs
{
// NO THE MATRIX ISN'T AFFECTED.  IT'S THE VIEWPORT Y, WHICH MAKES NO DIFFERENCE FOR A CENTERED WINDOW
// WELL --- methinks I am 2x wrong here.
	float winy = /*mainus.screen_descending_y ?*/ screenheight - _winy;// : _winy;
#pragma message ("Note uncentered viewports probably fail.  OpenGL is like a Mac.  We may need to prepare a goofy mouse viewport")
	return _Mat4_UnProject_Classic_Normal (winx, winy, winz, modelview, projection, viewport, objx, objy, objz);
}

// Translate and scale the picked region to the entire window
glmatrix *Mat4_Pick (glmatrix *m, float x, float y, float deltax, float deltay, const int *viewport /*[4]*/)
{
    if (deltax <= 0 || deltay <= 0) {
		return NULL;
    }

	Mat4_Translate (m,  (viewport[2] - 2 * (x - viewport[0])) / deltax,
							(viewport[3] - 2 * (y - viewport[1])) / deltay,
							0);

    Mat4_Scale		(m, viewport[2] / deltax,
							viewport[3] / deltay,
							1.0);

	return m;
}



