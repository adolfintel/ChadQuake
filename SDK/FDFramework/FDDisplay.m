
//
// "FDDisplay.m"
//
// Written by:  Axel 'awe' Wefers           [mailto:awe@fruitz-of-dojo.de].
//              ©2001-2012 Fruitz Of Dojo  [http://www.fruitz-of-dojo.de].
//


#import "FDDisplay.h"
#import "FDDisplayMode.h"
#import "FDDefines.h"

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>



static NSArray*                         sDisplays           = nil;
static const NSUInteger                 skGammaTableSize    = 1024;



typedef struct
{
    CGGammaValue        mRed[skGammaTableSize];
    CGGammaValue        mGreen[skGammaTableSize];
    CGGammaValue        mBlue[skGammaTableSize];
    uint32_t            mCount;
} GammaTable;



@interface FDDisplayMode ()

- (id) initWithCGDisplayMode: (CGDisplayModeRef) mode;
- (CGDisplayModeRef) cgDisplayMode;

@end



@interface _FDDisplay : FDDisplay
{
@public
    NSString*           mDisplayName;
    NSArray*            mDisplayModes;
    FDDisplayMode*      mDisplayModeOriginal;
    CGDirectDisplayID   mCGDisplayId;
    CGGammaValue        mCGGamma;
    GammaTable          mGammaTable;
    BOOL                mCanSetGamma;
}

- (id) initWithCGDisplayID: (CGDirectDisplayID) displayId;
- (BOOL) readGammaTable: (GammaTable*) gammaTable;
- (void) applyGamma: (CGGammaValue) gamma withTable: (GammaTable*) gammaTable;

@end



@implementation _FDDisplay

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



- (id) initWithCGDisplayID: (CGDirectDisplayID) displayId
{
    self = [super init];
    
    if (self != nil )
    {
        NSString*           displayName     = nil;
        CGDisplayModeRef    originalMode    = CGDisplayCopyDisplayMode (displayId);
        
        if (CGDisplayIsMain (displayId) == YES)
        {
            displayName = @"Main";
        }
        else
        {
            displayName = [NSString stringWithFormat: @"%lu", (unsigned long) [sDisplays count]];
        }
        
        if (CGDisplayIsBuiltin (displayId) == YES)
        {
            displayName = [displayName stringByAppendingString: @" (built in)"];
        }
        
        if (originalMode)
        {
            mDisplayModeOriginal = [[FDDisplayMode alloc] initWithCGDisplayMode: originalMode];
            
            CGDisplayModeRelease (originalMode);
        }
        
        // filter and sort displaymodes
        CFArrayRef  modes = CGDisplayCopyAllDisplayModes (displayId, NULL);
        
        if (modes != NULL)
        {
            const CFIndex   numModes    = CFArrayGetCount (modes);
            NSMutableArray* modeList    = [[NSMutableArray alloc] initWithCapacity: numModes];
            
            for (CFIndex i = 0; i < numModes; ++i)
            {
                CGDisplayModeRef    mode            = (CGDisplayModeRef) CFArrayGetValueAtIndex (modes, i);                
                FDDisplayMode*      displayMode     = [[FDDisplayMode alloc] initWithCGDisplayMode: mode];
                
                if (displayMode != nil)
                {
                    NSUInteger          bitsPerPixel    = [displayMode bitsPerPixel];
                    BOOL                isValid         = (bitsPerPixel == 32);// || (bitsPerPixel == 16);
                    
                    if (isValid == YES)
                    {
                        NSEnumerator*   modeEnum = [modeList objectEnumerator];
                        FDDisplayMode*  curMode  = nil;
                        
                        while ((isValid == YES) && (curMode = [modeEnum nextObject]))
                        {
                            isValid = ![displayMode isEqualTo: curMode];
                        }
                    }
                    
                    if (isValid == YES)
                    {
                        [modeList addObject: [displayMode autorelease]];
                    }
                    else
                    {
                        [displayMode release];
                    }
                }
            }
            
            CFRelease (modes);
            
            [modeList sortUsingSelector: @selector (compare:)];
            
            mDisplayModes = modeList;
        }

        mDisplayName    = [displayName retain];
        mCGDisplayId    = displayId;
        mCGGamma        = 1.0f;
        mCanSetGamma    = [self readGammaTable: &mGammaTable];
    }
    
    return self;
}



- (void) dealloc
{
    [self setGamma: 1.0f update: YES];
    [self setDisplayMode: mDisplayModeOriginal];
    
    [mDisplayName release];
    [mDisplayModes release];
    [mDisplayModeOriginal release];
    
    [super dealloc];
}



