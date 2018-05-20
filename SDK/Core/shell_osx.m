/*
Copyright (C) 2012-2016 Baker

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// shell_osx.m -- Mac graphical interaction layer
// Platform unavoidable and SDL doesn't provide.
// Bundle - Storage Paths - Images for clipboard - Dialogs
// There are dialogs in here and folder explorers.  A console app wouldn't normal use them, but no reason to prevent them.

#include "environment.h"
#ifdef PLATFORM_OSX

#define CORE_LOCAL
#include "core.h"

///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: URL QUERY
///////////////////////////////////////////////////////////////////////////////

// Baker: Function is unused currently
const char *File_Binary_URL (void)
{
    static char binary_url[MAX_OSPATH];
    if (!binary_url[0]) {
#ifndef _CONSOLE
        NSString *_basePath = [[[NSBundle mainBundle] bundlePath] stringByStandardizingPath];
        c_strlcpy (binary_url, TO_CSTRING(_basePath)  );
#else
        pid_t pid = getpid();
        int length;

        char linkname[MAX_OSPATH];
        c_snprintf1 (linkname, "/proc/%d/exe", pid);

        length = readlink (linkname, binary_url, sizeof(binary_url)-1);

        // In case of an error, leave the handling up to the caller
        if (length == -1 || length >= (int)sizeof(binary_url) )
            log_fatal ("Couldn't determine executable directory");

        binary_url[length] = 0;
#endif
    }
    return binary_url;
}





const char *_Shell_Folder_Caches_By_AppName (const char *appname)
{
    // UNUSED: appname   (appname isn't necessary for Mac)
    static char caches_folder_for_appname[MAX_OSPATH];

    if (!caches_folder_for_appname[0]) {
#ifndef _CONSOLE
        NSArray* paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
        NSString* mydir = [paths objectAtIndex:0];
        NSString *myAppID = [[NSBundle mainBundle] bundleIdentifier];
        NSString *cachesDirectory = [NSString stringWithFormat:@"%@/%@", mydir, myAppID];
        c_strlcpy (caches_folder_for_appname, TO_CSTRING(cachesDirectory)  );
#else
        c_strlcpy (caches_folder_for_appname, mainus.startup_cwd);
#endif
    }



    return caches_folder_for_appname;
}


///////////////////////////////////////////////////////////////////////////////
//  SHELL: Bundle
///////////////////////////////////////////////////////////////////////////////

#ifdef _CONSOLE
// This is a library vs. application breach and would prevent
// core from being a library.
    #include "bundle_bin2.h"
#else
    const char *sBundleFolder (void)
    {
        static char bundlefolder[MAX_OSPATH];

        if (!bundlefolder[0]) {
            NSString *mydir = [[NSBundle mainBundle] resourcePath];
            c_strlcpy (bundlefolder, TO_CSTRING(mydir)  );
        }

        return bundlefolder;
    }

    const char *sBundlePack (void)
    {
        static char buf[MAX_OSPATH];

        if (!buf[0])
            c_snprintf2 (buf, "%s/%s", sBundleFolder(), "bundle.pak");

        return buf;
    }
#endif // !_CONSOLE

const void *Shell_Data_From_Resource (size_t *mem_length, cbool *must_free)
{

#ifdef _CONSOLE // Terminal app only.
// bin2hex-ing it up!  If Mac console app!
    size_t num_bytes    = _binary_bundle_pak_size;
    const void *mem_    = _binary_bundle_pak;

    NOT_MISSING_ASSIGN (must_free, false); // No because we loaded directly from bin2h memory
#else
// Bundling it up!
// For Apple we will load from bundle data.

    const char *bundle_url = sBundlePack();

    size_t num_bytes;
    void *mem           = File_To_Memory_Alloc (bundle_url, &num_bytes);

    NOT_MISSING_ASSIGN (must_free, true); // Yes because we loaded from app bundle file.
#endif
    NOT_MISSING_ASSIGN (mem_length, num_bytes);
    return num_bytes ? mem : NULL;
}




///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: CLIPBOARD IMAGE OPERATIONS
///////////////////////////////////////////////////////////////////////////////




cbool _Shell_Clipboard_Set_Image_RGBA (const unsigned *rgba, int width, int height)
{
#ifdef _CONSOLE
    return false;
#else
    // TO PASTEBOARD:
    NSBitmapImageRep *image_rep = [[NSBitmapImageRep alloc]
                                   initWithBitmapDataPlanes:NULL
                                   pixelsWide:width
                                   pixelsHigh:height
                                   bitsPerSample:8
                                   samplesPerPixel:RGBA_4
                                   hasAlpha:YES
                                   isPlanar:NO
                                   colorSpaceName:NSCalibratedRGBColorSpace
                                   bitmapFormat:0
                                   bytesPerRow:width * RGBA_4
                                   bitsPerPixel:32];

    memcpy([image_rep bitmapData], rgba, width * height * RGBA_4);


    NSPasteboard *myPasteboard    = [NSPasteboard generalPasteboard];
    //  [myPasteboard clearContents];
    [myPasteboard declareTypes:[NSArray arrayWithObjects:NSTIFFPboardType, nil] owner:nil];
    [myPasteboard setData:[image_rep TIFFRepresentation] forType:NSTIFFPboardType];
    return true;
#endif
}



unsigned *_Shell_Clipboard_Get_Image_RGBA_Alloc (int *outwidth, int *outheight)
{
    unsigned *rgba_a = NULL;

#ifndef _CONSOLE
    NSPasteboard    *myPasteboard       = [NSPasteboard generalPasteboard];

    if ( [NSBitmapImageRep canInitWithPasteboard:myPasteboard] == NO)
        return NULL; // Can't get

    NSString *type = [myPasteboard availableTypeFromArray:[NSArray arrayWithObjects:NSTIFFPboardType /*,NSPICTPboardType*/ ,nil]];

    // Might be a small hole in this: What if datatype isn't TIFF but PICT or something?
    NSData *data            = [myPasteboard dataForType:type];

    // Load and format the data
    CGImageRef  imgRepData  = [[NSBitmapImageRep imageRepWithData: data] CGImage];

    if (imgRepData == nil)
    {
        logd (SPRINTSFUNC QUOTED_S " exists but couldn't load", __func__, "Fix me");
        return NULL;
    }

    // Allocate image data and fill it in
    {
        int imageWidth  = (int)CGImageGetWidth(imgRepData);
        int imageHeight = (int)CGImageGetHeight(imgRepData);
        size_t numBytes = imageWidth * imageHeight * RGBA_4 /* RGBA_BYTES_PER_PIXEL_IS_4 */;
        rgba_a     = core_calloc (1, numBytes);

        // Create a bitmap context with specified characteristics
        CGContextRef imageContext = CGBitmapContextCreate (rgba_a, imageWidth, imageHeight, 8,
                                                            imageWidth * RGBA_4 /* RGBA_BYTES_PER_PIXEL_IS_4 */ ,
                                                             CGImageGetColorSpace(imgRepData),
                                                             kCGImageAlphaPremultipliedLast);

        // Fill in the texture context
        CGContextDrawImage          (imageContext, CGRectMake(0.0, 0.0, (float)imageWidth, (float)imageHeight), imgRepData);
        CGContextRelease            (imageContext);

        *outwidth = imageWidth, *outheight = imageHeight;
    }

    // No cleanup needed here due to ARC!
