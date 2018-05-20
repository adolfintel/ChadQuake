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

// unimplemented for various reasons: either the functionality doesn't exist in D3D, is not hardware-accelerated, or i didn't bother

#ifdef DIRECT3D9_WRAPPER
#include "d3d9_internal.h"

void APIENTRY d3d9mh_glAccum (GLenum op, GLfloat value) {}
GLboolean APIENTRY d3d9mh_glAreTexturesResident (GLsizei n, const GLuint *textures, GLboolean *residences) {return GL_FALSE;}

void APIENTRY d3d9mh_glBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap) {}
void APIENTRY d3d9mh_glCallList (GLuint list) {}
void APIENTRY d3d9mh_glCallLists (GLsizei n, GLenum type, const GLvoid *lists) {}
void APIENTRY d3d9mh_glClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {}
void APIENTRY d3d9mh_glClearIndex (GLfloat c) {}
void APIENTRY d3d9mh_glClipPlane (GLenum plane, const GLdouble *equation) {}
void APIENTRY d3d9mh_glColor3b (GLbyte red, GLbyte green, GLbyte blue) {}
void APIENTRY d3d9mh_glColor3bv (const GLbyte *v) {}
void APIENTRY d3d9mh_glColor3d (GLdouble red, GLdouble green, GLdouble blue) {}
void APIENTRY d3d9mh_glColor3dv (const GLdouble *v) {}
void APIENTRY d3d9mh_glColor3i (GLint red, GLint green, GLint blue) {}
void APIENTRY d3d9mh_glColor3iv (const GLint *v) {}
void APIENTRY d3d9mh_glColor3s (GLshort red, GLshort green, GLshort blue) {}
void APIENTRY d3d9mh_glColor3sv (const GLshort *v) {}
void APIENTRY d3d9mh_glColor3ub (GLubyte red, GLubyte green, GLubyte blue) {}
void APIENTRY d3d9mh_glColor3ui (GLuint red, GLuint green, GLuint blue) {}
void APIENTRY d3d9mh_glColor3uiv (const GLuint *v) {}
void APIENTRY d3d9mh_glColor3us (GLushort red, GLushort green, GLushort blue) {}
void APIENTRY d3d9mh_glColor3usv (const GLushort *v) {}
void APIENTRY d3d9mh_glColor4b (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha) {}
void APIENTRY d3d9mh_glColor4bv (const GLbyte *v) {}
void APIENTRY d3d9mh_glColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) {}
void APIENTRY d3d9mh_glColor4dv (const GLdouble *v) {}
void APIENTRY d3d9mh_glColor4i (GLint red, GLint green, GLint blue, GLint alpha) {}
void APIENTRY d3d9mh_glColor4iv (const GLint *v) {}
void APIENTRY d3d9mh_glColor4s (GLshort red, GLshort green, GLshort blue, GLshort alpha) {}
void APIENTRY d3d9mh_glColor4sv (const GLshort *v) {}
void APIENTRY d3d9mh_glColor4ui (GLuint red, GLuint green, GLuint blue, GLuint alpha) {}
void APIENTRY d3d9mh_glColor4uiv (const GLuint *v) {}
void APIENTRY d3d9mh_glColor4us (GLushort red, GLushort green, GLushort blue, GLushort alpha) {}
void APIENTRY d3d9mh_glColor4usv (const GLushort *v) {}
void APIENTRY d3d9mh_glColorMaterial (GLenum face, GLenum mode) {}

void APIENTRY d3d9mh_glCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type) {}
void APIENTRY d3d9mh_glCopyTexImage1D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border) {}
void APIENTRY d3d9mh_glCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {}
void APIENTRY d3d9mh_glDeleteLists (GLuint list, GLsizei range) {}

void APIENTRY d3d9mh_glDrawBuffer (GLenum mode)
{
	// d3d doesn't like us messing with the front buffer, which is all glquake uses this for, so here
	// we just silently ignore requests to change the draw buffer
}

void APIENTRY d3d9mh_glDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {}
void APIENTRY d3d9mh_glEdgeFlag (GLboolean flag) {}

void APIENTRY d3d9mh_glEdgeFlagv (const GLboolean *flag) {}