- (FDDisplayMode*) displayMode
{
    CGDisplayModeRef    cgDisplayMode   = CGDisplayCopyDisplayMode (mCGDisplayId);
    FDDisplayMode*      currentMode     = nil;
    
    if (cgDisplayMode != NULL)
    {
        currentMode = [[[FDDisplayMode alloc] initWithCGDisplayMode: cgDisplayMode] autorelease];
        
        CGDisplayModeRelease (cgDisplayMode);
    }
    
    return currentMode;
}



- (FDDisplayMode*) originalMode
{
    return mDisplayModeOriginal;
}



- (NSString*) description
{
    return mDisplayName;
}



- (NSRect) frame
{
    const CGRect    main = CGDisplayBounds (CGMainDisplayID ());
    const CGRect    rect = CGDisplayBounds (mCGDisplayId);

    return NSMakeRect (rect.origin.x, main.size.height - rect.origin.y - rect.size.height, rect.size.width, rect.size.height);
}



- (BOOL) isMainDisplay
{
    return CGDisplayIsMain (mCGDisplayId);
}



- (BOOL) isBuiltinDisplay
{
    return CGDisplayIsBuiltin (mCGDisplayId);
}



- (BOOL) isCaptured
{
    return CGDisplayIsCaptured (mCGDisplayId);
}



- (BOOL) hasFSAA
{
    GLint               maxSampleBuffers    = 0;
    GLint               maxSamples          = 0;
    GLint               numRenderers        = 0;
    CGLRendererInfoObj  rendererInfo        = { 0 };
    CGOpenGLDisplayMask displayMask         = CGDisplayIDToOpenGLDisplayMask (mCGDisplayId);
    CGLError            err                 = CGLQueryRendererInfo (displayMask, &rendererInfo, &numRenderers);
    
    if (err == kCGErrorSuccess)
    {
        for (GLint i = 0; i < numRenderers; ++i)
        {
            GLint numSampleBuffers = 0;
            
            err = CGLDescribeRenderer (rendererInfo, i, kCGLRPMaxSampleBuffers, &numSampleBuffers);
            
            if ((err == kCGErrorSuccess) && (numSampleBuffers > 0))
            {
                GLint numSamples = 0;
                
                err = CGLDescribeRenderer (rendererInfo, i, kCGLRPMaxSamples, &numSamples);
            
                if ((err == kCGErrorSuccess) && (numSamples > maxSamples))
                {
                    maxSamples          = numSamples;
                    maxSampleBuffers    = numSampleBuffers;
                }
            }
        }
        
        CGLDestroyRendererInfo (rendererInfo);
    }
    
    // NOTE: we could return the max number of samples at this point, but unfortunately there is a bug
    //       with the ATI Radeon/PCI drivers: We would return 4 instead of 8. So we assume that the
    //       max samples are always 8 if we have sample buffers and max samples is greater than 1.

    return (maxSampleBuffers > 0) && (maxSamples > 1);
}



- (NSArray*) displayModes
{
    return mDisplayModes;
}



- (BOOL) setDisplayMode: (FDDisplayMode*) displayMode;
{
    return CGDisplaySetDisplayMode (mCGDisplayId, [displayMode cgDisplayMode], NULL) == kCGErrorSuccess;
}



- (BOOL) readGammaTable: (GammaTable*) gammaTable
{
    CGError err = CGGetDisplayTransferByTable (mCGDisplayId,
                                               FD_SIZE_OF_ARRAY (gammaTable->mRed),
                                               &(gammaTable->mRed[0]),
                                               &(gammaTable->mGreen[0]),
                                               &(gammaTable->mBlue[0]),
                                               &(gammaTable->mCount));
    
    return err == kCGErrorSuccess;
}



- (void) applyGamma: (CGGammaValue) gamma withTable: (GammaTable*) gammaTable
{
    if (mCanSetGamma == YES)
    {
        GammaTable  newTable;
        
        for (NSUInteger i = 0; i < gammaTable->mCount; ++i)
        {
            newTable.mRed[i]   = gamma * gammaTable->mRed[i];
            newTable.mGreen[i] = gamma * gammaTable->mGreen[i];
            newTable.mBlue[i]  = gamma * gammaTable->mBlue[i];
        }
        
        CGSetDisplayTransferByTable (mCGDisplayId, gammaTable->mCount, newTable.mRed, newTable.mGreen, newTable.mBlue);
    }
}



- (void) setGamma: (float) gamma update: (BOOL) doUpdate 
{
    if ([self isCaptured])
    {
        if (mCGGamma != gamma)
        {
            mCGGamma = gamma;

            if (doUpdate == YES)
            {
                [self applyGamma: gamma withTable: &mGammaTable];
            }
         }
    }
}