#endif // ! _CONSOLE
    return rgba_a;
}

///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: VIDEO LIMITED
///////////////////////////////////////////////////////////////////////////////


cbool Vid_Desktop_Properties_Get (reply int *left, reply int *top, reply int *width, reply int *height)
{
// Mac must report this in terms of screen.  Part 1 of 2.  Part 2 is in Vid_Window_Create
    NSRect rect = [[NSScreen mainScreen] visibleFrame];

    int display_height; Vid_Display_Properties_Get (NULL, NULL, NULL, &display_height, NULL);
    int display_top = display_height - rect.size.height - rect.origin.y;

    NOT_MISSING_ASSIGN(left, rect.origin.x);
    NOT_MISSING_ASSIGN(top, display_top);
    NOT_MISSING_ASSIGN(width, rect.size.width);
    NOT_MISSING_ASSIGN(height, rect.size.height);

    return true;
}


int _Shell_Window_Style (wdostyle_e style)
{
    int dw_bordered    =  NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask;
    int dw_borderless  =  NSBorderlessWindowMask;
    int ret = Flag_Check (style, wdostyle_borderless) ? dw_borderless : dw_bordered;

    if (!Flag_Check (style, wdostyle_resizable))
        ret = Flag_Remove (style, NSResizableWindowMask);

    return ret;
}

int _Shell_Window_StyleEx (wdostyle_e style)
{
    return 0;
}


