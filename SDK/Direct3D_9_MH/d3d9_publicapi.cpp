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
// public api - this is the only code that directly references d3d_Context
#include "d3d9_internal.h"

static context_t *d3d_Context = NULL;

// forward declarations where necessary
BOOL WINAPI Direct3D9_wglSwapIntervalEXT (int interval);
int WINAPI Direct3D9_wglGetSwapIntervalEXT (void);

LONG WINAPI /*Baker added WINAPI*/ Direct3D9_ChangeDisplaySettings (LPDEVMODE lpDevMode, DWORD dwflags)
{
	// we're only interested in CDS_TEST here because Quake uses that for enumerating video modes
	if (dwflags & CDS_TEST)
	{
		// ensure that we have a mode list
		d3d_Globals.CreateDirect3D ();
		d3d_Globals.GetModeList ();

		// check the mode list for a match
		for (UINT i = 0; i < d3d_Globals.NumModeList; i++)
		{
			// build a DEVMODE from this mode
			DEVMODE dm;
			d3d_Globals.D3DModeToDEVMODE (&dm, &d3d_Globals.ModeList[i]);

			// see do they match
			if ((lpDevMode->dmFields & DM_BITSPERPEL) && dm.dmBitsPerPel != lpDevMode->dmBitsPerPel) continue;
			if ((lpDevMode->dmFields & DM_PELSWIDTH) && dm.dmPelsWidth != lpDevMode->dmPelsWidth) continue;
			if ((lpDevMode->dmFields & DM_PELSHEIGHT) && dm.dmPelsHeight != lpDevMode->dmPelsHeight) continue;
			if ((lpDevMode->dmFields & DM_DISPLAYFREQUENCY) && dm.dmDisplayFrequency != lpDevMode->dmDisplayFrequency) continue;

			// this is a valid mode
			return DISP_CHANGE_SUCCESSFUL;
		}

		// not in the list
		return DISP_CHANGE_FAILED;
	}

	// always signal success; d3d doesn't use CDS but instead does it automatically as part of it's device create/reset
	return DISP_CHANGE_SUCCESSFUL;
}


BOOL WINAPI /* Baker added WINAPI */ Direct3D9_EnumDisplaySettings (LPCSTR lpszDeviceName, DWORD iModeNum, LPDEVMODE lpDevMode)
{
	// https://msdn.microsoft.com/en-us/library/windows/desktop/dd162611%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

	/*
	When you call EnumDisplaySettings with iModeNum set to zero, the operating system initializes and caches information about the
	display device. When you call EnumDisplaySettings with iModeNum set to a nonzero value, the function returns the information that
	was cached the last time the function was called with iModeNum set to zero.

	ENUM_CURRENT_SETTINGS	Retrieve the current settings for the display device.
	ENUM_REGISTRY_SETTINGS	Retrieve the settings for the display device that are currently stored in the registry.
	*/

	// need the d3d object up so that we can get at the info
	d3d_Globals.CreateDirect3D ();

	switch (iModeNum)
	{
	case 0:
		d3d_Globals.GetModeList ();
	default:
		if (iModeNum >= d3d_Globals.NumModeList)
			return FALSE;

		d3d_Globals.D3DModeToDEVMODE (lpDevMode, &d3d_Globals.ModeList[iModeNum]);
		return TRUE;

	case ENUM_CURRENT_SETTINGS:
		d3d_Globals.D3DModeToDEVMODE (lpDevMode, &d3d_Globals.DesktopMode);
		return TRUE;

	case ENUM_REGISTRY_SETTINGS:
		// no equivalent
		return EnumDisplaySettingsA (lpszDeviceName, iModeNum, lpDevMode);
	}

	// never hit
	return EnumDisplaySettingsA (lpszDeviceName, iModeNum, lpDevMode);
}


void WINAPI Direct3D9_glActiveTexture (GLenum texture)
{
	d3d_Context->State.CurrentTMU = texture - GLD3D_TEXTURE0;
}

void WINAPI Direct3D9_glClientActiveTexture (GLenum texture)
{
	d3d_Context->ClientActiveTexture = texture - GLD3D_TEXTURE0;
}

void WINAPI Direct3D9_glMultiTexCoord1f (GLenum target, GLfloat s)
{
	d3d_Context->Geometry.EmitTexCoord (target - GLD3D_TEXTURE0, s, 0);
}

void WINAPI Direct3D9_glMultiTexCoord2f (GLenum target, GLfloat s, GLfloat t)
{
	d3d_Context->Geometry.EmitTexCoord (target - GLD3D_TEXTURE0, s, t);
}

void WINAPI Direct3D9_glMultiTexCoord2fv (GLenum target, GLfloat *st)
{
	d3d_Context->Geometry.EmitTexCoord (target - GLD3D_TEXTURE0, st[0], st[1]);
}

