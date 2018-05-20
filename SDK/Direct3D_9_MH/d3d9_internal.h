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


// private internal-only interface
#ifdef DIRECT3D9_WRAPPER

#ifdef __cplusplus
extern "C" {
#endif

// override the stock D3DMATRIX with our own variant including the more useful m16 member
// must be done before including any D3D files so that the override will work
// must be binary-compatible with the original D3DMATRIX struct
typedef struct _D3DMATRIX
{
	union
	{
		struct
		{
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};

		float m4x4[4][4];
		float m16[16];
	};
} D3DMATRIX;

// now we tell D3D that we've defined our matrix type so please fuck off
#define D3DMATRIX_DEFINED

// now we can include d3d9.h
#include <d3d9.h>
#include <math.h>

#include "d3d9_publicapi.h"
#include "d3d9_mathlib.h"


// externs we need from the Quake engine
// Baker: gcc gives me awesome printf warnings using __attribute__
//   ... like telling me when args mismatch or are omitted.
// Under my new system, most Con_Printf like functions shouldn't provide a newline
// and instead a Con_PrintLinef should be used.
// Eliminates the human error of messages where the trailing \n was forgotten
// Which I discovered in many places, didn't investigate to see how many were
// in the original id Software source release.
int System_Error (const char *fmt, ...);// __core_attribute__((__format__(__printf__,1,2), __noreturn__));
int Con_PrintLinef (const char *fmt, ...); // __core_attribute__((__format__(__printf__,1,2)));
int Con_SafePrintLinef (const char *fmt, ...); // __core_attribute__((__format__(__printf__,1,2)));
char *va (const char *format, ...); // __core_attribute__((__format__(__printf__,1,2)));

#define MIN_MODE_WIDTH_640			640
#define MIN_MODE_HEIGHT_400			400


// utility
#define BYTE_CLAMP(i) (int) ((((i) > 255) ? 255 : (((i) < 0) ? 0 : (i))))

#define SAFE_RELEASE(p) {if (p) (p)->Release (); (p) = NULL;}

#define VDECL(Stream, Offset, Type, Usage, UsageIndex) {Stream, Offset, Type, D3DDECLMETHOD_DEFAULT, Usage, UsageIndex}

__inline DWORD D3D_FloatToDWORD (float f)
{
	return ((DWORD *) &f)[0];
}

__inline int D3D_TMUForTexture (GLenum texture)
{
	// GL spec guarantees these are consecutive numbered
	return texture - GLD3D_TEXTURE0;
}

__inline DWORD GLColorToD3DColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	return D3DCOLOR_ARGB (
		BYTE_CLAMP (alpha * 255.0f),
		BYTE_CLAMP (red * 255.0f),
		BYTE_CLAMP (green * 255.0f),
		BYTE_CLAMP (blue * 255.0f)
	);
}

__inline void Adjust_BottomLeftToTopLeft (RECT *r, int x, int y, int w, int h, int height)
{
	r->left = x;
	r->top = height - (y + h);

	r->right = r->left + w;
	r->bottom = r->top + h;
}

__inline int D3D_BytesForFormat (GLint format)
{
	if (format == 1 || format == GL_LUMINANCE)
		return 1;
	else if (format == 3 || format == GL_RGB || format == GL_BGR_EXT)
		return 3;
	else if (format == 4 || format == GL_RGBA || format == GL_BGRA_EXT)
		return 4;

	System_Error ("D3D_BytesForFormat: illegal format");
	return 0;
}


// opengl specified up to 32 TMUs, Quake only uses 2, Baker has requested 8
// BUT nvidia only offer 4 with the fixed pipeline in GL, so 4 it is...
#define D3D_MAX_TMUS	4


// matrixes
// OpenGL only has a stack depth of 2 for projection, but no reason to not use the full depth
#define MAX_MATRIX_STACK	32

class matrixstack_t
{
public:
	BOOL dirty;
	int stackdepth;
	D3DMATRIX stack[MAX_MATRIX_STACK];
	D3DMATRIX *Current;

