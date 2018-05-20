//#ifdef CORE_SDL
//
///*
//Copyright (C) 1996-2001 Id Software, Inc.
//Copyright (C) 2002-2009 John Fitzgibbons and others
//
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//See the GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//*/
//
//#include "quakedef.h"
//#include "sdlquake.h"
//
//										//  dedicated before exiting
//byte key_map[SDL_NUM_SCANCODES /*SDLK_LAST*/];
//int num_key_map = sizeof(key_map) / sizeof(key_map[0]);
//
//void BuildKeyMaps()
//{
//    int i;
//
//    for (i = 0; i < num_key_map; i++)
//        key_map[i] = 0;
//
//	key_map[SDLK_BACKSPACE]     = K_BACKSPACE;
//	key_map[SDLK_TAB]           = K_TAB;
//	key_map[SDLK_RETURN]        = K_ENTER;
////	key_map[SDLK_PAUSE]         = K_PAUSE; // Why not?
//	key_map[SDLK_ESCAPE]        = K_ESCAPE;
//	key_map[SDLK_SPACE]         = K_SPACE;
//	key_map[SDLK_EXCLAIM]       = '!';
//	key_map[SDLK_QUOTEDBL]      = '"';
//	key_map[SDLK_HASH]          = '#';
//	key_map[SDLK_DOLLAR]        = '$';
//	key_map[SDLK_AMPERSAND]     = '&';
//	key_map[SDLK_QUOTE]         = '\'';
//	key_map[SDLK_LEFTPAREN]     = '(';
//	key_map[SDLK_RIGHTPAREN]    = ')';
//	key_map[SDLK_ASTERISK]      = '*';
//	key_map[SDLK_PLUS]          = '+';
//	key_map[SDLK_COMMA]         = ',';
//	key_map[SDLK_MINUS]         = '-';
//	key_map[SDLK_PERIOD]        = '.';
//	key_map[SDLK_SLASH]         = '/';
//
//	key_map[SDLK_0]             = '0';
//	key_map[SDLK_1]             = '1';
//	key_map[SDLK_2]             = '2';
//	key_map[SDLK_3]             = '3';
//	key_map[SDLK_4]             = '4';
//	key_map[SDLK_5]             = '5';
//	key_map[SDLK_6]             = '6';
//	key_map[SDLK_7]             = '7';
//	key_map[SDLK_8]             = '8';
//	key_map[SDLK_9]             = '9';
//
//	key_map[SDLK_COLON]         = ':';
//	key_map[SDLK_SEMICOLON]     = ';';
//	key_map[SDLK_LESS]          = '<';
//	key_map[SDLK_EQUALS]        = '=';
//	key_map[SDLK_GREATER]       = '>';
//	key_map[SDLK_QUESTION]      = '?';
//	key_map[SDLK_AT]            = '@';
//	key_map[SDLK_LEFTBRACKET]   = '[';
//	key_map[SDLK_BACKSLASH]     = '\\';
//	key_map[SDLK_RIGHTBRACKET]  = ']';
//	key_map[SDLK_CARET]         = '^';
//	key_map[SDLK_UNDERSCORE]    = '_';
//	key_map[SDLK_BACKQUOTE]     = '`';
//
//	key_map[SDLK_a]             = 'a';
//	key_map[SDLK_b]             = 'b';
//	key_map[SDLK_c]             = 'c';
//	key_map[SDLK_d]             = 'd';
//	key_map[SDLK_e]             = 'e';
//	key_map[SDLK_f]             = 'f';
//	key_map[SDLK_g]             = 'g';
//	key_map[SDLK_h]             = 'h';
//	key_map[SDLK_i]             = 'i';
//	key_map[SDLK_j]             = 'j';
//	key_map[SDLK_k]             = 'k';
//	key_map[SDLK_l]             = 'l';
//	key_map[SDLK_m]             = 'm';
//	key_map[SDLK_n]             = 'n';
//	key_map[SDLK_o]             = 'o';
//	key_map[SDLK_p]             = 'p';
//	key_map[SDLK_q]             = 'q';
//	key_map[SDLK_r]             = 'r';
//	key_map[SDLK_s]             = 's';
//	key_map[SDLK_t]             = 't';
//	key_map[SDLK_u]             = 'u';
//	key_map[SDLK_v]             = 'v';
//	key_map[SDLK_w]             = 'w';
//	key_map[SDLK_x]             = 'x';
//	key_map[SDLK_y]             = 'y';
//	key_map[SDLK_z]             = 'z';
//
//	key_map[SDLK_DELETE]        = K_DELETE;
//#if 0
//	key_map[SDLK_KP_0]           = K_NUMPAD_0; //K_KP_INS;
//	key_map[SDLK_KP_1]           = K_NUMPAD_1; //K_KP_END;
//	key_map[SDLK_KP_2]           = K_NUMPAD_2; //K_KP_DOWNARROW;
//	key_map[SDLK_KP_3]           = K_NUMPAD_3; //K_KP_PGDN;
//	key_map[SDLK_KP_4]           = K_NUMPAD_4; //K_KP_LEFTARROW;
//	key_map[SDLK_KP_5]           = K_NUMPAD_5; //K_KP_5;
//	key_map[SDLK_KP_6]           = K_NUMPAD_6; //K_KP_RIGHTARROW;
//	key_map[SDLK_KP_7]           = K_NUMPAD_7; //K_KP_HOME;
//	key_map[SDLK_KP_8]           = K_NUMPAD_8; //K_KP_UPARROW;
//	key_map[SDLK_KP_9]           = K_NUMPAD_9; // K_KP_PGUP;
//	key_map[SDLK_KP_PERIOD]     = K_NUMPAD_PERIOD; // K_KP_DEL;
//	key_map[SDLK_KP_DIVIDE]     = K_NUMPAD_DIVIDE;
//	key_map[SDLK_KP_MULTIPLY]   = K_NUMPAD_MULTIPLY;
//	key_map[SDLK_KP_MINUS]      = K_NUMPAD_MINUS;
//	key_map[SDLK_KP_PLUS]       = K_NUMPAD_PLUS;
//
//	//key_map[SDLK_KP_ENTER]      = K_KP_ENTER;  // Sorry charlie?  Becomes non-supported.  But don't we still map to enter?
//	key_map[SDLK_KP_EQUALS]     = 0;
//
//	key_map[SDLK_UP]            = K_UPARROW;
//	key_map[SDLK_DOWN]          = K_DOWNARROW;
//	key_map[SDLK_RIGHT]         = K_RIGHTARROW;
//	key_map[SDLK_LEFT]          = K_LEFTARROW;
//	key_map[SDLK_INSERT]        = K_INSERT;
//	key_map[SDLK_HOME]          = K_HOME;
//	key_map[SDLK_END]           = K_END;
//	key_map[SDLK_PAGEUP]        = K_PGUP;
//	key_map[SDLK_PAGEDOWN]      = K_PGDN;
//
//	key_map[SDLK_F1]            = K_F1;
//	key_map[SDLK_F2]            = K_F2;
//	key_map[SDLK_F3]            = K_F3;
//	key_map[SDLK_F4]            = K_F4;
//	key_map[SDLK_F5]            = K_F5;
//	key_map[SDLK_F6]            = K_F6;
//	key_map[SDLK_F7]            = K_F7;
//	key_map[SDLK_F8]            = K_F8;
//	key_map[SDLK_F9]            = K_F9;
//	key_map[SDLK_F10]           = K_F10;
//	key_map[SDLK_F11]           = K_F11;
//	key_map[SDLK_F12]           = K_F12;
//	key_map[SDLK_F13]           = 0;
//	key_map[SDLK_F14]           = 0;
//	key_map[SDLK_F15]           = 0;
//
//	key_map[SDLK_NUMLOCKCLEAR]  = K_KP_NUMLOCK;
//	key_map[SDLK_CAPSLOCK]      = 0;
//	key_map[SDLK_SCROLLLOCK]    = 0;
//	key_map[SDLK_RSHIFT]        = K_SHIFT;
//	key_map[SDLK_LSHIFT]        = K_SHIFT;
//	key_map[SDLK_RCTRL]         = K_CTRL;
//	key_map[SDLK_LCTRL]         = K_CTRL;
//	key_map[SDLK_RALT]          = K_ALT;
//	key_map[SDLK_LALT]          = K_ALT;
////	key_map[SDLK_RMETA]         = 0;
////	key_map[SDLK_LMETA]         = 0;
////	key_map[SDLK_LSUPER]        = 0;
////	key_map[SDLK_RSUPER]        = 0;
//	key_map[SDLK_MODE]          = 0;
////	key_map[SDLK_COMPOSE]       = 0;
//	key_map[SDLK_HELP]          = 0;
////	key_map[SDLK_PRINT]         = 0;
//	key_map[SDLK_SYSREQ]        = 0;
////	key_map[SDLK_BREAK]         = 0;
//	key_map[SDLK_MENU]          = 0;
//	key_map[SDLK_POWER]         = 0;
////	key_map[SDLK_EURO]          = 0;
//	key_map[SDLK_UNDO]          = 0;
//#endif
//}
//
///*
//=======
//MapKey
//
//Map from SDL to quake keynums
//=======
//*/
//inline int IN_SDL2_ScancodeToQuakeKey(SDL_Scancode scancode)
//{
//	switch (scancode)
//	{
//	case SDL_SCANCODE_TAB: return K_TAB;
//	case SDL_SCANCODE_RETURN: return K_ENTER;
//	case SDL_SCANCODE_RETURN2: return K_ENTER;
//	case SDL_SCANCODE_ESCAPE: return K_ESCAPE;
//	case SDL_SCANCODE_SPACE: return K_SPACE;
//
//	case SDL_SCANCODE_A: return 'a';
//	case SDL_SCANCODE_B: return 'b';
//	case SDL_SCANCODE_C: return 'c';
//	case SDL_SCANCODE_D: return 'd';
//	case SDL_SCANCODE_E: return 'e';
//	case SDL_SCANCODE_F: return 'f';
//	case SDL_SCANCODE_G: return 'g';
//	case SDL_SCANCODE_H: return 'h';
//	case SDL_SCANCODE_I: return 'i';
//	case SDL_SCANCODE_J: return 'j';
//	case SDL_SCANCODE_K: return 'k';
//	case SDL_SCANCODE_L: return 'l';
//	case SDL_SCANCODE_M: return 'm';
//	case SDL_SCANCODE_N: return 'n';
//	case SDL_SCANCODE_O: return 'o';
//	case SDL_SCANCODE_P: return 'p';
//	case SDL_SCANCODE_Q: return 'q';
//	case SDL_SCANCODE_R: return 'r';
//	case SDL_SCANCODE_S: return 's';
//	case SDL_SCANCODE_T: return 't';
//	case SDL_SCANCODE_U: return 'u';
//	case SDL_SCANCODE_V: return 'v';
//	case SDL_SCANCODE_W: return 'w';
//	case SDL_SCANCODE_X: return 'x';
//	case SDL_SCANCODE_Y: return 'y';
//	case SDL_SCANCODE_Z: return 'z';
//
//	case SDL_SCANCODE_1: return '1';
//	case SDL_SCANCODE_2: return '2';
//	case SDL_SCANCODE_3: return '3';
//	case SDL_SCANCODE_4: return '4';
//	case SDL_SCANCODE_5: return '5';
//	case SDL_SCANCODE_6: return '6';
//	case SDL_SCANCODE_7: return '7';
//	case SDL_SCANCODE_8: return '8';
//	case SDL_SCANCODE_9: return '9';
//	case SDL_SCANCODE_0: return '0';
//
//	case SDL_SCANCODE_MINUS: return '-';
//	case SDL_SCANCODE_EQUALS: return '=';
//	case SDL_SCANCODE_LEFTBRACKET: return '[';
//	case SDL_SCANCODE_RIGHTBRACKET: return ']';
//	case SDL_SCANCODE_BACKSLASH: return '\\';
//	case SDL_SCANCODE_NONUSHASH: return '#';
//	case SDL_SCANCODE_SEMICOLON: return ';';
//	case SDL_SCANCODE_APOSTROPHE: return '\'';
//	case SDL_SCANCODE_GRAVE: return '`';
//	case SDL_SCANCODE_COMMA: return ',';
//	case SDL_SCANCODE_PERIOD: return '.';
//	case SDL_SCANCODE_SLASH: return '/';
//	case SDL_SCANCODE_NONUSBACKSLASH: return '\\';
//
//	case SDL_SCANCODE_BACKSPACE: return K_BACKSPACE;
//	case SDL_SCANCODE_UP: return K_UPARROW;
//	case SDL_SCANCODE_DOWN: return K_DOWNARROW;
//	case SDL_SCANCODE_LEFT: return K_LEFTARROW;
//	case SDL_SCANCODE_RIGHT: return K_RIGHTARROW;
//
//	case SDL_SCANCODE_LALT: return K_ALT;
//	case SDL_SCANCODE_RALT: return K_ALT;
//	case SDL_SCANCODE_LCTRL: return K_CTRL;
//	case SDL_SCANCODE_RCTRL: return K_CTRL;
//	case SDL_SCANCODE_LSHIFT: return K_SHIFT;
//	case SDL_SCANCODE_RSHIFT: return K_SHIFT;
//
//	case SDL_SCANCODE_F1: return K_F1;
//	case SDL_SCANCODE_F2: return K_F2;
//	case SDL_SCANCODE_F3: return K_F3;
//	case SDL_SCANCODE_F4: return K_F4;
//	case SDL_SCANCODE_F5: return K_F5;
//	case SDL_SCANCODE_F6: return K_F6;
//	case SDL_SCANCODE_F7: return K_F7;
//	case SDL_SCANCODE_F8: return K_F8;
//	case SDL_SCANCODE_F9: return K_F9;
//	case SDL_SCANCODE_F10: return K_F10;
//	case SDL_SCANCODE_F11: return K_F11;
//	case SDL_SCANCODE_F12: return K_F12;
//	case SDL_SCANCODE_INSERT: return K_INSERT;
//	case SDL_SCANCODE_DELETE: return K_DELETE;
//	case SDL_SCANCODE_PAGEDOWN: return K_PAGEDOWN;
//	case SDL_SCANCODE_PAGEUP: return K_PAGEUP;
//	case SDL_SCANCODE_HOME: return K_HOME;
//	case SDL_SCANCODE_END: return K_END;
//
//	case SDL_SCANCODE_NUMLOCKCLEAR: return K_NUMLOCK;
//	case SDL_SCANCODE_KP_DIVIDE: return K_NUMPAD_DIVIDE;
//	case SDL_SCANCODE_KP_MULTIPLY: return K_NUMPAD_MULTIPLY;
//	case SDL_SCANCODE_KP_MINUS: return K_NUMPAD_MINUS;
//	case SDL_SCANCODE_KP_7: return K_HOME;
//	case SDL_SCANCODE_KP_8: return K_NUMPAD_8;
//	case SDL_SCANCODE_KP_9: return K_NUMPAD_9;
//	case SDL_SCANCODE_KP_PLUS: return K_NUMPAD_PLUS;
//	case SDL_SCANCODE_KP_4: return K_NUMPAD_4;
//	case SDL_SCANCODE_KP_5: return K_NUMPAD_5;
//	case SDL_SCANCODE_KP_6: return K_NUMPAD_6;
//	case SDL_SCANCODE_KP_1: return K_NUMPAD_1;
//	case SDL_SCANCODE_KP_2: return K_NUMPAD_2;
//	case SDL_SCANCODE_KP_3: return K_NUMPAD_3;
////	case SDL_SCANCODE_KP_ENTER: return K_KP_ENTER;
//	case SDL_SCANCODE_KP_0: return K_NUMPAD_0;
//	case SDL_SCANCODE_KP_PERIOD: return K_NUMPAD_PERIOD;
//
//	case SDL_SCANCODE_LGUI: return K_WINDOWS;
//	case SDL_SCANCODE_RGUI: return K_WINDOWS; // I think?
//
//	case SDL_SCANCODE_PAUSE: return K_PAUSE;
//
//	default: return 0;
//	}
//}
//
//Doesn't this whole here just go away?
//
//#endif // CORE_SDL
//
