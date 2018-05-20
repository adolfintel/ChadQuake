#ifndef CORE_SDL
#include "environment.h"
#ifdef PLATFORM_WINDOWS // Has to be here, set by a header


/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2012 John Fitzgibbons and others
Copyright (C) 2009-2014 Baker and others

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// in_win.c -- windows 95 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.


#include "quakedef.h"
#include "winquake.h"

///////////////////////////////////////////////////////////////////////////////
//  PLATFORM: DISPATCH.  AT LEAST THE DEFAULT ONE.
///////////////////////////////////////////////////////////////////////////////


int Input_Local_Capture_Mouse (cbool bDoCapture)
{
	static cbool captured = false;

	if (bDoCapture && !captured)
	{
		ShowCursor (FALSE); // Hides mouse cursor
		SetCapture (sysplat.mainwindow);	// Captures mouse events
		Con_DPrintLinef ("Mouse Captured");
		captured = true;
	}

	if (!bDoCapture && captured)
	{
		ShowCursor (TRUE); // Hides mouse cursor
		ReleaseCapture ();
		ClipCursor (NULL); // Can't hurt
		Con_DPrintLinef ("Mouse Released");
		captured = false;
	}

	return 1; // Accepted
}


cbool Input_Local_Update_Mouse_Clip_Region_Think (mrect_t* mouseregion)
{
	mrect_t oldregion = *mouseregion;
	WINDOWINFO windowinfo;
	windowinfo.cbSize = sizeof (WINDOWINFO);
	GetWindowInfo (sysplat.mainwindow, &windowinfo);	// client_area screen coordinates

	// Fill in top left, bottom, right, center
	mouseregion->left = windowinfo.rcClient.left;
	mouseregion->right = windowinfo.rcClient.right;
	mouseregion->bottom = windowinfo.rcClient.bottom;
	mouseregion->top = windowinfo.rcClient.top;

	if (memcmp (mouseregion, &oldregion, sizeof(mrect_t) ) != 0)
	{  // Changed!
		mouseregion->width = mouseregion->right - mouseregion->left;
		mouseregion->height = mouseregion->bottom - mouseregion->top;
		mouseregion->center_x = (mouseregion->left + mouseregion->right) / 2;
		mouseregion->center_y = (mouseregion->top + mouseregion->bottom) / 2;
		ClipCursor (&windowinfo.rcClient);
		return true;
	}
	return false;
}

void Input_Local_Mouse_Cursor_SetPos (int x, int y)
{
	SetCursorPos (x, y);
}

void Input_Local_Mouse_Cursor_GetPos (required int *px, required int *py)
{
	POINT current_pos;
	GetCursorPos (&current_pos);

	REQUIRED_ASSIGN (px, current_pos.x);
	REQUIRED_ASSIGN (py, current_pos.y);
}



