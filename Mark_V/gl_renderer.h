// Baker

#ifndef __GL_RENDERER_H
#define __GL_RENDERER_H

#include "core_opengl.h"

///////////////////////////////////////////////////////////////////////////////
//  OPENGL: Renderer capabilities
///////////////////////////////////////////////////////////////////////////////

typedef struct
{
	const char	*gl_vendor;
	const char	*gl_renderer;
	const char	*gl_version;
	const char	*gl_extensions;
	char		*gl_extensions_nice;

	cbool		isIntelVideo;
	cbool		gl_mtexable;
	int			gl_max_texture_size;
	cbool		gl_texture_env_combine;
	cbool		gl_texture_env_add;
	cbool		gl_texture_non_power_of_two;
	cbool		gl_swap_control;
	cbool		gl_anisotropy_able;
	float		gl_max_anisotropy; //johnfitz
	int			gl_stencilbits; //johnfitz

	GLenum		TEXTURE0, TEXTURE1; //johnfitz

	PFNGLMULTITEXCOORD2FARBPROC GL_MTexCoord2fFunc; // glMultiTexCoord2
	PFNGLACTIVETEXTUREARBPROC GL_SelectTextureFunc; // glActiveTexture ?

//	glColorTableEXT
//		glColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGB, 256, GL_RGB, GL_UNSIGNED_BYTE,
//		(void *) thePalette);
} renderer_t;

extern renderer_t renderer;

void GL_VID_SetMode_GL_Evaluate_Renderer (void);
void GL_VID_SetMode_GL_SetupState (void);

//#ifdef _WIN32
//#include <windows.h> // Why?  APIENTRY?
//#endif

#ifdef DIRECT3D8_WRAPPER // dx8 - The #include for the wrapper
	#include "dx8_mh_wrapper.h"
#endif // DIRECT3D8_WRAPPER
#ifdef DIRECT3D9_WRAPPER // dx9 - The #include for the wrapper
	#include "dx9_mh_wrapper.h"
#endif // DIRECT3D9_WRAPPER