void Vid_Handle_Borders_Get (wdostyle_e style, cbool have_menu, reply int *left, reply int *top, reply int *width, reply int *height, reply int *right, reply int *bottom)
{
    int plat_style      = _Shell_Window_Style   (style);
    int plat_style_ex   = _Shell_Window_StyleEx (style);

    const int width_320 = 320, height_240 = 240;

    NSRect window_rect = NSMakeRect (0, 0, width_320, height_240);
    NSRect client_rect = [NSWindow contentRectForFrameRect:window_rect styleMask:plat_style];

    NOT_MISSING_ASSIGN (left, window_rect.size.width  - client_rect.size.width); // I expect 0
    NOT_MISSING_ASSIGN (top, window_rect.size.height - client_rect.size.height);
    NOT_MISSING_ASSIGN (width, window_rect.size.width  - client_rect.size.width);
    NOT_MISSING_ASSIGN (height, window_rect.size.height - client_rect.size.height);
    NOT_MISSING_ASSIGN (right, 0); // No right or left border on the Mac, right?
    NOT_MISSING_ASSIGN (bottom, 0); // No bottom border, right?
}


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: FILE DIALOGS
///////////////////////////////////////////////////////////////////////////////





#ifndef _CONSOLE
static NSString *lastOpenDirectory;
#endif

