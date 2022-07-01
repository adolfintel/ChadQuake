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



#include "quakedef.h"


//void (*vid_menucmdfn)(void); //johnfitz
//void (*vid_menudrawfn)(void);
//void (*vid_menukeyfn)(int key);

enum m_state_e m_state;

void M_Menu_Main_f (lparse_t *unused);
	void M_Menu_SinglePlayer_f (void);
		void M_Menu_Load_f (void);
		void M_Menu_Save_f (void);
		void M_Menu_Levels_f (void);
	void M_Menu_MultiPlayer_f (void);
		void M_Menu_Demos_f (void);
		void M_Menu_Setup_f (void);
		void M_Menu_NameMaker_f (void);//JQ1.5dev
#if 0 // Farethee well IPX ...
		void M_Menu_Net_f (void);
#endif // End Farethee well IPX ...
		void M_Menu_LanConfig_f (void);
		void M_Menu_GameOptions_f (void);
		void M_Menu_Search_f (enum slistScope_e scope);
		void M_Menu_ServerList_f (void);
	void M_Menu_Options_f (void);
		void M_Menu_Keys_f (void);
		void M_Menu_Video_f (void);
		void M_Menu_Preferences_f (void);
	void M_Menu_Help_f (void);
	void M_Menu_Quit_f (void);

void M_Main_Draw (void);
	void M_SinglePlayer_Draw (void);
		void M_Load_Draw (void);
		void M_Save_Draw (void);
		void M_Levels_Draw (void);
	void M_MultiPlayer_Draw (void);
		void M_Demos_Draw (void);
		void M_Setup_Draw (void);
		void M_NameMaker_Draw (void);
#if 0 // Farethee well IPX ...
		void M_Net_Draw (void);
#endif // End Farethee well IPX ...
		void M_LanConfig_Draw (void);
		void M_GameOptions_Draw (void);
		void M_Search_Draw (void);
		void M_ServerList_Draw (void);
	void M_Options_Draw (void);
		void M_Keys_Draw (void);
		void M_Video_Draw (void);
		void M_Preferences_Draw (void);
	void M_Help_Draw (void);
	void M_Quit_Draw (void);

void M_Main_Key (int key);
	void M_SinglePlayer_Key (int key);
		void M_Load_Key (int key);
		void M_Save_Key (int key);
		void M_Levels_Key (int key);
	void M_MultiPlayer_Key (int key);
		void M_Demos_Key (int key);
		void M_Setup_Key (int key);
		void M_NameMaker_Key (int key);
#if 0 // Farethee well IPX ...
		void M_Net_Key (int key);
#endif // End Farethee well IPX ...
		void M_LanConfig_Key (int key);
		void M_GameOptions_Key (int key);
		void M_Search_Key (int key);
		void M_ServerList_Key (int key);
	void M_Options_Key (int key);
		void M_Keys_Key (int key);
		void M_Video_Key (int key);
		void M_Preferences_Key (int key);
	void M_Help_Key (int key);
	void M_Quit_Key (int key);

cbool	m_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound
cbool	m_recursiveDraw;

enum m_state_e	m_return_state;
cbool	m_return_onerror;
char		m_return_reason [32];

#define StartingGame	(m_multiplayer_cursor == 1)
#define JoiningGame		(m_multiplayer_cursor == 0)
#if 0 // Farethee well IPX ...
#define	IPXConfig		(m_net_cursor == 0)
#define	TCPIPConfig		(m_net_cursor == 1)
#endif // End Farethee well IPX ...
void M_ConfigureNetSubsystem(void);

/*
================
M_DrawCharacter

Draws one solid graphics character
================
*/
void M_DrawCharacter (int cx, int line, int num)
{
	Draw_Character (cx, line, num);
}

void M_Print (int cx, int cy, const char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, (*str)+128);
		str++;
		cx += 8;
	}
}

void M_PrintWhite (int cx, int cy, const char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, *str);
		str++;
		cx += 8;
	}
}

void M_DrawTransPic (int x, int y, qpic_t *pic)
{
	Draw_TransPic (x, y, pic);
}

void M_DrawPic (int x, int y, qpic_t *pic)
{
	Draw_Pic (x, y, pic);
}

void M_DrawPicCentered (int y, qpic_t *pic)
{
	Draw_Pic ((320 - pic->width) / 2, y, pic);
}

#ifdef GLQUAKE_COLORMAP_TEXTURES
void M_DrawTransPicTranslate (int x, int y, qpic_t *pic, int top, int bottom) //johnfitz -- more parameters
{
	Draw_TransPicTranslate (x, y, pic, top, bottom); //johnfitz -- simplified becuase centering is handled elsewhere
}
#else // The WinQuake way ...
byte identityTable[PALETTE_COLORS_256];
byte translationTable[PALETTE_COLORS_256];

void M_BuildTranslationTable(int top, int bottom)
{
	int		j;
	byte	*dest, *source;

	for (j = 0; j < 256; j++)
		identityTable[j] = j;
	dest = translationTable;
	source = identityTable;
	memcpy (dest, source, 256);

	if (top < 128)	// the artists made some backwards ranges.  sigh.
		memcpy (dest + TOP_RANGE, source + top, 16);
	else
		for (j=0 ; j<16 ; j++)
			dest[TOP_RANGE+j] = source[top+15-j];

	if (bottom < 128)
		memcpy (dest + BOTTOM_RANGE, source + bottom, 16);
	else
		for (j=0 ; j<16 ; j++)
			dest[BOTTOM_RANGE+j] = source[bottom+15-j];
}


void M_DrawTransPicTranslate (int x, int y, qpic_t *pic)
{
	Draw_TransPicTranslate (x, y, pic, translationTable);
}
#endif // GLQUAKE_COLORMAP_TEXTURES

void M_DrawTextBox (int x, int y, int width, int lines)
{
	qpic_t	*p;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_bl.lmp");
	M_DrawTransPic (cx, cy+8, p);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = Draw_CachePic ("gfx/box_tm.lmp");
		M_DrawTransPic (cx, cy, p);
		p = Draw_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");
			M_DrawTransPic (cx, cy, p);
		}
		p = Draw_CachePic ("gfx/box_bm.lmp");
		M_DrawTransPic (cx, cy+8, p);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_br.lmp");
	M_DrawTransPic (cx, cy+8, p);
}


//=============================================================================
/* DEMOS MENU */

int	m_demos_cursor;

typedef struct
{
	char demoname[MAX_QPATH_64];
	char age[5];
	char kb[5];
	char mapstart[MAX_QPATH_64];
} demo_item_t;

int m_demos_menu_line_count;
const char *m_demos_cursor_name[MAX_QPATH_64];
demo_item_t* m_demos_list;
#ifdef _WIN32
const char *demos_tips[]={ "pgdn = demo rewind", "press \\ to explore"};
#else // Mac: Doesn't normally have those keys
const char *demos_tips[]={ "left arrow = demo rewind", "press \\ to explore"};
#endif
const char *demos_tip;
int demos_cursor;
int demos_view_first_row;

void M_Menu_Demos_NewGame (void)
{
	demos_cursor = demos_view_first_row = 0;
}

int m_demos_read;
int m_demos_count;
#define ITEM_COUNT_17 17
// Baker: Staggered read.  Read 17 in at a time every frame
// for user convenience, instead of stalling for several seconds
// if there are a lot of maps.

void M_Demos_Read (void)
{
	int count;
	clist_t	*cursor;
	demo_item_t *item;

	if (!(m_demos_list && m_demos_read < m_demos_count))
		return;

			// Now walk the tree again for last time
	for (
		cursor = (&list_info[list_type_demos_menu])->plist, count = 0;
		cursor && count < m_demos_read + ITEM_COUNT_17 /* do 17 at a time */;
		cursor = cursor->next, count ++
	)
	{
		item = &m_demos_list[count];

		if (count < m_demos_read)
			continue;  // Already did this

		c_strlcpy (item->demoname, cursor->name);

		// Now find the map name
		{
			int h;
//					FILE	*f = NULL;
			char qpath[MAX_QPATH_64];
			cbool is_dzip = false;

			if (String_Does_End_With_Caseless (item->demoname, ".dz"))
			{
				c_strlcpy (qpath, item->demoname);
				is_dzip = true;
			}
			else c_snprintf1 (qpath, "%s.dem", item->demoname);

			//COM_FOpenFile (qpath, &f);
			if (COM_OpenFile (qpath, &h) == -1)
				System_Error ("Can't open %s", item->demoname);

//			item->age; // Days
			if (!com_filesrcpak)
			{
				char namebuf[MAX_OSPATH];
				double filetime, nowtime, age;
				FS_FullPath_From_QPath (namebuf, qpath);
				filetime = File_Time (namebuf);
				nowtime = Time_Now ();
				age = (nowtime-filetime)/(24*60*60);

				c_snprintf1 (item->age, "%d", (int)age);

			} else  c_strlcpy (item->age, "pak");

			c_snprintf1 (item->kb, "%d", com_filesize / 1024);

			if (!is_dzip)
			{
				byte readbuf[300];
				byte *look;
				byte *lastslash;
				byte *bspstart;
				// Read the header
				System_FileRead (h, readbuf, sizeof(readbuf)-1);

				for (look = readbuf, bspstart = NULL, lastslash = NULL; look < readbuf + sizeof(readbuf) -4; look++)
				{
					if (look[0] == '.' && look[1] == 'b' && look[2] == 's' && look[3] == 'p' && look[4] == 0)
					{
						bspstart = look;
						break;
					} else if (look[0] == '/') lastslash = look;
				}

				if (bspstart)
				{
					File_URL_Copy_StripExtension (item->mapstart, (char*)&lastslash[1], sizeof(item->mapstart));
				}
				else c_strlcpy (item->mapstart, "??");

			}
			else c_strlcpy (item->mapstart, "[dzip]");


			// Find .bsp + NULL
			// Scan backwards to /
			// Get map name

			COM_CloseFile (h);

		} // End of "now find the level
	} // End of for loop #2

	m_demos_read = count;
}

void M_Menu_Demos_f (void)
{
	Key_SetDest (key_menu);
	m_state = m_demos;
	m_entersound = true;

	demos_tip = (rand() & 1) ? demos_tips[0] : demos_tips[1];

	// Assess Demos
	{
		Lists_Update_Demos_Menu (); // Baker: We must get only ones for current gamedir!

		do
		{
			// Walk the tree, determine amount of depth
			int count;
			clist_t	*cursor;
			demo_item_t *item;

			// Free if needed
			if (m_demos_list)
			{
				free (m_demos_list);
				m_demos_list = NULL;
			}

			// Baker: Deal with a count of 0 situation
			if (  (&list_info[list_type_demos_menu])->plist == NULL)
			{
				// Add an item to say there is nothing
				m_demos_menu_line_count = count = 1;
				m_demos_list = (demo_item_t *) calloc (sizeof(demo_item_t), m_demos_menu_line_count);

				item = &m_demos_list[0];
				c_strlcpy (item->demoname, "No demos!");
				break; // Bail!
			}

			for (cursor = (&list_info[list_type_demos_menu])->plist, count = 0; cursor; cursor = cursor->next, count ++)
				;

			// Allocate
			m_demos_read = 0;
			m_demos_count = count;
			m_demos_menu_line_count = count;
			m_demos_list = (demo_item_t *) calloc (sizeof(demo_item_t), m_demos_menu_line_count);


		} while (0);

	} // Of assess demos

//	Con_PrintLinef ("Out of there");
}

void M_Demos_Draw (void)
{
//	const char *title1 =  "<==NAME======><=age=><=KB=><=MAP====>"; // 38 chars
	const char *title1 =  "  name         age   kb     map"; // 38 chars
	const char *_title2 = "<============><====><=====><=========>"; // 38 chars
	static char title2[39]={0};

	int row;
	int yofs;

	M_Demos_Read (); // Staggered read

	if (title2[0]==0)
	{
		const char *src;
		char *dst;
		for (src = _title2, dst = title2; *src; src++, dst++)
		{
			if (src[0] == '<')
				dst[0] = '\35';
			else if (src[0] == '=')
				dst[0] = '\36';
			else if (src[0] == '>')
				dst[0] = '\37';
			else dst[0] = src[0];
		}
		dst[0] = 0;
	}

	if (demos_cursor < demos_view_first_row)
		demos_view_first_row = demos_cursor;
	else if (demos_cursor > (demos_view_first_row + (ITEM_COUNT_17 - 1)))
		demos_view_first_row = demos_cursor - (ITEM_COUNT_17 - 1);

	// Setup
	Draw_SetCanvas (CANVAS_MENU);

	Draw_Fill (0,0,320,200,0,0.50);
	Draw_Fill (8,8,304,184,17,0.50);
#ifdef GLQUAKE_ALPHA_DRAWING
	// Baker: we draw it slightly differently in WinQuake
	// Due to lack of alpha drawing capability
	Draw_Fill (16,16,288,184,17,0.50);
#endif

	M_DrawTransPic (8, 8, Draw_CachePic ("gfx/demos.lmp") );

	M_Print (320-160, 12, demos_tip);
	M_Print (8,28, title1);
	M_Print (8,36, title2);

	for (row = demos_view_first_row, yofs = 52; row < demos_view_first_row + ITEM_COUNT_17; row ++, yofs += 8)
	{
		if (row == demos_cursor)
			Draw_Character ( 8, yofs, 12+((int)(realtime*4)&1) );

		if 	(0 <= row && row < m_demos_menu_line_count)
		{
			M_Print ( 24, yofs,
				va("%-11s  %4s  %5s  %-9s",
					va("%.11s", m_demos_list[row].demoname),
					va("%.4s", m_demos_list[row].age),
					va("%.5s", m_demos_list[row].kb),
					va("%.9s", m_demos_list[row].mapstart)
				)  );

		}
	}

	{
		float pct = demos_cursor / (float)(m_demos_menu_line_count-1); // What percentile of range is visible
		const int drawsize = (ITEM_COUNT_17 - 1);
		int drawtoprange = 174-52;
		int top = 52 + c_rint (pct * drawtoprange);
		Draw_Fill (308, top, 4, drawsize, 6, 1);
	}
}


void M_Demos_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f (NULL);
		break;

	case K_DOWNARROW:
		demos_cursor ++;
		demos_cursor = CLAMP (0, demos_cursor, m_demos_menu_line_count - 1);
		break;

	case K_UPARROW:
		demos_cursor --;
		demos_cursor = CLAMP (0, demos_cursor, m_demos_menu_line_count - 1);
		break;

	case '\\':
		if (demos_cursor < m_demos_menu_line_count && m_demos_list[demos_cursor].demoname[0] && m_demos_list[demos_cursor].kb[0])
		{
			if (!strcmp(m_demos_list[demos_cursor].age, "pak"))
			{
			    SCR_ModalMessage ("Can't explore to demo." NEWLINE NEWLINE "It is in a pak file!", 0, true);
				break;
			}

			if (vid.screen.type == MODE_WINDOWED)
			{
				char filebuf [MAX_OSPATH];

				FS_FullPath_From_QPath (filebuf, m_demos_list[demos_cursor].demoname);
				if (!String_Does_End_With (filebuf, ".dz"))
					c_strlcat (filebuf, ".dem");

				Folder_Open_Highlight (filebuf);
			} else {
			    SCR_ModalMessage ("Can't explore to file" NEWLINE "in full-screen mode." NEWLINE "ALT-ENTER toggles full-screen" NEWLINE "and windowed mode.", 0, true);
            }
		}
		break;

	case K_HOME:
		demos_cursor = 0;
		break;

	case K_END:
		demos_cursor = m_demos_menu_line_count - 1;
		break;

	case K_PAGEUP:
		demos_view_first_row -= ITEM_COUNT_17;
		demos_cursor -= ITEM_COUNT_17;
		demos_view_first_row = CLAMP (0, demos_view_first_row, m_demos_menu_line_count < ITEM_COUNT_17 ? 0 : m_demos_menu_line_count - ITEM_COUNT_17);
		demos_cursor = CLAMP (0, demos_cursor, m_demos_menu_line_count - 1);
		break;

	case K_PAGEDOWN:
		demos_view_first_row += ITEM_COUNT_17;
		demos_cursor += ITEM_COUNT_17;
		demos_view_first_row = CLAMP (0, demos_view_first_row, m_demos_menu_line_count < ITEM_COUNT_17 ? 0 : m_demos_menu_line_count - ITEM_COUNT_17);
		demos_cursor = CLAMP (0, demos_cursor, m_demos_menu_line_count - 1);
		break;

	default:
		if (isdigit(key) || isalpha(key))
		{
			int cursor_offset = demos_cursor - demos_view_first_row;
			int i;
			int found = -1;

			key = tolower(key);
			for (i = demos_cursor + 1; i < m_demos_menu_line_count; i ++)
			{
				demo_item_t *item = &m_demos_list[i];

				if (item->demoname[0] && item->demoname[0] == key)
				{
					found = i;
					break;
				}
			}

			if (found == -1) // Wrap
			{
				for (i = 0; i < demos_cursor + 1; i ++)
				{
					demo_item_t *item = &m_demos_list[i];

					if (item->demoname[0] && item->demoname[0] == key)
					{
						found = i;
						break;
					}
				}
			}

			if (found != -1)
			{
				// Found one
				demos_cursor = found;
				demos_view_first_row = demos_cursor - cursor_offset;

				demos_view_first_row = CLAMP (0, demos_view_first_row, m_demos_menu_line_count - ITEM_COUNT_17);
				demos_cursor = CLAMP (0, demos_cursor, m_demos_menu_line_count - 1);
			}

		}
		break;

	case K_ENTER:
		if (demos_cursor < m_demos_menu_line_count && m_demos_list[demos_cursor].demoname[0] && m_demos_list[demos_cursor].kb[0])
		{
			Cbuf_AddTextLinef ("playdemo %s", m_demos_list[demos_cursor].demoname);
		}
		Key_SetDest (key_game);
		break;
	}
}


