
//
// "FDWindow.h"
//
// Written by:  Axel 'awe' Wefers           [mailto:awe@fruitz-of-dojo.de].
//              ©2001-2012 Fruitz Of Dojo  [http://www.fruitz-of-dojo.de].
//


#import "FDWindow.h"
#import "FDDisplay.h"
#import "FDDisplayMode.h"
#import "FDDebug.h"
#import "FDView.h"

#import <Cocoa/Cocoa.h>
//#include "quakedef.h"



#define FD_MINI_ICON_WIDTH          ( 128 )
#define FD_MINI_ICON_HEIGHT         ( 128 )



@interface FDView ()


- (void) setOpenGLContext: (NSOpenGLContext*) openGLContext;
- (NSBitmapImageRep*) bitmapRepresentation;


@end



@interface _FDWindow : FDWindow
{
@private
    NSImage*            mMiniImage;
    FDView*             mView;
    FDDisplay*          mDisplay;
}

- (NSOpenGLPixelFormat*) createGLPixelFormatWithBitsPerPixel: (NSUInteger) bitsPerPixel samples: (NSUInteger) samples;
- (NSOpenGLContext*) createGLContextWithBitsPerPixel: (NSUInteger) bitsPerPixel samples: (NSUInteger) samples;
- (NSImage*) createMiniImageWithSize: (NSSize) size;
- (void) drawMiniImage;
- (void) screenParametersDidChange: (NSNotification*) notification;
- (void) keyDown: (NSEvent*) event;
- (void)windowDidMove:(NSNotification *)notification;
- (void)windowWillMove:(NSNotification *)notification;


@end



@implementation _FDWindow

- (id) initForDisplay: (FDDisplay*) display samples: (NSUInteger) samples
{
    self = [super initWithContentRect: [display frame]
                            styleMask: NSBorderlessWindowMask
                              backing: NSBackingStoreBuffered
                                defer: NO];
    
    if (self != nil)
    {
        const NSUInteger    bitsPerPixel    = [[display displayMode] bitsPerPixel];
        const NSRect        frameRect       = [[self contentView] frame];
        NSOpenGLContext*    glContext       = [self createGLContextWithBitsPerPixel: bitsPerPixel samples: samples];
        
        mView       = [[FDView alloc] initWithFrame: frameRect];
        mDisplay    = [display retain];
        

        [self setAcceptsMouseMovedEvents: YES];
        [self setBackgroundColor: [NSColor blackColor]];
        [self setContentView: mView];
#if 1 // Exclusive to full screen mode
        [self setLevel: CGShieldingWindowLevel()];
        [self setOpaque: YES];
        [self setDocumentEdited: YES]; // Baker: Possibly to require polite shutdown requests from OS X
        //[self setHidesOnDeactivate: YES];
        [self disableScreenUpdatesUntilFlush];
  
        [mView setNeedsDisplay: YES];
#endif         
        [self makeFirstResponder: mView];

        [mView setOpenGLContext: glContext];
                
        [glContext release];
    }
    
    return self;
}



// Baker: VID_SetMode fullscreen calls this
- (id) initForDisplay: (FDDisplay*) display
{
    return [self initForDisplay: display samples: 0];
}



