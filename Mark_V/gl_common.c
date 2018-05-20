#ifdef CORE_GL // GLQUAKE and WinQuake via GL need this

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
// gl_common.c -- server code for moving users




#include "quakedef.h"

renderer_t renderer;

/*
===============
GL_MakeNiceExtensionsList -- johnfitz
===============
*/
static char *GL_MakeNiceExtensionsList (const char *in)
{
	char *copy, *token, *out;
	int i, count;
	size_t bufsize;

	if (!in) return Z_Strdup("(none)");

	//each space will be replaced by 4 chars, so count the spaces before we malloc
	for (i = 0, count = 1; i < (int)strlen(in); i++)
	{
		if (in[i] == ' ')
			count++;
	}

	bufsize = strlen(in) + count*3 + 1; //usually about 1-2k
	out = (char *) Z_Malloc (bufsize /*strlen(in) + count*3 + 1*/); //usually about 1-2k
	out[0] = 0;

	copy = (char *) Z_Strdup(in);
	for (token = strtok(copy, " "); token; token = strtok(NULL, " "))
	{
		strlcat (out, "\n   ", bufsize);
		strlcat (out, token, bufsize);
	}

	Z_Free (copy);
	return out;
}


/*
===============
GL_Info_f -- johnfitz
===============
*/
void GL_Info_f (lparse_t *unused)
{
	Con_SafePrintLinef ("GL_VENDOR: %s", renderer.gl_vendor);
	Con_SafePrintLinef ("GL_RENDERER: %s", renderer.gl_renderer);
	Con_SafePrintLinef ("GL_VERSION: %s", renderer.gl_version);
	Con_SafePrintLinef ("GL_EXTENSIONS: %s", renderer.gl_extensions_nice);
}

/*
===============
GL_CheckExtensions
===============
*/
// Exclusively called by GL_Evaluate_Renderer, which is exclusively called by VID_SetMode
static cbool GL_ParseExtensionList (const char *list, const char *name)
{
	const char	*start;
	const char	*where, *terminator;

	if (!list || !name || !*name)
		return false;
	if (strchr(name, ' ') != NULL)
		return false;	// extension names must not have spaces

	start = list;
	while (1) {
		where = strstr (start, name);
		if (!where)
			break;
		terminator = where + strlen (name);
		if (where == start || where[-1] == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return true;
		start = terminator;
	}
	return false;
}
// Exclusively called by GL_Evaluate_Renderer, which is exclusively called by VID_SetMode
static void GL_Evaluate_Renderer_CheckExtensions (void)
{
	int		(*printline_fn) (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2))) = Con_DPrintLinef; // Oct 2016 - formerly Con_Printf
	int		(*warnline_fn) (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2))) = Con_WarningLinef;

	if (host_initialized)
	{
		printline_fn = Con_DPrintLinef;
		warnline_fn = Con_DPrintLinef;
	}

	// poll max size from hardware.  Baker: Note that if this could actually change here, we would need to remesh glmodels because texture coords would be wrong
	eglGetIntegerv (GL_MAX_TEXTURE_SIZE, &renderer.gl_max_texture_size);

	//
	// multitexture
	//
	if (COM_CheckParm("-nomtex"))
		warnline_fn ("Multitexture disabled at command line");
	else if (GL_ParseExtensionList(renderer.gl_extensions, "GL_ARB_multitexture"))
	{
		renderer.GL_MTexCoord2fFunc = (PFNGLMULTITEXCOORD2FARBPROC) Vid_GL_GetProcAddress("glMultiTexCoord2fARB");
		renderer.GL_SelectTextureFunc = (PFNGLACTIVETEXTUREARBPROC) Vid_GL_GetProcAddress("glActiveTextureARB");
		if (renderer.GL_MTexCoord2fFunc && renderer.GL_SelectTextureFunc)
		{
			printline_fn ("FOUND: ARB_multitexture");
			renderer.TEXTURE0 = GL_TEXTURE0;
			renderer.TEXTURE1 = GL_TEXTURE1;
			renderer.gl_mtexable = true;
		}
		else warnline_fn ("Couldn't link to multitexture functions");
	}
	else warnline_fn ("multitexture not supported (extension not found)");

	//
	// texture_env_combine
	//
	if (COM_CheckParm("-nocombine"))
		warnline_fn ("texture_env_combine disabled at command line");
	else if (GL_ParseExtensionList(renderer.gl_extensions, "GL_ARB_texture_env_combine"))
	{
		printline_fn ("FOUND: ARB_texture_env_combine");
		renderer.gl_texture_env_combine = true;
	}
	else if (GL_ParseExtensionList(renderer.gl_extensions, "GL_EXT_texture_env_combine"))
	{
		printline_fn ("FOUND: EXT_texture_env_combine");
		renderer.gl_texture_env_combine = true;
	}
	else warnline_fn ("texture_env_combine not supported");

	//
	// texture_env_add
	//
	if (COM_CheckParm("-noadd"))
		warnline_fn ("texture_env_add disabled at command line");
	else if (GL_ParseExtensionList(renderer.gl_extensions, "GL_ARB_texture_env_add"))
	{
		printline_fn ("FOUND: ARB_texture_env_add");
		renderer.gl_texture_env_add = true;
	}
	else if (GL_ParseExtensionList(renderer.gl_extensions, "GL_EXT_texture_env_add"))
	{
		printline_fn ("FOUND: EXT_texture_env_add");
		renderer.gl_texture_env_add = true;
	}
	else warnline_fn ("texture_env_add not supported");

	//
	// gl_texture_non_power_of_two
	//
	if (COM_CheckParm("-nonpot"))
		warnline_fn ("texture_non_power_of_two disabled at command line");
	else if (GL_ParseExtensionList(renderer.gl_extensions, "GL_ARB_texture_non_power_of_two"))
	{
		printline_fn ("FOUND: GL_ARB_texture_non_power_of_two");
		renderer.gl_texture_non_power_of_two = true;
	}
	else warnline_fn ("texture_non_power_of_two not supported");