//=============================================================================

int m_save_demonum;


void M_Exit (void)
{
	m_state = m_none;
}

/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu_f (lparse_t *unused)
{
	m_entersound = true;

	if (key_dest == key_menu)
	{
		if (m_state != m_main)
		{
			M_Menu_Main_f (NULL);
			return;
		}

		Key_SetDest (key_game);
		return;
	}
	if (key_dest == key_console)
	{
		Con_ToggleConsole_f (NULL);
	}
	else
	{
		M_Menu_Main_f (NULL);
	}
}


//=============================================================================
/* MAIN MENU */

int	m_main_cursor;
#define	MAIN_ITEMS	5


void M_Menu_Main_f (lparse_t *unused)
{
	Key_SetDest (key_menu);
	m_state = m_main;
	m_entersound = true;
}


void M_Main_Draw (void)
{
	int		f;
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/ttl_main.lmp");
	M_DrawPicCentered (4, p);

#ifdef SUPPORTS_NEHAHRA
	if (nehahra_active)
		M_DrawTransPic (72, 32, Draw_CachePic ("gfx/gamemenu.lmp") );
	else
#endif // !SUPPORTS_NEHAHRA
	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/mainmenu.lmp") );

	f = (int)(realtime * 10)%6;

	M_DrawTransPic (54, 32 + m_main_cursor * 20,Draw_CachePic( va("gfx/menudot%d.lmp", f+1 ) ) );
}


void M_Main_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		Key_SetDest (key_game);
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_main_cursor)
		{
		case 0:
			M_Menu_SinglePlayer_f ();
			break;

		case 1:
			M_Menu_MultiPlayer_f ();
			break;

		case 2:
			M_Menu_Options_f ();
			break;

		case 3:
			M_Menu_Help_f ();
			break;

		case 4:
			M_Menu_Quit_f ();
			break;
		}
	}
}

//=============================================================================
/* SINGLE PLAYER MENU */

int	m_singleplayer_cursor;

int normal_menu=1;
int normal_backtile = 1;
#ifdef SUPPORTS_LEVELS_MENU_HACK
int	SINGLEPLAYER_ITEMS = (normal_menu && !COM_CheckParm ("-no_levels") ? 4: 3);
#else
int	SINGLEPLAYER_ITEMS = 3;
#endif


void M_Menu_SinglePlayer_f (void)
{
	Key_SetDest (key_menu);
	m_state = m_singleplayer;
	m_entersound = true;
}

void M_SinglePlayer_Draw (void)
{
	int		f;
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/ttl_sgl.lmp");
	M_DrawPicCentered (4, p);

	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/sp_menu.lmp") );


#ifdef SUPPORTS_LEVELS_MENU_HACK
	if (normal_menu && !COM_CheckParm ("-no_levels"))
		M_DrawTransPic (72, 32 + 20 * 3, Draw_CachePic ("gfx/levels.lmp") );
#endif // SUPPORTS_LEVELS_MENU_HACK

	f = (int)(realtime * 10)%6;

	if (m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
		m_singleplayer_cursor = 0;

	M_DrawTransPic (54, 32 + m_singleplayer_cursor * 20,Draw_CachePic( va("gfx/menudot%d.lmp", f+1 ) ) );

}


void M_SinglePlayer_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f (NULL);
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
			m_singleplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_singleplayer_cursor < 0)
			m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_singleplayer_cursor)
		{
		case 0:
			if (sv.active)
				if (!SCR_ModalMessage("Are you sure you want to" NEWLINE "start a new game?", 0, false))
					break;

			if (sv.active)
				CL_Disconnect ();

			SCR_BeginLoadingPlaque_Force_NoTransition ();
			Key_SetDest (key_game);
			console1.visible_pct = 0;

			Cbuf_AddTextLine ("maxplayers 1");
			Cbuf_AddTextLine ("deathmatch 0"); //johnfitz
			Cbuf_AddTextLine ("coop 0"); //johnfitz
			Cbuf_AddTextLine ("resetcvar sv_progs"); //johnfitz

			Cbuf_AddTextLinef ("map %s", game_startmap);
			break;

		case 1:
			M_Menu_Load_f ();
			break;

		case 2:
			M_Menu_Save_f ();
			break;

		case 3:
			M_Menu_Levels_f ();
			break;
		}
	}
}

//=============================================================================
/* LEVELS MENU */

int	m_levels_cursor;

typedef struct
{
	int depth;	// 0, 1, 2 or 3 --- 3 = caption only
	char mapname[MAX_QPATH_64];
	char levelname[MAX_QPATH_64];
} level_item_t;

int m_levels_menu_line_count;
const char *m_levels_cursor_name[MAX_QPATH_64];
level_item_t* m_levels_list;

int levels_cursor;
int levels_view_first_row;

void M_Menu_Levels_NewGame (void)
{
	levels_cursor = levels_view_first_row = 0;
}

int m_levels_read;
int m_levels_count;

// Baker: Staggered read.  Read 17 in at a time every frame
// for user convenience, instead of stalling for several seconds
// if there are a lot of maps.
void M_Levels_Read (void)
{

	int count;
	char last_depthchar;
	clist_t	*cursor;
	level_item_t *item;

	if (!(m_levels_list && m_levels_read < m_levels_menu_line_count))
		return;

	//alert ("Populating list ... %d of %d", m_levels_read, m_levels_menu_line_count);

	// Now walk the tree again for last time
	for (
		cursor = (&list_info[list_type_levels_menu])->plist, count = 0, last_depthchar = 0;
		cursor && count < m_levels_read + ITEM_COUNT_17 /* do 17 at a time */;
		cursor = cursor->next, count ++
	)
	{
		char cur_depthchar = cursor->name[0];
		item = &m_levels_list[count];

		if (last_depthchar && cur_depthchar != last_depthchar)
		{
			const char *caption;
			last_depthchar = cur_depthchar;

			// Insert 3 rows
			item->depth = 3; c_strlcpy (item->levelname, ""); count ++; item = &m_levels_list[count];
			if (cur_depthchar == '1')
				caption = "custom quake levels";
			else caption = "original quake levels";
			item->depth = 3; c_strlcpy (item->levelname, caption); count ++; item = &m_levels_list[count];
			item->depth = 3; c_strlcpy (item->levelname, ""); count ++; item = &m_levels_list[count];
		}
		else
			if (!last_depthchar)
				last_depthchar = cur_depthchar;

		switch (cur_depthchar)
		{
			case '0': item->depth = 0; break;
			case '1': item->depth = 1; break;
			case 'q': item->depth = 2; break;
			default: System_Error ("Invalid levels menu depth char");
		}

		if (count < m_levels_read)
			continue;  // Already did this

		c_strlcpy (item->mapname, &cursor->name[2]);

		// Now find the level name
		{
			FILE	*f = NULL;
			dheader_t  header;
			char qpath[MAX_QPATH_64];
			size_t filestart;

			c_snprintf1 (qpath, "maps/%s.bsp", item->mapname);
			COM_FOpenFile (qpath, &f);

			if (!f)
				System_Error ("Can't open %s", item->mapname);

			// Get the seek
			filestart = ftell (f);

			// Read the header
			fread (&header, sizeof(dheader_t), 1, f);

			// Find the offsets
			switch (header.version)
			{
			default:
				c_strlcpy (item->levelname, "(Unsupported map)");
				break; // Something unusual like Quake 3 ?

			case BSPVERSION:
			case BSPVERSION_HALFLIFE:
			case BSP2VERSION_2PSB:
			case BSP2VERSION_BSP2:
				{
					const char *map_title;
					char entities_buffer[1000]={0};

					size_t entityoffset = filestart + header.lumps[LUMP_ENTITIES].fileofs;
					size_t amount_to_read = c_min ((size_t) header.lumps[LUMP_ENTITIES].filelen, sizeof(entities_buffer));

					// Seek forward to entity data
					fseek (f, entityoffset, SEEK_SET);
					fread (entities_buffer, amount_to_read, 1, f);
					entities_buffer[sizeof(entities_buffer)-1]=0; // Null terminate just in case.

					// Seek what map title is
					map_title = COM_CL_Worldspawn_Value_For_Key (entities_buffer, "message" /* this is map title */);

					// If map has title
					if (map_title)
					{
						unsigned char *c;
						c_strlcpy (item->levelname, map_title);
						// De-bronze
						for (c = (unsigned char*)item->levelname; c[0]; c++)
						{
							if (c[0] > 128)
								c[0] -= 128;
						}
					}
				}
			}

			FS_fclose (f);
			// Read 300 chars or less at LUMP_
		} // End of "now find the level
	} // End of for loop #2



	m_levels_read = count;
	//alert ("Exit read %d of %d", m_levels_read, m_levels_menu_line_count);
}

void M_Menu_Levels_f (void)
{
	Key_SetDest (key_menu);
	m_state = m_levels;
	m_entersound = true;

	// Assess Levels
	{
		Lists_Update_Levels_Menu (); // Baker: We must get only ones for current gamedir!


		{
			// Walk the tree, determine amount of depth
			int count;
			char last_depthchar;
			int num_depths;
			clist_t	*cursor;

			for (
				cursor = (&list_info[list_type_levels_menu])->plist, count = 0, last_depthchar = 0, num_depths = 0;
				cursor;
				cursor = cursor->next, count ++
			)
			{
				char cur_depthchar = cursor->name[0];
				if (cur_depthchar != last_depthchar)
				{
					num_depths ++;
					last_depthchar = cur_depthchar;
				}
			}

			// Free if needed
			if (m_levels_list)
			{
				free (m_levels_list);
				m_levels_list = NULL;
			}

			// Allocate
			m_levels_count = count;
			m_levels_read = 0;
			m_levels_menu_line_count = count + ((num_depths - 1) * 3); // 3 spaces between depths
			m_levels_list = (level_item_t*) calloc (sizeof(level_item_t), m_levels_menu_line_count);
		}

	} // Of assess levels

//	Con_PrintLinef ("Out of there");
}

void M_Levels_Draw (void)
{
	int row;
	int yofs;

	M_Levels_Read (); // Staggered read


	if (levels_cursor < levels_view_first_row)
		levels_view_first_row = levels_cursor;
	else if (levels_cursor > (levels_view_first_row + (ITEM_COUNT_17 - 1)))
		levels_view_first_row = levels_cursor - (ITEM_COUNT_17 - 1);

	// Setup
	Draw_SetCanvas (CANVAS_MENU);

	Draw_Fill (0,0,320,200,0,0.50);
	Draw_Fill (8, 8, 304,184, 16, 0.50);
#ifdef GLQUAKE_ALPHA_DRAWING
	Draw_Fill (16,16,288,184,17,0.50);
#endif // GLQUAKE_ALPHA_DRAWING

	M_DrawTransPic (8, 8, Draw_CachePic ("gfx/levels.lmp") );

	M_Print (8, 36, Con_Quakebar (13));
	M_Print (112, 36, Con_Quakebar (24));

	for (row = levels_view_first_row, yofs = 52; row < levels_view_first_row + ITEM_COUNT_17; row ++, yofs += 8)
	{
		if (row == levels_cursor)
			Draw_Character ( 8, yofs, 12+((int)(realtime*4)&1) );

		if 	(0 <= row && row < m_levels_menu_line_count)
		{
			if (m_levels_list[row].mapname[0])
				M_Print ( 24, yofs, va("%-10s  %.22s", va("%.10s", m_levels_list[row].mapname), m_levels_list[row].levelname));
			else if (m_levels_list[row].levelname[0])
				M_PrintWhite ( 24, yofs, m_levels_list[row].levelname);
		}
	}

	{
		float pct = levels_cursor / (float)(m_levels_menu_line_count-1); // What percentile of range is visible
		const int drawsize = (ITEM_COUNT_17 - 1);
		int drawtoprange = 174-52;
		int top = 52 + c_rint (pct * drawtoprange);
		Draw_Fill (308, top, 4, drawsize, 6, 1);
	}
}


void M_Levels_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f (NULL);
		break;

	case K_DOWNARROW:
		levels_cursor ++;
		levels_cursor = CLAMP (0, levels_cursor, m_levels_menu_line_count - 1);
		break;

	case K_UPARROW:
		levels_cursor --;
		levels_cursor = CLAMP (0, levels_cursor, m_levels_menu_line_count - 1);
		break;

	case K_HOME:
		levels_cursor = 0;
		break;

	case K_END:
		levels_cursor = m_levels_menu_line_count - 1;
		break;

	case K_PAGEUP:
		levels_view_first_row -= ITEM_COUNT_17;
		levels_cursor -= ITEM_COUNT_17;
		levels_view_first_row = CLAMP (0, levels_view_first_row, m_levels_menu_line_count < ITEM_COUNT_17 ? 0 : m_levels_menu_line_count - ITEM_COUNT_17);
		levels_cursor = CLAMP (0, levels_cursor, m_levels_menu_line_count - 1);
		break;

	case K_PAGEDOWN:
		levels_view_first_row += ITEM_COUNT_17;
		levels_cursor += ITEM_COUNT_17;
		levels_view_first_row = CLAMP (0, levels_view_first_row, m_levels_menu_line_count < ITEM_COUNT_17 ? 0 : m_levels_menu_line_count - ITEM_COUNT_17);
		levels_cursor = CLAMP (0, levels_cursor, m_levels_menu_line_count - 1);
		break;

	default:
		if (isdigit(key) || isalpha(key))
		{
			int cursor_offset = levels_cursor - levels_view_first_row;
			int i;
			int found = -1;

			key = tolower(key);
			for (i = levels_cursor + 1; i < m_levels_menu_line_count; i ++)
			{
				level_item_t *item = &m_levels_list[i];

				if (item->mapname[0] && item->mapname[0] == key)
				{
					found = i;
					break;
				}
			}

			if (found == -1) // Wrap
			{
				for (i = 0; i < levels_cursor + 1; i ++)
				{
					level_item_t *item = &m_levels_list[i];

					if (item->mapname[0] && item->mapname[0] == key)
					{
						found = i;
						break;
					}
				}
			}

			if (found != -1)
			{
				// Found one
				levels_cursor = found;
				levels_view_first_row = levels_cursor - cursor_offset;

				levels_view_first_row = CLAMP (0, levels_view_first_row, m_levels_menu_line_count - ITEM_COUNT_17);
				levels_cursor = CLAMP (0, levels_cursor, m_levels_menu_line_count - 1);
			}

		}
		break;

	case K_ENTER:
		if (levels_cursor < m_levels_menu_line_count && m_levels_list[levels_cursor].mapname[0])
		{
			if (sv.active && cl.maxclients > 1)
				Cbuf_AddTextLinef ("changelevel %s", m_levels_list[levels_cursor].mapname);
			else
				Cbuf_AddTextLinef ("map %s", m_levels_list[levels_cursor].mapname);
		}
		Key_SetDest (key_game);
		break;
	}
}

//=============================================================================
/* LOAD/SAVE MENU */

int		load_cursor;		// 0 < load_cursor < MAX_SAVEGAMES

#define	MAX_SAVEGAMES		20
char	m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH_39 + 1];
int		loadable[MAX_SAVEGAMES];
int		numautosaves;