	matrixstack_t (void);
	void Initialize (void);
	void Push (void);
	void Pop (void);
	void Identity (void);
	void Frustum (float left, float right, float bottom, float top, float zNear, float zFar);
	void Ortho (float left, float right, float bottom, float top, float zNear, float zFar);
	void Rotate (float angle, float x, float y, float z);
	void Translate (float x, float y, float z);
	void Scale (float x, float y, float z);
	void Mult (const float *m);
	void Load (const float *m);
	void MultMatrix (D3DMATRIX *m);
};


// textures
#define MAX_D3D_TEXTURES	65536

class d3d_texture_t
{
public:
	d3d_texture_t (void);

	// format the texture was uploaded as; 1 or 4 bytes.
	// glTexSubImage2D needs this.
	GLenum internalformat;

	// parameters
	struct TexParms_t
	{
		DWORD AddressU;
		DWORD AddressV;
		DWORD Anisotropy;
		DWORD MagFilter;
		DWORD MinFilter;
		DWORD MipFilter;
		TexParms_t (void);
		void Initialize (void);
	} TexParms;

	// the texture image itself
	IDirect3DTexture9 *TexImage;

	// creation parms
	struct CreateParms_t
	{
		UINT Width;
		UINT Height;
		UINT Levels;
		DWORD Usage;
		D3DFORMAT Format;
		D3DPOOL Pool;
		CreateParms_t (void);
		void Initialize (void);
	} CreateParms;

	// track dirty rects for lightmap updating
	BOOL dirty;
	RECT DirtyRect;

	D3DLOCKED_RECT lockrect;
	BOOL locked;

	void Dirty (RECT *texrect);
	void Clean (void);
	void Mipmap (void);

	void Initialize (void);
	void Create (class context_t *ctx, GLsizei width, GLsizei height, D3DFORMAT d3dformat);
	void ReCreateRT (class context_t *ctx);
	void DestroyRT (void);
	void CheckDirty (class context_t *ctx);
	void CheckRenderTarget (class context_t *ctx);
	void Fill (int level, GLint xoffset, GLint yoffset, int width, int height, GLint srcformat, const void *pixels, DWORD lockflag);
};


class tmustate_t
{
public:
	void Initialize (void);

	d3d_texture_t *boundtexture;
	BOOL enabled;

	struct TexEnv_s
	{
		GLenum TEXTURE_ENV_MODE;
		GLenum COMBINE_RGB;
		GLenum COMBINE_ALPHA;
		GLenum SRC0_RGB;
		GLenum SRC1_RGB;
		GLenum SRC2_RGB;
		GLenum SRC0_ALPHA;
		GLenum SRC1_ALPHA;
		GLenum SRC2_ALPHA;
		GLenum OPERAND0_RGB;
		GLenum OPERAND1_RGB;
		GLenum OPERAND2_RGB;
		GLenum OPERAND0_ALPHA;
		GLenum OPERAND1_ALPHA;
		GLenum OPERAND2_ALPHA;
		float RGB_SCALE;
		float ALPHA_SCALE;
	} TexEnv;

	matrixstack_t TextureMatrix;

	BOOL texenvdirty;
	BOOL texparamdirty;

	void SetTexture (IDirect3DBaseTexture9 *texture);
};


BOOL D3D_CheckTextureFormat (D3DFORMAT TextureFormat, D3DFORMAT AdapterFormat);
void D3D_CopyTexels (byte *srcdata, GLint srcformat, byte *dstdata, GLint dstformat, int srcx, int srcy, int dstx, int dsty, int width, int height);


// renderstate
class state_t
{
public:
	state_t (void);

	struct
	{
#if 1
		BOOL FillEnabled;
		BOOL LineEnabled;
		BOOL PointEnabled;
		float Factor;
		float Units;
#else
		BOOL Enabled;
		float Factor;
		float Units;
#endif
	} PolygonOffset;

	IDirect3DBaseTexture9 *BoundTextures[D3D_MAX_TMUS];
	int CurrentTMU;

	BOOL SceneBegun;

	// view
	D3DVIEWPORT9 Viewport;

	// clear
	struct
	{
		DWORD Color;
		float Depth;
		DWORD Stencil;
	} Clear;

	// face/cull
	struct
	{
		GLboolean Enable;
		GLenum FrontFace;
		GLenum Mode;
	} Cull;

