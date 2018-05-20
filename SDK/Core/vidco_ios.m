/*
Copyright (C) 2013-2014 Baker

*/
// vid_ios.m -- vid

#include "environment.h"

#ifdef PLATFORM_GUI_IOS  // Not SDL build; not console build


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
	CGRect rect = [[UIScreen mainScreen] bounds];
// DO NOT ROTATE THESE
	NOT_MISSING_ASSIGN(left, 0); //rect.origin.x); // Better be 0
	NOT_MISSING_ASSIGN(top, 0); // rect.origin.y); // Better be 0
	NOT_MISSING_ASSIGN(width, rect.size.width);
	NOT_MISSING_ASSIGN(height, rect.size.height);
	NOT_MISSING_ASSIGN(bpp, PLATFORM_IOS_BPP_32); // Whatever ...

	return true;
}


// Send -1 to start, it returns count.
int Vid_Display_Modes_Properties_Get (int n, reply int *width, reply int *height, reply int *bpp)
{
	int					ret;
	int					modes_count		= 1;
	
	if (n == -1)				ret = modes_count;		// -1 wants count
	else if (n >= modes_count)	ret = 0; 				// Hit last return 0
	else {						ret = 1;				// continue
		// CGDisplayModeRef doesn't need released.  Apple doesn't in example.
		CGRect rect = [[UIScreen mainScreen] bounds]; // UIScreen does not have frame
		//int flags = CGDisplayModeGetIOFlags(mode);

		NOT_MISSING_ASSIGN(width, rect.size.width);
		NOT_MISSING_ASSIGN(height, rect.size.height);
		NOT_MISSING_ASSIGN(bpp,	PLATFORM_IOS_BPP_32); // BECAUSE IT ALWAYS IS
		
//		alert ("Mode %d %d %d", width ? *width : -1, height ? *height : -1, bpp ? *bpp : -1);
	}
	
	return ret;
}


void Vid_Handle_Client_RectRB_Get (sys_handle_t cw, reply int *left, reply int *top, reply int *width, reply int *height, reply int *right, reply int *bottom)
{
// This is client rect, no need to add border.
	CoreVidWindow * myWindow = (__bridge CoreVidWindow *)cw;
	CGRect frame = [[myWindow view] frame]; // Make me?
	int display_height; Vid_Display_Properties_Get (NULL, NULL, NULL, &display_height, NULL);
#pragma message ("Must create VidUIWindow method to get client rect from the GLKView!")
	int display_y_top = display_height - (frame.size.height +  frame.origin.y);

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
	return NULL;
}


void Vid_Handle_Context_Set (sys_handle_t cw, sys_handle_t draw_context, sys_handle_t render_context)
{
// With GLKView we have may no good way to control this.
// The CoreVidWindow does have a handle to the context, though.
	CoreVidWindow * myWindow = (__bridge CoreVidWindow *)cw;
}

void Vid_Handle_SwapBuffers (sys_handle_t cw, sys_handle_t draw_context, sys_handle_t render_context)
{
	
// With GLKView we have no control over this.
// The CoreVidWindow does have a handle to the context, though.
	CoreVidWindow * myWindow = (__bridge CoreVidWindow *)cw;
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
// Can't be done
}


void Vid_Handle_MaxSize (sys_handle_t cw, int width, int height)
{
// Can't be done
}


