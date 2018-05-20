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


// states that don't fit in neatly elsewhere
#ifdef DIRECT3D9_WRAPPER
#include "d3d9_internal.h"


state_t::state_t (void)
{
	this->PolygonOffset.FillEnabled = FALSE;
	this->PolygonOffset.LineEnabled = FALSE;
	this->PolygonOffset.PointEnabled = FALSE;
	this->PolygonOffset.Factor = 0;
	this->PolygonOffset.Units = 0;

	memset (this->BoundTextures, 0, sizeof (this->BoundTextures));
	this->CurrentTMU = 0;

	this->SceneBegun = FALSE;
	memset (&this->Viewport, 0, sizeof (this->Viewport));

	// clear
	this->Clear.Color = 0x00000000;
	this->Clear.Depth = 1.0f;
	this->Clear.Stencil = 0;

	this->Cull.Enable = GL_FALSE;
	this->Cull.Mode = GL_BACK;
	this->Cull.FrontFace = GL_CCW;

	memset (this->RenderStates, 0, sizeof (this->RenderStates));
	memset (this->SamplerStates, 0, sizeof (this->SamplerStates));
	memset (this->TextureStates, 0, sizeof (this->TextureStates));

	this->Gamma = 1;
	this->Contrast = 1;
	this->DoingGammaAndContrast = FALSE;
	this->OriginalRT = NULL;
	this->CurrentMatrix = NULL;
	this->DrawCount = 0;
}


void context_t::SetRenderState (D3DRENDERSTATETYPE state, DWORD value)
{
	// filter state
	if (this->State.RenderStates[(int) state] == value) return;

	this->FlushGeometry ();

	// set the state and cache it back
	this->Device->SetRenderState (state, value);
	this->State.RenderStates[(int) state] = value;
}