void M_ScanSaves (cbool exclude_autosaves)
{
	int		i, j, n;
	cbool autosaved;
	char	name[MAX_OSPATH];
	FILE	*f;
	int		version;

//	if (!sv_autosave.value)  // ALWAYS exclude autosaves!
		exclude_autosaves = true;

	numautosaves = 0;
	if (!exclude_autosaves)
	{
		for (i = 0; i < AUTO_SAVE_COUNT; i ++)
		{
			FS_FullPath_From_QPath (name, va("a%d.sav", i));
			if (File_Exists (name))
				numautosaves ++;
		}
	}

	for (i = 0, n = 0 ; i < MAX_SAVEGAMES ; i++)
	{
		c_strlcpy (m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;
		autosaved = false;

		if (i >= (MAX_SAVEGAMES - numautosaves))
		{
			FS_FullPath_From_QPath (name, va("a%d.sav", n));
			autosaved = true;
			n++;
		}
		else FS_FullPath_From_QPath (name, va("s%d.sav", i));

		f = FS_fopen_read (name, "rb");
		if (!f)
			continue;
		fscanf (f, "%d\n", &version);
		fscanf (f, "%79s\n", name);

		if (autosaved)
		{
//			int q = SAVEGAME_COMMENT_LENGTH_39;
			c_strlcpy (m_filenames[i], "Auto:");
			c_strlcat (m_filenames[i], name);
			m_filenames[i][21] = ' ';
			strlcpy (&m_filenames[i][22], &name[22], SAVEGAME_COMMENT_LENGTH_39 - 22);
		}
		else c_strlcpy (m_filenames[i], name);

	// change _ back to space
		for (j=0 ; j<SAVEGAME_COMMENT_LENGTH_39 ; j++)
		{
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';
		}
		loadable[i] = true;
		FS_fclose (f);
	}

}

void M_Menu_Load_f (void)
{
	m_entersound = true;
	Key_SetDest (key_menu); m_state = m_load;

	M_ScanSaves (false);
}


void M_Menu_Save_f (void)
{
	if (!sv.active)
		return;
	if (cl.intermission)
		return;
	if (svs.maxclients_internal != 1)
		return;
	m_entersound = true;
	m_state = m_save;

	Key_SetDest (key_menu);
	M_ScanSaves (true);
}


void M_Load_Draw (void)
{
	int		i;
	qpic_t	*p;

	p = Draw_CachePic ("gfx/p_load.lmp");
	M_DrawPicCentered (4, p);

	for (i=0 ; i< MAX_SAVEGAMES; i++)
	{
		M_Print (16, 32 + 8*i, m_filenames[i]);
	}

// line cursor
	M_DrawCharacter (8, 32 + load_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Save_Draw (void)
{
	int		i;
	qpic_t	*p;

	p = Draw_CachePic ("gfx/p_save.lmp");
	M_DrawPicCentered (4, p);

	for (i=0 ; i<MAX_SAVEGAMES; i++)
	{
		M_Print (16, 32 + 8*i, m_filenames[i]);
	}

// line cursor
	M_DrawCharacter (8, 32 + load_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Load_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
		S_LocalSound ("misc/menu2.wav");
		if (!loadable[load_cursor])
			return;

	// Host_Loadgame_f can't bring up the loading plaque because too much
	// stack space has been used, so do it now
		SCR_BeginLoadingPlaque ();
		Key_SetDest (key_game);
		console1.visible_pct = 0;

	// issue the load command
		if (load_cursor >= MAX_SAVEGAMES - numautosaves)
			Cbuf_AddTextLinef ("load a%d", load_cursor - (MAX_SAVEGAMES - numautosaves) );
		else Cbuf_AddTextLinef ("load s%d", load_cursor);

		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}


void M_Save_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
		Key_SetDest (key_game);
		Cbuf_AddTextLinef ("save s%d", load_cursor);
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}

//=============================================================================
/* MULTIPLAYER MENU */

int	m_multiplayer_cursor;
#define	MULTIPLAYER_ITEMS	(normal_menu ? 4 : 3)


void M_Menu_MultiPlayer_f (void)
{
	Key_SetDest (key_menu);
	m_state = m_multiplayer;
	m_entersound = true;
}


void M_MultiPlayer_Draw (void)
{
	int		f;
	qpic_t	*p;
	{
		// Baker: To peek at single player menu to see if it is "normal"
		qpic_t	*px = Draw_CachePic ("gfx/sp_menu.lmp");
		if (!px) System_Error ("Couldn't load sp_menu.lmp"); // Baker: Trick compiler to make sure above line is run
	}

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );

	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPicCentered (4, p);

	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/mp_menu.lmp") );

	if (normal_menu)
		M_DrawTransPic (72, 34 + 20* 3, Draw_CachePic ("gfx/demos.lmp") );

	f = (int)(realtime * 10)%6;

	if (m_multiplayer_cursor >= MULTIPLAYER_ITEMS)
		m_multiplayer_cursor = 0;

	M_DrawTransPic (54, 32 + m_multiplayer_cursor * 20,Draw_CachePic( va("gfx/menudot%d.lmp", f+1 ) ) );

	if (ipv4Available || ipv6Available)
		return;
	M_PrintWhite ((320/2) - ((27*8)/2), 148, "No Communications Available");
}


void M_MultiPlayer_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f (NULL);
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_multiplayer_cursor >= MULTIPLAYER_ITEMS)
			m_multiplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_multiplayer_cursor < 0)
			m_multiplayer_cursor = MULTIPLAYER_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;
		switch (m_multiplayer_cursor)
		{
		case 0:
			if (ipv4Available || ipv6Available)
#if 0 // Farethee well IPX ...
				M_Menu_Net_f ();
#else
				M_Menu_LanConfig_f ();
#endif
			break;

		case 1:
			if (ipv4Available || ipv6Available)
#if 0 // Farethee well IPX ...
				M_Menu_Net_f ();
#else
				M_Menu_LanConfig_f ();
#endif
			break;

		case 2:
			M_Menu_Setup_f ();
			break;

		case 3:
			M_Menu_Demos_f ();
			break;
		}
	}
}



//=============================================================================
/* SETUP MENU */

int  setup_cursor = 4;
int  setup_cursor_table[] = {40, 56, 80, 104, 140};
char stringbuf[16];
int buflen;

#define SETUP_TOP_COLOR (((int)cl_color.value) >> 4)
#define SETUP_BOTTOM_COLOR (((int)cl_color.value) & 15)
#define	NUM_SETUP_CMDS	5
#define SETBUF(_x) c_strlcpy (stringbuf, _x); buflen = strlen (stringbuf);

void M_Menu_Setup_f (void)
{
	Key_SetDest (key_menu);
	m_state = m_setup;
	m_entersound = true;
}


void M_Setup_Draw (void)
{
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPicCentered (4, p);

	M_Print (64, 40, "Hostname");
	M_DrawTextBox (160, 32, 16, 1);
	M_PrintWhite (168, 40, hostname.string);

	M_Print (64, 56, "Your name");
	M_DrawTextBox (160, 48, 16, 1);
	M_PrintWhite (168, 56, cl_name.string); // Baker 3.83: Draw it correctly

	M_Print (64, 80, "Shirt color");
	M_Print (64, 104, "Pants color");

	M_DrawTextBox (64, 140-8, 14, 1);
	M_Print (72, 140, "Accept Changes");

	p = Draw_CachePic ("gfx/bigbox.lmp");
	M_DrawTransPic (160, 64, p);
	p = Draw_CachePic ("gfx/menuplyr.lmp");
#ifdef GLQUAKE_COLORMAP_TEXTURES
	M_DrawTransPicTranslate (172, 72, p, SETUP_TOP_COLOR, SETUP_BOTTOM_COLOR);
#else // WinQuake ...
	M_BuildTranslationTable(SETUP_TOP_COLOR * 16, SETUP_BOTTOM_COLOR * 16);
	M_DrawTransPicTranslate (172, 72, p);
#endif // GLQUAKE_COLORMAP_TEXTURES

	M_DrawCharacter (56, setup_cursor_table [setup_cursor], 12+((int)(realtime*4)&1));

	if (setup_cursor == 0)
		M_DrawCharacter (168 + 8 * strlen(hostname.string), setup_cursor_table [setup_cursor], 10+((int)(realtime*4)&1));

	if (setup_cursor == 1)
	{
		M_DrawCharacter (168 + 8 * strlen(cl_name.string), setup_cursor_table [setup_cursor], 10+((int)(realtime*4)&1));
		Draw_StringEx (72, 156, "\bPress \bENTER\b for name editor");
	}
}


void M_Setup_Key (int k)
{
	int			l;

	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		setup_cursor--;
		if (setup_cursor < 0)
			setup_cursor = NUM_SETUP_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		setup_cursor++;
		if (setup_cursor >= NUM_SETUP_CMDS)
			setup_cursor = 0;
		break;

	case K_LEFTARROW:
		if (setup_cursor < 2)
			return;
		S_LocalSound ("misc/menu3.wav");
		if (setup_cursor == 2)
		{
			int setup_top = SETUP_TOP_COLOR, setup_bottom = SETUP_BOTTOM_COLOR;
			setup_top --;
			if (setup_top < 0) setup_top = 13;
			Cbuf_AddTextLinef ("color %d %d", setup_top, setup_bottom);
		}
		if (setup_cursor == 3)
		{
			int setup_top = SETUP_TOP_COLOR, setup_bottom = SETUP_BOTTOM_COLOR;
			setup_bottom --;
			if (setup_bottom < 0) setup_bottom = 13;
			Cbuf_AddTextLinef ("color %d %d", setup_top, setup_bottom);
		}
		break;
	case K_RIGHTARROW:
		if (setup_cursor < 2)
			return;
forward:
		S_LocalSound ("misc/menu3.wav");
		if (setup_cursor == 2)
		{
			int setup_top = SETUP_TOP_COLOR, setup_bottom = SETUP_BOTTOM_COLOR;
			setup_top ++;
			if (setup_top > 13) setup_top = 0;
			Cbuf_AddTextLinef ("color %d %d", setup_top, setup_bottom);
		}
		if (setup_cursor == 3)
		{
			int setup_top = SETUP_TOP_COLOR, setup_bottom = SETUP_BOTTOM_COLOR;
			setup_bottom ++;
			if (setup_bottom > 13) setup_bottom = 0;
			Cbuf_AddTextLinef ("color %d %d", setup_top, setup_bottom);
		}
		break;

	case K_ENTER:
		if (setup_cursor == 1)
		{
			m_entersound = true;
			M_Menu_NameMaker_f ();
			break;
		}

		if (setup_cursor == 0 || setup_cursor == 1)
			return;

		if (setup_cursor == 2 || setup_cursor == 3)
			goto forward;

		// setup_cursor == 4 (OK)
		m_entersound = true;
		M_Menu_MultiPlayer_f ();
		break;

	case K_BACKSPACE:
		if (setup_cursor == 0)
		{
			if (strlen (hostname.string))
			{
				SETBUF (hostname.string); // Sets stringbuf, len = buflen
				stringbuf[buflen - 1] = 0;
				Cvar_SetQuick (&hostname, stringbuf);
			}
		}

		if (setup_cursor == 1)
		{
			if (strlen (cl_name.string))
			{
				SETBUF (cl_name.string); // Sets stringbuf, len = buflen
				stringbuf[buflen - 1] = 0;
				Cbuf_AddTextLinef ("name " QUOTED_S, stringbuf);
			}
		}
		break;

	default:
		if (k < 32 || k > 127)
			break;
		if (setup_cursor == 0)
		{
			l = strlen (hostname.string);

			if ( (k == 'v' || k == 'V') && Key_Ctrl_Down())
			{
				if (strlen (Clipboard_Get_Text_Line ()))
					Cvar_SetQuick (&hostname, Clipboard_Get_Text_Line() );
			}
			else
			if (l < 15)
			{
				SETBUF (hostname.string); // Sets stringbuf, len = buflen
				stringbuf[buflen + 1] = 0;
				stringbuf[buflen] = k;
				Cvar_SetQuick (&hostname, stringbuf);
			}
		}
		if (setup_cursor == 1)
		{
			l = strlen (cl_name.string);

			if ( (k == 'v' || k == 'V') && Key_Ctrl_Down())
			{
				if (strlen (Clipboard_Get_Text_Line ()))
					Cbuf_AddTextLinef ("name " QUOTED_S, Clipboard_Get_Text_Line() );
			}
			else
			if (l < 15)
			{
				SETBUF (cl_name.string); // Sets stringbuf, len = buflen
				stringbuf[buflen + 1] = 0;
				stringbuf[buflen] = Key_Alt_Down() ? k | 128 : k;
				Cbuf_AddTextLinef ("name " QUOTED_S, stringbuf);
			}
		}
	}

}

//=============================================================================
/* NAME EDITOR MENU */    // From JoeQuake 0.15 Dev
//=============================================================================
int	namemaker_cursor_x, namemaker_cursor_y;
#define	NAMEMAKER_TABLE_SIZE	16

void M_Menu_NameMaker_f (void)
{
	Key_SetDest (key_menu);  m_state = m_namemaker;
	m_entersound = true;
}

void M_NameMaker_Draw (void)
{
	int	x, y;

	M_Print (48, 16, "Your name");
	M_DrawTextBox (120, 8, 16, 1);
	M_PrintWhite (128, 16, cl_name.string);

	for (y = 1 ; y < NAMEMAKER_TABLE_SIZE ; y++)
		for (x = 0 ; x < NAMEMAKER_TABLE_SIZE ; x++)
			M_DrawCharacter (32 + (16 * x), 32 + (8 * y), NAMEMAKER_TABLE_SIZE * y + x);

//	if (namemaker_cursor_y == NAMEMAKER_TABLE_SIZE - 1)
//		M_DrawCharacter (128, 184, 12 + ((int)(realtime*4)&1));
//	else
		M_DrawCharacter (24 + 16 * namemaker_cursor_x, 40 + 8 * namemaker_cursor_y, 12 + ((int)(realtime*4)&1));

	M_Print (144, 184, "Press ESC to exit");
}


void M_NameMaker_Key (int key)
{
	int	l;

	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Setup_f ();
		break;

	case K_UPARROW:
		namemaker_cursor_y--;
		if (namemaker_cursor_y < 0)
			namemaker_cursor_y = NAMEMAKER_TABLE_SIZE - 2;
		break;

	case K_DOWNARROW:
		namemaker_cursor_y++;
		if (namemaker_cursor_y > NAMEMAKER_TABLE_SIZE - 2)
			namemaker_cursor_y = 0;
		break;

	case K_PAGEUP:
		namemaker_cursor_y = 0;
		break;

	case K_PAGEDOWN:
		namemaker_cursor_y = NAMEMAKER_TABLE_SIZE - 2;
		break;

	case K_LEFTARROW:
		namemaker_cursor_x--;
		if (namemaker_cursor_x < 0)
			namemaker_cursor_x = NAMEMAKER_TABLE_SIZE - 1;
		break;

	case K_RIGHTARROW:
		namemaker_cursor_x++;
		if (namemaker_cursor_x >= NAMEMAKER_TABLE_SIZE - 2)
			namemaker_cursor_x = 0;
		break;

	case K_HOME:
		namemaker_cursor_x = 0;
		break;

	case K_END:
		namemaker_cursor_x = NAMEMAKER_TABLE_SIZE - 1;
		break;

	case K_BACKSPACE:
		SETBUF (cl_name.string); // Sets stringbuf, len = buflen
		stringbuf[buflen - 1] = 0;
		Cbuf_AddTextLinef ("name " QUOTED_S, stringbuf);
		break;
/*
	case K_MOUSECLICK_BUTTON1:

		{
			extern int extmousex, extmousey;
			extern int newmousex, newmousey;
			int		x, y, rectx, recty;
			cbool match = false;

			for (y=0 ; y<NAMEMAKER_TABLE_SIZE ; y++)
			{
				for (x=0 ; x<NAMEMAKER_TABLE_SIZE ; x++)
				{
					rectx = (32 + (16 * x));
					recty = 40 + (8 * y);
					rectx = rectx + ((vid.width - 320)>>1);

					if (extmousex >= rectx && extmousey >=recty)
					{
						if (extmousex <=rectx+7 && extmousey <= recty+7)
						{
							namemaker_cursor_x = x;
							namemaker_cursor_y = y;
							match = true;
						}
					}
					if (match) break;
				}
				if (match) break;
			}

			if (!match)
			{
				// Baker: nothing was hit
				return;
			}
		}
*/

		// If we reached this point, we are simulating ENTER

	case K_SPACE:
	case K_ENTER:
//		if (namemaker_cursor_y == NAMEMAKER_TABLE_SIZE)
//		{
//			M_Menu_Setup_f ();
//		}
//		else
//		{
			l = strlen (cl_name.string);
			if (l < 15)
			{
				int newchar = NAMEMAKER_TABLE_SIZE * (namemaker_cursor_y + 1) + namemaker_cursor_x;
				SETBUF (cl_name.string); // Sets stringbuf, len = buflen
				stringbuf[buflen + 1] = 0;
//				if (newchar == 0 || (newchar <= 13))
//					newchar = ' ';  // These characters cause too much chaos.
				stringbuf[buflen] = newchar;
				Cbuf_AddTextLinef ("name " QUOTED_S, stringbuf);
			}
//		}
		break;

	default:
		if (key < SPACE_CHAR_32 || key > MAX_ASCII_DELETE_CHAR_127)
			break;

		l = strlen (cl_name.string);
		if ( (key == 'v' || key == 'V') && Key_Ctrl_Down())
		{
			if (strlen (Clipboard_Get_Text_Line ()))
				Cbuf_AddTextLinef ("name " QUOTED_S, Clipboard_Get_Text_Line() );
		}
		else
		if (l < 15)
		{
//			int newchar = NAMEMAKER_TABLE_SIZE * namemaker_cursor_y + namemaker_cursor_x;
			SETBUF (cl_name.string); // Sets stringbuf, len = buflen
			stringbuf[buflen + 1] = 0;
			stringbuf[buflen] = Key_Alt_Down() ? key | 128 : key;
			Cbuf_AddTextLinef ("name " QUOTED_S, stringbuf);
		}
		break;
	}
}

#if 0 // Farethee well IPX ...
//=============================================================================
/* NET MENU */

int	m_net_cursor;
int m_net_items;

const char *net_helpMessage [] =
{
/* .........1.........2.... */
  " Novell network LANs    ",
  " or Windows 95 DOS-box. ",
  "                        ",
  "(LAN=Local Area Network)",

  " Commonly used to play  ",
  " over the Internet, but ",
  " also used on a Local   ",
  " Area Network.          "
};

