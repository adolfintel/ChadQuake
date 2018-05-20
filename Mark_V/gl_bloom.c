#ifdef GLQUAKE // GLQUAKE specific

/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// gl_bloom.c: 2D lighting post process effect

//#include "r_local.h"
#include "quakedef.h"

/* 
============================================================================== 
 
                        LIGHT BLOOMS
 
============================================================================== 
*/ 

//#define TEXPREF_WARPIMAGE 0 // Baker: evil override for the moment

static float Diamond8x[8][8] = 
{ 
        0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 0.2f, 0.3f, 0.3f, 0.2f, 0.0f, 0.0f, 
        0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, 
        0.1f, 0.3f, 0.6f, 0.9f, 0.9f, 0.6f, 0.3f, 0.1f, 
        0.1f, 0.3f, 0.6f, 0.9f, 0.9f, 0.6f, 0.3f, 0.1f, 
        0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, 
        0.0f, 0.0f, 0.2f, 0.3f, 0.3f, 0.2f, 0.0f, 0.0f, 
        0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f 
};

static float Diamond6x[6][6] = 
{ 
        0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, 
        0.0f, 0.3f, 0.5, 0.5, 0.3f, 0.0f, 
        0.1f, 0.5, 0.9f, 0.9f, 0.5, 0.1f, 
        0.1f, 0.5, 0.9f, 0.9f, 0.5, 0.1f, 
        0.0f, 0.3f, 0.5, 0.5, 0.3f, 0.0f, 
        0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f 
};

static float Diamond4x[4][4] = 
{  
        0.3f, 0.4f, 0.4f, 0.3f, 
        0.4f, 0.9f, 0.9f, 0.4f, 
        0.4f, 0.9f, 0.9f, 0.4f, 
        0.3f, 0.4f, 0.4f, 0.3f 
};

static int glt_bloom_effect_size;

static gltexture_t *glt_bloom_screen;
static gltexture_t *glt_bloom_effect;
static gltexture_t *glt_bloom_screen_backup;
static gltexture_t *glt_bloom_screen_downsample;

static int glt_bloom_screen_downsample_size;
static int glt_bloom_screen_downsample_width, glt_bloom_screen_downsample_height;

static int glt_bloom_screen_width_pow2, glt_bloom_screen_height_pow2;

static int glt_bloom_screen_backup_size; // Set to zero to ignore it.
static int glt_bloom_screen_backup_width, glt_bloom_screen_backup_height; 

cbool r_bloom_temp_disable = true;



// current refdef size -- Baker ... 0,0,clwidth,clheight at moment.
static int	curView_x;
static int  curView_y;
static int  curView_width;
static int  curView_height;

// texture coordinates of screen data inside screentexture
static float screenText_tcw;
static float screenText_tch;

static int g_sample_width;
static int g_sample_height;

//texture coordinates of adjusted textures
static float glt_bloom_screen_downsample_width_tcw;
static float glt_bloom_screen_downsample_height_tch;

//this macro is in sample size workspace coordinates
#define sGL_Bloom_SamplePass(xpos, ypos)                            \
    eglBegin(GL_QUADS);                                             \
    eglTexCoord2f	(0, glt_bloom_screen_downsample_height_tch);							\
    eglVertex2f		(xpos, ypos);									\
    eglTexCoord2f	(0, 0);											\
    eglVertex2f		(xpos, ypos + g_sample_height);					\
    eglTexCoord2f	(glt_bloom_screen_downsample_width_tcw, 0);							\
    eglVertex2f		(xpos + g_sample_width, ypos + g_sample_height);\
    eglTexCoord2f	(glt_bloom_screen_downsample_width_tcw, glt_bloom_screen_downsample_height_tch);			\
    eglVertex2f		(xpos + g_sample_width, ypos);					\
    eglEnd();

#define sGL_Bloom_Quad(x, y, width, height, textwidth, textheight)  \
    eglBegin		(GL_QUADS);                                     \
    eglTexCoord2f	(0, textheight);								\
    eglVertex2f		(x, y);											\
    eglTexCoord2f	(0, 0);											\
    eglVertex2f		(x, y + height);								\
    eglTexCoord2f	(textwidth, 0);                                 \
    eglVertex2f		(x+width, y + height);                          \
    eglTexCoord2f	(textwidth, textheight);                        \
    eglVertex2f		(x + width, y);                                 \
    eglEnd			();


