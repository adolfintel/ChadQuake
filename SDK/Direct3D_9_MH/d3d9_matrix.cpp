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


/*
===================================================================================================================

			MATRIXES

	Most matrix ops occur in software in D3D; the only part that may (or may not) occur in hardware (depending
	on whether or not we have a hardware vp device) is the final transformation of submitted verts.  Note that
	OpenGL is probably the same.

	This interface just models the most common OpenGL matrix ops with their D3D equivalents.  I don't use the
	IDirect3DXMatrixStack interface because of overhead and baggage associated with it.

===================================================================================================================
*/

#ifdef DIRECT3D9_WRAPPER

// Includes
#include "d3d9_internal.h"

#include <xmmintrin.h>
#include <smmintrin.h>


void matrixstack_t::MultMatrix (D3DMATRIX *m)
{
	// up to 4x the perf of raw C code
	__m128 mrow;

	__m128 m2c0 = _mm_loadu_ps (this->Current->m4x4[0]);
	__m128 m2c1 = _mm_loadu_ps (this->Current->m4x4[1]);
	__m128 m2c2 = _mm_loadu_ps (this->Current->m4x4[2]);
	__m128 m2c3 = _mm_loadu_ps (this->Current->m4x4[3]);

	_MM_TRANSPOSE4_PS (m2c0, m2c1, m2c2, m2c3);

	mrow = _mm_loadu_ps (m->m4x4[0]);
	_mm_storeu_ps (this->Current->m4x4[0], _mm_hadd_ps (_mm_hadd_ps (_mm_mul_ps (mrow, m2c0), _mm_mul_ps (mrow, m2c1)), _mm_hadd_ps (_mm_mul_ps (mrow, m2c2), _mm_mul_ps (mrow, m2c3))));

	mrow = _mm_loadu_ps (m->m4x4[1]);
	_mm_storeu_ps (this->Current->m4x4[1], _mm_hadd_ps (_mm_hadd_ps (_mm_mul_ps (mrow, m2c0), _mm_mul_ps (mrow, m2c1)), _mm_hadd_ps (_mm_mul_ps (mrow, m2c2), _mm_mul_ps (mrow, m2c3))));

	mrow = _mm_loadu_ps (m->m4x4[2]);
	_mm_storeu_ps (this->Current->m4x4[2], _mm_hadd_ps (_mm_hadd_ps (_mm_mul_ps (mrow, m2c0), _mm_mul_ps (mrow, m2c1)), _mm_hadd_ps (_mm_mul_ps (mrow, m2c2), _mm_mul_ps (mrow, m2c3))));

	mrow = _mm_loadu_ps (m->m4x4[3]);
	_mm_storeu_ps (this->Current->m4x4[3], _mm_hadd_ps (_mm_hadd_ps (_mm_mul_ps (mrow, m2c0), _mm_mul_ps (mrow, m2c1)), _mm_hadd_ps (_mm_mul_ps (mrow, m2c2), _mm_mul_ps (mrow, m2c3))));

	this->dirty = TRUE;
}


matrixstack_t::matrixstack_t (void)
{
	// initializes a matrix to a known state prior to rendering
	this->dirty = TRUE;
	this->stackdepth = 0;
	this->Current = &this->stack[0];
	this->Identity ();
}


void matrixstack_t::Initialize (void)
{
	// initializes a matrix to a known state prior to rendering
	this->dirty = TRUE;
	this->stackdepth = 0;
	this->Current = &this->stack[0];
	this->Identity ();
}


void matrixstack_t::Push (void)
{
	if (this->stackdepth <= (MAX_MATRIX_STACK - 1))
	{
		D3DMATRIX *Former = this->Current;

		// go to a new matrix (only push if there's room to push)
		this->stackdepth++;
		this->Current = &this->stack[this->stackdepth];

		// copy up the previous matrix (per spec)
		memcpy (this->Current, Former, sizeof (D3DMATRIX));
	}

	// flag as dirty
	this->dirty = TRUE;
}


