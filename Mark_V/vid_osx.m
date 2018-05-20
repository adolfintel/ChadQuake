/*
Copyright (C) 2001-2012 Axel 'awe' Wefers (Fruitz Of Dojo)
Copyright (C) 2009-2014 Baker

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
// vid.c -- common video

#import <AppKit/AppKit.h>
#import <OpenGL/gl.h>

#import <FDFramework/FDFramework.h>

#import "quakedef.h"
#import "macquake.h"


void VID_Local_Window_PreSetup (void)
{
    sysplat.gVidDisplay = [FDDisplay mainDisplay];

    if (!sysplat.gVidDisplay)
        System_Error ("No valid display found!");    
}


vmode_t VID_Local_GetDesktopProperties (void)
{
    vmode_t desktop = {0};

    desktop.type        =   MODE_FULLSCREEN;
    desktop.width       =   (int)NSWidth    ([[NSScreen mainScreen] frame]);
    desktop.height      =   (int)NSHeight   ([[NSScreen mainScreen] frame]);
    desktop.bpp         =   (int)NSBitsPerPixelFromDepth( [[NSScreen mainScreen] depth] );

    return desktop;
}





//
// vid modes
//


void VID_Local_AddFullscreenModes (void)
{
    NSArray*        displayModes    = [sysplat.gVidDisplay displayModes];
    
    if (displayModes == nil)
        System_Error ("Unable to get list of available display modes.");
    
    for (FDDisplayMode* displayMode in [sysplat.gVidDisplay displayModes])
    {
        if ([displayMode bitsPerPixel] != BPP_32)
         continue;

        vmode_t test = { MODE_FULLSCREEN, NULL, (int)[displayMode width], (int)[displayMode height], BPP_32};
    
        cbool bpp_ok     = true;
        cbool width_ok   = in_range (MIN_MODE_WIDTH_640, (int)[displayMode width], MAX_MODE_WIDTH_10000);
        cbool height_ok  = in_range (MIN_MODE_HEIGHT_400, (int)[displayMode height], MAX_MODE_HEIGHT_10000);
        cbool qualified  = (bpp_ok && width_ok && height_ok);

        if (qualified && !VID_Mode_Exists(&test, NULL) )
        {
            test.ptr = (void *)[displayMode retain];
            // Not a dup and test = ok ---> add it
            memcpy (&vid.modelist[vid.nummodes++], &test, sizeof(vmode_t) );
#if 0
            Con_SafePrintLinef ("Added %d: %d x %d %d", vid.nummodes -1, vid.modelist[vid.nummodes-1].width, vid.modelist[vid.nummodes-1].height, vid.modelist[vid.nummodes-1].bpp);
#endif
        }
    }
}

#ifdef WINQUAKE_RENDERER_SUPPORT
// Exclusively called by sVID_Local_SetMode_GetBuffers
static void sVID_GetBuffers_InitializeTexture (int newwidth, int newheight)
{
    if (vid.texture_initialized)
        return;
    

    vid.texture_actual_width    = Image_Power_Of_Two_Size (newwidth);
    vid.texture_actual_height   = Image_Power_Of_Two_Size (newheight);
    GLint           actualTexWidth  = -1;
    GLint           actualTexHeight = -1;
    GLenum          error           = 0;
    
    [[sysplat.gVidWindow openGLContext] makeCurrentContext];
    
    glGenTextures (1, &vid.texture);
    glBindTexture (GL_TEXTURE_2D, vid.texture);
    glEnable (GL_TEXTURE_2D);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    glGetError();
    glTexImage2D (GL_PROXY_TEXTURE_2D, 0, GL_RGBA, vid.texture_actual_width, vid.texture_actual_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    
    error = glGetError();
    
    glGetTexLevelParameteriv (GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &actualTexWidth);
    glGetTexLevelParameteriv (GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &actualTexHeight);
    
    // now let's see if the width is equal to our requested value:
    if (error != GL_NO_ERROR || vid.texture_actual_width != actualTexWidth || vid.texture_actual_height != actualTexHeight)
        System_Error ("Out of video RAM. Please try a lower resolution and/or depth!");
    
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, vid.texture_actual_width, vid.texture_actual_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, vid.bitmap);
    vid.texture_s1 = newwidth /*vid.texture_width*/ / (float)vid.texture_actual_width;
    vid.texture_t1 = newheight /*texture_height*/ / (float)vid.texture_actual_height;

    vid.texture_initialized = true;
}


