
//
// "QAboutPanel.m"
//
// Written by:  Axel 'awe' Wefers           [mailto:awe@fruitz-of-dojo.de].
//              ©2001-2012 Fruitz Of Dojo  [http://www.fruitz-of-dojo.de].
//


#import "QAboutPanel.h"
#import "QController.h"

#import <FDFramework/FDFramework.h>

#include "quakedef.h" 

@implementation QAboutPanel

- (NSString *) nibName
{
    return @"AboutPanel";
}

- (void) awakeFromNib
{
    [mCommandLine setStringValue: [[FDPreferences sharedPrefs] stringForKey:QUAKE_PREFS_KEY_CMDLINE]];
    [self setTitle: @"Start Up"];
}

- (NSString*) toolbarIdentifier
{
    return @"Quake About ToolbarItem";
}

- (NSToolbarItem*) toolbarItem
{
    NSToolbarItem* item = [super toolbarItem];
    
    [item setLabel: @"About"];
    [item setPaletteLabel: @"Optional Command Line"];
    [item setToolTip: @"Optional Command Line"];
    [item setImage: [NSImage imageNamed: @"About.icns"]];
    
    return item;
}


- (void) synchronize
{
    [[FDPreferences sharedPrefs] setObject: [mCommandLine stringValue] forKey: QUAKE_PREFS_KEY_CMDLINE];
}

@end

