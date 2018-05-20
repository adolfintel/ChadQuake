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
texture object interface

a gl texture object is the texture image plus sampler state.
a d3d texture is just the texture image and sampler state is separate.
we mimic the gl behaviour by combining the two.
*/

#ifdef DIRECT3D9_WRAPPER
#include "d3d9_internal.h"


d3d_texture_t::CreateParms_t::CreateParms_t (void)
{
	this->Initialize ();
}


void d3d_texture_t::CreateParms_t::Initialize (void)
{
	this->Width = 0;
	this->Height = 0;
	this->Levels = 0;
	this->Usage = 0;

	// hack hack hack - because D3DFMT_UNKNOWN is defined to 0, a memset 0 will incorrectly identify a wiped texture as having an unknown format
	// so put it to something else instead
	this->Format = D3DFMT_FORCE_DWORD;

	// hack hack hack - because D3DPOOL_DEFAULT is defined to 0, a memset 0 will incorrectly identify a wiped texture as belonging to the default pool
	// so put it to something else instead
	this->Pool = D3DPOOL_FORCE_DWORD;
}


d3d_texture_t::TexParms_t::TexParms_t (void)
{
	this->Initialize ();
}


void d3d_texture_t::TexParms_t::Initialize (void)
{
	// restore default texparams
	this->AddressU = D3DTADDRESS_WRAP;
	this->AddressV = D3DTADDRESS_WRAP;

	// https://www.opengl.org/sdk/docs/man4/html/glTexParameter.xhtml
	// The initial value of GL_TEXTURE_MAG_FILTER is GL_LINEAR.
	// The initial value of GL_TEXTURE_MIN_FILTER is GL_NEAREST_MIPMAP_LINEAR.
	this->MagFilter = D3DTEXF_LINEAR;
	this->MinFilter = D3DTEXF_POINT;
	this->MipFilter = D3DTEXF_LINEAR;

	this->Anisotropy = 1;
}


d3d_texture_t::d3d_texture_t (void)
{
	// ensure that SAFE_RELEASE won't fire and otherwise send through standard initialization
	this->TexImage = NULL;
	this->Initialize ();
}


void d3d_texture_t::Dirty (RECT *dirtyrect)
{
	// mark as dirty and update the dirty rect
	UnionRect (&this->DirtyRect, &this->DirtyRect, dirtyrect);
	this->dirty = TRUE;
}


void d3d_texture_t::Clean (void)
{
	// un-dirty the texture
	SetRect (&this->DirtyRect, 0xffff, 0xffff, 0, 0);
	this->dirty = FALSE;
}


void d3d_texture_t::Initialize (void)
{
	// set up default params
	this->TexParms.Initialize ();
	this->CreateParms.Initialize ();

	// destroy the texture
	SAFE_RELEASE (this->TexImage);

	// un-dirty the texture
	this->Clean ();
}


void context_t::InitTextures (void)
{
	// now setup each individual texture with known good values
	for (int i = 0; i < MAX_D3D_TEXTURES; i++)
	{
		this->Textures[i].Initialize ();
	}
}


void context_t::ReleaseTextures (void)
{
	// now setup each individual texture with known good values
	for (int i = 0; i < MAX_D3D_TEXTURES; i++)
	{
		this->Textures[i].Initialize ();
	}
}


/*
======================
D3D_CheckTextureFormat

Ensures that a given texture format will be available
======================
*/
BOOL D3D_CheckTextureFormat (D3DFORMAT TextureFormat, D3DFORMAT AdapterFormat)
{
	HRESULT hr = d3d_Globals.Object->CheckDeviceFormat (
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		AdapterFormat,
		0,
		D3DRTYPE_TEXTURE,
		TextureFormat
	);

	return SUCCEEDED (hr);
}