void WINAPI Direct3D9_glTexStorage2D (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
	D3DFORMAT d3dformat = D3DFMT_X8R8G8B8;

	// ensure that it's valid to create textures
	if (target != GL_TEXTURE_2D) return;

	// validate format
	switch (internalformat)
	{
	case 1:
	case GL_LUMINANCE:
		d3dformat = D3DFMT_L8;
		break;

	case 3:
	case GL_RGB:
		d3dformat = D3DFMT_X8R8G8B8;
		break;

	case 4:
	case GL_RGBA:
		d3dformat = D3DFMT_A8R8G8B8;
		break;

	default:
		System_Error ("invalid texture internal format");
	}

	d3d_Context->CreateTexture (d3d_Context->TMU[d3d_Context->State.CurrentTMU].boundtexture, width, height, d3dformat);
}

void Direct3D9_ResetMode (int width, int height, BOOL windowed, int client_left, int client_top, int desktop_width, int desktop_height, int is_resize, int *pborder_width, int *pborder_height)
{
	if (d3d_Context) d3d_Context->ResetMode (width, height, windowed, client_left, client_top, desktop_width, desktop_height, is_resize, pborder_width, pborder_height);
}


void Direct3D9_ScreenShotBMP (const char *filename) {d3d_Context->ScreenShot (filename, D3DXIFF_BMP);}
void Direct3D9_ScreenShotJPG (const char *filename) {d3d_Context->ScreenShot (filename, D3DXIFF_JPG);}
void Direct3D9_ScreenShotPNG (const char *filename) {d3d_Context->ScreenShot (filename, D3DXIFF_PNG);}


int WINAPI Direct3D9_ChoosePixelFormat (HDC hdc, CONST PIXELFORMATDESCRIPTOR *ppfd)
{
	// copy off the pixel format so that we can answer a subsequent call to DescribePixelFormat
	memcpy (&d3d_Globals.pfd, ppfd, sizeof (PIXELFORMATDESCRIPTOR));

	// pass through the pixel format and return it as format number 1
	// we only support one pixel format
	return 1;
}

int WINAPI Direct3D9_DescribePixelFormat (HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd)
{
	// failed
	if (!iPixelFormat) return 0;

	if (iPixelFormat == 1)
	{
		if (d3d_Globals.pfd.dwFlags)
		{
			// return the pfd supplied by a prior call to ChoosePixelFormal
			memcpy (ppfd, &d3d_Globals.pfd, sizeof (PIXELFORMATDESCRIPTOR));
		}
		else
		{
			// build a new PFD describing what we support
			static PIXELFORMATDESCRIPTOR pfd = {
				sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
				1,						// version number
				PFD_DRAW_TO_WINDOW |	// support window
				PFD_SUPPORT_OPENGL |	// support OpenGL
				PFD_DOUBLEBUFFER,		// double buffered
				PFD_TYPE_RGBA,			// RGBA type
				24,						// 24-bit color depth
				0, 0, 0, 0, 0, 0,		// color bits ignored
				0,						// no alpha buffer
				0,						// shift bit ignored
				0,						// no accumulation buffer
				0, 0, 0, 0, 			// accum bits ignored
				24,						// 24-bit z-buffer
				8,						// 8-bit stencil buffer
				0,						// no auxiliary buffer
				PFD_MAIN_PLANE,			// main layer
				0,						// reserved
				0, 0, 0					// layer masks ignored
			};

			// and copy it over
			memcpy (ppfd, &pfd, sizeof (PIXELFORMATDESCRIPTOR));
		}

		// we only support one pixel format
		return 1;
	}

	// failed
	return 0;
}

BOOL WINAPI Direct3D9_SetPixelFormat (HDC hdc, int format, CONST PIXELFORMATDESCRIPTOR *ppfd)
{
	// just flagging if we're asking for a stencil buffer
	if (ppfd->cStencilBits)
		d3d_Globals.RequestStencil = TRUE;
	else d3d_Globals.RequestStencil = FALSE;

	// just silently pass the PFD through unmodified
	return TRUE;
}

BOOL Direct3D9_SetupGammaAndContrast (float gamma, float contrast)
{
	return d3d_Context->SetupGammaAndContrast (gamma, contrast);
}

BOOL WINAPI /* Baker added WINAPI*/Direct3D9_SwapBuffers (HDC unused)
{
	d3d_Context->EndScene ();
	return TRUE; // Baker: Let's just always return true for simplicity.
}

HGLRC WINAPI Direct3D9_wglCreateContext (HDC hdc)
{
	// allocate and initialize a new context
	return (HGLRC) (new context_t (hdc));
}

BOOL WINAPI Direct3D9_wglDeleteContext (HGLRC hglrc)
{
	// what if GL tries to delete a NULL context???
	if (hglrc) ((context_t *) hglrc)->Release ();
	SAFE_RELEASE (d3d_Globals.Object);

	// success
	return TRUE;
}

HGLRC WINAPI Direct3D9_wglGetCurrentContext (VOID)
{
	return (HGLRC) d3d_Context;
}

HDC WINAPI Direct3D9_wglGetCurrentDC (VOID)
{
	return 0;
}

