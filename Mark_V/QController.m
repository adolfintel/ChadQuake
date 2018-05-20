
//
// "QController.m" - the controller.
//
// Written by:  Axel 'awe' Wefers           [mailto:awe@fruitz-of-dojo.de].
//              ©2001-2012 Fruitz Of Dojo   [http://www.fruitz-of-dojo.de].
//
// Quakeª is copyrighted by id software     [http://www.idsoftware.com].
//


#import <Cocoa/Cocoa.h>
#import <fcntl.h>
#import <unistd.h>

#import "quakedef.h"
#import "macquake.h"
#import "QController.h"



//#define DEBUG_EVENTS



@implementation QController





+ (void) initialize
{
#if 0
    // Baker: Use this to wipe the preferences
    NSString *appDomain = [[NSBundle mainBundle] bundleIdentifier];
    [[NSUserDefaults standardUserDefaults] removePersistentDomainForName:appDomain];
#endif
    
    FDPreferences*  prefs       = [FDPreferences sharedPrefs];

    [prefs registerDefaultObject: QUAKE_PREFS_VALUE_SUPPRESS_STARTUP_PROMPT_CMDLINE forKey: QUAKE_PREFS_KEY_SUPPRESS_STARTUP_PROMPT_CMDLINE];
    [prefs registerDefaultObject: QUAKE_PREFS_VALUE_CMDLINE forKey: QUAKE_PREFS_KEY_CMDLINE];
    
    Core_Init (ENGINE_FAMILY_NAME, &qfunction_set, NULL); // Handing off no window pointer for OS X at this time

}


- (id) init
{
    self = [super init];
    return self;
}


- (IBAction)cmdCopyScreenshot:(id)sender {
#pragma message ("Cbuf object OS X fixme")
#if 0
    if (mQuakeRunning)
        Cbuf->AddTextLine (Cbuf, "screenshot copy");
#endif
}

- (IBAction)cmdCopyConsole:(id)sender {
#pragma message ("Cbuf object OS X fixme")
#if 0    
	if (mQuakeRunning)
       Cbuf->AddTextLine (Cbuf, "copy");
#endif
}

- (IBAction)cmdCopyEnts:(id)sender {
#pragma message ("Cbuf object OS X fixme")
#if 0
    if (mQuakeRunning)
        Cbuf->AddTextLine (Cbuf, "copy ents");
#endif
}

-(BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    SEL theAction = [menuItem action];
    
    if (theAction == sel_registerName("cmdCopyScreenshot:")) return mQuakeRunning;
    if (theAction == sel_registerName("cmdCopyConsole:")) return mQuakeRunning;
    if (theAction == sel_registerName("cmdCopyEnts:")) return mQuakeRunning;
    if (theAction == sel_registerName("cmdHideWinow:")) return mQuakeRunning;

    
    return YES;
}

-(void) UpdateStartupDialogMenuItem
{
    if ([[FDPreferences sharedPrefs] boolForKey:QUAKE_PREFS_KEY_SUPPRESS_STARTUP_PROMPT_CMDLINE])
    {
        [mShowCommandLineAtStartup setState:0];
        [mShowCommandLineAtStartup setTitle: @"Show Prompt Dialog On Start (Disabled)"];
    } else
    {
        [mShowCommandLineAtStartup setState:1];
        [mShowCommandLineAtStartup setTitle: @"Show Prompt Dialog On Start"];
    }
}



- (void) dealloc
{
    [mSettingsWindow release];
    
    [super dealloc];
}




- (void) applicationDidFinishLaunching: (NSNotification*) notification
{
    FD_UNUSED (notification);
    
    FD_DURING
    {
        [mShowCommandLineAtStartup setEnabled: YES];
        [self UpdateStartupDialogMenuItem];

        [FDHIDManager checkForIncompatibleDevices];
        
        [NSTimer scheduledTimerWithTimeInterval: 0.5f
                                         target: self
                                       selector: @selector (setupDialog:)
                                       userInfo: nil
                                        repeats: NO];        
    }
    FD_HANDLER;
}

