
//
// "cd_osx.m" - MacOS X audio CD driver.
//
// Written by:  Axel 'awe' Wefers           [mailto:awe@fruitz-of-dojo.de].
//              ©2001-2012 Fruitz Of Dojo   [http://www.fruitz-of-dojo.de].
//
// Quakeª is copyrighted by id software     [http://www.idsoftware.com].
//
// Version History:
// v1.2.0: Rewritten. Uses now AudioGraph/AudioUnit for playback.
// v1.0.9: Rewritten. Uses now QuickTime for playback. Added support for MP3 and MP4 [AAC] playback.
// v1.0.3: Fixed an issue with requesting a track number greater than the max number.
// v1.0.1: Added "cdda" as extension for detection of audio-tracks [required by MacOS X v10.1 or later]
// v1.0.0: Initial release.
//


#import "quakedef.h"
#import "QController.h"

#import <FDFramework/FDFramework.h>

#import <sys/mount.h>



static FDAudioFile*     sCDAudio            = nil;
static NSString*        sCDAudioMountPath   = nil;
static NSMutableArray*  sCDAudioTrackList   = nil;
static NSUInteger       sCDAudioTrack       = 0;

cbool using_cd_tracks = -1;
cbool command_line_disabled;








void CDAudio_AddTracks2List (NSString* mountPath, NSArray* extensions, NSConditionLock* stopConditionLock)
{
    NSFileManager*          fileManager = [NSFileManager defaultManager];
    NSDirectoryEnumerator*  dirEnum     = [fileManager enumeratorAtPath: mountPath];
        
    if (dirEnum != nil)
    {
        NSUInteger  extensionCount  = [extensions count];
        NSString*   filePath        = nil;

        Con_PrintLinef ("%s", "Scanning for audio tracks. Be patient!");
        
        while ((filePath = [dirEnum nextObject]))
        {
            if (stopConditionLock != nil)
            {
                [stopConditionLock lock];
                
                const BOOL doStop = ([stopConditionLock condition] != 0);
                
                [stopConditionLock unlock];
                
                if (doStop == YES)
                {
                    break;
                }
            }

            for (NSUInteger i = 0; i < extensionCount; ++i)
            {
                if ([[filePath pathExtension] isEqualToString: [extensions objectAtIndex: i]])
                {
                    NSString*   fullPath = [mountPath stringByAppendingPathComponent: filePath];
                    alert ("%d: Adding %s to tracks list", (int)i, [fullPath cStringUsingEncoding: NSASCIIStringEncoding]);
                    [sCDAudioTrackList addObject: [NSURL fileURLWithPath: fullPath]];
                }
            }
        }
    }
        
    sCDAudioMountPath = [[NSString alloc] initWithString: mountPath];
}



BOOL CDAudio_ScanForMedia (NSString* mediaFolder, NSConditionLock* stopConditionLock)
{
    NSAutoreleasePool*  pool = [[NSAutoreleasePool alloc] init];
    
    [sCDAudioMountPath release];
    [sCDAudioTrackList release];
    
    sCDAudioMountPath   = nil;
    sCDAudioTrackList   = [[NSMutableArray alloc] init];
    sCDAudioTrack       = 0;
    
    // Get the current MP3 listing or retrieve the TOC of the AudioCD:
    if (mediaFolder != nil)
    {
        // Baker: This is using MP3 files and not CD tracks
        CDAudio_AddTracks2List (mediaFolder, [NSArray arrayWithObjects: @"mp3", @"mp4", @"m4a", nil], stopConditionLock);
        using_cd_tracks = 0;
    }
    else
    {
        struct statfs*  mountList = 0;
        UInt32          mountCount = getmntinfo (&mountList, MNT_NOWAIT);
        
        while (mountCount--)
        {
            // is the device read only?
            if ((mountList[mountCount].f_flags & MNT_RDONLY) != MNT_RDONLY)
            {
                continue;
            }
            
            // is the device local?
            if ((mountList[mountCount].f_flags & MNT_LOCAL) != MNT_LOCAL)
            {
                continue;
            }
            
            // is the device "cdda"?
            if (strcmp (mountList[mountCount].f_fstypename, "cddafs"))
            {
                continue;
            }
            
            // is the device a directory?
            if (strrchr (mountList[mountCount].f_mntonname, '/') == NULL)
            {
                continue;
            }
            
            mediaFolder = [NSString stringWithCString: mountList[mountCount].f_mntonname encoding: NSASCIIStringEncoding];
            
            CDAudio_AddTracks2List (mediaFolder, [NSArray arrayWithObjects: @"aiff", @"cdda", nil], stopConditionLock);
            
            break;
        }
        using_cd_tracks = 0;
    }

    [pool release];
    
    if ([sCDAudioTrackList count] == 0)
    {
        [sCDAudioTrackList release];
        sCDAudioTrackList = nil;
        Con_PrintLinef ("%s", "CDAudio: No audio tracks found!");
        using_cd_tracks = -2; // There aren't any of any kind.
        return NO;
    }
    
    return YES;
}

