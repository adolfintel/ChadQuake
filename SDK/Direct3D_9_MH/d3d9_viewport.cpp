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


void context_t::UpdateViewport (void)
{
	D3DVIEWPORT9 *vp = &this->State.Viewport;

	if (vp->Width > 0 && vp->Height > 0)
	{
		// this code is called whenever our viewport or render target changes
		float texelOffset[] = {-1.0f / vp->Width, 1.0f / vp->Height, 0.0f, 0.0f};

		this->FlushGeometry ();

		this->Device->SetVertexShaderConstantF (8, texelOffset, 1);
		this->Device->SetViewport (vp);
	}
}


void context_t::ResetViewport (void)
{
	this->State.Viewport.X = 0;
	this->State.Viewport.Y = 0;
	this->State.Viewport.Width = this->DisplayMode.Width;
	this->State.Viewport.Height = this->DisplayMode.Height;
	this->State.Viewport.MinZ = 0;
	this->State.Viewport.MaxZ = 1;
}


void context_t::GetViewport (GLint *params)
{
	params[0] = this->State.Viewport.X;
	params[1] = this->State.Viewport.Y;
	params[2] = this->State.Viewport.Width;
	params[3] = this->State.Viewport.Height;
}


void context_t::SaveViewport (D3DVIEWPORT9 *saved)
{
	saved->X = this->State.Viewport.X;
	saved->Y = this->State.Viewport.Y;
	saved->Width = this->State.Viewport.Width;
	saved->Height = this->State.Viewport.Height;
	saved->MinZ = this->State.Viewport.MinZ;
	saved->MaxZ = this->State.Viewport.MaxZ;
}


void context_t::RestoreViewport (D3DVIEWPORT9 *saved)
{
	this->State.Viewport.X = saved->X;
	this->State.Viewport.Y = saved->Y;
	this->State.Viewport.Width = saved->Width;
	this->State.Viewport.Height = saved->Height;
	this->State.Viewport.MinZ = saved->MinZ;
	this->State.Viewport.MaxZ = saved->MaxZ;
}


#endif
