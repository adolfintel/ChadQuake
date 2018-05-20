
//
// "FDView.m"
//
// Written by:  Axel 'awe' Wefers           [mailto:awe@fruitz-of-dojo.de].
//              ©2001-2012 Fruitz Of Dojo  [http://www.fruitz-of-dojo.de].
//


#import "FDView.h"
#import "FDWindow.h"
#import "FDDebug.h"

#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#import <OpenGL/glext.h>

#include "quakedef.h"


@interface _FDView : FDView
{
@private

    NSOpenGLContext*    mOpenGLContext;
    NSBitmapImageRep*   mBitmapRep;
    GLuint              mBitmapTexture;
}

- (void) setOpenGLContext: (NSOpenGLContext*) openGLContext;
- (NSBitmapImageRep*) bitmapRepresentation;
- (BOOL) canHide;
@end



@implementation _FDView

- (void) dealloc
{
    
    [self setOpenGLContext: nil];
    [mBitmapRep release];
    
    [super dealloc];
}

- (BOOL) canHide
{
    return YES;
}

- (BOOL) getVsync
{
    BOOL isEnabled = NO;
    
    if (mOpenGLContext != nil)
    {
        GLint param = 0;
        
        [mOpenGLContext makeCurrentContext];

        if (CGLGetParameter (CGLGetCurrentContext (), kCGLCPSwapInterval, &param) == CGDisplayNoErr)
        {
            isEnabled = (param != 0);
        }
    }
    
    return isEnabled;
}

- (BOOL) setVsync: (BOOL) enabled
{
    if (mOpenGLContext != nil)
    {
        const GLint param = (enabled == YES) ? 1 : 0;
        
        [mOpenGLContext makeCurrentContext];
                        
 
        if (CGLSetParameter (CGLGetCurrentContext (), kCGLCPSwapInterval, &param) != CGDisplayNoErr)
        {
            FDLog (@"Failed to set CGL swap interval");
        }
    }

    return [self getVsync];
}


- (void) setOpenGLContext: (NSOpenGLContext*) openGLContext
{
    CGLContextObj kOldContext = CGLGetCurrentContext ();
    if (mOpenGLContext != nil)
    {
        [mOpenGLContext makeCurrentContext];
                                
        [NSOpenGLContext clearCurrentContext];
        [mOpenGLContext clearDrawable];
        [mOpenGLContext release];
   
        // Baker: We set the current context, restore it back
        if (kOldContext &&  kOldContext!= mOpenGLContext)
            CGLSetCurrentContext(kOldContext);

        mOpenGLContext = nil;
        
    }
    
    if (openGLContext != nil)
    {
        [openGLContext setView: self];
        
        mOpenGLContext = [openGLContext retain];
        
    }
}


- (NSOpenGLContext*) openGLContext
{
    return mOpenGLContext;
}

- (NSBitmapImageRep*) bitmapRepresentation
{
    const NSRect      frame   = [self frame];
    const NSUInteger  width   = NSWidth (frame);
    const NSUInteger  height  = NSHeight (frame);
    
    if ((mBitmapRep == nil) || ([mBitmapRep pixelsWide] != width) || ([mBitmapRep pixelsHigh] != height))
    {
        [mBitmapRep release];
        
        mBitmapRep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes: NULL
                                                              pixelsWide: width
                                                              pixelsHigh: height
                                                           bitsPerSample: 8
                                                         samplesPerPixel: 4
                                                                hasAlpha: YES
                                                                isPlanar: NO
                                                          colorSpaceName: NSDeviceRGBColorSpace
                                                             bytesPerRow: width << 2
                                                            bitsPerPixel: 32];
    }
    
    if ((mBitmapRep != nil) && (mOpenGLContext != nil))
    {
        UInt8*  pBitmapBuffer = (UInt8*) [mBitmapRep bitmapData];
        
        if (pBitmapBuffer != NULL)
        {
            const UInt8*    pBitmapBufferEnd = pBitmapBuffer + (width << 2) * height;
            
            CGLFlushDrawable ([mOpenGLContext CGLContextObj]);
            
            glReadPixels (0, 0, (GLsizei) width, (GLsizei) height, GL_RGBA, GL_UNSIGNED_BYTE, pBitmapBuffer);
            
            pBitmapBuffer += 3;
            
            while (pBitmapBuffer < pBitmapBufferEnd)
            {
                *pBitmapBuffer  = 0xFF;
                pBitmapBuffer   += sizeof (UInt32);
            }
        }
    }
    
    return mBitmapRep;
}

@end


@implementation FDView

+ (id) allocWithZone: (NSZone*) zone
{
    return NSAllocateObject ([_FDView class], 0, zone);
}

- (BOOL) canHide
{
    return YES;
}
@end
