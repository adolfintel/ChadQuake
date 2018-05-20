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

static const char *GAndCShaderSource = " \
struct VS_INOUT \n \
{ \n \
	float4 Position: POSITION0; \n \
	float2 TexCoord: TEXCOORD0; \n \
}; \n \
 \n \
float4 RTInverseSize : register(c8); \n \
 \n \
VS_INOUT GammaContrastVS (VS_INOUT vs_in) \n \
{ \n \
	VS_INOUT vs_out; \n \
	float4 outPosition = vs_in.Position; \n \
 \n \
	outPosition.xy += RTInverseSize.xy * outPosition.w; \n \
 \n \
	vs_out.Position = outPosition; \n \
	vs_out.TexCoord = vs_in.TexCoord; \n \
 \n \
	return vs_out; \n \
} \n \
 \n \
texture tmu0Texture : register(t0); \n \
sampler tmu0Sampler : register(s0) = sampler_state { texture = <tmu0Texture>; }; \n \
 \n \
float4 Gamma : register(c0); \n \
float4 Contrast : register(c1); \n \
 \n \
float4 GammaContrastPS (VS_INOUT ps_in) : COLOR0 \n \
{ \n \
	return pow (tex2D (tmu0Sampler, ps_in.TexCoord) * Contrast, Gamma); \n \
}";

IDirect3DTexture9 *rttex;

void context_t::InitGammaAndContrast (void)
{
	D3DVERTEXELEMENT9 layout[] = {
		VDECL (0, 0, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_POSITION, 0),
		VDECL (0, 8, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 0),
		D3DDECL_END ()
	};

	this->Device->CreateVertexDeclaration (layout, &this->GAndCVD);

	this->GAndCVS = this->CreateVertexShader (GAndCShaderSource, strlen (GAndCShaderSource), "GammaContrastVS");
	this->GAndCPS = this->CreatePixelShader (GAndCShaderSource, strlen (GAndCShaderSource), "GammaContrastPS");

	IDirect3DSurface9 *bbsurf;

	if (SUCCEEDED (this->Device->GetRenderTarget (0, &bbsurf)))
	{
		D3DSURFACE_DESC desc;

		if (SUCCEEDED (bbsurf->GetDesc (&desc)))
		{
			this->Device->CreateTexture (
				desc.Width,
				desc.Height,
				1,
				D3DUSAGE_RENDERTARGET,
				desc.Format,
				D3DPOOL_DEFAULT,
				&this->GAndCRT,
				NULL);
		}

		// don't leak
		SAFE_RELEASE (bbsurf);
	}
}


void context_t::DestroyGammaAndContrast (void)
{
	SAFE_RELEASE (this->GAndCRT);
	SAFE_RELEASE (this->GAndCVS);
	SAFE_RELEASE (this->GAndCPS);
	SAFE_RELEASE (this->GAndCVD);
}


BOOL context_t::SetupGammaAndContrast (float gamma, float contrast)
{
	// initially disabled
	this->State.DoingGammaAndContrast = FALSE;

	// not needed
	if (gamma == 1.0f && contrast == 1.0f) return TRUE;

	// this uses default pool resources so don't run it if we have a lost device
	if (this->DeviceLost) return TRUE;

	// check if the objects exist
	// if we're returning FALSE it's because something failed or broke, so clean up all of our objects
	if (!this->GAndCRT) {this->DestroyGammaAndContrast (); return FALSE;}
	if (!this->GAndCVS) {this->DestroyGammaAndContrast (); return FALSE;}
	if (!this->GAndCPS) {this->DestroyGammaAndContrast (); return FALSE;}
	if (!this->GAndCVD) {this->DestroyGammaAndContrast (); return FALSE;}

	if (SUCCEEDED (this->Device->GetRenderTarget (0, &this->State.OriginalRT)))
	{
		IDirect3DSurface9 *rtsurf;

		if (SUCCEEDED (this->GAndCRT->GetSurfaceLevel (0, &rtsurf)))
		{
			if (SUCCEEDED (this->Device->SetRenderTarget (0, rtsurf)))
			{
				// store out and set as active
				this->State.DoingGammaAndContrast = TRUE;
				this->State.Gamma = gamma;
				this->State.Contrast = contrast;

				// don't leak
				SAFE_RELEASE (rtsurf);

				// return success
				return TRUE;
			}
		}

		// don't leak
		SAFE_RELEASE (rtsurf);
	}

	// don't leak
	SAFE_RELEASE (this->State.OriginalRT);

	// if we're returning FALSE it's because something failed or broke, so clean up all of our objects
	this->DestroyGammaAndContrast ();

	return FALSE;
}


typedef struct gandcvert_s
{
	float Position[2];
	float TexCoord[2];
} gandcvert_t;

void context_t::FinishGammaAndContrast (void)
{
	if (!this->State.DoingGammaAndContrast) return;

	static const gandcvert_t verts[] =
	{
		{{-1, -1}, {0, 1}},
		{{-1, 1}, {0, 0}},
		{{1, 1}, {1, 0}},
		{{1, -1}, {1, 1}}
	};

	float gammareg[] = {this->State.Gamma, this->State.Gamma, this->State.Gamma, 1.0f};
	float contrastreg[] = {this->State.Contrast, this->State.Contrast, this->State.Contrast, 1.0f};

	this->Device->SetRenderTarget (0, this->State.OriginalRT);

	// don't leak
	SAFE_RELEASE (this->State.OriginalRT);

	this->Device->SetVertexShader (this->GAndCVS);
	this->Device->SetPixelShader (this->GAndCPS);
	this->Device->SetVertexDeclaration (this->GAndCVD);

	// FitzQuake's GL_SetCanvas could leave the device with a viewport smaller than the RT at this stage so here
	// we do the same as for clearing and save it off, then reset it to the full RT, then do our stuff, then restore it
	D3DVIEWPORT9 saved;
	this->SaveViewport (&saved);

	// now reset and update it
	this->ResetViewport ();
	this->UpdateViewport ();

	// send these ones through the wrapper API rather than calling directly so they'll filter & cache correctly
	d3d9mh_glDisable (GL_DEPTH_TEST);
	d3d9mh_glDisable (GL_CULL_FACE);
	d3d9mh_glDisable (GL_BLEND);
	d3d9mh_glDisable (GL_ALPHA_TEST);

	this->SetTexture (0, this->GAndCRT);
	this->TMU[0].boundtexture = NULL;

	this->SetSamplerState (0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	this->SetSamplerState (0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	this->SetSamplerState (0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	this->SetSamplerState (0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	this->SetSamplerState (0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	// explicitly dirty the TMU so that state will properly refresh
	// texparam is needed because we're using sampler states here
	// texenv is needed because we're using a PS
	this->TMU[0].texenvdirty = TRUE;
	this->TMU[0].texparamdirty = TRUE;

	this->Device->SetPixelShaderConstantF (0, gammareg, 1);
	this->Device->SetPixelShaderConstantF (1, contrastreg, 1);

	this->Device->DrawPrimitiveUP (D3DPT_TRIANGLEFAN, 2, verts, sizeof (gandcvert_t));

	this->Device->SetPixelShader (NULL);

	// resolves "cannot render to a render target that is also used as a texture" because the RT will still be bound at
	// the start of the next frame
	this->SetTexture (0, NULL);

	// now restore the viewport to what was saved
	this->RestoreViewport (&saved);
	this->UpdateViewport ();
}


#endif