void M_Menu_Net_f (void)
{
	Key_SetDest (key_menu);
	m_state = m_net;
	m_entersound = true;
	m_net_items = 2;

	if (m_net_cursor >= m_net_items)
		m_net_cursor = 0;
	m_net_cursor--;
	M_Net_Key (K_DOWNARROW);
}


void M_Net_Draw (void)
{
	int		f;
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPicCentered (4, p);

	f = 32;

	p = Draw_CachePic ("gfx/dim_ipx.lmp");
	M_DrawTransPic (72, f, p);

	f += 19;
	if (ipv4Available || ipv6Available)
		p = Draw_CachePic ("gfx/netmen4.lmp");
	else
		p = Draw_CachePic ("gfx/dim_tcp.lmp");
	M_DrawTransPic (72, f, p);

	f = (320-26*8)/2;
	M_DrawTextBox (f, 96, 24, 4);
	f += 8;
	M_Print (f, 104, net_helpMessage[m_net_cursor*4+0]);
	M_Print (f, 112, net_helpMessage[m_net_cursor*4+1]);
	M_Print (f, 120, net_helpMessage[m_net_cursor*4+2]);
	M_Print (f, 128, net_helpMessage[m_net_cursor*4+3]);

	f = (int)(realtime * 10)%6;
	M_DrawTransPic (54, 32 + m_net_cursor * 20, Draw_CachePic( va("gfx/menudot%d.lmp", f+1 ) ) );
}


void M_Net_Key (int k)
{
again:
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_net_cursor >= m_net_items)
			m_net_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_net_cursor < 0)
			m_net_cursor = m_net_items - 1;
		break;

	case K_ENTER:
		m_entersound = true;
		M_Menu_LanConfig_f ();
		break;
	}

	if (m_net_cursor == 0)
		goto again;
	if (m_net_cursor == 1 && !(ipv4Available || ipv6Available))
		goto again;
}
#endif // End Farethee well IPX ...

//=============================================================================
/* OPTIONS MENU */

typedef enum
{
	opt_customize_00 = 0,
	opt_goconsole_01,
	opt_reset_02,
	opt_screensize_03,
	opt_contrast_04,
	opt_gamma_041,
	opt_mousespeed_05,
	opt_cdvolume_06,
	opt_soundvolume_07,
	//opt_external_music_07b,
	opt_alwaysrun_08,
	opt_usemouse_09,
	opt_invertmouse_10,
	opt_lookspring_11,
#ifdef CLASSIC_BUILD
	opt_lookstrafe_12,
#endif
	//opt_preferences_12b,
	opt_videomode_13,
	OPTIONS_ITEMS
} mopt_t;

// Draw type (none, slider, checkbox
// Range  (hi/low)
// Action
typedef void (*qboolfunc_t)(cbool);
typedef enum { mdraw_none, mdraw_check, mdraw_slide, mdraw_vmode, mdraw_prefs} mdraw_t;
typedef enum { op_nonzero, op_greaterthan, op_lessthan } moperation_t; // True is determined by what

typedef struct
{
	mopt_t 		optnum;
	const char *label;
	const char *description;
	mdraw_t		control_type;
	cvar_t*		cvar_eval;
	float		a, b, c;
	qboolfunc_t	btoggleFunc; // Any mdraw_check that isn't op_nonzero must have this
	cbool	disabled;
} moptions_t;

void M_AlwaysRun_Toggle (cbool action)
{
	if (action)
	{
		Cvar_SetValueQuick (&cl_forwardspeed, 400);
		Cvar_SetValueQuick (&cl_backspeed, 400);
	}
	else
	{
		Cvar_SetValueQuick (&cl_forwardspeed, 200);
		Cvar_SetValueQuick (&cl_backspeed, 200);
	}
}


void M_Pitch_Toggle (cbool action)
{
	Cvar_SetValueQuick (&m_pitch, -m_pitch.value);
}

moptions_t menu_options_draw [] =
{
	{opt_customize_00, 	"    Customize controls", "", mdraw_none},
	{opt_goconsole_01, 	"         Go to console", "", mdraw_none},
	{opt_reset_02, 		"     Reset to defaults", "", mdraw_none},
// slider: #increments, low, high
	{opt_screensize_03, "           Screen size", "", mdraw_slide, &scr_viewsize, 	10, 30, 120},
	// Baker: WinQuake's brightness slider is actually gamma.
	{opt_contrast_04,	"              Contrast", "", mdraw_slide,  &vid_contrast, 21, VID_MIN_CONTRAST, VID_MAX_CONTRAST},
	{opt_gamma_041,		"                 Gamma", "", mdraw_slide,  &vid_gamma, 21, VID_MAX_MENU_GAMMA, VID_MIN_MENU_GAMMA},
//	{opt_contrast_04, "            Brightness", "", mdraw_slide, &vid_contrast, 21, VID_MIN_CONTRAST, VID_MAX_CONTRAST},
	{opt_mousespeed_05, "           Mouse Speed", "", mdraw_slide, &sensitivity, 	11, 1, 11},
	{opt_cdvolume_06, 	"          Music Volume", "", mdraw_slide, &bgmvolume, 		11, 0, 3},
	{opt_soundvolume_07,"          Sound Volume", "", mdraw_slide, &sfxvolume, 		21, 0, 1},
	//{opt_external_music_07b,"        External Music", "cd or mp3 music", mdraw_check, &external_music, op_nonzero},
// mdraw_check: operation and value.
	{opt_alwaysrun_08,	"            Always Run", "", mdraw_check, &cl_forwardspeed, 	op_greaterthan, 200,0, M_AlwaysRun_Toggle},
	{opt_usemouse_09,   "            Mouse Look", "", mdraw_check, &in_freelook, 		op_nonzero },
	{opt_invertmouse_10,"          Invert Mouse", "", mdraw_check, &m_pitch, 			op_lessthan, 0,0, M_Pitch_Toggle },
	{opt_lookspring_11, "            Lookspring", "levels view for keyboarders", mdraw_check, &lookspring, 		op_nonzero},
#ifdef CLASSIC_BUILD
	{opt_lookstrafe_12, "            Lookstrafe", "left/right mouse moves side-to-side", mdraw_check, &lookstrafe, 		op_nonzero },
#endif
	//{opt_preferences_12b, "          Modern stuff",   "", mdraw_prefs },
	{opt_videomode_13, 	"         Video Options", "", mdraw_vmode },
};
int num_menu_options_draw = sizeof(menu_options_draw) / sizeof(menu_options_draw[0]);
int	options_available = OPTIONS_ITEMS;

static float Evaluate (const moptions_t* myopt)
{
	if (myopt->control_type == mdraw_check)
	{
		int evaltype = (int)myopt->a;
		float eval_number = myopt->b;
		switch (evaltype)
		{
			case op_nonzero:
				return myopt->cvar_eval->value != 0;
			case op_greaterthan:
				return myopt->cvar_eval->value > eval_number;
			case op_lessthan:
				return myopt->cvar_eval->value < eval_number;
			default:
				System_Error ("Unknown eval type");
		}

	}

	if (myopt->control_type == mdraw_slide)
	{
//		int num_ticks =  (int)myopt->a;
		float lowbar =  myopt->b;
		float highbar =  myopt->c;
		float tick_range = highbar - lowbar;
		float into_range = (myopt->cvar_eval->value - lowbar) / tick_range;

		return into_range;
	}

	return 0;
}




#define	SLIDER_RANGE	10

int		options_cursor;

void M_Menu_Options_f (void)
{
	Key_SetDest (key_menu);
	m_state = m_options;
	m_entersound = true;
}

void M_AdjustSliders (int options_cursor, float dir) //fdossena: replaced int dir with float dir to allow for finer control on the sliders
{
	const moptions_t* myopt = &menu_options_draw[options_cursor];
	S_LocalSound ("misc/menu3.wav");

	switch (myopt->control_type)
	{
	case mdraw_slide:
		{
			cbool slider_positive = myopt->c > myopt->b;
			float highval = slider_positive ? myopt->c : myopt->b;
			float lowval = slider_positive ? myopt->b : myopt->c;

			float unitamount = (myopt->c - myopt->b) / (myopt->a - 1);

			float newcvarval = myopt->cvar_eval->value +  unitamount * dir;
#if 0
			Con_PrintLinef ("Unit amount is %g", unitamount);
			Con_PrintLinef ("Dir %d Cur value is %g and wanting to add %g so newval is %g",
					dir, myopt->cvar_eval->value, unitamount*dir, newcvarval);
#endif
			newcvarval = CLAMP (lowval, newcvarval, highval);

			Cvar_SetValueQuick (myopt->cvar_eval, newcvarval);
			break;
		}

	case mdraw_check:
		{
			int evaltype = (int)myopt->a;
			float eval_number = myopt->b;
			cbool was_true;
			switch (evaltype)
			{
				case op_nonzero:
					Cvar_SetValueQuick (myopt->cvar_eval, !myopt->cvar_eval->value);
					break;
				case op_greaterthan:
					was_true = myopt->cvar_eval->value > eval_number;
					myopt->btoggleFunc(!was_true);
					break;
				case op_lessthan:
					was_true = myopt->cvar_eval->value < eval_number;
					myopt->btoggleFunc(!was_true);
					break;
				default:
					System_Error ("Unknown eval type");
			}
			break;
		}
	default:
		break;
	}
}


void M_DrawSlider (int x, int y, float range)
{
	int	i;

	if (range < 0)
		range = 0;
	if (range > 1)
		range = 1;
	M_DrawCharacter (x-8, y, 128);
	for (i=0 ; i<SLIDER_RANGE ; i++)
		M_DrawCharacter (x + i*8, y, 129);
	M_DrawCharacter (x+i*8, y, 130);
	M_DrawCharacter (x + (SLIDER_RANGE-1)*8 * range, y, 131);
}

void M_DrawCheckbox (int x, int y, int on)
{
#if 0
	if (on)
		M_DrawCharacter (x, y, 131);
	else
		M_DrawCharacter (x, y, 129);
#endif
	if (on)
		M_Print (x, y, "on");
	else
		M_Print (x, y, "off");
}