void D3D_CopyTexels (byte *srcdata, GLint srcformat, byte *dstdata, GLint dstformat, int srcx, int srcy, int dstx, int dsty, int width, int height)
{
	// generic copying and format conversion for texels and pixels
	int srcbytes = D3D_BytesForFormat (srcformat);
	int dstbytes = D3D_BytesForFormat (dstformat);
	BOOL swapbytes = ((srcformat == GL_RGB || srcformat == GL_RGBA) && (dstformat == GL_BGR_EXT || dstformat == GL_BGRA_EXT) ||
					  (srcformat == GL_BGR_EXT || srcformat == GL_BGRA_EXT) && (dstformat == GL_RGB || dstformat == GL_RGBA));

	int x, y;

	// offset the data (note: offsets may be 0)
	srcdata += (srcy * width + srcx) * srcbytes;
	dstdata += (dsty * width + dstx) * dstbytes;

	if (srcbytes == 1 && dstbytes == 1)
	{
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++, srcdata++, dstdata++)
			{
				dstdata[0] = srcdata[0];
			}
		}
	}
	else if (srcbytes == 1 && dstbytes == 3)
	{
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++, srcdata++, dstdata += 3)
			{
				dstdata[0] = srcdata[0];
				dstdata[1] = srcdata[0];
				dstdata[2] = srcdata[0];
			}
		}
	}
	else if (srcbytes == 1 && dstbytes == 4)
	{
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++, srcdata++, dstdata += 4)
			{
				dstdata[0] = srcdata[0];
				dstdata[1] = srcdata[0];
				dstdata[2] = srcdata[0];
				dstdata[3] = srcdata[0];
			}
		}
	}
	else if (srcbytes == 3 && dstbytes == 1)
	{
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++, srcdata += 3, dstdata++)
			{
				dstdata[0] = ((int) srcdata[0] + (int) srcdata[1] + (int) srcdata[2]) / 3;
			}
		}
	}
	else if (srcbytes == 3 && dstbytes == 3)
	{
		if (swapbytes)
		{
			for (y = 0; y < height; y++)
			{
				for (x = 0; x < width; x++, srcdata += 3, dstdata += 3)
				{
					dstdata[0] = srcdata[2];
					dstdata[1] = srcdata[1];
					dstdata[2] = srcdata[0];
				}
			}
		}
		else
		{
			for (y = 0; y < height; y++)
			{
				for (x = 0; x < width; x++, srcdata += 3, dstdata += 3)
				{
					dstdata[0] = srcdata[0];
					dstdata[1] = srcdata[1];
					dstdata[2] = srcdata[2];
				}
			}
		}
	}
	else if (srcbytes == 3 && dstbytes == 4)
	{
		if (swapbytes)
		{
			for (y = 0; y < height; y++)
			{
				for (x = 0; x < width; x++, srcdata += 3, dstdata += 4)
				{
					dstdata[0] = srcdata[2];
					dstdata[1] = srcdata[1];
					dstdata[2] = srcdata[0];
					dstdata[3] = 255;
				}
			}
		}
		else
		{
			for (y = 0; y < height; y++)
			{
				for (x = 0; x < width; x++, srcdata += 3, dstdata += 4)
				{
					dstdata[0] = srcdata[0];
					dstdata[1] = srcdata[1];
					dstdata[2] = srcdata[2];
					dstdata[3] = 255;
				}
			}
		}
	}
	else if (srcbytes == 4 && dstbytes == 1)
	{
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++, srcdata += 4, dstdata++)
			{
				dstdata[0] = ((int) srcdata[0] + (int) srcdata[1] + (int) srcdata[2]) / 3;
			}
		}
	}
	else if (srcbytes == 4 && dstbytes == 3)
	{
		if (swapbytes)
		{
			for (y = 0; y < height; y++)
			{
				for (x = 0; x < width; x++, srcdata += 4, dstdata += 3)
				{
					dstdata[0] = srcdata[2];
					dstdata[1] = srcdata[1];
					dstdata[2] = srcdata[0];
				}
			}
		}
		else
		{
			for (y = 0; y < height; y++)
			{
				for (x = 0; x < width; x++, srcdata += 4, dstdata += 3)
				{
					dstdata[0] = srcdata[0];
					dstdata[1] = srcdata[1];
					dstdata[2] = srcdata[2];
				}
			}
		}
	}
	else if (srcbytes == 4 && dstbytes == 4)
	{
		if (swapbytes)
		{
			for (y = 0; y < height; y++)
			{
				for (x = 0; x < width; x++, srcdata += 4, dstdata += 4)
				{
					dstdata[0] = srcdata[2];
					dstdata[1] = srcdata[1];
					dstdata[2] = srcdata[0];
					dstdata[3] = srcdata[3];
				}
			}
		}
		else
		{
			for (y = 0; y < height; y++)
			{
				for (x = 0; x < width; x++, srcdata += 4, dstdata += 4)
				{
					dstdata[0] = srcdata[0];
					dstdata[1] = srcdata[1];
					dstdata[2] = srcdata[2];
					dstdata[3] = srcdata[3];
				}
			}
		}
	}
}