	// d3d8 specifies 174 render states; here we just provide headroom
	DWORD RenderStates[256];

	DWORD TextureStates[D3D_MAX_TMUS][33];
	DWORD SamplerStates[D3D_MAX_TMUS][14];

	// gamma and contrast
	float Gamma;
	float Contrast;
	BOOL DoingGammaAndContrast;
	IDirect3DSurface9 *OriginalRT;

	// matrix stacks
	matrixstack_t *CurrentMatrix;

	int DrawCount;

protected:
};


// geometry
// this should be a multiple of 12 to support both GL_QUADS and GL_TRIANGLES
// it should also be large enough to hold the biggest tristrip or fan in use in the engine
// individual quads or tris can be submitted in batches
#define MAX_GEOM_VERTEXES	65536
#define MAX_GEOM_INDEXES	65536


// this may be a little wasteful as it's a full sized vertex for 8 TMUs
// we'll fix it if it becomes a problem (it's not like Quake stresses the GPU too much anyway)
typedef struct vertex_s
{
	float position[3];
	D3DCOLOR color;
	float texcoords[D3D_MAX_TMUS][2];
} vertex_t;


class geometry_t
{
public:
	geometry_t (void);

	GLenum Mode;	// GL_QUADS, GL_TRIANGLES, etc

	int NumVerts;	// total number of verts in all primitives, advanced at glEnd only
	int PrimVerts;	// number of verts in the current primitive, advanced at each glVertex call
	int NumIndexes;	// total number of indexes in all primitives, advanced at glEnd only

	// glColor and glTexCoord just store a current attribute; glVertex then comes along, picks up the current attribs
	// and emits a vertex using them and the vertex positions supplied
	D3DCOLOR CurrentColor;
	float CurrentTexCoord[D3D_MAX_TMUS][2];

	// i had a vertex buffers version but because we don't know in advance how much we're locking, I had to use D3DPOOL_SYSTEMMEM buffers
	// and lock the entire thing, and overall it was slower than just using UP.
	vertex_t Vertexes[MAX_GEOM_VERTEXES];
	unsigned short Indexes[MAX_GEOM_INDEXES];

public:
	// moved BeginPrimitive to context
	void Activate (void);
	void EmitColor (int red, int green, int blue, int alpha);
	void EmitTexCoord (int stage, float s, float t);
	void EmitVertex (float x, float y, float z);
	void EndPrimitive (void);
};


class varray_t
{
public:
	BOOL Enabled;
	int Size;
	GLenum Type;
	int Stride;
	const void *Pointer;

	void SetPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void *FetchArrayElement (int e);
	float *FetchArrayElementFloat (int e);
	unsigned char *FetchArrayElementByte (int e);
};


// d3dx crap - because Microsoft played musical chairs with d3dx versioning we can't just statically link and expect everything to work on different PCs,
// so instead we check d3dx dll versions and dynamically load from the best one we can find
// also, D3DX isn't included in more recent Windows SDKs so we must #define anything from it that we wish to use ourselves.
#ifndef __D3DX9TEX_H__
// define D3DX constants missing since we no longer have D3DX available
#define D3DX_FILTER_NONE             (1 << 0)
#define D3DX_FILTER_POINT            (2 << 0)
#define D3DX_FILTER_LINEAR           (3 << 0)
#define D3DX_FILTER_TRIANGLE         (4 << 0)
#define D3DX_FILTER_BOX              (5 << 0)

#define D3DX_FILTER_MIRROR_U         (1 << 16)
#define D3DX_FILTER_MIRROR_V         (2 << 16)
#define D3DX_FILTER_MIRROR_W         (4 << 16)
#define D3DX_FILTER_MIRROR           (7 << 16)

#define D3DX_FILTER_DITHER           (1 << 19)
#define D3DX_FILTER_DITHER_DIFFUSION (2 << 19)

#define D3DX_FILTER_SRGB_IN          (1 << 21)
#define D3DX_FILTER_SRGB_OUT         (2 << 21)
#define D3DX_FILTER_SRGB             (3 << 21)

#define D3DX_DEFAULT				((UINT) -1)

