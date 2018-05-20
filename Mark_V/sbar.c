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
// sbar.c -- status bar code

#pragma message ("For game -hires why does sbaralpha 0 still have backtile?")
#pragma message ("The 2D HUD *REALLY* needs some work, looks like shit with 'sbar 1'")

#include "quakedef.h"


int sb_updates;		// if >= vid.numpages, no update needed
float sbar_alpha;
cbool sbar_drawn;

#define STAT_MINUS		10	// num frame for '-' stats digit

qpic_t	*sb_nums[2][11];
qpic_t	*sb_colon, *sb_slash;
qpic_t	*sb_ibar;
qpic_t	*sb_sbar;
qpic_t	*sb_scorebar;

qpic_t	*sb_weapons[7][8];   // 0 is active, 1 is owned, 2-5 are flashes
qpic_t	*sb_ammo[4];
qpic_t	*sb_sigil[4];
qpic_t	*sb_armor[3];
qpic_t	*sb_items[32];

qpic_t	*sb_faces[7][2];		// 0 is gibbed, 1 is dead, 2-6 are alive
							// 0 is static, 1 is temporary animation
qpic_t	*sb_face_invis;
qpic_t	*sb_face_quad;
qpic_t	*sb_face_invuln;
qpic_t	*sb_face_invis_invuln;

cbool sb_showscores;
int sb_lines; // scan lines to draw

qpic_t	*rsb_invbar[2];
qpic_t	*rsb_weapons[5];
qpic_t	*rsb_items[2];
qpic_t	*rsb_ammo[3];
qpic_t	*rsb_teambord;		// PGM 01/19/97 - team color border

//MED 01/04/97 added two more weapons + 3 alternates for grenade launcher
qpic_t	*hsb_weapons[7][5];   // 0 is active, 1 is owned, 2-5 are flashes
//MED 01/04/97 added array to simplify weapon parsing
int hipweapons[4] = {HIT_LASER_CANNON_BIT,HIT_MJOLNIR_BIT,4,HIT_PROXIMITY_GUN_BIT};
//MED 01/04/97 added hipnotic items array
qpic_t	*hsb_items[2];

void Sbar_MiniDeathmatchOverlay (void);
void Sbar_DeathmatchOverlay (void);


/*
===============
Sbar_ShowScores

Tab key down
===============
*/
void Sbar_ShowScores (void)
{
	if (sb_showscores)
		return;
	sb_showscores = true;
	sb_updates = 0;
}

/*
===============
Sbar_DontShowScores

Tab key up
===============
*/
void Sbar_DontShowScores (void)
{
	sb_showscores = false;
	sb_updates = 0;
}

/*
===============
Sbar_Changed
===============
*/
void Sbar_Changed (void)
{
	sb_updates = 0;	// update next frame
}

/*
===============
Sbar_LoadPics -- johnfitz -- load all the sbar pics
===============
*/
void Sbar_LoadPics (void)
{
	int		i;

	for (i=0 ; i<10 ; i++)
	{
		sb_nums[0][i] = Draw_PicFromWad (va("num_%d",i));
		sb_nums[1][i] = Draw_PicFromWad (va("anum_%d",i));
	}

	sb_nums[0][10] = Draw_PicFromWad ("num_minus");
	sb_nums[1][10] = Draw_PicFromWad ("anum_minus");

	sb_colon = Draw_PicFromWad ("num_colon");
	sb_slash = Draw_PicFromWad ("num_slash");

	sb_weapons[0][0] = Draw_PicFromWad ("inv_shotgun");
	sb_weapons[0][1] = Draw_PicFromWad ("inv_sshotgun");
	sb_weapons[0][2] = Draw_PicFromWad ("inv_nailgun");
	sb_weapons[0][3] = Draw_PicFromWad ("inv_snailgun");
	sb_weapons[0][4] = Draw_PicFromWad ("inv_rlaunch");
	sb_weapons[0][5] = Draw_PicFromWad ("inv_srlaunch");
	sb_weapons[0][6] = Draw_PicFromWad ("inv_lightng");

	sb_weapons[1][0] = Draw_PicFromWad ("inv2_shotgun");
	sb_weapons[1][1] = Draw_PicFromWad ("inv2_sshotgun");
	sb_weapons[1][2] = Draw_PicFromWad ("inv2_nailgun");
	sb_weapons[1][3] = Draw_PicFromWad ("inv2_snailgun");
	sb_weapons[1][4] = Draw_PicFromWad ("inv2_rlaunch");
	sb_weapons[1][5] = Draw_PicFromWad ("inv2_srlaunch");
	sb_weapons[1][6] = Draw_PicFromWad ("inv2_lightng");

	for (i=0 ; i<5 ; i++)
	{
		sb_weapons[2+i][0] = Draw_PicFromWad (va("inva%d_shotgun",i+1));
		sb_weapons[2+i][1] = Draw_PicFromWad (va("inva%d_sshotgun",i+1));
		sb_weapons[2+i][2] = Draw_PicFromWad (va("inva%d_nailgun",i+1));
		sb_weapons[2+i][3] = Draw_PicFromWad (va("inva%d_snailgun",i+1));
		sb_weapons[2+i][4] = Draw_PicFromWad (va("inva%d_rlaunch",i+1));
		sb_weapons[2+i][5] = Draw_PicFromWad (va("inva%d_srlaunch",i+1));
		sb_weapons[2+i][6] = Draw_PicFromWad (va("inva%d_lightng",i+1));
	}

	sb_ammo[0] = Draw_PicFromWad ("sb_shells");
	sb_ammo[1] = Draw_PicFromWad ("sb_nails");
	sb_ammo[2] = Draw_PicFromWad ("sb_rocket");
	sb_ammo[3] = Draw_PicFromWad ("sb_cells");

	sb_armor[0] = Draw_PicFromWad ("sb_armor1");
	sb_armor[1] = Draw_PicFromWad ("sb_armor2");
	sb_armor[2] = Draw_PicFromWad ("sb_armor3");

	sb_items[0] = Draw_PicFromWad ("sb_key1");
	sb_items[1] = Draw_PicFromWad ("sb_key2");
	sb_items[2] = Draw_PicFromWad ("sb_invis");
	sb_items[3] = Draw_PicFromWad ("sb_invuln");
	sb_items[4] = Draw_PicFromWad ("sb_suit");
	sb_items[5] = Draw_PicFromWad ("sb_quad");

	sb_sigil[0] = Draw_PicFromWad ("sb_sigil1");
	sb_sigil[1] = Draw_PicFromWad ("sb_sigil2");
	sb_sigil[2] = Draw_PicFromWad ("sb_sigil3");
	sb_sigil[3] = Draw_PicFromWad ("sb_sigil4");

	sb_faces[4][0] = Draw_PicFromWad ("face1");
	sb_faces[4][1] = Draw_PicFromWad ("face_p1");
	sb_faces[3][0] = Draw_PicFromWad ("face2");
	sb_faces[3][1] = Draw_PicFromWad ("face_p2");
	sb_faces[2][0] = Draw_PicFromWad ("face3");
	sb_faces[2][1] = Draw_PicFromWad ("face_p3");
	sb_faces[1][0] = Draw_PicFromWad ("face4");
	sb_faces[1][1] = Draw_PicFromWad ("face_p4");
	sb_faces[0][0] = Draw_PicFromWad ("face5");
	sb_faces[0][1] = Draw_PicFromWad ("face_p5");

	sb_face_invis = Draw_PicFromWad ("face_invis");
	sb_face_invuln = Draw_PicFromWad ("face_invul2");
	sb_face_invis_invuln = Draw_PicFromWad ("face_inv2");
	sb_face_quad = Draw_PicFromWad ("face_quad");

	sb_sbar = Draw_PicFromWad ("sbar");
	sb_ibar = Draw_PicFromWad ("ibar");
	sb_scorebar = Draw_PicFromWad ("scorebar");

//MED 01/04/97 added new hipnotic weapons
	if (com_gametype == gametype_hipnotic || com_gametype == gametype_quoth)
	{
	  hsb_weapons[0][0] = Draw_PicFromWad ("inv_laser");
	  hsb_weapons[0][1] = Draw_PicFromWad ("inv_mjolnir");
	  hsb_weapons[0][2] = Draw_PicFromWad ("inv_gren_prox");
	  hsb_weapons[0][3] = Draw_PicFromWad ("inv_prox_gren");
	  hsb_weapons[0][4] = Draw_PicFromWad ("inv_prox");

	  hsb_weapons[1][0] = Draw_PicFromWad ("inv2_laser");
	  hsb_weapons[1][1] = Draw_PicFromWad ("inv2_mjolnir");
	  hsb_weapons[1][2] = Draw_PicFromWad ("inv2_gren_prox");
	  hsb_weapons[1][3] = Draw_PicFromWad ("inv2_prox_gren");
	  hsb_weapons[1][4] = Draw_PicFromWad ("inv2_prox");

	  for (i=0 ; i<5 ; i++)
	  {
		 hsb_weapons[2+i][0] = Draw_PicFromWad (va("inva%d_laser",i+1));
		 hsb_weapons[2+i][1] = Draw_PicFromWad (va("inva%d_mjolnir",i+1));
		 hsb_weapons[2+i][2] = Draw_PicFromWad (va("inva%d_gren_prox",i+1));
		 hsb_weapons[2+i][3] = Draw_PicFromWad (va("inva%d_prox_gren",i+1));
		 hsb_weapons[2+i][4] = Draw_PicFromWad (va("inva%d_prox",i+1));
	  }

	  hsb_items[0] = Draw_PicFromWad ("sb_wsuit");
	  hsb_items[1] = Draw_PicFromWad ("sb_eshld");
	}

	if (com_gametype == gametype_rogue)
	{
		rsb_invbar[0] = Draw_PicFromWad ("r_invbar1");
		rsb_invbar[1] = Draw_PicFromWad ("r_invbar2");

		rsb_weapons[0] = Draw_PicFromWad ("r_lava");
		rsb_weapons[1] = Draw_PicFromWad ("r_superlava");
		rsb_weapons[2] = Draw_PicFromWad ("r_gren");
		rsb_weapons[3] = Draw_PicFromWad ("r_multirock");
		rsb_weapons[4] = Draw_PicFromWad ("r_plasma");

		rsb_items[0] = Draw_PicFromWad ("r_shield1");
        rsb_items[1] = Draw_PicFromWad ("r_agrav1");

// PGM 01/19/97 - team color border
        rsb_teambord = Draw_PicFromWad ("r_teambord");
// PGM 01/19/97 - team color border

		rsb_ammo[0] = Draw_PicFromWad ("r_ammolava");
		rsb_ammo[1] = Draw_PicFromWad ("r_ammomulti");
		rsb_ammo[2] = Draw_PicFromWad ("r_ammoplasma");
	}
}

