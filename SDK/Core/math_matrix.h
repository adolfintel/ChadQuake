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
// math_matrix.h

#ifndef __MATH_MATRIX_H__
#define __MATH_MATRIX_H__

#include "environment.h"

typedef struct _glmatrix
{
    union
	{
		// put first because gcc barfs a little
		float m16[16];
        float m4x4[4][4];

        struct
		{
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
    };
} glmatrix;


glmatrix *Mat4_Multiply (glmatrix *m, const glmatrix *a, const glmatrix *b);
glmatrix *Mat4_Translate (glmatrix *m, float x, float y, float z);
glmatrix *Mat4_Scale (glmatrix *m, float x, float y, float z);
glmatrix *Mat4_Rotate (glmatrix *m, float a, float x, float y, float z);
glmatrix *Mat4_Identity_Set (glmatrix *m);
glmatrix *Mat4_Copy (glmatrix *m, const glmatrix *src);

glmatrix *Mat4_Invert (glmatrix *m, const glmatrix *src);

glmatrix *Mat4_Ortho (glmatrix *m, double left, double right, double bottom, double top, double znear, double zfar);
glmatrix *Mat4_OrthoAspect (glmatrix *m, float width, float height);
glmatrix *Mat4_OrthoAspectRect (glmatrix *m, float left, float top, float width, float height);
glmatrix *Mat4_OrthoZero (glmatrix *m, float width, float height);

glmatrix *Mat4_Ortho3D (glmatrix *m, double width, double height);

glmatrix *Mat4_Frustum (glmatrix *m, double left, double right, double bottom, double top, double znear, double zfar);
// glFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
// glOrtho   (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);

glmatrix *Mat4_Perspective (glmatrix *m, float fovy, float aspect, float zNear, float zFar);

glmatrix *Mat4_LookAt (glmatrix *m, float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz);


cbool _Mat4_Project_Classic_Normal (float objx, float objy, float objz, const glmatrix *modelview, const glmatrix *projection, const int *viewport /*[4]*/,
									float *winx, float *winy, float *winz);

cbool Mat4_Project_Smart (float objx, float objy, float objz, const glmatrix *modelview, const glmatrix *projection, const int *viewport /*[4]*/, int screenheight,
									float *winx, float *winy, float *winz);



// Direct gluUnproject equivalent except it takes floats instead of doubles
cbool _Mat4_UnProject_Classic_Normal (float winx, float winy, float winz, const glmatrix *modelview, const glmatrix *projection, const int *viewport /*[4]*/,
			   float *objx, float *objy, float *objz);


// This converts the screen coordinates as if they were Windows-like where y = 0 is top of screen.  Mac y = 0 is bottom, just like OpenGL.
cbool Mat4_UnProject_Smart (float winx, float winy, float winz, const glmatrix *model, const glmatrix *projection, const int *viewport /*[4]*/, int screenheight,
			   float *objx, float *objy, float *objz);


glmatrix *Mat4_Pick (glmatrix *m, float x, float y, float deltax, float deltay, const int *viewport /*[4]*/);


cbool Mat4_Compare (glmatrix *a, glmatrix *b);
char *Mat4_String (glmatrix *m);

#endif // ! __MATH_MATRIX_H__


