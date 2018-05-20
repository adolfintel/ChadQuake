/*
Copyright (C) 2009-2013 Baker

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
// q_lists.h -- Lists (maps, demos, games, keys, saves)

#ifndef __Q_LISTS_H__
#define __Q_LISTS_H__

typedef enum
{
	list_type_none= 0,
	list_type_config,
	list_type_demo,
	list_type_demos_menu,
	list_type_game,
	list_type_game_hud,
	list_type_give,
	list_type_quaddicted,
	list_type_key,
	list_type_map,
	list_type_map2,
	list_type_levels_menu,
	list_type_savegame,
	list_type_sky,
	list_type_mp3,
	list_type_sound,
#ifdef GLQUAKE_TEXTUREMODES
	list_type_texmode,
#endif	// GLQUAKE_TEXTUREMODES
	MAX_LIST_TYPES,
} list_type_t;

enum list_directives_e
{
	SPECIAL_NONE = 0,
	SPECIAL_NO_ID1_PAKFILE = 1,
	SPECIAL_DONOT_STRIP_EXTENSION = 2,
	SPECIAL_STRIP_6_SKYBOX = 4,
	SPECIAL_GAMEDIR_ONLY_IF_COUNT = 8,	// For maps, demos -- only show demos/map in gamedir unless gamedir has none
	SPECIAL_GAMEDIR_ONLY = 16,			// For save games which are only valid for own gamedir anyway
	SPECIAL_GAMEDIR_PREFERENCE = 32,	// For levels menu, shows everything but treated special.
	SPECIAL_GAMEDIR_TIER_2 = 64,		// For maps, if gamedir has own maps create 2nd chance list of suboptimals
	SPECIAL_GAMEDIR_DIRECTORY_ONLY = 128,		// Don't return items in pak files
};

typedef struct
{
	int				unused;
	const char		*description, *extension, *associated_commands;
	clist_t*	plist;
} list_info_t;

extern list_info_t list_info[];

void Lists_NewGame (void);
void Lists_Refresh_NewMap (void);
void Lists_Update_Savelist (void);
void Lists_Update_Demolist (void);
void Lists_Update_Maps (void);
void Lists_Update_ModList (void);
void Lists_Update_Levels_Menu (void);
void Lists_Update_Demos_Menu (void);
void Lists_Init (void);


cbool List_Filelist_Rebuild (clist_t** list, const char *slash_subfolder, const char *dot_extension, int pakminfilesize, int directives);
// Due to data types this is in common.c right now :(  I could redesign but not time well spent.


extern char game_startmap[MAX_QPATH_64];
extern cbool game_map_tiers;

#endif // ! __Q_LISTS_H__
