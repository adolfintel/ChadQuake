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
// lists.c -- Lists (maps, demos, games, keys, saves)

#include "quakedef.h"


//	int	unused;	const char	*description, *extension, *associated_commands;
list_info_t list_info[] =
{
	{ list_type_none,		"commands/aliases/vars",NULL,		NULL,		},
	{ list_type_config,		"config",	".cfg",		"exec"					},
	{ list_type_demo,		"demo",		".dem",		"playdemo,capturedemo"	},
	{ list_type_demos_menu,	"demos",	".dem",		"none"					},
	{ list_type_game,		"game",		NULL,		"game,uninstall,hdfolder",		},
	{ list_type_game_hud,	"-game",	NULL,		"game"					},
	{ list_type_give,		"give",		NULL,		"give"					},
	{ list_type_quaddicted,	"install",	NULL,		"install"				},
	{ list_type_key,		"key",		NULL,		"bind,unbind"			},
	{ list_type_map,		"map",		".bsp",		"map,changelevel"		},
	{ list_type_map2,		"map",		".bsp",		"map,changelevel,sv_map_rotation"		},
	{ list_type_levels_menu,"levels",	".bsp",		"none"					},
	{ list_type_savegame,	"save game",".sav",		"save,load"				},
	{ list_type_sky,		"sky",		"rt.tga",	"sky"					},
	{ list_type_mp3,		"setmusic",	".mp3",		"setmusic"				},
	{ list_type_sound,		"sound",	".wav",		"play,playvol"			},
#ifdef GLQUAKE_TEXTUREMODES
	{ list_type_texmode,	"texmode",	NULL,		"gl_texturemode"		},
#endif // GLQUAKE_TEXTUREMODES
};


static void List_QPrint (list_info_t* list_info_item)
{
	int count;
	clist_t	*cursor;

	for (cursor = list_info_item->plist, count = 0; cursor; cursor = cursor->next, count ++)
		Con_SafePrintLinef ("   %s", cursor->name);

	if (count)
		Con_SafePrintLinef ("%d %s(s)", count, list_info_item->description);
	else Con_SafePrintLinef ("no %ss found", list_info_item->description);
}


//==============================================================================
//johnfitz -- modlist management
//==============================================================================

void Lists_Update_ModList (void)
{
	DIR				*dir_p;
	struct dirent	*dir_t;
	char			dir_string[MAX_OSPATH];

	FS_FullPath_From_Basedir (dir_string, "");
//	c_snprintf1 (dir_string, "%s/", com_basedir); // Fill in the com_basedir into dir_string

	dir_p = opendir(dir_string);
	if (dir_p == NULL)
		return;

	if (/*&  Clang caught this*/ list_info[list_type_game].plist) // Nice catch Clang!
	{
		List_Free (&list_info[list_type_game].plist);
		if (list_info[list_type_game].plist) System_Error ("Not freed?");
	}

	while ((dir_t = readdir(dir_p)) != NULL)
	{
		cbool		progs_found = false, pak_found = false, subfolder_found = false;
		char			mod_dir_string[MAX_OSPATH];
		DIR				*mod_dir_p;
		struct dirent	*mod_dir_t;

		// Baker: Ignore "." and ".."
		if (!strcmp(dir_t->d_name, ".") || !strcmp(dir_t->d_name, "..") )
			continue;

		// Baker: Try to open the directory, if not it is a file.
		c_snprintf2 (mod_dir_string, "%s%s/", dir_string, dir_t->d_name);
		mod_dir_p = opendir (mod_dir_string);
		if (mod_dir_p == NULL)
			continue;

		// Baker: We want to find evidence this is playable and not just a folder.
		// Baker: Find either a progs.dat, a pak file or .bsp or textures
		{
			const char *sub_folders[] = {"maps","sound","textures","progs", "music", NULL}; // Baker: should cover it
			int i;
			for (i = 0; sub_folders[i]; i++ )
			{
				const char *cur = sub_folders[i];
				DIR *mod_subdir_p;

				c_snprintf3 (mod_dir_string, "%s%s/%s/", dir_string, dir_t->d_name, cur);
//				alert (mod_dir_string);
#if 1
				// A super long directory name isn't supported by Quake.
				if (strlen(dir_t->d_name) > MAX_QPATH_64)
					continue;
#endif

				mod_subdir_p = opendir (mod_dir_string);
				if (mod_subdir_p)
				{
					// Found content
					subfolder_found = true;
					closedir (mod_subdir_p);
					break;
				}

			}
		}

		if (!subfolder_found)
		{
			// find progs.dat and pak file(s)
			while ((mod_dir_t = readdir(mod_dir_p)) != NULL)
			{
				if (!strcmp(mod_dir_t->d_name, ".") || !strcmp(mod_dir_t->d_name, ".."))
					continue;
				if (strcasecmp(mod_dir_t->d_name, DEFAULT_PROGS_DAT_NAME) == 0)
					progs_found = true;
				if (strstr(mod_dir_t->d_name, ".pak") || strstr(mod_dir_t->d_name, ".PAK"))
					pak_found = true;
				if (progs_found || pak_found)
					break;
			}
		}

		closedir(mod_dir_p);

		// Baker: If we didn't find a progs, pak or subfolder like "maps", skip this.
		if (!subfolder_found && !progs_found && !pak_found)
			continue;
		List_Add(&list_info[list_type_game].plist, dir_t->d_name);
	}

	closedir(dir_p);
}