- (float) gamma
{
    return mCGGamma;
}




- (void) captureDisplay
{
    if ([self isCaptured] == NO)
    {
        CGDisplayCapture (mCGDisplayId);
    }
}



- (void) releaseDisplay
{
    if ([self isCaptured] == YES)
    {
        CGDisplayRelease (mCGDisplayId);
    }
}

@end



@implementation FDDisplay

+ (id) allocWithZone: (NSZone*) zone
{
    return NSAllocateObject ([_FDDisplay class], 0, zone);
}



+ (NSArray*) displays
{
    if ( sDisplays == nil )
    {
        uint32_t            numDisplays     = 0;
        CGDirectDisplayID*  pDisplays       = NULL;
        BOOL                success         = (CGGetActiveDisplayList (0, NULL, &numDisplays) == CGDisplayNoErr);
        
        if (success == YES)
        {
            success     = (numDisplays > 0);
        }
        
        if (success == YES)
        {
            pDisplays   = malloc (numDisplays * sizeof (CGDirectDisplayID));
            success     = (pDisplays != NULL);
        }
        
        if (success == YES)
        {
            success = (CGGetActiveDisplayList (numDisplays, pDisplays, &numDisplays) == CGDisplayNoErr);
        }
        
        if (success == YES)
        {
            NSMutableArray* displayList = [[NSMutableArray alloc] initWithCapacity: numDisplays];
            
            sDisplays = displayList;
            
            for (uint32_t i = 0; i < numDisplays; ++i)
            {
                [displayList addObject: [[[_FDDisplay alloc] initWithCGDisplayID: pDisplays[i]] autorelease]];
            }
        }
        
        if (pDisplays != NULL)
        {
            free (pDisplays);
        }
    }
    
    return sDisplays;
}



+ (FDDisplay*) mainDisplay
{
    FDDisplay*  mainDisplay = nil;
    
    for (FDDisplay* display in [FDDisplay displays])
    {
        if ([display isMainDisplay] == YES)
        {
            mainDisplay = display;
            break;
        }
    }
    
    return mainDisplay;
}



+ (BOOL) isAnyDisplayCaptured
{
    BOOL isAnyCaptured = NO;
    
    for (FDDisplay* display in [FDDisplay displays])
    {
        isAnyCaptured = [display isCaptured];
        
        if (isAnyCaptured == YES)
        {
            break;
        }
    }
    
    return isAnyCaptured;
}



- (NSRect) frame
{
    [self doesNotRecognizeSelector: _cmd];
    
    return NSZeroRect;
}



- (NSString*) description
{
    [self doesNotRecognizeSelector: _cmd];
    
    return nil;
}



- (FDDisplayMode*) displayMode
{
    [self doesNotRecognizeSelector: _cmd];
    
    return nil;
}



- (FDDisplayMode*) originalMode
{
    [self doesNotRecognizeSelector: _cmd];
    
    return nil;
}



- (NSArray*) displayModes
{
    [self doesNotRecognizeSelector: _cmd];
    
    return nil;
}



- (BOOL) setDisplayMode: (FDDisplayMode*) displayMode
{
    FD_UNUSED (displayMode);
    
    [self doesNotRecognizeSelector: _cmd];
    
    return NO;
}



- (BOOL) isMainDisplay
{
    [self doesNotRecognizeSelector: _cmd];
    
    return NO;
}



- (BOOL) isBuiltinDisplay
{
    [self doesNotRecognizeSelector: _cmd];
    
    return NO;
}



- (BOOL) isCaptured
{
    [self doesNotRecognizeSelector: _cmd];
    
    return NO;
}



- (BOOL) hasFSAA
{
    [self doesNotRecognizeSelector: _cmd];
    
    return NO;
}



- (float) gamma
{
    [self doesNotRecognizeSelector: _cmd];
    
    return 0.0f;
}



- (void) setGamma: (float) gamma update: (BOOL) doUpdate
{
    FD_UNUSED (gamma, doUpdate);

    [self doesNotRecognizeSelector: _cmd];
}



- (void) fadeOutDisplay: (float) seconds
{
    FD_UNUSED (seconds);
    
    [self doesNotRecognizeSelector: _cmd];
}



- (void) fadeInDisplay: (float) seconds
{
    FD_UNUSED (seconds);
    
    [self doesNotRecognizeSelector: _cmd];
}



- (void) captureDisplay
{
    [self doesNotRecognizeSelector: _cmd];
}



- (void) releaseDisplay
{
    [self doesNotRecognizeSelector: _cmd];
}

@end