static void sVID_FlushBuffers_ShutdownTexture (void)
{
    if (vid.texture_initialized)
    {
        [[sysplat.gVidWindow openGLContext] makeCurrentContext];
        
        glDeleteTextures (1, &vid.texture);
        vid.texture_initialized = false;
    }
}


// Baker: Done by resize handler and VID_Update
void VID_RenderTexture (void)
{
    if (!vid.bitmap)
        return;
    
    const NSRect    contentRect = [[sysplat.gVidWindow contentView] frame];
    
    [[sysplat.gVidWindow openGLContext] makeCurrentContext];

    glViewport (0, 0, (GLsizei) NSWidth (contentRect), (GLsizei) NSHeight (contentRect));
    
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glOrtho (0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glClearColor( 0, 0, 0, 0 );
    glClear( GL_COLOR_BUFFER_BIT );
    
    glBindTexture (GL_TEXTURE_2D, vid.texture);
    
    glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, vid.screen.width /*vid.texture_width*/, vid.screen.height /*vid.texture_height*/, GL_RGBA, GL_UNSIGNED_BYTE, vid.bitmap);
    glEnable (GL_TEXTURE_2D);
    
    glColor3f( 1, 1, 1 );
    
    glBegin( GL_TRIANGLE_STRIP );
    glTexCoord2f (vid.texture_s1, 0);
    glVertex2f (1, 0);
    
    glTexCoord2f (0, 0);
    glVertex2f (0, 0);
    
    glTexCoord2f (vid.texture_s1, vid.texture_t1);
    glVertex2f (1, 1);
    
    glTexCoord2f (0, vid.texture_t1);
    glVertex2f (0, 1);
    glEnd();
    
    [sysplat.gVidWindow endFrame];

}


// Baker: VID_SetMode designates this as the resize handler.  Fruitz has resizable window for both GL and Win
// The GL version updates the size info and runs a frame or 2.
// Baker: WinQuake was using it.  It currently is unused.  It may make a comeback in the future.
void VID_ResizeHandler (id view, void* pContext)
{
    FD_UNUSED (view, pContext);
    
    VID_RenderTexture();
}


// Exclusively called VID_Local_SetMode and only once
static void sVID_Local_SetMode_GetBuffers (int newwidth, int newheight)
{
// We are similar to VID_WinQuake_AllocBuffers.
    const size_t    col32Bytes      = newwidth * newheight * sizeof (unsigned int);
    const size_t    colorBytes      = newwidth * newheight * sizeof (byte);
    const size_t    depthBytes      = newwidth * newheight * sizeof (short);
    const size_t    cacheBytes      = D_SurfaceCacheForRes (newwidth, newheight);
    const size_t    totalBytes      = col32Bytes + colorBytes + depthBytes + cacheBytes;
    void *          pBuffer         = malloc (totalBytes);
    
    if (pBuffer == NULL)
        System_Error("Not enough memory for video buffers left!");
    
    memset (pBuffer, 0x00, totalBytes);
    
    vid.bitmap             = pBuffer;
    vid.buffer = ((byte*) pBuffer) + col32Bytes;
    d_pzbuffer = vid.pzbuffer = (short*) (vid.buffer + colorBytes);
    vid.surfcache = vid.buffer + colorBytes + depthBytes;
    vid.surfcachesize = totalBytes;
    vid.rowbytes = newwidth;
    
    D_InitCaches (vid.surfcache, D_SurfaceCacheForRes (newwidth, newheight));
    sVID_GetBuffers_InitializeTexture(newwidth, newheight);
}