#pragma message ("clheight is wrong, it's refdef.height or what not")
///*
//=================
//sGL_Bloom_InitEffectTexture
//=================
//*/
//void sGL_Bloom_InitEffectTexture (void)
//{
//    if (gl_bloom_sample_size.value < 32)
//        Cvar_SetValueQuick (&gl_bloom_sample_size, 32);
//
//    //make sure bloom size is a power of 2
//    glt_bloom_effect_size = Image_Power_Of_Two_Size (gl_bloom_sample_size.value);
//
//    // make sure bloom size doesn't have stupid values
//    if (glt_bloom_effect_size > glt_bloom_screen_width_pow2 || glt_bloom_effect_size > glt_bloom_screen_height_pow2)
//        glt_bloom_effect_size = c_min (glt_bloom_screen_width_pow2, glt_bloom_screen_height_pow2);
//
//	// make the cvar reflect the accepted value
//    if (glt_bloom_effect_size != gl_bloom_sample_size.value)
//        Cvar_SetValueQuick (&gl_bloom_sample_size, glt_bloom_effect_size);
//
//	{ 
//		unsigned *dummy_data = calloc (glt_bloom_effect_size * glt_bloom_effect_size, sizeof(unsigned) /*RGBA_4*/);
//		// Baker: Make sure txgamma doesn't affect this.  Make sure is like warp texture and not reloaded.  But like warp texture, must size adjust.
//		
//		glt_bloom_effect = TexMgr_LoadImage (
//			NULL,							// Model owner
//			-1,								// BSP texture number
//			"glt_bloom_effect",		// Description
//			glt_bloom_effect_size,					// Width
//			glt_bloom_effect_size,					// Height
//			SRC_RGBA,						// Source format src_format_e
//			dummy_data,						// Data
//			"",								// Source filename (qpath)
//			(src_offset_t)dummy_data,		// Offset into file or offset into memory offset_t or uintptr_t
//
//			// Flags ... Recalculate, alpha, nearest, persistent, don't picmip, blended means don't txgamma us
//			TEXPREF_WARPIMAGE | TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_PERSIST | TEXPREF_NOPICMIP | TEXPREF_BLENDED
//		);
//			
//		free (dummy_data);
//		//ORIGINAL: glt_bloom_effect = GL_LoadTexture  ("***glt_bloom_effect***", glt_bloom_effect_size, glt_bloom_effect_size, data, TEX_LUMA, 4);
//		// See also: glt_bloom_effect = LoadATex ( /*rgba store*/ &pt_persist_tex[0], 0, "gfx/particles/particlefont", "qmb:particlefont", true /*force 256 */);    
//	}
//}

/*
=================
sGL_Bloom_InitTextures
=================
*/

/*
=================
R_InitBloomTextures
=================
*/


