
//
// "QAboutPanel.h"
//
// Written by:	Axel 'awe' Wefers			[mailto:awe@fruitz-of-dojo.de].
//				©2001-2012 Fruitz Of Dojo 	[http://www.fruitz-of-dojo.de].
//


#import "QSettingsPanel.h"
#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>



@interface QAboutPanel : QSettingsPanel
{
@private
	IBOutlet NSTextFieldCell *mCommandLine;
}

- (NSString*) nibName;
- (void) awakeFromNib;

- (NSString*) toolbarIdentifier;
- (NSToolbarItem*) toolbarItem;

- (void) synchronize;
@end