void M_Options_Draw (void)
{
	int i, col_16 = 16, col_220 = 220, row;
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_option.lmp");
	M_DrawPicCentered (4, p);


#if defined(GLQUAKE_RENDERER_SUPPORT) && !defined (DIRECT3D9_WRAPPER) // dx9 = No ... future (vid_shadergamma.value == 0)
	if ((menu_options_draw[opt_gamma_041].disabled = (vid_hardwaregamma.value == 0) ))
	{
		options_available = num_menu_options_draw - 1; // Disabled
		if (options_cursor >= options_available)
			options_cursor = options_available - 1;
	}
	else options_available = num_menu_options_draw;
#endif // GLQUAKE_RENDERER_SUPPORT && !DIRECT3D9_WRAPPER

	for (i = 0, row = 32; i < num_menu_options_draw; i ++)
	{
		if (menu_options_draw[i].disabled)
			continue;

		M_Print (col_16, row, menu_options_draw[i].label);
		switch (menu_options_draw[i].control_type)
		{
		case mdraw_check:
			M_DrawCheckbox (col_220, row, Evaluate(&menu_options_draw[i]) );
			break;
		case mdraw_slide:
			M_DrawSlider (col_220, row, Evaluate(&menu_options_draw[i]) );
			break;
		case mdraw_vmode:
#ifdef WINQUAKE_RENDERER_SUPPORT
	if (vid.stretch_x > 1) {
		M_Print (col_220, row, va("%dx%d x%d", vid.screen.width, vid.screen.height, (int)vid.stretch_x) );
	}
	else
#endif
		M_Print (col_220, row, va("%dx%d", vid.screen.width, vid.screen.height) );

		default:
			break;
		}
		row += 8;
	}

	M_PrintWhite (160 - strlen(menu_options_draw[options_cursor].description)*(8/2), row + 8, menu_options_draw[options_cursor].description);

// cursor
	M_DrawCharacter (200, 32 + options_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Options_Key (int k)
{
	int key_action_cursor = options_cursor;

	if (menu_options_draw[opt_gamma_041].disabled && key_action_cursor >= opt_gamma_041)
		key_action_cursor ++;

	switch (k)
	{
	case K_ESCAPE:
		Host_WriteConfiguration (); // Baker: Save config here to guarantee time and effort setting up is not lost.
		M_Menu_Main_f (NULL);
		break;

	case K_ENTER:
		m_entersound = true;
		switch (key_action_cursor)
		{
		case opt_customize_00:
			M_Menu_Keys_f ();
			break;
		case opt_goconsole_01:
//			m_state = m_none;
			Con_ToggleConsole_f (NULL);
			break;
		case opt_reset_02:
			if (!SCR_ModalMessage("Are you sure you want to reset" NEWLINE "all keys and settings?", 0, false))
					break;
			Cbuf_AddTextLine ("resetall"); //johnfitz
			Cbuf_AddTextLine ("exec default.cfg");
			break;
		/*case opt_preferences_12b:
			M_Menu_Preferences_f ();
			break;*/
		case opt_videomode_13:
			M_Menu_Video_f ();
			break;
		default:
			M_AdjustSliders (key_action_cursor, 1);
			break;
		}
		return;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		options_cursor--;
		if (options_cursor < 0)
			options_cursor = options_available - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		options_cursor++;
		if (options_cursor >= options_available)
			options_cursor = 0;
		break;

	case K_LEFTARROW:
		M_AdjustSliders (key_action_cursor, -0.5);
		break;

	case K_RIGHTARROW:
		M_AdjustSliders (key_action_cursor, 0.5);
		break;
	}

}

//=============================================================================
/* PREFERENCES MENU */

int		preferences_cursor;
#define PREFERENCES_ITEMS 15



void M_Menu_Preferences_f (void)
{
	Key_SetDest (key_menu); m_state = m_preferences;
	m_entersound = true;
}

void M_Preferences_Draw (void)
{
	int col1 = 7*8;
	int col2 = 22*8;
	int cursor_row, cursor_col;
	int i = 24;
	qpic_t	*p;
#define FUHQUAKE_POSITION_1_2 (1.0 + 13.0/64.0) //  1.203125 .. should be a nice and solid binary-friendly number for floating point storage
	const char *_crosshair	= scr_crosshair.value > 1 ? "Small Dot" : (scr_crosshair.value ? "Quake Default" : "None");
	const char *_weapondraw = r_viewmodel_offset.value ? "Gangsta" : r_viewmodel_quake.value ? (r_viewmodel_size.value >= FUHQUAKE_POSITION_1_2 ? "High" : "Quake Default") : "FitzQuake (Low)"; // 1 + 13/64 = 1.203125
	const char *_invisibility = r_viewmodel_ring.value ? "Draw Weapon" : "Quake Default";
	const char *_viewblends = v_polyblend_lite.value ? "Lite" : (v_polyblend.value ? "Quake Default" : "None");
	const char *_bobbing	= cl_bob.value ? (cl_sidebobbing.value ? "DarkPlaces" : "Quake Default") : "None";
#ifdef GLQUAKE_FLASH_BLENDS
	const char *_flashblend = gl_flashblend.value ? "GLQuake Style" : "Quake Default";
#else
	const char *_flashblend = "(GL Only)";
#endif // GLQUAKE_FLASH_BLENDS
	const char *_stainmaps  = r_stains.value ? "Subtle" : "Off";
	const char *_startdemos = host_startdemos.value ? "Quake Default" : "Do not start";
	const char *_server_aim = sv_aim.value >= 1.0 ? "Aim Help Off" : "Quake Default";
	const char *_clock		= scr_clock.value >= 1 ? "Always" : (scr_clock.value ? "Deathmatch Only" : "Never");
#ifdef GLQUAKE_RENDERER_SUPPORT
	const char *_scaling	= scr_scaleauto.value > 2 ? "Auto Large" :
							  scr_scaleauto.value > 1 ? "Auto Medium":
							  scr_scaleauto.value > 0 ? "Auto Small":
							  scr_scaleauto.value < 0 ? "Forced Off" : "User Cvar Control";
#else
	const char *_scaling	= "(GL Only)";
#endif // GLQUAKE_FLASH_BLENDS

	const char *_statusbar	= scr_viewsize.value  == 110	? "Minimal (QW-ish)":
							scr_sbaralpha.value < 1		? "Translucent (GL)":
							scr_sbarcentered.value		?   "Centered":  "Quake Default";
	const char *_effects	=
#ifdef GLQUAKE_SUPPORTS_QMB // def GLQUAKE_RENDERER_SUPPORT
		qmb_active.value ?	"JoeQuake (QMB)" :
#endif // GLQUAKE_SUPPORTS_QMB
							 !r_lerpmodels.value ?  "Normal + Jerky" : "Normal";



	const char *help_text[] =
	{
		"Standard Quake crosshair is a '+'",
		"FitzQuake position is lower",
		"Default: invisible = no weapon draw",
		"Screen blend underwater, powerup, ..",
		"Bobbing: cl_bob, cl_rollangle, ..",
		"Moving dynamic light drawing",
		"Darkened stains that gradually fade",
		"Play startup demos on Quake startup",
		"Lite aim help mostly for keyboarders",
		"Show amount of time into level",
		"Adjust status bar for resolution",
		"Only default is uncentered in deathmatch",
		"Original had jerky stairs/monsters", //, QMB is GL only",
		"Set FitzQuake 0.85 default settings",
		"Set Mark V revised settings",
	};

/*
Crosshair			Quake Default | None | Dot
Weapon position		Quake Default | FitzQuake Default

View Blends			Quake Default | Lite
Bobbing/Kicks		Quake Default | Standard + Side Bobbing | None
Flashes				Quake Default | GLQuake flashblend
Water Alpha

Show clock			On | Off | Speed Runner (shows kills + monsters)
server aim "sv_aim" Quake Default | Off
*/

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_option.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	M_Print     (col1, i+=8, "   Crosshair");	M_Print (col2, i, _crosshair);
	M_Print     (col1, i+=8, " Weapon Draw");	M_Print (col2, i, _weapondraw);
	M_Print     (col1, i+=8, "Invisibility");	M_Print (col2, i, _invisibility);
	M_Print     (col1, i+=8, " View Blends");	M_Print (col2, i, _viewblends);
	M_Print     (col1, i+=8, "     Bobbing");	M_Print (col2, i, _bobbing);
	M_Print     (col1, i+=8, "  Flashblend");	M_Print (col2, i, _flashblend);
	M_Print     (col1, i+=8, "  Stain Maps");	M_Print (col2, i, _stainmaps);
	i += 8;
	M_Print     (col1, i+=8, "  Startdemos");	M_Print (col2, i, _startdemos);
	M_Print     (col1, i+=8, "  Server Aim");	M_Print (col2, i, _server_aim);
	M_Print     (col1, i+=8, "  Draw Clock");	M_Print (col2, i, _clock);
	i += 8;
	M_Print     (col1, i+=8, "   Autoscale");	M_Print (col2, i, _scaling);
	M_Print     (col1, i+=8, "  Status Bar");	M_Print (col2, i, _statusbar);
	M_Print     (col1, i+=8, "     Effects");	M_Print (col2, i, _effects);
//	i += 8;
	i += 8;
	M_Print     (14*8, i+=8, "Set To FitzQuake");
	M_Print     (14*8-4, i+=8, "  Set To Mark V");
	i += 8;
//	i += 8;

	cursor_col = col2-16;
	if (preferences_cursor < 7 )
		cursor_row = 32 + preferences_cursor * 8;
	else if (preferences_cursor < 10) // was 10
		cursor_row = 40 + preferences_cursor * 8; // The space between stain/start demos
	else if (preferences_cursor < 13) // was 10
		cursor_row = 48 + preferences_cursor * 8; // The space between stain/start demos
	else
	{
		cursor_col = 12 * 8;
		cursor_row = 56 + preferences_cursor * 8;
	}

	M_PrintWhite (20*8 - strlen(help_text[preferences_cursor])*(8/2), i += 8, help_text[preferences_cursor]);

	M_DrawCharacter (cursor_col, cursor_row, 12+((int)(realtime*4)&1));
}


void M_Preferences_Key (int k)
{
	int dir = 0, newval;

	switch (k)
	{
	case K_ESCAPE:
		Host_WriteConfiguration (); // Baker: Save config here to guarantee time and effort setting up is not lost.
		M_Menu_Options_f ();
		return;

	case K_ENTER:
	case K_RIGHTARROW:
		dir = 1;
		break;

	case K_LEFTARROW:
		dir = -1;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		preferences_cursor--;
		if (preferences_cursor < 0)
			preferences_cursor = PREFERENCES_ITEMS-1;
		return;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		preferences_cursor++;
		if (preferences_cursor >= PREFERENCES_ITEMS)
			preferences_cursor = 0;
		return;

	}

	S_LocalSound ("misc/menu3.wav");

	if (!dir)
		return;

	switch (preferences_cursor)
	{
	case 0: // Crosshair
		     if (scr_crosshair.value > 1)	Cvar_SetValueQuick (&scr_crosshair, dir > 0 ? 0 : 1);
		else if (scr_crosshair.value)		Cvar_SetValueQuick (&scr_crosshair, dir > 0 ? 2 : 0);
		else							Cvar_SetValueQuick (&scr_crosshair, dir > 0 ? 1 : 2);
		break;

	case 1: // Weapon
		newval = r_viewmodel_offset.value ? 3 : r_viewmodel_size.value >= FUHQUAKE_POSITION_1_2 ? 2 : r_viewmodel_quake.value ? 1 : 0;
		newval += dir;
		switch (newval) {
		case 4:	// Flips to 0.  Fall through ...

		case 0:	// We are FitzQuake
			Cvar_ResetQuick (&r_viewmodel_quake);
			Cvar_ResetQuick (&r_viewmodel_size);
			Cvar_ResetQuick (&r_viewmodel_offset);
			break;

		case 1: // We are Quake
			Cvar_SetValueQuick (&r_viewmodel_quake, 1.0);
			Cvar_ResetQuick (&r_viewmodel_size);
			Cvar_ResetQuick (&r_viewmodel_offset);

			break;
		case 2: // We are FuhQuake
			Cvar_SetValueQuick (&r_viewmodel_quake, 1.0);
			Cvar_SetValueQuick (&r_viewmodel_size, FUHQUAKE_POSITION_1_2);
			Cvar_ResetQuick (&r_viewmodel_offset);
			break;

		case 3: // Fall through ...
		case -1: // Flips to 3.  We are Cool Guy
			Cvar_SetValueQuick (&r_viewmodel_quake, 1.0);
			Cvar_SetValueQuick (&r_viewmodel_size, FUHQUAKE_POSITION_1_2);
			Cvar_SetValueQuick (&r_viewmodel_offset, 5.0);
			break;
		default:	System_Error ("Menu range");
		}
		break;

	case 2: // Invisibility Weapon
		Cvar_SetValueQuick (&r_viewmodel_ring, !r_viewmodel_ring.value);
		break;

	case 3: // View Blends

		newval = 0;

			if (v_polyblend_lite.value) newval = 1;
		else if (!v_polyblend.value) newval = 2;

		newval = newval + dir;

		if (newval == -1) newval = 2;
		if (newval == 3) newval = 0;

		switch (newval)
		{
		case 0: // Quake Default
			Cvar_ResetQuick (&v_polyblend);
			Cvar_ResetQuick (&v_polyblend_lite);
			Cvar_ResetQuick (&r_waterwarp);
			break;
		case 1: // Lite
			Cvar_ResetQuick (&v_polyblend);
			Cvar_SetValueQuick (&v_polyblend_lite, 1);
			Cvar_ResetQuick (&r_waterwarp);					// r_waterwarp defaults to 1
			break;
		case 2: // None
			Cvar_SetValueQuick (&v_polyblend, 0);
			Cvar_ResetQuick (&v_polyblend_lite);
			Cvar_SetValueQuick (&r_waterwarp, 0);			// Turn it off.
			break;
		}

		break;

	case 4: // Bobbing

		newval = 1;

		if (cl_sidebobbing.value) newval = 2;
		else if (cl_bob.value) newval = 0;

		newval = newval + dir;

		if (newval == -1) newval = 2;
		if (newval == 3) newval = 0;

		switch (newval)
		{
		case 0:  // Quake default
			Cvar_ResetQuick (&v_gunkick); // 9 cvars + sidebobbing
			Cvar_ResetQuick (&v_kickpitch);
			Cvar_ResetQuick (&v_kickroll);
			Cvar_ResetQuick (&v_kicktime);
			Cvar_ResetQuick (&cl_bob);
			Cvar_ResetQuick (&cl_bobcycle);
			Cvar_ResetQuick (&cl_bobup);
			Cvar_ResetQuick (&cl_rollangle);
			Cvar_ResetQuick (&cl_sidebobbing);
			break;
		case 2:  // Quake default + side
			Cvar_ResetQuick (&v_gunkick); // 9 cvars + sidebobbing
			Cvar_ResetQuick (&v_kickpitch);
			Cvar_ResetQuick (&v_kickroll);
			Cvar_ResetQuick (&v_kicktime);
			Cvar_ResetQuick (&cl_bob);
			Cvar_ResetQuick (&cl_bobcycle);
			Cvar_ResetQuick (&cl_bobup);
			Cvar_ResetQuick (&cl_rollangle);
			Cvar_SetValueQuick (&cl_sidebobbing, 1.0);
			break;
		default:
			Cvar_SetValueQuick (&v_gunkick, 0); // 9 cvars + sidebobbing
			Cvar_SetValueQuick (&v_kickpitch, 0);
			Cvar_SetValueQuick (&v_kickroll, 0);
			Cvar_SetValueQuick (&v_kicktime, 0);
			Cvar_SetValueQuick (&cl_bob, 0);
			Cvar_SetValueQuick (&cl_bobcycle, 0);
			Cvar_SetValueQuick (&cl_bobup, 0);
			Cvar_SetValueQuick (&cl_rollangle, 0);
			Cvar_ResetQuick (&cl_sidebobbing);
			break;
		}

		break;

	case 5: // Flashblend
#ifdef GLQUAKE_FLASH_BLENDS
		Cvar_SetValueQuick (&gl_flashblend, !gl_flashblend.value);
#endif // GLQUAKE_FLASH_BLENDS
		break;

	case 6: // Stain maps
		Cvar_SetValueQuick (&r_stains, !r_stains.value);
		break;

	case 7: // Startdemos
		Cvar_SetValueQuick (&host_startdemos, !host_startdemos.value);
		break;

	case 8: // Server Aim
		Cvar_SetValueQuick (&sv_aim, sv_aim.value >= 1 ? 0.93 : 2);
		break;

	case 9: // Clock
		     if (scr_clock.value >= 1)	Cvar_SetValueQuick (&scr_clock, dir > 0 ? -1 :  0);
		else if (scr_clock.value >= 0)	Cvar_SetValueQuick (&scr_clock, dir > 0 ?  1 : -1);
		else							Cvar_SetValueQuick (&scr_clock, dir > 0 ?  0 :  1);
		break;

	case 10: // Autoscale
			 if (scr_scaleauto.value > 2)	Cvar_SetValueQuick (&scr_scaleauto, dir > 0 ? -1 : 2);  // Large (2) to User-defined.
		else if (scr_scaleauto.value > 1)	Cvar_SetValueQuick (&scr_scaleauto, dir > 0 ?  3 : 1);  // (2) Large (2) to User-defined.
		else if (scr_scaleauto.value > 0)	Cvar_SetValueQuick (&scr_scaleauto, dir > 0 ?  2 : 0);
		else if (scr_scaleauto.value ==0)   Cvar_SetValueQuick (&scr_scaleauto, dir > 0 ?  1 : -1);
		else							    Cvar_SetValueQuick (&scr_scaleauto, dir > 0 ?  0 : 3);
		break;
	case 11:	// Status bar
		newval = (scr_viewsize.value  == 110) ? 3 : (scr_sbaralpha.value < 1) ? 2 : (scr_sbarcentered.value) ? 1 : 0;
		newval += dir;
		switch (newval) {
		case 4:	// Flips to 0.  Fall through ...

		case 0:	// We are original
			Cvar_ResetQuick (&scr_sbaralpha);
			Cvar_SetValueQuick (&scr_sbarcentered, 0.0);
			Cvar_ResetQuick (&scr_viewsize);
			break;

		case 1: // We are original, centered
			Cvar_ResetQuick (&scr_sbaralpha);
			Cvar_ResetQuick (&scr_sbarcentered); // Default is 1!!!
			Cvar_ResetQuick (&scr_viewsize);

			break;
		case 2: // We are transparent 50%
			Cvar_SetValueQuick (&scr_sbaralpha, 0.5);
			Cvar_ResetQuick (&scr_sbarcentered);  // Default is 1!!!
			Cvar_ResetQuick (&scr_viewsize);
			break;

		case 3: // Fall through ...
		case -1: // Flips to 3.  We are Quakeworld-ish
			Cvar_SetValueQuick (&scr_sbaralpha, 0.0);
			Cvar_ResetQuick (&scr_sbarcentered);  // Default is 1!!!
			Cvar_SetValueQuick (&scr_viewsize, 110);
			break;
		default:	System_Error ("Menu range");
		}

		break;

	case 12:	// Effects
#ifdef GLQUAKE_SUPPORTS_QMB // GLQUAKE_RENDERER_SUPPORT
		newval = (qmb_active.value) ? 2 : (!r_lerpmodels.value) ? 0 : 1;
#else // ! GLQUAKE_SUPPORTS_QMB

		newval = (!r_lerpmodels.value) ? 0 : 1;
#endif // Not GLQUAKE_SUPPORTS_QMB

		newval += dir;
#ifdef GLQUAKE_SUPPORTS_QMB // GLQUAKE_RENDERER_SUPPORT
		if (newval == -1) newval = 2;
		if (newval ==  3) newval = 0;
#else // not GLQUAKE_SUPPORTS_QMB
		if (newval == -1) newval = 1;
		if (newval ==  2) newval = 0;
#endif // GLQUAKE_SUPPORTS_QMB

		switch (newval) {
		case 1:	// We are original
			Cvar_ResetQuick (&v_smoothstairs);
			Cvar_ResetQuick (&r_lerpmodels);
			Cvar_ResetQuick (&r_lerpmove);
#ifdef GLQUAKE_SUPPORTS_QMB // GLQUAKE_RENDERER_SUPPORT
			Cvar_ResetQuick (&qmb_active);
#endif // GLQUAKE_SUPPORTS_QMB
			break;

		case 2: // We are QMB
			Cvar_ResetQuick (&v_smoothstairs);
			Cvar_ResetQuick (&r_lerpmodels);
			Cvar_ResetQuick (&r_lerpmove);
#ifdef GLQUAKE_SUPPORTS_QMB // GLQUAKE_RENDERER_SUPPORT
			Cvar_SetValueQuick (&qmb_active, 1);
#endif // GLQUAKE_SUPPORTS_QMB
			break;

		case 0: // Jerky retro
			Cvar_SetValueQuick (&v_smoothstairs, 0);
			Cvar_SetValueQuick (&r_lerpmodels, 0);
			Cvar_SetValueQuick (&r_lerpmove, 0);
#ifdef GLQUAKE_SUPPORTS_QMB // GLQUAKE_RENDERER_SUPPORT
			Cvar_ResetQuick (&qmb_active);
#endif // GLQUAKE_SUPPORTS_QMB
			break;
		default:	System_Error ("Menu range");
		}
		break;

	case 13: // Set to FitzQuake
		Cvar_ResetQuick (&scr_crosshair);
		Cvar_ResetQuick (&r_viewmodel_quake);
		Cvar_ResetQuick (&r_viewmodel_offset);
		Cvar_ResetQuick (&r_viewmodel_ring);
		Cvar_ResetQuick (&v_polyblend);
		Cvar_ResetQuick (&v_polyblend_lite);
		Cvar_SetValueQuick (&r_waterwarp, 2); // FitzQuake waterwarp is now 2.
#ifdef GLQUAKE_FLASH_BLENDS
		Cvar_ResetQuick (&gl_flashblend);
#endif // GLQUAKE_FLASH_BLENDS
		Cvar_SetValueQuick (&r_stains, 0);
		Cvar_ResetQuick (&sv_aim);
		Cvar_SetValueQuick (&scr_clock, 0);

		Cvar_ResetQuick (&v_gunkick);
		Cvar_ResetQuick (&v_kickpitch);
		Cvar_ResetQuick (&v_kickroll);
		Cvar_ResetQuick (&v_kicktime);
		Cvar_ResetQuick (&cl_bob);
		Cvar_ResetQuick (&cl_bobcycle);
		Cvar_ResetQuick (&cl_bobup);
		Cvar_ResetQuick (&cl_rollangle);
		Cvar_ResetQuick (&cl_sidebobbing);
		Cvar_ResetQuick (&host_startdemos);

		Cvar_ResetQuick (&v_smoothstairs);
		Cvar_ResetQuick (&r_lerpmodels);
		Cvar_ResetQuick (&r_lerpmove);
#ifdef GLQUAKE_SUPPORTS_QMB //GLQUAKE_RENDERER_SUPPORT
		Cvar_ResetQuick (&qmb_active);
#endif // GLQUAKE_SUPPORTS_QMB
		Cvar_ResetQuick (&scr_sbaralpha);
		Cvar_SetValueQuick (&scr_sbarcentered, 0.0);
		Cvar_ResetQuick (&scr_viewsize);
		break;

	case 14: // Set to Mark V
		Cvar_SetValueQuick (&scr_crosshair, 2);
		Cvar_ResetQuick (&r_viewmodel_quake);
		Cvar_ResetQuick (&r_viewmodel_offset);
		Cvar_SetValueQuick (&r_viewmodel_ring, 1);
		Cvar_ResetQuick (&v_polyblend);
		Cvar_SetValueQuick (&v_polyblend_lite, 1);
		Cvar_ResetQuick (&r_waterwarp);

#ifdef GLQUAKE_FLASH_BLENDS
		Cvar_SetValueQuick (&gl_flashblend, 0);
#endif // GLQUAKE_FLASH_BLENDS

		Cvar_ResetQuick (&r_stains);
		Cvar_ResetQuick (&sv_aim);
		Cvar_ResetQuick (&scr_clock);

		Cvar_ResetQuick (&v_gunkick);
		Cvar_ResetQuick (&v_kickpitch);
		Cvar_ResetQuick (&v_kickroll);
		Cvar_ResetQuick (&v_kicktime);
		Cvar_ResetQuick (&cl_bob);
		Cvar_ResetQuick (&cl_bobcycle);
		Cvar_ResetQuick (&cl_bobup);
		Cvar_ResetQuick (&cl_rollangle);
		Cvar_ResetQuick (&cl_sidebobbing);
		Cvar_ResetQuick (&host_startdemos);

		Cvar_ResetQuick (&r_lerpmodels);
		Cvar_ResetQuick (&r_lerpmove);
		Cvar_ResetQuick (&v_smoothstairs);
#ifdef GLQUAKE_SUPPORTS_QMB //GLQUAKE_RENDERER_SUPPORT
		Cvar_ResetQuick (&qmb_active);
#endif // GLQUAKE_SUPPORTS_QMB
		Cvar_ResetQuick (&scr_sbaralpha);
		Cvar_ResetQuick (&scr_sbarcentered);  // Default is 1!!!
		Cvar_ResetQuick (&scr_viewsize);
		break;
	}
}

//=============================================================================
/* KEYS MENU */

const char *bindnames[][2] =
{
{"+attack", 		"attack"},
{"+jump", 			"jump"},
{"+forward", 		"move forward"},
{"+back", 			"move back"},
{"+moveleft", 		"move left"},
{"+moveright", 		"move right"},
{"+left", 			"turn left"},
{"+right", 			"turn right"},
{"+moveup", 		"swim up"},
{"+movedown", 		"swim down"},
{"impulse 10", 		"next weapon"},
{"impulse 12", 		"last weapon"},
{"+speed", 			"run"},
{"+lookup", 		"look up"},
{"+lookdown", 		"look down"},
{"+mlook", 			"mouse look"},
{"+klook", 			"keyboard look"},
{"+strafe",			"sidestep"},
{"centerview",		"center view"}
};

#define	NUMCOMMANDS	(sizeof(bindnames)/sizeof(bindnames[0]))

static int		keys_cursor;
cbool		m_keys_bind_grab;

void M_Menu_Keys_f (void)
{
	Key_SetDest (key_menu);
	m_state = m_keys;
	m_entersound = true;
}


void M_FindKeysForCommand (const char *command, int *threekeys)
{
	int		count;
	int		j;
	int		leng;
	const	char *b;

	threekeys[0] = threekeys[1] = threekeys[2] = -1;
	leng = strlen(command);
	count = 0;

	for (j = 0; j < KEYMAP_COUNT_512; j++)
	{
#ifdef SUPPORTS_KEYBIND_FLUSH
		// What we want to do here is show the current
		// effective key bindings
		// So if a server key bind is in effect, show that.
		b = Key_GetBinding (j);
#else // old way ...
		b = keybindings[j];
#endif // SUPPORTS_KEYBIND_FLUSH
		if (!b)
			continue;
		if (!strcmp (b, command/*, leng*/) ) // Formerly strncmp, caused "impulse 10" to match "impulse 101", etc.
		{
			threekeys[count] = j;
			count++;
			if (count == 3)
				break;
		}
	}
}


void M_UnbindCommand (const char *command)
{
	int		leng = strlen(command);
	int		j;
	const char *b;

	for (j = 0; j < KEYMAP_COUNT_512 ; j ++) {
#ifdef SUPPORTS_KEYBIND_FLUSH
// We want the server and user binds unbound
// Key_SetBinding should handle this situation properly in all cases.
		b = Key_GetBinding (j);
#else // old way ...
		b = keybindings[j];
#endif // !SUPPORTS_KEYBIND_FLUSH
		if (!b)
			continue;

		if (!strcmp (b, command /*, leng*/) ) // Formerly strncmp, caused "impulse 10" to match "impulse 101", etc.
			Key_SetBinding (j, "");
	}
}


void M_Keys_Draw (void)
{
	int		i, x, y;
	int		keys[3];
	const char	*name;
	qpic_t	*p;

	p = Draw_CachePic ("gfx/ttl_cstm.lmp");
	M_DrawPicCentered (4, p);

	if (m_keys_bind_grab)
		M_Print (12, 32, "Press a key or button for this action");
	else
		M_Print (18, 32, "Enter to change, backspace to clear");

// search for known bindings
	for (i=0 ; i < (int)NUMCOMMANDS ; i++) {
		y = 48 + 8*i;

		M_Print (16, y, bindnames[i][1]);
		M_FindKeysForCommand (bindnames[i][0], keys);

		x = 0;
		if (keys[0] == -1) {
			M_Print (140 + x, y, "???");
			continue;
		}

		name = Key_KeynumToString (keys[0], key_local_name);
		M_Print (140 + x, y, name); x += strlen(name) * 8;

		if (keys[1] == -1)
			continue;
		else M_Print (140 + x + 8, y, "or"); x += 4 /*chars*/ * 8;

		name = Key_KeynumToString (keys[1], key_local_name);

		M_Print (140 + x, y, name); 	x += strlen(name) * 8;

		if (keys[2] == -1)
			continue;
		else M_Print (140 + x + 8, y, "or"); x += 4 /*chars*/ * 8;

		name = Key_KeynumToString (keys[2], key_local_name);
		M_Print (140 + x, y, name); 	x += strlen(name) * 8;
	}

	if (m_keys_bind_grab)
		M_DrawCharacter (130, 48 + keys_cursor*8, '=');
	else
		M_DrawCharacter (130, 48 + keys_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Keys_Key (int k)
{
	char	cmd[80];
	int		keys[3];

	if (m_keys_bind_grab)
	{	// defining a key
		S_LocalSound ("misc/menu1.wav");
		if ((k != K_ESCAPE) && (k != '`'))
		{
			c_snprintf2 (cmd, "bind " QUOTED_S " " QUOTED_S NEWLINE, Key_KeynumToString (k, key_local_name), bindnames[keys_cursor][0]);
// Change me to not use the command buffer?  Shouldn't be necessary.

			Cbuf_InsertText (cmd);
		}

		m_keys_bind_grab = false;
		return;
	}

	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Options_f ();
		break;

	case K_LEFTARROW:
	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		keys_cursor--;
		if (keys_cursor < 0)
			keys_cursor = NUMCOMMANDS-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		keys_cursor++;
		if (keys_cursor >= (int) NUMCOMMANDS)
			keys_cursor = 0;
		break;

	case K_ENTER:		// go into bind mode
		M_FindKeysForCommand (bindnames[keys_cursor][0], keys);
		S_LocalSound ("misc/menu2.wav");
		if (keys[2] != -1)
			M_UnbindCommand (bindnames[keys_cursor][0]);
		m_keys_bind_grab = true;
		break;

	case K_BACKSPACE:		// delete bindings
	case K_DELETE:				// delete bindings
		S_LocalSound ("misc/menu2.wav");
		M_UnbindCommand (bindnames[keys_cursor][0]);
		break;
	}
}

//=============================================================================
/* HELP MENU */

int		help_page;
int		normal_help = 1;
int		normal_tileclear = 1;
#define	NUM_HELP_PAGES	6


void M_Menu_Help_f (void)
{
	Key_SetDest (key_menu);
	m_state = m_help;
	m_entersound = true;
	help_page = 0;
}



void M_Help_Draw (void)
{
//	int xofs = 42;//, y = 42;
	qpic_t* p = Draw_CachePic ("gfx/help0.lmp");
	if (!p)
		System_Error ("Can't find help0.lmp"); // Baker trick compiler

	switch (help_page)
	{
	default:
		M_DrawPic (0, 0, Draw_CachePic ( va("gfx/help%d.lmp", help_page)) );
		break;
	}
}


void M_Help_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f (NULL);
		break;

	case K_UPARROW:
	case K_RIGHTARROW:
		m_entersound = true;
		if (++help_page >= NUM_HELP_PAGES)
			help_page = 0;
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		m_entersound = true;
		if (--help_page < 0)
			help_page = NUM_HELP_PAGES-1;
		break;
	}

}

//=============================================================================
/* QUIT MENU */

int		msgNumber;
enum m_state_e	m_quit_prevstate;
cbool	wasInMenus;

void M_Menu_Quit_f (void)
{
	if (m_state == m_quit)
		return;
	wasInMenus = (key_dest == key_menu);
	Key_SetDest (key_menu);
	m_quit_prevstate = m_state;
	m_state = m_quit;
	m_entersound = true;
	msgNumber = rand()&7;
}


void M_Quit_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case 'n':
	case 'N':
		if (wasInMenus)
		{
			m_state = m_quit_prevstate;
			m_entersound = true;
		}
		else
		{
			Key_SetDest (key_game);
		}
		break;

	case 'Y':
	case 'y':
		Host_Quit();
		break;

	default:
		break;
	}

}