void context_t::GetRenderStates (void)
{
	this->FlushGeometry ();

	this->Device->GetRenderState (D3DRS_ZENABLE, &this->State.RenderStates[D3DRS_ZENABLE]);
	this->Device->GetRenderState (D3DRS_FILLMODE, &this->State.RenderStates[D3DRS_FILLMODE]);
	this->Device->GetRenderState (D3DRS_SHADEMODE, &this->State.RenderStates[D3DRS_SHADEMODE]);
	this->Device->GetRenderState (D3DRS_ZWRITEENABLE, &this->State.RenderStates[D3DRS_ZWRITEENABLE]);
	this->Device->GetRenderState (D3DRS_ALPHATESTENABLE, &this->State.RenderStates[D3DRS_ALPHATESTENABLE]);
	this->Device->GetRenderState (D3DRS_LASTPIXEL, &this->State.RenderStates[D3DRS_LASTPIXEL]);
	this->Device->GetRenderState (D3DRS_SRCBLEND, &this->State.RenderStates[D3DRS_SRCBLEND]);
	this->Device->GetRenderState (D3DRS_DESTBLEND, &this->State.RenderStates[D3DRS_DESTBLEND]);
	this->Device->GetRenderState (D3DRS_CULLMODE, &this->State.RenderStates[D3DRS_CULLMODE]);
	this->Device->GetRenderState (D3DRS_ZFUNC, &this->State.RenderStates[D3DRS_ZFUNC]);
	this->Device->GetRenderState (D3DRS_ALPHAREF, &this->State.RenderStates[D3DRS_ALPHAREF]);
	this->Device->GetRenderState (D3DRS_ALPHAFUNC, &this->State.RenderStates[D3DRS_ALPHAFUNC]);
	this->Device->GetRenderState (D3DRS_DITHERENABLE, &this->State.RenderStates[D3DRS_DITHERENABLE]);
	this->Device->GetRenderState (D3DRS_ALPHABLENDENABLE, &this->State.RenderStates[D3DRS_ALPHABLENDENABLE]);
	this->Device->GetRenderState (D3DRS_FOGENABLE, &this->State.RenderStates[D3DRS_FOGENABLE]);
	this->Device->GetRenderState (D3DRS_SPECULARENABLE, &this->State.RenderStates[D3DRS_SPECULARENABLE]);
	this->Device->GetRenderState (D3DRS_FOGCOLOR, &this->State.RenderStates[D3DRS_FOGCOLOR]);
	this->Device->GetRenderState (D3DRS_FOGTABLEMODE, &this->State.RenderStates[D3DRS_FOGTABLEMODE]);
	this->Device->GetRenderState (D3DRS_FOGSTART, &this->State.RenderStates[D3DRS_FOGSTART]);
	this->Device->GetRenderState (D3DRS_FOGEND, &this->State.RenderStates[D3DRS_FOGEND]);
	this->Device->GetRenderState (D3DRS_FOGDENSITY, &this->State.RenderStates[D3DRS_FOGDENSITY]);
	this->Device->GetRenderState (D3DRS_RANGEFOGENABLE, &this->State.RenderStates[D3DRS_RANGEFOGENABLE]);
	this->Device->GetRenderState (D3DRS_STENCILENABLE, &this->State.RenderStates[D3DRS_STENCILENABLE]);
	this->Device->GetRenderState (D3DRS_STENCILFAIL, &this->State.RenderStates[D3DRS_STENCILFAIL]);
	this->Device->GetRenderState (D3DRS_STENCILZFAIL, &this->State.RenderStates[D3DRS_STENCILZFAIL]);
	this->Device->GetRenderState (D3DRS_STENCILPASS, &this->State.RenderStates[D3DRS_STENCILPASS]);
	this->Device->GetRenderState (D3DRS_STENCILFUNC, &this->State.RenderStates[D3DRS_STENCILFUNC]);
	this->Device->GetRenderState (D3DRS_STENCILREF, &this->State.RenderStates[D3DRS_STENCILREF]);
	this->Device->GetRenderState (D3DRS_STENCILMASK, &this->State.RenderStates[D3DRS_STENCILMASK]);
	this->Device->GetRenderState (D3DRS_STENCILWRITEMASK, &this->State.RenderStates[D3DRS_STENCILWRITEMASK]);
	this->Device->GetRenderState (D3DRS_TEXTUREFACTOR, &this->State.RenderStates[D3DRS_TEXTUREFACTOR]);
	this->Device->GetRenderState (D3DRS_WRAP0, &this->State.RenderStates[D3DRS_WRAP0]);
	this->Device->GetRenderState (D3DRS_WRAP1, &this->State.RenderStates[D3DRS_WRAP1]);
	this->Device->GetRenderState (D3DRS_WRAP2, &this->State.RenderStates[D3DRS_WRAP2]);
	this->Device->GetRenderState (D3DRS_WRAP3, &this->State.RenderStates[D3DRS_WRAP3]);
	this->Device->GetRenderState (D3DRS_WRAP4, &this->State.RenderStates[D3DRS_WRAP4]);
	this->Device->GetRenderState (D3DRS_WRAP5, &this->State.RenderStates[D3DRS_WRAP5]);
	this->Device->GetRenderState (D3DRS_WRAP6, &this->State.RenderStates[D3DRS_WRAP6]);
	this->Device->GetRenderState (D3DRS_WRAP7, &this->State.RenderStates[D3DRS_WRAP7]);
	this->Device->GetRenderState (D3DRS_CLIPPING, &this->State.RenderStates[D3DRS_CLIPPING]);
	this->Device->GetRenderState (D3DRS_LIGHTING, &this->State.RenderStates[D3DRS_LIGHTING]);
	this->Device->GetRenderState (D3DRS_AMBIENT, &this->State.RenderStates[D3DRS_AMBIENT]);
	this->Device->GetRenderState (D3DRS_FOGVERTEXMODE, &this->State.RenderStates[D3DRS_FOGVERTEXMODE]);
	this->Device->GetRenderState (D3DRS_COLORVERTEX, &this->State.RenderStates[D3DRS_COLORVERTEX]);
	this->Device->GetRenderState (D3DRS_LOCALVIEWER, &this->State.RenderStates[D3DRS_LOCALVIEWER]);
	this->Device->GetRenderState (D3DRS_NORMALIZENORMALS, &this->State.RenderStates[D3DRS_NORMALIZENORMALS]);
	this->Device->GetRenderState (D3DRS_DIFFUSEMATERIALSOURCE, &this->State.RenderStates[D3DRS_DIFFUSEMATERIALSOURCE]);
	this->Device->GetRenderState (D3DRS_SPECULARMATERIALSOURCE, &this->State.RenderStates[D3DRS_SPECULARMATERIALSOURCE]);
	this->Device->GetRenderState (D3DRS_AMBIENTMATERIALSOURCE, &this->State.RenderStates[D3DRS_AMBIENTMATERIALSOURCE]);
	this->Device->GetRenderState (D3DRS_EMISSIVEMATERIALSOURCE, &this->State.RenderStates[D3DRS_EMISSIVEMATERIALSOURCE]);
	this->Device->GetRenderState (D3DRS_VERTEXBLEND, &this->State.RenderStates[D3DRS_VERTEXBLEND]);
	this->Device->GetRenderState (D3DRS_CLIPPLANEENABLE, &this->State.RenderStates[D3DRS_CLIPPLANEENABLE]);
	this->Device->GetRenderState (D3DRS_POINTSIZE, &this->State.RenderStates[D3DRS_POINTSIZE]);
	this->Device->GetRenderState (D3DRS_POINTSIZE_MIN, &this->State.RenderStates[D3DRS_POINTSIZE_MIN]);
	this->Device->GetRenderState (D3DRS_POINTSPRITEENABLE, &this->State.RenderStates[D3DRS_POINTSPRITEENABLE]);
	this->Device->GetRenderState (D3DRS_POINTSCALEENABLE, &this->State.RenderStates[D3DRS_POINTSCALEENABLE]);
	this->Device->GetRenderState (D3DRS_POINTSCALE_A, &this->State.RenderStates[D3DRS_POINTSCALE_A]);
	this->Device->GetRenderState (D3DRS_POINTSCALE_B, &this->State.RenderStates[D3DRS_POINTSCALE_B]);
	this->Device->GetRenderState (D3DRS_POINTSCALE_C, &this->State.RenderStates[D3DRS_POINTSCALE_C]);
	this->Device->GetRenderState (D3DRS_MULTISAMPLEANTIALIAS, &this->State.RenderStates[D3DRS_MULTISAMPLEANTIALIAS]);
	this->Device->GetRenderState (D3DRS_MULTISAMPLEMASK, &this->State.RenderStates[D3DRS_MULTISAMPLEMASK]);
	this->Device->GetRenderState (D3DRS_PATCHEDGESTYLE, &this->State.RenderStates[D3DRS_PATCHEDGESTYLE]);
	this->Device->GetRenderState (D3DRS_DEBUGMONITORTOKEN, &this->State.RenderStates[D3DRS_DEBUGMONITORTOKEN]);
	this->Device->GetRenderState (D3DRS_POINTSIZE_MAX, &this->State.RenderStates[D3DRS_POINTSIZE_MAX]);
	this->Device->GetRenderState (D3DRS_INDEXEDVERTEXBLENDENABLE, &this->State.RenderStates[D3DRS_INDEXEDVERTEXBLENDENABLE]);
	this->Device->GetRenderState (D3DRS_COLORWRITEENABLE, &this->State.RenderStates[D3DRS_COLORWRITEENABLE]);
	this->Device->GetRenderState (D3DRS_TWEENFACTOR, &this->State.RenderStates[D3DRS_TWEENFACTOR]);
	this->Device->GetRenderState (D3DRS_BLENDOP, &this->State.RenderStates[D3DRS_BLENDOP]);
	this->Device->GetRenderState (D3DRS_POSITIONDEGREE, &this->State.RenderStates[D3DRS_POSITIONDEGREE]);
	this->Device->GetRenderState (D3DRS_NORMALDEGREE, &this->State.RenderStates[D3DRS_NORMALDEGREE]);
	this->Device->GetRenderState (D3DRS_SCISSORTESTENABLE, &this->State.RenderStates[D3DRS_SCISSORTESTENABLE]);
	this->Device->GetRenderState (D3DRS_SLOPESCALEDEPTHBIAS, &this->State.RenderStates[D3DRS_SLOPESCALEDEPTHBIAS]);
	this->Device->GetRenderState (D3DRS_ANTIALIASEDLINEENABLE, &this->State.RenderStates[D3DRS_ANTIALIASEDLINEENABLE]);
	this->Device->GetRenderState (D3DRS_MINTESSELLATIONLEVEL, &this->State.RenderStates[D3DRS_MINTESSELLATIONLEVEL]);
	this->Device->GetRenderState (D3DRS_MAXTESSELLATIONLEVEL, &this->State.RenderStates[D3DRS_MAXTESSELLATIONLEVEL]);
	this->Device->GetRenderState (D3DRS_ADAPTIVETESS_X, &this->State.RenderStates[D3DRS_ADAPTIVETESS_X]);
	this->Device->GetRenderState (D3DRS_ADAPTIVETESS_Y, &this->State.RenderStates[D3DRS_ADAPTIVETESS_Y]);
	this->Device->GetRenderState (D3DRS_ADAPTIVETESS_Z, &this->State.RenderStates[D3DRS_ADAPTIVETESS_Z]);
	this->Device->GetRenderState (D3DRS_ADAPTIVETESS_W, &this->State.RenderStates[D3DRS_ADAPTIVETESS_W]);
	this->Device->GetRenderState (D3DRS_ENABLEADAPTIVETESSELLATION, &this->State.RenderStates[D3DRS_ENABLEADAPTIVETESSELLATION]);
	this->Device->GetRenderState (D3DRS_TWOSIDEDSTENCILMODE, &this->State.RenderStates[D3DRS_TWOSIDEDSTENCILMODE]);
	this->Device->GetRenderState (D3DRS_CCW_STENCILFAIL, &this->State.RenderStates[D3DRS_CCW_STENCILFAIL]);
	this->Device->GetRenderState (D3DRS_CCW_STENCILZFAIL, &this->State.RenderStates[D3DRS_CCW_STENCILZFAIL]);
	this->Device->GetRenderState (D3DRS_CCW_STENCILPASS, &this->State.RenderStates[D3DRS_CCW_STENCILPASS]);
	this->Device->GetRenderState (D3DRS_CCW_STENCILFUNC, &this->State.RenderStates[D3DRS_CCW_STENCILFUNC]);
	this->Device->GetRenderState (D3DRS_COLORWRITEENABLE1, &this->State.RenderStates[D3DRS_COLORWRITEENABLE1]);
	this->Device->GetRenderState (D3DRS_COLORWRITEENABLE2, &this->State.RenderStates[D3DRS_COLORWRITEENABLE2]);
	this->Device->GetRenderState (D3DRS_COLORWRITEENABLE3, &this->State.RenderStates[D3DRS_COLORWRITEENABLE3]);
	this->Device->GetRenderState (D3DRS_BLENDFACTOR, &this->State.RenderStates[D3DRS_BLENDFACTOR]);
	this->Device->GetRenderState (D3DRS_SRGBWRITEENABLE, &this->State.RenderStates[D3DRS_SRGBWRITEENABLE]);
	this->Device->GetRenderState (D3DRS_DEPTHBIAS, &this->State.RenderStates[D3DRS_DEPTHBIAS]);
	this->Device->GetRenderState (D3DRS_WRAP8, &this->State.RenderStates[D3DRS_WRAP8]);
	this->Device->GetRenderState (D3DRS_WRAP9, &this->State.RenderStates[D3DRS_WRAP9]);
	this->Device->GetRenderState (D3DRS_WRAP10, &this->State.RenderStates[D3DRS_WRAP10]);
	this->Device->GetRenderState (D3DRS_WRAP11, &this->State.RenderStates[D3DRS_WRAP11]);
	this->Device->GetRenderState (D3DRS_WRAP12, &this->State.RenderStates[D3DRS_WRAP12]);
	this->Device->GetRenderState (D3DRS_WRAP13, &this->State.RenderStates[D3DRS_WRAP13]);
	this->Device->GetRenderState (D3DRS_WRAP14, &this->State.RenderStates[D3DRS_WRAP14]);
	this->Device->GetRenderState (D3DRS_WRAP15, &this->State.RenderStates[D3DRS_WRAP15]);
	this->Device->GetRenderState (D3DRS_SEPARATEALPHABLENDENABLE, &this->State.RenderStates[D3DRS_SEPARATEALPHABLENDENABLE]);
	this->Device->GetRenderState (D3DRS_SRCBLENDALPHA, &this->State.RenderStates[D3DRS_SRCBLENDALPHA]);
	this->Device->GetRenderState (D3DRS_DESTBLENDALPHA, &this->State.RenderStates[D3DRS_DESTBLENDALPHA]);
	this->Device->GetRenderState (D3DRS_BLENDOPALPHA, &this->State.RenderStates[D3DRS_BLENDOPALPHA]);
}