typedef const char *(*constcharfunc_t) (void);
static void List_Func_Rebuild (clist_t** list, constcharfunc_t runFunction)
{
	const char *namestring;
	if (*list)
	{
		List_Free (list);
		if (*list) System_Error ("Not freed?");
	}

	while ( (namestring = runFunction()    ) )
	{
		List_Add (list, namestring);
//		Con_PrintLinef ("Adding %s", namestring);
	}

}

void Shareware_Notify (void)
{
	if (!static_registered) {
		Con_PrintLine ();
		Con_PrintLinef ("Note: You are using shareware.");
		Con_PrintLinef ("Custom maps and games are not available.");
		Con_PrintLine ();
	}
}

void List_Configs_f (void)		{ List_QPrint (&list_info[list_type_config]);  }
void List_Demos_f (void)		{ List_QPrint (&list_info[list_type_demo]); }
void List_Games_f (void) 		{ Lists_Update_ModList ();  List_QPrint (&list_info[list_type_game]); Shareware_Notify ();  }
void List_Game_Huds_f (void)	{ List_QPrint (&list_info[list_type_game_hud]); }
void List_Keys_f (void)			{ List_QPrint (&list_info[list_type_key]); }
void List_Maps_f (void)	 		{ List_QPrint (&list_info[list_type_map]); Shareware_Notify (); }
void List_Savegames_f (void)	{ List_QPrint (&list_info[list_type_savegame]); }
void List_Skys_f (void)			{ List_QPrint (&list_info[list_type_sky]); Con_PrintLinef ("Located in gfx/env folder"); }
void List_MP3s_f (void)			{ List_QPrint (&list_info[list_type_mp3]); Con_PrintLinef ("Located in music folder"); }

void List_Sounds_f (void)		{ List_QPrint (&list_info[list_type_sound]); Con_PrintLinef ("These are cached sounds only -- loaded in memory"); }




// These rely on the cache
void Lists_Refresh_NewMap (void)
{
	List_Func_Rebuild (&list_info[list_type_sound].plist, S_Sound_ListExport);
}

// For when demo record is finished
void Lists_Update_Demolist (void)
{
	List_Filelist_Rebuild (&list_info[list_type_demo].plist, "", list_info[list_type_demo].extension, 0, SPECIAL_GAMEDIR_ONLY_IF_COUNT);
}

void Lists_Update_Savelist (void)
{
	List_Filelist_Rebuild (&list_info[list_type_savegame].plist, "", list_info[list_type_savegame].extension, 0, SPECIAL_GAMEDIR_ONLY);
}

char game_startmap[MAX_QPATH_64];
cbool game_map_tiers;
enum startmap_e
{
	startmap_named_start = 0,
	startmap_undergate_intro_rockgate,
	startmap_start_in_name,
	startmap_same_as_gamedir,
	startmap_alphabest,
	startmap_nothing,
};

