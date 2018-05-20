
//
// "FDAudioFile.h" - Sound file playback.
//
// Written by:	Axel 'awe' Wefers			[mailto:awe@fruitz-of-dojo.de].
//				©2001-2012 Fruitz Of Dojo 	[http://www.fruitz-of-dojo.de].
//


#import "FDAudioMixer.h"

#import <Cocoa/Cocoa.h>



@interface FDAudioFile : NSObject
{
}

- (id) initWithMixer: (FDAudioMixer*) mixer;

- (void) setVolume: (float) volume;
- (float) volume;

- (BOOL) startFile: (const char*) path loop: (BOOL) loop;
- (BOOL) stop;

- (void) pause;
- (void) resume;

- (BOOL) isPlaying;
- (BOOL) isFinished;
- (BOOL) loops;

@end