// Baker: VID_SetMode windowed mode calls this
- (id) initWithContentRect: (NSRect) rect samples: (NSUInteger) samples
{
    self = [super initWithContentRect: rect
                            styleMask: NSTitledWindowMask |
                                       NSClosableWindowMask |
                                       NSMiniaturizableWindowMask /*|
         Baker: Resize off             NSResizableWindowMask */
                              backing: NSBackingStoreBuffered
                                defer: NO];
    
    if (self != nil)
    {
        const NSUInteger    bitsPerPixel    = NSBitsPerPixelFromDepth ([[self screen] depth]);
        NSOpenGLContext*    glContext       = [self createGLContextWithBitsPerPixel: bitsPerPixel samples: samples];
        
        mView = [[FDView alloc] initWithFrame: rect];

        [self center];

        [self setAcceptsMouseMovedEvents: YES];
        [self setBackgroundColor: [NSColor blackColor]];
        [self setContentView: mView];
#if 1 // Exclusive to windowed mode
        [self setDocumentEdited: YES]; // Baker: Possibly to require polite shutdown requests from OS X
        [self setMinSize: rect.size];
        [self setContentAspectRatio: rect.size];
 //       [self setShowsResizeIndicator: NO];
        [self useOptimizedDrawing: NO];
        
        
        
#endif
        [self makeFirstResponder: mView];
//      [self center];
        [mView setOpenGLContext: glContext];
//        [self center];
        [glContext release];
#if 1 //  Exclusive to windowed mode
        mMiniImage = [self createMiniImageWithSize: NSMakeSize (FD_MINI_ICON_WIDTH, FD_MINI_ICON_HEIGHT)]; 
        
        [[NSNotificationCenter defaultCenter] addObserver: self
                                                 selector: @selector (screenParametersDidChange:)
                                                     name: NSApplicationDidChangeScreenParametersNotification
                                                   object: nil];
        
//      NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
        [[NSNotificationCenter defaultCenter]  addObserver: self selector:@selector(windowWillMove:) name:NSWindowWillMoveNotification object:nil];
        [[NSNotificationCenter defaultCenter]  addObserver: self selector:@selector(windowDidMove:) name:NSWindowDidMoveNotification object:nil];
#endif // Exclusive to windowed view
//      [self center];
    }
    
    return self;
}



- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver: self
                                                    name: NSApplicationDidChangeScreenParametersNotification
                                                  object: nil];
    Input_Local_Capture_Mouse (0); // Baker: Need to show the cursor briefly on vid_restart
    [mMiniImage release];
    [mView release];

    [mDisplay release];
    
    [super dealloc];
}



- (NSOpenGLContext*) openGLContext
{
    return [mView openGLContext];
}

- (void)windowDidMove:(NSNotification *)notification
{
#if 0
    NSPoint p =[mView convertPoint:[self convertScreenToBase:[NSEvent mouseLocation]] fromView:nil];

    NSRect m = [self frame];
    NSRect v = [mView frame];

        NSRect  nsRect3     = [self frame];
    nsRect3.size.height = v.size.height;
    nsRect3.origin.x = 0;
    nsRect3.origin.y = 0;   Con_Printf ("windowDidMove cursor %g %g (frame: %g %g - %g %g)\n", p.x, p.y,
                m.origin.x, m.origin.y, m.origin.x + m.size.width, m.origin.y + m.size.height);
#endif
//  isMoving = NO;
}

BOOL isMoving;
- (void)windowWillMove:(NSNotification *)notification
{
#ifdef DEBUG_EVENTS
    Con_Printf ("windowWillMove\n");
#endif
    isMoving = YES;
    
}

- (BOOL) inDragMove
{
    return isMoving;
}

- (void) dragMoveEnded
{
    isMoving = NO;
}