//
//
//
//
//
//
//
//
//
//
//
//

void CDAudio_Play (byte track, cbool looping)
{

    if (command_line_disabled)
        return;

    if (!external_music.value)
        return;

    CDAudio_Stop();
    
    const char *track_file = va("music/track%02d.mp3", track);
    const char *absolute_filename = COM_FindFile_NoPak (track_file);

    if (!absolute_filename)
    {
        Con_PrintLinef ("Track: %s not found", track_file);
        return;
    } else Con_PrintLinef ("Current music track: %s", track_file);
    
    
    if ([sCDAudio startFile: absolute_filename loop: (looping != false)])
    {
        sCDAudioTrack = track;
    }
    else
    {
        Con_PrintLinef ("%s", "CDAudio: Failed to start playback!");
    }

#if 0
    if (sCDAudio != nil)
    {

        if ((track <= 0) || (track > [sCDAudioTrackList count]))
        {
            track = 1;
        }
        
        if (sCDAudioTrackList != nil)
        {
            if ([sCDAudio startFile: [sCDAudioTrackList objectAtIndex: track - 1] loop: (loop != false)])
            {
                sCDAudioTrack = track;
            }
            else
            {
                Con_PrintLinef ("%s", "CDAudio: Failed to start playback!");
            }
        }
    }
#endif

}

//
//
//
//

void CDAudio_Stop (void)
{
    if (sCDAudio != nil)
    {
        [sCDAudio stop];
    }
}


void CDAudio_Pause (void)
{
    if (sCDAudio != nil)
    {
        [sCDAudio pause];
    }
}


void CDAudio_Resume (void)
{
    if (sCDAudio != nil)
    {
        [sCDAudio resume];
    }
}




void CD_f (lparse_t* line)
{
    const char *command;

    if (line->count < 2)
    {
        Con_PrintLinef ("Usage: %s {play|stop|on|off|info|pause|resume} [filename]", line->args[0]);
        Con_PrintLinef ("  Note: music files should be in gamedir/music");
        Con_PrintLinef ("Example: quake/id1/music/track04.mp3 ");
        Con_PrintLine ();
        Con_PrintLinef ("%s is set to " QUOTED_S " and if set to 0, will prohibit music.", external_music.name, external_music.string);

        return;
    }
    
    command = line->args[1]; //Cmd_Argv (1);
    
    // turn CD playback on:
    if (strcasecmp (command, "on") == 0)
    {
        if (sCDAudio == nil)
        {
            sCDAudio = [[FDAudioFile alloc] initWithMixer: [FDAudioMixer sharedAudioMixer]];
            
            CDAudio_Play (1, 0);
        }

        return;
    }
    
    if (sCDAudio == nil)
    {
        return;
    }
    
    // turn CD playback off:
    if (strcasecmp (command, "off") == 0)
    {
        [sCDAudio release];
        sCDAudio = nil;
        
        return;
    }
    
#if 0
    // reset the current CD:
    if (strcasecmp (command, "reset") == 0)
    {
        CDAudio_Stop ();
        
        if (CDAudio_ScanForMedia ([[NSApp delegate] mediaFolder], nil))
        {
            NSUInteger  numTracks = 0;
            
            if (sCDAudioTrackList != nil)
            {
                numTracks = [sCDAudioTrackList count];
            }
            
            if ([[NSApp delegate] mediaFolder] == nil)
            {
                Con_Printf ("%s", "CD");
            }
            else
            {
                Con_Printf ("%s", "Audio files");
            }
            Con_PrintLinef (" found. %d tracks.", (int)numTracks);
        }
        
        return;
    }

    // just for compatibility:
    if (strcasecmp (command, "remap") == 0)
    {
        return;
    }

    
    // the following commands require a valid track array, so build it, if not present:
    if (sCDAudioTrackList == nil)
    {
        if (!CDAudio_ScanForMedia ([[NSApp delegate] mediaFolder], nil))
        {
            return;
        }
    }
#endif

    // play the selected track:
    if (strcasecmp (command, "play") == 0)
    {
        CDAudio_Play ((byte)atoi (line->args[2]), false);
        return;
    }
    
    // loop the selected track:
    if (strcasecmp (command, "loop") == 0)
    {
        CDAudio_Play ((byte)atoi (line->args[2]), true);
        return;
    }
    
    // stop the current track:
    if (strcasecmp (command, "stop") == 0)
    {
        CDAudio_Stop ();
        return;
    }
    
    // pause the current track:
    if (strcasecmp (command, "pause") == 0)
    {
        CDAudio_Pause ();
        return;
    }
    
    // resume the current track:
    if (strcasecmp (command, "resume") == 0)
    {
        CDAudio_Resume ();
        return;
    }
#if 0
    // eject the CD:
    if (strcasecmp (command, "eject") == 0)
    {
        if (([[NSApp delegate] mediaFolder] == nil) && (sCDAudioMountPath != nil))
        {
            NSURL*      url = [NSURL fileURLWithPath: sCDAudioMountPath];
            NSError*    err = nil;
            
            [sCDAudio stop];
            
            if (![[NSWorkspace sharedWorkspace] unmountAndEjectDeviceAtURL: url error: &err])
            {
                Con_PrintLinef ("CDAudio: Failed to eject media!");
            }
        }
        else
        {
            Con_PrintLinef ("CDAudio: No media mounted!");
        }

        return;
    }
    
    // output CD info:
    if (strcasecmp(command, "info") == 0)
    {
        if (sCDAudioTrackList == nil)
        {
            Con_PrintLinef ("%s", "CDAudio: No audio tracks found!");
        }
        else
        {
            const NSUInteger    numTracks = [sCDAudioTrackList count];
            const char *        mountPath = [sCDAudioMountPath cStringUsingEncoding: NSASCIIStringEncoding];
            
            if ([sCDAudio isPlaying] == YES)
            {
                Con_PrintLinef ("Playing track %d of %d (" QUOTED_S ").", (int)sCDAudioTrack, (int)numTracks, mountPath);
            }
            else
            {
                Con_PrintLinef ("Not playing. Tracks: %d (" QUOTED_S ").", (int)numTracks, mountPath);
            }
 
            Con_PrintLinef ("CD volume is: %.2f.", bgmvolume.value);
        }
        
        return;
    }
#endif
    
}