#endif // WINQUAKE_RENDERER_SUPPORT

// Baker: Called by setmode and shutdown
void VID_FlushBuffers (void)
{
#ifdef GLQUAKE_RENDERER_SUPPORT
//	delete textures?
#endif // GLQUAKE_RENDERER_SUPPORT
	
#ifdef WINQUAKE_RENDERER_SUPPORT
	if (vid.bitmap)
    {
        D_FlushCaches ();
        free (vid.bitmap);
        vid.bitmap = NULL, vid.buffer = NULL;
    }
    
    sVID_FlushBuffers_ShutdownTexture();
#endif // WINQUAKE_RENDERER_SUPPORT
}

// Baker: Supposed to be called by VID_Activate only but we have WillHide an WillUnhide calling this
// On the Mac.  For now ...
void VID_Local_Suspend (cbool doSuspend)
{
    if (vid.wassuspended == doSuspend || vid.screen.type == MODE_WINDOWED)
        return;
    
    if (doSuspend)
    {
        
        [sysplat.gVidWindow orderOut: nil];
        [sysplat.gVidDisplay setDisplayMode: [sysplat.gVidDisplay originalMode]];
        [sysplat.gVidDisplay releaseDisplay];
    }
    else
    {
        [sysplat.gVidDisplay captureDisplay];
        [sysplat.gVidDisplay setDisplayMode: (FDDisplayMode*)vid.modelist[vid.modenum_screen].ptr];
        [sysplat.gVidWindow makeKeyAndOrderFront: nil];
        
    }
    
    vid.wassuspended = doSuspend;
}




void VID_SetOriginalMode (void);
void VID_Local_Window_Renderer_Teardown (int destroy, cbool reset_video_mode)
{
    VID_FlushBuffers ();
    VID_SetOriginalMode ();
}


// Baker: Called by setmode and shutdown
// Baker: Note that this destroys the window.  Mac setmode will reinitialize one.
void VID_SetOriginalMode (void)
{
    if (sysplat.gVidWindow)
    {
        [sysplat.gVidWindow close];
        sysplat.gVidWindow = nil;
    }
    
    if ([FDDisplay isAnyDisplayCaptured] == YES)
    {
        [sysplat.gVidDisplay setDisplayMode: [sysplat.gVidDisplay originalMode]];
        [sysplat.gVidDisplay releaseDisplay];
    }
}

void VID_Local_Vsync (void)
{

    const BOOL enable  = (!!vid_vsync.value);
    const BOOL result = [sysplat.gVidWindow setVsync: enable];

    if (enable == result)
    {
        Con_DPrintLinef ("video wait successfully %s!", enable ? "enabled" : "disabled");
    }
    else Con_PrintLinef ("Error while trying to change video wait!");
}

void VID_Local_Vsync_f (cvar_t *var)
{
// Baker: In Windows, this function might tell the user something about vsync and Direct3D
// But that doesn't apply on the Mac, so we just run the function.
    VID_Local_Vsync ();
}