- (BOOL) captureMouse: (BOOL) captureTheMouse
{

    // Baker: We might refuse the capture if the cursor is above the window
    // This allows dragging the window by the title bar or using the menu on a Mac.
    // Need to implement this on windows too.

    if (captureTheMouse && mDisplay == nil /*vid.screen.type == MODE_WINDOWED*/)
    {
        static int first = 1;
        if (!first && isMoving)
        {
#pragma message ("Baker: Todo, if isMoving, intercept mouse up")
            return NO; // Moving and I can't trust the coordinates
            
        }

        NSPoint myPoint =[mView convertPoint:[self convertScreenToBase:[NSEvent mouseLocation]] fromView:nil];
        
        NSRect v = [mView frame];
        NSRect  nsRect3     = [self frame];
        
        nsRect3.size.height = v.size.height;
        nsRect3.origin.x = 0;
        nsRect3.origin.y = 0;
        BOOL pointInRect = NSPointInRect (myPoint, nsRect3 );
        
    
        if (!first && !pointInRect /*&& myPoint.y > nsRect2.origin.y + nsRect2.size.height*/)
        {
            // Baker: Do not acquire until mouse is in the rect.
            // Because we could be dragging the window or something.
            return NO;
        }
        
        first = 0; // Baker: We never reject the first capture.
    }

    CGAssociateMouseAndMouseCursorPosition (!captureTheMouse);

    if (captureTheMouse)
    {
        const NSRect    nsRect      = [self frame];
        const CGRect    cgRect      = CGDisplayBounds (CGMainDisplayID ());
        const NSPoint   nsCenter    = NSMakePoint (NSMidX (nsRect), NSMidY (nsRect));
        const CGPoint   cgCenter    = CGPointMake (nsCenter.x, cgRect.size.height - nsCenter.y);
        
        CGWarpMouseCursorPosition (cgCenter);
        [NSCursor hide];
    }
    else // Relase the mouse
    {
        // Baker: Release the mouse somewhere non-annoying, i.e. not center of screen
        const NSRect    nsRect      = [self frame]; // Baker: This is relative to the screen
        const CGRect    cgRect      = CGDisplayBounds (CGMainDisplayID ());
        const NSPoint   nsCenter    = NSMakePoint (NSMidX (nsRect), NSMidY (nsRect));
        const CGPoint   cgCenter    = CGPointMake (nsCenter.x + nsRect.size.width * 0.40, cgRect.size.height - nsCenter.y - nsRect.size.height * 0.40);
        
        CGWarpMouseCursorPosition (cgCenter);

        [NSCursor unhide];
    }
    
    return YES;
}

- (BOOL) setVsync: (BOOL) enabled
{
    return [mView setVsync: enabled];
}

// Baker: This may as well be swapbuffers
- (void) endFrame
{
    if ([self isMiniaturized] == YES)
    {
        [self drawMiniImage];
    }
    else
    {        
        CGLFlushDrawable ([[self openGLContext] CGLContextObj]);
    }
}



- (BOOL) acceptsFirstResponder
{
    return YES;
}



- (BOOL) canBecomeMainWindow
{
    return YES;
}



- (BOOL) canBecomeKeyWindow
{
    return YES;
}



- (BOOL) canHide
{
    return YES;
}



- (BOOL) windowShouldClose: (id) sender
{
    const BOOL  shouldClose = (mDisplay != nil); // Was [self isFullscreen] If mDisplay, we are fullscreen
    
    if (shouldClose == NO)
    {
        [NSApp terminate: nil];

    }
    
    return shouldClose;
}



- (NSOpenGLPixelFormat*) createGLPixelFormatWithBitsPerPixel: (NSUInteger) bitsPerPixel samples: (NSUInteger) samples
{
    NSOpenGLPixelFormat*            pixelFormat = nil;
    NSOpenGLPixelFormatAttribute    attributes[32];
    UInt16                          i = 0;
    
    if (bitsPerPixel != 16)
    {
        bitsPerPixel = 32;
    }
    
    attributes[i++] = NSOpenGLPFANoRecovery;
    
    attributes[i++] = NSOpenGLPFAClosestPolicy;
    
    attributes[i++] = NSOpenGLPFAAccelerated;
    
    attributes[i++] = NSOpenGLPFADoubleBuffer;
    
    attributes[i++] = NSOpenGLPFADepthSize;
    attributes[i++] = 1;
    
    attributes[i++] = NSOpenGLPFAAlphaSize;
    attributes[i++] = 0;
    
    attributes[i++] = NSOpenGLPFAStencilSize;
    attributes[i++] = 8; // Technically only the GLQUAKE needs this, but this is supposed to be unaware of project level information
    
    attributes[i++] = NSOpenGLPFAAccumSize;
    attributes[i++] = 0;
    
    attributes[i++] = NSOpenGLPFAColorSize;
    attributes[i++] = (NSOpenGLPixelFormatAttribute) bitsPerPixel;

    if (samples > 0)
    {
        switch (samples)
        {
            case 2:
            case 4:
            case 8:
                break;
                
            default:
                samples = 8;
                break;
        }
        
        attributes[i++] = NSOpenGLPFASampleBuffers;
        attributes[i++] = 1;
        attributes[i++] = NSOpenGLPFASamples;
        attributes[i++] = (NSOpenGLPixelFormatAttribute) samples;
    }

    attributes[i++] = 0;
    
    pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes: attributes];
    
    if (pixelFormat == nil)
    {
        FDError (@"Unable to find a matching pixelformat. Please try other displaymode(s).");
    }
    
    return pixelFormat;
}



