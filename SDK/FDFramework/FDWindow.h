
//
// "FDWindow.h"
//
// Written by:	Axel 'awe' Wefers			[mailto:awe@fruitz-of-dojo.de].
//				©2001-2012 Fruitz Of Dojo 	[http://www.fruitz-of-dojo.de].
//


#import "FDDisplay.h"
#import <Cocoa/Cocoa.h>

@interface FDWindow : NSWindow
{
}

- (id) initForDisplay: (FDDisplay*) display; // WinQuake Fullscreen
- (id) initForDisplay: (FDDisplay*) display samples: (NSUInteger) samples; // GLQuake fullscreen

- (id) initWithContentRect: (NSRect) rect; // WinQuake
- (id) initWithContentRect: (NSRect) rect samples: (NSUInteger) samples; // GLQuake

- (BOOL) setVsync: (BOOL) enabled;
- (BOOL) inDragMove;
- (void) dragMoveEnded;
- (BOOL) captureMouse: (BOOL) captureTheMouse;
- (NSOpenGLContext*) openGLContext;

- (void) endFrame; // Baker: This might as well be VID_Local_SwapBuffers


@end
