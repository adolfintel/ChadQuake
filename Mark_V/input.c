/*
Copyright (C) 2013 Baker

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
// input.c -- input


#include "quakedef.h"

#ifdef INPUT_RELATIVE
	int input_accum_x, input_accum_y;
	/// #define Input_Local_Mouse_Cursor_SetPos // Baker: wicked evil ... but not just yet
#endif // INPUT_RELATIVE

// How do we convert this to Quake?
keyvalue_t key_scancodes_table [108] = {
	{ "BACKSPACE",      K_BACKSPACE         },
	{ "TAB",            K_TAB               },
	{ "ENTER",          K_ENTER             },
	{ "ESCAPE",         K_ESCAPE            },
	{ "SPACE",          K_SPACE             },
	{ "SEMICOLON",      K_SEMICOLON         },
	{ "TILDE",          K_GRAVE             },
	{ "LCTRL",          K_LCTRL             },
	{ "RCTRL",          K_RCTRL             },
	{ "LALT",           K_LALT              },
	{ "RALT",           K_RALT              },
	{ "LSHIFT",         K_LSHIFT            },
	{ "RSHIFT",         K_RSHIFT            },
	{ "LWIN",           K_LWIN              },
	{ "RWIN",           K_RWIN              },
	{ "MENU",           K_MENU              },
	{ "CAPSLOCK",       K_CAPSLOCK          },
	{ "NUMLOCK",        K_NUMLOCK           }, // Is same as keypad numlock or is different key?
	{ "SCROLLLOCK",     K_SCROLLLOCK        },
	{ "PAUSE",          K_PAUSE             },
	{ "PRINTSCREEN",    K_PRINTSCREEN       },
	{ "INS",            K_INSERT            },
	{ "DEL",            K_DELETE            },
	{ "LEFTARROW",      K_LEFTARROW         },
	{ "RIGHTARROW",     K_RIGHTARROW        },
	{ "UPARROW",        K_UPARROW           },
	{ "DOWNARROW",      K_DOWNARROW         },
	{ "PGUP",           K_PAGEUP            },
	{ "PGDN",           K_PAGEDOWN          },
	{ "HOME",           K_HOME              },
	{ "END",            K_END               },
	{ "F1",             K_F1                },
	{ "F2",             K_F2                },
	{ "F3",             K_F3                },
	{ "F4",             K_F4                },
	{ "F5",             K_F5                },
	{ "F6",             K_F6                },
	{ "F7",             K_F7                },
	{ "F8",             K_F8                },
	{ "F9",             K_F9                },
	{ "F10",            K_F10               },
	{ "F11",            K_F11               },
	{ "F12",            K_F12               },

	{ "KP_0",           K_NUMPAD_0          },	// Emission
	{ "KP_1",           K_NUMPAD_1          },
	{ "KP_2",           K_NUMPAD_2          },
	{ "KP_3",           K_NUMPAD_3          },
	{ "KP_4",           K_NUMPAD_4          },
	{ "KP_5",           K_NUMPAD_5          },
	{ "KP_6",           K_NUMPAD_6          },
	{ "KP_7",           K_NUMPAD_7          },
	{ "KP_8",           K_NUMPAD_8          },
	{ "KP_9",           K_NUMPAD_9          },
	{ "KP_MULTIPLY",    K_NUMPAD_MULTIPLY   },
	{ "KP_PLUS",        K_NUMPAD_PLUS       },
	{ "KP_SEPARATOR",   K_NUMPAD_SEPARATOR  },
	{ "KP_MINUS",       K_NUMPAD_MINUS      },
	{ "KP_PERIOD",      K_NUMPAD_PERIOD     },
	{ "KP_DIVIDE",      K_NUMPAD_DIVIDE     },
#if 0 // Disallow, I think - Build 1010
	{ "KP_ENTER",		K_NUMPAD_ENTER		},	// Emission!!!!
#endif
	{ "MOUSE1",         K_MOUSE1            },
	{ "MOUSE2",         K_MOUSE2            },
	{ "MOUSE3",         K_MOUSE3            },
	{ "MOUSE4",         K_MOUSE4            },
	{ "MOUSE5",         K_MOUSE5            },

	{ "MWHEELUP",       K_MOUSEWHEELUP      },
	{ "MWHEELDOWN",     K_MOUSEWHEELDOWN    },
	{ "JOY1",           K_JOY1              },
	{ "JOY2",           K_JOY2              },
	{ "JOY3",           K_JOY3              },
	{ "JOY4",           K_JOY4              },
	{ "AUX1",           K_AUX1              },
	{ "AUX2",           K_AUX2              },
	{ "AUX3",           K_AUX3              },
	{ "AUX4",           K_AUX4              },
	{ "AUX5",           K_AUX5              },
	{ "AUX6",           K_AUX6              },
	{ "AUX7",           K_AUX7              },
	{ "AUX8",           K_AUX8              },
	{ "AUX9",           K_AUX9              },
	{ "AUX10",          K_AUX10             },
	{ "AUX11",          K_AUX11             },
	{ "AUX12",          K_AUX12             },
	{ "AUX13",          K_AUX13             },
	{ "AUX14",          K_AUX14             },
	{ "AUX15",          K_AUX15             },
	{ "AUX16",          K_AUX16             },
	{ "AUX17",          K_AUX17             },
	{ "AUX18",          K_AUX18             },
	{ "AUX19",          K_AUX19             },
	{ "AUX20",          K_AUX20             },
	{ "AUX21",          K_AUX21             },
	{ "AUX22",          K_AUX22             },
	{ "AUX23",          K_AUX23             },
	{ "AUX24",          K_AUX24             },
	{ "AUX25",          K_AUX25             },
	{ "AUX26",          K_AUX26             },
	{ "AUX27",          K_AUX27             },
	{ "AUX28",          K_AUX28             },
	{ "AUX29",          K_AUX29             },
	{ "AUX30",          K_AUX30             },
	{ "AUX31",          K_AUX31             },
	{ "AUX32",          K_AUX32             },
NULL, 0}; // Null term


void Input_Force_CenterView_f (lparse_t *unnused) { cl.viewangles[PITCH] = 0; }


typedef enum
{
	input_none,
	input_have_keyboard,
	input_have_mouse_keyboard,
	input_have_windowskey,
} input_state_t;

typedef struct
{
	input_state_t	current_state;
	cbool		initialized, have_mouse, have_keyboard;
	cbool		disabled_windows_key;

// Internals
	mrect_t			mouse_clip_screen_rect;
	int				mouse_accum_x, mouse_accum_y;
	int				mouse_old_button_state;
} inp_info_t;


#define MRECT_PRINT(_x) _x.left, _x.top, _x.right, _x.bottom, _x.center_x, _x.center_y
enum { GET_IT = 1, LOSE_IT = 2 };


keyvalue_t input_state_text [] =
{
	KEYVALUE (input_none),
	KEYVALUE (input_have_keyboard),
	KEYVALUE (input_have_mouse_keyboard),
NULL, 0 };  // NULL termination

static inp_info_t inps;


void Input_Info_f (void)
{
	Con_PrintLinef ("IN Info ...");
	Con_PrintLinef ("%-25s :  %s", "current_state", KeyValue_GetKeyString (input_state_text, inps.current_state) );
	Con_PrintLinef ("%-25s :  %d", "initialized", inps.initialized);
	Con_PrintLinef ("%-25s :  %d", "have_mouse", inps.have_mouse);
	Con_PrintLinef ("%-25s :  %d", "have_keyboard", inps.have_keyboard);
	Con_PrintLinef ("%-25s :  %d", "disabled_windows_key", inps.disabled_windows_key);
	Con_PrintLinef ("%-25s :  (%d, %d)-(%d, %d) center: %d, %d", "mouse_clip_screen_rect:", MRECT_PRINT(inps.mouse_clip_screen_rect) );
	Con_PrintLinef ("%-25s :  %d", "mouse_accum_x", inps.mouse_accum_x);
	Con_PrintLinef ("%-25s :  %d", "mouse_accum_y", inps.mouse_accum_y);
	Con_PrintLinef ("%-25s :  %d", "mouse_old_button_state", inps.mouse_old_button_state);
}

#pragma message ("OS X mouse input has to be purely event oriented, we can't just nab the screen at any given time")
#ifdef PLATFORM_OSX
void Input_Think (void) { }
#else
void Input_Think (void)
{
	input_state_t	newstate = (inps.initialized && vid.ActiveApp && !vid.Minimized && !vid.Hidden) ? input_have_keyboard : input_none;
	cbool		windowed_mouse_grab = !cl.paused && !console1.forcedup && ( key_dest == key_game  || key_dest == key_message || (key_dest == key_menu && m_keys_bind_grab));
	cbool		mouse_grab = (vid.screen.type == MODE_FULLSCREEN || windowed_mouse_grab);

	cbool		disable_windows_key = input_have_keyboard && vid.screen.type == MODE_FULLSCREEN;
//	cbool		can_mouse_track = inps.initialized && !vid.Minimized && !vid.Hidden) && dont have mouse

	if (disable_windows_key != inps.disabled_windows_key)
	{
		switch (disable_windows_key)
		{
		case true:
			if (vid.system_enhanced_keys) Shell_Input_KeyBoard_Capture (true /*capture*/, false /*act on stickey*/, vid.screen.type == MODE_FULLSCREEN /*act on windows key*/);
			break;
		case false:
			if (vid.system_enhanced_keys) Shell_Input_KeyBoard_Capture (false /*capture*/, false /*act on stickey*/, vid.screen.type == MODE_FULLSCREEN /*act on windows key*/);
			break;
		}

		inps.disabled_windows_key = disable_windows_key;
	}

	// newstate upgrades from should have "keyboard" to should have "mouse"
	// If the key_dest is game or we are binding keys in the menu
	if (newstate == input_have_keyboard && mouse_grab && in_nomouse.value == 0 && vid.nomouse == 0)
		newstate = input_have_mouse_keyboard;