void APIENTRY d3d9mh_glEndList (void) {}
void APIENTRY d3d9mh_glEvalCoord1d (GLdouble u) {}
void APIENTRY d3d9mh_glEvalCoord1dv (const GLdouble *u) {}
void APIENTRY d3d9mh_glEvalCoord1f (GLfloat u) {}
void APIENTRY d3d9mh_glEvalCoord1fv (const GLfloat *u) {}
void APIENTRY d3d9mh_glEvalCoord2d (GLdouble u, GLdouble v) {}
void APIENTRY d3d9mh_glEvalCoord2dv (const GLdouble *u) {}
void APIENTRY d3d9mh_glEvalCoord2f (GLfloat u, GLfloat v) {}
void APIENTRY d3d9mh_glEvalCoord2fv (const GLfloat *u) {}
void APIENTRY d3d9mh_glEvalMesh1 (GLenum mode, GLint i1, GLint i2) {}
void APIENTRY d3d9mh_glEvalMesh2 (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2) {}
void APIENTRY d3d9mh_glEvalPoint1 (GLint i) {}
void APIENTRY d3d9mh_glEvalPoint2 (GLint i, GLint j) {}
void APIENTRY d3d9mh_glFeedbackBuffer (GLsizei size, GLenum type, GLfloat *buffer) {}
void APIENTRY d3d9mh_glFlush (void) {}
GLuint APIENTRY d3d9mh_glGenLists (GLsizei range) {return 0;}
void APIENTRY d3d9mh_glGetBooleanv (GLenum pname, GLboolean *params) {}
void APIENTRY d3d9mh_glGetClipPlane (GLenum plane, GLdouble *equation) {}
void APIENTRY d3d9mh_glGetDoublev (GLenum pname, GLdouble *params) {}
GLenum APIENTRY d3d9mh_glGetError (void) {return GL_NO_ERROR;}
void APIENTRY d3d9mh_glGetLightfv (GLenum light, GLenum pname, GLfloat *params) {}
void APIENTRY d3d9mh_glGetLightiv (GLenum light, GLenum pname, GLint *params) {}
void APIENTRY d3d9mh_glGetMapdv (GLenum target, GLenum query, GLdouble *v) {}
void APIENTRY d3d9mh_glGetMapfv (GLenum target, GLenum query, GLfloat *v) {}
void APIENTRY d3d9mh_glGetMapiv (GLenum target, GLenum query, GLint *v) {}
void APIENTRY d3d9mh_glGetMaterialfv (GLenum face, GLenum pname, GLfloat *params) {}
void APIENTRY d3d9mh_glGetMaterialiv (GLenum face, GLenum pname, GLint *params) {}
void APIENTRY d3d9mh_glGetPixelMapfv (GLenum map, GLfloat *values) {}
void APIENTRY d3d9mh_glGetPixelMapuiv (GLenum map, GLuint *values) {}
void APIENTRY d3d9mh_glGetPixelMapusv (GLenum map, GLushort *values) {}

void APIENTRY d3d9mh_glGetPolygonStipple (GLubyte *mask) {}
void APIENTRY d3d9mh_glGetTexEnvfv (GLenum target, GLenum pname, GLfloat *params) {}
void APIENTRY d3d9mh_glGetTexEnviv (GLenum target, GLenum pname, GLint *params) {}
void APIENTRY d3d9mh_glGetTexGendv (GLenum coord, GLenum pname, GLdouble *params) {}
void APIENTRY d3d9mh_glGetTexGenfv (GLenum coord, GLenum pname, GLfloat *params) {}
void APIENTRY d3d9mh_glGetTexGeniv (GLenum coord, GLenum pname, GLint *params) {}
void APIENTRY d3d9mh_glGetTexLevelParameterfv (GLenum target, GLint level, GLenum pname, GLfloat *params) {}
void APIENTRY d3d9mh_glGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint *params) {}
void APIENTRY d3d9mh_glGetTexParameteriv (GLenum target, GLenum pname, GLint *params) {}
void APIENTRY d3d9mh_glHint (GLenum target, GLenum mode) {}
void APIENTRY d3d9mh_glIndexMask (GLuint mask) {}