sys_handle_t *Vid_Handle_Create (void *obj_ptr, const char *caption, crect_t window_rect, wdostyle_e style, cbool havemenu, required sys_handle_t *draw_context_out, required sys_handle_t *gl_context_out)
{
	crect_t border;
	Vid_Handle_Borders_Get (style, true /* menu is irrelevant on Mac */, RECT_REPLY (border), NULL, NULL);
	int display_height; Vid_Display_Properties_Get (NULL, NULL, NULL, &display_height, NULL);

		int display_y_bottom = display_height - (window_rect.height + /*border_height +*/ window_rect.top); // 900 - 300 - 22 - 30 - 1
	//CGRect clientRect = CGRectMake (window_rect.left, window_rect.top, window_rect.width - border_width, window_rect.height - border_height);
	CGRect clientRect = CGRectMake (window_rect.left, display_y_bottom, window_rect.width - border.width, window_rect.height - border.height);

	CoreVidWindow *myWindow = [[CoreVidWindow alloc] createWindow: clientRect multisamples:0 style:style withPtr:obj_ptr];

	void *thing = (__bridge_retained void *)myWindow;
	
	sys_handle_t drawcontext =  NULL;   // We don't need access to this.
	sys_handle_t glcontext = NULL;  //	[[myWindow openGLContext] makeCurrentContext];

	Vid_Handle_Context_Set (thing, drawcontext, glcontext);

	// Don't need because CoreVidWindow is object ----SetWindowLongPtr (cw, GWLP_USERDATA, (LONG_PTR)obj_ptr);  // Points to us.
	// Don't need because This happens in VidWindowView or whatever it is //	WIN_SetupPixelFormat (*draw_context, 24, 32, 8); // color bits, depthbits, stencil bits
	// Considering we aren't recreating a window at the moment, this is irrelevant

	REQUIRED_ASSIGN (draw_context_out, drawcontext);
	REQUIRED_ASSIGN (gl_context_out, glcontext);

	return thing;
}





//// For main display.
//NSRect VID_Display_Frame (void)
//{
//    const CGRect    main = CGDisplayBounds (CGMainDisplayID ());
//    const CGRect    rect = CGDisplayBounds (CGMainDisplayID ());
//    
//    return NSMakeRect (rect.origin.x, main.size.height - rect.origin.y - rect.size.height, rect.size.width, rect.size.height);
//}

void Vid_Window_Hide (sys_handle_t cw)
{
	// Don't really have this option
	CoreVidWindow * myWindow = (__bridge CoreVidWindow *)cw;
	[myWindow window].hidden = YES;
}


void Vid_Handle_Hide (sys_handle_t cw)
{
	// Not sure!
#pragma message ("How do we hide a window on the Mac?")
}

void Vid_Handle_Move (sys_handle_t cw, wdostyle_e style, cbool have_menu, int left, int top, int width, int height)
{
	#pragma message ("Can't do?")
}


void Vid_Handle_Show (sys_handle_t cw)
{
	CoreVidWindow * myWindow = (__bridge CoreVidWindow *)cw;
	
// Don't really have this option
	[[myWindow window] makeKeyAndVisible];
}


void Vid_Handle_ZOrder (sys_handle_t cw)
{
// SDL doesn't have.  Probably similar to mac.
#pragma message ("Todo")
}


void _Vid_Handle_Caption (sys_handle_t cw, const char *text)
{
	return; // WE DON'T HAVE THOSE!
// Could we?
//	CoreVidWindow * myWindow = (CoreVidWindow *)cw;
//	[myWindow setTitle:CSTRING(text)];
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



///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: MSGBOX HELPER
///////////////////////////////////////////////////////////////////////////////

// Title cannot be NULL, System_MessageBox checks for that.
int _Platform_MessageBox (const char *title, const char *text)
{	
    UIAlertView *alert = [[UIAlertView alloc]
                          initWithTitle:CSTRING(title) message:CSTRING(text) delegate:nil
                          cancelButtonTitle:nil otherButtonTitles:@"OK", nil];
    [alert show];

    while ([alert isVisible]) {
		NSRunLoop *rl = [NSRunLoop currentRunLoop];
        NSDate *d = [[NSDate alloc] init];
        [rl runUntilDate:d];
    }
	return 0;
}

char *_Platform_Text_Dialog_Popup_Alloc (sys_handle_t parent_window, const char *prompt, const char *text, cbool _is_multiline)
{	
	msgbox (prompt, "Paste text to clipboard and click ok.  Default will be " QUOTED_S, text);
	return Clipboard_Get_Text_Alloc ();
}


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


// Please see VidUIWindow.h

int keymap [KEYMAP_COUNT_512][5]; // Need this.  Probably should match Mac?
// They have a way to get keys on IOS, but I'm don't want to waste time at moment.


///////////////////////////////////////////////////////////////////////////////
//  END DISPATCH
///////////////////////////////////////////////////////////////////////////////




#endif // PLATFORM_GUI_IOS