- (NSApplicationTerminateReply) applicationShouldTerminate: (NSApplication*) sender
{
    FD_UNUSED (sender);
    
    NSApplicationTerminateReply replier = NSTerminateNow;
    
    if ([self getQuakeRunning] == YES)
    {
        if ([NSApp isHidden] == YES || [NSApp isActive] == NO)
        {
            [NSApp activateIgnoringOtherApps: YES];
        }
        
        if (vid.screen.type == MODE_WINDOWED && sysplat.gVidWindow != NULL)
        {
            if ([sysplat.gVidWindow isMiniaturized] == YES)
            {
                [sysplat.gVidWindow deminiaturize: NULL];
            }
            
            [sysplat.gVidWindow orderFront: NULL];
        }
        
        System_Quit (); // Baker: This doesn't return
        
        replier = NSTerminateCancel; // Offers the opportunity to bail on the quit, but we don't use it

    }
    
    return replier;
}



- (void)applicationWillTerminate: (NSNotification*) notification
{
    FD_UNUSED (notification);
    
    [mSettingsWindow release];
    mSettingsWindow = nil;
    
    [[FDPreferences sharedPrefs] synchronize];

    if ([self getQuakeRunning])
    {
       System_Quit (); // Baker: This doesn't return
    }
    
}

#if 0 // Baker: It isn't working, would need to setup notification
- (void)windowDidDeminiaturize:(NSNotification *)notification
{
    if ([self getQuakeRunning] == NO)
        return;
    
    VID_AppActivate(true, false, false);
    
#ifdef DEBUG_EVENTS
    Con_PrintLinef ("unmini");
#endif
}

- (void)windowWillMiniaturize:(NSNotification *)notification
{
    if ([self getQuakeRunning] == NO)
        return;
    
    VID_AppActivate(false, true, false);
    
#ifdef DEBUG_EVENTS
    Con_PrintLinef ("Mini");
#endif
}
#endif // Baker: It isn't working

- (void)applicationWillBecomeActive:(NSApplication *)app
{
    FD_UNUSED (notification);
    
    if ([self getQuakeRunning] == NO)
        return;

    VID_AppActivate(true, false, false);
#ifdef DEBUG_EVENTS
    Con_PrintLinef ("Will become active");
#endif
}

- (void) applicationWillResignActive: (NSNotification*) notification
{
    FD_UNUSED (notification);
    
    if ([self getQuakeRunning] == NO)
        return;
    
    VID_AppActivate(false, false, false);
#ifdef DEBUG_EVENTS
    Con_PrintLinef ("Will resign active");
#endif

}



- (void) applicationWillHide: (NSNotification *) notification
{
    FD_UNUSED (notification);
    
    if ([self getQuakeRunning] == NO)
        return;
    
    VID_AppActivate(false, false, true);

    if (mFrameTimer != nil)
    {
        [mFrameTimer invalidate];
        mFrameTimer = nil;
    }
#ifdef DEBUG_EVENTS
    Con_PrintLinef ("Hide");
#endif
}



- (void) applicationWillUnhide: (NSNotification *) notification
{
    FD_UNUSED (notification);
    
    if ([self getQuakeRunning] == NO)
        return;
    
    VID_AppActivate(true, false, false);

    [self installFrameTimer];
#ifdef DEBUG_EVENTS
    Con_PrintLinef ("Unhide");
#endif
}


- (void) setQuakeRunning: (BOOL) theState
{
    mQuakeRunning = theState;
}


- (BOOL) getQuakeRunning
{
    return mQuakeRunning;
}






- (IBAction) visitFOD: (id) sender
{
    FD_UNUSED (sender);
    
    [[NSWorkspace sharedWorkspace] openURL: [NSURL URLWithString: @ENGINE_URL]];
}

- (IBAction)cmdOpenFolder:(id)sender
{
    Folder_Open_Highlight_Binary ();
}