void Lists_Update_Maps (void)
{
	game_map_tiers = List_Filelist_Rebuild (&list_info[list_type_map].plist, "/maps", list_info[list_type_map].extension, 32*1024, SPECIAL_GAMEDIR_ONLY_IF_COUNT);
	if (game_map_tiers)
		List_Filelist_Rebuild (&list_info[list_type_map2].plist, "/maps", list_info[list_type_map2].extension, 32*1024, SPECIAL_GAMEDIR_TIER_2);
#pragma message ("We need some sort of general completion hint for single player mods that should play anything")
	// else clear
	// Determine the start map

	// Find a map named start, then if only 1 that one, then undergate, then one with "start" in the name, last resort: top alphabetical

	{
		enum startmap_e best_type = startmap_nothing;
		clist_t	*best = NULL;
		clist_t	*cursor;

		for (cursor = (&list_info[list_type_map])->plist; cursor; cursor = cursor->next)
		{
			if (best_type > startmap_named_start && !strcmp (cursor->name, "start"))
			{
				best = cursor;
				best_type = startmap_named_start;
				break; // This cannot be beat
			}
			else if (best_type > startmap_undergate_intro_rockgate && (!strcmp (cursor->name, "undergate") || !strcmp (cursor->name, "intro") || !strcmp (cursor->name, "rockgate")) )
			{
				best = cursor;
				best_type = startmap_undergate_intro_rockgate;
			}
			else if (best_type > startmap_start_in_name && strstr (cursor->name, "start") )
			{
				best = cursor;
				best_type = startmap_start_in_name;
			}
			else if (best_type > startmap_same_as_gamedir && strstr (cursor->name, gamedir_shortname() ) )
			{
				best = cursor;
				best_type = startmap_same_as_gamedir;
			}
			else if (best_type > startmap_alphabest) // Baker: This should work because map list is alpha sorted
			{
				best = cursor;
				best_type = startmap_alphabest;
			}
		}

		c_strlcpy (game_startmap, best->name);
		Con_DPrintLinef ("Start map determined to be %s", game_startmap);
	}

}

void Lists_Update_Levels_Menu (void)
{
	List_Filelist_Rebuild (&list_info[list_type_levels_menu].plist, "/maps", list_info[list_type_levels_menu].extension, 32*1024, SPECIAL_GAMEDIR_PREFERENCE);
}

void Lists_Update_Demos_Menu (void)
{
	List_Filelist_Rebuild (&list_info[list_type_demos_menu].plist, "", list_info[list_type_demos_menu].extension, 0, SPECIAL_GAMEDIR_ONLY);
}


void Lists_NewGame (void)
{
	// Only demos and maps should be here (for menu).  That won't be regenerating in real time.
	Lists_Update_ModList (); // Works
	Lists_Refresh_NewMap ();
	Lists_Update_Demolist();
	Lists_Update_Savelist ();

	Lists_Update_Maps ();
	List_Filelist_Rebuild (&list_info[list_type_sky].plist, "/gfx/env", list_info[list_type_sky].extension, 0, 4);
	List_Filelist_Rebuild (&list_info[list_type_mp3].plist, "/music", list_info[list_type_mp3].extension, 0, 0);
	List_Filelist_Rebuild (&list_info[list_type_config].plist, "", list_info[list_type_config].extension, 0, 2);

	M_Menu_Levels_NewGame ();
	M_Menu_Demos_NewGame ();
#ifdef CORE_PTHREADS
	ReadList_NewGame ();	
#endif // CORE_PTHREADS
}

static const char *gamehuds[] =
{
	"-nehahra",
	"-hipnotic",
	"-rogue",
	"-quoth",
};
#define NUM_GAMEHUDS (int)(sizeof(gamehuds)/sizeof(gamehuds[0]))


const char *GameHud_ListExport (void)
{
	static char returnbuf[32];

	static int last = -1; // Top of list.
	// We want first entry >= this
	int		wanted = CLAMP(0, last + 1, NUM_GAMEHUDS );  // Baker: bounds check

	int i;

	for (i = wanted; i < NUM_GAMEHUDS ; i++)
	{

		if (i >= wanted) // Baker: i must be >=want due to way I setup this function
		{
			c_strlcpy (returnbuf, gamehuds[i]);
			String_Edit_To_Lower_Case (returnbuf); // Baker: avoid hassles with uppercase keynames

			last = i;
//			Con_PrintLinef ("Added %s", returnbuf);
			return returnbuf;
		}
	}

	// Not found, reset to top of list and return NULL
	last = -1;
	return NULL;
}

static const char *give_strings[] = {
	"armor",
	"cells",
	"goldkey",		// trans
	"health",
	"nails",
	"rockets",
	"rune1",		// trans
	"rune2",		// trans
	"rune3",		// trans
	"rune4",		// trans
	"shells",
	"silverkey",	// trans
}; static const int give_strings_count = ARRAY_COUNT(give_strings);