/*
===============
Sbar_Init -- johnfitz -- rewritten
===============
*/
void Sbar_Init (void)
{
	Cmd_AddCommands (Sbar_Init);

	Sbar_LoadPics ();
}


//=============================================================================

// drawing routines are relative to the status bar location

/*
=============
Sbar_DrawPic -- johnfitz -- rewritten now that Draw_SetCanvas is doing the work
=============
*/
void Sbar_DrawPic (int x, int y, qpic_t *pic)
{
	Draw_Pic (x, y + 24, pic);
}

#ifdef GLQUAKE_ALPHA_DRAWING
/*
=============
Sbar_DrawPicAlpha -- johnfitz
=============
*/
void Sbar_DrawPicAlpha (int x, int y, qpic_t *pic, float alpha)
{
	eglDisable (GL_ALPHA_TEST);
	eglEnable (GL_BLEND);
	eglColor4f(1,1,1,alpha);
	Draw_Pic (x, y + 24, pic);
	eglColor3f(1,1,1);
	eglDisable (GL_BLEND);
	eglEnable (GL_ALPHA_TEST);
}

#define Sbar_DrawTransPic Sbar_DrawPic

#else // WinQuake ...
/*
=============
Sbar_DrawTransPic
=============
*/
void Sbar_DrawTransPic (int x, int y, qpic_t *pic)
{
	Draw_TransPic (x, y + 24, pic);
}

/*
=============
Sbar_DrawPicAlpha -- johnfitz
=============
*/
void Sbar_DrawPicAlpha (int x, int y, qpic_t *pic, float alpha)
{
	if (alpha < 1)
		Draw_TransPic (x, y + 24, pic);
	else
		Draw_Pic (x, y + 24, pic);
}

#endif // !GLQUAKE_ALPHA_DRAWING

/*
================
Sbar_DrawCharacter -- johnfitz -- rewritten now that Draw_SetCanvas is doing the work
================
*/
void Sbar_DrawCharacter (int x, int y, int num)
{
	Draw_Character (x, y + 24, num);
}

/*
================
Sbar_DrawString -- johnfitz -- rewritten now that Draw_SetCanvas is doing the work
================
*/
void Sbar_DrawString (int x, int y, const char *str)
{
	Draw_String (x, y + 24, str);
}

