/*
Copyright (C) 2013-2014 Baker

*/
// vid_osx.m -- vid

#include "environment.h"

// For now ... since we aren't using pure on Mac yet ..   def PLATFORM_GUI_OSX  // Not SDL build; not console build


#define CORE_LOCAL
#include "core.h"
#include "vidco.h" // Courtesy

//void Vid_InitOnce (void)
//{
//	static cbool done;
//
//	if (!done) {
//		// Nada!  At least at moment.
//
//		done = true;
//	}
//}


cbool Vid_Display_Properties_Get (reply int *left, reply int *top, reply int *width, reply int *height, reply int *bpp)
{
// Better be zero for origin otherwise on Apple we will have to height - y.
	NSRect rect = [[NSScreen mainScreen] frame]; // Maybe the one time I use bounds?
// DO NOT ROTATE THESE
	NOT_MISSING_ASSIGN(left, 0); //rect.origin.x); // Better be 0
	NOT_MISSING_ASSIGN(top, 0); //rect.origin.y); // Better be 0
	NOT_MISSING_ASSIGN(width, rect.size.width);
	NOT_MISSING_ASSIGN(height, rect.size.height);
	NOT_MISSING_ASSIGN(bpp, NSBitsPerPixelFromDepth( [[NSScreen mainScreen] depth] ));

	return true;
}