PROC WINAPI Direct3D9_wglGetProcAddress (LPCSTR s)
{
	static struct entrypoint_t
	{
		char *funcname;
		PROC funcproc;
	} d3d_EntryPoints[] =
	{
		{"glActiveTexture", (PROC) Direct3D9_glActiveTexture},
		{"glActiveTextureARB", (PROC) Direct3D9_glActiveTexture},
		{"glBindTexture", (PROC) d3d9mh_glBindTexture},
		{"glMultiTexCoord1f", (PROC) Direct3D9_glMultiTexCoord1f},
		{"glMultiTexCoord1fARB", (PROC) Direct3D9_glMultiTexCoord1f},
		{"glMultiTexCoord2f", (PROC) Direct3D9_glMultiTexCoord2f},
		{"glMultiTexCoord2fARB", (PROC) Direct3D9_glMultiTexCoord2f},
		{"glMultiTexCoord2fv", (PROC) Direct3D9_glMultiTexCoord2fv},
		{"glMultiTexCoord2fvARB", (PROC) Direct3D9_glMultiTexCoord2fv},
		{"glClientActiveTexture", (PROC) Direct3D9_glClientActiveTexture},
		{"glClientActiveTextureARB", (PROC) Direct3D9_glClientActiveTexture},
		{"glTexStorage2D", (PROC) Direct3D9_glTexStorage2D},
		{"wglSwapIntervalEXT", (PROC) Direct3D9_wglSwapIntervalEXT},
		{"wglGetSwapIntervalEXT", (PROC) Direct3D9_wglGetSwapIntervalEXT},
		{NULL, NULL}
	};

	// check all of our entrypoints
	for (int i = 0; ; i++)
	{
		// no more entrypoints
		if (!d3d_EntryPoints[i].funcname) break;

		if (!strcmp (s, d3d_EntryPoints[i].funcname))
		{
			return d3d_EntryPoints[i].funcproc;
		}
	}

	// LocalDebugBreak();
	return 0;
}

int WINAPI Direct3D9_wglGetSwapIntervalEXT (void)
{
	if (d3d_Context->DisplayMode.VSync)
		return 1;
	else return 0;
}

BOOL WINAPI Direct3D9_wglMakeCurrent (HDC hdc, HGLRC hglrc)
{
	// otherwise update the D3D device pointer
	d3d_Context = (context_t *) hglrc;
	return TRUE;
}

BOOL WINAPI Direct3D9_wglSwapIntervalEXT (int interval)
{
	d3d_Context->SetVSync (interval);
	return TRUE;
}

void APIENTRY d3d9mh_glAlphaFunc (GLenum func, GLclampf ref)
{
	d3d_Context->SetCompFunc (D3DRS_ALPHAFUNC, func);
	d3d_Context->SetRenderState (D3DRS_ALPHAREF, BYTE_CLAMP ((int) (ref * 255)));
}

void APIENTRY d3d9mh_glArrayElement (GLint e)
{
	d3d_Context->ArrayElement (e);
}

void APIENTRY d3d9mh_glBegin (GLenum mode)
{
	d3d_Context->BeginPrimitive (mode);
}

void APIENTRY d3d9mh_glBindTexture (GLenum target, GLuint texture)
{
	if (target != GL_TEXTURE_2D) return;
	if (texture >= MAX_D3D_TEXTURES) System_Error ("glBindTexture: overflow");

	// i thought i'd gotten rid of this....
	// this was the reason why warpimages are lost following an alt-tab, and also the reason why device resets sometimes fail
	// this time i'll leave it here but commented out just so i'm not tempted to reinstate it in future...
	//if (!d3d_Context->Textures[texture].TexImage)
	//	d3d_Context->Textures[texture].Initialize ();

	d3d_Context->TMU[d3d_Context->State.CurrentTMU].boundtexture = &d3d_Context->Textures[texture];
	d3d_Context->TMU[d3d_Context->State.CurrentTMU].texparamdirty = TRUE;
}

void APIENTRY d3d9mh_glBlendFunc (GLenum sfactor, GLenum dfactor)
{
	d3d_Context->BlendFunc (D3DRS_SRCBLEND, sfactor);
	d3d_Context->BlendFunc (D3DRS_DESTBLEND, dfactor);
}

void APIENTRY d3d9mh_glClear (GLbitfield mask)
{
	DWORD ClearFlags = 0;

	// no accumulation buffer in d3d
	if (mask & GL_COLOR_BUFFER_BIT) ClearFlags |= D3DCLEAR_TARGET;
	if (mask & GL_DEPTH_BUFFER_BIT) ClearFlags |= D3DCLEAR_ZBUFFER;
	if (mask & GL_STENCIL_BUFFER_BIT) ClearFlags |= D3DCLEAR_STENCIL;

	d3d_Context->Clear (ClearFlags);
}

void APIENTRY d3d9mh_glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	d3d_Context->State.Clear.Color = GLColorToD3DColor (red, green, blue, alpha);
}


void APIENTRY d3d9mh_glClearDepth (GLclampd depth)
{
	d3d_Context->State.Clear.Depth = depth;
}

void APIENTRY d3d9mh_glClearStencil (GLint s)
{
	d3d_Context->State.Clear.Stencil = s;
}