typedef enum _D3DXIMAGE_FILEFORMAT
{
	D3DXIFF_BMP = 0,
	D3DXIFF_JPG = 1,
	D3DXIFF_TGA = 2,
	D3DXIFF_PNG = 3,
	D3DXIFF_DDS = 4,
	D3DXIFF_PPM = 5,
	D3DXIFF_DIB = 6,
	D3DXIFF_HDR = 7,
	D3DXIFF_PFM = 8,
	D3DXIFF_FORCE_DWORD = 0x7fffffff
} D3DXIMAGE_FILEFORMAT;
#endif


typedef HRESULT (WINAPI *D3DXLoadSurfaceFromMemoryProc) (IDirect3DSurface9 *, CONST PALETTEENTRY *, CONST RECT *, LPCVOID, D3DFORMAT, UINT, CONST PALETTEENTRY *, CONST RECT *, DWORD, D3DCOLOR);
typedef HRESULT (WINAPI *D3DXSaveSurfaceToFileProc) (LPCSTR, D3DXIMAGE_FILEFORMAT, IDirect3DSurface9 *, CONST PALETTEENTRY *, CONST RECT *);
typedef HRESULT (WINAPI *D3DXLoadSurfaceFromSurfaceProc) (IDirect3DSurface9 *, CONST PALETTEENTRY *, CONST RECT *, IDirect3DSurface9 *, CONST PALETTEENTRY *, CONST RECT *, DWORD, D3DCOLOR);
typedef HRESULT (WINAPI *D3DXFilterTextureProc) (LPDIRECT3DBASETEXTURE9, const PALETTEENTRY *, UINT, DWORD);


extern D3DXLoadSurfaceFromMemoryProc QD3DXLoadSurfaceFromMemory;
extern D3DXSaveSurfaceToFileProc QD3DXSaveSurfaceToFile;
extern D3DXLoadSurfaceFromSurfaceProc QD3DXLoadSurfaceFromSurface;
extern D3DXFilterTextureProc QD3DXFilterTexture;

// extension functions
void WINAPI Direct3D9_glMultiTexCoord2f (GLenum target, GLfloat s, GLfloat t);
void WINAPI Direct3D9_glMultiTexCoord1f (GLenum target, GLfloat s);
void WINAPI Direct3D9_glMultiTexCoord2fv (GLenum target, GLfloat *st);
void WINAPI Direct3D9_glClientActiveTexture (GLenum texture);
void WINAPI Direct3D9_glTexStorage2D (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);

class context_t
{
public:
	context_t (HDC hdc);
	void Release (void);

	IDirect3DDevice9 *Device;
	BOOL DeviceLost;
	char GLExtensions[1024]; // never go above this because Q1 itself writes them into a buffer this size
	D3DADAPTER_IDENTIFIER9 AdapterID;

	D3DPRESENT_PARAMETERS *SetupPresentParams (int width, int height, BOOL windowed, BOOL vsync);
	void UpdateDisplayMode (void);

	HWND Window;

	struct modedesc_t
	{
		int Width;
		int Height;
		BOOL Windowed;
		BOOL VSync;
		D3DFORMAT dsFmt;
	} DisplayMode;

	geometry_t Geometry;
	state_t State;

	d3d_texture_t Textures[MAX_D3D_TEXTURES];

	// gamma and contrast objects
	IDirect3DTexture9 *GAndCRT;
	IDirect3DVertexShader9 *GAndCVS;
	IDirect3DPixelShader9 *GAndCPS;
	IDirect3DVertexDeclaration9 *GAndCVD;

	// Main Shader objects
	IDirect3DVertexShader9 *MainVS;
	IDirect3DVertexDeclaration9 *MainVD;

	// matrix stacks
	matrixstack_t ViewMatrix;
	matrixstack_t ProjMatrix;

	// vertex arrays
	varray_t VertexArray;
	varray_t ColorArray;
	varray_t TexCoordArray[D3D_MAX_TMUS];
	int ClientActiveTexture;

	// texture units
	tmustate_t TMU[D3D_MAX_TMUS];

	void BeginScene (void);
	void EndScene (void);