void matrixstack_t::Pop (void)
{
	if (!this->stackdepth)
	{
		// opengl silently allows this and so should we (witness TQ's R_DrawAliasModel, which pushes the
		// matrix once but pops it twice - on line 423 and line 468
		this->dirty = TRUE;
		return;
	}

	// go to a new matrix
	this->stackdepth--;
	this->Current = &this->stack[this->stackdepth];

	// flag as dirty
	this->dirty = TRUE;
}


void matrixstack_t::Identity (void)
{
	D3DMATRIX *m = this->Current;

	m->m4x4[0][0] = 1; m->m4x4[0][1] = 0; m->m4x4[0][2] = 0; m->m4x4[0][3] = 0;
	m->m4x4[1][0] = 0; m->m4x4[1][1] = 1; m->m4x4[1][2] = 0; m->m4x4[1][3] = 0;
	m->m4x4[2][0] = 0; m->m4x4[2][1] = 0; m->m4x4[2][2] = 1; m->m4x4[2][3] = 0;
	m->m4x4[3][0] = 0; m->m4x4[3][1] = 0; m->m4x4[3][2] = 0; m->m4x4[3][3] = 1;

	this->dirty = TRUE;
}


void matrixstack_t::Frustum (float left, float right, float bottom, float top, float zNear, float zFar)
{
	// this is a d3d-style frustum matrix with ndc-z going 0...1 so don't try to "correct" it to gl-style or very near objects will be cut off
	D3DMATRIX m2 = {
		(2.0f * zNear) / (right - left),
		0,
		0,
		0,
		0,
		(2.0f * zNear) / (top - bottom),
		0,
		0,
		(left + right) / (right - left),
		(top + bottom) / (top - bottom),
		zFar / (zNear - zFar),
		-1,
		0,
		0,
		(zNear * zFar) / (zNear - zFar),
		0
	};

	this->MultMatrix (&m2);
	this->dirty = TRUE;
}


void matrixstack_t::Ortho (float left, float right, float bottom, float top, float zNear, float zFar)
{
	// this is a d3d-style ortho matrix with ndc-z going 0...1 so don't try to "correct" it to gl-style or very near objects will be cut off
	D3DMATRIX m2 = {
		2 / (right - left),
		0,
		0,
		0,
		0,
		2 / (top - bottom),
		0,
		0,
		0,
		0,
		1 / (zNear - zFar),
		0,
		(left + right) / (left - right),
		(top + bottom) / (bottom - top),
		zNear / (zNear - zFar),
		1
	};

	this->MultMatrix (&m2);
	this->dirty = TRUE;
}


void matrixstack_t::Rotate (float angle, float x, float y, float z)
{
	float xyz[3] = {x, y, z};

	Vector3Normalize (xyz);
	angle = DEG2RAD (angle);

	float sa = sin (angle);
	float ca = cos (angle);

	D3DMATRIX m2 = {
		(1.0f - ca) * xyz[0] * xyz[0] + ca,
		(1.0f - ca) * xyz[1] * xyz[0] + sa * xyz[2],
		(1.0f - ca) * xyz[2] * xyz[0] - sa * xyz[1],
		0.0f,
		(1.0f - ca) * xyz[0] * xyz[1] - sa * xyz[2],
		(1.0f - ca) * xyz[1] * xyz[1] + ca,
		(1.0f - ca) * xyz[2] * xyz[1] + sa * xyz[0],
		0.0f,
		(1.0f - ca) * xyz[0] * xyz[2] + sa * xyz[1],
		(1.0f - ca) * xyz[1] * xyz[2] - sa * xyz[0],
		(1.0f - ca) * xyz[2] * xyz[2] + ca,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		1.0f
	};

	this->MultMatrix (&m2);
	this->dirty = TRUE;
}