void APIENTRY d3d9mh_glIndexd (GLdouble c) {}
void APIENTRY d3d9mh_glIndexdv (const GLdouble *c) {}
void APIENTRY d3d9mh_glIndexf (GLfloat c) {}
void APIENTRY d3d9mh_glIndexfv (const GLfloat *c) {}
void APIENTRY d3d9mh_glIndexi (GLint c) {}
void APIENTRY d3d9mh_glIndexiv (const GLint *c) {}
void APIENTRY d3d9mh_glIndexs (GLshort c) {}
void APIENTRY d3d9mh_glIndexsv (const GLshort *c) {}
void APIENTRY d3d9mh_glIndexub (GLubyte c) {}
void APIENTRY d3d9mh_glIndexubv (const GLubyte *c) {}
void APIENTRY d3d9mh_glInitNames (void) {}

GLboolean APIENTRY d3d9mh_glIsEnabled (GLenum cap) {return GL_FALSE;}
GLboolean APIENTRY d3d9mh_glIsList (GLuint list) {return GL_FALSE;}
GLboolean APIENTRY d3d9mh_glIsTexture (GLuint texture) {return GL_FALSE;}
void APIENTRY d3d9mh_glLightModelf (GLenum pname, GLfloat param) {}
void APIENTRY d3d9mh_glLightModelfv (GLenum pname, const GLfloat *params) {}
void APIENTRY d3d9mh_glLightModeli (GLenum pname, GLint param) {}
void APIENTRY d3d9mh_glLightModeliv (GLenum pname, const GLint *params) {}
void APIENTRY d3d9mh_glLightf (GLenum light, GLenum pname, GLfloat param) {}
void APIENTRY d3d9mh_glLightfv (GLenum light, GLenum pname, const GLfloat *params) {}
void APIENTRY d3d9mh_glLighti (GLenum light, GLenum pname, GLint param) {}
void APIENTRY d3d9mh_glLightiv (GLenum light, GLenum pname, const GLint *params) {}
void APIENTRY d3d9mh_glLineStipple (GLint factor, GLushort pattern) {}
void APIENTRY d3d9mh_glLineWidth (GLfloat width) {}
void APIENTRY d3d9mh_glListBase (GLuint base) {}
void APIENTRY d3d9mh_glLoadMatrixd (const GLdouble *m) {}
void APIENTRY d3d9mh_glLoadName (GLuint name) {}
void APIENTRY d3d9mh_glLogicOp (GLenum opcode) {}
void APIENTRY d3d9mh_glMap1d (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points) {}
void APIENTRY d3d9mh_glMap1f (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points) {}
void APIENTRY d3d9mh_glMap2d (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points) {}
void APIENTRY d3d9mh_glMap2f (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points) {}
void APIENTRY d3d9mh_glMapGrid1d (GLint un, GLdouble u1, GLdouble u2) {}
void APIENTRY d3d9mh_glMapGrid1f (GLint un, GLfloat u1, GLfloat u2) {}
void APIENTRY d3d9mh_glMapGrid2d (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2) {}
void APIENTRY d3d9mh_glMapGrid2f (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2) {}
void APIENTRY d3d9mh_glMaterialf (GLenum face, GLenum pname, GLfloat param) {}
void APIENTRY d3d9mh_glMaterialfv (GLenum face, GLenum pname, const GLfloat *params) {}
void APIENTRY d3d9mh_glMateriali (GLenum face, GLenum pname, GLint param) {}
void APIENTRY d3d9mh_glMaterialiv (GLenum face, GLenum pname, const GLint *params) {}
void APIENTRY d3d9mh_glMultMatrixd (const GLdouble *m) {}
void APIENTRY d3d9mh_glNewList (GLuint list, GLenum mode) {}
void APIENTRY d3d9mh_glNormal3b (GLbyte nx, GLbyte ny, GLbyte nz) {}
void APIENTRY d3d9mh_glNormal3bv (const GLbyte *v) {}
void APIENTRY d3d9mh_glNormal3d (GLdouble nx, GLdouble ny, GLdouble nz) {}
void APIENTRY d3d9mh_glNormal3dv (const GLdouble *v) {}
void APIENTRY d3d9mh_glNormal3f (GLfloat nx, GLfloat ny, GLfloat nz) {}
void APIENTRY d3d9mh_glNormal3fv (const GLfloat *v) {}
void APIENTRY d3d9mh_glNormal3i (GLint nx, GLint ny, GLint nz) {}
void APIENTRY d3d9mh_glNormal3iv (const GLint *v) {}
void APIENTRY d3d9mh_glNormal3s (GLshort nx, GLshort ny, GLshort nz) {}
void APIENTRY d3d9mh_glNormal3sv (const GLshort *v) {}