//
//
//
//
//
void CDAudio_Update (void)
{
//  static float old_effective_volume = -1;
//  effective_volume = 9999;
    if (sCDAudio != nil)
    {
//      float effective_volume = bgmvolume.value * sfxvolume.value;
//      if (effective_volume == old_effective_volume)
//          return;

//      old_effective_volume = effective_volume;

        [sCDAudio setVolume: bgmvolume.value];
        
        if (([sCDAudio loops] == NO) && ([sCDAudio isFinished] == YES))
        {
            CDAudio_Stop ();
            //CDAudio_Play (sCDAudioTrack + 1, 0);
        }
    }
}

cbool disabled_musics = false;
void external_music_toggle_f (cvar_t *var)
{
    if (command_line_disabled)
        return; // Command line disabled

    if (var->value)
    {
        if (cls.state == ca_connected)
        {
            // First stop the music, previous value might not have been zero
 // Now incorporated into play           CDAudio_Stop ();
            // Try to start the music
            CDAudio_Play ((byte)cl.cdtrack, true);
        }
    }
    else CDAudio_Stop ();
}

void Set_Music_f (lparse_t *unused)
{
	
	
}

// Baker: The return value is not used, making it void
void CDAudio_Init (void)
{
    int success = 0;
    
    if (COM_CheckParm("-nosound"))
        command_line_disabled = true; // No sound --> no track music either
    
    if (COM_CheckParm("-nocdaudio"))
        command_line_disabled = true;
    
    if (command_line_disabled)
        Con_PrintLinef ("CD disabled by command line");
//    else Cmd_AddCommand ("cd", CD_f);
    
	Cmd_AddCommands (CDAudio_Init);
//    Cvar_RegisterVariableWithCallback (&external_music, external_music_toggle_f);
    
    sCDAudioTrack       = 0;
    sCDAudio            = [[FDAudioFile alloc] initWithMixer: [FDAudioMixer sharedAudioMixer]];
    
    [[FDAudioMixer sharedAudioMixer] start];
    if (!command_line_disabled)
    {
        if (sCDAudio != nil)
        {
            if (sCDAudioTrackList == nil)
            {
                Con_SafePrintLinef ("%s", "CD driver: no audio tracks!");
            }
            
            Con_SafePrintLinef ("%s", "CD Audio Initialized");
            
            success = 1;
        }
        else
        {
            Con_SafePrintLinef ("%s", "Failed to initialize CD driver!");
            
        }
    }
}

void CDAudio_Shutdown (void)
{
    [sCDAudio release];
    [sCDAudioMountPath release];
    [sCDAudioTrackList release];
    
    sCDAudio            = nil;
    sCDAudioMountPath   = nil;
    sCDAudioTrackList   = nil;
}