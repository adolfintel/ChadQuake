
//
// "FDAudioMixer.h" - Audio mixer.
//
// Written by:	Axel 'awe' Wefers			[mailto:awe@fruitz-of-dojo.de].
//				©2001-2012 Fruitz Of Dojo 	[http://www.fruitz-of-dojo.de].
//


#import <Cocoa/Cocoa.h>



@interface FDAudioMixer : NSObject
{
}

+ (FDAudioMixer*) sharedAudioMixer;

- (id) init;

- (void) start;
- (void) stop;

@end


