
//
// "FDPreferences.h"
//
// Written by:	Axel 'awe' Wefers			[mailto:awe@fruitz-of-dojo.de].
//				©2001-2012 Fruitz Of Dojo 	[http://www.fruitz-of-dojo.de].
//


#import <Cocoa/Cocoa.h>



@interface FDPreferences : NSObject
{
}

+ (FDPreferences*) sharedPrefs;

- (void) registerDefaults: (NSDictionary*) dictionary;
- (void) registerDefaultObject: (NSObject*) object forKey: (NSString*) key;

- (void) setObject: (id) object forKey: (NSString*) key;

- (void) setBool: (BOOL) value forKey: (NSString*) key;
- (BOOL) boolForKey: (NSString*) key;
- (NSInteger) integerForKey: (NSString*) key;
- (NSString*) stringForKey: (NSString*) key;
- (NSArray*) arrayForKey: (NSString*) key;

- (BOOL) synchronize;

@end