void APIENTRY d3d9mh_glColor3f (GLfloat red, GLfloat green, GLfloat blue)
{
	d3d_Context->Geometry.EmitColor (red * 255, green * 255, blue * 255, 255);
}

void APIENTRY d3d9mh_glColor3fv (const GLfloat *v)
{
	d3d_Context->Geometry.EmitColor (v[0] * 255, v[1] * 255, v[2] * 255, 255);
}

void APIENTRY d3d9mh_glColor3ubv (const GLubyte *v)
{
	d3d_Context->Geometry.EmitColor (v[0], v[1], v[2], 255);
}

void APIENTRY d3d9mh_glColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	d3d_Context->Geometry.EmitColor (red * 255, green * 255, blue * 255, alpha * 255);
}

void APIENTRY d3d9mh_glColor4fv (const GLfloat *v)
{
	d3d_Context->Geometry.EmitColor (v[0] * 255, v[1] * 255, v[2] * 255, v[3] * 255);
}

void APIENTRY d3d9mh_glColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
	d3d_Context->Geometry.EmitColor (red, green, blue, alpha);
}

void APIENTRY d3d9mh_glColor4ubv (const GLubyte *v)
{
	d3d_Context->Geometry.EmitColor (v[0], v[1], v[2], v[3]);
}

void APIENTRY d3d9mh_glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	DWORD mask = 0;

	if (red) mask |= D3DCOLORWRITEENABLE_RED;
	if (green) mask |= D3DCOLORWRITEENABLE_GREEN;
	if (blue) mask |= D3DCOLORWRITEENABLE_BLUE;
	if (alpha) mask |= D3DCOLORWRITEENABLE_ALPHA;

	d3d_Context->SetRenderState (D3DRS_COLORWRITEENABLE, mask);
}

void APIENTRY d3d9mh_glColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	d3d_Context->ColorArray.SetPointer (size, type, stride, pointer);
}

void APIENTRY d3d9mh_glCopyTexImage2D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	// equivalent to glTexImage followed by glCopyTexSubImage
	d3d9mh_glTexImage2D (target, level, internalFormat, width, height, border, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	d3d9mh_glCopyTexSubImage2D (target, level, 0, 0, x, y, width, height);
}

void APIENTRY d3d9mh_glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	d3d_Context->CopyFrameBufferToTexture (d3d_Context->TMU[d3d_Context->State.CurrentTMU].boundtexture, level, xoffset, yoffset, x, y, width, height);
}

void APIENTRY d3d9mh_glCullFace (GLenum mode)
{
	d3d_Context->State.Cull.Mode = mode;
	d3d_Context->UpdateCull ();
}

void APIENTRY d3d9mh_glDeleteTextures (GLsizei n, const GLuint *textures)
{
	d3d_Context->FlushGeometry ();

	for (int i = 0; i < n; i++)
	{
		d3d_Context->Textures[textures[i]].Initialize ();
	}
}

void APIENTRY d3d9mh_glDepthFunc (GLenum func)
{
	d3d_Context->SetCompFunc (D3DRS_ZFUNC, func);
}

void APIENTRY d3d9mh_glDepthMask (GLboolean flag)
{
	// if only they were all so easy...
	d3d_Context->SetRenderState (D3DRS_ZWRITEENABLE, flag == GL_TRUE ? TRUE : FALSE);
}

void APIENTRY d3d9mh_glDepthRange (GLclampd zNear, GLclampd zFar)
{
	// update the viewport
	d3d_Context->State.Viewport.MinZ = zNear;
	d3d_Context->State.Viewport.MaxZ = zFar;
	d3d_Context->UpdateViewport ();
}

void APIENTRY d3d9mh_glDisable (GLenum cap)
{
	d3d_Context->EnableDisable (cap, FALSE);
}

void APIENTRY d3d9mh_glDisableClientState (GLenum array)
{
	d3d_Context->EnableClientState (array, FALSE);
}

void APIENTRY d3d9mh_glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
	d3d9mh_glBegin (mode);

	for (int i = 0; i < count; i++)
		d3d_Context->ArrayElement (first + i);

	d3d9mh_glEnd ();
}

void APIENTRY d3d9mh_glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
	d3d9mh_glBegin (mode);

	for (int i = 0; i < count; i++)
	{
		switch (type)
		{
		case GL_UNSIGNED_BYTE:
			d3d_Context->ArrayElement (((unsigned char *) indices)[i]);
			break;

		case GL_UNSIGNED_SHORT:
			d3d_Context->ArrayElement (((unsigned short *) indices)[i]);
			break;

		case GL_UNSIGNED_INT:
			d3d_Context->ArrayElement (((unsigned int *) indices)[i]);
			break;
		}
	}

	d3d9mh_glEnd ();
}

void APIENTRY d3d9mh_glEnable (GLenum cap)
{
	d3d_Context->EnableDisable (cap, TRUE);
}

void APIENTRY d3d9mh_glEnableClientState (GLenum array)
{
	d3d_Context->EnableClientState (array, TRUE);
}

void APIENTRY d3d9mh_glEnd (void)
{
	d3d_Context->Geometry.EndPrimitive ();
}