void matrixstack_t::Translate (float x, float y, float z)
{
	D3DMATRIX *m = this->Current;

	m->m4x4[3][0] += x * m->m4x4[0][0] + y * m->m4x4[1][0] + z * m->m4x4[2][0];
	m->m4x4[3][1] += x * m->m4x4[0][1] + y * m->m4x4[1][1] + z * m->m4x4[2][1];
	m->m4x4[3][2] += x * m->m4x4[0][2] + y * m->m4x4[1][2] + z * m->m4x4[2][2];
	m->m4x4[3][3] += x * m->m4x4[0][3] + y * m->m4x4[1][3] + z * m->m4x4[2][3];

	this->dirty = TRUE;
}


void matrixstack_t::Scale (float x, float y, float z)
{
	D3DMATRIX *m = this->Current;

	Vector4Scalef (m->m4x4[0], m->m4x4[0], x);
	Vector4Scalef (m->m4x4[1], m->m4x4[1], y);
	Vector4Scalef (m->m4x4[2], m->m4x4[2], z);

	this->dirty = TRUE;
}


void matrixstack_t::Mult (const float *m)
{
	this->MultMatrix ((D3DMATRIX *) m);
	this->dirty = TRUE;
}


void matrixstack_t::Load (const float *m)
{
	memcpy (this->Current->m16, m, sizeof (float) * 16);
	this->dirty = TRUE;
}


void context_t::CheckDirtyMatrix (D3DTRANSFORMSTATETYPE usage, int reg, matrixstack_t *m)
{
	if (m->dirty)
	{
		this->FlushGeometry ();

		// fixed function fog needs the SetTransform call otherwise it won't draw
		this->Device->SetTransform (usage, m->Current);
		this->Device->SetVertexShaderConstantF (reg, m->Current->m16, 4);

		m->dirty = FALSE;
	}
}


void context_t::DirtyMatrixes (void)
{
	int i;

	this->ViewMatrix.dirty = TRUE;
	this->ProjMatrix.dirty = TRUE;

	for (i = 0; i < D3D_MAX_TMUS; i++)
	{
		this->TMU[i].TextureMatrix.dirty = TRUE;
	}
}


void context_t::UpdateTransforms (void)
{
	int i, reg;

	this->CheckDirtyMatrix (D3DTS_VIEW, 4, &this->ViewMatrix);
	this->CheckDirtyMatrix (D3DTS_PROJECTION, 0, &this->ProjMatrix);

	for (i = 0, reg = 10; i < D3D_MAX_TMUS; i++, reg += 4)
	{
		this->CheckDirtyMatrix ((D3DTRANSFORMSTATETYPE) ((int) D3DTS_TEXTURE0 + i), reg, &this->TMU[i].TextureMatrix);
	}
}


void context_t::InitializeTransforms (void)
{
	// create an identity world matrix
	matrixstack_t worldmatrix;
	worldmatrix.Initialize ();

	// load into reg 4, it doesn't matter because it will be overwritten anyway and we don't touch world in HLSL code
	this->CheckDirtyMatrix (D3DTS_WORLD, 4, &worldmatrix);

	this->ViewMatrix.Initialize ();
	this->ProjMatrix.Initialize ();

	for (int i = 0; i < D3D_MAX_TMUS; i++)
		this->TMU[i].TextureMatrix.Initialize ();

	this->State.CurrentMatrix = &this->ViewMatrix;
}


void context_t::GetMatrix (GLenum pname, GLfloat *params)
{
	switch (pname)
	{
	case GL_MODELVIEW_MATRIX:
		memcpy (params, this->ViewMatrix.Current->m16, sizeof (float) * 16);
		break;

	case GL_PROJECTION_MATRIX:
		memcpy (params, this->ProjMatrix.Current->m16, sizeof (float) * 16);
		break;

	case GL_TEXTURE_MATRIX:
		memcpy (params, this->TMU[this->State.CurrentTMU].TextureMatrix.Current->m16, sizeof (float) * 16);
		break;
	}
}


#endif