#if 0
	Con_PrintLinef ("current_state: %s (init %d active %d mini %d)", Keypair_String (input_state_text, inps.current_state),
		inps.initialized, vid.ActiveApp, vid.Minimized);
#endif

	if (newstate != inps.current_state)
	{ // New state.
		char	mouse_action	= ( newstate == input_have_mouse_keyboard && inps.have_mouse == false) ? GET_IT :  (( newstate != input_have_mouse_keyboard && inps.have_mouse == true) ? LOSE_IT : 0);
		char	keyboard_action = ( newstate != input_none && inps.have_keyboard == false) ? GET_IT :  (( newstate == input_none && inps.have_keyboard == true) ? LOSE_IT : 0);

#if 0
		Con_PrintLinef ("State change");
#endif

		switch (keyboard_action)
		{
		case GET_IT:
			// Sticky keys
			if (vid.system_enhanced_keys) Shell_Input_KeyBoard_Capture (true /*capture*/, true /*act on stickey*/, vid.screen.type == MODE_FULLSCREEN /*act on windows key*/);

			inps.have_keyboard = true;
			break;

		case LOSE_IT:
			// Note we still need our key ups when entering the console
			// Sticky keys, Window key reenabled

			if (vid.system_enhanced_keys) Shell_Input_KeyBoard_Capture (false, true /*act on stickey*/, vid.screen.type == MODE_FULLSCREEN);
			// Key ups

			inps.have_keyboard = false;
			break;
		}

		switch (mouse_action)
		{
		case GET_IT:

			// Load window screen coords to mouse_clip_screen_rect
			// And clip the mouse cursor to that area
			Input_Local_Update_Mouse_Clip_Region_Think (&inps.mouse_clip_screen_rect);

			// Hide the mouse cursor and attach it
			Input_Local_Capture_Mouse (true);

			// Center the mouse on-screen
			Input_Local_Mouse_Cursor_SetPos (inps.mouse_clip_screen_rect.center_x, inps.mouse_clip_screen_rect.center_y);

			// Clear movement accumulation
			inps.mouse_accum_x = inps.mouse_accum_y = 0;

			inps.have_mouse = true;
			break;

		case LOSE_IT:
			// Baker: We have to release the mouse buttons because we can no longer receive
			// mouse up events.
			Key_Release_Mouse_Buttons ();

			// Release it somewhere out of the way
			Input_Local_Mouse_Cursor_SetPos (inps.mouse_clip_screen_rect.right - 80, inps.mouse_clip_screen_rect.top + 80);

			// Release the mouse and show the cursor.  Also unclips mouse.
			Input_Local_Capture_Mouse (false);

			// Clear movement accumulation and buttons
			inps.mouse_accum_x = inps.mouse_accum_y = inps.mouse_old_button_state = 0;

			inps.have_mouse = false;
			break;
		}
		inps.current_state = newstate;
	}

	if (inps.have_mouse && Input_Local_Update_Mouse_Clip_Region_Think (&inps.mouse_clip_screen_rect) == true)
	{
		// Re-center the mouse cursor and clear mouse accumulation
		Input_Local_Mouse_Cursor_SetPos (inps.mouse_clip_screen_rect.center_x, inps.mouse_clip_screen_rect.center_y);
		inps.mouse_accum_x = inps.mouse_accum_y = 0;
	}

	// End of function
}