	// state methods
	void SetVSync (int interval);
	void GetRenderStates (void);
	void SetRenderStates (void);
	void InitStates (void);
	void DirtyAllStates (void);
	void CheckPolygonOffset (GLenum mode);
	void SetTexture (int stage, IDirect3DBaseTexture9 *texture);
	void SetCompFunc (D3DRENDERSTATETYPE mode, GLenum func);
	void BlendFunc (D3DRENDERSTATETYPE rs, GLenum factor);
	void StencilOp (D3DRENDERSTATETYPE d3dState, GLenum GLstate);

	void InitTexEnv (void);
	void GetTextureStates (void);
	void SetTextureStates (void);
	void CheckDirtyTextureStates (int stage);
	void SetTextureState (DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
	void SetSamplerState (DWORD Stage, D3DSAMPLERSTATETYPE Type, DWORD Value);
	void TexEnv (GLenum target, GLenum pname, GLint param);

	void CopyFrameBufferToTexture (d3d_texture_t *tex, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	void CreateTexture (d3d_texture_t *tex, GLsizei width, GLsizei height, D3DFORMAT d3dformat);
	void SaveTextureToMemory (d3d_texture_t *tex, GLint level, GLenum format, GLvoid *pixels);

	void ReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, unsigned char *dstdata);

	void SetRenderState (D3DRENDERSTATETYPE state, DWORD value);
	void UpdateCull (void);
	void EnableDisable (GLenum cap, BOOL enable);

	void FlushGeometry (void);

	void SaveViewport (D3DVIEWPORT9 *saved);
	void RestoreViewport (D3DVIEWPORT9 *saved);

	void Clear (DWORD ClearFlags);
	void ResetViewport (void);
	void GetViewport (GLint *params);
	void UpdateViewport (void);
	void ScreenShot (const char *filename, D3DXIMAGE_FILEFORMAT format);

	void SetupTexturesForDrawing (void);

	void InitGeometry (void);
	void BeginPrimitive (GLenum mode);
	void InitTextures (void);
	void ReleaseTextures (void);
	void InitializeStates (void);

	// gamma and contrast
	void InitGammaAndContrast (void);
	void DestroyGammaAndContrast (void);
	BOOL SetupGammaAndContrast (float gamma, float contrast);
	void FinishGammaAndContrast (void);

	// vertex shader
	BOOL CreateCommonVertexShader (void);
	void DestroyVertexShader (void);
	void ActivateVertexShader (void);
	IDirect3DVertexShader9 *CreateVertexShader (const char *Source, const int Len, const char *EntryPoint);
	IDirect3DPixelShader9 *CreatePixelShader (const char *Source, const int Len, const char *EntryPoint);

	// matrixes
	void DirtyMatrixes (void);
	void CheckDirtyMatrix (D3DTRANSFORMSTATETYPE usage, int reg, matrixstack_t *m);
	void UpdateTransforms (void);
	void InitializeTransforms (void);
	void GetMatrix (GLenum pname, GLfloat *params);

	void PreReset (void);
	void ResetDevice (D3DPRESENT_PARAMETERS *PresentParams);
	
	// Baker: The extra information is for window positioning and to restrict resize precisely in WM_GETMINMAX
	void ResetMode (int width, int height, BOOL windowed, int client_left, int client_top, int desktop_width, int desktop_height, int is_resize, int *pborder_width, int *pborder_height);

	void PostReset (void);
	void Sync (void);

	void ReleaseDefaultPoolTextures (void);
	void RecreateDefaultPoolTextures (void);

	// vertex arrays
	void EnableClientState (GLenum array, BOOL enable);
	void ArrayElement (GLint e);
};


class globals_t
{
public:
	globals_t (void);
	void CreateDirect3D (void);
	void D3DModeToDEVMODE (LPDEVMODE lpDevMode, D3DDISPLAYMODE *mode);
	void GetModeList (void);

	D3DDISPLAYMODE ModeList[666];
	UINT NumModeList;

	D3DDISPLAYMODE DesktopMode;
	PIXELFORMATDESCRIPTOR pfd;
	BOOL RequestStencil;
	IDirect3D9 *Object;
	D3DCAPS9 DeviceCaps;
};

extern globals_t d3d_Globals;

#ifdef __cplusplus
};
#endif

#endif
