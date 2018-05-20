/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
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
// keys_mac_old.h

#ifndef __KEYS_MAC_OLD_H__
#define __KEYS_MAC_OLD_H__


#ifdef PLATFORM_OSX // Crusty
	#define K_SEMICOLON			59
	#define K_GRAVE				96
	#define K_RCTRL				0	// Not used in practice at this time
	#define K_RALT				0	// Not used in practice at this time
	#define K_RSHIFT			0	// Not used in practice at this time
	#define K_RWIN				0	// Not used in practice at this time
	#define K_CAPSLOCK			0	// Too bad, so sad
	#define K_MENU				0	// Too bad, so sad
	#define K_SCROLLLOCK		0	// Too bad, so sad
	#define K_PRINTSCREEN		0	// Too bad, so sad
	#define K_NUMPAD_SEPARATOR	0	// Too bad, so sad
#endif
//
// these are the key numbers that should be passed to Key_Event
//
#define	K_TAB			9
#define	K_ENTER			13
#define	K_ESCAPE		27
#define	K_SPACE			32

// normal keys should be passed as lowercased ascii

#define	K_BACKSPACE		127
#define	K_UPARROW		128
#define	K_DOWNARROW		129
#define	K_LEFTARROW		130
#define	K_RIGHTARROW	131

#define	K_LSHIFT		132
#define	K_LCTRL			133
#define K_LWIN			134 // K_OPTION physical spot (unless you plug a PC keyboard in, then they are reversed)
#define	K_LALT			135 // K_COMMAND physical spot (unless you plug a PC keyboard in, then they are reversed)
#define	K_F1			136
#define	K_F2			137
#define	K_F3			138
#define	K_F4			139
#define	K_F5			140
#define	K_F6			141
#define	K_F7			142
#define	K_F8			143
#define	K_F9			144
#define	K_F10			145
#define	K_F11			146
#define	K_F12			147
#define	K_F13			148 // Mac keyboard
#define	K_F14			149 // Mac keyboard
#define	K_F15			150 // Mac keyboard
#define	K_INSERT		151
#define	K_DELETE		152
#define	K_PAGEDOWN		153
#define	K_PAGEUP		154
#define	K_HOME			155
#define	K_END			156


#define	K_NUMLOCK			157		// K_KP_NUMLOCK
#define	K_NUMPAD_DIVIDE		158		// K_KP_SLASH		
#define	K_NUMPAD_MULTIPLY	159		// K_KP_STAR
#define	K_NUMPAD_MINUS		160		// K_KP_MINUS

#define	K_NUMPAD_PLUS		164		// KP_PLUS
//#define	K_KP_ENTER		165		// Too bad, so sad.
#define	K_NUMPAD_PERIOD		166		// K_KP_DEL

#define	K_NUMPAD_0		167 // KP 0		K_KP_INS
#define	K_NUMPAD_1		168 // KP 1		K_KP_END
#define	K_NUMPAD_2		169 // KP 2		K_KP_DOWNARROW
#define	K_NUMPAD_3		170 // KP 3		K_KP_PGDN
#define	K_NUMPAD_4		171 // KP 4		K_KP_LEFTARROW
#define	K_NUMPAD_5		172 // KP 5		K_KP_5
#define	K_NUMPAD_6		173 // KP 6		K_KP_RIGHTARROW
#define	K_NUMPAD_7		174 // KP 7		K_KP_HOME
#define	K_NUMPAD_8		175 // KP 8		K_KP_UPARROW
#define	K_NUMPAD_9		176 // KP 9		K_KP_PGUP			//K_KP_PGUP




//
// mouse buttons generate virtual keys
//
#define	K_MOUSE1		180
#define	K_MOUSE2		181
#define	K_MOUSE3		182
// thumb buttons
#define K_MOUSE4		183
#define K_MOUSE5		184

//
// joystick buttons
//
#define	K_JOY1			203
#define	K_JOY2			204
#define	K_JOY3			205
#define	K_JOY4			206
// aux keys are for multi-buttoned joysticks to generate so they can use
// the normal binding process
// aux29-32: reserved for the HAT (POV) switch motion
#define	K_AUX1			207
#define	K_AUX2			208
#define	K_AUX3			209
#define	K_AUX4			210
#define	K_AUX5			211
#define	K_AUX6			212
#define	K_AUX7			213
#define	K_AUX8			214
#define	K_AUX9			215
#define	K_AUX10			216
#define	K_AUX11			217
#define	K_AUX12			218
#define	K_AUX13			219
#define	K_AUX14			220
#define	K_AUX15			221
#define	K_AUX16			222
#define	K_AUX17			223
#define	K_AUX18			224
#define	K_AUX19			225
#define	K_AUX20			226
#define	K_AUX21			227
#define	K_AUX22			228
#define	K_AUX23			229
#define	K_AUX24			230
#define	K_AUX25			231
#define	K_AUX26			232
#define	K_AUX27			233
#define	K_AUX28			234
#define	K_AUX29			235
#define	K_AUX30			236
#define	K_AUX31			237
#define	K_AUX32			238

// JACK: Intellimouse(c) Mouse Wheel Support

#define K_MOUSEWHEELUP		239
#define K_MOUSEWHEELDOWN	240

#define K_PAUSE			255

#endif // ! __KEYS_MAC_OLD_H__