void M_Quit_Draw (void)
{
	if (wasInMenus)
	{
		m_state = m_quit_prevstate;
		m_recursiveDraw = true;
		M_Draw ();
		m_state = m_quit;
	}

	M_DrawTextBox (0, -2, 38, 23);
	M_PrintWhite (16, 10,  "  Quake version 1.09 by id Software"); // These shouldn't need newlines, this isn't a print operation (there is no cursor involved, you set X, Y)
	M_PrintWhite (16, 26,  "Programming        Art ");
	M_Print (16, 34,  " John Carmack       Adrian Carmack");
	M_Print (16, 42,  " Michael Abrash     Kevin Cloud");
	M_Print (16, 50,  " John Cash          Paul Steed");
	M_Print (16, 58,  " Dave 'Zoid' Kirsch");
	M_PrintWhite (16, 66,  "Design             Biz");
	M_Print (16, 74,  " John Romero        Jay Wilbur");
	M_Print (16, 82,  " Sandy Petersen     Mike Wilson");
	M_Print (16, 90,  " American McGee     Donna Jackson");
	M_Print (16, 98,  " Tim Willits        Todd Hollenshead");
	M_PrintWhite (16, 106, "Support            Projects");
	M_Print (16, 114, " Barrett Alexander  Shawn Green");
	M_PrintWhite (16, 122, "Sound Effects");
	M_Print (16, 130, " Trent Reznor and Nine Inch Nails");
	M_PrintWhite (16, 138, "Quake is a trademark of Id Software,");
	M_PrintWhite (16, 146, "inc., (c)1996 Id Software, inc. All");
	M_PrintWhite (16, 154, "rights reserved. NIN logo is a");
	M_PrintWhite (16, 162, "registered trademark licensed to");
	M_PrintWhite (16, 170, "Nothing Interactive, Inc. All rights");
	M_PrintWhite (16, 178, "reserved. Press y to exit");
}

//=============================================================================
/* LAN CONFIG MENU */

int		lanConfig_cursor = -1;
int		lanConfig_cursor_table [] = {72, 92, 100, 132};
#define NUM_LANCONFIG_CMDS	4

int 	lanConfig_port;
char	lanConfig_portname[6];
char	lanConfig_joinname[22];

void M_Menu_LanConfig_f (void)
{
	Key_SetDest (key_menu);
	m_state = m_lanconfig;
	m_entersound = true;
	if (lanConfig_cursor == -1)
		if (JoiningGame)
			lanConfig_cursor = JoiningGame ? 2 : 1; // If cursor was unset, then start us @ 2 for joining game else start us @ 1
	if (StartingGame && out_of_bounds(0, lanConfig_cursor, 1))
		lanConfig_cursor = 1;
	lanConfig_port = DEFAULTnet_hostport;
	c_snprintf1 (lanConfig_portname, "%u", lanConfig_port);

	m_return_onerror = false;
	m_return_reason[0] = 0;
}