static holy_key = 0;
cbool WIN_IN_ReadInputMessages (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	cbool down = false;

	int button_bits = 0;

	switch (msg)
	{
//
// Input events
//

// Keyboard character emission
	case WM_CHAR:
//		if (holy_key) {
//			Con_DPrintLinef ("Rejected a scan code %c %d", in_range(32, wparam, 126) ? wparam : 0, wparam);
//		}
//		else 
		{
#pragma message ("TODO: wparam is UTF-16 or UCS2 or something")
			int unicode = wparam;
			int ascii 	= in_range (32, unicode, 126) ? unicode : 0;
			// We do not do control characters here.
			Key_Event_Ex (NO_WINDOW_NULL, SCANCODE_0, true, ascii, unicode, shiftbits() );  // ascii, unicode, shift
		}
		return true;

//
// Keyboard scancode emission
//

	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		down = true;  // Fall through ...

	case WM_SYSKEYUP:
	case WM_KEYUP:
		if (in_keymap.value /* off = no*/) { // 1005
			// Looking for 96.
			int key = ((int) lparam >> 16) & 255;
//			Con_PrintLinef ("lparam %d key %d", lparam, key);
			// Top position, right?
			if (key == 41) {
				Key_Event_Ex (NO_WINDOW_NULL, (KEYMAP_COUNT_512 - 1), down, /*should_emit*/  ASCII_0, /*unicode*/ UNICODE_0, shiftbits());
				Shell_Input_ResetDeadKeys ();
				holy_key = true;
				return true;
			}
			holy_key = false;
			// Otherwise keep going ...
		}
			

		if (1) {
			int theirs				= wparam;
			key_scancode_e scancode	= keymap[theirs][2];
			char instruction		= keymap[theirs][3]; // 0, E, R, N
			int ascii				= scancode;
			cbool extended			= !!((lparam >> 24) & 1);
			int is_right;
			cbool should_emit 		= down && !in_range (32, scancode, 126); // Control keys only?
			// Key combos aren't working?
			switch (instruction) {
			default:			Host_Error ("Unknown instruction");
			case_break  0 :		/* No special instruction*/
//								if (in_range(32, scancode, 126)) // Commented out testing stuff.
//									ascii = ascii; 
			case_break 'L':		if (!extended) {
									scancode = keymap[theirs][4]; // Use numpad scancode.
								}
			case_break 'E':		// If extended, emit the same but "ours" translation scancode is specified value
								if (extended) {
									scancode = keymap[theirs][4]; // Use numpad scancode.
								}

			case_break 'N':		// No emission even though scancode is in 128-255 range.
								ascii = 0;
			case_break 'R':		// Determine left vs. right
								switch (theirs) {
								default:				Host_Error ("Unknown platform scan code");
								case_break VK_CONTROL:	is_right = (lparam >> 24) & 1;			scancode = ascii = is_right ? K_RCTRL :  K_LCTRL;
								case_break VK_MENU:		is_right = (lparam >> 24) & 1;			scancode = ascii = is_right ? K_RALT :   K_LALT;
								case_break VK_SHIFT:	is_right = (lparam >> 16 & 255) == 54;	scancode = ascii = is_right ? K_RSHIFT : K_LSHIFT;
								}
			}

			// Scan code event:  Send the scancode.
			//                   ascii -- only if down AND it is a control character.
			//                      Because there is no ascii emission on up.
			//                            And ascii emission on down is ONLY for control characters
			//                              because the keymap event in WM_CHAR does the emit for ascii.
			//                   Even if we aren't using keymapping, the function being called would need to use the scancode only.
			//                   Because this is a scancode event here, that does ascii for control characters.
			if (scancode)
				Key_Event_Ex (NO_WINDOW_NULL, scancode, down, /*should_emit*/  ASCII_0, /*unicode*/ UNICODE_0, shiftbits());
		}

		return true; // handled

	case WM_MOUSEWHEEL:
		if (1) {
			cbool direction = (short)HIWORD(wparam) > 0; // short.  Not int.
			key_scancode_e scancode = direction ? K_MOUSEWHEELUP : K_MOUSEWHEELDOWN;
			// Can't remember if we are supposed to send the ascii or not for something with no ascii
			// I think NO, but not 100%
			// Figure out the right way make the function fatal error if receives something out of line
			// So we can catch stupidity.
			Key_Event_Ex (NO_WINDOW_NULL, scancode, true, ASCII_0 , UNICODE_0, CORE_SHIFTBITS_UNREAD_NEG1);
			Key_Event_Ex (NO_WINDOW_NULL, scancode, false,   ASCII_0 , UNICODE_0, CORE_SHIFTBITS_UNREAD_NEG1);
		}
		return true; // handled

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_LBUTTONUP:

	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
//	case WM_MOUSEMOVE: // Yes mouse move is in here.

		if (1) {
			int buttons, shift, x, y;
			
			getmousebits (wparam, lparam, &buttons, &shift, &x, &y);
			Input_Mouse_Button_Event (buttons);
		}
		return true; // handled


	default:
		return false; // not handled
	}
}

/*

  Joystick

*/

// joystick defines and variables
// where should defines be moved?
#define JOY_ABSOLUTE_AXIS	0x00000000		// control like a joystick
#define JOY_RELATIVE_AXIS	0x00000010		// control like a mouse, spinner, trackball
#define	JOY_MAX_AXES		6			// X, Y, Z, R, U, V
#define JOY_AXIS_X			0
#define JOY_AXIS_Y			1
#define JOY_AXIS_Z			2
#define JOY_AXIS_R			3
#define JOY_AXIS_U			4
#define JOY_AXIS_V			5



DWORD dwAxisFlags[JOY_MAX_AXES] =
{
	JOY_RETURNX, JOY_RETURNY, JOY_RETURNZ, JOY_RETURNR, JOY_RETURNU, JOY_RETURNV
};

DWORD	joyAxisMap[JOY_MAX_AXES];
DWORD	dwControlMap[JOY_MAX_AXES];
PDWORD	pdwRawValue[JOY_MAX_AXES];

// none of these cvars are saved over a session.
// this means that advanced controller configuration needs to be executed each time.
// this avoids any problems with getting back to a default usage or when changing from one controller to another.
// this way at least something works.

