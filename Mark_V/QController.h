
//
// "QController.h"
//
// Written by:	Axel 'awe' Wefers			[mailto:awe@fruitz-of-dojo.de].
//				©2001-2012 Fruitz Of Dojo 	[http://www.fruitz-of-dojo.de].
//
// Quakeª is copyrighted by id software		[http://www.idsoftware.com].
//


#import "QSettingsWindow.h"

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>




#define QUAKE_PREFS_KEY_AUDIO_PATH          				@"Quake Audio Files Path"
#define QUAKE_PREFS_KEY_SUPPRESS_STARTUP_PROMPT_CMDLINE		@"Quake Prompt For Command-Line" // Bool, Yes or No
#define QUAKE_PREFS_KEY_CMDLINE								@"Quake Command-Line Arguments"



#define QUAKE_PREFS_VALUE_AUDIO_PATH        @""
#define QUAKE_PREFS_VALUE_SUPPRESS_STARTUP_PROMPT_CMDLINE   @"0"
#define QUAKE_PREFS_VALUE_CMDLINE							@""



@interface QController : NSObject <NSMenuDelegate>
{
    QSettingsWindow*                mSettingsWindow;

    NSTimer*                        mFrameTimer;
    double							mPrevFrameTime;
    BOOL                            mQuakeRunning;
	IBOutlet NSMenuItem *mShowCommandLineAtStartup;

	IBOutlet NSMenuItem *mCopyScreenshot;

	IBOutlet NSMenuItem *mCopyConsole;
	IBOutlet NSMenuItem *mCopyEnts;


	NSMenuItem *cmdStartupDialog;
	NSMenuItem *cmdPakFileList;
}

-(BOOL)validateMenuItem:(NSMenuItem *)menuItem;
+ (void) initialize;
- (id) init;
- (void) dealloc;
- (NSApplicationTerminateReply) applicationShouldTerminate: (NSApplication*) sender;
- (void) applicationDidFinishLaunching: (NSNotification*) notification;
- (void)applicationWillBecomeActive:(NSApplication *)app;
- (void)applicationWillTerminate: (NSNotification*) notification;
- (void)applicationWillResignActive:(NSApplication *)app;
//- (void) applicationDidResignActive: (NSNotification*) notification;
- (void) applicationWillHide: (NSNotification *) notification;
- (void) applicationWillUnhide: (NSNotification *) notification;
#if 1 // Baker
- (BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app;
#endif
- (void) setQuakeRunning: (BOOL) state;
- (BOOL) getQuakeRunning;

//- (NSString *) mediaFolder;

- (IBAction)cmdCopyScreenshot:(id)sender;
- (IBAction)cmdCopyConsole:(id)sender;
- (IBAction)cmdCopyEnts:(id)sender;
- (IBAction)cmdOpenFolder:(id)sender;

- (IBAction)cmdPakFileMake:(id)sender;
- (IBAction)cmdPakFileUnpack:(id)sender;
- (IBAction)cmdPakFileList:(id)sender;


- (IBAction)cmdDialogAtStartup:(id)sender;

- (IBAction)cmdHideWinow:(id)sender;

- (void) setupDialog: (NSTimer*) timer;
- (void) newGame: (id) sender;
- (void) installFrameTimer;
- (void) doFrame: (NSTimer*) timer;

@end