/*
===============
Sbar_DrawScrollString -- johnfitz

scroll the string inside a glscissor region
===============
*/
void Sbar_DrawScrollString (int x, int y, int width, const char *str)
{
#ifdef GLQUAKE_SCALED_DRAWING
	float scale;
	int len, ofs, left;

	// HERE AUTO
	//scale = CLAMP (1.0, gl_sbarscale.value, (float)clwidth / 320.0);
	scale = CLAMP (1.0, vid.sbar_scale /*gl_sbarscale.value*/, (float)clwidth / 320.0);
	left = x * scale;

	if (scr_sbarcentered.value || cl.gametype != GAME_DEATHMATCH)
		left += (((float)clwidth - 320.0 * scale) / 2);

	eglEnable (GL_SCISSOR_TEST);
	eglScissor (left, 0, width * scale, clheight);

	len = strlen(str)*8 + 40;
	ofs = ((int)(realtime*30))%len;
	Sbar_DrawString (x - ofs, y, str);
	Sbar_DrawCharacter (x - ofs + len - 32, y, '/');
	Sbar_DrawCharacter (x - ofs + len - 24, y, '/');
	Sbar_DrawCharacter (x - ofs + len - 16, y, '/');
	Sbar_DrawString (x - ofs + len, y, str);

	eglDisable (GL_SCISSOR_TEST);

#else // WinQuake ...

// Baker: I would invest the time, but where is a levelname > 40 chars
// I think JPL's Hellchepsout?
	char buf[400]={0};
	if (width > 40) width = 40;
	strlcpy (buf, cl.levelname, width);
	Sbar_DrawString (x, y, buf);

#endif // GLQUAKE_SCALED_DRAWING
}


/*
=============
Sbar_itoa
=============
*/
int Sbar_itoa (int num, char *buf)
{
	char	*str;
	int	pow10, dig;

	str = buf;

	if (num < 0)
	{
		*str++ = '-';
		num = -num;
	}

	for (pow10 = 10 ; num >= pow10 ; pow10 *= 10)
	;

	do {
		pow10 /= 10;
		dig = num/pow10;
		*str++ = '0'+dig;
		num -= dig*pow10;
	} while (pow10 != 1);

	*str = 0;

	return str-buf;
}


/*
=============
Sbar_DrawNum
=============
*/
void Sbar_DrawNum (int x, int y, int num, int digits, int color)
{
	char str[12], *ptr;
	int				l, frame;

	num = c_min(999,num); //johnfitz -- cap high values rather than truncating number

	l = Sbar_itoa (num, str);
	ptr = str;
	if (l > digits)
		ptr += (l-digits);
	if (l < digits)
		x += (digits-l)*24;

	while (*ptr)
	{
		frame = (*ptr == '-') ? STAT_MINUS : *ptr -'0';

		Sbar_DrawTransPic (x,y,sb_nums[color][frame]);
		x += 24;
		ptr++;
	}
}

//=============================================================================

int		fragsort[MAX_SCOREBOARD_16];

char	scoreboardtext[MAX_SCOREBOARD_16][20];
int		scoreboardtop[MAX_SCOREBOARD_16];
int		scoreboardbottom[MAX_SCOREBOARD_16];
int		scoreboardcount[MAX_SCOREBOARD_16];
int		scoreboardlines;

/*
===============
Sbar_SortFrags
===============
*/
void Sbar_SortFrags (void)
{
	int		i, j, k;

// sort by frags
	scoreboardlines = 0;
	for (i=0 ; i<cl.maxclients ; i++)
	{
		if (cl.scores[i].name[0])
		{
			fragsort[scoreboardlines] = i;
			scoreboardlines++;
		}
	}

	for (i=0 ; i<scoreboardlines ; i++)
	{
		for (j=0 ; j<scoreboardlines-1-i ; j++)
		{
			if (cl.scores[fragsort[j]].frags < cl.scores[fragsort[j+1]].frags)
			{
				k = fragsort[j];
				fragsort[j] = fragsort[j+1];
				fragsort[j+1] = k;
			}
		}
	}
}


/* JPG - added this for teamscores in status bar
==================
Sbar_SortTeamFrags
==================
*/
void Sbar_SortTeamFrags (void)
{
	int		i, j, k;

// sort by frags
	scoreboardlines = 0;
	for (i=0 ; i<14 ; i++)
	{
		if (cl.pq_teamscores[i].colors)
		{
			fragsort[scoreboardlines] = i;
			scoreboardlines++;
		}
	}

	for (i=0 ; i<scoreboardlines ; i++)
		for (j=0 ; j<scoreboardlines-1-i ; j++)
			if (cl.pq_teamscores[fragsort[j]].frags < cl.pq_teamscores[fragsort[j+1]].frags)
			{
				k = fragsort[j];
				fragsort[j] = fragsort[j+1];
				fragsort[j+1] = k;
			}
}


int	Sbar_ColorForMap (int m)
{
	return m < 128 ? m + 8 : m + 8;
}

/*
===============
Sbar_UpdateScoreboard
===============
*/
void Sbar_UpdateScoreboard (void)
{
	int i, k, top, bottom;
	scoreboard_t	*s;

	Sbar_SortFrags ();

// draw the text
	memset (scoreboardtext, 0, sizeof(scoreboardtext));

	for (i=0 ; i< scoreboardlines; i++)
	{
		k = fragsort[i];
		s = &cl.scores[k];
		c_snprintfc (&scoreboardtext[i][1], sizeof(scoreboardtext[i])-1, "%3d %s", s->frags, s->name);

		top = s->colors & 240; //0xf0;
		bottom = (s->colors & 15) * 16; // <<4; Make readable
		scoreboardtop[i] = Sbar_ColorForMap (top);
		scoreboardbottom[i] = Sbar_ColorForMap (bottom);
	}
}

/*
===============
Sbar_SoloScoreboard -- johnfitz -- new layout
===============
*/

void Sbar_DrawClock (void)
{
	int		minutes;
	int		seconds;
	char	str[256];	
	cbool	proquake_match_timer = pq_timer.value && (cl.pq_minutes != 255); // JPG - check to see if we need to draw the timer.  255
	// What is the 255?

	if (!proquake_match_timer) {
		minutes = Time_Minutes ((int)cl.ctime);
		seconds = Time_Seconds ((int)cl.ctime);
		c_snprintf2 (str, "%d:%02d", minutes, seconds);
	} else {
		// ProQuake match timer.
		//                 prepare for match            no match time yet                        match countdown             match time
		int match_state = (cl.pq_minutes == 254) ? 0 : (!cl.pq_minutes && !cl.pq_seconds) ?  1 : (cl.pq_seconds >= 128) ? 2 : 3;
		int match_time;
		cbool mask = false;
		
		switch (match_state) {
		case 0:		// Starting up
					c_strlcpy (str, "    SD");
					mask = true;
					break;

		case 1:		// No match time yet, use real time.
					minutes = cl.time / 60, seconds = cl.time - 60 * minutes, minutes = minutes & 511; // &511 is probably to limit digit output
					c_snprintf2 (str, "%3d:%02d", minutes, seconds);			// Not hard to imagine a rarely used server on same level for months
					break;

		case 2:		// Negative time ("the countdown")
					c_snprintf1 (str, " -0:%02d", cl.pq_seconds - 128);
					break;

		case 3:		// Match time
					if (cl.pq_match_pause_time)
						match_time = ceil(60.0 * cl.pq_minutes + cl.pq_seconds - (cl.pq_match_pause_time - cl.pq_last_match_time));
					else
						match_time = ceil(60.0 * cl.pq_minutes + cl.pq_seconds - (cl.time - cl.pq_last_match_time));
					minutes = match_time / 60;
					seconds = match_time - 60 * minutes;
					c_snprintf2 (str, "%3d:%02d", minutes, seconds);
					if (!minutes)   // ProQuake I think draws bronzed time when the time left is under 1 minute.  We aren't wired to do that.
						mask = true;
		}
		// End of switch
		
		// Conditionally bronze
		if (mask) {
			int j, len = strlen(str); 
			for (j = 0; j < len; j++)
				str[j] |= 128;
		}

	} // End if master if
	
	Sbar_DrawString ((320 - 8 -3 ) - strlen(str) * 8, -24, str); // Baker: Note time drawn here
}