// Baker: Returning true means reused context ok
// Mac needs to return false.
cbool VID_Local_SetMode (int modenum)
{
#if 0    
    Con_PrintLinef ("Setting mode #%d", modenum);
    Con_PrintLinef ("Switching to: %dx%d...", vid.modelist[mode].width, vid.modelist[mode].height);
#endif
    
    // free all buffers:
    VID_FlushBuffers ();
    
    if (vid.modelist[modenum].type == MODE_FULLSCREEN)
    {
        static int first_time = 1;
        
        if ([sysplat.gVidDisplay isCaptured] == NO)
            [sysplat.gVidDisplay captureDisplay];

        [sysplat.gVidWindow close];
        sysplat.gVidWindow = nil;

        if (![sysplat.gVidDisplay setDisplayMode: (FDDisplayMode*)vid.modelist[modenum].ptr])
            System_Error ("Unable to switch the displaymode!");

#if 1 // Baker: If initially setting it, seems to need a second kick otherwise sets wrong mode.
        if (first_time)
        {
            if (![sysplat.gVidDisplay setDisplayMode: (FDDisplayMode*)vid.modelist[modenum].ptr])
                System_Error ("Unable to switch the displaymode!");
        
            first_time = 0;
        }
#endif

#ifdef GLQUAKE_RENDERER_SUPPORT
        sysplat.gVidWindow = [[FDWindow alloc] initForDisplay: sysplat.gVidDisplay samples:(int)vid_multisample.value];
#else
		sysplat.gVidWindow = [[FDWindow alloc] initForDisplay: sysplat.gVidDisplay];
#endif
        [sysplat.gVidWindow makeKeyAndOrderFront: nil];
        [sysplat.gVidWindow flushWindow];
        
    }
    else
    {
        const NSRect contentRect = NSMakeRect ((vid.desktop.width - vid.modelist[modenum].width) / 2, (vid.desktop.height - vid.modelist[modenum].height) / 2, vid.modelist[modenum].width, vid.modelist[modenum].height);

        VID_SetOriginalMode ();
#ifdef GLQUAKE_RENDERER_SUPPORT
        sysplat.gVidWindow = [[FDWindow alloc] initWithContentRect: contentRect samples:(int)vid_multisample.value];
#else
        sysplat.gVidWindow = [[FDWindow alloc] initWithContentRect: contentRect];
#endif
        
        [sysplat.gVidWindow setTitle: @ENGINE_FAMILY_NAME];
        
        // Baker: The form will position itself, lets not be annyoing and do it yet again
        // if we start up in windowed mode
        [sysplat.gVidWindow makeKeyAndOrderFront: nil];
        [sysplat.gVidWindow makeMainWindow];
        [sysplat.gVidWindow flushWindow];
   }

#ifdef GLQUAKE_RENDERER_SUPPORT
	[[sysplat.gVidWindow openGLContext] makeCurrentContext];
#endif // GLQUAKE_RENDERER_SUPPORT


#ifdef WINQUAKE_RENDERER_SUPPORT
    // allocate new buffers:

#if 1	
	// Find best integral factor, set both the x and the y

	for (vid.stretch_x = 1; vid.modelist[modenum].width  / vid.stretch_x > WINQUAKE_MAX_WIDTH_3000 ; vid.stretch_x ++);
	for (vid.stretch_y = 1; vid.modelist[modenum].height / vid.stretch_y > WINQUAKE_MAX_HEIGHT_1080; vid.stretch_y ++);

	#if 1 // Too much of hassle for right now.
		vid.stretch_x = vid.stretch_y = c_max (vid.stretch_x, vid.stretch_y);

	#else
		vid.stretch_old_cvar_val = (int)vid_sw_stretch.value; // This isn't the actual stretch, but the cvar value attempted.
		// Ok we need to validate this.
		// Let's say I want 4.  I can't have 4 in 640x480.  /320  /240  highx = (int)(vid.modelist[modenum].width / 320);

		vid.stretch_x = vid.stretch_y = c_max (vid.stretch_x, vid.stretch_y); // Take the larger of the 2.  Lowest it can be.
		{
			int high_x   = (int)(vid.modelist[modenum].width  / 320);
			int high_y   = (int)(vid.modelist[modenum].height / 240);
			int high_any = c_min (high_x, high_y);

			//int stretch_try = vid.stretch_old_cvar_val;
			int stretch_try = CLAMP(0, vid.stretch_old_cvar_val, 2);
			
			switch (stretch_try) {
			case 0:	stretch_try = 1; break;
			case 2:	stretch_try = 9999; break;
			case 1:	stretch_try = (int)(high_any / 2.0 + 0.5); break;
			}

			if (stretch_try > high_any)
				stretch_try = high_any;

			if (stretch_try < vid.stretch_x)
				stretch_try = vid.stretch_x;

			vid.stretch_x = vid.stretch_y = stretch_try;
		}
	#endif
	vid.conwidth  = vid.modelist[modenum].width  / vid.stretch_x;
	vid.conheight  = vid.modelist[modenum].height  / vid.stretch_y;

	vid.aspect = ((float) vid.conwidth / (float) vid.conheight) * (320.0 / 240.0); // toxic

	sVID_Local_SetMode_GetBuffers (vid.conwidth, vid.conheight);
#else
    sVID_Local_SetMode_GetBuffers (vid.modelist[mode].width, vid.modelist[mode].height);
#endif


#endif // WINQUAKE_RENDERER_SUPPORT
	
	
	VID_Local_Set_Window_Caption (ENGINE_NAME);
	
    vid.ActiveApp = 1;
    vid.canalttab = 1;

	return false; // UNABLE TO REUSE CONTEXT, REUPLOAD TEXTURES
}