void Input_Mouse_Button_Event (int mstate)
{
	// Why was menu commented out?
	if (inps.have_mouse || (key_dest == key_menu && m_keys_bind_grab) )
	{  // perform button actions
		int i;
		for (i = 0 ; i < INPUT_NUM_MOUSE_BUTTONS ; i ++)
		{
			int button_bit = (1 << i);
			cbool button_pressed  =  (mstate & button_bit) && !(inps.mouse_old_button_state & button_bit);
			cbool button_released = !(mstate & button_bit) &&  (inps.mouse_old_button_state & button_bit);

			if (button_pressed || button_released) {
#ifdef PLATFORM_OSX // Crusty Mac
				Key_Event (K_MOUSE1 + i, button_pressed ? true : false, 0 /*not special*/);
#else // Crusty Mac ^^
				Key_Event_Ex (NO_WINDOW_NULL, K_MOUSE1 + i, button_pressed ? true : false, ASCII_0, UNICODE_0, CORE_SHIFTBITS_UNREAD_NEG1);
#endif
			}

		}
		inps.mouse_old_button_state = mstate;
	}
}

// This re-centers the mouse, so it means more than simple build-up of accumulation alone.
// S_ExtraUpdate calls this, which is called several places.
// The only other caller is Input_Mouse_Move (us!)
// In perfect work, something like DirectInput would always be used making this unnecessary.
void Input_Mouse_Accumulate (void)
{
	static int last_key_dest;
	int new_mouse_x, new_mouse_y;

	Input_Think ();


	if (inps.have_mouse)
	{
		cbool nuke_mouse_accum = false;

		// Special cases: fullscreen doesn't release mouse so doesn't clear accum
		// when entering/exiting the console.  I consider those input artifacts.  Also
		// we simply don't want accum from fullscreen if not key_dest == key_game.
		if (vid.screen.type == MODE_FULLSCREEN)
		{
			if (cl.paused)
				nuke_mouse_accum = true;
			else
			{
				cbool in_game_or_message = (key_dest == key_game || key_dest == key_message);
				cbool was_in_game_or_message = (last_key_dest == key_game || last_key_dest == key_message);
				cbool entered_game_or_message = in_game_or_message && !was_in_game_or_message;
				if (entered_game_or_message || !in_game_or_message)
					nuke_mouse_accum = true;
			}
		}

#ifdef INPUT_RELATIVE
		inps.mouse_accum_x += input_accum_x; input_accum_x = 0;
		inps.mouse_accum_y += input_accum_y; input_accum_y = 0;
#else // ^^^ INPUT_RELATIVE
		Input_Local_Mouse_Cursor_GetPos (&new_mouse_x, &new_mouse_y); // GetCursorPos (&current_pos);

		inps.mouse_accum_x += new_mouse_x - inps.mouse_clip_screen_rect.center_x;
		inps.mouse_accum_y += new_mouse_y - inps.mouse_clip_screen_rect.center_y;
#endif // !INPUT_RELATIVE

		// Re-center the mouse cursor
		Input_Local_Mouse_Cursor_SetPos (inps.mouse_clip_screen_rect.center_x, inps.mouse_clip_screen_rect.center_y);

		if (nuke_mouse_accum)
			inps.mouse_accum_x = inps.mouse_accum_y = 0;
	}
	last_key_dest = key_dest;
}