// Baker: Note this draws even in deathmatch.
void Sbar_SoloScoreboard (void)
{
	static const char *skillstrings[] = {"easy", "normal", "hard", "nightmare"};
	const char	*skillstr = NULL;

	int		levelnamelen = strlen (cl.levelname);
	char	str[256];

	c_snprintf2 (str, "Kills: %d/%d", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
	Sbar_DrawString (8, 12, str);

	c_snprintf2 (str, "Secrets: %d/%d", cl.stats[STAT_SECRETS], cl.stats[STAT_TOTALSECRETS]);
	Sbar_DrawString (312 - strlen(str)*8, 12, str);

	// If not displaying deathmatch scoreboard, show skill level if we know it.
	if (cl.gametype != GAME_DEATHMATCH)
	{
		if (sv.active  || cl.skillhint)
		{
			int _cl_skill_int = sv.active ? (int)(pr_skill.value  + 0.5) : (cl.skillhint - 1);
			int cl_skill_int = CLAMP (0, _cl_skill_int, 3);
			skillstr = skillstrings[cl_skill_int];
		} else skillstr = "skill ???";

		Sbar_DrawString (160 - strlen(skillstr)*4, 12, skillstr);
	}

	if (cls.demoplayback)
	{
		float completed_amount_0_to_1 = (cls.demo_offset_current - cls.demo_offset_start)/(float)cls.demo_file_length;
		int complete_pct_int = 100 - (int)(100 * completed_amount_0_to_1 + 0.5);
		char *tempstring = va ("-%d%%", complete_pct_int);
		int len = strlen(tempstring), i;
		int	leveldrawwidth = 320;

		// Bronze it
		for (i = 0; i < len; i ++)
			tempstring[i] |= 128;

		Sbar_DrawString (312 - len * 8, 4, tempstring);

		leveldrawwidth -= (len + 3) * 8; // Subtract off a few chars for level name

		// If demo playback, we will draw level name left aligned
		if (levelnamelen > 32)
			Sbar_DrawScrollString (8, 4, leveldrawwidth, cl.levelname);
		else Sbar_DrawString (8, 4, cl.levelname);
	}

	// If serving multiplayer (coop or whatever) let's reveal the IP address so another player can connect
	// This is LAN feature.

	else if (sv.active && svs.maxclients_internal > 1 && ipv4Available)  // Because the cap can change at any time now.
	{
		int k = strlen(my_ipv4_address);
		int remaining = 40 - (k + 5); // +2 = one space before ip and one after.

		// Spaces available = 320/8 = 40 - 1 = 39
		// 39 spaces.  (Subtract strlen ip address + 1)

		// Show IP address as a convenience
		Sbar_DrawString (8, 4, my_ipv4_address);

		if (levelnamelen <= remaining)
			Sbar_DrawString ((320 - 8) - (levelnamelen * 8), 4, cl.levelname);  // Right align the level name
		else Sbar_DrawScrollString ((320 - 8) - (remaining * 8), 4, remaining * 8, cl.levelname);
	}
	else
	{
		if (levelnamelen > 40)
			Sbar_DrawScrollString (0, 4, 320, cl.levelname);
		else
			Sbar_DrawString (160 - levelnamelen*4, 4, cl.levelname);
	}
}

/*
===============
Sbar_DrawScoreboard
===============
*/
void Sbar_DrawScoreboard (void)
{
	Sbar_SoloScoreboard ();
	if (cl.gametype == GAME_DEATHMATCH || cl.maxclients > 1)
		Sbar_DeathmatchOverlay ();
}

//=============================================================================

/*
===============
Sbar_DrawInventory
===============
*/
void Sbar_DrawInventory (void)
{
	int		i, flashon;
	char	num[6];
	float	time;

	if (sbar_alpha) {
		if (com_gametype == gametype_rogue)
		{
			if ( cl.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN )
				Sbar_DrawPicAlpha (0, -24, rsb_invbar[0], sbar_alpha);
			else
				Sbar_DrawPicAlpha (0, -24, rsb_invbar[1], sbar_alpha);
		}
		else
		{
			Sbar_DrawPicAlpha (0, -24, sb_ibar, sbar_alpha);
		}
	}

// weapons
	for (i=0 ; i<7 ; i++)
	{
		if (cl.items & (IT_SHOTGUN<<i) )
		{
			time = cl.item_gettime[i];
			flashon = (int)((cl.time - time)*10);
			if (flashon < 0)
				flashon = 0;
			if (flashon >= 10)
				flashon = (cl.stats[STAT_ACTIVEWEAPON] == (IT_SHOTGUN<<i)) ? 1 : 0;
			else
				flashon = (flashon%5) + 2;

         	Sbar_DrawPic (i*24, -16, sb_weapons[flashon][i]);

			if (flashon > 1)
				sb_updates = 0;		// force update to remove flash
		}
	}

// MED 01/04/97
// hipnotic weapons
    if (com_gametype == gametype_hipnotic || com_gametype == gametype_quoth)
    {
      int grenadeflashing=0;

      for (i=0 ; i<4 ; i++)
      {
         if (cl.items & (1<<hipweapons[i]) )
         {
            time = cl.item_gettime[hipweapons[i]];
            flashon = (int)((cl.time - time)*10);
				if (flashon < 0)
					flashon = 0;
            if (flashon >= 10)
				flashon = ( cl.stats[STAT_ACTIVEWEAPON] == (1<<hipweapons[i])  ) ? 1 : 0;
            else
               flashon = (flashon%5) + 2;

            // check grenade launcher
            if (i==2)
            {
               if ((cl.items & HIT_PROXIMITY_GUN) && flashon)
               {
					grenadeflashing = 1;
                     Sbar_DrawPic (96, -16, hsb_weapons[flashon][2]);
               }
            }
            else if (i==3)
            {
               if (cl.items & (IT_SHOTGUN<<4))
               {
                  if (flashon && !grenadeflashing)
                  {
                     Sbar_DrawPic (96, -16, hsb_weapons[flashon][3]);
                  }
                  else if (!grenadeflashing)
                  {
                     Sbar_DrawPic (96, -16, hsb_weapons[0][3]);
                  }
               }
               else
                  Sbar_DrawPic (96, -16, hsb_weapons[flashon][4]);
            }
            else
               Sbar_DrawPic (176 + (i*24), -16, hsb_weapons[flashon][i]);

            if (flashon > 1)
               sb_updates = 0;      // force update to remove flash
         }
      }
    }

	if (com_gametype == gametype_rogue)
	{ // check for powered up weapon.
		if ( cl.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN )
		{
			for (i=0;i<5;i++)
			{
				if (cl.stats[STAT_ACTIVEWEAPON] == (RIT_LAVA_NAILGUN << i))
					Sbar_DrawPic ((i+2)*24, -16, rsb_weapons[i]);
			}
		}
	}

// ammo counts (Baker: Note this location)
	for (i=0 ; i<4 ; i++)
	{
		c_snprintf1 (num, "%3d", c_min(999,cl.stats[STAT_SHELLS+i])); //johnfitz -- cap displayed value to 999
		if (num[0] != ' ')
			Sbar_DrawCharacter ( (6*i+1)*8 + 2, -24, 18 + num[0] - '0');
		if (num[1] != ' ')
			Sbar_DrawCharacter ( (6*i+2)*8 + 2, -24, 18 + num[1] - '0');
		if (num[2] != ' ')
			Sbar_DrawCharacter ( (6*i+3)*8 + 2, -24, 18 + num[2] - '0');
	}

	flashon = 0;

   // items
   for (i=0 ; i<6 ; i++)
	{
      if (cl.items & (1<<(17+i)))
      {
         time = cl.item_gettime[17+i];
         if (time && time > cl.time - 2 && flashon )
         {  // flash frame
            sb_updates = 0;
         }
         else
         {
         //MED 01/04/97 changed keys
            if (!(com_gametype == gametype_hipnotic || com_gametype == gametype_quoth) || (i > 1))
            {
               Sbar_DrawPic (192 + i*16, -16, sb_items[i]);
            }
         }
         if (time && time > cl.time - 2)
            sb_updates = 0;
      }
	}

   //MED 01/04/97 added hipnotic items
   // hipnotic items
	if (com_gametype == gametype_hipnotic || com_gametype == gametype_quoth)
	{
 		for (i=0 ; i<2 ; i++)
		{
			if (cl.items & (1<<(24+i)))
			{
				time = cl.item_gettime[24+i];
				if (time && time > cl.time - 2 && flashon ) // flash frame
					sb_updates = 0;
				else
					Sbar_DrawPic (288 + i*16, -16, hsb_items[i]);
				
				if (time && time > cl.time - 2)
					sb_updates = 0;
			}
  		 }
	}

	// rogue items
	if (com_gametype == gametype_rogue)
	{
	// new rogue items
		for (i=0 ; i<2 ; i++)
		{
			if (cl.items & (1<<(29+i)))
			{
				time = cl.item_gettime[29+i];
				if (time &&	time > cl.time - 2 && flashon ) // flash frame
					sb_updates = 0;
				else
					Sbar_DrawPic (288 + i*16, -16, rsb_items[i]);

				if (time &&	time > cl.time - 2)
					sb_updates = 0;
			}
		}
	}
	else
	{
	// sigils
		for (i=0 ; i<4 ; i++)
		{
			if (cl.items & (1<<(28+i)))
			{
				time = cl.item_gettime[28+i];
				if (time &&	time > cl.time - 2 && flashon ) // flash frame
					sb_updates = 0;
				else
					Sbar_DrawPic (320-32 + i*8, -16, sb_sigil[i]); // Baker note: SIGIL
				if (time &&	time > cl.time - 2)
					sb_updates = 0;
			}
		}
	}
}

//=============================================================================

/*
===============
Sbar_DrawFrags -- johnfitz -- heavy revision
===============
*/
void Sbar_DrawFrags (void)
{
	int				numscores, i, x, color, k;
	char			num[12];
	scoreboard_t	*s;
	int				teamscores, colors, frags; //;//, ent, minutes, seconds, mask; // JPG - added these
	int				player_me = cl.viewentity_player - 1;
//	int				match_time; // JPG - added this

	// JPG - check to see if we should sort teamscores instead
	teamscores = pq_teamscores.value && cl.pq_teamgame;

	if (teamscores)
		Sbar_SortTeamFrags();
	else
		Sbar_SortFrags ();

// draw the text
	numscores = c_min (scoreboardlines, 4);

	// Baker:  If going to draw the clock, cut this back to 2 so nothing draws behind it.
	if ((cl.gametype == GAME_DEATHMATCH && scr_clock.value) || sb_showscores || scr_clock.value > 0)
		numscores = c_min (scoreboardlines, 2);

	for (i = 0, x = 184 ; i < numscores ; i++, x += 32)
	{
		k = fragsort[i]; // Sequence number I guess ...

		// JPG - check for teamscores
		if (teamscores)
		{
			colors = cl.pq_teamscores[k].colors;
			frags = cl.pq_teamscores[k].frags;
		}
		else
		{
			s = &cl.scores[k];
			if (!s->name[0])
				continue;
			colors = s->colors;
			frags = s->frags;
		}

	// top color  // THIS IS ON HUD
		color = colors & 240; // 0xf0;
		color = Sbar_ColorForMap (color);
		Draw_Fill (x + 10, 1, 28, 4, color, 1);

	// bottom color    // THIS IS ON HUD
		color = (colors & 15) * 16 ; // <<4;
		color = Sbar_ColorForMap (color);
		Draw_Fill (x + 10, 4, 28, 4, color, 1);

	// number  // THIS IS ON HUD
		c_snprintf1 (num, "%3d", frags);
		Sbar_DrawCharacter (x + 12, -24, num[0]);
		Sbar_DrawCharacter (x + 20, -24, num[1]);
		Sbar_DrawCharacter (x + 28, -24, num[2]);

	// brackets
		//if (fragsort[i] == cl.viewentity_player - 1)
		if ((teamscores && ((colors & 15) == (cl.scores[player_me].colors & 15))) || (!teamscores && (k == player_me)))
		{
			Sbar_DrawCharacter (x + 5, -24, 16);
			Sbar_DrawCharacter (x + 32, -24, 17);
		}
	}
}

//=============================================================================


/*
===============
Sbar_DrawFace
===============
*/
void Sbar_DrawFace (void)
{
	int		f, anim;

// PGM 01/19/97 - team color drawing
// PGM 03/02/97 - fixed so color swatch only appears in CTF modes

// Baker: Note, whatever rogue was trying to do here won't work for a client, teamplay is server variable and client doesn't know the value
	if (com_gametype == gametype_rogue && (cl.maxclients != 1) && (pr_teamplay.value > 3) && (pr_teamplay.value < 7))
	{
		int				top, bottom;
		int				xofs;
		char			num[12];
		scoreboard_t	*s;

		s = &cl.scores[cl.viewentity_player - 1];
		// draw background
		top = s->colors & 240; // 0xf0;
		bottom = (s->colors & 15) * 16; //<<4;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);

		if (!scr_sbarcentered.value && cl.gametype == GAME_DEATHMATCH)
			xofs = 113;
		else
			xofs = ((clwidth - 320)/2) + 113; // >>1 make readable

		Sbar_DrawPic (112, 0, rsb_teambord);
		Draw_Fill (xofs, /*vid.height-*/24+3, 22, 9, top, 1); //johnfitz -- sbar coords are now relative
		Draw_Fill (xofs, /*vid.height-*/24+12, 22, 9, bottom, 1); //johnfitz -- sbar coords are now relative

		// draw number
		f = s->frags;
		c_snprintf1 (num, "%3d",f);

		if (top==8)
		{
			if (num[0] != ' ')
				Sbar_DrawCharacter(113, 3, 18 + num[0] - '0');
			if (num[1] != ' ')
				Sbar_DrawCharacter(120, 3, 18 + num[1] - '0');
			if (num[2] != ' ')
				Sbar_DrawCharacter(127, 3, 18 + num[2] - '0');
		}
		else
		{
			Sbar_DrawCharacter ( 113, 3, num[0]);
			Sbar_DrawCharacter ( 120, 3, num[1]);
			Sbar_DrawCharacter ( 127, 3, num[2]);
		}

		return;
	}
// PGM 01/19/97 - team color drawing

	if ( (cl.items & (IT_INVISIBILITY | IT_INVULNERABILITY) )
	== (IT_INVISIBILITY | IT_INVULNERABILITY) )
	{
		Sbar_DrawPic (112, 0, sb_face_invis_invuln);
		return;
	}
	if (cl.items & IT_QUAD)
	{
		Sbar_DrawPic (112, 0, sb_face_quad );
		return;
	}
	if (cl.items & IT_INVISIBILITY)
	{
		Sbar_DrawPic (112, 0, sb_face_invis );
		return;
	}
	if (cl.items & IT_INVULNERABILITY)
	{
		Sbar_DrawPic (112, 0, sb_face_invuln);
		return;
	}

		f = cl.stats[STAT_HEALTH] / 20;
	f = CLAMP (0, f, 4);

	if (cl.time <= cl.faceanimtime)
	{
		anim = 1;
		sb_updates = 0;		// make sure the anim gets drawn over
	}
	else
	{
		anim = 0;
	}

	Sbar_DrawPic (112, 0, sb_faces[f][anim]);
}

/*
===============
Sbar_Draw
===============
*/
void Sbar_Draw (void)
{
	float w; //johnfitz

	if (console1.visible_pct == 1)
		return;		// console is full screen

	if (cls.titledemo)
		return;

	if (cl.intermission)
		return; //johnfitz -- never draw sbar during intermission

	// Evaluate must draw scenarios
	do
	{
		if (sb_updates < vid.numpages)
			break;	// Must draw

		if (cls.signon == SIGNONS && (sb_lines &&  (scr_clock.value > 0 || (cl.gametype == GAME_DEATHMATCH && scr_clock.value)) ) )
			break;	// Must draw because clock is showing.

		if (scr_sbaralpha.value < 1)
			break;	// Must draw because sbar is translucent

#ifdef GLQUAKE_RENDERER_SUPPORT
		if (gl_clear.value || /*renderer.isIntelVideo || temp*/ vid.direct3d /* d3d status bar issue, we'll just glclear for now*/)
			break;	// Must draw because screen is cleared

		if (!vid_hardwaregamma.value && vid_contrast.value != 1)
			break;	// Must draw if using polyblend style contrast

		break; // This is owning me pretty bad and I don't want to mess with it right now.
				// With vid_hardwaregamma on, no sbar transparency, no clock display there should only be cause to draw unless status bar changes.
				// Somehow it isn't working and it gets cleared.
				// Is it the stencil buffer clearing?  Or the skybox.
				// Seems to work fine in WinQuake.
				// I could own this, but it isn't worth my time.
				// It could the the intel video thing in glClear .. and maybe nothing is wrong and I'm just being paranoid.
#endif // GLQUAKE_RENDERER_SUPPORT

		// No must draw conditions met, don't draw
		return;
	} while (0);

	sb_updates++;
	Draw_SetCanvas (CANVAS_DEFAULT);

	
#ifdef GLQUAKE_RENDERER_SUPPORT
	sbar_alpha = scr_sbaralpha.value;
	//johnfitz -- don't waste fillrate by clearing the area behind the sbar
	// HERE AUTO
	//w = CLAMP (320.0f, gl_sbarscale.value * 320.0f, (float)clwidth);
	w = CLAMP (320.0f, vid.sbar_scale /*gl_sbarscale.value*/ * 320.0f, (float)clwidth);
	
	if (sb_lines && clwidth > w)
	{
		if (sbar_alpha < 1)
			Draw_TileClear (0, clheight - sb_lines, clwidth, sb_lines);

		if (!scr_sbarcentered.value && cl.gametype == GAME_DEATHMATCH)
			Draw_TileClear (w, clheight - sb_lines, clwidth - w, sb_lines);
		else
		{
			Draw_TileClear (0, clheight - sb_lines, (clwidth - w) / 2.0f, sb_lines);
			Draw_TileClear ((clwidth - w) / 2.0f + w, clheight - sb_lines, (clwidth - w) / 2.0f, sb_lines);
		}
	}
	//johnfitz
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
	winquake_scr_copyeverything = 1;
	sbar_alpha = (scr_sbaralpha.value >= 1); // WinQuake shall use 0 or 1.

	w = 320;
	if (sb_lines && clwidth > w)
		Draw_TileClear (0, clheight - sb_lines, clwidth, sb_lines);
#endif // WINQUAKE_RENDERER_SUPPORT

	Draw_SetCanvas (CANVAS_SBAR); //johnfitz

	if (scr_viewsize.value < 110) //johnfitz -- check viewsize instead of sb_lines
	{
		Sbar_DrawInventory ();
		if (cl.maxclients != 1)
			Sbar_DrawFrags ();
	}

	if (sb_showscores || cl.stats[STAT_HEALTH] <= 0)
	{
		if (sbar_alpha)
			Sbar_DrawPicAlpha (0, 0, sb_scorebar, sbar_alpha);
		Sbar_DrawScoreboard ();
		sb_updates = 0;
	}
	else if (scr_viewsize.value < 120) //johnfitz -- check viewsize instead of sb_lines
	{
		if (sbar_alpha)
			Sbar_DrawPicAlpha (0, 0, sb_sbar, sbar_alpha);

   // keys (hipnotic only)
		//MED 01/04/97 moved keys here so they would not be overwritten
		if (com_gametype == gametype_hipnotic || com_gametype == gametype_quoth)
		{
		 if (cl.items & IT_KEY1)
			Sbar_DrawPic (209, 3, sb_items[0]);
		 if (cl.items & IT_KEY2)
			Sbar_DrawPic (209, 12, sb_items[1]);
		}
   // armor
		if (cl.items & IT_INVULNERABILITY)
		{
			Sbar_DrawNum (24, 0, 666, 3, 1);
			Sbar_DrawPic (0, 0, draw_disc);
		}
		else
		{
			Sbar_DrawNum (24, 0, cl.stats[STAT_ARMOR], 3, cl.stats[STAT_ARMOR] <= 25);

			if (com_gametype == gametype_rogue)
			{
				if (cl.items & RIT_ARMOR3)
					Sbar_DrawPic (0, 0, sb_armor[2]);
				else if (cl.items & RIT_ARMOR2)
					Sbar_DrawPic (0, 0, sb_armor[1]);
				else if (cl.items & RIT_ARMOR1)
					Sbar_DrawPic (0, 0, sb_armor[0]);
			}
			else
			{
				if (cl.items & IT_ARMOR3)
					Sbar_DrawPic (0, 0, sb_armor[2]);
				else if (cl.items & IT_ARMOR2)
					Sbar_DrawPic (0, 0, sb_armor[1]);
				else if (cl.items & IT_ARMOR1)
					Sbar_DrawPic (0, 0, sb_armor[0]);
			}
		}

	// face
		Sbar_DrawFace ();

	// health
		Sbar_DrawNum (136, 0, cl.stats[STAT_HEALTH], 3, cl.stats[STAT_HEALTH] <= 25);

	// ammo icon
		if (com_gametype == gametype_rogue)
		{
			if (cl.items & RIT_SHELLS)
				Sbar_DrawPic (224, 0, sb_ammo[0]);
			else if (cl.items & RIT_NAILS)
				Sbar_DrawPic (224, 0, sb_ammo[1]);
			else if (cl.items & RIT_ROCKETS)
				Sbar_DrawPic (224, 0, sb_ammo[2]);
			else if (cl.items & RIT_CELLS)
				Sbar_DrawPic (224, 0, sb_ammo[3]);
			else if (cl.items & RIT_LAVA_NAILS)
				Sbar_DrawPic (224, 0, rsb_ammo[0]);
			else if (cl.items & RIT_PLASMA_AMMO)
				Sbar_DrawPic (224, 0, rsb_ammo[1]);
			else if (cl.items & RIT_MULTI_ROCKETS)
				Sbar_DrawPic (224, 0, rsb_ammo[2]);
		}
		else
		{
			if (cl.items & IT_SHELLS)
				Sbar_DrawPic (224, 0, sb_ammo[0]);
			else if (cl.items & IT_NAILS)
				Sbar_DrawPic (224, 0, sb_ammo[1]);
			else if (cl.items & IT_ROCKETS)
				Sbar_DrawPic (224, 0, sb_ammo[2]);
			else if (cl.items & IT_CELLS)
				Sbar_DrawPic (224, 0, sb_ammo[3]);
		}

		Sbar_DrawNum (248, 0, cl.stats[STAT_AMMO], 3, cl.stats[STAT_AMMO] <= 10);
	}

#ifdef GLQUAKE_RENDERER_SUPPORT
	//johnfitz -- removed the vid.width > 320 check here

	if (!scr_sbarcentered.value && cl.gametype == GAME_DEATHMATCH)
			Sbar_MiniDeathmatchOverlay ();
#else
	if (clwidth > 320)
	{
		if (!scr_sbarcentered.value && cl.gametype == GAME_DEATHMATCH)
			Sbar_MiniDeathmatchOverlay ();
	}
#endif

	// Game clock drawn if deathmatch and scr_clock != 0 (could be -1 or 1) or scr_clock > 0 ( = 1 always)
	if ( sb_showscores || (   (scr_clock.value > 0 || (cl.gametype == GAME_DEATHMATCH && scr_clock.value) )))
		Sbar_DrawClock ();

	sbar_drawn = true;
}

//=============================================================================

/*
==================
Sbar_IntermissionNumber

==================
*/
void Sbar_IntermissionNumber (int x, int y, int num, int digits, int color)
{
	char			str[12];
	char			*ptr;
	int				l, frame;

	l = Sbar_itoa (num, str);
	ptr = str;
	if (l > digits)
		ptr += (l-digits);
	if (l < digits)
		x += (digits-l)*24;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Draw_TransPic (x,y,sb_nums[color][frame]);
		x += 24;
		ptr++;
	}
}