void context_t::SetRenderStates (void)
{
	this->FlushGeometry ();

	// this forces a set of all render states with the current cached values rather than filtering, to
	// ensure that they are properly updated
	this->Device->SetRenderState (D3DRS_ZENABLE, this->State.RenderStates[D3DRS_ZENABLE]);
	this->Device->SetRenderState (D3DRS_FILLMODE, this->State.RenderStates[D3DRS_FILLMODE]);
	this->Device->SetRenderState (D3DRS_SHADEMODE, this->State.RenderStates[D3DRS_SHADEMODE]);
	this->Device->SetRenderState (D3DRS_ZWRITEENABLE, this->State.RenderStates[D3DRS_ZWRITEENABLE]);
	this->Device->SetRenderState (D3DRS_ALPHATESTENABLE, this->State.RenderStates[D3DRS_ALPHATESTENABLE]);
	this->Device->SetRenderState (D3DRS_LASTPIXEL, this->State.RenderStates[D3DRS_LASTPIXEL]);
	this->Device->SetRenderState (D3DRS_SRCBLEND, this->State.RenderStates[D3DRS_SRCBLEND]);
	this->Device->SetRenderState (D3DRS_DESTBLEND, this->State.RenderStates[D3DRS_DESTBLEND]);
	this->Device->SetRenderState (D3DRS_CULLMODE, this->State.RenderStates[D3DRS_CULLMODE]);
	this->Device->SetRenderState (D3DRS_ZFUNC, this->State.RenderStates[D3DRS_ZFUNC]);
	this->Device->SetRenderState (D3DRS_ALPHAREF, this->State.RenderStates[D3DRS_ALPHAREF]);
	this->Device->SetRenderState (D3DRS_ALPHAFUNC, this->State.RenderStates[D3DRS_ALPHAFUNC]);
	this->Device->SetRenderState (D3DRS_DITHERENABLE, this->State.RenderStates[D3DRS_DITHERENABLE]);
	this->Device->SetRenderState (D3DRS_ALPHABLENDENABLE, this->State.RenderStates[D3DRS_ALPHABLENDENABLE]);
	this->Device->SetRenderState (D3DRS_FOGENABLE, this->State.RenderStates[D3DRS_FOGENABLE]);
	this->Device->SetRenderState (D3DRS_SPECULARENABLE, this->State.RenderStates[D3DRS_SPECULARENABLE]);
	this->Device->SetRenderState (D3DRS_FOGCOLOR, this->State.RenderStates[D3DRS_FOGCOLOR]);
	this->Device->SetRenderState (D3DRS_FOGTABLEMODE, this->State.RenderStates[D3DRS_FOGTABLEMODE]);
	this->Device->SetRenderState (D3DRS_FOGSTART, this->State.RenderStates[D3DRS_FOGSTART]);
	this->Device->SetRenderState (D3DRS_FOGEND, this->State.RenderStates[D3DRS_FOGEND]);
	this->Device->SetRenderState (D3DRS_FOGDENSITY, this->State.RenderStates[D3DRS_FOGDENSITY]);
	this->Device->SetRenderState (D3DRS_RANGEFOGENABLE, this->State.RenderStates[D3DRS_RANGEFOGENABLE]);
	this->Device->SetRenderState (D3DRS_STENCILENABLE, this->State.RenderStates[D3DRS_STENCILENABLE]);
	this->Device->SetRenderState (D3DRS_STENCILFAIL, this->State.RenderStates[D3DRS_STENCILFAIL]);
	this->Device->SetRenderState (D3DRS_STENCILZFAIL, this->State.RenderStates[D3DRS_STENCILZFAIL]);
	this->Device->SetRenderState (D3DRS_STENCILPASS, this->State.RenderStates[D3DRS_STENCILPASS]);
	this->Device->SetRenderState (D3DRS_STENCILFUNC, this->State.RenderStates[D3DRS_STENCILFUNC]);
	this->Device->SetRenderState (D3DRS_STENCILREF, this->State.RenderStates[D3DRS_STENCILREF]);
	this->Device->SetRenderState (D3DRS_STENCILMASK, this->State.RenderStates[D3DRS_STENCILMASK]);
	this->Device->SetRenderState (D3DRS_STENCILWRITEMASK, this->State.RenderStates[D3DRS_STENCILWRITEMASK]);
	this->Device->SetRenderState (D3DRS_TEXTUREFACTOR, this->State.RenderStates[D3DRS_TEXTUREFACTOR]);
	this->Device->SetRenderState (D3DRS_WRAP0, this->State.RenderStates[D3DRS_WRAP0]);
	this->Device->SetRenderState (D3DRS_WRAP1, this->State.RenderStates[D3DRS_WRAP1]);
	this->Device->SetRenderState (D3DRS_WRAP2, this->State.RenderStates[D3DRS_WRAP2]);
	this->Device->SetRenderState (D3DRS_WRAP3, this->State.RenderStates[D3DRS_WRAP3]);
	this->Device->SetRenderState (D3DRS_WRAP4, this->State.RenderStates[D3DRS_WRAP4]);
	this->Device->SetRenderState (D3DRS_WRAP5, this->State.RenderStates[D3DRS_WRAP5]);
	this->Device->SetRenderState (D3DRS_WRAP6, this->State.RenderStates[D3DRS_WRAP6]);
	this->Device->SetRenderState (D3DRS_WRAP7, this->State.RenderStates[D3DRS_WRAP7]);
	this->Device->SetRenderState (D3DRS_CLIPPING, this->State.RenderStates[D3DRS_CLIPPING]);
	this->Device->SetRenderState (D3DRS_LIGHTING, this->State.RenderStates[D3DRS_LIGHTING]);
	this->Device->SetRenderState (D3DRS_AMBIENT, this->State.RenderStates[D3DRS_AMBIENT]);
	this->Device->SetRenderState (D3DRS_FOGVERTEXMODE, this->State.RenderStates[D3DRS_FOGVERTEXMODE]);
	this->Device->SetRenderState (D3DRS_COLORVERTEX, this->State.RenderStates[D3DRS_COLORVERTEX]);
	this->Device->SetRenderState (D3DRS_LOCALVIEWER, this->State.RenderStates[D3DRS_LOCALVIEWER]);
	this->Device->SetRenderState (D3DRS_NORMALIZENORMALS, this->State.RenderStates[D3DRS_NORMALIZENORMALS]);
	this->Device->SetRenderState (D3DRS_DIFFUSEMATERIALSOURCE, this->State.RenderStates[D3DRS_DIFFUSEMATERIALSOURCE]);
	this->Device->SetRenderState (D3DRS_SPECULARMATERIALSOURCE, this->State.RenderStates[D3DRS_SPECULARMATERIALSOURCE]);
	this->Device->SetRenderState (D3DRS_AMBIENTMATERIALSOURCE, this->State.RenderStates[D3DRS_AMBIENTMATERIALSOURCE]);
	this->Device->SetRenderState (D3DRS_EMISSIVEMATERIALSOURCE, this->State.RenderStates[D3DRS_EMISSIVEMATERIALSOURCE]);
	this->Device->SetRenderState (D3DRS_VERTEXBLEND, this->State.RenderStates[D3DRS_VERTEXBLEND]);
	this->Device->SetRenderState (D3DRS_CLIPPLANEENABLE, this->State.RenderStates[D3DRS_CLIPPLANEENABLE]);
	this->Device->SetRenderState (D3DRS_POINTSIZE, this->State.RenderStates[D3DRS_POINTSIZE]);
	this->Device->SetRenderState (D3DRS_POINTSIZE_MIN, this->State.RenderStates[D3DRS_POINTSIZE_MIN]);
	this->Device->SetRenderState (D3DRS_POINTSPRITEENABLE, this->State.RenderStates[D3DRS_POINTSPRITEENABLE]);
	this->Device->SetRenderState (D3DRS_POINTSCALEENABLE, this->State.RenderStates[D3DRS_POINTSCALEENABLE]);
	this->Device->SetRenderState (D3DRS_POINTSCALE_A, this->State.RenderStates[D3DRS_POINTSCALE_A]);
	this->Device->SetRenderState (D3DRS_POINTSCALE_B, this->State.RenderStates[D3DRS_POINTSCALE_B]);
	this->Device->SetRenderState (D3DRS_POINTSCALE_C, this->State.RenderStates[D3DRS_POINTSCALE_C]);
	this->Device->SetRenderState (D3DRS_MULTISAMPLEANTIALIAS, this->State.RenderStates[D3DRS_MULTISAMPLEANTIALIAS]);
	this->Device->SetRenderState (D3DRS_MULTISAMPLEMASK, this->State.RenderStates[D3DRS_MULTISAMPLEMASK]);
	this->Device->SetRenderState (D3DRS_PATCHEDGESTYLE, this->State.RenderStates[D3DRS_PATCHEDGESTYLE]);
	this->Device->SetRenderState (D3DRS_DEBUGMONITORTOKEN, this->State.RenderStates[D3DRS_DEBUGMONITORTOKEN]);
	this->Device->SetRenderState (D3DRS_POINTSIZE_MAX, this->State.RenderStates[D3DRS_POINTSIZE_MAX]);
	this->Device->SetRenderState (D3DRS_INDEXEDVERTEXBLENDENABLE, this->State.RenderStates[D3DRS_INDEXEDVERTEXBLENDENABLE]);
	this->Device->SetRenderState (D3DRS_COLORWRITEENABLE, this->State.RenderStates[D3DRS_COLORWRITEENABLE]);
	this->Device->SetRenderState (D3DRS_TWEENFACTOR, this->State.RenderStates[D3DRS_TWEENFACTOR]);
	this->Device->SetRenderState (D3DRS_BLENDOP, this->State.RenderStates[D3DRS_BLENDOP]);
	this->Device->SetRenderState (D3DRS_POSITIONDEGREE, this->State.RenderStates[D3DRS_POSITIONDEGREE]);
	this->Device->SetRenderState (D3DRS_NORMALDEGREE, this->State.RenderStates[D3DRS_NORMALDEGREE]);
	this->Device->SetRenderState (D3DRS_SCISSORTESTENABLE, this->State.RenderStates[D3DRS_SCISSORTESTENABLE]);
	this->Device->SetRenderState (D3DRS_SLOPESCALEDEPTHBIAS, this->State.RenderStates[D3DRS_SLOPESCALEDEPTHBIAS]);
	this->Device->SetRenderState (D3DRS_ANTIALIASEDLINEENABLE, this->State.RenderStates[D3DRS_ANTIALIASEDLINEENABLE]);
	this->Device->SetRenderState (D3DRS_MINTESSELLATIONLEVEL, this->State.RenderStates[D3DRS_MINTESSELLATIONLEVEL]);
	this->Device->SetRenderState (D3DRS_MAXTESSELLATIONLEVEL, this->State.RenderStates[D3DRS_MAXTESSELLATIONLEVEL]);
	this->Device->SetRenderState (D3DRS_ADAPTIVETESS_X, this->State.RenderStates[D3DRS_ADAPTIVETESS_X]);
	this->Device->SetRenderState (D3DRS_ADAPTIVETESS_Y, this->State.RenderStates[D3DRS_ADAPTIVETESS_Y]);
	this->Device->SetRenderState (D3DRS_ADAPTIVETESS_Z, this->State.RenderStates[D3DRS_ADAPTIVETESS_Z]);
	this->Device->SetRenderState (D3DRS_ADAPTIVETESS_W, this->State.RenderStates[D3DRS_ADAPTIVETESS_W]);
	this->Device->SetRenderState (D3DRS_ENABLEADAPTIVETESSELLATION, this->State.RenderStates[D3DRS_ENABLEADAPTIVETESSELLATION]);
	this->Device->SetRenderState (D3DRS_TWOSIDEDSTENCILMODE, this->State.RenderStates[D3DRS_TWOSIDEDSTENCILMODE]);
	this->Device->SetRenderState (D3DRS_CCW_STENCILFAIL, this->State.RenderStates[D3DRS_CCW_STENCILFAIL]);
	this->Device->SetRenderState (D3DRS_CCW_STENCILZFAIL, this->State.RenderStates[D3DRS_CCW_STENCILZFAIL]);
	this->Device->SetRenderState (D3DRS_CCW_STENCILPASS, this->State.RenderStates[D3DRS_CCW_STENCILPASS]);
	this->Device->SetRenderState (D3DRS_CCW_STENCILFUNC, this->State.RenderStates[D3DRS_CCW_STENCILFUNC]);
	this->Device->SetRenderState (D3DRS_COLORWRITEENABLE1, this->State.RenderStates[D3DRS_COLORWRITEENABLE1]);
	this->Device->SetRenderState (D3DRS_COLORWRITEENABLE2, this->State.RenderStates[D3DRS_COLORWRITEENABLE2]);
	this->Device->SetRenderState (D3DRS_COLORWRITEENABLE3, this->State.RenderStates[D3DRS_COLORWRITEENABLE3]);
	this->Device->SetRenderState (D3DRS_BLENDFACTOR, this->State.RenderStates[D3DRS_BLENDFACTOR]);
	this->Device->SetRenderState (D3DRS_SRGBWRITEENABLE, this->State.RenderStates[D3DRS_SRGBWRITEENABLE]);
	this->Device->SetRenderState (D3DRS_DEPTHBIAS, this->State.RenderStates[D3DRS_DEPTHBIAS]);
	this->Device->SetRenderState (D3DRS_WRAP8, this->State.RenderStates[D3DRS_WRAP8]);
	this->Device->SetRenderState (D3DRS_WRAP9, this->State.RenderStates[D3DRS_WRAP9]);
	this->Device->SetRenderState (D3DRS_WRAP10, this->State.RenderStates[D3DRS_WRAP10]);
	this->Device->SetRenderState (D3DRS_WRAP11, this->State.RenderStates[D3DRS_WRAP11]);
	this->Device->SetRenderState (D3DRS_WRAP12, this->State.RenderStates[D3DRS_WRAP12]);
	this->Device->SetRenderState (D3DRS_WRAP13, this->State.RenderStates[D3DRS_WRAP13]);
	this->Device->SetRenderState (D3DRS_WRAP14, this->State.RenderStates[D3DRS_WRAP14]);
	this->Device->SetRenderState (D3DRS_WRAP15, this->State.RenderStates[D3DRS_WRAP15]);
	this->Device->SetRenderState (D3DRS_SEPARATEALPHABLENDENABLE, this->State.RenderStates[D3DRS_SEPARATEALPHABLENDENABLE]);
	this->Device->SetRenderState (D3DRS_SRCBLENDALPHA, this->State.RenderStates[D3DRS_SRCBLENDALPHA]);
	this->Device->SetRenderState (D3DRS_DESTBLENDALPHA, this->State.RenderStates[D3DRS_DESTBLENDALPHA]);
	this->Device->SetRenderState (D3DRS_BLENDOPALPHA, this->State.RenderStates[D3DRS_BLENDOPALPHA]);

	// set up anything else we need
	this->State.PolygonOffset.FillEnabled = FALSE;
	this->State.PolygonOffset.LineEnabled = FALSE;
	this->State.PolygonOffset.PointEnabled = FALSE;
	this->State.PolygonOffset.Factor = 0;
	this->State.PolygonOffset.Units = 0;

	this->SetRenderState (D3DRS_LIGHTING, FALSE);
}