#if 0 
static int DisplayBitsPerPixel( CGDirectDisplayID displayId )
{
	CGDisplayModeRef mode = CGDisplayCopyDisplayMode(displayId);
	CFStringRef pixEnc = CGDisplayModeCopyPixelEncoding(mode);
	
	if(CFStringCompare(pixEnc, CFSTR(IO32BitDirectPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)			return 32;
	else if(CFStringCompare(pixEnc, CFSTR(IO16BitDirectPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)	return 16;
	else if(CFStringCompare(pixEnc, CFSTR(IO8BitIndexedPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)	return 8;
	
	log_fatal ("Display mode has unknown color depth");
	return 0;
}


// Send -1 to start, it returns count.
int Vid_Display_Modes_Properties_Get (int n, reply int *width, reply int *height, reply int *bpp)
{
	int					ret;
	CGDirectDisplayID	main_display	= CGMainDisplayID ();
	CFArrayRef			modeList		= CGDisplayCopyAllDisplayModes(main_display, NULL);
	
	int					modes_count		= CFArrayGetCount(modeList);
	
	
	if (n == -1)				ret = modes_count;		// -1 wants count
	else if (n >= modes_count)	ret = 0; 				// Hit last return 0
	else {						ret = 1;				// continue
		// CGDisplayModeRef doesn't need released.  Apple doesn't in example.
		CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modeList, n);
		//int flags = CGDisplayModeGetIOFlags(mode);

		NOT_MISSING_ASSIGN(width, CGDisplayModeGetWidth(mode));
		NOT_MISSING_ASSIGN(height, CGDisplayModeGetHeight(mode));
		NOT_MISSING_ASSIGN(bpp,	DisplayBitsPerPixel (main_display));
		
//		alert ("Mode %d %d %d", width ? *width : -1, height ? *height : -1, bpp ? *bpp : -1);
	}
	
	CFRelease (modeList);
	return ret;
}



void Vid_Handle_Client_RectRB_Get (sys_handle_t cw, reply int *left, reply int *top, reply int *width, reply int *height, reply int *right, reply int *bottom)
{
// This is client rect, no need to add border.
	CoreVidWindow * myWindow = (__bridge CoreVidWindow *)cw;
	NSRect frame = [[myWindow.window contentView] frame];
	
	int display_height; Vid_Display_Properties_Get (NULL, NULL, NULL, &display_height, NULL);
	int display_y_top = display_height - (frame.size.height + frame.origin.y);

// DO NOT ROTATE THESE EVER.  We use matrix transformation to get x,y
	NOT_MISSING_ASSIGN (left, frame.origin.x);
	NOT_MISSING_ASSIGN (top, display_y_top);
	NOT_MISSING_ASSIGN (width, frame.size.width);
	NOT_MISSING_ASSIGN (height, frame.size.height);
	NOT_MISSING_ASSIGN (right, frame.origin.x + frame.size.width);
	NOT_MISSING_ASSIGN (bottom, display_y_top + frame.size.height);
}


sys_handle_t Vid_Handle_Context_Get (void)
{
	return (__bridge sys_handle_t)([NSOpenGLContext currentContext]);
}


void Vid_Handle_Context_Set (sys_handle_t cw, sys_handle_t draw_context, sys_handle_t gl_context)
{
	CoreVidWindow * myWindow = (__bridge CoreVidWindow *)cw;
	NSOpenGLContext *glc = [myWindow getOpenGLContext];
	[glc makeCurrentContext];
}

void Vid_Handle_SwapBuffers (sys_handle_t cw, sys_handle_t draw_context, sys_handle_t gl_context)
{
	CoreVidWindow * myWindow = (__bridge CoreVidWindow *)cw;
	[[myWindow getOpenGLContext] flushBuffer];
}


sys_handle_t *Vid_Handle_Destroy (sys_handle_t cw, required sys_handle_t *draw_context, required sys_handle_t *gl_context, cbool should_delete_context)
{
	// destroy = 1 = TEARDOWN_FULL else TEARDOWN_NO_DELETE_GL_CONTEXT (don't destroy the context or destroy window)
	CoreVidWindow * myWindow = (__bridge_transfer CoreVidWindow *)cw;
	[myWindow destroyWindow];

	myWindow = nil; // Should release it
	return NULL;
}


void Vid_Handle_MinSize (sys_handle_t cw, int width, int height)
{
	CoreVidWindow * myWindow = (__bridge CoreVidWindow *)cw;
	NSSize size = NSMakeSize(width,height);
	myWindow.window.contentMinSize = size;
}


void Vid_Handle_MaxSize (sys_handle_t cw, int width, int height)
{
	CoreVidWindow * myWindow = (__bridge CoreVidWindow *)cw;
	NSSize size = NSMakeSize(width,height);
	myWindow.window.contentMaxSize = size;
}


sys_handle_t *Vid_Handle_Create (void *obj_ptr, const char *caption, crect_t window_rect, wdostyle_e style, cbool havemenu, required sys_handle_t *draw_context_out, required sys_handle_t *gl_context_out)
{
	crect_t border;
	Vid_Handle_Borders_Get (style, true /* menu is irrelevant on Mac */, RECT_REPLY (border), NULL, NULL);

	int display_height; Vid_Display_Properties_Get (NULL, NULL, NULL, &display_height, NULL);
	int display_y_bottom = display_height - (window_rect.height + /*border_height +*/ window_rect.top); // 900 - 300 - 22 - 30 - 1
// Ok, this needs to specify the bottom of the window.  Possibly from the bottom of the visible screen
// So, let's say dock is 855 y h 45 and window is 600 + 22 and we want 30.
// 30 + 600 + 22 + ? = 840 - ?
//	NSRect rect = [[NSScreen mainScreen] visibleFrame];

	CGRect clientRect = CGRectMake (window_rect.left, display_y_bottom, window_rect.width - border.width, window_rect.height - border.height);

	CoreVidWindow *myWindow = [[CoreVidWindow alloc] createWindow: clientRect multisamples:0 style:style withPtr:obj_ptr];

	void *thing = (__bridge_retained void *)myWindow;

	sys_handle_t drawcontext = NULL;   // We don't need access to this.
	sys_handle_t glcontext = NULL;  //	[[myWindow openGLContext] makeCurrentContext];

	Vid_Handle_Context_Set  (thing, drawcontext, glcontext);

	// Don't need because CoreVidWindow is object ----SetWindowLongPtr (cw, GWLP_USERDATA, (LONG_PTR)obj_ptr);  // Points to us.
	// Don't need because This happens in VidWindowView or whatever it is //	WIN_SetupPixelFormat (*draw_context, 24, 32, 8); // color bits, depthbits, stencil bits
	// Considering we aren't recreating a window at the moment, this is irrelevant

	Vid_Handle_Caption (thing, "%s", caption);

	NOT_MISSING_ASSIGN (draw_context_out, drawcontext);
	NOT_MISSING_ASSIGN (gl_context_out, glcontext);

	return thing;
}


// For main display.
NSRect VID_Display_Frame (void)
{
    const CGRect    main = CGDisplayBounds (CGMainDisplayID ());
    const CGRect    rect = CGDisplayBounds (CGMainDisplayID ());
    
    return NSMakeRect (rect.origin.x, main.size.height - rect.origin.y - rect.size.height, rect.size.width, rect.size.height);
}


void Vid_Handle_Hide (sys_handle_t cw)
{
	// Not sure!
#pragma message ("How do we hide a window on the Mac?")
	CoreVidWindow * myWindow = (__bridge CoreVidWindow *)cw;
	[[myWindow window] orderOut:nil];
}


void Vid_Handle_Move (sys_handle_t cw, wdostyle_e style, cbool have_menu, int left, int top, int width, int height)
{
	CoreVidWindow * myWindow = (__bridge CoreVidWindow *)cw; // Client coords inbound
//	NSRect frame = [window frame];
//frame.size = theSizeYouWant;
//[window setFrame: frame display: YES animate: whetherYouWantAnimation];
//NSRect frame = [window frame];
//frame.origin.y -= frame.size.height; // remove the old height
//frame.origin.y += theSizeYouWant.height; // add the new height
//frame.size = theSizeYouWant;
//
//	crect_t border;
//	Vid_Handle_Borders_Get (style, true /* menu is irrelevant on Mac */, RECT_REPLY (border), NULL, NULL);

	int display_height; Vid_Display_Properties_Get (NULL, NULL, NULL, &display_height, NULL);
	int display_y_bottom = display_height - (height + /*border_height +*/ top); // 900 - 300 - 22 - 30 - 1
// Ok, this needs to specify the bottom of the window.  Possibly from the bottom of the visible screen
// So, let's say dock is 855 y h 45 and window is 600 + 22 and we want 30.
// 30 + 600 + 22 + ? = 840 - ?
//	NSRect rect = [[NSScreen mainScreen] visibleFrame];

//	NSRect clientRectMac = CGRectMake (window_rect.left, display_y_bottom, window_rect.width - border.width, window_rect.height - border.height);
	NSRect clientRectMac = CGRectMake (left, display_y_bottom, width, height);

	[myWindow.window setFrame: clientRectMac display: YES /*well if we are hidden hmmm*/ animate: NO];
}


void Vid_Handle_Show (sys_handle_t cw)
{
	CoreVidWindow * myWindow = (__bridge CoreVidWindow *)cw;
	
    [[myWindow window] makeKeyAndOrderFront: myWindow]; // Hmmm?  Why was this nil before?
    [[myWindow window] makeMainWindow]; // Differences
    [[myWindow window] flushWindow]; // Hmmm.  Isn't this a buffer flip?

//   if (![myWindow isMiniaturized]) {
//      [myWindow makeKeyAndOrderFront:nil]; // SDL uses nil
//  }
// SDL: makeKeyAndOrderFront: has the side-effect of deminiaturizing and showing
// a minimized or hidden window, so check for that before showing it
}


void Vid_Handle_ZOrder (sys_handle_t cw)
{
// SDL: makeKeyAndOrderFront: has the side-effect of deminiaturizing and showing
// a minimized or hidden window, so check for that before showing it
	CoreVidWindow * myWindow = (__bridge CoreVidWindow *)cw;
    if (![[myWindow window] isMiniaturized] && [[myWindow window] isVisible]) {
        [[myWindow window] makeKeyAndOrderFront:nil];
    }
}


void _Vid_Handle_Caption (sys_handle_t cw, const char *text)
{
	CoreVidWindow * myWindow = (__bridge CoreVidWindow *)cw;
	[[myWindow window] setTitle:CSTRING(text)];
}

#if 0
static LPCTSTR W_For_CursorEnum (mousepointer_e mousepointer)
{
	typedef struct tt  {
		int i; 
		LPCTSTR name; 
	};

	static cbool init;
	static struct tt table[MAX_MOUSEPOINTERS] = {
		{ mousepointer_invalid,				IDC_ARROW		},		// Invalid
		{ mousepointer_arrow,				IDC_ARROW		},		// Default
		{ mousepointer_crosshair,			IDC_CROSS		},		// Think image editor
		{ mousepointer_hand,				IDC_HAND		},		// Button or action, in theory, in practice that's an arrow
		{ mousepointer_help,				IDC_HELP		},		// Help.  I've never seen this decently used.
		{ mousepointer_hourglass,			IDC_WAIT		},		// Hourglass
		{ mousepointer_move,				IDC_SIZEALL		},		// Move something
		{ mousepointer_prohibited,			IDC_NO			},		// Move something
		{ mousepointer_size_up,				IDC_SIZENS		},		// Size north south
		{ mousepointer_size_left,			IDC_SIZEWE		},		// Size east west
		{ mousepointer_size_both,			IDC_SIZENWSE	},		// Size diagonal
		{ mousepointer_text,				IDC_IBEAM		},		// Text
	};

	if (! in_range(0, mousepointer, MAX_MOUSEPOINTERS - 1) )
		log_fatal ("Invalid mousepointer");
	return table[mousepointer].name;
}
#endif


void Vid_Handle_MousePointer (sys_handle_t cw, sys_handle_t *hmousepointer, mousepointer_e mousepointer)
{
// I guess for the Mac, the handle might be the view?
//https://developer.apple.com/library/mac/documentation/Cocoa/Conceptual/CursorMgmt/Tasks/ChangingCursors.html
//[aView addCursorRect:aRect cursor:aCursor];
//[aCursor setOnMouseEntered:YES];
//arrowCursor, IBeamCursor, etc.
//	*hmousepointer = LoadCursor (NULL, W_For_CursorEnum(mousepointer) );
//
//	//SetCursor (hicon);
//	SetClassLong(h, GCL_HCURSOR, (DWORD)*hmousepointer); 
//[yourButton addCursorRect:[yourButton frame] cursor:[theCursorYouWant]];
}
#endif // 0


///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: MSGBOX HELPER
///////////////////////////////////////////////////////////////////////////////

// Title cannot be NULL, System_MessageBox checks for that.
int _Platform_MessageBox (const char *title, const char *text)
{	
    NSAlert *alert = [[NSAlert alloc] init]; ///*autorelease*/];
    [alert setMessageText:CSTRING(title)];
    [alert setInformativeText:CSTRING(text)];
    [alert runModal];
	return 0;
}

char *_Platform_Text_Dialog_Popup_Alloc (sys_handle_t parent_window, const char *prompt, const char *text, cbool _is_multiline)
{	
	msgbox (prompt, "Paste text to clipboard and click ok.  Default will be " QUOTED_S, text);
	return Clipboard_Get_Text_Alloc ();
}

#if 0

///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: EVENTS
///////////////////////////////////////////////////////////////////////////////


void Platform_Events_SleepForInput (required sys_handle_t *phandle_tevent, double max_wait_seconds)
{
	// This doesn't apply to console applications nor timeslice
	// So no code needs to go in here for a Mac at all!
}


cbool Platform_Events_Do (void)
{
	// This doesn't apply to console applications nor timeslice
	// So no code needs to go in here for a Mac at all!

	return true;
}


///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: Simple sound
///////////////////////////////////////////////////////////////////////////////


void Shell_Play_Wav_File (const char *path_to_file)
{
//	PlaySound(path_to_file, NULL, SND_FILENAME | SND_ASYNC);
}

void Sound_Play_Wav_Memory (const void *wav_pointer, size_t wav_length)
{
//	PlaySound(wav_pointer, NULL, SND_MEMORY | SND_ASYNC);
}

void Shell_Play_Wav_Bundle (const char *path_to_file)
{
//	PlaySound(Bundle_File_Memory_Pointer(path_to_file, NULL), NULL, SND_MEMORY | SND_ASYNC);
}


//void Sound_Play_Stop (const void *in_wav_pointer, size_t in_wav_length)
//{
//	// I do nothing!  Future?
//}

void Sound_Item_Free (sound_item_t *t)
{
	//AudioServicesDisposeSystemSoundID(cur->soundid_Apple);

}


void Sound_Item_Load_Mem (sound_item_t *t)
{
	const char *tempname = va("%s/%s", Folder_Caches_URL(), File_URL_SkipPath(t->path_to_file));
	NSURL 		*tempnameURL = [NSURL fileURLWithPath:CSTRING(tempname)];
	//Clipboard_Set_Text(tempname);
	File_Mkdir_Recursive (tempname);
	File_Memory_To_File (tempname, t->mem, t->mem_length);
	//Folder_Open_Highlight(tempname);
	OSStatus error = AudioServicesCreateSystemSoundID((__bridge CFURLRef)tempnameURL, &t->audiobox_sound_id);
	if (error != kAudioServicesNoError)
		log_fatal ("Audio error");
//	File_Delete(tempname);
//	return SoundID;
	
}


cbool Sound_Item_Play (sound_item_t *t)
{
	AudioServicesPlaySystemSound (t->audiobox_sound_id);
	return true; // Looks good!
}


///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: DISPATCH.  AT LEAST THE DEFAULT ONE.
///////////////////////////////////////////////////////////////////////////////


// Please see VidWindow.h

int keymap [KEYMAP_COUNT_512][5] = {
	{ 0  , kVK_ANSI_A              , 'A'                 , 0        }, /* vk hex = 0x00 */
	{ 1  , kVK_ANSI_S              , 'S'                 , 0        }, /* vk hex = 0x01 */
	{ 2  , kVK_ANSI_D              , 'D'                 , 0        }, /* vk hex = 0x02 */
	{ 3  , kVK_ANSI_F              , 'F'                 , 0        }, /* vk hex = 0x03 */
	{ 4  , kVK_ANSI_H              , 'H'                 , 0        }, /* vk hex = 0x04 */
	{ 5  , kVK_ANSI_G              , 'G'                 , 0        }, /* vk hex = 0x05 */
	{ 6  , kVK_ANSI_Z              , 'Z'                 , 0        }, /* vk hex = 0x06 */
	{ 7  , kVK_ANSI_X              , 'X'                 , 0        }, /* vk hex = 0x07 */
	{ 8  , kVK_ANSI_C              , 'C'                 , 0        }, /* vk hex = 0x08 */
	{ 9  , kVK_ANSI_V              , 'V'                 , 0        }, /* vk hex = 0x09 */
	{ 10 , kVK_ISO_Section         , 0                   , 0        }, /* vk hex = 0x0A */
	{ 11 , kVK_ANSI_B              , 'B'                 , 0        }, /* vk hex = 0x0B */
	{ 12 , kVK_ANSI_Q              , 'Q'                 , 0        }, /* vk hex = 0x0C */
	{ 13 , kVK_ANSI_W              , 'W'                 , 0        }, /* vk hex = 0x0D */
	{ 14 , kVK_ANSI_E              , 'E'                 , 0        }, /* vk hex = 0x0E */
	{ 15 , kVK_ANSI_R              , 'R'                 , 0        }, /* vk hex = 0x0F */
	{ 16 , kVK_ANSI_Y              , 'Y'                 , 0        }, /* vk hex = 0x10 */
	{ 17 , kVK_ANSI_T              , 'T'                 , 0        }, /* vk hex = 0x11 */
	{ 18 , kVK_ANSI_1              , '1'                 , 0        }, /* vk hex = 0x12 */
	{ 19 , kVK_ANSI_2              , '2'                 , 0        }, /* vk hex = 0x13 */
	{ 20 , kVK_ANSI_3              , '3'                 , 0        }, /* vk hex = 0x14 */
	{ 21 , kVK_ANSI_4              , '4'                 , 0        }, /* vk hex = 0x15 */
	{ 22 , kVK_ANSI_6              , '6'                 , 0        }, /* vk hex = 0x16 */
	{ 23 , kVK_ANSI_5              , '5'                 , 0        }, /* vk hex = 0x17 */
	{ 24 , kVK_ANSI_Equal          , K_EQUALS            , 0        }, /* vk hex = 0x18 */
	{ 25 , kVK_ANSI_9              , '9'                 , 0        }, /* vk hex = 0x19 */
	{ 26 , kVK_ANSI_7              , '7'                 , 0        }, /* vk hex = 0x1A */
	{ 27 , kVK_ANSI_Minus          , K_MINUS             , 0        }, /* vk hex = 0x1B */
	{ 28 , kVK_ANSI_8              , '8'                 , 0        }, /* vk hex = 0x1C */
	{ 29 , kVK_ANSI_0              , '0'                 , 0        }, /* vk hex = 0x1D */
	{ 30 , kVK_ANSI_RightBracket   , K_RIGHTBRACKET      , 0        }, /* vk hex = 0x1E */
	{ 31 , kVK_ANSI_O              , 'O'                 , 0        }, /* vk hex = 0x1F */
	{ 32 , kVK_ANSI_U              , 'U'                 , 0        }, /* vk hex = 0x20 */
	{ 33 , kVK_ANSI_LeftBracket    , K_LEFTBRACKET       , 0        }, /* vk hex = 0x21 */
	{ 34 , kVK_ANSI_I              , 'I'                 , 0        }, /* vk hex = 0x22 */
	{ 35 , kVK_ANSI_P              , 'P'                 , 0        }, /* vk hex = 0x23 */
	{ 36 , kVK_Return              , K_ENTER             , 0        }, /* vk hex = 0x24 */
	{ 37 , kVK_ANSI_L              , 'L'                 , 0        }, /* vk hex = 0x25 */
	{ 38 , kVK_ANSI_J              , 'J'                 , 0        }, /* vk hex = 0x26 */
	{ 39 , kVK_ANSI_Quote          , K_APOSTROPHE        , 0        }, /* vk hex = 0x27 */
	{ 40 , kVK_ANSI_K              , 'K'                 , 0        }, /* vk hex = 0x28 */
	{ 41 , kVK_ANSI_Semicolon      , K_SEMICOLON         , 0        }, /* vk hex = 0x29 */
	{ 42 , kVK_ANSI_Backslash      , K_BACKSLASH         , 0        }, /* vk hex = 0x2A */
	{ 43 , kVK_ANSI_Comma          , K_COMMA             , 0        }, /* vk hex = 0x2B */
	{ 44 , kVK_ANSI_Slash          , K_SLASH             , 0        }, /* vk hex = 0x2C */
	{ 45 , kVK_ANSI_N              , 'N'                 , 0        }, /* vk hex = 0x2D */
	{ 46 , kVK_ANSI_M              , 'M'                 , 0        }, /* vk hex = 0x2E */
	{ 47 , kVK_ANSI_Period         , K_PERIOD            , 0        }, /* vk hex = 0x2F */
	{ 48 , kVK_Tab                 , K_TAB               , 0        }, /* vk hex = 0x30 */
	{ 49 , kVK_Space               , K_SPACE             , 0        }, /* vk hex = 0x31 */
	{ 50 , kVK_ANSI_Grave          , K_GRAVE             , 0        }, /* vk hex = 0x32 */
	{ 51 , kVK_Delete              , K_BACKSPACE         , 0        }, /* vk hex = 0x33 */
	{ 52 , -1                      , 0                   , 0        }, /* vk hex = 0x34 */
	{ 53 , kVK_Escape              , K_ESCAPE            , 0        }, /* vk hex = 0x35 */
	{ 54 , kVK_RightCommand_X      , K_RWIN              , 0x100010 }, /* vk hex = 0x36 */
	{ 55 , kVK_Command             , K_LWIN              , 0x100008 }, /* vk hex = 0x37 */
	{ 56 , kVK_Shift               , K_LSHIFT            , 0x020002 }, /* vk hex = 0x38 */
	{ 57 , kVK_CapsLock            , 0 /*K_CAPSLOCK*/    , 0x100000 }, /* vk hex = 0x39 */
	{ 58 , kVK_Option              , K_LALT              , 0x080020 }, /* vk hex = 0x3A */
	{ 59 , kVK_Control             , K_LCTRL             , 0x040001 }, /* vk hex = 0x3B */
	{ 60 , kVK_RightShift          , K_RSHIFT            , 0x020004 }, /* vk hex = 0x3C */
	{ 61 , kVK_RightOption         , K_RALT              , 0x080040 }, /* vk hex = 0x3D */
	{ 62 , kVK_RightControl        , K_RCTRL             , 0x042000 }, /* vk hex = 0x3E */
	{ 63 , kVK_Function            , 0                   , 0        }, /* vk hex = 0x3F */
	{ 64 , kVK_F17                 , 0                   , 0        }, /* vk hex = 0x40 */
	{ 65 , kVK_ANSI_KeypadDecimal  , K_NUMPAD_PERIOD     , 0        }, /* vk hex = 0x41 */
	{ 66 , -1                      , 0                   , 0        }, /* vk hex = 0x42 */
	{ 67 , kVK_ANSI_KeypadMultiply , K_NUMPAD_MULTIPLY   , 0        }, /* vk hex = 0x43 */
	{ 68 , 0                       , 0                   , 0        }, /* vk hex = 0x44 */
	{ 69 , kVK_ANSI_KeypadPlus     , K_NUMPAD_PLUS       , 0        }, /* vk hex = 0x45 */
	{ 70 , -1                      , 0                   , 0        }, /* vk hex = 0x46 */
	{ 71 , kVK_ANSI_KeypadClear    , 0                   , 0        }, /* vk hex = 0x47 */
	{ 72 , kVK_VolumeUp            , 0                   , 0        }, /* vk hex = 0x48 */
	{ 73 , kVK_VolumeDown          , 0                   , 0        }, /* vk hex = 0x49 */
	{ 74 , kVK_Mute                , 0                   , 0        }, /* vk hex = 0x4A */
	{ 75 , kVK_ANSI_KeypadDivide   , K_NUMPAD_DIVIDE     , 0        }, /* vk hex = 0x4B */
	{ 76 , kVK_ANSI_KeypadEnter    , K_NUMPAD_ENTER      , 0        }, /* vk hex = 0x4C */
	{ 77 , -1                      , 0                   , 0        }, /* vk hex = 0x4D */
	{ 78 , kVK_ANSI_KeypadMinus    , K_NUMPAD_MINUS      , 0        }, /* vk hex = 0x4E */
	{ 79 , kVK_F18                 , 0                   , 0        }, /* vk hex = 0x4F */
	{ 80 , kVK_F19                 , 0                   , 0        }, /* vk hex = 0x50 */
	{ 81 , kVK_ANSI_KeypadEquals   , 0                   , 0        }, /* vk hex = 0x51 */
	{ 82 , kVK_ANSI_Keypad0        , K_NUMPAD_0          , 0        }, /* vk hex = 0x52 */
	{ 83 , kVK_ANSI_Keypad1        , K_NUMPAD_1          , 0        }, /* vk hex = 0x53 */
	{ 84 , kVK_ANSI_Keypad2        , K_NUMPAD_2          , 0        }, /* vk hex = 0x54 */
	{ 85 , kVK_ANSI_Keypad3        , K_NUMPAD_3          , 0        }, /* vk hex = 0x55 */
	{ 86 , kVK_ANSI_Keypad4        , K_NUMPAD_4          , 0        }, /* vk hex = 0x56 */
	{ 87 , kVK_ANSI_Keypad5        , K_NUMPAD_5          , 0        }, /* vk hex = 0x57 */
	{ 88 , kVK_ANSI_Keypad6        , K_NUMPAD_6          , 0        }, /* vk hex = 0x58 */
	{ 89 , kVK_ANSI_Keypad7        , K_NUMPAD_7          , 0        }, /* vk hex = 0x59 */
	{ 90 , kVK_F20                 , 0                   , 0        }, /* vk hex = 0x5A */
	{ 91 , kVK_ANSI_Keypad8        , K_NUMPAD_8          , 0        }, /* vk hex = 0x5B */
	{ 92 , kVK_ANSI_Keypad9        , K_NUMPAD_9          , 0        }, /* vk hex = 0x5C */
	{ 93 , kVK_JIS_Yen             , 0                   , 0        }, /* vk hex = 0x5D */
	{ 94 , kVK_JIS_Underscore      , 0                   , 0        }, /* vk hex = 0x5E */
	{ 95 , kVK_JIS_KeypadComma     , 0                   , 0        }, /* vk hex = 0x5F */
	{ 96 , kVK_F5                  , K_F5                , 0        }, /* vk hex = 0x60 */
	{ 97 , kVK_F6                  , K_F6                , 0        }, /* vk hex = 0x61 */
	{ 98 , kVK_F7                  , K_F7                , 0        }, /* vk hex = 0x62 */
	{ 99 , kVK_F3                  , K_F3                , 0        }, /* vk hex = 0x63 */
	{ 100, kVK_F8                  , K_F8                , 0        }, /* vk hex = 0x64 */
	{ 101, kVK_F9                  , K_F9                , 0        }, /* vk hex = 0x65 */
	{ 102, kVK_JIS_Eisu            , 0                   , 0        }, /* vk hex = 0x66 */
	{ 103, kVK_F11                 , K_F11               , 0        }, /* vk hex = 0x67 */
	{ 104, kVK_JIS_Kana            , 0                   , 0        }, /* vk hex = 0x68 */
	{ 105, kVK_F13                 , 0                   , 0        }, /* vk hex = 0x69 */
	{ 106, kVK_F16                 , 0                   , 0        }, /* vk hex = 0x6A */
	{ 107, kVK_F14                 , 0                   , 0        }, /* vk hex = 0x6B */
	{ 108, -1                      , 0                   , 0        }, /* vk hex = 0x6C */
	{ 109, kVK_F10                 , K_F10               , 0        }, /* vk hex = 0x6D */
	{ 110, kVK_MENU_X              , K_MENU              , 0        }, /* vk hex = 0x6E */
	{ 111, kVK_F12                 , K_F12               , 0        }, /* vk hex = 0x6F */
	{ 112, -1                      , 0                   , 0        }, /* vk hex = 0x70 */
	{ 113, kVK_F15                 , 0                   , 0        }, /* vk hex = 0x71 */
	{ 114, kVK_Help                , 0                   , 0        }, /* vk hex = 0x72 */
	{ 115, kVK_Home                , K_HOME              , 0        }, /* vk hex = 0x73 */
	{ 116, kVK_PageUp              , K_PAGEUP            , 0        }, /* vk hex = 0x74 */
	{ 117, kVK_ForwardDelete       , K_DELETE            , 0        }, /* vk hex = 0x75 */
	{ 118, kVK_F4                  , K_F4                , 0        }, /* vk hex = 0x76 */
	{ 119, kVK_End                 , K_END               , 0        }, /* vk hex = 0x77 */
	{ 120, kVK_F2                  , K_F2                , 0        }, /* vk hex = 0x78 */
	{ 121, kVK_PageDown            , K_PAGEDOWN          , 0        }, /* vk hex = 0x79 */
	{ 122, kVK_F1                  , K_F1                , 0        }, /* vk hex = 0x7A */
	{ 123, kVK_LeftArrow           , K_LEFT              , 0        }, /* vk hex = 0x7B */
	{ 124, kVK_RightArrow          , K_RIGHT             , 0        }, /* vk hex = 0x7C */
	{ 125, kVK_DownArrow           , K_DOWN              , 0        }, /* vk hex = 0x7D */
	{ 126, kVK_UpArrow             , K_UP                , 0        }, /* vk hex = 0x7E */
	{ 127, -1                      , 0                   , 0        }, /* vk hex = 0x7F */
};


///////////////////////////////////////////////////////////////////////////////
//  END DISPATCH
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: MAIN
///////////////////////////////////////////////////////////////////////////////


// entry point
int main (int argc, const char **pArgv)
{
//	ProcessSerialNumber psn = {0, kCurrentProcess};
//	OSStatus status =
//	TransformProcessType(&psn, kProcessTransformToForegroundApplication);
//	
	// Our loop occurs in a frame timer.
	mainus.loop_timer_method = loop_timer_method_timeslice;
	mainus.no_graphics		 = false;

	MAINUS_ARGV_COPY (argc, pArgv);

	_main_central_start ();  // Initialize Core and other early init
	
    return NSApplicationMain (argc, pArgv);
}


#endif // PLATFORM_GUI_OSX