void Input_Mouse_Move (usercmd_t *cmd)
{
	Input_Mouse_Accumulate ();

	if (inps.mouse_accum_x || inps.mouse_accum_y)
	{
		int	mouse_x = inps.mouse_accum_x *= sensitivity.value;
		int mouse_y = inps.mouse_accum_y *= sensitivity.value;
	// add mouse X/Y movement to cmd
		if ( (in_strafe.state & 1) || (lookstrafe.value && MOUSELOOK_ACTIVE ))
			cmd->sidemove += m_side.value * mouse_x;
		else cl.viewangles[YAW] -= m_yaw.value * mouse_x;

		if (MOUSELOOK_ACTIVE)
			View_StopPitchDrift ();

		if ( MOUSELOOK_ACTIVE && !(in_strafe.state & 1))
		{
			cl.viewangles[PITCH] += m_pitch.value * mouse_y;

			CL_BoundViewPitch (cl.viewangles);
		}
		else
		{
			if ((in_strafe.state & 1) && cl.noclip_anglehack)
				cmd->upmove -= m_forward.value * mouse_y;
			else cmd->forwardmove -= m_forward.value * mouse_y;
		}
		inps.mouse_accum_x = inps.mouse_accum_y = 0;
	}
}
#endif // !PLATFORM_OSX

