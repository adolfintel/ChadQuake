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
// screen.c -- master for refresh, status bar, console, chat, notify, etc



#include "quakedef.h"


/*

background clear
rendering
turtle/net/ram icons
sbar
centerprint / slow centerprint
notify lines
intermission / finale overlay
loading plaque
console
menu

required background clears
required update regions


syncronous draw mode or async
One off screen buffer, with updates either copied or xblited
Need to double buffer?


async draw will require the refresh area to be cleared, because it will be
xblited, but sync draw can just ignore it.

sync
draw

CenterPrint ()
SlowPrint ()
Screen_Update ();
Con_Printf ();

net
turn off messages option

the refresh is always rendered, unless the console is full screen


console is:
	notify lines
	half
	full

*/


int			clx, cly, clwidth, clheight;

// only the refresh window will be updated unless these variables are flagged
#ifdef WINQUAKE_RENDERER_SUPPORT
int			winquake_scr_copytop;
int			winquake_scr_copyeverything;
int			winquake_scr_fullupdate;
#endif // WINQUAKE_RENDERER_SUPPORT

#ifdef GLQUAKE_RENDERER_SUPPORT
int			glquake_scr_tileclear_updates = 0; //johnfitz
#endif // GLQUAKE_RENDERER_SUPPORT

float		oldscreensize, oldfov, oldsbaralpha;

#ifdef GLQUAKE_SCALED_DRAWING
float		oldsbarscale, oldmenuscale; //johnfitz -- added oldsbarscale and oldsbaralpha
#endif // GLQUAKE_SCALED_DRAWING



cbool	scr_skipupdate;

cbool	scr_initialized;		// ready to draw

qpic_t		*scr_ram;
qpic_t		*scr_net;
qpic_t		*scr_turtle;

int			clearconsole;
int			clearnotify;


vrect_t		scr_vrect;

cbool	scr_disabled_for_loading;
cbool	scr_drawloading;
float		scr_disabled_time;


//void SCR_ScreenShot_f (void);

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
//float		scr_centertime_off;		// moved to cl.
int			scr_center_lines;
int			scr_erase_lines;
int			scr_erase_center;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (const char *str) //update centerprint data
{
	c_strlcpy (scr_centerstring, str);
	cl.scr_centertime_off = cl.time + scr_centertime.value;
	scr_centertime_start = cl.time;

// count the number of lines for centering
	scr_center_lines = 1;
	str = scr_centerstring;
	while (*str)
	{
		if (*str == '\n')
			scr_center_lines++;
		str++;
	}
}