void d3d_texture_t::Fill (int level, GLint xoffset, GLint yoffset, int width, int height, GLint srcformat, const void *pixels, DWORD lockflag)
{
	D3DLOCKED_RECT lockrect;
	RECT texrect = {xoffset, yoffset, (xoffset + width), (yoffset + height)};

	// optionally don't update dirty rects when locking; this will have the side-effect of not updating the texture
	// so we flag the texture as being dirty then we can add a dirty rect just before rendering.
	if (SUCCEEDED (this->TexImage->LockRect (level, &lockrect, &texrect, lockflag)))
	{
		// copy - we have to pretend that the dst format is BGRA here otherwise nothing will copy (D3D creates 32-bit/4-byte with BGRA ordering)
		// because we locked the specific tex rect we reset the offsets to 0,0 but retain width and height
		D3D_CopyTexels ((byte *) pixels, srcformat, (byte *) lockrect.pBits, GL_BGRA_EXT, 0, 0, 0, 0, width, height);

		// and unlock
		this->TexImage->UnlockRect (level);

		if (lockflag == D3DLOCK_NO_DIRTY_UPDATE)
		{
			// mark as dirty and update the dirty rect
			this->Dirty (&texrect);
		}
	}
}


void d3d_texture_t::Mipmap (void)
{
	QD3DXFilterTexture (this->TexImage, NULL, 0, D3DX_DEFAULT);
}


void d3d_texture_t::CheckRenderTarget (class context_t *ctx)
{
	// check if the destination texture was created correctly; if not destroy it and recreate it.
	// this is required so that we can use StretchRect to copy from the backbuffer; otherwise
	// we would need to go through the (much slower) D3DXLoadSurfaceFromSurface path
	if (!(this->CreateParms.Usage & D3DUSAGE_RENDERTARGET))
	{
		SAFE_RELEASE (this->TexImage);

		ctx->Device->CreateTexture (
			this->CreateParms.Width,
			this->CreateParms.Height,
			1, //this->CreateParms.Levels,
			D3DUSAGE_RENDERTARGET,
			this->CreateParms.Format,
			D3DPOOL_DEFAULT,
			&this->TexImage,
			NULL);

		// update the texture properties
		this->CreateParms.Levels = 1;
		this->CreateParms.Pool = D3DPOOL_DEFAULT;
		this->CreateParms.Usage = D3DUSAGE_RENDERTARGET;
	}
}


void context_t::CopyFrameBufferToTexture (d3d_texture_t *tex, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	IDirect3DSurface9 *srcsurf;
	IDirect3DSurface9 *dstsurf;
	D3DSURFACE_DESC srcdesc;
	D3DSURFACE_DESC dstdesc;

	this->FlushGeometry ();

	// this uses default pool resources so don't run it if we have a lost device
	if (this->DeviceLost) return;

	if (SUCCEEDED (this->Device->GetRenderTarget (0, &srcsurf)))
	{
		if (SUCCEEDED (srcsurf->GetDesc (&srcdesc)))
		{
			// check if the destination texture was created correctly; if not destroy it and recreate it.
			// this is required so that we can use StretchRect to copy from the backbuffer, otherwise
			// we would need to go through the (much slower) D3DXLoadSurfaceFromSurface path
			tex->CheckRenderTarget (this);

			if (!tex->TexImage)
			{
				tex->CreateParms.Usage &= ~D3DUSAGE_RENDERTARGET;
			}

			if (tex->TexImage) {
				if (SUCCEEDED (tex->TexImage->GetSurfaceLevel (level, &dstsurf)))
				{
					if (SUCCEEDED (dstsurf->GetDesc (&dstdesc)))
					{
						RECT srcrect;
						RECT dstrect;
	
						Adjust_BottomLeftToTopLeft (&srcrect, x, y, width, height, srcdesc.Height);
						Adjust_BottomLeftToTopLeft (&dstrect, xoffset, yoffset, width, height, dstdesc.Height);
	
						this->Device->StretchRect (srcsurf, &srcrect, dstsurf, &dstrect, D3DTEXF_NONE);
					}
	
					SAFE_RELEASE (dstsurf);
				}
			}
		}

		SAFE_RELEASE (srcsurf);
	}
}