void APIENTRY d3d9mh_glPassThrough (GLfloat token) {}
void APIENTRY d3d9mh_glPixelMapfv (GLenum map, GLsizei mapsize, const GLfloat *values) {}
void APIENTRY d3d9mh_glPixelMapuiv (GLenum map, GLsizei mapsize, const GLuint *values) {}
void APIENTRY d3d9mh_glPixelMapusv (GLenum map, GLsizei mapsize, const GLushort *values) {}
void APIENTRY d3d9mh_glPixelStoref (GLenum pname, GLfloat param) {}
void APIENTRY d3d9mh_glPixelStorei (GLenum pname, GLint param) {}
void APIENTRY d3d9mh_glPixelTransferf (GLenum pname, GLfloat param) {}
void APIENTRY d3d9mh_glPixelTransferi (GLenum pname, GLint param) {}
void APIENTRY d3d9mh_glPixelZoom (GLfloat xfactor, GLfloat yfactor) {}
void APIENTRY d3d9mh_glPointSize (GLfloat size) {}
void APIENTRY d3d9mh_glPolygonStipple (const GLubyte *mask) {}
void APIENTRY d3d9mh_glPopAttrib (void) {}
void APIENTRY d3d9mh_glPopClientAttrib (void) {}
void APIENTRY d3d9mh_glPopName (void) {}
void APIENTRY d3d9mh_glPrioritizeTextures (GLsizei n, const GLuint *textures, const GLclampf *priorities) {}
void APIENTRY d3d9mh_glPushAttrib (GLbitfield mask) {}
void APIENTRY d3d9mh_glPushClientAttrib (GLbitfield mask) {}
void APIENTRY d3d9mh_glPushName (GLuint name) {}
void APIENTRY d3d9mh_glRasterPos2d (GLdouble x, GLdouble y) {}
void APIENTRY d3d9mh_glRasterPos2dv (const GLdouble *v) {}
void APIENTRY d3d9mh_glRasterPos2f (GLfloat x, GLfloat y) {}
void APIENTRY d3d9mh_glRasterPos2fv (const GLfloat *v) {}
void APIENTRY d3d9mh_glRasterPos2i (GLint x, GLint y) {}
void APIENTRY d3d9mh_glRasterPos2iv (const GLint *v) {}
void APIENTRY d3d9mh_glRasterPos2s (GLshort x, GLshort y) {}
void APIENTRY d3d9mh_glRasterPos2sv (const GLshort *v) {}
void APIENTRY d3d9mh_glRasterPos3d (GLdouble x, GLdouble y, GLdouble z) {}
void APIENTRY d3d9mh_glRasterPos3dv (const GLdouble *v) {}
void APIENTRY d3d9mh_glRasterPos3f (GLfloat x, GLfloat y, GLfloat z) {}
void APIENTRY d3d9mh_glRasterPos3fv (const GLfloat *v) {}
void APIENTRY d3d9mh_glRasterPos3i (GLint x, GLint y, GLint z) {}
void APIENTRY d3d9mh_glRasterPos3iv (const GLint *v) {}
void APIENTRY d3d9mh_glRasterPos3s (GLshort x, GLshort y, GLshort z) {}
void APIENTRY d3d9mh_glRasterPos3sv (const GLshort *v) {}
void APIENTRY d3d9mh_glRasterPos4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w) {}
void APIENTRY d3d9mh_glRasterPos4dv (const GLdouble *v) {}
void APIENTRY d3d9mh_glRasterPos4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w) {}
void APIENTRY d3d9mh_glRasterPos4fv (const GLfloat *v) {}
void APIENTRY d3d9mh_glRasterPos4i (GLint x, GLint y, GLint z, GLint w) {}
void APIENTRY d3d9mh_glRasterPos4iv (const GLint *v) {}
void APIENTRY d3d9mh_glRasterPos4s (GLshort x, GLshort y, GLshort z, GLshort w) {}
void APIENTRY d3d9mh_glRasterPos4sv (const GLshort *v) {}