#if 0 // Nice try
	// 8-Bit Palette WinQuake GL
	if (COM_CheckParm("-no8bit"))
		warnline_fn ("8-Bit Palette Disabled via command line");
	else //if (GL_ParseExtensionList(renderer.gl_extensions, "GL_ARB_multitexture"))
	{
		renderer.GLColorTableEXT = (qGLColorTableEXT) Vid_GL_GetProcAddress("GLColorTableEXT");
		if (renderer.GLColorTableEXT) {
			printline_fn ("FOUND: GLColorTableEXT");
		}
		else warnline_fn ("GLColorTableEXT not available");

		renderer.GLSetPaletteEXT = (qgl3DfxSetPaletteEXT) Vid_GL_GetProcAddress("GLSetPaletteEXT");
		if (renderer.GLSetPaletteEXT) {
			printline_fn ("FOUND: GLSetPaletteEXT");
		}
		else warnline_fn ("GLSetPaletteEXT not available");
	}
#endif	

	//
	// anisotropic filtering
	//
	if (GL_ParseExtensionList(renderer.gl_extensions, "GL_EXT_texture_filter_anisotropic"))
	{
		float test1,test2;
		GLuint tex;

		// test to make sure we really have control over it
		// 1.0 and 2.0 should always be legal values
		eglGenTextures(1, &tex);
		eglBindTexture (GL_TEXTURE_2D, tex);
		eglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
		eglGetTexParameterfv (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, &test1);
		eglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2.0f);
		eglGetTexParameterfv (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, &test2);
		eglDeleteTextures(1, &tex);

		if (test1 == 1 && test2 == 2)
		{
			printline_fn ("FOUND: EXT_texture_filter_anisotropic");
			renderer.gl_anisotropy_able = true;
		}
		else warnline_fn ("anisotropic filtering locked by driver. Current driver setting is %f", test1);

		//get max value either way, so the menu and stuff know it
		eglGetFloatv (GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &renderer.gl_max_anisotropy);

	} else warnline_fn ("texture_filter_anisotropic not supported");

	//
	// swap control
	//

#ifdef PLATFORM_OSX
	renderer.gl_swap_control = true; // This should be true even if using SDL
#else
	renderer.gl_swap_control = VID_Local_Vsync_Init (renderer.gl_extensions);
	if (renderer.gl_swap_control)
		printline_fn  ("Swap control enabled");
	else warnline_fn ("Swap control not available");
#endif // PLATFORM_OSX

}

/*
===============
GL_SetupState -- johnfitz

does all the stuff from GL_Init that needs to be done every time a new GL render context is created
GL_Init will still do the stuff that only needs to be done once
===============
*/
// Exclusively called by VID_SetMode

void GL_VID_SetMode_GL_SetupState (void)
{
	eglClearColor (0.15, 0.15, 0.15, 0); //johnfitz -- originally 1,0,0,0
	eglCullFace(GL_BACK); //johnfitz -- glquake used CCW with backwards culling -- let's do it right
	eglFrontFace(GL_CW); //johnfitz -- glquake used CCW with backwards culling -- let's do it right
	eglEnable(GL_TEXTURE_2D);
	eglEnable(GL_ALPHA_TEST);
	eglAlphaFunc(GL_GREATER, 0.666);
	eglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	eglShadeModel (GL_FLAT);
	eglHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); //johnfitz
	eglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	eglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	//spike -- these are invalid as there is no texture bound to receive this state.
//	eglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//	eglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//	eglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//	eglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

// Baker: OLD: eglDepthRange (0, 1);
// Baker: NEW: eglDepthRange (0, 0.5); // So mirror can use other depth range
//johnfitz -- moved here becuase gl_ztrick is gone.
#ifdef GLQUAKE
	eglDepthRange (Q_GLDEPTHRANGE_MIN_0_0, Q_GLDEPTHRANGE_MAX_0_5); // Baker: Half the possible Z range is still plenty. Mirror support.
