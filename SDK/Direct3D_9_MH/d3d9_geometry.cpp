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


#ifdef DIRECT3D9_WRAPPER
#include "d3d9_internal.h"

/*
===================================================================================================================

			VERTEX SUBMISSION

	Per the spec for glVertex (http://www.opengl.org/sdk/docs/man/xhtml/glVertex.xml) glColor, glNormal and
	glTexCoord just specify a *current* color, normal and texcoord, whereas glVertex actually creates a vertex
	that inherits the current color, normal and texcoord.

	everything is drawn as indexed triangle lists for simplicity and invariance

	Real D3D code looks *nothing* like this.

===================================================================================================================
*/


geometry_t::geometry_t (void)
{
	// set up initial modes with nothing in them
	this->Mode = GL_INVALID_VALUE;
	this->NumVerts = 0;
	this->PrimVerts = 0;
	this->NumIndexes = 0;
}


void geometry_t::Activate (void)
{
	// set up initial modes with nothing in them
	this->Mode = GL_INVALID_VALUE;
	this->NumVerts = 0;
	this->PrimVerts = 0;
	this->NumIndexes = 0;
}


void geometry_t::EmitColor (int red, int green, int blue, int alpha)
{
	this->CurrentColor = D3DCOLOR_ARGB (
		BYTE_CLAMP (alpha),
		BYTE_CLAMP (red),
		BYTE_CLAMP (green),
		BYTE_CLAMP (blue)
	);
}


D3DPRIMITIVETYPE D3D_BatchTypeForGLMode (GLenum mode)
{
	switch (mode)
	{
	case GL_INVALID_VALUE: return D3DPT_FORCE_DWORD;
	case GL_POINTS: return D3DPT_POINTLIST;
	case GL_LINES: return D3DPT_LINELIST;
	case GL_LINE_STRIP: return D3DPT_LINESTRIP;
	default: return D3DPT_TRIANGLELIST;
	}
}


void context_t::BeginPrimitive (GLenum mode)
{
	this->BeginScene ();

	// flush the previous batches if switching between different batching types
	if (D3D_BatchTypeForGLMode (mode) != D3D_BatchTypeForGLMode (this->Geometry.Mode))
		this->FlushGeometry ();

	// yee-hah immediate mode - we don't know in advance how big the verts are gonna be so we just write out and hope we never overflow!
	if ((this->Geometry.NumVerts + 64) >= MAX_GEOM_VERTEXES || (this->Geometry.NumIndexes + 128) >= MAX_GEOM_INDEXES)
	{
		// flush accumulated geometry
		this->FlushGeometry ();
	}

	// check polygon offset - this is deferred because it's multiple interacting states so we don't know till draw time
	this->CheckPolygonOffset (mode);

	// check for dirty matrixes
	this->UpdateTransforms ();

	// set up textures
	this->SetupTexturesForDrawing ();

	// just store out the mode, all heavy lifting is done in glEnd
	this->Geometry.Mode = mode;

	// begin a new primitive
	this->Geometry.PrimVerts = 0;
}


void geometry_t::EmitTexCoord (int stage, float s, float t)
{
	this->CurrentTexCoord[stage][0] = s;
	this->CurrentTexCoord[stage][1] = t;
}


void geometry_t::EmitVertex (float x, float y, float z)
{
	vertex_t *vert = &this->Vertexes[this->NumVerts + this->PrimVerts];

	// add a new vertex to the list with the specified xyz and inheriting the current normal, color and texcoords
	// (per spec at http://www.opengl.org/sdk/docs/man/xhtml/glVertex.xml)
	vert->position[0] = x;
	vert->position[1] = y;
	vert->position[2] = z;

	vert->color = this->CurrentColor;

	for (int i = 0; i < D3D_MAX_TMUS; i++)
	{
		vert->texcoords[i][0] = this->CurrentTexCoord[i][0];
		vert->texcoords[i][1] = this->CurrentTexCoord[i][1];
	}

	// go to a new vertex
	this->PrimVerts++;
}