/*
==================
Sbar_DeathmatchOverlay

==================
*/
// MAIN MULTIPLAY SCOREBOARD
void Sbar_DeathmatchOverlay (void)
{
	qpic_t			*pic;
	int				i, k, l;
	int				top, bottom;
	int				x, y, f;
	char			num[12];
	scoreboard_t	*s;
	int				j, ping;

	if (scr_scoreboard_pings.value && cl.last_ping_time < cl.time - 5)
	{
		MSG_WriteByte (&cls.message, clc_stringcmd);
		SZ_Print (&cls.message, "ping" NEWLINE);
		cl.last_ping_time = cl.time;
		cl.expecting_ping = true;
	}

	if (scr_originalquake2d.value)
		Draw_SetCanvas (CANVAS_SCOREBOARD2); // Baker
	else
		Draw_SetCanvas (CANVAS_MENU); //johnfitz

#ifdef WINQUAKE_RENDERER_SUPPORT
	winquake_scr_copyeverything = 1;
	winquake_scr_fullupdate = 0;
#endif // WINQUAKE_RENDERER_SUPPORT

	pic = Draw_CachePic ("gfx/ranking.lmp");
	M_DrawPicCentered (4, pic);

// scores
	Sbar_SortFrags ();

// draw the text
	l = scoreboardlines;

			// Baker: Adjusted up 4 pixels (was 8 to 4) because looked vertically off-center with 16 players.
	x = 76; //johnfitz -- simplified becuase some positioning is handled elsewhere  
	y = 32;
// draw "ping"
	if (cl.maxclients > 1 && scr_scoreboard_pings.value)
	{
		c_strlcpy (num, "ping");
		for (i = 0 ; i < 4 ; i++)
			Draw_Character (x - 56 + i *8, y - 10 - 2, num[i]+128); // Baker: Tweaked it up just a little
	}

	for (i=0 ; i<l ; i++)
	{
		k = fragsort[i];
		s = &cl.scores[k];
		if (!s->name[0])
			continue;

	// draw background    // MAIN SCOREBOARD
		top = s->colors & 240; // 0xf0;
		bottom = (s->colors & 15) *16; // <<4;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);

		Draw_Fill ( x, y, 40, 4, top, 1);
		Draw_Fill ( x, y+4, 40, 4, bottom, 1);

	//  draw ping
		if (s->ping && scr_scoreboard_pings.value)
		{
			ping = 1;
			c_snprintf1 (num, "%4d", s->ping);

			for (j = 0 ; j < 4 ; j++)
				Draw_Character(x-56+j*8, y, num[j]);
		}

	// draw number
		f = s->frags;
		c_snprintf1 (num, "%3d",f);

		Draw_Character ( x+8 , y, num[0]);
		Draw_Character ( x+16 , y, num[1]);
		Draw_Character ( x+24 , y, num[2]);

		if (k == cl.viewentity_player - 1)
		{
			Draw_Character(x-2, y, 16);	// Baker (int), JPG 3.00 from katua - draw [ ] around our score in the
			Draw_Character(x+32, y, 17);	// scoreboard overlay
		}


	// draw name
		// Baker: Changed to M_PrintWhite because of complaint about bronzing
		Draw_String (x + 64, y, s->name);

		y += 10;
	}

	Draw_SetCanvas (CANVAS_SBAR); //johnfitz
}