void context_t::SaveTextureToMemory (d3d_texture_t *tex, GLint level, GLenum format, GLvoid *pixels)
{
	IDirect3DSurface9 *texsurf;

	tex->CheckDirty (this);

	if (SUCCEEDED (tex->TexImage->GetSurfaceLevel (level, &texsurf)))
	{
		D3DSURFACE_DESC desc;

		if (SUCCEEDED (texsurf->GetDesc (&desc)))
		{
			IDirect3DSurface9 *locksurf;

			// because textures can be different formats we copy it to one big enough to hold them all
			if (SUCCEEDED (this->Device->CreateOffscreenPlainSurface (desc.Width, desc.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &locksurf, NULL)))
			{
				if (SUCCEEDED (QD3DXLoadSurfaceFromSurface (locksurf, NULL, NULL, texsurf, NULL, NULL, D3DX_FILTER_NONE, 0)))
				{
					D3DLOCKED_RECT lockrect;

					// now we have a surface we can lock
					if (SUCCEEDED (locksurf->LockRect (&lockrect, NULL, D3DLOCK_READONLY)))
					{
						D3D_CopyTexels ((byte *) lockrect.pBits, GL_BGRA_EXT, (byte *) pixels, format, 0, 0, 0, 0, desc.Width, desc.Height);
						locksurf->UnlockRect ();
					}
				}

				SAFE_RELEASE (locksurf);
			}
		}

		SAFE_RELEASE (texsurf);
	}
}


void d3d_texture_t::Create (context_t *ctx, GLsizei width, GLsizei height, D3DFORMAT d3dformat)
{
	// overwrite an existing texture - just release it so that we can recreate
	SAFE_RELEASE (this->TexImage);

	// create the texture from the data, creating a full mipmap chain because OpenGL allows partial textures but d3d doesn't;
	// see the rant on the implementation of glTexImage2D for more on this cheerful topic.
	if (FAILED (ctx->Device->CreateTexture (
		width,
		height,
		0,
		0,
		d3dformat,
		D3DPOOL_MANAGED,
		&this->TexImage,
		NULL
	))) System_Error ("context_t::CreateTexture: unable to create a texture");

	// copy over creation parms
	this->CreateParms.Width = width;
	this->CreateParms.Height = height;
	this->CreateParms.Levels = 0;
	this->CreateParms.Usage = 0;
	this->CreateParms.Format = d3dformat;
	this->CreateParms.Pool = D3DPOOL_MANAGED;

	// set intial dirty rect and state
	this->Clean ();
}


void context_t::CreateTexture (d3d_texture_t *tex, GLsizei width, GLsizei height, D3DFORMAT d3dformat)
{
	this->FlushGeometry ();
	tex->Create (this, width, height, d3dformat);
}


void d3d_texture_t::DestroyRT (void)
{
	// the only default pool textures should be render targets which are updated each frame so we don't need to worry about restoring image data
	if (this->TexImage && this->CreateParms.Width > 0 && this->CreateParms.Height > 0 && this->CreateParms.Pool == D3DPOOL_DEFAULT)
	{
		SAFE_RELEASE (this->TexImage);
		this->TexImage = NULL;
	}
}


void context_t::ReleaseDefaultPoolTextures (void)
{
	this->FlushGeometry ();

	// now setup each individual texture with known good values
	for (int i = 0; i < MAX_D3D_TEXTURES; i++)
	{
		this->Textures[i].DestroyRT ();
	}
}


void d3d_texture_t::ReCreateRT (class context_t *ctx)
{
	// the only default pool textures should be render targets which are updated each frame so we don't need to worry about restoring image data
	if (!this->TexImage &&
		this->CreateParms.Width > 0 &&
		this->CreateParms.Height > 0 &&
		this->CreateParms.Pool == D3DPOOL_DEFAULT)
	{
		ctx->Device->CreateTexture (
			this->CreateParms.Width,
			this->CreateParms.Height,
			this->CreateParms.Levels,
			this->CreateParms.Usage,
			this->CreateParms.Format,
			this->CreateParms.Pool,
			&this->TexImage,
			NULL);
	}
}


void context_t::RecreateDefaultPoolTextures (void)
{
	this->FlushGeometry ();

	// now setup each individual texture with known good values
	for (int i = 0; i < MAX_D3D_TEXTURES; i++)
	{
		this->Textures[i].ReCreateRT (this);
	}
}


void d3d_texture_t::CheckDirty (context_t *ctx)
{
	// tetxures are checked for dirty rects before drawing with them so that we can in theory batch up
	// multiple small updates into single large updates and get better perf; in practice this doesn't
	// work with FitzQuake because it uses the most idiotic lightmap update strategy ever, but it could
	if (this->dirty)
	{
		ctx->FlushGeometry ();

		if (this->TexImage)
			this->TexImage->AddDirtyRect (&this->DirtyRect);

		this->Clean ();
	}
}


void context_t::SetTexture (int stage, IDirect3DBaseTexture9 *texture)
{
	if (this->State.BoundTextures[stage] != texture)
	{
		this->FlushGeometry ();
		this->Device->SetTexture (stage, texture);
		this->State.BoundTextures[stage] = texture;
	}
}


#endif

