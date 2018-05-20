/*
Copyright (C) 2013-2014 Baker

*/
// vid_console.c -- vid

#include "environment.h"

#ifdef _CONSOLE


#define CORE_LOCAL
#include "core.h"
#include "vid.h" // Courtesy

static const char *unsupported_msg_text = "Windowing not supported for console applications.";
#define UNSUPPORTED_ALERT(_x)	log_warn (unsupported_msg_text)


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
	UNSUPPORTED_ALERT (0);
	return false;
}


// Send -1 to start, it returns count.
int Vid_Display_Modes_Properties_Get (int n, reply int *width, reply int *height, reply int *bpp)
{
	UNSUPPORTED_ALERT (0);
	return 0;
}




void Vid_Handle_Client_RectRB_Get (sys_handle_t cw, reply int *left, reply int *top, reply int *width, reply int *height, reply int *right, reply int *bottom)
{
	UNSUPPORTED_ALERT (0);
}


sys_handle_t Vid_Handle_Context_Get (void)
{
	UNSUPPORTED_ALERT (0);
	return NULL;
}


void Vid_Handle_Context_Set (sys_handle_t cw, sys_handle_t draw_context, sys_handle_t gl_context)
{
	UNSUPPORTED_ALERT (0);
}

void Vid_Handle_SwapBuffers (sys_handle_t cw, sys_handle_t draw_context, sys_handle_t gl_context)
{
	UNSUPPORTED_ALERT (0);
}


sys_handle_t *Vid_Handle_Destroy (sys_handle_t cw, required sys_handle_t *draw_context, required sys_handle_t *gl_context, cbool should_delete_context)
{
	UNSUPPORTED_ALERT (0);
	return NULL;
}


void Vid_Handle_MinSize (sys_handle_t cw, int width, int height)
{
	UNSUPPORTED_ALERT (0);
}


void Vid_Handle_MaxSize (sys_handle_t cw, int width, int height)
{
	UNSUPPORTED_ALERT (0);
}


sys_handle_t *Vid_Handle_Create (void *obj_ptr, const char *caption, crect_t window_rect, wdostyle_e style, cbool havemenu, required sys_handle_t *draw_context_out, required sys_handle_t *gl_context_out)
{
	UNSUPPORTED_ALERT (0);
	return NULL;
}


void Vid_Handle_Move (sys_handle_t cw, wdostyle_e style, cbool have_menu, int left, int top, int width, int height)
{
	UNSUPPORTED_ALERT (0);
}


void Vid_Handle_Hide (sys_handle_t cw)
{
	UNSUPPORTED_ALERT (0);
}


void Vid_Handle_Show (sys_handle_t cw)
{
	UNSUPPORTED_ALERT (0);
}


void Vid_Handle_ZOrder (sys_handle_t cw)
{
	UNSUPPORTED_ALERT (0);
}


void _Vid_Handle_Caption (sys_handle_t cw, const char *text)
{
	UNSUPPORTED_ALERT (0);
}



void Vid_Handle_MousePointer (sys_handle_t cw, sys_handle_t *hmousepointer, mousepointer_e mousepointer)
{
	UNSUPPORTED_ALERT (0);
}



///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: MSGBOX HELPER
///////////////////////////////////////////////////////////////////////////////

// Title cannot be NULL, System_MessageBox checks for that.
int _Platform_MessageBox (const char *title, const char *_text)
{
// Do a blocking input box requiring a keypress
	text_a *text = String_Edit_For_Stdout_Ascii (strdup(_text));
	printline ();
	printline ();
	printlinef ("> %s: %s ...", title, text); // newline care
	printlinef ("(press enter to continue)");
	getchar ();
	printline ();
	txtfree (&text);
	return 0;
}

char *_Platform_Text_Dialog_Popup_Alloc (sys_handle_t parent_window, const char *prompt, const char *text, cbool _is_multiline)
{
	char *input_text = NULL;

	printf ("%s: ", prompt);
	while (! (input_text = Terminal_Input ()) )
		Platform_Sleep_Milliseconds (1); // Slightly better than a busy wait.

    return (strdup(input_text));
}


///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: EVENTS
///////////////////////////////////////////////////////////////////////////////


void Platform_Events_SleepForInput (required sys_handle_t *phandle_tevent, double max_wait_seconds)
{
	// This doesn't apply to console applications
	// So no code needs to go in here at all!
}


cbool Platform_Events_Do (void)
{
	// This doesn't apply to console applications
	// So no code needs to go in here at all!

	return true;
}


///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: Simple sound
///////////////////////////////////////////////////////////////////////////////


void Shell_Play_Wav_File (const char *path_to_file)
{
	// We do not have sound.  Right?
}

void Sound_Play_Wav_Memory (const void *wav_pointer, size_t wav_length)
{
	// We do not have sound.  Right?
}

void Shell_Play_Wav_Bundle (const char *path_to_file)
{
	// We do not have sound.  Right?
}


//void Sound_Play_Stop (const void *in_wav_pointer, size_t in_wav_length)
//{
//	// I do nothing!  Future?
//}

void Sound_Item_Free (sound_item_t *t)
{

}


void Sound_Item_Load_Mem (sound_item_t *t)
{
	// We do not have sound.  Right?
}


cbool Sound_Item_Play (sound_item_t *t)
{
	// We do not have sound.  Right?

	return false; // Right?
}


///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: DISPATCH.  AT LEAST THE DEFAULT ONE.
///////////////////////////////////////////////////////////////////////////////


// This is a console app, it does not have dispatch.
// OK, got it?  Good.



///////////////////////////////////////////////////////////////////////////////
//  END DISPATCH
///////////////////////////////////////////////////////////////////////////////




#endif // _CONSOLE