void APIENTRY d3d9mh_glFinish (void)
{
	d3d_Context->Sync ();
}

void APIENTRY d3d9mh_glFogf (GLenum pname, GLfloat param)
{
	D3DRENDERSTATETYPE rs;

	switch (pname)
	{
		// this calls into glFogi
	case GL_FOG_MODE: d3d9mh_glFogi (pname, param); return;

		// these set a state directly
	case GL_FOG_DENSITY: rs = D3DRS_FOGDENSITY; break;
	case GL_FOG_START: rs = D3DRS_FOGSTART; break;
	case GL_FOG_END: rs = D3DRS_FOGEND; break;

		// these do nothing
	case GL_FOG_COLOR: return;
	default: return;
	}

	d3d_Context->SetRenderState (rs, D3D_FloatToDWORD (param));
}

void APIENTRY d3d9mh_glFogfv (GLenum pname, const GLfloat *params)
{
	switch (pname)
	{
	case GL_FOG_COLOR:
		d3d_Context->SetRenderState (D3DRS_FOGCOLOR, GLColorToD3DColor (params[0], params[1], params[2], params[3]));
		break;

	default:
		d3d9mh_glFogf (pname, params[0]);
		break;
	}
}

void APIENTRY d3d9mh_glFogi (GLenum pname, GLint param)
{
	DWORD vmode = D3DFOG_NONE;
	DWORD tmode = D3DFOG_NONE;

	switch (pname)
	{
	case GL_FOG_MODE:
		// select the correct pixel fog mode
		if (param == GL_LINEAR)
			tmode = D3DFOG_LINEAR;
		else if (param == GL_EXP)
			tmode = D3DFOG_EXP;
		else if (param == GL_EXP2)
			tmode = D3DFOG_EXP2;
		else tmode = D3DFOG_NONE;
		break;

	default:
		d3d9mh_glFogf (pname, param);
		return;
	}

	d3d_Context->SetRenderState (D3DRS_FOGVERTEXMODE, vmode);
	d3d_Context->SetRenderState (D3DRS_FOGTABLEMODE, tmode);
}

void APIENTRY d3d9mh_glFogiv (GLenum pname, const GLint *params)
{
	switch (pname)
	{
	case GL_FOG_COLOR:
		// the spec isn't too clear on how these map to the fp values... oh well...
		d3d_Context->SetRenderState (D3DRS_FOGCOLOR, D3DCOLOR_ARGB (BYTE_CLAMP (params[3]), BYTE_CLAMP (params[0]), BYTE_CLAMP (params[1]), BYTE_CLAMP (params[2])));
		break;

	default:
		d3d9mh_glFogi (pname, params[0]);
		break;
	}
}

void APIENTRY d3d9mh_glFrontFace (GLenum mode)
{
	d3d_Context->State.Cull.FrontFace = mode;
	d3d_Context->UpdateCull ();
}

void APIENTRY d3d9mh_glFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	d3d_Context->State.CurrentMatrix->Frustum (left, right, bottom, top, zNear, zFar);
}

void APIENTRY d3d9mh_glGenTextures (GLsizei n, GLuint *textures)
{
	int i, j;

	d3d_Context->FlushGeometry ();

	// now setup each individual texture with known good values
	// never gen texture object 0
	for (i = 1, j = 0; i < MAX_D3D_TEXTURES; i++)
	{
		d3d_texture_t *tex = &d3d_Context->Textures[i];

		if (!tex->TexImage)
		{
			tex->Initialize ();
			textures[j++] = i;
		}

		// all filled
		if (j == n) return;
	}

	System_Error ("glGenTextures: overflow");
}

void APIENTRY d3d9mh_glGetFloatv (GLenum pname, GLfloat *params)
{
	switch (pname)
	{
	case GL_MODELVIEW_MATRIX:
	case GL_PROJECTION_MATRIX:
	case GL_TEXTURE_MATRIX:
		d3d_Context->GetMatrix (pname, params);
		break;

	case GLD3D_MAX_TEXTURE_MAX_ANISOTROPY_EXT:
		params[0] = (float) d3d_Globals.DeviceCaps.MaxAnisotropy;
		break;

	default:
		break;
	}
}

void APIENTRY d3d9mh_glGetIntegerv (GLenum pname, GLint *params)
{
	// here we only bother getting the values that glquake uses
	switch (pname)
	{
	case GL_MAX_TEXTURE_SIZE:
		// D3D allows both to be different so return the lowest
		params[0] = (d3d_Globals.DeviceCaps.MaxTextureWidth > d3d_Globals.DeviceCaps.MaxTextureHeight ? d3d_Globals.DeviceCaps.MaxTextureHeight : d3d_Globals.DeviceCaps.MaxTextureWidth);
		break;

	case GL_VIEWPORT:
		d3d_Context->GetViewport (params);
		break;

	case GL_STENCIL_BITS:
		if (d3d_Context->DisplayMode.dsFmt == D3DFMT_D24S8)
			params[0] = 8;
		else params[0] = 0;

		break;

	default:
		params[0] = 0;
		return;
	}
}