/*
==================
Sbar_MiniDeathmatchOverlay
==================
*/
void Sbar_MiniDeathmatchOverlay (void)
{
	int				i, k, top, bottom, x, y, f, numlines, printed=0;
	char			num[12];
	scoreboard_t	*s;

#ifdef GLQUAKE_RENDERER_SUPPORT
	float			scale; //johnfitz
	// HERE AUTO
	//scale = CLAMP (1.0, gl_sbarscale.value, (float)clwidth / 320.0); //johnfitz
	scale = CLAMP (1.0, vid.sbar_scale /*gl_sbarscale.value*/, (float)clwidth / 320.0); //johnfitz

	//MAX_SCOREBOARDNAME_32 = 32, so total width for this overlay plus sbar is 632, but we can cut off some i guess
	if (clwidth/scale < 512 || scr_viewsize.value >= 120) //johnfitz -- test should consider gl_sbarscale
		return;
#else // WinQuake
	if (clwidth < 512 || !sb_lines)
		return;

	winquake_scr_copyeverything = 1;// Copy more than viewsize box view
	winquake_scr_fullupdate = 0;
#endif // GLQUAKE_RENDERER_SUPPORT

// scores
	Sbar_SortFrags ();

// draw the text
	numlines = (scr_viewsize.value >= 110) ? 3 : 6; //johnfitz

	//find us (looks through the look and i will be our number!
	for (i = 0; i < scoreboardlines; i++)
		if (fragsort[i] == cl.viewentity_player - 1)
			break;

	// i is now our position in the frag sorted list

    if (i == scoreboardlines) // we're not there
            i = 0;
    else // figure out start
            i = i - numlines/2;
    i = CLAMP (0, i, scoreboardlines - numlines);
	if (i < 0)
		i = 0; // "scoreboardlines-numlines" can result in a negative number, so we need this.  Baker  An example is a 4 player deathmatch (4 participants - 6 lines = -2!!!)

	x = 324;
	y = (scr_viewsize.value >= 110) ? 24 : 0; //johnfitz -- start at the right place
	for ( ; i < scoreboardlines && y <= 48; i++, y+=8)
	{
		k = fragsort[i];
		s = &cl.scores[k];
		if (!s->name[0])
			continue;

	// colors  // THIS IS OPTIONAL AND AT THE SIDE
		top = s->colors & 240; // 0xf0;
		bottom = (s->colors & 15) * 16; //<<4;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);

#pragma message ("Is the following 4 and then 3 or 3,4 .  Bet the latter")
		Draw_Fill ( x, y+1, 40, 4, top, 1);
		Draw_Fill ( x, y+4, 40, 3, bottom, 1); // Baker (int)

	// number
		f = s->frags;
		c_snprintf1 (num, "%3d",f);

		Draw_Character ( x+8 , y, num[0]);
		Draw_Character ( x+16 , y, num[1]);
		Draw_Character ( x+24 , y, num[2]);

	// brackets
		if (k == cl.viewentity_player - 1)
		{
			Draw_Character ( x - 2, y, 16); // Baker (int)
			Draw_Character ( x+32, y, 17);
		}

	// name
		Draw_String (x+48, y, s->name);
		printed++;
	}
}

