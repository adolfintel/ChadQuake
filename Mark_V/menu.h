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

#ifndef __MENU_H__
#define __MENU_H__

enum m_state_e {
	m_none,
	m_main,
	m_singleplayer,
	m_load,
	m_save,
	m_levels,
	m_multiplayer,
	m_demos,
	m_setup,
	m_namemaker,
#if 0 // Farethee well IPX ...
	m_net,
#endif // End Farethee well IPX ...
	m_options,
	m_preferences,
	m_video,
	m_keys,
	m_help,
	m_quit,
	m_lanconfig,
	m_gameoptions,
	m_search,
	m_slist
};

extern enum m_state_e m_state;
extern enum m_state_e m_return_state;

extern cbool m_entersound;

extern cbool m_keys_bind_grab;

//
// menus
//
void M_Init (void);
void M_Keydown (int key);
void M_ToggleMenu_f (lparse_t *unused);
void M_Exit (void); // Exiting the menu, sets m_state = m_none;

void M_Menu_Main_f (lparse_t *unused);
//cmd void M_Menu_Options_f (void);
//cmd void M_Menu_Quit_f (void);
void M_Menu_Help_f (void);

void M_Print (int cx, int cy, const char *str);
void M_PrintWhite (int cx, int cy, const char *str);

void M_Draw (void);
void M_DrawCharacter (int cx, int line, int num);

void M_DrawPic (int x, int y, qpic_t *pic);
void M_DrawPicCentered (int y, qpic_t *pic);
void M_DrawTransPic (int x, int y, qpic_t *pic);
void M_DrawCheckbox (int x, int y, int on);

//cmd void M_Menu_Quit_f (void);
void M_Menu_Levels_NewGame (void);
void M_Menu_Demos_NewGame (void);


void VID_Menu_Init (void); //johnfitz
void VID_Menu_f (void); //johnfitz
void VID_MenuDraw (void);
void VID_MenuKey (int key);


typedef struct
{
	const char	*name;
	const char	*description;
} level_t;

extern level_t levels[];
extern const int num_quake_original_levels;

#endif // ! __MENU_H__