void VID_BeginRendering_Resize_Think_Resize_Act (void)
{
}

void VID_Local_Multisample_f (cvar_t* var)
{
#ifdef GLQUAKE_RENDERER_SUPPORT
	if (host_initialized) { // This is run early, don't warn
		Con_PrintLinef ("%s set to " QUOTED_S ".  Will take effect on next video mode change (i.e. press of ALT-ENTER, etc.) .", var->name, var->string);
		Con_PrintLinef ("Note settings are: 2, 4, 8 and 0");
	}
#endif // GLQUAKE_RENDERER_SUPPORT
}



//
//
//
// Equivalent of swap buffers
//
//
//

#ifdef WINQUAKE_RENDERER_SUPPORT
void VID_Update (vrect_t *rects)
{
    if (!vid.initialized || !vid.bitmap)
        return;

    // copy the rendered scene to the texture buffer:
    unsigned int   *pDst = vid.bitmap;
    const byte *   pSrc = vid.buffer;
    const byte *   pEnd = pSrc + vid.screen.width * vid.screen.height;
    
    while (pSrc < pEnd)
    {
        *pDst++ = vid.rgbapal[*pSrc++];
    }

    VID_RenderTexture();

}
#endif // WINQUAKE_RENDERER_SUPPORT

void VID_Local_SwapBuffers (void) // GLQuake uses
{
    [sysplat.gVidWindow endFrame];
}

//
//
//
// Palette Set
// 
//
//

#ifdef WINQUAKE_RENDERER_SUPPORT
void VID_Local_Modify_Palette (byte *palette)
{
// The OS X style implementation doesn't need to do anything here
}

void VID_Local_SetPalette (byte *palette)
{
    for (unsigned int i = 0; PALETTE_COLORS_256 < 256; ++i)
    {
        const UInt  red     = palette[i * 3 + 0];
        const UInt  green   = palette[i * 3 + 1];
        const UInt  blue    = palette[i * 3 + 2];
        const UInt  alpha   = 0xFF;
        const UInt  color   = (red <<  0) + (green <<  8) + (blue << 16) + (alpha << 24);
        vid.rgbapal[i]      = color;
    }
}
#endif // WINQUAKE_RENDERER_SUPPORT

void VID_Local_Set_Window_Caption (const char *text)
{
    if (sysplat.gVidWindow != nil)
    {
        if (text)
        {
            [sysplat.gVidWindow setTitle: [NSString stringWithCString: text encoding: NSASCIIStringEncoding]];
        }
        else
        {
            [sysplat.gVidWindow setTitle: [NSString stringWithCString: ENGINE_FAMILY_NAME encoding: NSASCIIStringEncoding]];
        }
    }
}

void VID_Local_Init (void)
{
	// Early
#ifdef WINQUAKE_RENDERER_SUPPORT
	VID_Palette_NewGame ();
#endif // WINQUAKE_RENDERER_SUPPORT
	// Early
#ifdef GLQUAKE_RENDERER_SUPPORT
	VID_Renderer_Setup ();
#endif // GLQUAKE_RENDERER_SUPPORT
}

void VID_Local_Shutdown (void)
{
	VID_Local_Window_Renderer_Teardown (TEARDOWN_FULL_1, true /*reset video mode*/);
}