void M_LanConfig_Draw (void)
{
	qpic_t	*p;
	int		basex;
	const char	*startJoin;
	const char	*protocol = "TCP/IP";

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	basex = (320-p->width)/2;
	M_DrawPic (basex, 4, p);

	if (StartingGame)
		startJoin = "New Game";
	else
		startJoin = "Join Game";

	M_Print (basex, 32, va ("%s - %s", startJoin, protocol));
	basex += 8;

	M_Print (basex, 52, "Address:");

	if (ipv4Available && ipv6Available)
	{
		M_Print (basex+9*8, 52-4, my_ipv4_server_address);
		M_Print (basex+9*8, 52+4, my_ipv6_address);
	}
	else
	{
		if (ipv4Available)
			M_Print (basex+9*8, 52, my_ipv4_server_address);
		if (ipv6Available)
			M_Print (basex+9*8, 52, my_ipv6_address);
	}

	M_Print (basex, lanConfig_cursor_table[0], "Port");
	M_DrawTextBox (basex+8*8, lanConfig_cursor_table[0]-8, 6, 1);
	M_Print (basex+9*8, lanConfig_cursor_table[0], lanConfig_portname);

	if (JoiningGame)
	{
		M_Print (basex, lanConfig_cursor_table[1], "Search for local games...");
		M_Print (basex, lanConfig_cursor_table[2], "Search for public games...");
		M_Print (basex, 108, "Join game at:");
		M_DrawTextBox (basex+8, lanConfig_cursor_table[3]-8, 22, 1);
		M_Print (basex+16, lanConfig_cursor_table[3], lanConfig_joinname);
	}
	else
	{
		M_DrawTextBox (basex, lanConfig_cursor_table[1]-8, 2, 1);
		M_Print (basex + 8, lanConfig_cursor_table[1], "OK");
	}

	M_DrawCharacter (basex - 8, lanConfig_cursor_table [lanConfig_cursor], 12+((int)(realtime*4)&1));

	if (lanConfig_cursor == 0)
		M_DrawCharacter (basex + 9 * 8 + 8*strlen(lanConfig_portname), lanConfig_cursor_table [0], 10+((int)(realtime*4)&1));

	if (lanConfig_cursor == 3)
		M_DrawCharacter (basex + 16 + 8*strlen(lanConfig_joinname), lanConfig_cursor_table [3], 10+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (basex, 148, m_return_reason);
}


void M_LanConfig_Key (int key)
{
	int		len, port;

	switch (key)
	{
	case K_ESCAPE:
#if 0 // Farethee well IPX ...
		M_Menu_Net_f ();
#else
		M_Menu_MultiPlayer_f ();
#endif
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		lanConfig_cursor--;
		if (lanConfig_cursor < 0)
			lanConfig_cursor = NUM_LANCONFIG_CMDS - (StartingGame ? 3 : 1);
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		lanConfig_cursor++;
		if (lanConfig_cursor >= NUM_LANCONFIG_CMDS)
			lanConfig_cursor = 0;
		break;

	case K_ENTER:
		if (lanConfig_cursor == 0)
			break;

		m_entersound = true;

		M_ConfigureNetSubsystem ();

		switch (!!StartingGame) {
		case true:
			if (lanConfig_cursor == 1)
				M_Menu_GameOptions_f ();
			break;
		default: // Joining game
			switch (lanConfig_cursor) {
			case 1:  	M_Menu_Search_f(SLIST_LAN); break;
			case 2:  	M_Menu_Search_f(SLIST_INTERNET); break;
			case 3:
						m_return_state = m_state;
						m_return_onerror = true;
						Key_SetDest (key_game);
						Cbuf_AddTextLinef ("connect " QUOTED_S, lanConfig_joinname);
						break;
			} // End cursor switch
		} // End Starting Game vs. Joining game switch

		break;

	case K_BACKSPACE:
		if (lanConfig_cursor == 0)
		{
			if (strlen(lanConfig_portname))
				lanConfig_portname[strlen(lanConfig_portname)-1] = 0;
		}

		if (lanConfig_cursor == 3)
		{
			if (strlen(lanConfig_joinname))
				lanConfig_joinname[strlen(lanConfig_joinname)-1] = 0;
		}
		break;

	default:
		if (out_of_bounds(32, key, 127)) // non-ascii = get out
			break;

		// Allow paste into name
		if (lanConfig_cursor == 3) {
			len = strlen(lanConfig_joinname);

			if (isin2(key, 'v', 'V') && Key_Ctrl_Down()) {
				c_strlcpy (lanConfig_joinname, Clipboard_Get_Text_Line ());
				// This is a way for non-digits to get into the field
			}
			else if (len < (int)sizeof(lanConfig_joinname) - 1 /*room for null*/)
			{
				lanConfig_joinname[len + 0] = key;
				lanConfig_joinname[len + 1] = 0;
			}
		}
		// Allow paste into port
		if (lanConfig_cursor == 0) {
			len = strlen(lanConfig_portname);

			if ( (key == 'v' || key == 'V') && Key_Ctrl_Down()) {
				c_strlcpy (lanConfig_portname, Clipboard_Get_Text_Line ());
			}
			else if (len <  (int)sizeof(lanConfig_portname) - 1 /* room for null*/) {
				if (!isdigit(key))
					break;

				lanConfig_portname[len + 0] = key;
				lanConfig_portname[len + 1] = 0;
			}
		}
	}

	if (StartingGame && lanConfig_cursor == 2)
	{
		if (key == K_UPARROW)
			lanConfig_cursor = 1;
		else
			lanConfig_cursor = 0;
	}

	port =  atoi(lanConfig_portname);
	lanConfig_port = in_range (0, port, 65535) ? port : DEFAULT_QPORT_26000;


	c_snprintf1 (lanConfig_portname, "%u", lanConfig_port);
}

//=============================================================================
/* GAME OPTIONS MENU */


level_t		levels[] =
{
	{"start", "Entrance"},	// 0

	{"e1m1", "Slipgate Complex"},				// 1
	{"e1m2", "Castle of the Damned"},
	{"e1m3", "The Necropolis"},
	{"e1m4", "The Grisly Grotto"},
	{"e1m5", "Gloom Keep"},
	{"e1m6", "The Door To Chthon"},
	{"e1m7", "The House of Chthon"},
	{"e1m8", "Ziggurat Vertigo"},

	{"e2m1", "The Installation"},				// 9
	{"e2m2", "Ogre Citadel"},
	{"e2m3", "Crypt of Decay"},
	{"e2m4", "The Ebon Fortress"},
	{"e2m5", "The Wizard's Manse"},
	{"e2m6", "The Dismal Oubliette"},
	{"e2m7", "Underearth"},

	{"e3m1", "Termination Central"},			// 16
	{"e3m2", "The Vaults of Zin"},
	{"e3m3", "The Tomb of Terror"},
	{"e3m4", "Satan's Dark Delight"},
	{"e3m5", "Wind Tunnels"},
	{"e3m6", "Chambers of Torment"},
	{"e3m7", "The Haunted Halls"},

	{"e4m1", "The Sewage System"},				// 23
	{"e4m2", "The Tower of Despair"},
	{"e4m3", "The Elder God Shrine"},
	{"e4m4", "The Palace of Hate"},
	{"e4m5", "Hell's Atrium"},
	{"e4m6", "The Pain Maze"},
	{"e4m7", "Azure Agony"},
	{"e4m8", "The Nameless City"},

	{"end", "Shub-Niggurath's Pit"},			// 31

	{"dm1", "Place of Two Deaths"},				// 32
	{"dm2", "Claustrophobopolis"},
	{"dm3", "The Abandoned Base"},
	{"dm4", "The Bad Place"},
	{"dm5", "The Cistern"},
	{"dm6", "The Dark Zone"}
};

//MED 01/06/97 added hipnotic levels
level_t     hipnoticlevels[] =
{
   {"start", "Command HQ"},  // 0

   {"hip1m1", "The Pumping Station"},          // 1
   {"hip1m2", "Storage Facility"},
   {"hip1m3", "The Lost Mine"},
   {"hip1m4", "Research Facility"},
   {"hip1m5", "Military Complex"},

   {"hip2m1", "Ancient Realms"},          // 6
   {"hip2m2", "The Black Cathedral"},
   {"hip2m3", "The Catacombs"},
   {"hip2m4", "The Crypt"},
   {"hip2m5", "Mortum's Keep"},
   {"hip2m6", "The Gremlin's Domain"},

   {"hip3m1", "Tur Torment"},       // 12
   {"hip3m2", "Pandemonium"},
   {"hip3m3", "Limbo"},
   {"hip3m4", "The Gauntlet"},

   {"hipend", "Armagon's Lair"},       // 16

   {"hipdm1", "The Edge of Oblivion"}           // 17
};

//PGM 01/07/97 added rogue levels
//PGM 03/02/97 added dmatch level
level_t		roguelevels[] =
{
	{"start",	"Split Decision"},
	{"r1m1",	"Deviant's Domain"},
	{"r1m2",	"Dread Portal"},
	{"r1m3",	"Judgement Call"},
	{"r1m4",	"Cave of Death"},
	{"r1m5",	"Towers of Wrath"},
	{"r1m6",	"Temple of Pain"},
	{"r1m7",	"Tomb of the Overlord"},
	{"r2m1",	"Tempus Fugit"},
	{"r2m2",	"Elemental Fury I"},
	{"r2m3",	"Elemental Fury II"},
	{"r2m4",	"Curse of Osiris"},
	{"r2m5",	"Wizard's Keep"},
	{"r2m6",	"Blood Sacrifice"},
	{"r2m7",	"Last Bastion"},
	{"r2m8",	"Source of Evil"},
	{"ctf1",    "Division of Change"}
};

const int num_quake_original_levels = sizeof(levels)/sizeof(levels[0]);

typedef struct
{
	const char	*description;
	int		firstLevel;
	int		levels;
} episode_t;

episode_t	episodes[] =
{
	{"Welcome to Quake", 0, 1},
	{"Doomed Dimension", 1, 8},
	{"Realm of Black Magic", 9, 7},
	{"Netherworld", 16, 7},
	{"The Elder World", 23, 8},
	{"Final Level", 31, 1},
	{"Deathmatch Arena", 32, 6}
};

//MED 01/06/97  added hipnotic episodes
episode_t   hipnoticepisodes[] =
{
   {"Scourge of Armagon", 0, 1},
   {"Fortress of the Dead", 1, 5},
   {"Dominion of Darkness", 6, 6},
   {"The Rift", 12, 4},
   {"Final Level", 16, 1},
   {"Deathmatch Arena", 17, 1}
};

//PGM 01/07/97 added rogue episodes
//PGM 03/02/97 added dmatch episode
episode_t	rogueepisodes[] =
{
	{"Introduction", 0, 1},
	{"Hell's Fortress", 1, 7},
	{"Corridors of Time", 8, 8},
	{"Deathmatch Arena", 16, 1}
};

int	startepisode;
int	startlevel;
int maxplayers;
//cbool m_serverInfoMessage = false;
//double m_serverInfoMessageTime;

void M_Menu_GameOptions_f (void)
{
	Key_SetDest (key_menu);
	m_state = m_gameoptions;
	m_entersound = true;
	if (maxplayers == 0)
		maxplayers = svs.maxclients_public; // Use the public fake # here because this is the cap  (TODO See what this does?)
	if (maxplayers < 2)
		maxplayers = svs.maxclientslimit;
}


int gameoptions_cursor_table[] = {40, 56, 64, 72, 80, 88, 96, 104, 120, 128};
#define	NUM_GAMEOPTIONS	10
int		gameoptions_cursor;

void M_GameOptions_Draw (void)
{
	qpic_t	*p;
//	int		x;
	int y = 40;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	M_DrawTextBox (152, y-8, 10, 1);
	M_Print (160, y, "begin game");
	y+=16;

	M_Print (0, y, "      Max players");
	M_Print (160, y, va("%d", maxplayers) );
	y+=8;

	M_Print (0, y, "           Public");
//	if (sv_public.value)
//		M_Print (160, y, "Yes");
//	else
		M_Print (160, y, "No");
	y+=8;

	M_Print (0, y, "        Game Type");
	if (pr_coop.value)
		M_Print (160, y, "Cooperative");
	else
		M_Print (160, y, "Deathmatch");
	y+=8;

	M_Print (0, y, "        Teamplay");
	if (com_gametype == gametype_rogue)
	{
		const char *msg;

		switch((int)pr_teamplay.value)
		{
			case 1: msg = "No Friendly Fire"; break;
			case 2: msg = "Friendly Fire"; break;
			case 3: msg = "Tag"; break;
			case 4: msg = "Capture the Flag"; break;
			case 5: msg = "One Flag CTF"; break;
			case 6: msg = "Three Team CTF"; break;
			default: msg = "Off"; break;
		}
		M_Print (160, y, msg);
	}
	else
	{
		const char *msg;

		switch((int)pr_teamplay.value)
		{
			case 1: msg = "No Friendly Fire"; break;
			case 2: msg = "Friendly Fire"; break;
			default: msg = "Off"; break;
		}
		M_Print (160, y, msg);
	}
	y+=8;

	M_Print (0, y, "            Skill");
	if (pr_skill.value == 0)
		M_Print (160, y, "Easy difficulty");
	else if (pr_skill.value == 1)
		M_Print (160, y, "Normal difficulty");
	else if (pr_skill.value == 2)
		M_Print (160, y, "Hard difficulty");
	else
		M_Print (160, y, "Nightmare difficulty");
	y+=8;

	M_Print (0, y, "       Frag Limit");
	if (pr_fraglimit.value == 0)
		M_Print (160, y, "none");
	else
		M_Print (160, y, va("%d frags", (int)pr_fraglimit.value));
	y+=8;

	M_Print (0, y, "       Time Limit");
	if (pr_timelimit.value == 0)
		M_Print (160, y, "none");
	else
		M_Print (160, y, va("%d minutes", (int)pr_timelimit.value));
	y+=8;

	y+=8;

	M_Print (0, y, "         Episode");
   //MED 01/06/97 added hipnotic episodes
   if (com_gametype == gametype_hipnotic)
		M_Print (160, y, hipnoticepisodes[startepisode].description);
   //PGM 01/07/97 added rogue episodes
   else if (com_gametype == gametype_rogue)
		M_Print (160, y, rogueepisodes[startepisode].description);
   else
		M_Print (160, y, episodes[startepisode].description);
	y+=8;

	M_Print (0, y, "           Level");
   //MED 01/06/97 added hipnotic episodes
   if (com_gametype == gametype_hipnotic)
   {
		M_Print (160, y, hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].description);
		M_Print (160, y+8, hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].name);
   }
   //PGM 01/07/97 added rogue episodes
   else if (com_gametype == gametype_rogue)
   {
		M_Print (160, y, roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].description);
		M_Print (160, y+8, roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].name);
   }
   else
   {
		M_Print (160, y, levels[episodes[startepisode].firstLevel + startlevel].description);
		M_Print (160, y+8, levels[episodes[startepisode].firstLevel + startlevel].name);
   }
	y+=8;

// line cursor
	M_DrawCharacter (144, gameoptions_cursor_table[gameoptions_cursor], 12+((int)(realtime*4)&1));
#if 0 // Baker: Yeah, right
	if (m_serverInfoMessage)
	{
		if ((realtime - m_serverInfoMessageTime) < 5.0)
		{
			x = (320-26*8)/2;
			M_DrawTextBox (x, 138, 24, 4);
			x += 8;
			M_Print (x, 146, va("  More than %d players   ", svs.maxclientslimit));
			M_Print (x, 154, " requires using command ");
			M_Print (x, 162, "line parameters; please ");
			M_Print (x, 170, "   see techinfo.txt.    ");
		}
		else
		{
			m_serverInfoMessage = false;
		}
	}
#endif
}


void M_NetStart_Change (int dir)
{
	int count;
	float	f;

	switch (gameoptions_cursor)
	{
	case 1:
		maxplayers += dir;
		if (maxplayers > svs.maxclientslimit)
		{
			maxplayers = svs.maxclientslimit;
//			m_serverInfoMessage = true;
//			m_serverInfoMessageTime = realtime;
		}
		if (maxplayers < 2)
			maxplayers = 2;
		break;

	case 2:
//		Cvar_SetValueQuick (&sv_public, sv_public.value ? 0 : 1);
		break;

	case 3:
		Cvar_SetValueQuick (&pr_coop, pr_coop.value ? 0 : 1);
		break;

	case 4:
		count = (com_gametype == gametype_rogue) ? 6 : 2;
		f = pr_teamplay.value + dir;
		if (f > count)	f = 0;
		else if (f < 0)	f = count;
		Cvar_SetValueQuick (&pr_teamplay, f);
		break;

	case 5:
		f = pr_skill.value + dir;
		if (f > 3)	f = 0;
		else if (f < 0)	f = 3;
		Cvar_SetValueQuick (&pr_skill, f);
		break;

	case 6:
		f = pr_fraglimit.value + dir * 10;
		if (f > 100)	f = 0;
		else if (f < 0)	f = 100;
		Cvar_SetValueQuick (&pr_fraglimit, f);
		break;

	case 7:
		f = pr_timelimit.value + dir * 5;
		if (f > 60)	f = 0;
		else if (f < 0)	f = 60;
		Cvar_SetValueQuick (&pr_timelimit, f);
		break;

	case 8:
		startepisode += dir;
	//MED 01/06/97 added hipnotic count
		if (com_gametype == gametype_hipnotic)
			count = 6;
	//PGM 01/07/97 added rogue count
	//PGM 03/02/97 added 1 for dmatch episode
		else if (com_gametype == gametype_rogue)
			count = 4;
		else if (registered.value)
			count = 7;
		else
			count = 2;

		if (startepisode < 0)
			startepisode = count - 1;

		if (startepisode >= count)
			startepisode = 0;

		startlevel = 0;
		break;

	case 9:
		startlevel += dir;
    //MED 01/06/97 added hipnotic episodes
		if (com_gametype == gametype_hipnotic)
			count = hipnoticepisodes[startepisode].levels;
	//PGM 01/06/97 added hipnotic episodes
		else if (com_gametype == gametype_rogue)
			count = rogueepisodes[startepisode].levels;
		else
			count = episodes[startepisode].levels;

		if (startlevel < 0)
			startlevel = count - 1;

		if (startlevel >= count)
			startlevel = 0;
		break;
	}
}

void M_GameOptions_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
#if 0 // Farethee well IPX ...
		M_Menu_Net_f ();
#else
		M_Menu_MultiPlayer_f ();
#endif
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		gameoptions_cursor--;
		if (gameoptions_cursor < 0)
			gameoptions_cursor = NUM_GAMEOPTIONS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		gameoptions_cursor++;
		if (gameoptions_cursor >= NUM_GAMEOPTIONS)
			gameoptions_cursor = 0;
		break;

	case K_LEFTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("misc/menu3.wav");
		M_NetStart_Change (-1);
		break;

	case K_RIGHTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("misc/menu3.wav");
		M_NetStart_Change (1);
		break;

	case K_ENTER:
		S_LocalSound ("misc/menu2.wav");
		if (gameoptions_cursor == 0)
		{
			if (sv.active)
				Cbuf_AddTextLine ("disconnect");
			Cbuf_AddTextLine  ("listen 0");	// so host_netport will be re-examined
			Cbuf_AddTextLinef ("maxplayers %d", maxplayers);
			SCR_BeginLoadingPlaque ();

			if (com_gametype == gametype_hipnotic)
				Cbuf_AddTextLinef ("map %s", hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].name );
			else if (com_gametype == gametype_rogue)
				Cbuf_AddTextLinef ("map %s", roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].name );
			else
				Cbuf_AddTextLinef ("map %s", levels[episodes[startepisode].firstLevel + startlevel].name );

			return;
		}

		M_NetStart_Change (1);
		break;
	}
}

//=============================================================================
/* SEARCH MENU */

cbool	searchComplete = false;
double		searchCompleteTime;
enum slistScope_e searchLastScope = SLIST_LAN;

void M_Menu_Search_f (enum slistScope_e scope)
{
	Key_SetDest (key_menu);
	m_state = m_search;
	m_entersound = false;
	slistSilent = true;
	slistScope = searchLastScope = scope;
	searchComplete = false;
	NET_Slist_f (NULL);

}


void M_Search_Draw (void)
{
	qpic_t	*p;
	int x;

	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	x = (320/2) - ((12*8)/2) + 4;
	M_DrawTextBox (x-8, 32, 12, 1);
	M_Print (x, 40, "Searching...");

	if(slistInProgress)
	{
		NET_Poll();
		return;
	}

	if (! searchComplete)
	{
		searchComplete = true;
		searchCompleteTime = realtime;
	}

	if (hostCacheCount)
	{
		M_Menu_ServerList_f ();
		return;
	}

	M_PrintWhite ((320/2) - ((22*8)/2), 64, "No Quake servers found");
	if ((realtime - searchCompleteTime) < 3.0)
		return;

	M_Menu_LanConfig_f ();
}


void M_Search_Key (int key)
{
}

//=============================================================================
/* SLIST MENU */

int		slist_cursor;
int		slist_first;
cbool slist_sorted;

void M_Menu_ServerList_f (void)
{
	Key_SetDest (key_menu);
	m_state = m_slist;
	m_entersound = true;
	slist_cursor = 0;
	slist_first = 0;
	m_return_onerror = false;
	m_return_reason[0] = 0;
	slist_sorted = false;
}


void M_ServerList_Draw (void)
{
	int		n, slist_shown;
	qpic_t	*p;

	if (!slist_sorted)
	{
		slist_sorted = true;
		NET_SlistSort ();
	}

	slist_shown = hostCacheCount;
	if (slist_shown > (200-32)/8) // what are these magic number 200 and 32?
		slist_shown = (200-32)/8;
	if (slist_first+slist_shown-1 < slist_cursor)
		slist_first = slist_cursor-(slist_shown-1);
	if (slist_first > slist_cursor)
		slist_first = slist_cursor;

	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	for (n = 0; n < slist_shown; n++)
		M_Print (16, 32 + 8*n, NET_SlistPrintServer (slist_first + n));
	M_DrawCharacter (0, 32 + (slist_cursor - slist_first)*8, 12+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (16, 148, m_return_reason);
}


void M_ServerList_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_LanConfig_f ();
		break;

	case K_SPACE:
		M_Menu_Search_f (searchLastScope);
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		slist_cursor--;
		if (slist_cursor < 0)
			slist_cursor = hostCacheCount - 1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		slist_cursor++;
		if (slist_cursor >= hostCacheCount)
			slist_cursor = 0;
		break;

	case K_ENTER:
		S_LocalSound ("misc/menu2.wav");
		m_return_state = m_state;
		m_return_onerror = true;
		slist_sorted = false;
		Key_SetDest (key_game);
		Cbuf_AddTextLinef ("connect " QUOTED_S, NET_SlistPrintServerName(slist_cursor) );
		break;

	default:
		break;
	}

}