void geometry_t::EndPrimitive (void)
{
	// build indexes for the current mode
	switch (this->Mode)
	{
	case GL_LINES:
	case GL_POINTS:
	case GL_LINE_STRIP:
		// handled separately
		this->NumVerts += this->PrimVerts;
		this->PrimVerts = 0;
		return;

	case GL_QUAD_STRIP:
		for (int i = 2; i < this->PrimVerts; i += 2)
		{
			this->Indexes[this->NumIndexes++] = this->NumVerts + (i - 2);
			this->Indexes[this->NumIndexes++] = this->NumVerts + (i - 1);
			this->Indexes[this->NumIndexes++] = this->NumVerts + (i + 1);
			this->Indexes[this->NumIndexes++] = this->NumVerts + (i - 2);
			this->Indexes[this->NumIndexes++] = this->NumVerts + (i + 1);
			this->Indexes[this->NumIndexes++] = this->NumVerts + i;
		}

		break;

	case GL_TRIANGLE_STRIP:
		for (int i = 2; i < this->PrimVerts; i++)
		{
			this->Indexes[this->NumIndexes++] = this->NumVerts + (i - 2);
			this->Indexes[this->NumIndexes++] = this->NumVerts + ((i & 1) ? i : (i - 1));
			this->Indexes[this->NumIndexes++] = this->NumVerts + ((i & 1) ? (i - 1) : i);
		}

		break;

	case GL_POLYGON:
	case GL_TRIANGLE_FAN:
		for (int i = 2; i < this->PrimVerts; i++)
		{
			this->Indexes[this->NumIndexes++] = this->NumVerts;
			this->Indexes[this->NumIndexes++] = this->NumVerts + (i - 1);
			this->Indexes[this->NumIndexes++] = this->NumVerts + i;
		}

		break;

	case GL_TRIANGLES:
		for (int i = 0; i < this->PrimVerts; i += 3)
		{
			this->Indexes[this->NumIndexes++] = this->NumVerts + i;
			this->Indexes[this->NumIndexes++] = this->NumVerts + (i + 1);
			this->Indexes[this->NumIndexes++] = this->NumVerts + (i + 2);
		}

		break;

	case GL_QUADS:
		for (int i = 0; i < this->PrimVerts; i += 4)
		{
			this->Indexes[this->NumIndexes++] = this->NumVerts + i;
			this->Indexes[this->NumIndexes++] = this->NumVerts + (i + 1);
			this->Indexes[this->NumIndexes++] = this->NumVerts + (i + 2);
			this->Indexes[this->NumIndexes++] = this->NumVerts + i;
			this->Indexes[this->NumIndexes++] = this->NumVerts + (i + 2);
			this->Indexes[this->NumIndexes++] = this->NumVerts + (i + 3);
		}

		break;

	default:
		// unimplemented mode does not add indexes so the verts for it don't get drawn;
		// by returning we also don't increment PrimVerts to NumVerts so the verts are discarded
		return;
	}

	// advance to a new primitive
	this->NumVerts += this->PrimVerts;
	this->PrimVerts = 0;
}


void context_t::FlushGeometry (void)
{
	// any state change needs to flush currently batched-up stuff
	// batching - glBegin stores out current vertex pointer, index pointer, mode and numbers of each
	// glVertex adds a vertex to the batch
	// glEnd builds indexes based on mode and number of vertexes in the batch, then updates pointers and counters
	// flush submits the batch then resets pointers and counters

	switch (this->Geometry.Mode)
	{
	case GL_INVALID_VALUE:
		// this is the initially set mode at the start of a frame and nothing is drawn
		break;

	case GL_LINES:
		// handle lines separately
		this->Device->DrawPrimitiveUP (D3DPT_LINELIST, this->Geometry.NumVerts / 2, this->Geometry.Vertexes, sizeof (vertex_t));
		this->State.DrawCount++;
		break;

	case GL_LINE_STRIP:
		// handle lines separately
		this->Device->DrawPrimitiveUP (D3DPT_LINESTRIP, this->Geometry.NumVerts - 1, this->Geometry.Vertexes, sizeof (vertex_t));
		this->State.DrawCount++;
		break;

	case GL_POINTS:
		// handle points separately
		this->Device->DrawPrimitiveUP (D3DPT_POINTLIST, this->Geometry.NumVerts, this->Geometry.Vertexes, sizeof (vertex_t));
		this->State.DrawCount++;
		break;

	default:
		if (this->State.Cull.Mode == GL_FRONT_AND_BACK)
			; // do nothing - the poly was culled (per GL spec, points and lines do not obey the cull mode
		else if (this->Geometry.NumVerts && this->Geometry.NumIndexes)
		{
			this->Device->DrawIndexedPrimitiveUP (
				D3DPT_TRIANGLELIST,
				0,
				this->Geometry.NumVerts,
				this->Geometry.NumIndexes / 3,
				this->Geometry.Indexes,
				D3DFMT_INDEX16,
				this->Geometry.Vertexes,
				sizeof (vertex_t)
			);

			this->State.DrawCount++;
		}

		break;
	}

	// reset everything in case one is 0 but the other is not
	this->Geometry.NumVerts = 0;
	this->Geometry.NumIndexes = 0;
	this->Geometry.PrimVerts = 0;
}


void context_t::InitGeometry (void)
{
	// modes and counts
	this->Geometry.Activate ();

	// initial current attribs
	this->Geometry.CurrentColor = 0xffffffff;

	for (int i = 0; i < D3D_MAX_TMUS; i++)
		this->Geometry.EmitTexCoord (i, 0, 0);
}


#endif