void context_t::CheckPolygonOffset (GLenum mode)
{
	switch (mode)
	{
	case GL_QUAD_STRIP:
	case GL_TRIANGLE_STRIP:
	case GL_POLYGON:
	case GL_TRIANGLE_FAN:
	case GL_TRIANGLES:
	case GL_QUADS:
		if (this->State.PolygonOffset.FillEnabled && this->State.RenderStates[D3DRS_FILLMODE] == D3DFILL_SOLID)
		{
			this->SetRenderState (D3DRS_SLOPESCALEDEPTHBIAS, D3D_FloatToDWORD (this->State.PolygonOffset.Factor));
			this->SetRenderState (D3DRS_DEPTHBIAS, D3D_FloatToDWORD (this->State.PolygonOffset.Units));
		}
		else if (this->State.PolygonOffset.LineEnabled && this->State.RenderStates[D3DRS_FILLMODE] == D3DFILL_WIREFRAME)
		{
			this->SetRenderState (D3DRS_SLOPESCALEDEPTHBIAS, D3D_FloatToDWORD (this->State.PolygonOffset.Factor));
			this->SetRenderState (D3DRS_DEPTHBIAS, D3D_FloatToDWORD (this->State.PolygonOffset.Units));
		}
		else if (this->State.PolygonOffset.PointEnabled && this->State.RenderStates[D3DRS_FILLMODE] == D3DFILL_POINT)
		{
			this->SetRenderState (D3DRS_SLOPESCALEDEPTHBIAS, D3D_FloatToDWORD (this->State.PolygonOffset.Factor));
			this->SetRenderState (D3DRS_DEPTHBIAS, D3D_FloatToDWORD (this->State.PolygonOffset.Units));
		}
		else // wow!
	default:
		{
			this->SetRenderState (D3DRS_SLOPESCALEDEPTHBIAS, D3D_FloatToDWORD (0));
			this->SetRenderState (D3DRS_DEPTHBIAS, D3D_FloatToDWORD (0));
		}
		break;
	}
}


