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

// baker has threatened to implement vertex arrays in MarkV so here's a basic-ish vertex array interface.
// this was written with functionality in mind rather than performance.
// this is completely untested so it may have subtle (or not so subtle) bugs, but the code reads OK to me...
#ifdef DIRECT3D9_WRAPPER
#include "d3d9_internal.h"


void context_t::EnableClientState (GLenum array, BOOL enable)
{
	switch (array)
	{
	case GL_VERTEX_ARRAY:
		this->VertexArray.Enabled = enable;
		break;

	case GL_COLOR_ARRAY:
		this->ColorArray.Enabled = enable;
		break;

	case GL_TEXTURE_COORD_ARRAY:
		this->TexCoordArray[this->ClientActiveTexture].Enabled = enable;
		break;

	default:
		System_Error ("GL_EnableClientState_Internal : unimplemented");
	}
}


void varray_t::SetPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	this->Size = size;
	this->Type = type;
	this->Stride = stride;
	this->Pointer = pointer;
}


void *varray_t::FetchArrayElement (int e)
{
	if (this->Stride)
	{
		// access by stride between elements
		return ((unsigned char *) this->Pointer) + (this->Stride * e);
	}
	else
	{
		// tightly packed - combine size and type to get the offset into pointer for element e
		int stride = this->Size;

		switch (this->Type)
		{
		case GL_FLOAT: stride *= sizeof (float); break;
		case GL_UNSIGNED_BYTE: stride *= sizeof (unsigned char); break;
		default: System_Error ("varray_t::FetchArrayElement : unimplemented");
		}

		return ((unsigned char *) this->Pointer) + (stride * e);
	}
}


float *varray_t::FetchArrayElementFloat (int e)
{
	return (float *) this->FetchArrayElement (e);
}


unsigned char *varray_t::FetchArrayElementByte (int e)
{
	return (unsigned char *) this->FetchArrayElement (e);
}


void context_t::ArrayElement (GLint e)
{
	for (int i = 0; i < D3D_MAX_TMUS; i++)
	{
		if (this->TexCoordArray[i].Enabled)
		{
			if (this->TexCoordArray[i].Size == 2 && this->TexCoordArray[i].Type == GL_FLOAT)
				Direct3D9_glMultiTexCoord2fv (GLD3D_TEXTURE0 + i, this->TexCoordArray[i].FetchArrayElementFloat (e));
			else System_Error ("context_t::ArrayElement : unimplemented");
		}
	}

	if (this->ColorArray.Enabled)
	{
		if (this->ColorArray.Size == 3 && this->ColorArray.Type == GL_FLOAT)
			d3d9mh_glColor3fv (this->ColorArray.FetchArrayElementFloat (e));
		else if (this->ColorArray.Size == 4 && this->ColorArray.Type == GL_FLOAT)
			d3d9mh_glColor4fv (this->ColorArray.FetchArrayElementFloat (e));
		else if (this->ColorArray.Size == 3 && this->ColorArray.Type == GL_UNSIGNED_BYTE)
			d3d9mh_glColor3ubv (this->ColorArray.FetchArrayElementByte (e));
		else if (this->ColorArray.Size == 4 && this->ColorArray.Type == GL_UNSIGNED_BYTE)
			d3d9mh_glColor4ubv (this->ColorArray.FetchArrayElementByte (e));
		else System_Error ("context_t::ArrayElement : unimplemented");
	}

	if (this->VertexArray.Enabled)
	{
		if (this->VertexArray.Size == 2 && this->ColorArray.Type == GL_FLOAT)
			d3d9mh_glVertex2fv (this->VertexArray.FetchArrayElementFloat (e));
		else if (this->VertexArray.Size == 3 && this->ColorArray.Type == GL_FLOAT)
			d3d9mh_glVertex3fv (this->VertexArray.FetchArrayElementFloat (e));
		else System_Error ("context_t::ArrayElement : unimplemented");
	}
	else System_Error ("context_t::ArrayElement : GL_VERTEX_ARRAY not enabled");
}


#endif