/*
=================
sGL_Bloom_DrawEffect
=================
*/
static void sGL_BloomBlend_DrawEffect (void)
{
	float bloom_alpha = CLAMP(0, gl_bloom_alpha.value, 1);
    //GL_Bind(0, glt_bloom_effect);
	//glBindTexture(GL_TEXTURE_2D, glt_bloom_effect);
	GL_Bind			(glt_bloom_effect);
    eglEnable		(GL_BLEND);
//  eglBlendFunc	(GL_ONE, GL_ONE);
	eglBlendFunc	(GL_ONE, GL_ONE_MINUS_SRC_COLOR);

    eglColor4f		(bloom_alpha, bloom_alpha, bloom_alpha, 0.8);
    eglTexEnvf		(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    eglBegin		(GL_QUADS);                         

    eglTexCoord2f	(0, glt_bloom_screen_downsample_height_tch);  
    eglVertex2f		(curView_x, curView_y);      
	
    eglTexCoord2f	(0, 0);              
    eglVertex2f		(curView_x, curView_y + curView_height);  

    eglTexCoord2f	(glt_bloom_screen_downsample_width_tcw, 0);
    eglVertex2f		(curView_x + curView_width, curView_y + curView_height);  

    eglTexCoord2f	(glt_bloom_screen_downsample_width_tcw, glt_bloom_screen_downsample_height_tch);  
    eglVertex2f		(curView_x + curView_width, curView_y);

	eglEnd			();
    
    eglDisable(GL_BLEND);
}

/*
=================
sGL_Bloom_GeneratexDiamonds
=================
*/
static void sGL_BloomBlend_GeneratexDiamonds (void)
{
    int         i, j;
    static float intensity;
	extern cvar_t gl_overbright;

    //set up sample size workspace
    eglViewport				(0, 0, g_sample_width, g_sample_height);
    eglMatrixMode			(GL_PROJECTION);
    eglLoadIdentity			();
    eglOrtho				(0, g_sample_width, g_sample_height, 0, -10, 100);
    eglMatrixMode			(GL_MODELVIEW);
    eglLoadIdentity			();

    //copy small scene into glt_bloom_effect
    //GL_Bind(0, glt_bloom_effect);
	GL_Bind					(glt_bloom_effect);
    eglCopyTexSubImage2D	(GL_TEXTURE_2D, 0, 0, 0, 0, 0, g_sample_width, g_sample_height);

    //start modifying the small scene corner
    eglColor4f				(1.0, 1.0, 1.0, 1.0);
    eglEnable				(GL_BLEND);

    //darkening passes
    if (gl_bloom_darken.value)
    {
        eglBlendFunc			(GL_DST_COLOR, GL_ZERO);
        eglTexEnvf				(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        
        for (i = 0; i < gl_bloom_darken.value; i++) {
            sGL_Bloom_SamplePass (0, 0);
        }
        eglCopyTexSubImage2D	(GL_TEXTURE_2D, 0, 0, 0, 0, 0, g_sample_width, g_sample_height);
    }

    //bluring passes
    //glBlendFunc(GL_ONE, GL_ONE);
    eglBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_COLOR);
    
    if (gl_bloom_diamond_size.value > 7 || gl_bloom_diamond_size.value <= 3)
    {
        if (gl_bloom_diamond_size.value != 8) 
			Cvar_SetValueQuick (&gl_bloom_diamond_size, 8);

        for (i = 0; i < gl_bloom_diamond_size.value; i++) 
		{
            for (j = 0; j < gl_bloom_diamond_size.value; j++) 
			{
				
				if (gl_overbright.value /*chase_active.value || cl.viewent_gun.model->name == NULL ... Baker <--- seriously?*/)
					intensity = gl_bloom_intensity.value * 0.1 * Diamond8x[i][j];
				else
					intensity = gl_bloom_intensity.value * 0.3 * Diamond8x[i][j];

                if (intensity < 0.01) continue;
                eglColor4f (intensity, intensity, intensity, 1.0);
                sGL_Bloom_SamplePass (i - 4, j - 4);
            }
        }
    } 
	else 
	if (gl_bloom_diamond_size.value > 5) 
	{
        
        if (gl_bloom_diamond_size.value != 6)
			Cvar_SetValueQuick (&gl_bloom_diamond_size, 6);

        for (i = 0; i < gl_bloom_diamond_size.value; i++) 
		{
            for(j = 0; j < gl_bloom_diamond_size.value; j++) 
			{
				if (chase_active.value || gl_overbright.value || cl.viewent_gun.model->name == NULL)
					intensity = gl_bloom_intensity.value * 0.1 * Diamond6x[i][j];
				else
					intensity = gl_bloom_intensity.value * 0.5 * Diamond6x[i][j];
                
				if (intensity < 0.01) 
					continue;

                eglColor4f (intensity, intensity, intensity, 1.0);
                sGL_Bloom_SamplePass (i - 3, j - 3);
            }
        }
    } 
	else 
	if (gl_bloom_diamond_size.value > 3) 
	{
		if (gl_bloom_diamond_size.value != 4) 
			Cvar_SetValueQuick (&gl_bloom_diamond_size, 4);

        for (i = 0; i < gl_bloom_diamond_size.value; i++) {
            for (j = 0; j < gl_bloom_diamond_size.value; j++) {
				
				if (chase_active.value || gl_overbright.value || cl.viewent_gun.model->name == NULL)
					intensity = gl_bloom_intensity.value * 0.1 * Diamond4x[i][j];
				else
					intensity = gl_bloom_intensity.value * 0.8 * Diamond4x[i][j];

                if (intensity < 0.01) 
					continue;
                eglColor4f		(intensity, intensity, intensity, 1.0);
                sGL_Bloom_SamplePass (i - 2, j - 2);
            }
        }
    }
    
    eglCopyTexSubImage2D		(GL_TEXTURE_2D, 0, 0, 0, 0, 0, g_sample_width, g_sample_height);

    //restore full screen workspace
    eglViewport					(0, 0, clwidth, clheight);
    eglMatrixMode				(GL_PROJECTION);
    eglLoadIdentity				();
    eglOrtho					(0, clwidth, clheight, 0, -10, 100); // Baker: Uh, your fucking kidding me right?
    eglMatrixMode				(GL_MODELVIEW);
    eglLoadIdentity				();
}                                           

/*
=================
sGL_Bloom_DownsampleView
=================
*/
void sGL_BloomBlend_DownsampleView (void)
{
    eglDisable (GL_BLEND);
    eglColor4f (1.0, 1.0, 1.0, 1.0);

    // stepped downsample
    if (glt_bloom_screen_downsample_size)
    {
        int midsample_width		= glt_bloom_screen_downsample_size * glt_bloom_screen_downsample_width_tcw;
        int midsample_height	= glt_bloom_screen_downsample_size * glt_bloom_screen_downsample_height_tch;
        
        //copy the screen and draw resized
		GL_Bind					(glt_bloom_screen);
        eglCopyTexSubImage2D	(GL_TEXTURE_2D, 0, 0, 0, curView_x, clheight - (curView_y + curView_height), curView_width, curView_height);
        sGL_Bloom_Quad			(0, clheight - midsample_height, midsample_width, midsample_height, screenText_tcw, screenText_tch );
        
        //now copy into Downsampling (mid-sized) texture
        //GL_Bind(0, r_bloomdownsamplingtexture);
		GL_Bind					(glt_bloom_screen_downsample);
        eglCopyTexSubImage2D	(GL_TEXTURE_2D, 0, 0, 0, 0, 0, midsample_width, midsample_height);

        //now draw again in bloom size
        eglColor4f				(0.5, 0.5, 0.5, 1.0);
        sGL_Bloom_Quad			(0, clheight - g_sample_height, g_sample_width, g_sample_height, glt_bloom_screen_downsample_width_tcw, glt_bloom_screen_downsample_height_tch);
        
        //now blend the big screen texture into the bloom generation space (hoping it adds some blur)
        eglEnable				(GL_BLEND);
        eglBlendFunc			(GL_ONE, GL_ONE);
        eglColor4f				(0.5, 0.5, 0.5, 1.0);
        
		GL_Bind					(glt_bloom_screen);
        sGL_Bloom_Quad			(0, clheight - g_sample_height, g_sample_width, g_sample_height, screenText_tcw, screenText_tch);
        eglColor4f				(1.0, 1.0, 1.0, 1.0);
        eglDisable				(GL_BLEND);

    } else {    
		// downsample simple
		GL_Bind					(glt_bloom_screen);
        eglCopyTexSubImage2D	(GL_TEXTURE_2D, 0 /*level*/, 0 /* xoffset within texture */, 0 /*y offset within texture*/, curView_x, clheight - (curView_y + curView_height), curView_width, curView_height);
        sGL_Bloom_Quad			(0, clheight - g_sample_height, g_sample_width, g_sample_height, screenText_tcw, screenText_tch);
    }
}

/*
=================
GL_BloomBlend
=================
*/
void GL_BloomBlend (void)
{
    if (!gl_bloom.value || vid.direct3d /* == 8 even DX9 at the moment... likely my fault somehow */) // DX8 - doesn't have CopyTexSubImage
        return;

	if (r_bloom_temp_disable)
		return;

    if (!glt_bloom_effect_size)
		System_Error ("Bloom_InitTextures never called, glt_bloom_effect_size is zero");

	if (glt_bloom_screen_width_pow2 < glt_bloom_effect_size || glt_bloom_screen_height_pow2 < glt_bloom_effect_size) {
		logd ("Screen size %d %d smaller than bloom size %d, %d.", glt_bloom_screen_width_pow2, glt_bloom_screen_height_pow2, glt_bloom_effect_size, glt_bloom_effect_size);
        return; // Baker: Isn't this an error?
	}

    // set up full screen workspace
    eglViewport			(0, 0, clwidth, clheight);
    eglMatrixMode		(GL_PROJECTION);
    eglLoadIdentity		();
    eglOrtho			(0, clwidth, clheight, 0, -10, 100); // left right bottom top near far
    eglMatrixMode		(GL_MODELVIEW);
    eglLoadIdentity		();

	eglDisable			(GL_DEPTH_TEST);
    eglDisable			(GL_CULL_FACE);
    eglDisable			(GL_BLEND);
    eglEnable			(GL_TEXTURE_2D);
    eglColor4f			(1, 1, 1, 1);

    // set up current sizes.  Baker: This should be r_refdef.x, y, width, height but fuck it because 
	// I'm not impressed with the performance at all.
	curView_x = 0;
	curView_y = 0;
    curView_width = clwidth;
    curView_height = clheight;

	// Calc texture coords
    screenText_tcw = (float)clwidth / glt_bloom_screen_width_pow2;
    screenText_tch = (float)clheight / glt_bloom_screen_height_pow2;

    if (clheight > clwidth) {
        glt_bloom_screen_downsample_width_tcw = (float)clwidth / (float)clheight;
		glt_bloom_screen_downsample_height_tch = 1.0;
    } 
	else {
        glt_bloom_screen_downsample_width_tcw = 1.0;
        glt_bloom_screen_downsample_height_tch = (float)clheight / (float)clwidth;
    }

	// Sample size
    g_sample_width  = glt_bloom_effect_size * glt_bloom_screen_downsample_width_tcw;
    g_sample_height = glt_bloom_effect_size * glt_bloom_screen_downsample_height_tch;
    
    // copy the screen space we'll use to work into the backup texture
	GL_Bind						(glt_bloom_screen_backup);
    eglCopyTexSubImage2D		(GL_TEXTURE_2D, 0 /*level*/, 0, 0 /* <-- offset x, y*/, 0, 0, glt_bloom_screen_backup_width, glt_bloom_screen_backup_height);  
	
    // create the bloom image
	sGL_BloomBlend_DownsampleView ();
    sGL_BloomBlend_GeneratexDiamonds ();

    //restore the screen-backup to the screen
    eglDisable					(GL_BLEND);
	GL_Bind						(glt_bloom_screen_backup);
    eglColor4f					(1, 1, 1, 1);
    sGL_Bloom_Quad				(0, clheight - glt_bloom_screen_backup_height, glt_bloom_screen_backup_width, glt_bloom_screen_backup_height, 1.0, 1.0);
	sGL_BloomBlend_DrawEffect	();
	eglColor3f					(1, 1, 1);   
    eglDisable					(GL_BLEND); 
    eglEnable					(GL_TEXTURE_2D);   
	eglEnable					(GL_DEPTH_TEST);
    eglBlendFunc				(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);   
	eglViewport					(clx, cly, clwidth, clheight);
}

#define TEX_REUPLOAD(_myglt, _fillbyte, _mywidth, _myheight)													\
	{																											\
		size_t bytes_size = (_mywidth) * (_myheight) * sizeof(unsigned) /* RGBA_4*/;							\
		unsigned *pels_rgba_alloced = calloc(bytes_size, 1);													\
		if ( (_fillbyte) ) memset (pels_rgba_alloced, (_fillbyte), bytes_size);									\
																												\
		GL_Bind ( (_myglt) );																					\
		eglTexImage2D (GL_TEXTURE_2D, 0 /*miplevel*/, GL_RGBA /*gl_alpha_format*/, (_mywidth), (_myheight),		\
			0 /*border*/, GL_RGBA, GL_UNSIGNED_BYTE, pels_rgba_alloced);										\
		(_myglt)->width = (_mywidth), (_myglt)->height = (_myheight);											\
		free (pels_rgba_alloced);																				\
	} /* end of macro */

void GL_Bloom_RecalcImageSize (void)
{
	if  (vid.direct3d /* == 8 even DX9 at the moment... likely my fault somehow */)
		return;  // No CopyTexImage

    // find screen size power of 2 size
    glt_bloom_screen_width_pow2   = Image_Power_Of_Two_Size (clwidth);
	glt_bloom_screen_height_pow2  = Image_Power_Of_Two_Size (clheight);

    // disable blooms if we can't handle a texture of that size
	if (glt_bloom_screen_width_pow2 > renderer.gl_max_texture_size || glt_bloom_screen_height_pow2 > renderer.gl_max_texture_size) {
		// This theoretical situation is nearly impossible.
		r_bloom_temp_disable = true;
		logd ("Required power of 2 size too large.  Fix me.  Or disable the feature or something.");
		return;
	} else r_bloom_temp_disable = false;

	// BLOOM SCREEN TEXTURE - (white filled, pow2 size of screen)
	TEX_REUPLOAD (glt_bloom_screen, 255 /*white*/, glt_bloom_screen_width_pow2, glt_bloom_screen_height_pow2) // No closure
		
	// BLOOM EFFECT TEXTURE
	if (gl_bloom_sample_size.value < 32)  Cvar_SetValueQuick (&gl_bloom_sample_size, 32); // If cvar set below 32, fix it.
    glt_bloom_effect_size = Image_Power_Of_Two_Size (gl_bloom_sample_size.value); // POW2 size if necessary.

    if (glt_bloom_effect_size > glt_bloom_screen_width_pow2)  glt_bloom_effect_size = glt_bloom_screen_width_pow2;  // Shrink to fit screen width pow2 if needed
	if (glt_bloom_effect_size > glt_bloom_screen_height_pow2) glt_bloom_effect_size = glt_bloom_screen_height_pow2; // Shrink to fit screen height pow2 if needed

	TEX_REUPLOAD (glt_bloom_effect, 0 /*white*/, glt_bloom_effect_size, glt_bloom_effect_size) // No closure

	// BLOOM SCREEN DOWNSAMPLE TEXTURE
	// - if screensize is more than 2x the bloom effect texture, set up for stepped downsampling if gl_bloom_fast_sample is 0.
    
	if (gl_bloom_fast_sample.value) {
		if (clwidth > glt_bloom_effect_size * 2 || clheight > glt_bloom_effect_size * 2) {
			glt_bloom_screen_downsample_size = glt_bloom_screen_downsample_width = glt_bloom_screen_downsample_height = glt_bloom_effect_size * 2;
			TEX_REUPLOAD (glt_bloom_screen_downsample, 0 /*white*/, glt_bloom_screen_downsample_width, glt_bloom_screen_downsample_height) // No closure
		}
	} else glt_bloom_screen_downsample_size = 0;

    // BLOOM SCREEN BACKUP TEXTURE - Init the screen backup texture
	glt_bloom_screen_backup_size		= glt_bloom_screen_downsample_size ? glt_bloom_screen_downsample_size : glt_bloom_effect_size;
	glt_bloom_screen_backup_width		= glt_bloom_screen_backup_size;
    glt_bloom_screen_backup_height		= glt_bloom_screen_backup_size;

	TEX_REUPLOAD (glt_bloom_screen_backup, 0 /*white*/, glt_bloom_screen_backup_width, glt_bloom_screen_backup_height) // No closure
}


#define TEXUPLOADFAKE(gltname, iwidth, iheight, pdata)																	\
	gltname = TexMgr_LoadImage (																									\
		NULL,					/* Model owner					*/														\
		-1,						/* BSP texture number			*/														\
		STRINGIFY(gltname),		/* Description					*/														\
		iwidth,					/* Width						*/														\
		iheight,				/* Height						*/														\
		SRC_RGBA,				/* Source format src_format_e	*/														\
		pdata,					/* Data							*/														\
		"",						/* Source filename (qpath)		*/														\
		(src_offset_t)pdata,	/* Offset into file or offset into memory offset_t or uintptr_t */						\
																														\
		/* Flags ... Recalculate, alpha, nearest, persistent, don't picmip, blended means don't txgamma us */			\
		TEXPREF_BLOOMSCREEN | TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_PERSIST | TEXPREF_NOPICMIP | TEXPREF_BLENDED	\
	);

void GL_Bloom_Init_Once (void)
{
	#define dummy_width_32		32
	#define dummy_height_32		32
	byte dummy_texture[dummy_width_32 * dummy_height_32 * RGBA_4]; // Don't think this even needs to be static.

	if (vid.direct3d /* == 8 even DX9 at the moment... likely my fault somehow */)
		return;  // No CopyTexImage

	// Baker:  Create all 4 textures with inappropriate sizes
	TEXUPLOADFAKE (glt_bloom_screen, dummy_width_32, dummy_height_32, dummy_texture); // White filled
	TEXUPLOADFAKE (glt_bloom_effect, dummy_width_32, dummy_height_32, dummy_texture); // Black filled
	TEXUPLOADFAKE (glt_bloom_screen_downsample, dummy_width_32, dummy_height_32, dummy_texture); // Black
	TEXUPLOADFAKE (glt_bloom_screen_backup, dummy_width_32, dummy_height_32, dummy_texture); // Black

	GL_Bloom_RecalcImageSize ();
}


#endif // GLQUAKE specific