void context_t::DirtyAllStates (void)
{
	this->FlushGeometry ();

	// dirty TMUs
	for (int i = 0; i < D3D_MAX_TMUS; i++)
	{
		this->TMU[i].texenvdirty = TRUE;
		this->TMU[i].texparamdirty = TRUE;
		this->State.BoundTextures[i] = NULL;
	}

	// and these as well
	this->DirtyMatrixes ();
}


void context_t::InitStates (void)
{
	this->FlushGeometry ();

	this->InitTexEnv ();

	// store out all states
	this->GetRenderStates ();
	this->GetTextureStates ();

	// force all states to dirty on entry
	this->DirtyAllStates ();
}


void context_t::SetCompFunc (D3DRENDERSTATETYPE mode, GLenum func)
{
	switch (func)
	{
	case GL_NEVER: this->SetRenderState (mode, D3DCMP_NEVER); break;
	case GL_LESS: this->SetRenderState (mode, D3DCMP_LESS); break;
	case GL_LEQUAL: this->SetRenderState (mode, D3DCMP_LESSEQUAL); break;
	case GL_EQUAL: this->SetRenderState (mode, D3DCMP_EQUAL); break;
	case GL_GREATER: this->SetRenderState (mode, D3DCMP_GREATER); break;
	case GL_NOTEQUAL: this->SetRenderState (mode, D3DCMP_NOTEQUAL); break;
	case GL_GEQUAL: this->SetRenderState (mode, D3DCMP_GREATEREQUAL); break;
	case GL_ALWAYS: this->SetRenderState (mode, D3DCMP_ALWAYS); break;
	default: break;
	}
}