- (IBAction)cmdPakFileUnpack:(id)sender
{
    const char* selected_pak = File_Dialog_Open_Type("Select Pak File To Obtain Contents", NULL, "pak");
    
    if (selected_pak[0])
    {
//      alert (selected_pak);
        char extractpath[MAX_OSPATH];
        int result;
        
        c_strlcpy (extractpath, selected_pak);
        File_URL_Edit_Remove_Extension(extractpath);
        c_strlcat (extractpath, "_contents");
        
        result = Pak_Unzip (selected_pak, extractpath);
        if (result)
        {
            msgbox("Pak Successful Unpack", "Pak " QUOTED_S " extracted to " QUOTED_S " with %d files unpacked", File_URL_SkipPath(selected_pak), extractpath, result);
            Folder_Open(extractpath);
        }
        else msgbox ("Pak Unpack Failed", "Pak " QUOTED_S " could not be extracted to " QUOTED_S, File_URL_SkipPath(selected_pak), extractpath);
    }
}

- (IBAction)cmdPakFileList:(id)sender
{
    const char* selected_pak = File_Dialog_Open_Type("Select Pak File To Obtain Contents", NULL, "pak");
    
    if (selected_pak[0])
    {
        char pakurl[MAX_OSPATH];
        c_strlcpy (pakurl, selected_pak);
        
        clist_t *list = Pak_List_Alloc(pakurl);
        clist_t *cur;
        
        int count;
        
        const char *stringa = NULL;
        txtcat (&stringa, "Pak Listing of %s" NEWLINE "----" NEWLINE, pakurl);
        
        for (cur = list, count = 0; cur; cur = cur->next, count++)
        {
            txtcat (&stringa, "%s" NEWLINE, cur->name);
        }
        
        txtcat (&stringa, "----\nFiles = %d" NEWLINE, count);
        
        Clipboard_Set_Text(stringa);
        
        // Free allocations
        List_Free(&list);
        stringa = core_free (stringa);

        msgbox("Pak Listing Placed On Clipboard", "Pak listing of " QUOTED_S " placed on clipboard", File_URL_SkipPath(pakurl));
    }
}



- (IBAction)cmdPakFileMake:(id)sender {
    const char* selected_directory = File_Dialog_Open_Directory ("Select Folder To Pack Into Pakfile", NULL);
    
    if (selected_directory[0])
    {
        char pakpath[MAX_OSPATH];
        char pakurl[MAX_OSPATH];
        int result;
        // Validate source
        c_strlcpy (pakpath, selected_directory);
        c_strlcpy (pakurl, selected_directory);
        
        File_URL_Edit_Force_Extension(pakurl, ".pak", sizeof(pakurl));
        
        result = Pak_Zip_Folder (pakurl, pakpath);
        if (result)
        {
            msgbox("Pak Created", "Pak " QUOTED_S " created with %d files", File_URL_SkipPath(pakurl), result);
            Folder_Open_Highlight(pakurl);
        } else msgbox ("Pak Creation Failed", "Unable to create pak from " QUOTED_S, pakpath);
    }
}





- (IBAction)cmdDialogAtStartup:(id)sender {
    BOOL bshow =  [[FDPreferences sharedPrefs] boolForKey:QUAKE_PREFS_KEY_SUPPRESS_STARTUP_PROMPT_CMDLINE];
    
    if (bshow)
    [[FDPreferences sharedPrefs] setBool:NO forKey: QUAKE_PREFS_KEY_SUPPRESS_STARTUP_PROMPT_CMDLINE];
    else [[FDPreferences sharedPrefs] setBool:YES forKey: QUAKE_PREFS_KEY_SUPPRESS_STARTUP_PROMPT_CMDLINE];
    
    [self UpdateStartupDialogMenuItem];
    [[FDPreferences sharedPrefs] synchronize]; // Baker: Paranoid?
}

- (IBAction)cmdHideWinow:(id)sender {
    if (mQuakeRunning && vid.screen.type == MODE_FULLSCREEN)
        return;

    [NSApp hide:nil];
}