const GLubyte *APIENTRY d3d9mh_glGetString (GLenum name)
{
	static char *emptystring = "";

	// we advertise ourselves as version 1.1 so that as few assumptions about capability as possible are made
	static char *GL_VersionString = "1.1";

	switch (name)
	{
	case GL_VENDOR:
		return (const GLubyte *) d3d_Context->AdapterID.Description;
	case GL_RENDERER:
		return (const GLubyte *) d3d_Context->AdapterID.Driver;
	case GL_VERSION:
		return (const GLubyte *) GL_VersionString;
	case GL_EXTENSIONS:
		return (const GLubyte *) d3d_Context->GLExtensions;
	default:
		return (const GLubyte *) emptystring;
	}
}

void APIENTRY d3d9mh_glGetTexImage (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{
	d3d_Context->SaveTextureToMemory (d3d_Context->TMU[d3d_Context->State.CurrentTMU].boundtexture, level, format, pixels);
}

void APIENTRY d3d9mh_glGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params)
{
	if (target != GL_TEXTURE_2D) return;

	d3d_Context->FlushGeometry ();

	switch (pname)
	{
	case GLD3D_TEXTURE_MAX_ANISOTROPY_EXT:
		params[0] = d3d_Context->TMU[d3d_Context->State.CurrentTMU].boundtexture->TexParms.Anisotropy;
		break;

	default:
		break;
	}
}

void APIENTRY d3d9mh_glLoadIdentity (void)
{
	d3d_Context->State.CurrentMatrix->Identity ();
}

void APIENTRY d3d9mh_glLoadMatrixf (const GLfloat *m)
{
	d3d_Context->State.CurrentMatrix->Load (m);
}

void APIENTRY d3d9mh_glMatrixMode (GLenum mode)
{
	switch (mode)
	{
	case GL_MODELVIEW:
		d3d_Context->State.CurrentMatrix = &d3d_Context->ViewMatrix;
		break;

	case GL_PROJECTION:
		d3d_Context->State.CurrentMatrix = &d3d_Context->ProjMatrix;
		break;

	case GL_TEXTURE:
		d3d_Context->State.CurrentMatrix = &d3d_Context->TMU[d3d_Context->State.CurrentTMU].TextureMatrix;
		break;

	default:
		System_Error ("glMatrixMode: unimplemented mode");
		break;
	}
}

void APIENTRY d3d9mh_glMultMatrixf (const GLfloat *m)
{
	d3d_Context->State.CurrentMatrix->Mult (m);
}

void APIENTRY d3d9mh_glOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	d3d_Context->State.CurrentMatrix->Ortho (left, right, bottom, top, zNear, zFar);
}

void APIENTRY d3d9mh_glPolygonMode (GLenum face, GLenum mode)
{
	// we don't have the ability to specify which side of the poly is filled, so just do it
	// the way that it's specified and hope for the best!
	if (mode == GL_LINE)
		d3d_Context->SetRenderState (D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	else if (mode == GL_POINT)
		d3d_Context->SetRenderState (D3DRS_FILLMODE, D3DFILL_POINT);
	else d3d_Context->SetRenderState (D3DRS_FILLMODE, D3DFILL_SOLID);
}

void APIENTRY d3d9mh_glPolygonOffset (GLfloat factor, GLfloat units)
{
	// D3D: Offset = m * D3DRS_SLOPESCALEDEPTHBIAS + D3DRS_DEPTHBIAS
	// GL: offset is factor × DZ + r × units
	// "m" and "DZ" are calculated the same way confirming that D3DRS_SLOPESCALEDEPTHBIAS is the factor param
	// so D3DRS_DEPTHBIAS is the equivalent of "r × units"
	// in GL "r" is implementation-specific, so we just assume that for D3D it's 1.
	// so D3DRS_DEPTHBIAS is the units param
	d3d_Context->State.PolygonOffset.Factor = factor;
	d3d_Context->State.PolygonOffset.Units = units;
}

void APIENTRY d3d9mh_glPopMatrix (void)
{
	d3d_Context->State.CurrentMatrix->Pop ();
}

void APIENTRY d3d9mh_glPushMatrix (void)
{
	d3d_Context->State.CurrentMatrix->Push ();
}

void APIENTRY d3d9mh_glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
	if ((format != GL_RGB && format != GL_RGBA && format != GL_BGR_EXT && format != GL_BGRA_EXT) || type != GL_UNSIGNED_BYTE)
	{
		Con_PrintLinef ("glReadPixels: unimplemented format or type");
		return;
	}

	d3d_Context->ReadPixels (x, y, width, height, format, (unsigned char *) pixels);
}

void APIENTRY d3d9mh_glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	d3d_Context->State.CurrentMatrix->Rotate (angle, x, y, z);
}

void APIENTRY d3d9mh_glScalef (GLfloat x, GLfloat y, GLfloat z)
{
	d3d_Context->State.CurrentMatrix->Scale (x, y, z);
}