cbool joy_advancedinit;
static cbool	joy_haspov;
static DWORD	joy_oldbuttonstate, joy_oldpovstate;

static int		joy_id;
static DWORD	joy_flags;
static DWORD	joy_numbuttons;


static	JOYINFOEX	ji;

PDWORD RawValuePointer (int axis)
{
	switch (axis)
	{
		case JOY_AXIS_X: return &ji.dwXpos;
		case JOY_AXIS_Y: return &ji.dwYpos;
		case JOY_AXIS_Z: return &ji.dwZpos;
		case JOY_AXIS_R: return &ji.dwRpos;
		case JOY_AXIS_U: return &ji.dwUpos;
		case JOY_AXIS_V: return &ji.dwVpos;
	}

	return NULL;	// shut up compiler
}

void Input_Local_Joy_AdvancedUpdate_f (lparse_t *unused)
{

	// called once by IN_ReadJoystick and by user whenever an update is needed
	// cvars are now available
	int	i;
	DWORD	dwTemp;

	// initialize all the maps
	for (i = 0 ; i < JOY_MAX_AXES ; i++)
	{
		joyAxisMap[i] = eAxisNone;
		dwControlMap[i] = JOY_ABSOLUTE_AXIS;
		pdwRawValue[i] = RawValuePointer(i);
	}

	if (!joy_advanced.value)
	{
		// default joystick initialization
		// 2 axes only with joystick control
		joyAxisMap[JOY_AXIS_X] = eAxisTurn;
		// dwControlMap[JOY_AXIS_X] = JOY_ABSOLUTE_AXIS;
		joyAxisMap[JOY_AXIS_Y] = eAxisForward;
		// dwControlMap[JOY_AXIS_Y] = JOY_ABSOLUTE_AXIS;
	}
	else
	{
		if (strcmp (joy_name.string, "joystick"))
		{
			// notify user of advanced controller
			Con_PrintLinef (NEWLINE "%s configured" NEWLINE, joy_name.string);
		}

		// advanced initialization here
		// data supplied by user via joy_axisn cvars
		dwTemp = (DWORD) joy_advaxisx.value;
		joyAxisMap[JOY_AXIS_X] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_X] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisy.value;
		joyAxisMap[JOY_AXIS_Y] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Y] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisz.value;
		joyAxisMap[JOY_AXIS_Z] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Z] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisr.value;
		joyAxisMap[JOY_AXIS_R] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_R] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisu.value;
		joyAxisMap[JOY_AXIS_U] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_U] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisv.value;
		joyAxisMap[JOY_AXIS_V] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_V] = dwTemp & JOY_RELATIVE_AXIS;
	}

	// compute the axes to collect from DirectInput
	joy_flags = JOY_RETURNCENTERED | JOY_RETURNBUTTONS | JOY_RETURNPOV;
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		if (joyAxisMap[i] != eAxisNone)
			joy_flags |= dwAxisFlags[i];
	}
}

cbool Input_Local_Joystick_Startup (void)
{
	int		numdevs;
	JOYCAPS		jc;
	MMRESULT	mmr;

	// verify joystick driver is present
	if ((numdevs = joyGetNumDevs ()) == 0)
	{
		Con_PrintLinef (NEWLINE "joystick not found -- driver not present" NEWLINE);
		return false;
	}

	// cycle through the joystick ids for the first valid one
	for (mmr = -1, joy_id = 0 ; joy_id < numdevs ; joy_id++)
	{
		memset (&ji, 0, sizeof(ji));
		ji.dwSize = sizeof(ji);
		ji.dwFlags = JOY_RETURNCENTERED;

		if ((mmr = joyGetPosEx(joy_id, &ji)) == JOYERR_NOERROR)
			break;
	}

	// abort startup if we didn't find a valid joystick
	if (mmr != JOYERR_NOERROR)
	{
		Con_PrintLinef ("joystick not found -- no valid joysticks (%x)", mmr);
		return false;
	}

	// get the capabilities of the selected joystick
	// abort startup if command fails
	memset (&jc, 0, sizeof(jc));
	if ((mmr = joyGetDevCaps(joy_id, &jc, sizeof(jc))) != JOYERR_NOERROR)
	{
		Con_PrintLinef ("joystick not found -- invalid joystick capabilities (%x)", mmr);
		return false;
	}

	// save the joystick's number of buttons and POV status
	joy_numbuttons = jc.wNumButtons;
	joy_haspov = jc.wCaps & JOYCAPS_HASPOV;

	// old button and POV states default to no buttons pressed
	joy_oldbuttonstate = joy_oldpovstate = 0;

	// mark the joystick as available and advanced initialization not completed
	// this is needed as cvars are not available during initialization

	joy_advancedinit = false;
#pragma message ("Command rogues")
	Cmd_AddCommands ((voidfunc_t)Input_Local_Joystick_Startup); // Warning because Input_Local_Joystick_Startup is cbool return not void

	Con_PrintLinef (NEWLINE "joystick detected" NEWLINE);
	return true;
}