/*
==================
Sbar_IntermissionOverlay
==================
*/
void Sbar_IntermissionOverlay (void)
{
	qpic_t	*pic;
	int		dig;
	int		num;

#ifdef WINQUAKE_RENDERER_SUPPORT
	winquake_scr_copyeverything = 1;   // Copy more than viewsize box view
	winquake_scr_fullupdate = 0;
#endif // WINQUAKE_RENDERER_SUPPORT

	if (cl.gametype == GAME_DEATHMATCH)
	{
		Sbar_DeathmatchOverlay ();
		return;
	}

	if (cl.maxclients > 1 && vm_coop_enhancements.value && sb_showscores)
	{
		Sbar_DeathmatchOverlay ();
		return;
	}

	Draw_SetCanvas (CANVAS_MENU); //johnfitz

	pic = Draw_CachePic ("gfx/complete.lmp");
	Draw_Pic (64, 24, pic);

	pic = Draw_CachePic ("gfx/inter.lmp");
	Draw_TransPic (0, 56, pic);

// time
	dig = cl.completed_time/60;
	Sbar_IntermissionNumber (152, 64, dig, 3, 0);
	num = cl.completed_time - dig*60;
	Draw_TransPic (224,64,sb_colon);
	Draw_TransPic (240,64,sb_nums[0][num/10]);
	Draw_TransPic (264,64,sb_nums[0][num%10]);

	Sbar_IntermissionNumber (152, 104, cl.stats[STAT_SECRETS], 3, 0);
	Draw_TransPic (224,104,sb_slash);
	Sbar_IntermissionNumber (240, 104, cl.stats[STAT_TOTALSECRETS], 3, 0);

	Sbar_IntermissionNumber (152, 144, cl.stats[STAT_MONSTERS], 3, 0);
	Draw_TransPic (224,144,sb_slash);
	Sbar_IntermissionNumber (240, 144, cl.stats[STAT_TOTALMONSTERS], 3, 0);
}


/*
==================
Sbar_FinaleOverlay
==================
*/
void Sbar_FinaleOverlay (void)
{
	qpic_t	*pic;

	Draw_SetCanvas (CANVAS_MENU_INTERMISSION_TEXT); //johnfitz
#ifdef WINQUAKE_RENDERER_SUPPORT
	winquake_scr_copyeverything = 1;
#endif // WINQUAKE_RENDERER_SUPPORT

	pic = Draw_CachePic ("gfx/finale.lmp");
	Draw_TransPic ( (320 - pic->width)/2, 16, pic);
}