void context_t::UpdateCull (void)
{
	if (this->State.Cull.Enable == GL_FALSE)
	{
		// disable culling
		this->SetRenderState (D3DRS_CULLMODE, D3DCULL_NONE);
		return;
	}

	if (this->State.Cull.FrontFace == GL_CCW)
	{
		if (this->State.Cull.Mode == GL_BACK)
			this->SetRenderState (D3DRS_CULLMODE, D3DCULL_CW);
		else if (this->State.Cull.Mode == GL_FRONT)
			this->SetRenderState (D3DRS_CULLMODE, D3DCULL_CCW);
		else if (this->State.Cull.Mode == GL_FRONT_AND_BACK)
			; // do nothing; we cull in software in GL_SubmitVertexes instead
		else System_Error ("context_t::UpdateCull: illegal glCullFace");
	}
	else if (this->State.Cull.FrontFace == GL_CW)
	{
		if (this->State.Cull.Mode == GL_BACK)
			this->SetRenderState (D3DRS_CULLMODE, D3DCULL_CCW);
		else if (this->State.Cull.Mode == GL_FRONT)
			this->SetRenderState (D3DRS_CULLMODE, D3DCULL_CW);
		else if (this->State.Cull.Mode == GL_FRONT_AND_BACK)
			; // do nothing; we cull in software in GL_SubmitVertexes instead
		else System_Error ("context_t::UpdateCull: illegal glCullFace");
	}
	else System_Error ("context_t::UpdateCull: illegal glFrontFace");
}