void APIENTRY d3d9mh_glReadBuffer (GLenum mode)
{
	// d3d doesn't like us messing with the front buffer, which is all glquake uses this for, so here
	// we just silently ignore requests to change the draw buffer
}

void APIENTRY d3d9mh_glRectd (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2) {}
void APIENTRY d3d9mh_glRectdv (const GLdouble *v1, const GLdouble *v2) {}
void APIENTRY d3d9mh_glRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) {}
void APIENTRY d3d9mh_glRectfv (const GLfloat *v1, const GLfloat *v2) {}
void APIENTRY d3d9mh_glRecti (GLint x1, GLint y1, GLint x2, GLint y2) {}
void APIENTRY d3d9mh_glRectiv (const GLint *v1, const GLint *v2) {}
void APIENTRY d3d9mh_glRects (GLshort x1, GLshort y1, GLshort x2, GLshort y2) {}
void APIENTRY d3d9mh_glRectsv (const GLshort *v1, const GLshort *v2) {}
GLint APIENTRY d3d9mh_glRenderMode (GLenum mode) {return 0;}
void APIENTRY d3d9mh_glRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z) {}
void APIENTRY d3d9mh_glScaled (GLdouble x, GLdouble y, GLdouble z) {}
void APIENTRY d3d9mh_glSelectBuffer (GLsizei size, GLuint *buffer) {}
void APIENTRY d3d9mh_glTexCoord1d (GLdouble s) {}
void APIENTRY d3d9mh_glTexCoord1dv (const GLdouble *v) {}
void APIENTRY d3d9mh_glTexCoord1f (GLfloat s) {}
void APIENTRY d3d9mh_glTexCoord1fv (const GLfloat *v) {}
void APIENTRY d3d9mh_glTexCoord1i (GLint s) {}
void APIENTRY d3d9mh_glTexCoord1iv (const GLint *v) {}
void APIENTRY d3d9mh_glTexCoord1s (GLshort s) {}
void APIENTRY d3d9mh_glTexCoord1sv (const GLshort *v) {}
void APIENTRY d3d9mh_glTexCoord2d (GLdouble s, GLdouble t) {}
void APIENTRY d3d9mh_glTexCoord2dv (const GLdouble *v) {}
void APIENTRY d3d9mh_glTexCoord2i (GLint s, GLint t) {}
void APIENTRY d3d9mh_glTexCoord2iv (const GLint *v) {}
void APIENTRY d3d9mh_glTexCoord2s (GLshort s, GLshort t) {}
void APIENTRY d3d9mh_glTexCoord2sv (const GLshort *v) {}
void APIENTRY d3d9mh_glTexCoord3d (GLdouble s, GLdouble t, GLdouble r) {}
void APIENTRY d3d9mh_glTexCoord3dv (const GLdouble *v) {}
void APIENTRY d3d9mh_glTexCoord3f (GLfloat s, GLfloat t, GLfloat r) {}
void APIENTRY d3d9mh_glTexCoord3fv (const GLfloat *v) {}
void APIENTRY d3d9mh_glTexCoord3i (GLint s, GLint t, GLint r) {}
void APIENTRY d3d9mh_glTexCoord3iv (const GLint *v) {}
void APIENTRY d3d9mh_glTexCoord3s (GLshort s, GLshort t, GLshort r) {}
void APIENTRY d3d9mh_glTexCoord3sv (const GLshort *v) {}
void APIENTRY d3d9mh_glTexCoord4d (GLdouble s, GLdouble t, GLdouble r, GLdouble q) {}
void APIENTRY d3d9mh_glTexCoord4dv (const GLdouble *v) {}
void APIENTRY d3d9mh_glTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q) {}
void APIENTRY d3d9mh_glTexCoord4fv (const GLfloat *v) {}
void APIENTRY d3d9mh_glTexCoord4i (GLint s, GLint t, GLint r, GLint q) {}
void APIENTRY d3d9mh_glTexCoord4iv (const GLint *v) {}
void APIENTRY d3d9mh_glTexCoord4s (GLshort s, GLshort t, GLshort r, GLshort q) {}
void APIENTRY d3d9mh_glTexCoord4sv (const GLshort *v) {}