- (void) setupDialog: (NSTimer*) timer
{
    FD_UNUSED (timer);
    
    FD_DURING
    {
        BOOL bsuppress =  [[FDPreferences sharedPrefs] boolForKey:QUAKE_PREFS_KEY_SUPPRESS_STARTUP_PROMPT_CMDLINE];
        if (bsuppress == NO)
        {
            mSettingsWindow = [[QSettingsWindow alloc] init];
            
            [mSettingsWindow setNewGameAction: @selector (newGame:) target: self];
            [mSettingsWindow showWindow: self];
        }
        else
        {

            [self newGame: nil];
        }
    }
    FD_HANDLER;
}



- (void) newGame: (id) sender
{
    FD_UNUSED (sender);
    
    FD_DURING
    {
        mSettingsWindow = nil;
 
#if 0
        [QMediaScan scanFolder: [self mediaFolder] observer: self selector: @selector (initGame:)];
#endif
    }
    FD_HANDLER;
    
    {
        FD_UNUSED (notification);
		{
            NSString*   _cmdline = [[FDPreferences sharedPrefs] stringForKey:QUAKE_PREFS_KEY_CMDLINE];
            const char  *lpCmdLineText = [_cmdline cStringUsingEncoding:NSASCIIStringEncoding];
			
			char		cmdline[SYSTEM_STRING_SIZE_1024];
			uintptr_t 	fakemainwindow = 0;
			double 		oldtime;
			
            // prepare host init:
            [[FDDebug sharedDebug] setLogHandler: &Con_Printf];
            [[FDDebug sharedDebug] setErrorHandler: &System_Error];
            signal (SIGFPE, SIG_IGN); // Baker: Ignore floating point exceptions
			
			c_strlcpy (cmdline, lpCmdLineText);
#if 0//fdef _DEBUG
            c_strlcat (cmdline, " -basedir /Users/iOS/Desktop/Quake -window");
#endif
			
			
			Main_Central (cmdline, &fakemainwindow, false /* we perform loop ourselves */);

            if (isDedicated)
                System_Error ("Please use Mark V dedicated server build which uses a Terminal interface");
			
			oldtime = System_DoubleTime ();
			
            [self setQuakeRunning: YES];
            [NSApp setServicesProvider: self];      
            [self installFrameTimer];
        }
    }
}




- (void) installFrameTimer
{
    if (mFrameTimer == nil)
    {
        mFrameTimer = [NSTimer scheduledTimerWithTimeInterval: 0.0003f
                                                       target: self
                                                     selector: @selector (doFrame:)
                                                     userInfo: nil
                                                      repeats: YES];
        
        if (mFrameTimer != nil)
        {
            [[NSRunLoop currentRunLoop] addTimer: mFrameTimer forMode: NSEventTrackingRunLoopMode];
        }
        else
        {
            System_Error ("Failed to install the renderer loop!");
        }
    }
}



- (void) doFrame: (NSTimer*) timer
{
    FD_UNUSED (timer);

    // Baker: A hidden app is going to lose connection to server
    // if host frame isn't running.  Likewise, any connected clients will
    // lose their connection.
    
    if ([NSApp isHidden] == NO)
    {

        double  curTime = System_DoubleTime();
        double  deltaTime = curTime - mPrevFrameTime;
        
        if (isDedicated)
        {            
            if (deltaTime < sys_ticrate.value)
            {
                System_Sleep_Milliseconds (QUAKE_DEDICATED_SLEEP_TIME_MILLISECONDS_1);
                return;
            }
            
            deltaTime = sys_ticrate.value;
        }
        
        if (deltaTime > sys_ticrate.value * 2)
        {
            mPrevFrameTime = curTime;
        }
        else
        {
            mPrevFrameTime += deltaTime;
        }

        Host_Frame (deltaTime);
    }
}




// Baker: Close window to exit
-(BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app
{
    return YES;
}

@end