const char * _Shell_Dialog_Open_Directory (const char *title, const char *starting_folder_url)
{
#ifdef _CONSOLE
    return NULL;
#else // ! _CONSOLE
    static char directorySelected[MAX_OSPATH];

    directorySelected[0] = 0;

    // Determine starting directory
    NSString *startingDirectory = nil;

    if (starting_folder_url)
        startingDirectory = [NSString stringWithUTF8String:starting_folder_url];
    else if ([lastOpenDirectory length] == 0)
        startingDirectory = [NSString stringWithUTF8String:Folder_Binary_Folder_URL()];
    else startingDirectory = [lastOpenDirectory copy];

    NSURL *startingDirectoryURL = [NSURL URLWithString:[startingDirectory
                                                        stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];

    // Determine title string
    NSString *titleString = @"Select Folder";

    if (title)
        titleString = [NSString stringWithUTF8String:title];

#if 1
    NSOpenPanel *panel = [NSOpenPanel openPanel]; // ARC way
#else
    NSAutoreleasePool*  pool    = [[NSAutoreleasePool alloc] init];
    NSOpenPanel*        panel   = [[[NSOpenPanel alloc] init] autorelease];
#endif

    [panel setDirectoryURL:startingDirectoryURL];
    [panel setAllowsMultipleSelection: NO];
    [panel setCanChooseFiles: NO];
    [panel setCanChooseDirectories: YES];
    [panel setTitle: titleString];

    if ([panel runModal])
    {
        // Ok button was selected
        NSString *_directorySelected = [[panel directoryURL] path]; // Directory result
        c_strlcpy (directorySelected, TO_CSTRING(_directorySelected)   );
        lastOpenDirectory = [_directorySelected copy];
    }

#if 1
#else
    [pool release]; // Non-ARC Way
#endif

    return directorySelected;
#endif // ! _CONSOLE
}


const char *_Shell_Dialog_Open_Type (const char *title, const char * starting_folder_url, const char *extensions_comma_delimited)
{
    static char file_selected[MAX_OSPATH];

#ifdef _CONSOLE
    return NULL;
#else

    // Determine permitted extensions
    NSArray *allowedExtensions = nil;

    if (extensions_comma_delimited)
    {
        NSString *extensionsList = CSTRING(extensions_comma_delimited);
        allowedExtensions = [extensionsList componentsSeparatedByString:@","];
    }

    // Determine starting directory
    NSString *startingDirectory = nil;

    if (starting_folder_url)                    startingDirectory =  CSTRING(starting_folder_url);
    else if ([lastOpenDirectory length])        startingDirectory = [lastOpenDirectory copy];
    else                                        startingDirectory =  CSTRING(Folder_Binary_Folder_URL());

    NSURL *startingDirectoryURL = [NSURL URLWithString:[startingDirectory
                                                        stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
    file_selected[0] = 0;

    // Determine title string
    NSString *titleString = title ? CSTRING(title) : @"Select Folder";

#if 1
    NSOpenPanel *panel = [NSOpenPanel openPanel];
#else
    NSAutoreleasePool*  pool    = [[NSAutoreleasePool alloc] init];
    NSOpenPanel*        panel   = [[[NSOpenPanel alloc] init] autorelease];
#endif

    [panel setDirectoryURL:startingDirectoryURL];
    [panel setAllowsMultipleSelection: NO];
    [panel setCanChooseFiles: YES];
    [panel setCanChooseDirectories: NO];
    [panel setTitle: titleString];

    // Conditionally limit file extension
    if (extensions_comma_delimited)
        [panel setAllowedFileTypes:allowedExtensions];

    if ([panel runModal])
    {
        // Ok button was selected
        NSString *_directorySelected = [[panel directoryURL] path]; // Directory result
        c_strlcpy (file_selected, [_directorySelected cStringUsingEncoding: NSASCIIStringEncoding]);

        // Get the URL and convert it to an NSString without URL encoding (i.e. no %20 as a space, etc.)
        NSURL    *_selectedFileURL   = [[panel URLs]objectAtIndex:0];
        NSString *_selectedFile      = [NSString stringWithFormat:@"%@", _selectedFileURL];
        NSString *_selectedFile2     = [_selectedFile stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
        NSString *selectedFile       = [_selectedFile2 lastPathComponent];

        c_strlcat (file_selected, "/");
        c_strlcat (file_selected, TO_CSTRING(selectedFile)  );

        lastOpenDirectory = [_directorySelected copy];
    }
#if 1
//  [panel release];
#else
    [pool release];
#endif
    return file_selected;
#endif // ! _CONSOLE
}



const char * _Shell_Dialog_Save_Type (const char *title, const char * starting_folder_url, const char *extensions_comma_delimited)
{

    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OS: FILE MANAGER INTERACTION
///////////////////////////////////////////////////////////////////////////////


// Folder must exist.  It must be a folder.
cbool _Shell_Folder_Open_Folder_Must_Exist (const char *path_to_file)
{
#ifdef _CONSOLE
    return false;
#else
    NSURL       *fileURL = [NSURL fileURLWithPath:CSTRING(path_to_file) isDirectory:YES];

    [[NSWorkspace sharedWorkspace] openURL:fileURL];

    return true;
#endif // ! _CONSOLE
}

// File must exist
cbool _Shell_Folder_Open_Highlight_URL_Must_Exist (const char *path_to_file)
{
#ifdef _CONSOLE
    return false;
#else
    NSURL *fileURL = [NSURL fileURLWithPath:CSTRING(path_to_file) isDirectory:NO];

    NSArray *fileURLs = [NSArray arrayWithObjects:fileURL, nil];
    [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:fileURLs];

    //http://stackoverflow.com/questions/7652928/launch-osx-finder-window-with-specific-files-selected
    return true;
#endif // !CONSOLE
}


#ifndef CORE_SDL // MachineTime and Clipboard - we defer to SDL if CORE_SDL build

    ///////////////////////////////////////////////////////////////////////////////
    //  PLATFORM: MACHINE TIME
    ///////////////////////////////////////////////////////////////////////////////

    // Double self-initializes now

    double Platform_MachineTime (void) // no set metric
    {
        static uint64_t start_time   = 0;
        static double   scale       = 0.0;
        const uint64_t  time        = mach_absolute_time();

        if (start_time == 0)
        {
            mach_timebase_info_data_t   info = { 0 };

            if (mach_timebase_info (&info) != 0)
            {
                log_fatal ("Failed to read timebase!");
            }

            scale       = 1e-9 * ((double) info.numer) / ((double) info.denom);
            start_time   = time;
        }

        return (double) (time - start_time) * scale;
    }



    ///////////////////////////////////////////////////////////////////////////////
    //  PLATFORM: CLIPBOARD HELPER FUNCTIONS FOR INTERFACE.C
    ///////////////////////////////////////////////////////////////////////////////


    char *_Platform_Clipboard_Get_Text_Alloc (void)
    {
        char    *text_out = NULL;

    #ifndef _CONSOLE
        NSPasteboard    *myPasteboard  = [NSPasteboard generalPasteboard];
        NSArray         *types       = [myPasteboard types];

        if ([types containsObject: NSStringPboardType]) {
            NSString *clipboardString = [myPasteboard stringForType: NSStringPboardType];


            if (clipboardString && [clipboardString length])
                text_out = core_strdup (TO_CSTRING (clipboardString) );
        }
    #endif // ! _CONSOLE
        return text_out;
    }


    // copies given text to clipboard.  Text can't be NULL
    cbool _Platform_Clipboard_Set_Text (const char *text_to_clipboard)
    {
    #ifdef _CONSOLE
        // Nothing!
        return false;
    #else
        //Get general pasteboard
        NSPasteboard    *myPasteboard    = [NSPasteboard generalPasteboard];


        // NSPasteboardTypeString instead of NSStringPboardType as NSStringPboardType is deprecated in 10.6
        [myPasteboard clearContents];
        [myPasteboard declareTypes:[NSArray arrayWithObjects:NSStringPboardType, nil] owner:nil];
        [myPasteboard setString:CSTRING(text_to_clipboard) forType:NSStringPboardType];

    #endif // ! _CONSOLE
        return true;
    }

#endif // !CORE_SDL -- MachineTime and Clipboard - we defer to SDL if CORE_SDL build

#endif // PLATFORM_OSX


