
//
// "QSettingsPanel.h"
//
// Written by:	Axel 'awe' Wefers			[mailto:awe@fruitz-of-dojo.de].
//				©2001-2012 Fruitz Of Dojo 	[http://www.fruitz-of-dojo.de].
//


#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>



@interface QSettingsPanel : NSViewController
{
    id      mDelegate;
}

- (NSString*) toolbarIdentifier;
- (NSToolbarItem*) toolbarItem;

- (void) setDelegate: (id) delegate;
- (id) delegate;

- (void) synchronize;

@end


