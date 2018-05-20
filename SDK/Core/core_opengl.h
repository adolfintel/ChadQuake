#ifdef CORE_GL
/*
Copyright (C) 2012-2014 Baker

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
// core_opengl.h -- portable opengl header

#ifndef __CORE_OPENGL_H__
#define __CORE_OPENGL_H__

///////////////////////////////////////////////////////////////////////////////
//  OPENGL: Determine platform #includes
///////////////////////////////////////////////////////////////////////////////



#ifdef PLATFORM_WINDOWS
	#include "core_windows.h"

	#ifdef DIRECT3DX_WRAPPER // dx8 + dx9 - Need "gl_constants.h"
		#include "gl_constants.h"
	#endif // DIRECT3DX_WRAPPER // dx8 + dx9 - Need "gl_constants.h"

	#ifndef DIRECT3DX_WRAPPER
		#include <GL/gl.h>
		#include <GL/glu.h>
		#pragma comment (lib, "opengl32.lib")
		#pragma comment (lib, "glu32.lib")
	#endif // DIRECT3DX_WRAPPER

	#undef APIENTRY
	#define APIENTRY WINAPI
	
	#ifdef PLATFORM_GUI_WINDOWS
		#if defined(CORE_GL) && defined (PLATFORM_WINDOWS)
			extern LONG (WINAPI *eChangeDisplaySettings) (LPDEVMODE lpDevMode, DWORD dwflags);
			extern LONG (WINAPI *eEnumDisplaySettings) (LPCSTR lpszDeviceName, DWORD iModeNum, LPDEVMODE lpDevMode);

			extern HGLRC (WINAPI *ewglCreateContext) (HDC);
			extern BOOL  (WINAPI *ewglDeleteContext) (HGLRC);
			extern HGLRC (WINAPI *ewglGetCurrentContext) (VOID);
			extern HDC   (WINAPI *ewglGetCurrentDC) (VOID);
			extern PROC  (WINAPI *ewglGetProcAddress)(LPCSTR);
			extern BOOL  (WINAPI *ewglMakeCurrent) (HDC, HGLRC);
			extern BOOL  (WINAPI *eChoosePixelFormat) (HDC, CONST PIXELFORMATDESCRIPTOR *);
			extern BOOL  (WINAPI *eDescribePixelFormat) (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
			extern BOOL  (WINAPI *eSetPixelFormat) (HDC, int, CONST PIXELFORMATDESCRIPTOR *);
			extern BOOL  (WINAPI *eSwapBuffers) (HDC);
		#endif // CORE_GL + PLATFORM_WINDOWS
	#endif // PLATFORM_GUI_WINDOWS

#else
	#define APIENTRY
#endif // PLATFORM_WINDOWS

#ifdef PLATFORM_OSX
	#include 	<OpenGL/gl.h>
	#include 	<OpenGL/glu.h>
	#include	<OpenGL/glext.h>
	#include	<math.h>
#endif // PLATFORM_OSX

#ifdef PLATFORM_IOS
	#include <OpenGLES/ES1/gl.h>				// OpenGL ES 1
	#include <OpenGLES/ES1/glext.h>				// OpenGL ES 1
	#define PLATFORM_OPENGLES
#endif

#ifdef PLATFORM_LINUX
//	#include 	<OpenGL/gl.h>
//	#include 	<OpenGL/glu.h>
//	#include	<OpenGL/glext.h>

    #include    <SDL2/SDL_opengl.h>//    #include
#endif

#ifdef PLATFORM_ANDROID
    #include <GLES/gl.h>
	#include <GLES/glext.h>

	#define PLATFORM_OPENGLES
#endif

#ifdef PLATFORM_OPENGLES
//	#pragma message ("Not orthoed")
	#define glOrtho glOrthof
	#define glFrustum glFrustumf
#else
	#define glOrthof "Nope"			// We want consistency.  Use "glOrtho" and "glFrustum".
	#define glFrustumf "Nope"		// NEVER glOrthof or glFrustumf.  We cannot #define glOrthof to glOrtho and disallow glOrtho
//	#pragma message ("Orthoed")		// Because that would be macro recursion.  So glOrtho and glFrustum it is!
#endif // PLATFORM_OPENGL_DESKTOP	// Note that in decent and final code, we will never be using either of those anyway.


///////////////////////////////////////////////////////////////////////////////
//  OPENGL: Patch up stuff missing from gl.h and friends
///////////////////////////////////////////////////////////////////////////////

#ifndef PLATFORM_OPENGLES // Hmmmm.  I'm not sure this will survive.  I would wire the functions to glActiveTexture and glMultiTexCoord2f or whatever its name is.
	typedef void (APIENTRY *PFNGLMULTITEXCOORD2FARBPROC) (GLenum, GLfloat, GLfloat);
	typedef void (APIENTRY *PFNGLACTIVETEXTUREARBPROC) (GLenum);
	typedef void (APIENTRY *qGLColorTableEXT) (int, int, int, int, int, const void*);
	typedef void (APIENTRY *qgl3DfxSetPaletteEXT) (GLuint *);
#endif // PLATFORM_OPENGL_DESKTOP

#ifndef GL_CLAMP_TO_EDGE
	#define GL_CLAMP_TO_EDGE 0x812F
#endif // GL_CLAMP_TO_EDGE not defined

#ifdef PLATFORM_WINDOWS

	#define GL_TEXTURE0						  0x84C0
	#define GL_TEXTURE1						  0x84C1
	#define GL_TEXTURE2					      0x84C2
	#define GL_TEXTURE3						  0x84C3

	#define GL_MAX_TEXTURE_UNITS              0x84E2

	#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
	#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

	#define GL_COMBINE                        0x8570
	#define GL_COMBINE_RGB                    0x8571
	#define GL_COMBINE_ALPHA                  0x8572
	#define GL_RGB_SCALE                      0x8573
	#define GL_ADD_SIGNED                     0x8574
	#define GL_INTERPOLATE                    0x8575
	#define GL_CONSTANT                       0x8576
	#define GL_PRIMARY_COLOR                  0x8577
	#define GL_PREVIOUS                       0x8578
	#define GL_SUBTRACT                       0x84E7

	#define GL_SRC0_RGB                       0x8580
	#define GL_SRC1_RGB                       0x8581
	#define GL_SRC2_RGB                       0x8582
	#define GL_SRC0_ALPHA                     0x8588
	#define GL_SRC1_ALPHA                     0x8589
	#define GL_SRC2_ALPHA                     0x858A

	#define GL_OPERAND0_RGB                   0x8590
	#define GL_OPERAND1_RGB                   0x8591
	#define GL_OPERAND2_RGB                   0x8592
	#define GL_OPERAND0_ALPHA                 0x8598
	#define GL_OPERAND1_ALPHA                 0x8599
	#define GL_OPERAND2_ALPHA                 0x859A

/*
	// OpenGL ES does not have GL_BGRA
	//#define GL_BGRA							0x80E1

	#define GL_UNSIGNED_INT_8_8_8_8_REV 		0x8367

	#define	GL_TEXTURE0_ARB 					0x84C0
	#define	GL_TEXTURE1_ARB 					0x84C1
	#define	GL_TEXTURE2_ARB 					0x84C2
	#define	GL_TEXTURE3_ARB 					0x84C3

	#define GL_MAX_TEXTURE_UNITS_ARB			0x84E2

	#define	GL_TEXTURE_MAX_ANISOTROPY_EXT 		0x84FE
	#define	GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 	0x84FF

	#define GL_COMBINE_EXT						0x8570
	#define GL_COMBINE_RGB_EXT					0x8571
	#define GL_COMBINE_ALPHA_EXT				0x8572
	#define GL_RGB_SCALE_EXT					0x8573
	#define GL_CONSTANT_EXT						0x8576
	#define GL_PRIMARY_COLOR_EXT				0x8577
	#define GL_PREVIOUS_EXT						0x8578
	#define GL_SOURCE0_RGB_EXT					0x8580
	#define GL_SOURCE1_RGB_EXT					0x8581
	#define GL_SOURCE0_ALPHA_EXT				0x8588
	#define GL_SOURCE1_ALPHA_EXT				0x8589
	#define GL_OPERAND0_RGB_EXT					0x8590
	#define GL_OPERAND1_RGB_EXT					0x8591
	#define GL_OPERAND2_RGB_EXT					0x8592
 */

#endif // PLATFORM_WINDOWS

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OUR STUFF
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void _GL_CheckError (const char *funcname, int linenum);

#define GL_CheckError _GL_CheckError (NULL, __LINE__)

#ifdef _DEBUG
	#undef  GL_CheckError
	#define GL_CheckError _GL_CheckError (__func__, __LINE__)
#endif

#endif // __CORE_OPENGL_H__

#endif // CORE_GL