void context_t::EnableDisable (GLenum cap, BOOL enable)
{
	D3DRENDERSTATETYPE rs;

	switch (cap)
	{
	case GL_STENCIL_TEST:
		rs = D3DRS_STENCILENABLE;
		break;

	case GL_SCISSOR_TEST:
		rs = D3DRS_SCISSORTESTENABLE;
		break;

	case GL_BLEND:
		rs = D3DRS_ALPHABLENDENABLE;
		break;

	case GL_ALPHA_TEST:
		rs = D3DRS_ALPHATESTENABLE;
		break;

	case GL_TEXTURE_2D:
		// this is a texture stage state rather than a render state
		if ((enable && !this->TMU[this->State.CurrentTMU].enabled) || (!enable && this->TMU[this->State.CurrentTMU].enabled))
		{
			this->TMU[this->State.CurrentTMU].enabled = enable;
			this->TMU[this->State.CurrentTMU].texenvdirty = TRUE;
			this->TMU[this->State.CurrentTMU].texparamdirty = TRUE;
		}

		// we're not setting state yet here...
		return;

	case GL_CULL_FACE:
		this->State.Cull.Enable = enable ? GL_TRUE : GL_FALSE;
		this->UpdateCull ();

		// we're not setting state yet here...
		return;

	case GL_FOG:
		rs = D3DRS_FOGENABLE;
		break;

	case GL_DEPTH_TEST:
		rs = D3DRS_ZENABLE;
		if (enable) enable = D3DZB_TRUE; else enable = D3DZB_FALSE;
		break;

		// we're not setting state yet here...
	case GL_POLYGON_OFFSET_FILL: this->State.PolygonOffset.FillEnabled = enable; return;
	case GL_POLYGON_OFFSET_LINE: this->State.PolygonOffset.LineEnabled = enable; return;
	case GL_POLYGON_OFFSET_POINT: this->State.PolygonOffset.PointEnabled = enable; return;

	default:
		System_Error ("context_t::EnableDisable : unknown param");
		return;
	}

	this->SetRenderState (rs, enable);
}


#endif