extern void (APIENTRY *eglAlphaFunc) (GLenum func, GLclampf ref);
extern void (APIENTRY *eglBegin) (GLenum mode);
extern void (APIENTRY *eglBindTexture) (GLenum target, GLuint texture);
extern void (APIENTRY *eglBlendFunc) (GLenum sfactor, GLenum dfactor);
extern void (APIENTRY *eglClear) (GLbitfield mask);
extern void (APIENTRY *eglClearColor) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern void (APIENTRY *eglClearStencil) (GLint s);
extern void (APIENTRY *eglColor3f) (GLfloat red, GLfloat green, GLfloat blue);
extern void (APIENTRY *eglColor3fv) (const GLfloat *v);
extern void (APIENTRY *eglColor3ubv) (const GLubyte *v);
extern void (APIENTRY *eglColor4f) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void (APIENTRY *eglColor4fv) (const GLfloat *v);
extern void (APIENTRY *eglColor4ub) (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
extern void (APIENTRY *eglColor4ubv) (const GLubyte *v);
extern void (APIENTRY *eglCopyTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRY *eglColorMask) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
extern void (APIENTRY *eglCullFace) (GLenum mode);
extern void (APIENTRY *eglDeleteTextures) (GLsizei n, const GLuint *textures);
extern void (APIENTRY *eglDepthFunc) (GLenum func);
extern void (APIENTRY *eglDepthMask) (GLboolean flag);
extern void (APIENTRY *eglDepthRange) (GLclampd zNear, GLclampd zFar);
extern void (APIENTRY *eglDisable) (GLenum cap);
extern void (APIENTRY *eglDrawBuffer) (GLenum mode);
extern void (APIENTRY *eglEnable) (GLenum cap);
extern void (APIENTRY *eglEnd) (void);
extern void (APIENTRY *eglFinish) (void);
extern void (APIENTRY *eglFogf) (GLenum pname, GLfloat param);
extern void (APIENTRY *eglFogfv) (GLenum pname, const GLfloat *params);
extern void (APIENTRY *eglFogi) (GLenum pname, GLint param);
extern void (APIENTRY *eglFogiv) (GLenum pname, const GLint *params);
extern void (APIENTRY *eglFrontFace) (GLenum mode);
extern void (APIENTRY *eglFrustum) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern void (APIENTRY *eglGenTextures) (GLsizei n, GLuint *textures);
extern void (APIENTRY *eglGetFloatv) (GLenum pname, GLfloat *params);
extern void (APIENTRY *eglGetIntegerv) (GLenum pname, GLint *params);
const GLubyte *(APIENTRY *eglGetString) (GLenum name);
extern void (APIENTRY *eglGetTexImage) (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
extern void (APIENTRY *eglGetTexParameterfv) (GLenum target, GLenum pname, GLfloat *params);extern void (APIENTRY *eglHint) (GLenum target, GLenum mode);
extern void (APIENTRY *eglLineWidth) (GLfloat width);
extern void (APIENTRY *eglLoadIdentity) (void);
extern void (APIENTRY *eglLoadMatrixf) (const GLfloat *m);
extern void (APIENTRY *eglMatrixMode) (GLenum mode);
extern void (APIENTRY *eglMultMatrixf) (const GLfloat *m);
extern void (APIENTRY *eglNormal3f) (GLfloat nx, GLfloat ny, GLfloat nz);
extern void (APIENTRY *eglOrtho) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern void (APIENTRY *eglPixelStorei) (GLenum pname, GLint param);
extern void (APIENTRY *eglPolygonMode) (GLenum face, GLenum mode);
extern void (APIENTRY *eglPolygonOffset) (GLfloat factor, GLfloat units);
extern void (APIENTRY *eglPopMatrix) (void);
extern void (APIENTRY *eglPushMatrix) (void);
extern void (APIENTRY *eglReadBuffer) (GLenum mode);
extern void (APIENTRY *eglReadPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
extern void (APIENTRY *eglRotatef) (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRY *eglScalef) (GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRY *eglScissor) (GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRY *eglSelectBuffer) (GLsizei size, GLuint *buffer);
extern void (APIENTRY *eglShadeModel) (GLenum mode);
extern void (APIENTRY *eglStencilFunc) (GLenum func, GLint ref, GLuint mask);
extern void (APIENTRY *eglStencilOp) (GLenum fail, GLenum zfail, GLenum zpass);
extern void (APIENTRY *eglTexCoord2f) (GLfloat s, GLfloat t);
extern void (APIENTRY *eglTexCoord2fv) (const GLfloat *v);
extern void (APIENTRY *eglTexEnvf) (GLenum target, GLenum pname, GLfloat param);
extern void (APIENTRY *eglTexEnvi) (GLenum target, GLenum pname, GLint param);
extern void (APIENTRY *eglTexImage2D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void (APIENTRY *eglTexParameterf) (GLenum target, GLenum pname, GLfloat param);
extern void (APIENTRY *eglTexParameteri) (GLenum target, GLenum pname, GLint param);
extern void (APIENTRY *eglTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern void (APIENTRY *eglTranslatef) (GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRY *eglVertex2f) (GLfloat x, GLfloat y);
extern void (APIENTRY *eglVertex2fv) (const GLfloat *v);
extern void (APIENTRY *eglVertex3f) (GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRY *eglVertex3fv) (const GLfloat *v);
extern void (APIENTRY *eglViewport) (GLint x, GLint y, GLsizei width, GLsizei height);

#ifdef _WIN32

// I think I commented this out because it is flat out wrong.
// We were creating variables here :(  In every file, no less!
// Or were we?  Anyway you need an extern on them if you want to do this still.

//LONG (WINAPI *eChangeDisplaySettings) (LPDEVMODE lpDevMode, DWORD dwflags);
//
//HGLRC (WINAPI *ewglCreateContext) (HDC);
//BOOL  (WINAPI *ewglDeleteContext) (HGLRC);
//HGLRC (WINAPI *ewglGetCurrentContext) (VOID);
//HDC   (WINAPI *ewglGetCurrentDC) (VOID);
//PROC  (WINAPI *ewglGetProcAddress)(LPCSTR);
//BOOL  (WINAPI *ewglMakeCurrent) (HDC, HGLRC);
//BOOL  (WINAPI *eSetPixelFormat) (HDC, int, CONST PIXELFORMATDESCRIPTOR *);

#define eSystem_GL_GetProcAddress ewglGetProcAddress

#endif // _WIN32

#ifndef _WIN32
#define eSystem_GL_GetProcAddress System_GL_GetProcAddress
#endif // ! WIN32

#endif //  ! __GL_RENDERER_H


