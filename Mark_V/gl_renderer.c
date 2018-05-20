#ifdef CORE_GL // Ok, even if we aren't GLQUAKE and are instead WinQuake through GL we need this

/*
Copyright (C) 2009-2013 Baker

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// gl_renderer.c


#include "quakedef.h"



// The function set provided by the wrapper
void (APIENTRY *eglAlphaFunc) (GLenum func, GLclampf ref);
void (APIENTRY *eglBegin) (GLenum mode);
void (APIENTRY *eglBindTexture) (GLenum target, GLuint texture);
void (APIENTRY *eglBlendFunc) (GLenum sfactor, GLenum dfactor);
void (APIENTRY *eglClear) (GLbitfield mask);
void (APIENTRY *eglClearColor) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void (APIENTRY *eglClearStencil) (GLint s);
void (APIENTRY *eglColor3f) (GLfloat red, GLfloat green, GLfloat blue);
void (APIENTRY *eglColor3fv) (const GLfloat *v);
void (APIENTRY *eglColor3ubv) (const GLubyte *v);
void (APIENTRY *eglColor4f) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void (APIENTRY *eglColor4fv) (const GLfloat *v);
void (APIENTRY *eglColor4ub) (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void (APIENTRY *eglColor4ubv) (const GLubyte *v);

// Baker: eglCopyTexSubImage2D is not implemented in DX8 wrapper.  I suspect because r_oldwater 0 in Fitz achieves what it is doing
// in a very strange way that would require an unreasonable amount of work and this function was only used there.  I doubt
// this function presented any kind of challenge.

void (APIENTRY *eglCopyTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);

void (APIENTRY *eglColorMask) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void (APIENTRY *eglCullFace) (GLenum mode);
void (APIENTRY *eglDeleteTextures) (GLsizei n, const GLuint *textures);
void (APIENTRY *eglDepthFunc) (GLenum func);
void (APIENTRY *eglDepthMask) (GLboolean flag);
void (APIENTRY *eglDepthRange) (GLclampd zNear, GLclampd zFar);
void (APIENTRY *eglDisable) (GLenum cap);
void (APIENTRY *eglDrawBuffer) (GLenum mode);
void (APIENTRY *eglEnable) (GLenum cap);
void (APIENTRY *eglEnd) (void);
void (APIENTRY *eglFinish) (void);
void (APIENTRY *eglFogf) (GLenum pname, GLfloat param);
void (APIENTRY *eglFogfv) (GLenum pname, const GLfloat *params);
void (APIENTRY *eglFogi) (GLenum pname, GLint param);
void (APIENTRY *eglFogiv) (GLenum pname, const GLint *params);
void (APIENTRY *eglFrontFace) (GLenum mode);
void (APIENTRY *eglFrustum) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void (APIENTRY *eglGenTextures) (GLsizei n, GLuint *textures);
void (APIENTRY *eglGetFloatv) (GLenum pname, GLfloat *params);
void (APIENTRY *eglGetIntegerv) (GLenum pname, GLint *params);
const GLubyte *(APIENTRY *eglGetString) (GLenum name);
void (APIENTRY *eglGetTexImage) (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
void (APIENTRY *eglGetTexParameterfv) (GLenum target, GLenum pname, GLfloat *params);void (APIENTRY *eglHint) (GLenum target, GLenum mode);
void (APIENTRY *eglLineWidth) (GLfloat width);
void (APIENTRY *eglLoadIdentity) (void);
void (APIENTRY *eglLoadMatrixf) (const GLfloat *m);
void (APIENTRY *eglMatrixMode) (GLenum mode);
void (APIENTRY *eglMultMatrixf) (const GLfloat *m);
void (APIENTRY *eglNormal3f) (GLfloat nx, GLfloat ny, GLfloat nz);
void (APIENTRY *eglOrtho) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void (APIENTRY *eglPixelStorei) (GLenum pname, GLint param);
void (APIENTRY *eglPolygonMode) (GLenum face, GLenum mode);
void (APIENTRY *eglPolygonOffset) (GLfloat factor, GLfloat units);
void (APIENTRY *eglPopMatrix) (void);
void (APIENTRY *eglPushMatrix) (void);
void (APIENTRY *eglReadBuffer) (GLenum mode);
void (APIENTRY *eglReadPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void (APIENTRY *eglRotatef) (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void (APIENTRY *eglScalef) (GLfloat x, GLfloat y, GLfloat z);
void (APIENTRY *eglScissor) (GLint x, GLint y, GLsizei width, GLsizei height);
void (APIENTRY *eglSelectBuffer) (GLsizei size, GLuint *buffer);
void (APIENTRY *eglShadeModel) (GLenum mode);
void (APIENTRY *eglStencilFunc) (GLenum func, GLint ref, GLuint mask);
void (APIENTRY *eglStencilOp) (GLenum fail, GLenum zfail, GLenum zpass);
void (APIENTRY *eglTexCoord2f) (GLfloat s, GLfloat t);
void (APIENTRY *eglTexCoord2fv) (const GLfloat *v);
void (APIENTRY *eglTexEnvf) (GLenum target, GLenum pname, GLfloat param);
void (APIENTRY *eglTexEnvi) (GLenum target, GLenum pname, GLint param);
void (APIENTRY *eglTexImage2D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void (APIENTRY *eglTexParameterf) (GLenum target, GLenum pname, GLfloat param);
void (APIENTRY *eglTexParameteri) (GLenum target, GLenum pname, GLint param);
void (APIENTRY *eglTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void (APIENTRY *eglTranslatef) (GLfloat x, GLfloat y, GLfloat z);
void (APIENTRY *eglVertex2f) (GLfloat x, GLfloat y);
void (APIENTRY *eglVertex2fv) (const GLfloat *v);
void (APIENTRY *eglVertex3f) (GLfloat x, GLfloat y, GLfloat z);
void (APIENTRY *eglVertex3fv) (const GLfloat *v);
void (APIENTRY *eglViewport) (GLint x, GLint y, GLsizei width, GLsizei height);

// See vidco_win.c for Windows specific functions like ewglCreateContext, ewglGetProcAddress, eSetPixelFormat, eChangeDisplaySettings, ..

#ifdef DIRECT3D8_WRAPPER // dx8 - Hookup Open GL functions (or at least our equivalents)

void VID_Renderer_Set_Direct3D8 (void)
{
	eglAlphaFunc            = d3dmh_glAlphaFunc;
	eglBegin                = d3dmh_glBegin;
	eglBindTexture          = d3dmh_glBindTexture;
	eglBlendFunc            = d3dmh_glBlendFunc;
	eglClear                = d3dmh_glClear;
	eglClearColor           = d3dmh_glClearColor;
	eglClearStencil         = d3dmh_glClearStencil;
	eglColor3f              = d3dmh_glColor3f;
	eglColor3fv             = d3dmh_glColor3fv;
	eglColor3ubv            = d3dmh_glColor3ubv;
	eglColor4f              = d3dmh_glColor4f;
	eglColor4fv             = d3dmh_glColor4fv;
	eglColor4ub             = d3dmh_glColor4ub;
	eglColor4ubv            = d3dmh_glColor4ubv;
	eglColorMask            = d3dmh_glColorMask;

	eglCopyTexSubImage2D	= d3dmh_glCopyTexSubImage2D;

	eglCullFace             = d3dmh_glCullFace;
	eglDeleteTextures       = d3dmh_glDeleteTextures;
	eglDepthFunc            = d3dmh_glDepthFunc;
	eglDepthMask            = d3dmh_glDepthMask;
	eglDepthRange           = d3dmh_glDepthRange;
	eglDisable              = d3dmh_glDisable;
	eglDrawBuffer           = d3dmh_glDrawBuffer;
	eglEnable               = d3dmh_glEnable;
	eglEnd                  = d3dmh_glEnd;
	eglFinish               = d3dmh_glFinish;
	eglFogf                 = d3dmh_glFogf;
	eglFogfv                = d3dmh_glFogfv;
	eglFogi                 = d3dmh_glFogi;
	eglFogiv                = d3dmh_glFogiv;
	eglFrontFace            = d3dmh_glFrontFace;
	eglFrustum              = d3dmh_glFrustum;
	eglGenTextures          = d3dmh_glGenTextures;
	eglGetFloatv            = d3dmh_glGetFloatv;
	eglGetIntegerv          = d3dmh_glGetIntegerv;
	eglGetString            = d3dmh_glGetString;
	eglGetTexImage          = d3dmh_glGetTexImage;
	eglGetTexParameterfv    = d3dmh_glGetTexParameterfv;
	eglHint                 = d3dmh_glHint;
	eglLineWidth            = d3dmh_glLineWidth;
	eglLoadIdentity         = d3dmh_glLoadIdentity;
	eglLoadMatrixf          = d3dmh_glLoadMatrixf;
	eglMatrixMode           = d3dmh_glMatrixMode;
	eglMultMatrixf          = d3dmh_glMultMatrixf;
	eglNormal3f             = d3dmh_glNormal3f;
	eglOrtho                = d3dmh_glOrtho;
	eglPixelStorei			= d3dmh_glPixelStorei;
	eglPolygonMode          = d3dmh_glPolygonMode;
	eglPolygonOffset        = d3dmh_glPolygonOffset;
	eglPopMatrix            = d3dmh_glPopMatrix;
	eglPushMatrix           = d3dmh_glPushMatrix;
	eglReadBuffer           = d3dmh_glReadBuffer;
	eglReadPixels           = d3dmh_glReadPixels;
	eglRotatef              = d3dmh_glRotatef;
	eglScalef               = d3dmh_glScalef;
	eglScissor              = d3dmh_glScissor;
	eglShadeModel           = d3dmh_glShadeModel;
	eglStencilFunc          = d3dmh_glStencilFunc;
	eglStencilOp            = d3dmh_glStencilOp;
	eglTexCoord2f           = d3dmh_glTexCoord2f;
	eglTexCoord2fv          = d3dmh_glTexCoord2fv;
	eglTexEnvf              = d3dmh_glTexEnvf;
	eglTexEnvi              = d3dmh_glTexEnvi;
	eglTexImage2D           = d3dmh_glTexImage2D;
	eglTexParameterf        = d3dmh_glTexParameterf;
	eglTexParameteri        = d3dmh_glTexParameteri;
	eglTexSubImage2D        = d3dmh_glTexSubImage2D;
	eglTranslatef           = d3dmh_glTranslatef;
	eglVertex2f             = d3dmh_glVertex2f;
	eglVertex2fv            = d3dmh_glVertex2fv;
	eglVertex3f             = d3dmh_glVertex3f;
	eglVertex3fv            = d3dmh_glVertex3fv;
	eglViewport             = d3dmh_glViewport;

#ifdef _WIN32
	ewglCreateContext       = Direct3D8_wglCreateContext;
	ewglDeleteContext       = Direct3D8_wglDeleteContext;
	ewglGetCurrentContext   = Direct3D8_wglGetCurrentContext;
	ewglGetCurrentDC        = Direct3D8_wglGetCurrentDC;
	ewglMakeCurrent         = Direct3D8_wglMakeCurrent;
	ewglGetProcAddress		= Direct3D8_wglGetProcAddress;

	eSwapBuffers			= Direct3D8_SwapBuffers;

	eChoosePixelFormat		= ChoosePixelFormat; // The real winapi one
	eDescribePixelFormat	= DescribePixelFormat; // The real winapi one
	eSetPixelFormat         = Direct3D8_SetPixelFormat;

	eChangeDisplaySettings  = Direct3D8_ChangeDisplaySettings;
	eEnumDisplaySettings    = EnumDisplaySettings; // The real winapi one.
#endif // _WIN32

}

#elif defined(DIRECT3D9_WRAPPER)

void VID_Renderer_Set_Direct3D9 (void)
{
	eglAlphaFunc            = d3d9mh_glAlphaFunc;
	eglBegin                = d3d9mh_glBegin;
	eglBindTexture          = d3d9mh_glBindTexture;
	eglBlendFunc            = d3d9mh_glBlendFunc;
	eglClear                = d3d9mh_glClear;
	eglClearColor           = d3d9mh_glClearColor;
	eglClearStencil         = d3d9mh_glClearStencil;
	eglColor3f              = d3d9mh_glColor3f;
	eglColor3fv             = d3d9mh_glColor3fv;
	eglColor3ubv            = d3d9mh_glColor3ubv;
	eglColor4f              = d3d9mh_glColor4f;
	eglColor4fv             = d3d9mh_glColor4fv;
	eglColor4ub             = d3d9mh_glColor4ub;
	eglColor4ubv            = d3d9mh_glColor4ubv;
	eglColorMask            = d3d9mh_glColorMask;

	eglCopyTexSubImage2D	= d3d9mh_glCopyTexSubImage2D;

	eglCullFace             = d3d9mh_glCullFace;
	eglDeleteTextures       = d3d9mh_glDeleteTextures;
	eglDepthFunc            = d3d9mh_glDepthFunc;
	eglDepthMask            = d3d9mh_glDepthMask;
	eglDepthRange           = d3d9mh_glDepthRange;
	eglDisable              = d3d9mh_glDisable;
	eglDrawBuffer           = d3d9mh_glDrawBuffer;
	eglEnable               = d3d9mh_glEnable;
	eglEnd                  = d3d9mh_glEnd;
	eglFinish               = d3d9mh_glFinish;
	eglFogf                 = d3d9mh_glFogf;
	eglFogfv                = d3d9mh_glFogfv;
	eglFogi                 = d3d9mh_glFogi;
	eglFogiv                = d3d9mh_glFogiv;
	eglFrontFace            = d3d9mh_glFrontFace;
	eglFrustum              = d3d9mh_glFrustum;
	eglGenTextures          = d3d9mh_glGenTextures;
	eglGetFloatv            = d3d9mh_glGetFloatv;
	eglGetIntegerv          = d3d9mh_glGetIntegerv;
	eglGetString            = d3d9mh_glGetString;
	eglGetTexImage          = d3d9mh_glGetTexImage;
	eglGetTexParameterfv    = d3d9mh_glGetTexParameterfv;
	eglHint                 = d3d9mh_glHint;
	eglLineWidth            = d3d9mh_glLineWidth;
	eglLoadIdentity         = d3d9mh_glLoadIdentity;
	eglLoadMatrixf          = d3d9mh_glLoadMatrixf;
	eglMatrixMode           = d3d9mh_glMatrixMode;
	eglMultMatrixf          = d3d9mh_glMultMatrixf;
	eglNormal3f             = d3d9mh_glNormal3f;
	eglOrtho                = d3d9mh_glOrtho;
	eglPixelStorei			= d3d9mh_glPixelStorei;
	eglPolygonMode          = d3d9mh_glPolygonMode;
	eglPolygonOffset        = d3d9mh_glPolygonOffset;
	eglPopMatrix            = d3d9mh_glPopMatrix;
	eglPushMatrix           = d3d9mh_glPushMatrix;
	eglReadBuffer           = d3d9mh_glReadBuffer;
	eglReadPixels           = d3d9mh_glReadPixels;
	eglRotatef              = d3d9mh_glRotatef;
	eglScalef               = d3d9mh_glScalef;
	eglScissor              = d3d9mh_glScissor;
	eglShadeModel           = d3d9mh_glShadeModel;
	eglStencilFunc          = d3d9mh_glStencilFunc;
	eglStencilOp            = d3d9mh_glStencilOp;
	eglTexCoord2f           = d3d9mh_glTexCoord2f;
	eglTexCoord2fv          = d3d9mh_glTexCoord2fv;
	eglTexEnvf              = d3d9mh_glTexEnvf;
	eglTexEnvi              = d3d9mh_glTexEnvi;
	eglTexImage2D           = d3d9mh_glTexImage2D;
	eglTexParameterf        = d3d9mh_glTexParameterf;
	eglTexParameteri        = d3d9mh_glTexParameteri;
	eglTexSubImage2D        = d3d9mh_glTexSubImage2D;
	eglTranslatef           = d3d9mh_glTranslatef;
	eglVertex2f             = d3d9mh_glVertex2f;
	eglVertex2fv            = d3d9mh_glVertex2fv;
	eglVertex3f             = d3d9mh_glVertex3f;
	eglVertex3fv            = d3d9mh_glVertex3fv;
	eglViewport             = d3d9mh_glViewport;

#ifdef _WIN32
	ewglCreateContext       = Direct3D9_wglCreateContext;
	ewglDeleteContext       = Direct3D9_wglDeleteContext;
	ewglGetCurrentContext   = Direct3D9_wglGetCurrentContext;
	ewglGetCurrentDC        = Direct3D9_wglGetCurrentDC;
	ewglMakeCurrent         = Direct3D9_wglMakeCurrent;
	ewglGetProcAddress		= Direct3D9_wglGetProcAddress;

	eChoosePixelFormat		= Direct3D9_ChoosePixelFormat; // The real winapi one
	eDescribePixelFormat	= Direct3D9_DescribePixelFormat; // The real winapi one
	eSetPixelFormat         = Direct3D9_SetPixelFormat;
	eSwapBuffers			= Direct3D9_SwapBuffers;
	eChangeDisplaySettings  = Direct3D9_ChangeDisplaySettings;
	eEnumDisplaySettings    = Direct3D9_EnumDisplaySettings; // The DX9 one.
#endif // WIN32

}

#else // OpenGL

void VID_Renderer_Set_OpenGL (void)
{
#ifdef PLATFORM_WINDOWS // Even SDL needs this
	const char *binary_folder		= Folder_Binary_Folder_URL ();
	const char *opengl32_dll_url	= va("%s/opengl32.dll", binary_folder);


	if (File_Exists (opengl32_dll_url)) {
		const char *Shell_Windows_Folder_System32 (void);
		const char *sys32dir = Shell_Windows_Folder_System32 (); // ("SYSTEM");
		const char *system32_opengl32_dll_url = va("%s/opengl32.dll", sys32dir);
		const char *failure_message =
			"OpenGL32.dll detected in Quake folder, this file is almost without exception obsolete and will result in the engine crashing." NEWLINE NEWLINE
			"Tried to use standard OpenGL32.dll located at " QUOTED_S " but %s." NEWLINE NEWLINE
			"Please rename or delete the opengl32.dll in your Quake folder." NEWLINE NEWLINE
			"Opening folder ...";

		Con_WarningLinef ("An OpenGL32.DLL was found in the same folder as the binary " QUOTED_S, opengl32_dll_url);
		Con_WarningLinef ("Attempting reroute to " QUOTED_S, system32_opengl32_dll_url);
		if (!File_Exists (system32_opengl32_dll_url)) {
			Con_WarningLinef ("Failed as file not found " QUOTED_S, system32_opengl32_dll_url);
			msgbox ("OpenGL32.dll detected in Quake folder", failure_message, system32_opengl32_dll_url, "file not found");
			Folder_Open_Highlight (opengl32_dll_url);
			System_Error ("OpenGL32.dll found in Quake folder.  Please remedy and restart.");
		}
		else
		{
			HMODULE hOpenGL32 = LoadLibraryA (system32_opengl32_dll_url);
			void *last;
			const char *str_failed_proc = NULL;

			if (!hOpenGL32) {
				Con_WarningLinef ("Failed as DLL would not load " QUOTED_S, system32_opengl32_dll_url);
				msgbox ("OpenGL32.dll detected in Quake folder", failure_message, system32_opengl32_dll_url, "DLL could not load.");
				Folder_Open_Highlight (opengl32_dll_url);
				System_Error ("OpenGL32.dll found in Quake folder.  Please remedy and restart.");
			}


			#define OPENGL_GETFUNC(f) (last = (void *)GetProcAddress(hOpenGL32, #f)); if (!last) str_failed_proc = #f
			eglAlphaFunc            = OPENGL_GETFUNC(glAlphaFunc);
			eglBegin                = OPENGL_GETFUNC(glBegin);
			eglBindTexture          = OPENGL_GETFUNC(glBindTexture);
			eglBlendFunc            = OPENGL_GETFUNC(glBlendFunc);
			eglClear                = OPENGL_GETFUNC(glClear);
			eglClearColor           = OPENGL_GETFUNC(glClearColor);
			eglClearStencil         = OPENGL_GETFUNC(glClearStencil);
			eglColor3f              = OPENGL_GETFUNC(glColor3f);
			eglColor3fv             = OPENGL_GETFUNC(glColor3fv);
			eglColor3ubv            = OPENGL_GETFUNC(glColor3ubv);
			eglColor4f              = OPENGL_GETFUNC(glColor4f);
			eglColor4fv             = OPENGL_GETFUNC(glColor4fv);
			eglColor4ub             = OPENGL_GETFUNC(glColor4ub);
			eglColor4ubv            = OPENGL_GETFUNC(glColor4ubv);
			eglColorMask            = OPENGL_GETFUNC(glColorMask);

			eglCopyTexSubImage2D	= OPENGL_GETFUNC(glCopyTexSubImage2D);

			eglCullFace             = OPENGL_GETFUNC(glCullFace);
			eglDeleteTextures       = OPENGL_GETFUNC(glDeleteTextures);
			eglDepthFunc            = OPENGL_GETFUNC(glDepthFunc);
			eglDepthMask            = OPENGL_GETFUNC(glDepthMask);
			eglDepthRange           = OPENGL_GETFUNC(glDepthRange);
			eglDisable              = OPENGL_GETFUNC(glDisable);
			eglDrawBuffer           = OPENGL_GETFUNC(glDrawBuffer);
			eglEnable               = OPENGL_GETFUNC(glEnable);
			eglEnd                  = OPENGL_GETFUNC(glEnd);
			eglFinish               = OPENGL_GETFUNC(glFinish);
			eglFogf                 = OPENGL_GETFUNC(glFogf);
			eglFogfv                = OPENGL_GETFUNC(glFogfv);
			eglFogi                 = OPENGL_GETFUNC(glFogi);
			eglFogiv                = OPENGL_GETFUNC(glFogiv);
			eglFrontFace            = OPENGL_GETFUNC(glFrontFace);
			eglFrustum              = OPENGL_GETFUNC(glFrustum);
			eglGenTextures          = OPENGL_GETFUNC(glGenTextures);
			eglGetFloatv            = OPENGL_GETFUNC(glGetFloatv);
			eglGetIntegerv          = OPENGL_GETFUNC(glGetIntegerv);
			eglGetString            = OPENGL_GETFUNC(glGetString);
			eglGetTexImage          = OPENGL_GETFUNC(glGetTexImage);
			eglGetTexParameterfv    = OPENGL_GETFUNC(glGetTexParameterfv);
			eglHint                 = OPENGL_GETFUNC(glHint);
			eglLineWidth            = OPENGL_GETFUNC(glLineWidth);
			eglLoadIdentity         = OPENGL_GETFUNC(glLoadIdentity);
			eglLoadMatrixf          = OPENGL_GETFUNC(glLoadMatrixf);
			eglMatrixMode           = OPENGL_GETFUNC(glMatrixMode);
			eglMultMatrixf          = OPENGL_GETFUNC(glMultMatrixf);
			eglNormal3f             = OPENGL_GETFUNC(glNormal3f);
			eglOrtho                = OPENGL_GETFUNC(glOrtho);
			eglPixelStorei			= OPENGL_GETFUNC(glPixelStorei);
			eglPolygonMode          = OPENGL_GETFUNC(glPolygonMode);
			eglPolygonOffset        = OPENGL_GETFUNC(glPolygonOffset);
			eglPopMatrix            = OPENGL_GETFUNC(glPopMatrix);
			eglPushMatrix           = OPENGL_GETFUNC(glPushMatrix);
			eglReadBuffer           = OPENGL_GETFUNC(glReadBuffer);
			eglReadPixels           = OPENGL_GETFUNC(glReadPixels);
			eglRotatef              = OPENGL_GETFUNC(glRotatef);
			eglScalef               = OPENGL_GETFUNC(glScalef);
			eglScissor              = OPENGL_GETFUNC(glScissor);
			eglShadeModel           = OPENGL_GETFUNC(glShadeModel);
			eglStencilFunc          = OPENGL_GETFUNC(glStencilFunc);
			eglStencilOp            = OPENGL_GETFUNC(glStencilOp);
			eglTexCoord2f           = OPENGL_GETFUNC(glTexCoord2f);
			eglTexCoord2fv          = OPENGL_GETFUNC(glTexCoord2fv);
			eglTexEnvf              = OPENGL_GETFUNC(glTexEnvf);
			eglTexEnvi              = OPENGL_GETFUNC(glTexEnvi);
			eglTexImage2D           = OPENGL_GETFUNC(glTexImage2D);
			eglTexParameterf        = OPENGL_GETFUNC(glTexParameterf);
			eglTexParameteri        = OPENGL_GETFUNC(glTexParameteri);
			eglTexSubImage2D        = OPENGL_GETFUNC(glTexSubImage2D);
			eglTranslatef           = OPENGL_GETFUNC(glTranslatef);
			eglVertex2f             = OPENGL_GETFUNC(glVertex2f);
			eglVertex2fv            = OPENGL_GETFUNC(glVertex2fv);
			eglVertex3f             = OPENGL_GETFUNC(glVertex3f);
			eglVertex3fv            = OPENGL_GETFUNC(glVertex3fv);
			eglViewport             = OPENGL_GETFUNC(glViewport);

			#ifdef PLATFORM_GUI_WINDOWS // SDL does not use these
			ewglCreateContext       = OPENGL_GETFUNC(wglCreateContext);
			ewglDeleteContext       = OPENGL_GETFUNC(wglDeleteContext);
			ewglGetCurrentContext   = OPENGL_GETFUNC(wglGetCurrentContext);
			ewglGetCurrentDC        = OPENGL_GETFUNC(wglGetCurrentDC);
			ewglMakeCurrent         = OPENGL_GETFUNC(wglMakeCurrent);
			ewglGetProcAddress		= OPENGL_GETFUNC(wglGetProcAddress);

			eChoosePixelFormat		= ChoosePixelFormat; // The real winapi one
			eDescribePixelFormat	= DescribePixelFormat; // The real winapi one
			eSetPixelFormat         = SetPixelFormat;
			eSwapBuffers			= SwapBuffers;

			eChangeDisplaySettings  = ChangeDisplaySettings; // Different beast!  Not in opengl32.dll
			#endif // PLATFORM_GUI_WINDOWS

			if (str_failed_proc) {
				const char *str_fail_reason = va("(%s function not found)", str_failed_proc);
				Con_WarningLinef (QUOTED_S " loading " QUOTED_S, str_fail_reason, system32_opengl32_dll_url);
				msgbox ("OpenGL32.dll detected in Quake folder", failure_message, system32_opengl32_dll_url, str_fail_reason);
				Folder_Open_Highlight (opengl32_dll_url);
				System_Error ("OpenGL32.dll found in Quake folder.  Please remedy and restart.");
			}

			Con_WarningLinef ("Succeeded in reroute use of " QUOTED_S, system32_opengl32_dll_url);
			// FreeLibrary (hOpenGL32);  // Don't do that!
			return;
		}

	}
	// If we are here, on Windows there was no opengl32.dll in the folder.

	{
		HMODULE hOpenGL32 = LoadLibraryA ("opengl32.dll"); // Hit it here. MH fix dwere.
		GetProcAddress (hOpenGL32, "glBegin");
	}
#endif // PLATFORM_WINDOWS // Even SDL needs this

	eglAlphaFunc            = glAlphaFunc;
	eglBegin                = glBegin;
	eglBindTexture          = glBindTexture;
	eglBlendFunc            = glBlendFunc;
	eglClear                = glClear;
	eglClearColor           = glClearColor;
	eglClearStencil         = glClearStencil;
	eglColor3f              = glColor3f;
	eglColor3fv             = glColor3fv;
	eglColor3ubv            = glColor3ubv;
	eglColor4f              = glColor4f;
	eglColor4fv             = glColor4fv;
	eglColor4ub             = glColor4ub;
	eglColor4ubv            = glColor4ubv;
	eglColorMask            = glColorMask;

	eglCopyTexSubImage2D	= glCopyTexSubImage2D;

	eglCullFace             = glCullFace;
	eglDeleteTextures       = glDeleteTextures;
	eglDepthFunc            = glDepthFunc;
	eglDepthMask            = glDepthMask;
	eglDepthRange           = glDepthRange;
	eglDisable              = glDisable;
	eglDrawBuffer           = glDrawBuffer;
	eglEnable               = glEnable;
	eglEnd                  = glEnd;
	eglFinish               = glFinish;
	eglFogf                 = glFogf;
	eglFogfv                = glFogfv;
	eglFogi                 = glFogi;
	eglFogiv                = glFogiv;
	eglFrontFace            = glFrontFace;
	eglFrustum              = glFrustum;
	eglGenTextures          = glGenTextures;
	eglGetFloatv            = glGetFloatv;
	eglGetIntegerv          = glGetIntegerv;
	eglGetString            = glGetString;
	eglGetTexImage          = glGetTexImage;
	eglGetTexParameterfv    = glGetTexParameterfv;
	eglHint                 = glHint;
	eglLineWidth            = glLineWidth;
	eglLoadIdentity         = glLoadIdentity;
	eglLoadMatrixf          = glLoadMatrixf;
	eglMatrixMode           = glMatrixMode;
	eglMultMatrixf          = glMultMatrixf;
	eglNormal3f             = glNormal3f;
	eglOrtho                = glOrtho;
	eglPixelStorei			= glPixelStorei;
	eglPolygonMode          = glPolygonMode;
	eglPolygonOffset        = glPolygonOffset;
	eglPopMatrix            = glPopMatrix;
	eglPushMatrix           = glPushMatrix;
	eglReadBuffer           = glReadBuffer;
	eglReadPixels           = glReadPixels;
	eglRotatef              = glRotatef;
	eglScalef               = glScalef;
	eglScissor              = glScissor;
	eglShadeModel           = glShadeModel;
	eglStencilFunc          = glStencilFunc;
	eglStencilOp            = glStencilOp;
	eglTexCoord2f           = glTexCoord2f;
	eglTexCoord2fv          = glTexCoord2fv;
	eglTexEnvf              = glTexEnvf;
	eglTexEnvi              = glTexEnvi;
	eglTexImage2D           = glTexImage2D;
	eglTexParameterf        = glTexParameterf;
	eglTexParameteri        = glTexParameteri;
	eglTexSubImage2D        = glTexSubImage2D;
	eglTranslatef           = glTranslatef;
	eglVertex2f             = glVertex2f;
	eglVertex2fv            = glVertex2fv;
	eglVertex3f             = glVertex3f;
	eglVertex3fv            = glVertex3fv;
	eglViewport             = glViewport;

#ifdef PLATFORM_GUI_WINDOWS // SDL does not use these
	ewglCreateContext       = wglCreateContext;
	ewglDeleteContext       = wglDeleteContext;
	ewglGetCurrentContext   = wglGetCurrentContext;
	ewglGetCurrentDC        = wglGetCurrentDC;
	ewglMakeCurrent         = wglMakeCurrent;
	ewglGetProcAddress		= wglGetProcAddress;

	eChoosePixelFormat		= ChoosePixelFormat; // The real winapi one
	eDescribePixelFormat	= DescribePixelFormat; // The real winapi one
	eSetPixelFormat         = SetPixelFormat;
	eSwapBuffers			= SwapBuffers;
	eChangeDisplaySettings  = ChangeDisplaySettings;
	eEnumDisplaySettings    = EnumDisplaySettings; // The real winapi one.
#endif // PLATFORM_GUI_WINDOWS
}

#endif // ! DIRECT3DX_WRAPPER


void VID_Renderer_Setup (void)
{

#if defined(DIRECT3D8_WRAPPER) // DX8 Wrapper build
	VID_Renderer_Set_Direct3D8 ();
#elif defined(DIRECT3D9_WRAPPER)  // DX9 Wrapper build
	VID_Renderer_Set_Direct3D9 ();
#else
	VID_Renderer_Set_OpenGL ();
#endif // DIRECT3D_WRAPPER

	vid.direct3d = DIRECT3D_WRAPPER_VERSION;
}

#endif // CORE_GL