- (NSOpenGLContext*) createGLContextWithBitsPerPixel: (NSUInteger) bitsPerPixel samples: (NSUInteger) samples
{
    NSOpenGLPixelFormat*    pixelFormat = [self createGLPixelFormatWithBitsPerPixel: bitsPerPixel samples: samples];
    NSOpenGLContext*        context     = [[NSOpenGLContext alloc] initWithFormat: pixelFormat shareContext: nil];
    
    if (context == nil)
    {
        FDError (@"Unable to create an OpenGL context. Please try other displaymode(s).");
    }

    [pixelFormat release];
    
    return context;
}



- (NSImage*) createMiniImageWithSize: (NSSize) size
{
    NSGraphicsContext*  graphicsContext = nil;
    NSImage*            miniImage = [[NSImage alloc] initWithSize: size];
    
    [miniImage setFlipped: YES];
    [miniImage lockFocus];
    
    graphicsContext = [NSGraphicsContext currentContext];
    [graphicsContext setImageInterpolation: NSImageInterpolationNone];
    [graphicsContext setShouldAntialias: NO];
    
    [miniImage unlockFocus];
    
    return miniImage;
}



- (void) drawMiniImage
{
    if ([self isMiniaturized] == YES)
    {
        if (mView != nil)
        {
            NSBitmapImageRep* bitmap = [mView bitmapRepresentation];
            
            if (bitmap != nil)
            {
                const NSSize size           = [mMiniImage size];
                const NSRect contentRect    = [mView frame];
                const float  aspect         = NSWidth (contentRect) / NSHeight (contentRect);
                const NSRect clearRect      = NSMakeRect( 0.0, 0.0, size.width, size.height );
                NSRect       miniImageRect  = clearRect;
                
                if (aspect >= 1.0f)
                {
                    miniImageRect.size.height /= aspect;
                    miniImageRect.origin.y = (size.height - NSHeight (miniImageRect)) * 0.5f;
                }
                else
                {
                    miniImageRect.size.width /= aspect;
                    miniImageRect.origin.x = (size.width - NSWidth (miniImageRect)) * 0.5f;
                }
                
                [mMiniImage lockFocus];
                [[NSColor clearColor] set];
                NSRectFill (clearRect);
                [bitmap drawInRect: miniImageRect];
                [mMiniImage unlockFocus];
                
                [self setMiniwindowImage: mMiniImage];
            }
        }
    }
}


- (void) screenParametersDidChange: (NSNotification*) notification
{
    const NSRect frameRect = [self constrainFrameRect: [self frame] toScreen: [self screen]];
    
    [self setFrame: frameRect display: YES];
    [self center];
}


- (void) keyDown: (NSEvent*) event
{
    // Already handled by FDHIDInput, implementation avoids the NSBeep() caused by unhandled key events.
}

@end


@implementation FDWindow

+ (id) allocWithZone: (NSZone*) zone
{
    return NSAllocateObject ([_FDWindow class], 0, zone);
}


- (id) init
{
    self = [super init];
    
    if (self != nil)
    {
        [self doesNotRecognizeSelector: _cmd];
        [self release];
    }
    
    return nil;
}


- (id) initForDisplay: (FDDisplay*) display samples: (NSUInteger) samples
{
    FD_UNUSED (display, samples);
    
    self = [super init];
    
    return self;
}


- (id) initForDisplay: (FDDisplay*) display
{
    return [self initForDisplay: display samples: 0];
}


- (id) initWithContentRect: (NSRect) rect samples: (NSUInteger) samples
{
    FD_UNUSED (rect, samples);
    
    self = [super init];
    
    return self;    
}


- (id) initWithContentRect: (NSRect) rect
{
    return [self initWithContentRect: rect samples: 0];
}

- (NSOpenGLContext*) openGLContext
{
    [self doesNotRecognizeSelector: _cmd];
    
    return nil;
}

- (void) endFrame
{
    [self doesNotRecognizeSelector: _cmd];
}


@end