const char *Give_ListExport (void)
{
	static char returnbuf[32];
	
	static int last = -1; // Top of list.
	// We want first entry >= this
	int	wanted = CLAMP(0, last + 1, give_strings_count);  // Baker: bounds check

	int i;

	for (i = wanted; i < give_strings_count ; i++)
	{

		if (i >= wanted) // Baker: i must be >=want due to way I setup this function
		{
			c_strlcpy (returnbuf, give_strings[i]);
			String_Edit_To_Lower_Case (returnbuf); // Baker: avoid hassles with uppercase keynames

			last = i;
//			Con_PrintLinef ("Added %s", returnbuf);
			return returnbuf;
		}
	}

	// Not found, reset to top of list and return NULL
	last = -1;
	return NULL;
}

void Lists_Init (void)
{
	const char *Quaddicted_ListExport (void);

	Cmd_AddCommands (Lists_Init);

	Lists_NewGame ();

	Lists_Update_ModList ();
	List_Func_Rebuild (&list_info[list_type_key].plist, Key_ListExport);
	List_Func_Rebuild (&list_info[list_type_game_hud].plist, GameHud_ListExport);
	List_Func_Rebuild (&list_info[list_type_give].plist, Give_ListExport);
	List_Func_Rebuild (&list_info[list_type_quaddicted].plist, Quaddicted_ListExport);
#ifdef GLQUAKE_TEXTUREMODES
	List_Func_Rebuild (&list_info[list_type_texmode].plist, TexMgr_TextureModes_ListExport);
#endif // GLQUAKE_TEXTUREMODES

}