//=============================================================================
/* Menu Subsystem */


void M_Init (void)
{
	Cmd_AddCommands (M_Init);
}


void M_Draw (void)
{
	if (m_state == m_none || key_dest != key_menu)
		return;

	if (!m_recursiveDraw)
	{
		if (console1.visible_pct)
		{
			// Baker: We need this because console background can draw
			// At stupid times without it, at least for software Quake
			if (key_dest == key_console || key_dest == key_message)
				Draw_ConsoleBackground ();
			S_ExtraUpdate ();
		}

		Draw_FadeScreen (); //johnfitz -- fade even if console fills screen

#ifdef WINQUAKE_RENDERER_SUPPORT
// Baker: I suspect this isn't necessary, but haven't verified.
// because I think something above might set both or some other condition.
		winquake_scr_copyeverything = 1;
		winquake_scr_fullupdate = 0;
#endif // WINQUAKE_RENDERER_SUPPORT
	}
	else
	{
		m_recursiveDraw = false;
	}

	Draw_SetCanvas (CANVAS_MENU); //johnfitz

	switch (m_state)
	{
	case m_none:
		break;

	case m_main:
		M_Main_Draw ();
		break;

	case m_singleplayer:
		M_SinglePlayer_Draw ();
		break;

	case m_load:
		M_Load_Draw ();
		break;

	case m_save:
		M_Save_Draw ();
		break;

	case m_levels:
		M_Levels_Draw ();
		break;

	case m_multiplayer:
		M_MultiPlayer_Draw ();
		break;

	case m_demos:
		M_Demos_Draw ();
		break;

	case m_setup:
		M_Setup_Draw ();
		break;

	case m_namemaker:
		M_NameMaker_Draw ();
		break;

#if 0 // Farethee well IPX ...
	case m_net:
		M_Net_Draw ();
		break;
#endif

	case m_options:
		M_Options_Draw ();
		break;

	case m_keys:
		M_Keys_Draw ();
		break;

	case m_video:
		M_Video_Draw ();
		break;

	case m_preferences:
		M_Preferences_Draw ();
		break;

	case m_help:
		M_Help_Draw ();
		break;

	case m_quit:
		M_Quit_Draw ();
		break;

	case m_lanconfig:
		M_LanConfig_Draw ();
		break;

	case m_gameoptions:
		M_GameOptions_Draw ();
		break;

	case m_search:
		M_Search_Draw ();
		break;

	case m_slist:
		M_ServerList_Draw ();
		break;
	}

	if (m_entersound)
	{
		S_LocalSound ("misc/menu2.wav");
		m_entersound = false;
	}

	S_ExtraUpdate ();
}


void M_Keydown (int key)
{
	//Con_Queue_PrintLinef ("Menu_Keydown: (rnd %3d) code = %d rep: '%c'", (int)(RANDOM_FLOAT * 100), key, key);
	switch (m_state)
	{
	case m_none:
		return;

	case m_main:
		M_Main_Key (key);
		return;

	case m_singleplayer:
		M_SinglePlayer_Key (key);
		return;

	case m_load:
		M_Load_Key (key);
		return;

	case m_save:
		M_Save_Key (key);
		return;

	case m_levels:
		M_Levels_Key (key);
		return;

	case m_multiplayer:
		M_MultiPlayer_Key (key);
		return;

	case m_demos:
		M_Demos_Key (key);
		break;

	case m_setup:
		M_Setup_Key (key);
		return;

	case m_namemaker:
		M_NameMaker_Key (key);
		return;

#if 0 // Farethee well IPX ...
	case m_net:
		M_Net_Key (key);
		return;
#endif

	case m_options:
		M_Options_Key (key);
		return;

	case m_keys:
		M_Keys_Key (key);
		return;

	case m_video:
		M_Video_Key (key);
		return;

	case m_preferences:
		M_Preferences_Key (key);
		return;

	case m_help:
		M_Help_Key (key);
		return;

	case m_quit:
		M_Quit_Key (key);
		return;

	case m_lanconfig:
		M_LanConfig_Key (key);
		return;

	case m_gameoptions:
		M_GameOptions_Key (key);
		return;

	case m_search:
		M_Search_Key (key);
		break;

	case m_slist:
		M_ServerList_Key (key);
		return;
	}
}


void M_ConfigureNetSubsystem(void)
{
// enable/disable net systems to match desired config
	Cbuf_AddTextLine ("stopdemo"); // Baker:  Ah, cute.

	//if (/*IPXConfig ||*/ TCPIPConfig)
	net_hostport = lanConfig_port;
}

//==========================================================================
//
//  NEW VIDEO MENU -- johnfitz
//
//==========================================================================

#ifdef WINQUAKE_RENDERER_SUPPORT
	int	video_cursor_table[] = {48, 56, 72, 88, 96};	// mode, fullscreen, stretch, test, apply)
#else
	int	video_cursor_table[] = {48, 56, 72, 80, 112, 120};	// mode, fullscreen, test, apply, gamma, pixels
#endif // !WINQUAKE_RENDERER_SUPPORT

enum {
	VID_OPT_MODE_0,
	VID_OPT_FULLSCREEN_1,

#ifdef WINQUAKE_RENDERER_SUPPORT
	VID_OPT_STRETCH,
#endif // WINQUAKE_RENDERER_SUPPORT

	VID_OPT_TEST,
	VID_OPT_APPLY,

#ifdef GLQUAKE_RENDERER_SUPPORT
	VID_OPT_HWGAMMA,
	VID_OPT_TEXTUREFILTER,
#endif // WINQUAKE_RENDERER_SUPPORT

	VIDEO_OPTIONS_ITEMS
};


int		video_options_cursor = 0;

typedef struct {int width,height;} vid_menu_mode;

int vid_menu_rwidth;
int vid_menu_rheight;

//TODO: replace these fixed-length arrays with hunk_allocated buffers

vid_menu_mode vid_menu_modes[MAX_MODE_LIST];
int vid_menu_nummodes=0;


/*
================
VID_Menu_Init
================
*/
void VID_Menu_Init (void)
{
	int i,j,h,w;

//	vid_menucmdfn = VID_Menu_f; //johnfitz
//	vid_menudrawfn = VID_MenuDraw;
//	vid_menukeyfn = VID_MenuKey;

	for (i = 1; i < vid.nummodes; i ++) //start i at mode 1 because 0 is windowed mode
	{
		w = vid.modelist[i].width;
		h = vid.modelist[i].height;

		for (j = 0; j < vid_menu_nummodes; j++)
		{
			if (vid_menu_modes[j].width == w &&
				vid_menu_modes[j].height == h)
				break;
		}

		if (j == vid_menu_nummodes)
		{
			vid_menu_modes[j].width = w;
			vid_menu_modes[j].height = h;
			vid_menu_nummodes++;
		}
	}
}

/*
================
VID_Menu_CalcAspectRatio

calculates aspect ratio for current vid_width/vid_height
================
*/
void VID_Menu_CalcAspectRatio (void)
{
	int w,h,f;
	w = vid_width.value;
	h = vid_height.value;
	f = 2;
	while (f < w && f < h)
	{
		if ((w/f)*f == w && (h/f)*f == h)
		{
			w/=f;
			h/=f;
			f=2;
		}
		else
			f++;
	}
	vid_menu_rwidth = w;
	vid_menu_rheight = h;
}

/*
================
VID_Menu_ChooseNextMode

chooses next resolution in order, then updates vid_width and
vid_height cvars, then updates bpp and refreshrate lists
================
*/
void VID_Menu_ChooseNextMode (int dir)
{
	int i;

	for (i = 0; i < vid_menu_nummodes; i++)
	{
		if (vid_menu_modes[i].width == vid_width.value &&
			vid_menu_modes[i].height == vid_height.value)
			break;
	}

	if (i == vid_menu_nummodes) //can't find it in list, so it must be a custom windowed res
	{
		i = 0;
	}
	else
	{
		i+=dir;
		if (i>=vid_menu_nummodes)
			i = 0;
		else if (i < 0)
			i = vid_menu_nummodes - 1;
	}

	Cvar_SetValueQuick (&vid_width, (float)vid_menu_modes[i].width);
	Cvar_SetValueQuick (&vid_height, (float)vid_menu_modes[i].height);

	VID_Menu_CalcAspectRatio ();
}


/*
================
VID_MenuKey
================
*/

extern int glmode_idx;
void M_Video_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		VID_Cvars_Sync_To_Mode (&vid.modelist[vid.modenum_screen]); //sync cvars before leaving menu. FIXME: there are other ways to leave menu
		S_LocalSound ("misc/menu1.wav");
		M_Menu_Options_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		video_options_cursor --;
		if (video_options_cursor < 0)
			video_options_cursor = VIDEO_OPTIONS_ITEMS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		video_options_cursor++;
		if (video_options_cursor >= VIDEO_OPTIONS_ITEMS)
			video_options_cursor = 0;
		break;

	case K_LEFTARROW:
		S_LocalSound ("misc/menu3.wav");
		switch (video_options_cursor)
		{
		case VID_OPT_MODE_0:
			VID_Menu_ChooseNextMode (-1);
			break;
		case VID_OPT_FULLSCREEN_1:
			Cbuf_AddTextLine ("toggle vid_fullscreen");
			break;
#ifdef WINQUAKE_RENDERER_SUPPORT
		case VID_OPT_STRETCH: {
			int newval = vid_sw_stretch.value <=0 ? 2 : CLAMP(0, (int)vid_sw_stretch.value - 1, 2);
			Cvar_SetValueQuick (&vid_sw_stretch, newval);
			break;
		}
#else
		case VID_OPT_HWGAMMA:
			Cbuf_AddTextLine ("toggle vid_hardwaregamma");
			break;
//#define TEXMODE_GL_NEAREST_0							0
//#define TEXMODE_GL_NEAREST_MIPMAP_NEAREST_1			1
//#define TEXMODE_GL_NEAREST_MIPMAP_LINEAR_2			2
//#define TEXMODE_GL_LINEAR_MIPMAP_LINEAR_5			5
		case VID_OPT_TEXTUREFILTER: // 0 (default "5"), 1 - pixelated (default), 2 -
			switch (glmode_idx) { // Left 0, 5, 1
			default:
			case TEXMODE_GL_LINEAR_MIPMAP_LINEAR_5:		Cvar_SetQuick (&gl_texturemode, "GL_NEAREST");					break;
			case TEXMODE_GL_NEAREST_MIPMAP_NEAREST_1:	Cvar_SetQuick (&gl_texturemode, "GL_LINEAR_MIPMAP_LINEAR");		break;
			case TEXMODE_GL_NEAREST_0:					Cvar_SetQuick (&gl_texturemode, "GL_NEAREST_MIPMAP_NEAREST");   break;
			}
			break;

#endif //WINQUAKE_RENDERER_SUPPORT



		default:
			break;
		}
		break;

	case K_RIGHTARROW:
		S_LocalSound ("misc/menu3.wav");
		switch (video_options_cursor)
		{
		case VID_OPT_MODE_0:
			VID_Menu_ChooseNextMode (1);
			break;
		case VID_OPT_FULLSCREEN_1:
			Cbuf_AddTextLine ("toggle vid_fullscreen");
			break;
#ifdef WINQUAKE_RENDERER_SUPPORT
		case VID_OPT_STRETCH: {
			int newval = vid_sw_stretch.value >=2 ? 0 : CLAMP(0, (int)vid_sw_stretch.value + 1, 2);
			Cvar_SetValueQuick (&vid_sw_stretch, newval);
			break;
		}
#else

		case VID_OPT_HWGAMMA:
			Cbuf_AddTextLine ("toggle vid_hardwaregamma");
			break;

		case VID_OPT_TEXTUREFILTER:
			switch (glmode_idx) { // Left 0, 5, 1
			default:
			case TEXMODE_GL_LINEAR_MIPMAP_LINEAR_5:		Cvar_SetQuick (&gl_texturemode, "GL_NEAREST_MIPMAP_NEAREST");					break;
			case TEXMODE_GL_NEAREST_MIPMAP_NEAREST_1:	Cvar_SetQuick (&gl_texturemode, "GL_NEAREST");		break;
			case TEXMODE_GL_NEAREST_0:					Cvar_SetQuick (&gl_texturemode, "GL_LINEAR_MIPMAP_LINEAR");   break;
			}
			break;

#endif //WINQUAKE_RENDERER_SUPPORT

		default:
			break;
		}
		break;

	case K_ENTER:
		m_entersound = true;
		switch (video_options_cursor)
		{
		case VID_OPT_MODE_0:
			VID_Menu_ChooseNextMode (1);
			break;
		case VID_OPT_FULLSCREEN_1:
			Cbuf_AddTextLine ("toggle vid_fullscreen");
			break;
#ifdef WINQUAKE_RENDERER_SUPPORT
		case VID_OPT_STRETCH:
			break;
#else
		case VID_OPT_HWGAMMA:
			break;

		case VID_OPT_TEXTUREFILTER:
			break;

#endif //WINQUAKE_RENDERER_SUPPORT
		case VID_OPT_TEST:
			Cbuf_AddTextLine ("vid_test");
			break;
		case VID_OPT_APPLY:
			Cbuf_AddTextLine ("vid_restart");
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}
}

/*
================
VID_MenuDraw
================
*/
void M_Video_Draw (void)
{
	int i = 0;
	qpic_t *p;
	char *title;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp"));

	//p = Draw_CachePic ("gfx/vidmodes.lmp");
	p = Draw_CachePic ("gfx/p_option.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	// title
	title = "Video Options";
	M_PrintWhite ((320-8*strlen(title))/2, 32, title);

	// options
	M_Print (16, video_cursor_table[i], "        Video mode");
	M_Print (184, video_cursor_table[i], va("%dx%d (%d:%d)", (int)vid_width.value, (int)vid_height.value, vid_menu_rwidth, vid_menu_rheight));
	i++;

	M_Print (16, video_cursor_table[i], "        Fullscreen");
	M_DrawCheckbox (184, video_cursor_table[i], (int)vid_fullscreen.value);
	i++;

#ifdef WINQUAKE_RENDERER_SUPPORT
	M_Print (16, video_cursor_table[i], "     Integer scale");
	#ifdef CORE_GL
		// WinQuake GL -- keep users out of trouble
		M_Print (184, video_cursor_table[i], (vid_sw_stretch.value >= 2) ? "320x240 nearest" : (vid_sw_stretch.value >= 1) ? "640x480 nearest" : "Auto");
	#else
		M_Print (184, video_cursor_table[i], (vid_sw_stretch.value >= 2) ? "320x240 nearest" : (vid_sw_stretch.value >= 1) ? "640x480 nearest" : "Auto");
	#endif
	i++;
#endif // !WINQUAKE_RENDERER_SUPPORT

	M_Print (16, video_cursor_table[i], "      Test changes");
	i++;

	M_Print (16, video_cursor_table[i], "     Apply changes");
	i++;

#ifdef GLQUAKE_RENDERER_SUPPORT
	M_Print (16, video_cursor_table[i], "        Brightness");
	M_Print (184, video_cursor_table[i], (vid_hardwaregamma.value) ? "Hardware gamma" : vid.direct3d == 9 ? "Shader gamma" : "Texture gamma");
	i++;

	M_Print (16, video_cursor_table[i], "        Pixelation");
	M_Print (184, video_cursor_table[i],	glmode_idx == TEXMODE_GL_LINEAR_MIPMAP_LINEAR_5 ? "Smooth (Default)" :
						glmode_idx == TEXMODE_GL_NEAREST_MIPMAP_NEAREST_1 ? "Pixelated" : "Pixelated/Rough" /* TEXMODE_GL_NEAREST_0*/
		);


	if (video_options_cursor == VID_OPT_TEXTUREFILTER) {
		M_PrintWhite (16, video_cursor_table[i] + 24,
			glmode_idx == TEXMODE_GL_LINEAR_MIPMAP_LINEAR_5 ?   "     Filter: GL_LINEAR_MIPMAP_LINEAR" :
			glmode_idx == TEXMODE_GL_NEAREST_MIPMAP_NEAREST_1 ? "     Filter: GL_NEAREST_MIPMAP_NEAREST" :
			glmode_idx == TEXMODE_GL_NEAREST_0				  ? "     Filter: GL_NEAREST" :
																"     Filter: (other)"
		);
	}

	i++;

#endif // !WINQUAKE_RENDERER_SUPPORT


	// cursor
	M_DrawCharacter (168, video_cursor_table[video_options_cursor], 12 + ((int)(realtime*4) & 1));

	// notes          "345678901234567890123456789012345678"
//	M_Print (16, 172, "Windowed modes always use the desk- ");
//	M_Print (16, 180, "top color depth, and can never be   ");
//	M_Print (16, 188, "larger than the desktop resolution. ");
}

/*
================
VID_Menu_f
================
*/
void M_Menu_Video_f (void)
{
	Key_SetDest (key_menu); m_state = m_video;
	m_entersound = true;

	//set all the cvars to match the current mode when entering the menu
	VID_Cvars_Sync_To_Mode (&vid.modelist[vid.modenum_screen]);

	//aspect ratio
	VID_Menu_CalcAspectRatio ();
}