void Input_Move (usercmd_t *cmd)
{
    Input_Mouse_Move (cmd);
    Input_Joystick_Move (cmd);
}


cbool joy_avail;

/*
===========
IN_JoyMove
===========
*/

//#ifdef _WIN32
//#include "winquake.h"
//#endif // _WIN32

void Input_Joystick_Move (usercmd_t *cmd)
{
}

void Input_Commands (void)
{
#ifdef PLATFORM_OSX
	Key_Console_Repeats ();
#endif // PLATFORM_OSX
	Input_Local_Joystick_Commands ();

}

void Input_Joystick_Init (void)
{
	// joystick variables
 	// assume no joystick
	joy_avail = Input_Local_Joystick_Startup();

	if (joy_avail)
	{
		Cmd_AddCommands (Input_Joystick_Init);

	}
}

void Input_Init (void)
{
	Cmd_AddCommands (Input_Init);

#pragma message ("Baker: Implement m_filter on Windows")

	// This doesn't work because the config.cfg will be read and just override it.
	// Now we do it earlier -- see video startup --- no we had to bail on that, now we use command line parm
	//if (COM_CheckParm ("-nomouse"))
	//	Cvar_SetValueQuick (&in_nomouse, 1);

	if (!COM_CheckParm ("-nojoy"))
		Input_Joystick_Init ();

	Input_Local_Init (); // Mac

	inps.initialized = true;
	Input_Think ();
	Con_PrintLinef ("Input initialized");
}

void Input_Shutdown (void)
{
	Input_Local_Shutdown (); // Mac

	inps.initialized = false;
	Input_Think (); // Will shut everything off
}

void Input_System_Enhanced_Keys_Changed (cvar_t *var)
{
	// Too late, remember this reads early in SND_Read_Early_Cvars
	if (host_post_initialized) {
		Con_PrintLinef ("System enhanced keys changed.  Requires engine restart to take effect.");
	}
}
