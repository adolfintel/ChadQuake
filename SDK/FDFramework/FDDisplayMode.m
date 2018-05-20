
//
// "FDDisplayMode.m"
//
// Written by:  Axel 'awe' Wefers           [mailto:awe@fruitz-of-dojo.de].
//              ©2001-2012 Fruitz Of Dojo  [http://www.fruitz-of-dojo.de].
//


#import "FDDisplayMode.h"
#import "FDDefines.h"

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>



typedef struct
{
    CFStringRef     mName;
    NSUInteger      mBitsPerPixel;
} PixelEncodingToBitsPerPixel;



const PixelEncodingToBitsPerPixel   skPixelEncodingToBitsPerPixel[] = {
                                                                        { CFSTR (IO1BitIndexedPixels),  1 },
                                                                        { CFSTR (IO2BitIndexedPixels),  2 },
                                                                        { CFSTR (IO4BitIndexedPixels),  4 },
                                                                        { CFSTR (IO8BitIndexedPixels),  8 },
                                                                        { CFSTR (IO16BitDirectPixels), 16 },
                                                                        { CFSTR (IO32BitDirectPixels), 32 }
                                                                      };



@interface _FDDisplayMode : FDDisplayMode
{
@private
    CGDisplayModeRef    mCGDisplayMode;
}

- (id) initWithCGDisplayMode: (CGDisplayModeRef) mode;
- (CGDisplayModeRef) cgDisplayMode;

@end



@implementation _FDDisplayMode

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



- (id) initWithCGDisplayMode: (CGDisplayModeRef) cgDisplayMode
{
    self = [super init];
    
    if (self)
    {
        const uint32_t  kMask   = kDisplayModeValidFlag | kDisplayModeNeverShowFlag | kDisplayModeNotGraphicsQualityFlag;
        BOOL            isValid = ((CGDisplayModeGetIOFlags (cgDisplayMode) & kMask) == kDisplayModeValidFlag);
        
        isValid &= ((CGDisplayModeGetWidth (cgDisplayMode) * CGDisplayModeGetHeight (cgDisplayMode)) > 1);
        
        if (isValid)
        {
            mCGDisplayMode = CGDisplayModeRetain (cgDisplayMode);
        }
        else
        {
            [self release];
            self = nil;
        }
    }
    
    return self;
}



- (void) dealloc
{
    if (mCGDisplayMode != NULL)
    {
        CGDisplayModeRelease (mCGDisplayMode);
    }
    
    [super dealloc];
}



- (NSUInteger) width
{
    return CGDisplayModeGetWidth (mCGDisplayMode);
}



- (NSUInteger) height
{
    return CGDisplayModeGetHeight (mCGDisplayMode);
}



- (BOOL) isStretched
{
    return (CGDisplayModeGetIOFlags (mCGDisplayMode) & kDisplayModeStretchedFlag) == kDisplayModeStretchedFlag;
}



- (BOOL) isDefault
{
    return (CGDisplayModeGetIOFlags (mCGDisplayMode) & kDisplayModeDefaultFlag) == kDisplayModeDefaultFlag;
}



- (NSUInteger) bitsPerPixel
{
    CFStringRef pixelEncoding   = CGDisplayModeCopyPixelEncoding (mCGDisplayMode);
    NSUInteger  bitsPerPixel    = 0;
    
    for ( NSUInteger i = 0; i < FD_SIZE_OF_ARRAY (skPixelEncodingToBitsPerPixel); ++i )
    {
        if (CFStringCompare (pixelEncoding, skPixelEncodingToBitsPerPixel[i].mName, 0) == kCFCompareEqualTo)
        {
            bitsPerPixel = skPixelEncodingToBitsPerPixel[i].mBitsPerPixel;
            
            break;
        }
    }

    CFRelease (pixelEncoding);
    
    return bitsPerPixel;
}



- (double) refreshRate
{
    return CGDisplayModeGetRefreshRate (mCGDisplayMode);
}



- (CGDisplayModeRef) cgDisplayMode
{
    return mCGDisplayMode;
}



- (NSString*) description
{
    const unsigned long width           = [self width];
    const unsigned long height          = [self height];
    const double        refreshReate    = [self refreshRate];
    NSString*           description     = [NSString stringWithFormat: @"%lux%lu %.0fHz", width, height, refreshReate];
    
    if ([self isStretched] == YES)
    {
        description = [description stringByAppendingString: @" (stretched)"];
    }
    
    return description;
}



- (BOOL) isEqualTo: (FDDisplayMode*) rhs
{
    return ([self width] == [rhs width]) &&
           ([self height] == [rhs height]) &&
           ([self bitsPerPixel] == [rhs bitsPerPixel]) &&
           ([self isStretched] == [rhs isStretched]) &&
           ([self refreshRate] == [rhs refreshRate]);
}



- (NSComparisonResult) compare: (FDDisplayMode*) rhs
{
    const NSUInteger    lhsArea     = [self width] * [self height];
    const NSUInteger    rhsArea     = [rhs width] * [rhs height];
    NSComparisonResult  result      = NSOrderedDescending;
    
    if (lhsArea < rhsArea)
    {
        result = NSOrderedAscending;
    }
    else if (lhsArea == rhsArea)
    {
        if ([self refreshRate] < [rhs refreshRate])
        {
            result = NSOrderedAscending;    
        }
        else if (([self isStretched] == NO) && ([rhs isStretched] == YES))
        {
            result = NSOrderedAscending;
        }
    }
    
    return result;
}

@end



@implementation FDDisplayMode

+ (id) allocWithZone: (NSZone*) zone
{
    return NSAllocateObject ([_FDDisplayMode class], 0, zone);
}



- (NSUInteger) width
{
    [self doesNotRecognizeSelector: _cmd];
    
    return 0;
}



- (NSUInteger) height
{
    [self doesNotRecognizeSelector: _cmd];
    
    return 0;    
}



- (BOOL) isStretched
{
    [self doesNotRecognizeSelector: _cmd];
    
    return NO;
}



- (BOOL) isDefault
{
    [self doesNotRecognizeSelector: _cmd];
    
    return NO;
}



- (NSUInteger) bitsPerPixel
{
    [self doesNotRecognizeSelector: _cmd];
    
    return 0;
}



- (double) refreshRate
{
    [self doesNotRecognizeSelector: _cmd];
    
    return 0.0;
}



- (NSString*) description
{
    [self doesNotRecognizeSelector: _cmd];
    
    return nil;
}



- (BOOL) isEqualTo: (FDDisplayMode*) object
{
    FD_UNUSED (object);
    
    [self doesNotRecognizeSelector: _cmd];
    
    return NO;
}



- (NSComparisonResult) compare: (FDDisplayMode*) rhs
{
    FD_UNUSED (rhs);
    
    [self doesNotRecognizeSelector: _cmd];
    
    return NSOrderedSame;
}

@end