void APIENTRY d3d9mh_glTexEnvfv (GLenum target, GLenum pname, const GLfloat *params) {}
void APIENTRY d3d9mh_glTexEnviv (GLenum target, GLenum pname, const GLint *params) {}
void APIENTRY d3d9mh_glTexGend (GLenum coord, GLenum pname, GLdouble param) {}
void APIENTRY d3d9mh_glTexGendv (GLenum coord, GLenum pname, const GLdouble *params) {}
void APIENTRY d3d9mh_glTexGenf (GLenum coord, GLenum pname, GLfloat param) {}
void APIENTRY d3d9mh_glTexGenfv (GLenum coord, GLenum pname, const GLfloat *params) {}
void APIENTRY d3d9mh_glTexGeni (GLenum coord, GLenum pname, GLint param) {}
void APIENTRY d3d9mh_glTexGeniv (GLenum coord, GLenum pname, const GLint *params) {}
void APIENTRY d3d9mh_glTexImage1D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {}
void APIENTRY d3d9mh_glTexParameterfv (GLenum target, GLenum pname, const GLfloat *params) {}
void APIENTRY d3d9mh_glTexParameteriv (GLenum target, GLenum pname, const GLint *params) {}
void APIENTRY d3d9mh_glTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels) {}
void APIENTRY d3d9mh_glTranslated (GLdouble x, GLdouble y, GLdouble z) {}
void APIENTRY d3d9mh_glVertex2d (GLdouble x, GLdouble y) {}
void APIENTRY d3d9mh_glVertex2dv (const GLdouble *v) {}
void APIENTRY d3d9mh_glVertex2i (GLint x, GLint y) {}
void APIENTRY d3d9mh_glVertex2iv (const GLint *v) {}
void APIENTRY d3d9mh_glVertex2s (GLshort x, GLshort y) {}
void APIENTRY d3d9mh_glVertex2sv (const GLshort *v) {}
void APIENTRY d3d9mh_glVertex3d (GLdouble x, GLdouble y, GLdouble z) {}
void APIENTRY d3d9mh_glVertex3dv (const GLdouble *v) {}
void APIENTRY d3d9mh_glVertex3i (GLint x, GLint y, GLint z) {}
void APIENTRY d3d9mh_glVertex3iv (const GLint *v) {}
void APIENTRY d3d9mh_glVertex3s (GLshort x, GLshort y, GLshort z) {}
void APIENTRY d3d9mh_glVertex3sv (const GLshort *v) {}
void APIENTRY d3d9mh_glVertex4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w) {}
void APIENTRY d3d9mh_glVertex4dv (const GLdouble *v) {}
void APIENTRY d3d9mh_glVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w) {}
void APIENTRY d3d9mh_glVertex4fv (const GLfloat *v) {}
void APIENTRY d3d9mh_glVertex4i (GLint x, GLint y, GLint z, GLint w) {}
void APIENTRY d3d9mh_glVertex4iv (const GLint *v) {}
void APIENTRY d3d9mh_glVertex4s (GLshort x, GLshort y, GLshort z, GLshort w) {}
void APIENTRY d3d9mh_glVertex4sv (const GLshort *v) {}


/*
unimplemented parts of the vertex array interface
*/

void APIENTRY d3d9mh_glEdgeFlagPointer (GLsizei stride, const GLvoid *pointer)
{
	System_Error ("glEdgeFlagPointer : unimplemented");
}

void APIENTRY d3d9mh_glGetPointerv (GLenum pname, GLvoid **params)
{
	System_Error ("glGetPointerv : unimplemented");
}

void APIENTRY d3d9mh_glIndexPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
{
	System_Error ("glIndexPointer : unimplemented");
}

void APIENTRY d3d9mh_glInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer)
{
	System_Error ("glInterleavedArrays : unimplemented");
}

void APIENTRY d3d9mh_glNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
{
	System_Error ("glNormalPointer : unimplemented");
}


#endif // DIRECT3D9_WRAPPER



