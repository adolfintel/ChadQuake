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
vertex shader interface

this allows us to:
(1) correct the half-texel offset, and (2) implement texture matrix ops in a sane manner
*/

#ifdef DIRECT3D9_WRAPPER
#include "d3d9_internal.h"

// D3D compiler is available on XP from versions 23 to 43 so this is safe to use too
#include <D3Dcompiler.h>

// needed for the shader compiler
// pD3DCompile is defined in D3Dcompiler.h so we don't need to typedef it ourselves
HINSTANCE hInstCompiler = NULL;
pD3DCompile QD3DCompile = NULL;


void D3D_UnloadShaderCompiler (void)
{
	// any future calls into here are errors
	QD3DCompile = NULL;

	// and unload the library
	if (hInstCompiler)
	{
		FreeLibrary (hInstCompiler);
		hInstCompiler = NULL;
	}
}


static const char *VSSourceCode = " \
struct VS_INOUT \n \
{ \n \
	float4 Position: POSITION0; \n \
	float4 Color: COLOR0; \n \
	float4 TexCoord0: TEXCOORD0; \n \
	float4 TexCoord1: TEXCOORD1; \n \
	float4 TexCoord2: TEXCOORD2; \n \
	float4 TexCoord3: TEXCOORD3; \n \
}; \n \
 \n \
 \n \
float4x4 ProjMatrix : register(c0); \n \
float4x4 ViewMatrix : register(c4); \n \
float4 RTInverseSize : register(c8); \n \
 \n \
float4x4 tex0Matrix : register(c10); \n \
float4x4 tex1Matrix : register(c14); \n \
float4x4 tex2Matrix : register(c18); \n \
float4x4 tex3Matrix : register(c22); \n \
 \n \
VS_INOUT MainVS (VS_INOUT vs_in) \n \
{ \n \
	VS_INOUT vs_out; \n \
 \n \
	// correct the dreaded half-texel offset \n \
	float4 outPosition = mul (ProjMatrix, mul (ViewMatrix, vs_in.Position)); \n \
	outPosition.xy += RTInverseSize.xy * outPosition.w; \n \
 \n \
	// copy over the corrected position \n \
	vs_out.Position = outPosition; \n \
 \n \
	// copy over color \n \
	vs_out.Color = vs_in.Color; \n \
 \n \
	// perform texture matrix multiplications \n \
	vs_out.TexCoord0 = mul (tex0Matrix, vs_in.TexCoord0); \n \
	vs_out.TexCoord1 = mul (tex1Matrix, vs_in.TexCoord1); \n \
	vs_out.TexCoord2 = mul (tex2Matrix, vs_in.TexCoord2); \n \
	vs_out.TexCoord3 = mul (tex3Matrix, vs_in.TexCoord3); \n \
 \n \
	return vs_out; \n \
}";


ID3DBlob *D3D_CompileShaderCommon (const char *Source, const int Len, const char *EntryPoint, const char *Profile)
{
	int i;
	ID3DBlob *ShaderBlob = NULL;
	ID3DBlob *ErrorBlob = NULL;
	HRESULT hr;

	// load the shader compiler
	// statically linking to d3dcompiler.lib causes it to use d3dcompiler_47.dll which
	// is not on earlier versions of Windows so link dynamically instead
	if (!hInstCompiler)
	{
		for (i = 99; i > 32; i--)
		{
			if ((hInstCompiler = LoadLibrary (va ("d3dcompiler_%d.dll", i))) != NULL)
			{
				if ((QD3DCompile = (pD3DCompile) GetProcAddress (hInstCompiler, "D3DCompile")) != NULL)
					break;
				else
				{
					FreeLibrary (hInstCompiler);
					hInstCompiler = NULL;
				}
			}
		}
	}

	// totally failed to load
	if (!hInstCompiler) System_Error ("R_CreateShaders : failed to load d3dcompiler.dll");

	// this should never happen
	if (!QD3DCompile) System_Error ("R_CreateShaders : GetProcAddress for \"D3DCompile\" failed");

	hr = QD3DCompile (
		Source,
		Len,
		NULL,
		NULL,
		NULL,
		EntryPoint,
		Profile,
		0,
		0,
		&ShaderBlob,
		&ErrorBlob);

	// these are error conditions because it is expected that the shader will compile
	if (ErrorBlob)
	{
		// error blob may have a value but the shader might have succeeded in compiling
		// typically these are just warnings
		// System_Error ("Shader error:\n%s\n", (char *) ErrorBlob->lpVtbl->GetBufferPointer (ErrorBlob));
		SAFE_RELEASE (ErrorBlob);
	}
	else if (FAILED (hr))
		System_Error ("Terminal shader %s error!!!\n");

	return ShaderBlob;
}


IDirect3DVertexShader9 *context_t::CreateVertexShader (const char *Source, const int Len, const char *EntryPoint)
{
	ID3DBlob *ShaderBlob = NULL;
	IDirect3DVertexShader9 *VSObject = NULL;

	if ((ShaderBlob = D3D_CompileShaderCommon (Source, Len, EntryPoint, "vs_2_0")) != NULL)
	{
		// convert compiled hardware-independent bytecode to a hardware-dependent shader
		if (SUCCEEDED (this->Device->CreateVertexShader ((DWORD *) ShaderBlob->GetBufferPointer (), &VSObject)))
		{
			SAFE_RELEASE (ShaderBlob);
			return VSObject;
		}
	}

	return NULL;
}


IDirect3DPixelShader9 *context_t::CreatePixelShader (const char *Source, const int Len, const char *EntryPoint)
{
	ID3DBlob *ShaderBlob = NULL;
	IDirect3DPixelShader9 *PSObject = NULL;

	if ((ShaderBlob = D3D_CompileShaderCommon (Source, Len, EntryPoint, "ps_2_0")) != NULL)
	{
		// convert compiled hardware-independent bytecode to a hardware-dependent shader
		if (SUCCEEDED (this->Device->CreatePixelShader ((DWORD *) ShaderBlob->GetBufferPointer (), &PSObject)))
		{
			SAFE_RELEASE (ShaderBlob);
			return PSObject;
		}
	}

	return NULL;
}


BOOL context_t::CreateCommonVertexShader (void)
{
	D3DVERTEXELEMENT9 layout[] = {
		VDECL (0, 0, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0),
		VDECL (0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR, 0),
		VDECL (0, 16, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 0),
		VDECL (0, 24, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 1),
		VDECL (0, 32, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 2),
		VDECL (0, 40, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 3),
		D3DDECL_END ()
	};

	this->Device->CreateVertexDeclaration (layout, &this->MainVD);

	if ((this->MainVS = this->CreateVertexShader (VSSourceCode, strlen (VSSourceCode), "MainVS")) != NULL)
		return TRUE;
	else return FALSE;
}


void context_t::DestroyVertexShader (void)
{
	D3D_UnloadShaderCompiler ();

	SAFE_RELEASE (this->MainVS);
	SAFE_RELEASE (this->MainVD);
}


void context_t::ActivateVertexShader (void)
{
	this->Device->SetVertexShader (this->MainVS);
	this->Device->SetVertexDeclaration (this->MainVD);
}


#endif