void SCR_DrawCenterString (void) //actually do the drawing
{
	char	*start;
	int		l;
	int		j;
	int		x, y;
	int		remaining;
	float 	sbscale = 1;
	float	conheight_ratio = 1;
	float	sb_lines_translated_to_conheight;

	if (cl.intermission) // Note: cl.intermission 1 never hits here (Monsters X/Y, Secrets, etc.)
		Draw_SetCanvas (CANVAS_MENU_INTERMISSION_TEXT); //johnfitz
	else
		Draw_SetCanvas (CANVAS_DEFAULT_CONSCALE);

// the finale prints the characters one at a time
	if (cl.intermission)
		remaining = scr_printspeed.value * (cl.time - scr_centertime_start);
	else
		remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	if (cl.intermission)
		y = 200 * 0.35 - 24 /* Baker */; //johnfitz -- 320x200 coordinate system
	else
	{
		// Baker: We need to translate the center print lines relative to the center of the screen
		// based on the scr_conheight and sblines including removing scr_sbarscale.  Ugggh.
		// Future: Come up with some sort of canvas to deal with this.
		// However, the following will place text identically regardless of conscale or sbarscale or resolution
		// Which is a major plus.  And it restores the placement to the WinQuake/GL location instead of dead-center

#ifdef GLQUAKE_SCALED_DRAWING
		// HERE AUTO
		//sbscale = CLAMP (1.0, gl_sbarscale.value, (float)clwidth / 320.0);
		sbscale = CLAMP (1.0, vid.sbar_scale /*gl_sbarscale.value*/, (float)clwidth / 320.0);
		conheight_ratio = (float) vid.conheight/clheight;
#endif // GLQUAKE_SCALED_DRAWING

		sb_lines_translated_to_conheight = sb_lines * sbscale * conheight_ratio;
//#ifdef GLQUAKE_RENDERER_SUPPORT
		if (scr_originalquake2d.value) {
			//Draw_SetCanvas (CANVAS_MENU_INTERMISSION_TEXT);
			Draw_SetCanvas (CANVAS_DEFAULT_CONSCALE); // We still need console considered.
			//Draw_SetCanvas (CANVAS_DEFAULT_CONSCALE); // 	CANVAS_DEFAULT_CONSCALE, // Baker: For centerprint except intermission
			if (scr_center_lines <= 4)
				y = vid.conheight * 0.35; // Hacks!  Not neutral.
			else
				y = 48;


		}

		else
//#endif // GLQUAKE_RENDERER_SUPPORT
		{
			if (scr_center_lines <= 4)
				y = (vid.conheight-sb_lines_translated_to_conheight/2.0f)*0.35;
			else
				y =  (vid.conheight-sb_lines_translated_to_conheight/2.0f)*0.35 - (scr_center_lines * 8 * conheight_ratio)/2;
		}
	}

	do
	{
	// scan the width of the line
		for (l = 0 ; l < 40 ; l ++)
			if (start[l] == '\n' || !start[l])
				break;

		if (cl.intermission)
			x = (320 - l*8)/2;	//johnfitz -- 320x200 coordinate system
		else
			x =  (vid.conwidth - l * 8)/2; // (gl_scr_winquake.value) ? 0 :

// Baker: detect crosshair scenario

		for (j=0 ; j<l ; j++, x+=8)
		{
			Draw_Character (x, y, start[j]);
			if (!remaining--)
				return;
		}

		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

void SCR_CheckDrawCenterString (void)
{
#ifdef WINQUAKE_RENDERER_SUPPORT
	winquake_scr_copytop = 1;
#endif // WINQUAKE_RENDERER_SUPPORT

	if (scr_center_lines > scr_erase_lines)
		scr_erase_lines = scr_center_lines;

	// Baker: independent physics
	//cl.scr_centertime_off -= host_frametime;
	if (cl.scr_centertime_off && cl.scr_centertime_off < cl.time)
		cl.scr_centertime_off = 0;

	if (cl.scr_centertime_off <= 0 && !cl.intermission)
		return;
//	if (key_dest != key_game) // WHY????
//		return;
//	if (cl.paused) //johnfitz -- don't show centerprint during a pause  WHY?
//		return;

	// Baker: For any type of game that displays the scoreboard on TAB
	// Don't draw the centerprint
	if (sb_showscores && (cl.gametype == GAME_DEATHMATCH || (cl.maxclients > 1 && vm_coop_enhancements.value)) )
		return;

	SCR_DrawCenterString ();
}

//=============================================================================

/*
====================
AdaptFovx
Adapt a 4:3 horizontal FOV to the current screen size using the "Hor+" scaling:
2.0 * atan(width / height * 3.0 / 4.0 * tan(fov_x / 2.0))
====================
*/
#define MH_ASPECT (432.0f/640)
float AdaptFovx (float fov_x, float width, float height)
{
	float	a, x;
	float fov_x_clamped = CLAMP(1, fov_x, 179); // Baker:  The Malice commercial Quake mod :(
	
//	if (fov_x < 1 || fov_x > 179)
//		Host_Error ("Bad fov: %f", fov_x);

	if ((x = height / width) == 0.75)
		return fov_x_clamped;
	a = atan(MH_ASPECT / x * tan(fov_x_clamped / 360 * M_PI));
	a = a * 360 / M_PI;
	return a;
}

/*
====================
CalcFovy
====================
*/
float CalcFovy (float fov_x, float width, float height)
{
    float   a, x;
	float fov_x_clamped = CLAMP(1, fov_x, 179); // Baker:  The Malice commercial Quake mod :(
    
//	if (fov_x < 1 || fov_x > 179)
//		Host_Error ("Bad fov: %f", fov_x);

    x = width/tan(fov_x_clamped/360*M_PI);
    a = atan (height/x);
    a = a*360/M_PI;
    return a;
}

/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/


static void SCR_CalcRefdef (void)
{
	float		size;
	float		fov_base;
#ifdef GLQUAKE_SCALED_DRAWING
	float		scale; //johnfitz -- scale
#else
	vrect_t		vrect;
#endif

	vid.recalc_refdef = 0;

// force the status bar to redraw
	Sbar_Changed ();

#ifdef GLQUAKE_RENDERER_SUPPORT
	glquake_scr_tileclear_updates = 0; //johnfitz
#endif //  GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
	winquake_scr_fullupdate = 0;		// force a background redraw
#endif // WINQUAKE_RENDERER_SUPPORT

// bound viewsize
	if (scr_viewsize.value < 30)
		Cvar_SetValueQuick (&scr_viewsize, 30); // Baker: Security risk, can commit a value from progs/demo/server without proper channel.
	if (scr_viewsize.value > 120)
		Cvar_SetValueQuick (&scr_viewsize, 120); // Baker: Security risk, can commit a value from progs/demo/server without proper channel.

// bound field of view
	if (scr_fov.value < 10)
		Cvar_SetValueQuick (&scr_fov, 10); // Baker: Security risk, can commit a value from progs/demo/server without proper channel.
	if (scr_fov.value > 170)
		Cvar_SetValueQuick (&scr_fov, 170); // Baker: Security risk, can commit a value from progs/demo/server without proper channel.

	//johnfitz -- rewrote this section
	size = scr_viewsize.value;

	if (size >= 120 || cl.intermission)
		sb_lines = 0;
	else if (cls.titledemo)
		sb_lines = 0;
	else if (size >= 110)
		sb_lines = 24;		// no inventory
	else
		sb_lines = 48;

	if (scr_sbaralpha.value < 1)
		sb_lines = 0;

#ifdef GLQUAKE_SCALED_DRAWING
	// HERE AUTO
	//scale = CLAMP (1.0, gl_sbarscale.value, (float)clwidth / 320.0);
	scale = CLAMP (1.0, vid.sbar_scale /*gl_sbarscale.value*/, (float)clwidth / 320.0);

	if (sb_lines) sb_lines *= scale;
#endif //  GLQUAKE_SCALED_DRAWING

	if (cls.titledemo)
		size = 1; // Title demos are maximum viewsize
	else
		size = c_min(scr_viewsize.value, 100) / 100.0f;

	if (cls.titledemo)
		fov_base = 90.0f;
	else
		fov_base = scr_fov.value;

#ifdef GLQUAKE_RENDERER_SUPPORT
	//johnfitz -- rewrote this section
	r_refdef.vrect.width = c_rint (c_max(clwidth * size, 96)); //no smaller than 96, for icons
	r_refdef.vrect.height = c_rint (c_min(clheight * size, clheight - sb_lines)); //make room for sbar
	r_refdef.vrect.x = (clwidth - r_refdef.vrect.width)/2;
	r_refdef.vrect.y = (clheight - sb_lines - r_refdef.vrect.height)/2;

	r_refdef.fov_x = AdaptFovx(scr_fov.value, r_refdef.vrect.width, r_refdef.vrect.height);
	r_refdef.fov_y = CalcFovy (r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);

	scr_vrect = r_refdef.vrect;
#else // WinQuake ...
	vrect.height = clheight - sb_lines;

	// Baker: Software works in a rather different order :(
	if (size < 1)
	{
		int rectwidth = c_max(clwidth * size, 96); //no smaller than 96, for icons
		int rectheight = c_min(clheight * size, clheight - sb_lines);
		r_refdef.fov_x = AdaptFovx (fov_base, rectwidth, rectheight);
		r_refdef.fov_y = CalcFovy (r_refdef.fov_x, rectwidth, rectheight);
	}
	else
	{
		r_refdef.fov_x = AdaptFovx (fov_base, clwidth, vrect.height);
		r_refdef.fov_y = CalcFovy (r_refdef.fov_x, clwidth, vrect.height);

	}

	r_fovx = r_refdef.fov_x;
	r_fovy = r_refdef.fov_y;

// these calculations mirror those in R_Init() for r_refdef, but take no
// account of water warping
	vrect.x = 0;
	vrect.y = 0;
	vrect.width = (int)clwidth;
	vrect.height = (int)clheight;

	R_SetVrect (&vrect, &scr_vrect, sb_lines);

// notify the refresh of the change
	R_ViewChanged (&vrect, sb_lines, vid.aspect);
#endif
}



/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f (void)
{
#ifdef SUPPORTS_CUTSCENE_PROTECTION
	if (cmd_from_server)
	{
		// Baker: To prevent viewsize check in screen.c from formally commiting an untrusted value out-of-range value.
		int newval = scr_viewsize.value+10;
		newval = CLAMP (30, newval, 120);
		Cvar_Set_Untrusted (scr_viewsize.name, va("%d", (int)newval) );
	}
	else
#endif // SUPPORTS_CUTSCENE_PROTECTION
	Cvar_SetValueQuick (&scr_viewsize, scr_viewsize.value + 10);
	vid.recalc_refdef = 1;
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f (void)
{
#ifdef SUPPORTS_CUTSCENE_PROTECTION
	if (cmd_from_server)
	{
		// Baker: To prevent viewsize check in screen.c from formally commiting an untrusted value out-of-range value.
		int newval = scr_viewsize.value-10;
		newval = CLAMP (30, newval, 120);
		Cvar_Set_Untrusted (scr_viewsize.name, va("%d", (int)newval) );
	}
	else
#endif // SUPPORTS_CUTSCENE_PROTECTION
	Cvar_SetValueQuick (&scr_viewsize, scr_viewsize.value - 10);
	vid.recalc_refdef = 1;
}

#ifdef GLQUAKE_SCALED_DRAWING
/*
==================
SCR_Conwidth_f -- johnfitz -- called when scr_conwidth or scr_conscale changes
==================
*/
void SCR_Conwidth_f (cvar_t *var)
{
	vid.consize_stale = true;
}
#endif // GLQUAKE_SCALED_DRAWING

//============================================================================

/*
==================
SCR_LoadPics -- johnfitz
==================
*/
void SCR_LoadPics (void)
{
	scr_ram = Draw_PicFromWad ("ram");
	scr_net = Draw_PicFromWad ("net");
	scr_turtle = Draw_PicFromWad ("turtle");
}

/*
==================
SCR_Init
==================
*/

void SCR_Init (void)
{
	
	Cmd_AddCommands (SCR_Init);

	SCR_LoadPics (); //johnfitz

#ifdef SUPPORTS_AVI_CAPTURE // Baker change
	Movie_Init ();
#endif // Baker change +

	scr_initialized = true;
}

//============================================================================

/*
==============
SCR_DrawFPS -- johnfitz
==============
*/
void SCR_DrawFPS (void)
{
	static double	oldtime = 0;
	static double	lastfps = 0;
	static int		oldframecount = 0;
	double	elapsed_time;
	int	frames;

	elapsed_time = realtime - oldtime;
	frames = cl.r_framecount - oldframecount;

	if (elapsed_time < 0 || frames < 0)
	{
		oldtime = realtime;
		oldframecount = cl.r_framecount;
		return;
	}
	// update value every 3/4 second
	if (elapsed_time > 0.75)
	{
		lastfps = frames / elapsed_time;
		oldtime = realtime;
		oldframecount = cl.r_framecount;
	}

	if (cl.worldmodel && scr_viewsize.value >= 100) {
		cbool display_demo = sb_showscores && (cls.demorecording || cl_autodemo.value);
		if (scr_showfps.value || scr_showpos.value || display_demo || scr_showspeed.value) {
			char			st[64];
			int				x, y = 12;
			
			if (sb_showscores && (cls.demorecording || cl_autodemo.value) ) {
				Draw_SetCanvas (CANVAS_TOPLEFT);
				if ( cls.demorecording) {
					const char *demo_short_name = File_URL_SkipPath (cls.demo_url);
					c_snprintf2 (st, "%c %s", 139, demo_short_name);
				} else {
					c_strlcpy (st, "No demo recording");
				}
				x = 8; // 320 - 8 - (strlen(st) * 8);
				Draw_StringEx (x, y, st);  // y += 8;
			}

			Draw_SetCanvas (CANVAS_TOPRIGHT);
			
			if (scr_showpos.value) {
				c_snprintf3 (st, "@ %3.0f %3.0f %3.0f", 
					cl_entities[cl.viewentity_player].origin[0], 
					cl_entities[cl.viewentity_player].origin[1], 
					cl_entities[cl.viewentity_player].origin[2]);
				x = 320 - 8 - (strlen(st) * 8);
				Draw_String (x, y, st);  y += 8;
			}

			if (scr_showspeed.value) {
				vec3_t	vel			= {cl.velocity[0], cl.velocity[1], 0};
				float	speed		= VectorLength (vel);
				float	vspeed		= cl.velocity[2];

				c_snprintf1 (st, "speed: %4.0f", speed);
				x = 320 - 8 - (strlen(st) * 8);
				Draw_String (x, y, st);  y += 8;
			}


			if (scr_showfps.value) {
				c_snprintf1 (st, "%4.0f", lastfps);
				x = 320 - 8 - (strlen(st) * 8);
				Draw_String (x, y, st);  y += 8;
			}



#ifdef GLQUAKE_RENDERER_SUPPORT
			glquake_scr_tileclear_updates = 0;
#endif // GLQUAKE_RENDERER_SUPPORT
		}
	}

}

/*
==============
SCR_DrawDevStats
==============
*/
void SCR_DrawDevStats (void)
{
	char	str[40];
	int		y = 25-9; //9=number of lines to print
	int		x = 0; //margin

	if (!devstats.value)
		return;

	Draw_SetCanvas (CANVAS_BOTTOMLEFT);

	Draw_Fill (x, y*8, 19*8, 9*8, 0, 0.5); //dark rectangle

	c_strlcpy (str, "devstats |Curr Peak");
	Draw_String (x, (y++)*8-x, str);

	c_strlcpy (str, "---------+---------");
	Draw_String (x, (y++)*8-x, str);

	c_snprintf2 (str, "Edicts   |%4d %4d", dev_stats.edicts, dev_peakstats.edicts);
	Draw_String (x, (y++)*8-x, str);

	c_snprintf2 (str, "Packet   |%4d %4d", dev_stats.packetsize, dev_peakstats.packetsize);
	Draw_String (x, (y++)*8-x, str);

	c_snprintf2 (str, "Visedicts|%4d %4d", dev_stats.visedicts, dev_peakstats.visedicts);
	Draw_String (x, (y++)*8-x, str);

	c_snprintf2 (str, "Efrags   |%4d %4d", dev_stats.efrags, dev_peakstats.efrags);
	Draw_String (x, (y++)*8-x, str);

	c_snprintf2 (str, "Dlights  |%4d %4d", dev_stats.dlights, dev_peakstats.dlights);
	Draw_String (x, (y++)*8-x, str);

	c_snprintf2 (str, "Beams    |%4d %4d", dev_stats.beams, dev_peakstats.beams);
	Draw_String (x, (y++)*8-x, str);

	c_snprintf2 (str, "Tempents |%4d %4d", dev_stats.tempents, dev_peakstats.tempents);
	Draw_String (x, (y++)*8-x, str);
}

/*
==============
SCR_DrawRam
==============
*/
void SCR_DrawRam (void)
{
#ifdef WINQUAKE_RENDERER_SUPPORT
	if (!scr_showram.value)
		return;

// Baker: r_cache_thrash shows RAM.  r_cache_thrash is WinQuake only.
// Result: GLQuake never shows RAM since condition can never be true.

	if (!r_cache_thrash)
		return;

	Draw_SetCanvas (CANVAS_DEFAULT); //johnfitz

	Draw_Pic (scr_vrect.x+32, scr_vrect.y, scr_ram);
#endif // WINQUAKE_RENDERER_SUPPORT
}

/*
==============
SCR_DrawTurtle
==============
*/
void SCR_DrawTurtle (void)
{
	static int	count;

	if (!scr_showturtle.value)
		return;

	if (host_frametime_ < 0.1) // Needs to measure true slowness
	{
		count = 0;
		return;
	}

	count++;
	if (count < 3)
		return;

	Draw_SetCanvas (CANVAS_DEFAULT); //johnfitz

	Draw_Pic (scr_vrect.x, scr_vrect.y, scr_turtle);
}

/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	if (cl.paused && sv.active) // Don't do this for active server in pause.
		return;

	if (realtime - cl.last_received_message < 0.3)
		return;

	if (cls.demoplayback)
		return;

	Draw_SetCanvas (CANVAS_DEFAULT); //johnfitz

	Draw_Pic (scr_vrect.x+64, scr_vrect.y, scr_net);
}

/*
==============
DrawPause
==============
*/
void SCR_DrawPause (void)
{
	qpic_t	*pic;

	if (!cl.paused)
		return;

	if (!scr_showpause.value)		// turn off for screenshots
		return;

	Draw_SetCanvas (CANVAS_MENU); //johnfitz

	pic = Draw_CachePic ("gfx/pause.lmp");
	Draw_Pic ( (320 - pic->width)/2, (240 - 48 - pic->height)/2, pic); //johnfitz -- stretched menus

#ifdef GLQUAKE_RENDERER_SUPPORT
	glquake_scr_tileclear_updates = 0; //johnfitz
#endif // GLQUAKE_RENDERER_SUPPORT
}

/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading (void)
{
	qpic_t	*pic;

	if (!scr_drawloading)
		return;

	Draw_SetCanvas (CANVAS_MENU); //johnfitz

	pic = Draw_CachePic ("gfx/loading.lmp");
	Draw_Pic ( (320 - pic->width) / 2, (240 - 48 - pic->height) / 2, pic); //johnfitz -- stretched menus

#ifdef GLQUAKE_RENDERER_SUPPORT
	glquake_scr_tileclear_updates = 0; //johnfitz (Baker: For when viewsize tile can be messed up
#endif // GLQUAKE_RENDERER_SUPPORT
}


/*
==============
SCR_DrawCrosshair -- johnfitz
==============
*/
void SCR_DrawCrosshair (void)
{
	if (!scr_crosshair.value)
		return;

	if (key_dest == key_menu)
		return;

	if (cls.titledemo)
		return;

	Draw_SetCanvas (CANVAS_CROSSHAIR);


#ifdef GLQUAKE_TEXTURE_MANAGER
	if (crosshair_weapon_textures_found && scr_weapon_crosshair.value  ) { // Per weapon
		int crosshair_num = Host_ActiveWeapon_0_to_24_or_Neg1 ();
		
		if (crosshair_num == -1)
			goto missing_weapon_crosshair; // This should never happen, I don't think.

		//int active_weapon = cl.stats[STAT_ACTIVEWEAPON]; // 0 = axe
		crosshair_num = CLAMP (0 /*axe*/, crosshair_num, MAX_CROSSHAIRS_25 - 1); // IT_SUPER_LIGHTNING or maybe plaz */ ); // Automatic
		if (!crosshair_weapon_textures [crosshair_num]) 
			goto missing_weapon_crosshair;  // Render a +
	
		Draw_GLTexture (crosshair_weapon_textures [crosshair_num], -15,-15,16,16);
		return;
	}
		

missing_weapon_crosshair:


#endif // GLQUAKE_TEXTURE_MANAGER

	if (scr_crosshair.value > 1) { // Dot crosshair
		Draw_Character (-4, -4, 15); //0,0 is center of viewport
		return;
	}

#ifdef GLQUAKE_TEXTURE_MANAGER
	if (crosshair_default_texture) {
		Draw_GLTexture (crosshair_default_texture, -15,-15,16,16);
		return;
	}

#endif // GLQUAKE_TEXTURE_MANAGER

	// Standard Quake crosshair
	Draw_Character (-4, -4, '+'); 
}




/*
==============
SCR_Draw_Download
==============
*/
#ifdef SUPPORTS_HTTP_DOWNLOAD
void SCR_Draw_Download (void)
{
// This should be used for map download, not mod download.
	int full_width, complete_width;

	if (!cls.download.name[0])
		return;

	Draw_SetCanvas (CANVAS_DEFAULT_CONSCALE);

	full_width = (vid.conwidth - 32);
	complete_width = cls.download.percent * full_width;

	Draw_Fill (0, 0, vid.conwidth, 32, 0, 1);
	Draw_Fill (16, 8, full_width, 4, 16, 1);
	Draw_Fill (16, 8, complete_width, 4, 23, 1);
	Draw_StringEx (16, 16, va("Downloading: \b%s\b - %s kb (\b%.1f\b%%)%s", cls.download.name, cls.download.total_bytes == -1 ? "???" : va("%d", cls.download.total_bytes / 1024), cls.download.percent * 100, cls.download.is_blocking ? "\n (ESC = abort)" : "") );
}
#endif // SUPPORTS_HTTP_DOWNLOAD


//=============================================================================


/*
==================
SCR_SetUpToDrawConsole
==================
*/
void SCR_SetUpToDrawConsole (void)
{
	//johnfitz -- let's hack away the problem of slow console when host_timescale is <0
	float	conheight_ratio = 1;



	Con_CheckResize ();

	if (scr_drawloading)
		return;		// never a console with loading plaque

// decide on the height of the console
	console1.forcedup = !cl.worldmodel || cls.signon != SIGNONS;

	if (console1.forcedup)
	{
		// Baker: Scenario #1 - console forced up because no map
		console1.wanted_pct =	// Baker: Wants 100% size (set console1.wanted_pct)
		console1.visible_pct =  // Baker: And it shall instantly take effect now (set console1.visible_pct)
			1;
	}
	else if (key_dest == key_console)
	{
		// Baker: Scenario #2 - console open.
		// Baker: clamping this every frame, possibly for video resize
		console1.wanted_pct = CLAMP (CONSOLE_MINIMUM_PCT_10, console1.user_pct, CONSOLE_MAX_USER_PCT_90);
	}
	else console1.wanted_pct = 0; // Baker: none visible, retraction in effect

	// Baker: The speed needs scaled by a formula
	if (console1.visible_pct != console1.wanted_pct)
	{
		// Resolution independent speed
		//float resolution_speed = (scr_conspeed.value * clheight / 480.0)/clheight;
		//float speed_amount = (scr_conspeed.value / 480.0) * host_frametime / frame_timescale;
		float speed_amount = (scr_conspeed.value / 480.0) * host_timeslice;  // Keep: 28 Apr 2015

		if (console1.visible_pct < console1.wanted_pct)
		{
			console1.visible_pct += speed_amount;
			// Check for missed destination condition
			if (console1.visible_pct > console1.wanted_pct)
				console1.visible_pct = console1.wanted_pct;
		}
		else
		{
			console1.visible_pct -= speed_amount;
			// Check for missed destination condition
			if (console1.visible_pct < console1.wanted_pct)
				console1.visible_pct = console1.wanted_pct;
		}


	}

#ifdef GLQUAKE_SCALED_DRAWING
	conheight_ratio = (float)vid.conheight / clheight;
#endif // GLQUAKE_SCALED_DRAWING

	// Baker: Clamp + calc time
	console1.visible_pct			= CLAMP(0, console1.visible_pct, 1);
	console1.visible_lines			= console1.visible_pct * (float)clheight;
	console1.visible_lines_conscale = console1.visible_lines * conheight_ratio;

	if (clearconsole++ < vid.numpages)
	{
		Sbar_Changed ();
#ifdef WINQUAKE_RENDERER_SUPPORT
		winquake_scr_copytop = 1;
		Draw_SetCanvas (CANVAS_DEFAULT);
		Draw_TileClear (0,(int)console1.visible_lines, clwidth, clheight - (int)console1.visible_lines);
	}
	else if (clearnotify++ < vid.numpages)
	{
		winquake_scr_copytop = 1;
		Draw_SetCanvas (CANVAS_DEFAULT);
		Draw_TileClear (0, 0, clwidth, 4 /*con_notifylines*/);
#endif //WINQUAKE_RENDERER_SUPPORT
	}

#ifdef GLQUAKE_RENDERER_SUPPORT
	if (!console1.forcedup && console1.visible_pct)
		glquake_scr_tileclear_updates = 0; //johnfitz
#endif // GLQUAKE_RENDERER_SUPPORT
}

/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole (void)
{
	if (console1.visible_pct)
	{
#ifdef WINQUAKE_RENDERER_SUPPORT
		winquake_scr_copyeverything = 1;
#endif // WINQUAKE_RENDERER_SUPPORT
		Con_DrawConsole (console1.visible_pct, true);
		clearconsole = 0;
	}
	else
	{
		if (key_dest == key_game || key_dest == key_message)
			if (!cls.titledemo) // Title demos do not get notify text
				Con_DrawNotify ();	// only draw notify in game
	}
}


/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

/*
==================
SCR_ScreenShot_f
==================
*/
void SCR_ScreenShot_f (lparse_t *line)
{
	char basefilename[MAX_QPATH_64];
	char *file_url;
	int     i;

	if (line->count == 2 && strcasecmp(line->args[1], "copy") == 0)
	{
		SCR_ScreenShot_Clipboard_f ();	// Copy to clipboard instead
		return;
	}

	if (line->count == 2) // Explicitly named
	{

		if (strstr(line->args[1], ".."))
		{
			Con_PrintLinef ("Relative pathnames are not allowed.");
			return;
		}
        c_strlcpy (basefilename, line->args[1]);
		File_URL_Edit_Force_Extension (basefilename, ".png", sizeof(basefilename));
		// Might overwrite but that is user responisibility
		goto takeshot;
	}

// find a file name to save it to
	for (i = 0; i < 10000; i++)
	{
		c_snprintf1 (basefilename, ENGINE_SHOT_PREFIX "%04d.png", i);
		file_url = qpath_to_url(basefilename);

		if (!File_Exists (file_url))
			break;	// file doesn't exist
	}

	if (i == 10000)
	{
		Con_PrintLinef ("SCR_ScreenShot_f: Couldn't find an unused filename");
		return;
 	}

takeshot:
// now write the file

#ifdef DIRECT3D9_WRAPPER
	if (vid.direct3d == 9 && (vid_hardwaregamma.value == 0 || (vid_contrast.value == 1 && vid_gamma.value == 1) /* no hardware gamma = ok */))
	{
		char *qpath_to_url (const char *relative_url); // What was the logic in not putting this in common.h?  Because I hate it?
		char *file_url = (char *)qpath_to_url (basefilename);
		File_URL_Edit_Force_Extension (file_url, ".png", CORE_STRINGS_VA_ROTATING_BUFFER_BUFSIZ_1024);


		Direct3D9_ScreenShotPNG (file_url); // This doesn't return success or failure, so we will need to check ourselves.

		// We'll say that if the file exists, it worked.
		if (File_Exists (file_url)) {
			Recent_File_Set_QPath (basefilename);
			Con_PrintLinef ("Wrote %s via DX9, type " QUOTEDSTR("showfile") " to open folder", basefilename);
		}
		else Con_PrintLinef ("SCR_ScreenShot_f: Couldn't create a PNG file");

		return; 
	}
#endif // DIRECT3D9_WRAPPER

	{
		int width, height;
		unsigned *buffer = VID_GetBuffer_RGBA_Malloc (&width, &height, 0 /* 0 = RGBA */); // Baker: Video buffer

		// Baker: The following function assumes it will get write access
		// to the buffer being supplied.  So for WinQuake this means
		// make a copy.
		if (Image_Save_PNG_QPath (basefilename, buffer, width, height))
		{
			Recent_File_Set_QPath (basefilename);
			Con_PrintLinef ("Wrote %s, type " QUOTEDSTR ("showfile") " to open folder", basefilename);
		}
		else Con_PrintLinef ("SCR_ScreenShot_f: Couldn't create a PNG file");
		free (buffer);
	}
}


void SCR_ScreenShot_Clipboard_f (void)
{
	int		width, height;
	unsigned	*buffer = VID_GetBuffer_RGBA_Malloc (&width, &height, 0 /* 1 = RGBA */); // Baker: Always RGBA

	Clipboard_Set_Image (buffer, width, height);
	Con_PrintLinef ("Screenshot to clipboard");
	free (buffer);
}


/*
===============
SCR_BeginLoadingPlaque

================
*/

void SCR_BeginLoadingPlaque_Force_NoTransition (void)
{
// We don't set keydest here or remove console
// Because player could be watching start demos while in menu or in console
// Caller should do that.
// Who calls us?  PlayOpenedDemo (when not a start demo), TimeDemo, CL_NextDemo, Host_Map_f, Host_Connect_f, Single Player New Game
	S_StopAllSounds (true);
	CDAudio_Stop (); // Stop the CD music

	Con_ClearNotify ();
	cl.scr_centertime_off = 0;

	scr_disabled_for_loading = true; scr_disabled_time = realtime;
}

// for live play level to level and reconnect
// where it should draw the loading plaque
void SCR_BeginLoadingPlaque_Force_Transition (void)
{
// Who calls us?  You died.  Or multiplayer level change.  Reconnect when not a demo.
	S_StopAllSounds (true);
	CDAudio_Stop (); // Stop the CD music

	Con_ClearNotify ();
	cl.scr_centertime_off = 0;
	//console1.visible_pct = 0;

	scr_drawloading = true;
#ifdef WINQUAKE_RENDERER_SUPPORT
	winquake_scr_fullupdate = 0;
#endif // WINQUAKE_RENDERER_SUPPORT
	Sbar_Changed ();
	SCR_UpdateScreen ();
	scr_drawloading = false;

	scr_disabled_for_loading = true; scr_disabled_time = realtime;
#ifdef WINQUAKE_RENDERER_SUPPORT
	winquake_scr_fullupdate = 0;
#endif // WINQUAKE_RENDERER_SUPPORT
}


void SCR_BeginLoadingPlaque (void)
{
// Who calls us? Load a game from menu.  Start a multplayer game from menu.
	S_StopAllSounds (true);
	CDAudio_Stop (); // Stop the CD music

	if (cls.state != ca_connected)
		return;
	if (cls.signon != SIGNONS)
		return;

// redraw with no console and the loading plaque
	Con_ClearNotify ();
	cl.scr_centertime_off = 0;
//	console1.visible_pct = 0;

	scr_drawloading = true;
#ifdef WINQUAKE_RENDERER_SUPPORT
	winquake_scr_fullupdate = 0;
#endif // WINQUAKE_RENDERER_SUPPORT
	Sbar_Changed ();
	SCR_UpdateScreen ();
	scr_drawloading = false;

	scr_disabled_for_loading = true; scr_disabled_time = realtime;
#ifdef WINQUAKE_RENDERER_SUPPORT
	winquake_scr_fullupdate = 0;
#endif // WINQUAKE_RENDERER_SUPPORT
}



/*
===============
SCR_EndLoadingPlaque

================
*/
void SCR_EndLoadingPlaque (void)
{
	scr_disabled_for_loading = false;
#ifdef WINQUAKE_RENDERER_SUPPORT
	winquake_scr_fullupdate = 0;
#endif // WINQUAKE_RENDERER_SUPPORT
	Con_ClearNotify ();
}

//=============================================================================

const char	*scr_notifystring;
cbool	scr_drawdialog;

void SCR_DrawNotifyString (void)
{
	const char	*start;
	int		l;
	int		j;
	int		x, y;

	Draw_SetCanvas (CANVAS_MENU); //johnfitz

	start = scr_notifystring;

	y = 200 * 0.35; //johnfitz -- stretched overlays

	do
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (320 - l*8)/2; //johnfitz -- stretched overlays
		for (j=0 ; j<l ; j++, x+=8)
			Draw_Character (x, y, start[j]);

		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

/*
==================
SCR_ModalMessage

Displays a text string in the center of the screen and waits for a Y or N
keypress.
==================
*/
#pragma message ("Baker: Note that because this loop gets control and doesn't run host frame, a connection to a server or connected clients can die here")

int SCR_ModalMessage (const char *text, float timeout, cbool enter_out) //johnfitz -- timeout
{
	double time1, time2; //johnfitz -- timeout

	if (isDedicated)
		return true;

	scr_notifystring = text;

// draw a fresh screen
#ifdef WINQUAKE_RENDERER_SUPPORT
	winquake_scr_fullupdate = 0;
#endif // WINQUAKE_RENDERER_SUPPORT

	scr_drawdialog = true;
	SCR_UpdateScreen ();
	scr_drawdialog = false;

	S_ClearBuffer ();		// so dma doesn't loop current sound

	time1 = System_DoubleTime () + timeout; //johnfitz -- timeout
	time2 = 0.0f; //johnfitz -- timeout

	do
	{
		key_count = -1;		// wait for a key down and up
		System_SendKeyEvents ();
		System_Sleep_Milliseconds (16);
		if (timeout) time2 = System_DoubleTime (); //johnfitz -- zero timeout means wait forever.
	} while (key_lastpress != 'y' &&
		key_lastpress != 'n' &&
		 key_lastpress != K_ESCAPE &&
			 (!enter_out || key_lastpress != K_ENTER) &&
		time2 <= time1);

	// make sure we don't ignore the next keypress
	if (key_count < 0)
		key_count = 0;

#ifdef WINQUAKE_RENDERER_SUPPORT
	winquake_scr_fullupdate = 0;
#endif // WINQUAKE_RENDERER_SUPPORT

	//johnfitz -- timeout
	if (time2 > time1)
		return false;
	//johnfitz

	return key_lastpress == 'y';
}


//=============================================================================



#ifdef WINQUAKE_RENDERER_SUPPORT
cbool SCR_EraseCenterString (void)
{
	if (scr_erase_center++ > vid.numpages)
	{
		scr_erase_lines = 0;
		return false;
	}

	// Perform erasure
	return true;
}


#endif // WINQUAKE RENDERER SUPPORT

/*
==================
SCR_TileClear -- johnfitz -- modified to use clwidth/clheight instead of vid.width/vid.height
                             also fixed the dimensions of right and top panels
							 also added scr_tileclear_updates
==================
*/
void SCR_TileClear (void)
{
#ifdef GLQUAKE_RENDERER_SUPPORT
	if (glquake_scr_tileclear_updates >= vid.numpages
			&& !gl_clear.value
			&& !renderer.isIntelVideo) //intel video workarounds from Baker
		return;

	glquake_scr_tileclear_updates++;
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
	if (winquake_scr_fullupdate >= vid.numpages && !SCR_EraseCenterString())
		return;

	winquake_scr_fullupdate++;

	winquake_scr_copyeverything = 1;	// Baker: It's a full update
	Sbar_Changed ();					// Baker: It's a full update

	Draw_SetCanvas (CANVAS_DEFAULT);
#endif // WINQUAKE_RENDERER_SUPPORT

	if (r_refdef.vrect.x > 0)
	{
		// left
		Draw_TileClear (0,
						0,
						r_refdef.vrect.x,
						clheight - sb_lines);
		// right
		Draw_TileClear (r_refdef.vrect.x + r_refdef.vrect.width,
						0,
						clwidth - r_refdef.vrect.x - r_refdef.vrect.width,
						clheight - sb_lines);
#ifdef WINQUAKE_RENDERER_SUPPORT
		winquake_scr_copytop = 1;
#endif // WINQUAKE_RENDERER_SUPPORT
	}

	if (r_refdef.vrect.y > 0)
	{
		// top
		Draw_TileClear (r_refdef.vrect.x,
						0,
						r_refdef.vrect.width,
						r_refdef.vrect.y);
		// bottom
		Draw_TileClear (r_refdef.vrect.x,
						r_refdef.vrect.y + r_refdef.vrect.height,
						r_refdef.vrect.width,
						clheight - r_refdef.vrect.y - r_refdef.vrect.height - sb_lines);
#ifdef WINQUAKE_RENDERER_SUPPORT
		winquake_scr_copytop = 1;
#endif // WINQUAKE_RENDERER_SUPPORT
	}
}


/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.

WARNING: be very careful calling this from elsewhere, because the refresh
needs almost the entire 256k of stack space!
==================
*/
void SCR_UpdateScreen (void)
{
	static cbool zero_sized	= false;
	

	// Baker:  Dedicated can find its way here
	if (isDedicated)
		return;				// stdout only

	if (vid.Hidden)
		return;		// can't

	if (vid.Minimized)
		return; // Why bother

	if (scr_skipupdate)
		return;

	if (scr_disabled_for_loading)
	{
		if (realtime - scr_disabled_time > 60)
		{
			scr_disabled_for_loading = false;
			Con_PrintLinef ("load failed.");
		}
		else
			return;
	}

	if (!scr_initialized || !console1.initialized)
		return;				// not initialized yet

	
	VID_BeginRendering (&clx, &cly, &clwidth, &clheight);

#ifdef GLQUAKE_SCALED_DRAWING
	// This is the idea place to do a once per frame value set.
	if (!clwidth || !clheight) {
		// Can happen if window loses focuses or never receives focus because we check system messages.
		// Can we fix?  How we fix.  Breakpoint, but how?
		// Happens in rare circumstances regarding command line, no focus, video setup and + command that needs to print that forces screen update
		// I don't know how we could even replicate that with Visual Studio, debug mode starts with focus.
		clwidth		= vid_width.value;
		clheight	= vid_height.value;
		
	}

	if (zero_sized || vid.consize_stale || oldsbarscale != gl_sbarscale.value || oldmenuscale != gl_menuscale.value) {
		int size		    = CLAMP(-1,(int)scr_scaleauto.value,3); // -1, 0, 1, 2, 3
		int text_width		= size > 1 ? 320 : 400;
		int text_height		= size > 1 ? 200 : 320;
		int text_width_num  = clwidth  / text_width;  // 640/320 = 2, 800/320 = 2, 1280/320 = 4
		int text_height_num = clheight / text_height;  // 480/200 = 2, 600/200 = 3, 1024/320 = 3
		int fit_count0		= text_height_num > text_width_num ? text_width_num : text_height_num;//   c_min (text_width_num, text_height_num); // Smaller of 
		int fit_count1		= c_max (1, fit_count0); // Floor of 1.
		int fit_count		= scr_scaleauto.value < 0 ? 1 : fit_count1; // scaleauto -1 means off no matter what
		float conscale_val	= scr_scaleauto.value ? fit_count : gl_conscale.value;
		
		cbool use_conwidth	= scr_scaleauto.value ? false : (gl_conwidth.value > 0); // If scaleauto is on, we don't use.
		// Note: Removed forcing of multiple of 8
		vid.conwidth = use_conwidth ? (int)gl_conwidth.value : (conscale_val > 0) ? (int)(clwidth/conscale_val) : clwidth;
		vid.conwidth = CLAMP (320, vid.conwidth, clwidth);
		if (!clheight || !clwidth) {
			vid.conwidth = 640;
			vid.conheight = 400;
			Con_WarningLinef ("Zero sized clwidth/clheight");
			zero_sized = true; // Hit it again next time.
		} else {
			// Normal operation.
			vid.conheight = vid.conwidth * clheight / clwidth;
			 zero_sized = false;
		}
		vid.consize_stale = false;  // Not just yet any more ... do it below.

		vid.menu_scale = scr_scaleauto.value ? fit_count : gl_menuscale.value;
		oldmenuscale = gl_menuscale.value;
		{
			int unitsize_height	= size == 3 ? 200 : (size == 2 ? 280 : 320);
			int height_count	= clheight / unitsize_height;
			float sbarscale = scr_scaleauto.value ? height_count : gl_sbarscale.value;

			if (size > 0)	vid.sbar_scale = height_count;
			if (size < 0)	vid.sbar_scale = 1;
			if (!size)		vid.sbar_scale = c_max (1, gl_sbarscale.value);
		}
		
		vid.recalc_refdef = true;
	}

	//johnfitz -- added oldsbarscale and oldsbaralpha
	// HERE AUTO
	if (oldsbarscale != gl_sbarscale.value)
	{
		// HERE AUTO
		oldsbarscale = gl_sbarscale.value;
		vid.recalc_refdef = true;
	}

#endif // GLQUAKE_SCALED_DRAWING

	if (oldsbaralpha != scr_sbaralpha.value)
	{
		oldsbaralpha = scr_sbaralpha.value;
		vid.recalc_refdef = true;
	}
	//johnfitz


// check for vid changes
	if (oldfov != scr_fov.value)
	{
		oldfov = scr_fov.value;
		vid.recalc_refdef = true;
	}

	if (oldscreensize != scr_viewsize.value)
	{
		oldscreensize = scr_viewsize.value;
		vid.recalc_refdef = true;
	}

	if (vid.recalc_refdef)
	{
	// something changed, so reorder the screen
		SCR_CalcRefdef ();
	}

//
// do 3D refresh drawing, and then update the screen
//
	SCR_SetUpToDrawConsole ();

	View_RenderView ();

	Draw_Set2D ();

	//FIXME: only call this when needed
	SCR_TileClear ();

	if (scr_drawdialog) //new game confirm
	{
		Sbar_Draw ();
		Draw_FadeScreen ();
		SCR_DrawNotifyString ();
	}
	else if (scr_drawloading) //loading
	{
		SCR_DrawLoading ();
		Sbar_Draw ();
		M_Draw (); // drawloading and draw menu can happen now
	}
	else if (cl.intermission) // == 1 /* && key_dest == key_game*/) //end of level
	{
		// cl.intermission = 1  --->  Monsters X/Y etc.	 (no center string)
		// cl.intermission = 2  --->  Congratualations!   The Chthon .... (text, text)
		// cl.intermission = 3  --->  Cutscene?  (no plaque)
			 if (cl.intermission == 1)   Sbar_IntermissionOverlay ();
		else if (cl.intermission == 2)   Sbar_FinaleOverlay ();

		if (cl.intermission > 1)   SCR_CheckDrawCenterString (); //
		SCR_DrawPause ();
		SCR_DrawConsole ();
		M_Draw ();
	}
	else
	{
#ifdef GLQUAKE_TEXTURE_POINTER
		TexturePointer_Draw ();
#endif // GLQUAKE_TEXTURE_POINTER
		SCR_DrawRam ();
		SCR_DrawNet ();
		SCR_DrawTurtle ();
		SCR_DrawPause ();
		SCR_CheckDrawCenterString (); // CENTER STRING LOCATION.
		SCR_DrawCrosshair ();  // Baker:moved
		Sbar_Draw ();
		SCR_DrawDevStats (); //johnfitz
		SCR_DrawFPS (); // johnfitz
		SCR_DrawConsole ();
#ifdef SUPPORTS_HTTP_DOWNLOAD
		SCR_Draw_Download ();
#endif // SUPPORTS_HTTP_DOWNLOAD

		M_Draw ();
	}

	View_UpdateBlend (); //johnfitz -- V_UpdatePalette cleaned up and renamed

	VID_EndRendering ();
}