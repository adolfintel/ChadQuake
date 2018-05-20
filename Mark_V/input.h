/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
Copyright (C) 2009-2014 Baker and others

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

#ifndef __INPUT_H__
#define __INPUT_H__

// input.h -- external (non-keyboard) input devices

void Input_Init (void);
void Input_Shutdown (void);
void Input_Think (void);
void Input_Move (usercmd_t *cmd);
void Input_Mouse_Move (usercmd_t *cmd);
void Input_Joystick_Move (usercmd_t *cmd);
void Input_Mouse_Accumulate (void); // Accumulates and re-centers only (for slow frames)
void Input_Mouse_Button_Event (int mstate);

void Input_System_Enhanced_Keys_Changed (cvar_t *var);

//cmd void Input_Info_f (void);

// Platform
void Input_Local_Init (void);
void Input_Local_Shutdown (void);
int Input_Local_Capture_Mouse (cbool bDoCapture); // Returns 0 if capture rejected (Mac)
//void Input_Local_Keyboard_Disable_Sticky_Keys (cbool bDoDisable);   Shell_Input_KeyBoard_Capture
//void Input_Local_Keyboard_Disable_Windows_Key (cbool bDoDisable);
void Input_Local_Mouse_Cursor_SetPos (int x, int y);
void Input_Local_Mouse_Cursor_GetPos (required int *px, required int *py);
cbool Input_Local_Update_Mouse_Clip_Region_Think (mrect_t *mouseregion);

void Input_Local_Deactivate (void); // Stops drag flag

#define INPUT_NUM_MOUSE_BUTTONS 5

typedef enum
{
    eAxisNone,
    eAxisForward,
    eAxisSide,
    eAxisTurn,
    eAxisLook,
#if 1 // Baker: Mac didn't have
	eAxisFly,
	eNumAxis_6,
#endif
} in_axismap_t;

#define	JOY_MAX_AXES 6 // X, Y, Z, R, U, V

extern cbool joy_avail;

void Input_Joystick_Init (void);
void Input_Commands (void); // Baker: Windows uses for joystick, Mac for fake console keyrepeats

void Input_Local_Joystick_Commands (void);
void Input_Local_Joy_AdvancedUpdate_f (lparse_t *unused);
cbool Input_Local_Joystick_Startup (void);
cbool Input_Local_Joystick_Read (void);

#ifdef CORE_SDL
	#define INPUT_RELATIVE
	//#pragma message ("INPUT_RELATIVE defined")
#endif // CORE_SDL

#ifdef INPUT_RELATIVE
	extern int input_accum_x, input_accum_y;
#endif // INPUT_RELATIVE

#define MOUSELOOK_ACTIVE (in_freelook.value || (in_mlook.state & 1))

void Input_Local_SendKeyEvents (void);
// Perform Key_Event () callbacks until the input que is empty

#endif	// ! __INPUT_H__