void APIENTRY d3d9mh_glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
	// bottom-left adjust the scissor rect
	RECT sr;

	Adjust_BottomLeftToTopLeft (&sr, x, y, width, height, d3d_Context->DisplayMode.Height);
	d3d_Context->Device->SetScissorRect (&sr);
}

void APIENTRY d3d9mh_glShadeModel (GLenum mode)
{
	// easy peasy
	d3d_Context->SetRenderState (D3DRS_SHADEMODE, mode == GL_FLAT ? D3DSHADE_FLAT : D3DSHADE_GOURAUD);
}

void APIENTRY d3d9mh_glStencilFunc (GLenum func, GLint ref, GLuint mask)
{
	d3d_Context->SetCompFunc (D3DRS_STENCILFUNC, func);
	d3d_Context->SetRenderState (D3DRS_STENCILREF, ref);
	d3d_Context->SetRenderState (D3DRS_STENCILMASK, mask);
}

void APIENTRY d3d9mh_glStencilMask (GLuint mask)
{
	d3d_Context->SetRenderState (D3DRS_STENCILWRITEMASK, mask);
}

void APIENTRY d3d9mh_glStencilOp (GLenum fail, GLenum zfail, GLenum zpass)
{
	d3d_Context->StencilOp (D3DRS_STENCILFAIL, fail);
	d3d_Context->StencilOp (D3DRS_STENCILZFAIL, zfail);
	d3d_Context->StencilOp (D3DRS_STENCILPASS, zpass);
}

void APIENTRY d3d9mh_glTexCoord2f (GLfloat s, GLfloat t)
{
	d3d_Context->Geometry.EmitTexCoord (0, s, t);
}

void APIENTRY d3d9mh_glTexCoord2fv (const GLfloat *v)
{
	d3d_Context->Geometry.EmitTexCoord (0, v[0], v[1]);
}

void APIENTRY d3d9mh_glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	d3d_Context->TexCoordArray[d3d_Context->ClientActiveTexture].SetPointer (size, type, stride, pointer);
}

void APIENTRY d3d9mh_glTexEnvf (GLenum target, GLenum pname, GLfloat param)
{
	d3d_Context->TexEnv (target, pname, (GLint) param);
}

void APIENTRY d3d9mh_glTexEnvi (GLenum target, GLenum pname, GLint param)
{
	d3d_Context->TexEnv (target, pname, param);
}

void APIENTRY d3d9mh_glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	if (level == 0)
	{
		// OpenGL texture specification is insane, on the following grounds:
		// - miplevels can be specified in any order,
		// - miplevels don't have to match sizes or formats during specification
		// - textures are allowed to be incompletely specified,
		// - and none of this can be validated until draw time.
		// to avoid all of the headaches that comes with emulating this, we step in and take over much of the specification
		// ourselves; in the wrapper:
		// - incomplete textures are not allowed (and are, in fact, impossible),
		// - every texture is created with a full mipchain (we use sampler states to specify unmipped textures),
		// - only specification of mip level 0 actually does anything; specification of other miplevels is ignored,
		// - we do our own mipmapping in the wrapper.
		D3DFORMAT d3dformat = D3DFMT_X8R8G8B8;

		if (type != GL_UNSIGNED_BYTE) System_Error ("glTexImage2D: Unrecognised pixel format");

		// ensure that it's valid to create textures
		if (target != GL_TEXTURE_2D) return;

		// validate format
		switch (internalformat)
		{
		case 1:
		case GL_LUMINANCE:
			d3dformat = D3DFMT_L8;
			break;

		case 3:
		case GL_RGB:
			d3dformat = D3DFMT_X8R8G8B8;
			break;

		case 4:
		case GL_RGBA:
			d3dformat = D3DFMT_A8R8G8B8;
			break;

		default:
			System_Error ("invalid texture internal format");
		}

		d3d_texture_t *tex = d3d_Context->TMU[d3d_Context->State.CurrentTMU].boundtexture;

		// create the texture if level 0 is seen; Open GL allows specification of texture levels in any order, as well as
		// incomplete specification, but D3D doesn't.  to fully mimic the OpenGL behaviour in D3D we would copy all of the
		// pixel data and params off to system memory, then only actually create a texture the first time it is used for
		// drawing.  but we won't do that.  instead the wrapper will require level 0 to be created first and create the
		// texture from that.  note that modern OpenGL - with glTexStorage and particularly DSA - has the same constraint
		// as D3D so the older OpenGL behaviour should be viewed as insanity rather than flexibility.
		d3d_Context->CreateTexture (tex, width, height, d3dformat);

		// GL allows specifying NULL pixels to create the texture but not fill it yet
		if (pixels)
		{
			tex->Fill (0, 0, 0, width, height, format, pixels, 0);
			tex->Mipmap ();
		}
	}
}