static const char *quaddicted_strings[] = {
	"1000cuts1a",
	"100b2",
	"100b3",
	"100brush",
	"5rivers",
	"768_5th",
	"768_negke",
	"768_pack",
	"a2d2",
	"a3",
	"abandon",
	"abw",
	"ac",
	"actaltrz",
	"ad_e3m2",
	"ad_v1_42final",
	"add",
	"add2",
	"alba01",
	"alba02",
	"alk05",
	"alk07",
	"alk08",
	"alk10",
	"alk11",
	"alk12",
	"alk13",
	"alk15",
	"anonca1",
	"anonca3",
	"ant",
	"aopfm_v2",
	"aopv105",
	"apdm3",
	"apsp1",
	"apsp2",
	"apsp3",
	"araivo",
	"arcane",
	"arcanum",
	"arcdemo",
	"arcextra",
	"area51.2",
	"ariadat",
	"arma2",
	"arwop",
	"asylum",
	"auximine",
	"awsp1",
	"back2forwards",
	"backstein1e",
	"badpak",
	"base-x2",
	"base_debris",
	"bbelief",
	"bbelief2008",
	"bbin1",
	"bnt",
	"bod",
	"camber",
	"canal",
	"cappuccino_v2",
	"carnage",
	"casspq1",
	"castled",
	"cda",
	"cdestroy",
	"ch1sp1",
	"chain2",
	"chaos",
	"chapter_necros2",
	"chapters",
	"chessp1",
	"citdoom",
	"cjhsp1",
	"cloning",
	"cmc",
	"coag3_negke",
	"coagula",
	"coagula2_flesh",
	"coagula3_bone",
	"coagula3_pack",
	"coagulacontest",
	"coagulacontest2",
	"coe",
	"cogs",
	"colony",
	"colony2",
	"coma",
	"commctr",
	"contract",
	"corpus",
	"could",
	"coven",
	"cradle",
	"cryo",
	"cursed",
	"czg01",
	"czg02",
	"czg03",
	"czg04",
	"czg07",
	"czgtoxic",
	"damaul3",
	"damaul6",
	"dark04b",
	"darkness",
	"dazsp1",
	"dazsp2",
	"dazsp3",
	"deja",
	"demonslair",
	"descent",
	"desout",
	"dig",
	"digs01",
	"digs02",
	"digs03",
	"digs04",
	"digs05",
	"digs06",
	"digs07_v2",
	"digs08",
	"dis_sp6",
	"distractions",
	"disturb",
	"dknite11",
	"dkt1000",
	"dm1m2",
	"dm3rmx",
	"dm456sp",
	"dm5rmx",
	"dm7rmx",
	"dmc3",
	"dom3m1",
	"dopa",
	"dotd4",
	"dragon",
	"dranzsp1",
	"drysorrow",
	"dstab1",
	"dwmtf",
	"dwroae_m3a",
	"dwroae_m3b",
	"e1m1red",
	"e1m1rmx",
	"e1m1rmx_hard",
	"e1m5quoth",
	"e1m5quotha",
	"e2m10",
	"e2m10glq",
	"e2m5rmx",
	"ebony1",
	"edom",
	"eels",
	"efdatsp1",
	"electric",
	"elek_neh_episode4",
	"elektra",
	"elsinore",
	"eltemple",
	"endtime",
	"epoch",
	"erotique",
	"etown",
	"event",
	"evildead",
	"evilexhumed",
	"evilwrld",
	"evspq1",
	"explorejam1",
	"fallen1c",
	"fantasy",
	"fatal-error",
	"fatalpla",
	"fc1",
	"februus",
	"fesp1",
	"finally",
	"flecksp1",
	"flesh",
	"fmb100",
	"fmb3",
	"fmb4",
	"fmb5",
	"fmb6",
	"fmb7",
	"fmb8",
	"fmb_bdg",
	"fmb_bdg2",
	"foon",
	"fort_driant-fullvis",
	"fourfeather0",
	"fr3n1m3",
	"fr3nrun2",
	"fromhell",
	"func_mapjam1",
	"func_mapjam2",
	"func_mapjam3",
	"func_mapjam4",
	"func_mapjam5",
	"ghost",
	"gibfact",
	"gmsp1",
	"gmsp3",
	"gmsp3tw",
	"goblin",
	"godsjt",
	"golgotha",
	"gor1",
	"gravelpitfinal2",
	"grc4beta",
	"gth",
	"guard",
	"guncotton",
	"gunman02",
	"gwynt",
	"harmsway",
	"haunting",
	"hayduke1",
	"hdn",
	"hellbridge",
	"hellchepsout",
	"hellctle",
	"hellhole",
	"heresp2",
	"heresp4",
	"hhouse",
	"hipside",
	"hive",
	"honey",
	"hostile",
	"hrdtrgt2",
	"hrim_sp1",
	"hrim_sp2",
	"hrim_sp3",
	"hrim_sp4",
	"hulk256",
	"iam1",
	"ikspq2",
	"ikspq3",
	"ikspq4",
	"ikspq5",
	"imp1sp1",
	"imp1sp2",
	"infernal_v14",
	"ins4",
	"invein",
	"its_demo_v1_1",
	"ivory1b",
	"jackboot",
	"jam6_daya",
	"jawbreak",
	"jjspq1",
	"jjspq2",
	"jjspq3",
	"jtkmap6",
	"kaahoo1a",
	"kellmet1",
	"kewsampl",
	"kinn_bastion",
	"kinn_marcher",
	"kjsp1",
	"koohoo",
	"kpcn2014",
	"kq",
	"ksp1",
	"lfsp1",
	"lfsp2",
	"lfsp3",
	"lfsp4",
	"lfsp5",
	"lfsp7",
	"lfspme",
	"lfspplus",
	"lighthse1",
	"lisland",
	"lostwrld",
	"lpqsp1",
	"lthsp1",
	"lthsp2",
	"lthsp3",
	"lthsp4",
	"lthsp5",
	"lunar",
	"lunar2",
	"lunsp1",
	"m_palace",
	"maelstrm",
	"maelstromv2",
	"mapjam6",
	"mapjam7",
	"mappi",
	"mappi2",
	"marine",
	"mars1",
	"masque_final",
	"matsp1",
	"matsp2",
	"mce",
	"mcomplex",
	"menk",
	"mescalito",
	"metmon1d",
	"mexx10",
	"mexx8",
	"mexx9",
	"mfxsp10",
	"mfxsp17",
	"mfxsp5",
	"middle",
	"mjb_horse",
	"monfree",
	"moonlite",
	"morbid_1",
	"morbid_2",
	"mospq1",
	"mrdrinc",
	"mstalk1c",
	"mstalk1d",
	"n3sp02_13",
	"n3sp02_extd",
	"n3sp03",
	"nanesp1",
	"nar_cat",
	"nar_vault",
	"nastrond",
	"ne_deadcity",
	"ne_lend_doom",
	"ne_marb",
	"ne_ruins",
	"ne_sp04",
	"ne_sp06",
	"ne_tower",
	"necrobrood",
	"nehahra",
	"nesp09",
	"nesp16",
	"networld",
	"nightjourney_v2",
	"nihilore",
	"nike",
	"nsoe",
	"nsoe2",
	"nyarlathotep",
	"obiwan",
	"obiwan2",
	"obiwan3",
	"oblivion",
	"oblivion_2",
	"obnejn",
	"odyssey1_v2",
	"omlabx",
	"oms2",
	"oms2_2",
	"oms_unmentionables",
	"opcondor",
	"orbit",
	"orlmaps",
	"oum",
	"outpost",
	"outpost-alpha",
	"outpost2",
	"ovrridn",
	"pbrsp1",
	"pcgwhse",
	"pdq1sp1_c",
	"percept",
	"perssp1",
	"perssp2",
	"pg2",
	"polybase",
	"polygon2",
	"precipice_continuum",
	"prey2",
	"prison2",
	"prodigy_se",
	"q1shw1sp",
	"q1shw2sp",
	"q1tm2",
	"qblack",
	"qshift",
	"qt_pre02",
	"quarantine",
	"quarantine02",
	"quoth2pt2full",
	"qx11_z1",
	"rapture",
	"rc1",
	"rc2_3",
	"rdestats161",
	"real",
	"red777",
	"requiem",
	"resurrection",
	"retrojam1",
	"retrojam2",
	"retrojam3",
	"retrojam4",
	"rettear",
	"ritualsp",
	"rmx-pack",
	"rpgsmse",
	"rpgsp1",
	"rrp",
	"rstart11",
	"rubicon",
	"rubicon2",
	"rumours",
	"runekeep",
	"runekeepgl",
	"runfear",
	"sadlark5",
	"sadlark6",
	"sadlark7",
	"sadlark8",
	"sadlark9",
	"sbe",
	"scampsp1",
	"scream",
	"se",
	"sewage",
	"sgc3",
	"sgc8",
	"sgodrune",
	"shadow",
	"shadowgl",
	"shesp1",
	"shoggoth",
	"sickbase",
	"siluette",
	"simmer2",
	"sksp2b",
	"slaughtr",
	"slave",
	"sludgefactory",
	"sm100turtle",
	"sm32_pack",
	"sm36",
	"sm57_pulsar_se",
	"sm82",
	"soe_full",
	"sofsp1",
	"sofsp2",
	"solarfall",
	"something_wicked",
	"sop_v2",
	"sp1",
	"spd",
	"spd2",
	"spirit1dm3sp",
	"src",
	"src2",
	"starkmon",
	"starship",
	"starshp2",
	"strong-hold",
	"tchak",
	"tcoa",
	"tdk11",
	"tdl",
	"techphob",
	"tefdbl3",
	"temple2",
	"temple_e1",
	"terra",
	"terracity",
	"terror",
	"tfl",
	"tfl2",
	"tfl3",
	"tfl4",
	"thecrypt",
	"thefly",
	"thehand",
	"thepit_2",
	"thtc",
	"tig00",
	"tigcat02",
	"tmc",
	"tmqe06_pack",
	"tms1",
	"toa",
	"tomb_2",
	"torment",
	"tpof",
	"travail",
	"trincasp2",
	"trincasp3",
	"tspe",
	"twelve",
	"twxfinal",
	"undrwrld",
	"unforgiven",
	"utah",
	"valour",
	"veni05a",
	"vigil",
	"village",
	"vision",
	"warpgate",
	"warpspasm",
	"wbase",
	"well",
	"whisper",
	"whiteroom",
	"wieder",
	"winterpack2005-2006",
	"wishes",
	"xnq1002",
	"xplore",
	"zendar1d",
	"zer",
	"zertm_pack",
	"zoom9",
	"zsp1",
}; static const int quaddicted_strings_count = ARRAY_COUNT(quaddicted_strings);


const char *Quaddicted_ListExport (void)
{
	static char returnbuf[32];
	
	static int last = -1; // Top of list.
	// We want first entry >= this
	int	wanted = CLAMP(0, last + 1, quaddicted_strings_count);  // Baker: bounds check

	int i;

	for (i = wanted; i < quaddicted_strings_count ; i++)
	{

		if (i >= wanted) // Baker: i must be >=want due to way I setup this function
		{
			c_strlcpy (returnbuf, quaddicted_strings[i]);
			String_Edit_To_Lower_Case (returnbuf); // Baker: avoid hassles with uppercase keynames

			last = i;
//			Con_PrintLinef ("Added %s", returnbuf);
			return returnbuf;
		}
	}

	// Not found, reset to top of list and return NULL
	last = -1;
	return NULL;
}