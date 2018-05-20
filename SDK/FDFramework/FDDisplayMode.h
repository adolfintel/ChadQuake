
//
// "FDDisplayMode.h"
//
// Written by:	Axel 'awe' Wefers			[mailto:awe@fruitz-of-dojo.de].
//				©2001-2012 Fruitz Of Dojo 	[http://www.fruitz-of-dojo.de].
//


#import <Cocoa/Cocoa.h>



@interface FDDisplayMode : NSObject
{
}

- (NSUInteger) width;
- (NSUInteger) height;
- (NSUInteger) bitsPerPixel;
- (BOOL) isStretched;
- (BOOL) isDefault;
- (double) refreshRate;

- (NSString*) description;

- (BOOL) isEqualTo: (FDDisplayMode*) object;
- (NSComparisonResult) compare: (FDDisplayMode*) rhs;

@end