void APIENTRY d3d9mh_glTexParameterf (GLenum target, GLenum pname, GLfloat param)
{
	d3d_texture_t *tex = d3d_Context->TMU[d3d_Context->State.CurrentTMU].boundtexture;

	if (!tex) return;
	if (target != GL_TEXTURE_2D) return;

	d3d_Context->FlushGeometry ();
	d3d_Context->TMU[d3d_Context->State.CurrentTMU].texparamdirty = TRUE;

	switch (pname)
	{
	case GL_TEXTURE_MIN_FILTER:
		if ((int) param == GL_NEAREST_MIPMAP_NEAREST)
		{
			tex->TexParms.MinFilter = D3DTEXF_POINT;
			tex->TexParms.MipFilter = D3DTEXF_POINT;
		}
		else if ((int) param == GL_LINEAR_MIPMAP_NEAREST)
		{
			tex->TexParms.MinFilter = D3DTEXF_LINEAR;
			tex->TexParms.MipFilter = D3DTEXF_POINT;
		}
		else if ((int) param == GL_NEAREST_MIPMAP_LINEAR)
		{
			tex->TexParms.MinFilter = D3DTEXF_POINT;
			tex->TexParms.MipFilter = D3DTEXF_LINEAR;
		}
		else if ((int) param == GL_LINEAR_MIPMAP_LINEAR)
		{
			tex->TexParms.MinFilter = D3DTEXF_LINEAR;
			tex->TexParms.MipFilter = D3DTEXF_LINEAR;
		}
		else if ((int) param == GL_LINEAR)
		{
			tex->TexParms.MinFilter = D3DTEXF_LINEAR;
			tex->TexParms.MipFilter = D3DTEXF_NONE;
		}
		else // GL_NEAREST
		{
			tex->TexParms.MinFilter = D3DTEXF_POINT;
			tex->TexParms.MipFilter = D3DTEXF_NONE;
		}
		break;

	case GL_TEXTURE_MAG_FILTER:
		if ((int) param == GL_LINEAR)
			tex->TexParms.MagFilter = D3DTEXF_LINEAR;
		else tex->TexParms.MagFilter = D3DTEXF_POINT;
		break;

	case GL_TEXTURE_WRAP_S:
		if ((int) param == GL_CLAMP)
			tex->TexParms.AddressU = D3DTADDRESS_CLAMP;
		else tex->TexParms.AddressU = D3DTADDRESS_WRAP;
		break;

	case GL_TEXTURE_WRAP_T:
		if ((int) param == GL_CLAMP)
			tex->TexParms.AddressV = D3DTADDRESS_CLAMP;
		else tex->TexParms.AddressV = D3DTADDRESS_WRAP;
		break;

	case GLD3D_TEXTURE_MAX_ANISOTROPY_EXT:
		// this is a texparam in OpenGL
		if (d3d_Globals.DeviceCaps.MaxAnisotropy > 1)
			tex->TexParms.Anisotropy = (int) param;
		else tex->TexParms.Anisotropy = 1;
		break;

	default:
		break;
	}
}

void APIENTRY d3d9mh_glTexParameteri (GLenum target, GLenum pname, GLint param)
{
	if (target != GL_TEXTURE_2D) return;
	if (!d3d_Context->TMU[d3d_Context->State.CurrentTMU].boundtexture) return;

	d3d_Context->FlushGeometry ();
	d3d9mh_glTexParameterf (target, pname, param);
}

void APIENTRY d3d9mh_glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	d3d_texture_t *tex = d3d_Context->TMU[d3d_Context->State.CurrentTMU].boundtexture;

	d3d_Context->FlushGeometry ();

	// D3D only records dirty rects for level 0; this is also a rough heuristic to detect a lightmap
	// (we typically don't call texsubimage on anything but lightmaps)
	if (level == 0)
		tex->Fill (0, xoffset, yoffset, width, height, format, pixels, D3DLOCK_NO_DIRTY_UPDATE);
	else tex->Fill (level, xoffset, yoffset, width, height, format, pixels, 0);
}

void APIENTRY d3d9mh_glTranslatef (GLfloat x, GLfloat y, GLfloat z)
{
	d3d_Context->State.CurrentMatrix->Translate (x, y, z);
}

void APIENTRY d3d9mh_glVertex2f (GLfloat x, GLfloat y)
{
	d3d_Context->Geometry.EmitVertex (x, y, 0);
}

void APIENTRY d3d9mh_glVertex2fv (const GLfloat *v)
{
	d3d_Context->Geometry.EmitVertex (v[0], v[1], 0);
}

void APIENTRY d3d9mh_glVertex3f (GLfloat x, GLfloat y, GLfloat z)
{
	d3d_Context->Geometry.EmitVertex (x, y, z);
}

void APIENTRY d3d9mh_glVertex3fv (const GLfloat *v)
{
	d3d_Context->Geometry.EmitVertex (v[0], v[1], v[2]);
}

void APIENTRY d3d9mh_glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	d3d_Context->VertexArray.SetPointer (size, type, stride, pointer);
}

void APIENTRY d3d9mh_glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
	// translate from OpenGL bottom-left to D3D top-left
	y = d3d_Context->DisplayMode.Height - (height + y);

	d3d_Context->State.Viewport.X = x;
	d3d_Context->State.Viewport.Y = y;
	d3d_Context->State.Viewport.Width = width;
	d3d_Context->State.Viewport.Height = height;

	d3d_Context->UpdateViewport ();
}


#endif