#endif // GLQUAKE
	eglDepthFunc (GL_LEQUAL); //johnfitz -- moved here becuase gl_ztrick is gone.
	if (renderer.gl_stencilbits) {
		eglClearStencil (0); // Stencil Sky Fix - 2015 April 13 Per Spike
	}

///////////////////////////////////////////////////////////////////////////////
// Baker: In the hope it will make pre-emptively make npot play nice with strange
// widths with "imagedump" which could occur with a replacement texture made by
// someone unknowledgable.
//
// This is in addition to the benefits of screenshots and video capture.
// Note: Direct3D wrapper does not support glPixelStorei, but doesn't support npot
// either so this will not affect "imagedump" because textures will be pow2 upscaled
// (hence, no alignment issues).
//
// Khronos: glPixelStorei
// "... parameter affects how pixel data is returned to client memory:"
// glReadPixels, glTexImage2D, glTexSubImage2D
//
///////////////////////////////////////////////////////////////////////////////

	eglPixelStorei (GL_PACK_ALIGNMENT, 1);
}

/*
===============
GL_Evaluate_Renderer
===============
*/
// Called exclusively by VID_SetMode and yes we call it every time, even though it is doubtful change would occur.
// If a change did occur (which barring multi-monitor or the crazy NVIDIA dual driver), I'm not sure we could deal with it.
void GL_VID_SetMode_GL_Evaluate_Renderer (void)
{
	static cbool first_init_complete = false;
	int		(*safeprintlinef_fn) (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));
	int		(*warnline_fn) (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));
	safeprintlinef_fn = first_init_complete ? Con_DPrintLinef : Con_SafePrintLinef;
	warnline_fn = first_init_complete ? Con_DPrintLinef : Con_WarningLinef;
	

	// Free extensions string before wiping the struct
	if (renderer.gl_extensions_nice != NULL)
		Z_Free (renderer.gl_extensions_nice);

	// Wipe the struct
	memset (&renderer, 0, sizeof(renderer));

	renderer.gl_vendor = (const char *) eglGetString (GL_VENDOR);
	renderer.gl_renderer = (const char *) eglGetString (GL_RENDERER);
	renderer.gl_version = (const char *) eglGetString (GL_VERSION);
	renderer.gl_extensions = (const char *) eglGetString (GL_EXTENSIONS);

	renderer.gl_extensions_nice = GL_MakeNiceExtensionsList (renderer.gl_extensions);

	if (!first_init_complete) {
		Con_DebugLogLine ("GL_VENDOR: %s", renderer.gl_vendor);
		Con_DebugLogLine ("GL_RENDERER: %s", renderer.gl_renderer);
		Con_DebugLogLine ("GL_VERSION: %s", renderer.gl_version);
		Con_DebugLogLine ("GL_EXTENSIONS: %s", renderer.gl_extensions_nice);
	}

	GL_Evaluate_Renderer_CheckExtensions (); //johnfitz

	//johnfitz -- intel video workarounds from Baker
	//if (!strcmp(renderer.gl_vendor, "Intel"))
	if (strstr(renderer.gl_vendor, "Intel")) // MH says
	{
		safeprintlinef_fn ("Intel Display Adapter detected");
		renderer.isIntelVideo = true;
	}
	//johnfitz

	//johnfitz -- confirm presence of stencil buffer
	//if (vid.direct3d)
#if !defined(GLQUAKE) && defined(CORE_GL) // skip for WinQuake GL
	// Skip for WinQuake GL
#else
	if (vid.direct3d == 8)
		Con_DPrintLinef ("Stencil not supported by Direct3D wrapper at this time");
	else if (COM_CheckParm("-nostencil"))
		warnline_fn ("Stencil disabled at command line"); // Stencil will be 0.
	else {
		eglGetIntegerv(GL_STENCIL_BITS, &renderer.gl_stencilbits);
		if(!renderer.gl_stencilbits)
			warnline_fn ("Could not create stencil buffer");
		else Con_DPrintLinef ("%d bit stencil buffer", renderer.gl_stencilbits);
	}
#endif // Skip for WinQuake GL

	// We don't have multisample for Direct3D
	if (!vid.direct3d && COM_CheckParm ("-nomultisample")) {
		warnline_fn ("Multisample disabled at command line"); // This is WAY TOO early.
//		return;  
	}

	if (!first_init_complete) {
		Con_DebugLogLine ("Max texture size: %d", renderer.gl_max_texture_size);
		Con_DebugLogLine ("Multitexture:     %d", renderer.gl_mtexable);
		Con_DebugLogLine ("Non-Power of 2:   %d", renderer.gl_texture_non_power_of_two);
		Con_DebugLogLine ("Combine:			 %d", renderer.gl_texture_env_combine);
		Con_DebugLogLine ("Stencil Bits:	 %d", renderer.gl_stencilbits);
	}
	first_init_complete = true;
}

#endif // CORE_GL