void Input_Local_Joystick_Commands (void)
{
	int	i, key_index;
	DWORD	buttonstate, povstate;

	if (!joy_avail)
		return;

	// loop through the joystick buttons
	// key a joystick event or auxillary event for higher number buttons for each state change
	buttonstate = ji.dwButtons;
	for (i = 0 ; i < (int)joy_numbuttons ; i++)
	{
		if ((buttonstate & (1 << i)) && !(joy_oldbuttonstate & (1 << i)))
		{
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			//            wdo scancode down
			Key_Event_Ex (NO_WINDOW_NULL, key_index + i, true, ASCII_0,  UNICODE_0, CORE_SHIFTBITS_UNREAD_NEG1);  // ascii, unicode, shift
		}

		if (!(buttonstate & (1 << i)) && (joy_oldbuttonstate & (1 << i)))
		{
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			//            wdo scancode down
			Key_Event_Ex (NO_WINDOW_NULL, key_index + i, false, ASCII_0, UNICODE_0, CORE_SHIFTBITS_UNREAD_NEG1);

		}
	}
	joy_oldbuttonstate = buttonstate;

	if (joy_haspov)
	{
		// convert POV information into 4 bits of state information
		// this avoids any potential problems related to moving from one
		// direction to another without going through the center position
		povstate = 0;
		if(ji.dwPOV != JOY_POVCENTERED)
		{
			if (ji.dwPOV == JOY_POVFORWARD)
				povstate |= 0x01;
			if (ji.dwPOV == JOY_POVRIGHT)
				povstate |= 0x02;
			if (ji.dwPOV == JOY_POVBACKWARD)
				povstate |= 0x04;
			if (ji.dwPOV == JOY_POVLEFT)
				povstate |= 0x08;
		}
		// determine which bits have changed and key an auxillary event for each change
		for (i=0 ; i<4 ; i++)
		{
			if ((povstate & (1 << i)) && !(joy_oldpovstate & (1 << i)))
				Key_Event_Ex (NO_WINDOW_NULL, K_AUX29 + i, true, ASCII_0, UNICODE_0, CORE_SHIFTBITS_UNREAD_NEG1);
			if (!(povstate & (1 << i)) && (joy_oldpovstate & (1 << i)))
				Key_Event_Ex (NO_WINDOW_NULL, K_AUX29 + i, false, ASCII_0, UNICODE_0, CORE_SHIFTBITS_UNREAD_NEG1);

		}
		joy_oldpovstate = povstate;
	}
}

cbool Input_Local_Joystick_Read (void)
{
	memset (&ji, 0, sizeof(ji));

	ji.dwSize = sizeof(ji);
	ji.dwFlags = joy_flags;

	if (joyGetPosEx(joy_id, &ji) == JOYERR_NOERROR)
	{
		// this is a hack -- there is a bug in the Logitech WingMan Warrior DirectInput Driver
		// rather than having 32768 be the zero point, they have the zero point at 32668
		// go figure -- anyway, now we get the full resolution out of the device
		if (joy_wwhack1.value != 0.0)
			ji.dwUpos += 100;

		return true;
	}
	else
	{
		// read error occurred
		// turning off the joystick seems too harsh for 1 read error,\
		// but what should be done?
		// Con_PrintLinef ("IN_ReadJoystick: no response");
		// joy_avail = false;
		return false;
	}
}



void Input_Local_Joystick_Shutdown (void)
{
	// We don't have to do anything here

}


void Input_Local_Init (void)
{

}

void Input_Local_Shutdown (void)
{

}


// Baker: On Windows these might not only be key events.
void Input_Local_SendKeyEvents (void)
{
    MSG        msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
	{
	// we always update if there are any event, even if we're paused
		scr_skipupdate = 0;

		if (!GetMessage (&msg, NULL, 0, 0))
			System_Quit ();

      	TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}
}

// Baker: Stops drag flag on Mac (when activation is received by a mouseclick on title bar and user drags it.
//  On Windows do this too.
void Input_Local_Deactivate (void)
{


}

#endif // PLATFORM_WINDOWS

#endif // !CORE_SDL