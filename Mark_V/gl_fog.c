#ifdef GLQUAKE // Build level define

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
//gl_fog.c -- global and volumetric fog



#include "quakedef.h"





//==============================================================================
//
//  GLOBAL FOG
//
//==============================================================================

static float fog_density;
static float fog_red;
static float fog_green;
static float fog_blue;

static float old_density;
static float old_red;
static float old_green;
static float old_blue;

static float fade_time; //duration of fade
static float fade_done; //time when fade will be done

#if 1 // Fogex
int fogex;  // 1 = extended fog control
static float fogex_start; // Kurok
static float fogex_end;   // Kurok
static float kold_start; // Kurok
static float kold_end;   // Kurok
#endif


static void Fog_ResetLocals (void)
{
	//initially no fog
	fog_density = 0.0;
	old_density = 0.0;
#if 1 // FogEx
	// Kurok starts here
	fogex_start = 50.0;
	kold_start = 0.0;

	fogex_end = 800.0;
	kold_end = 0.0;
	old_red = old_blue = old_green = 0.0,
	fogex = 0;
#endif


	fog_red = fog_green = fog_blue = 0.3 ;
	fade_time = 0.0;
	fade_done = 0.0;
}

#if 1 // FogEx
/*
=============
FogEx_Update

update internal variables for extended fog
=============
*/
static void FogEx_Update (float start, float end, float red, float green, float blue, float time)
{
	// If we are switching fog type reset vars
	if (!fogex)
	{
		Fog_ResetLocals ();
		fogex = 1;
	}
	//save previous settings for fade
	if (time > 0)
	{
		//check for a fade in progress
		if (fade_done > cl.time)
		{
			float f;//, d;

			f = (fade_done - cl.time) / fade_time;
			kold_start = f * kold_start + (1.0 - f) * fogex_start;
			kold_end = f * kold_end + (1.0 - f) * fogex_end;
			old_red = f * old_red + (1.0 - f) * fog_red;
			old_green = f * old_green + (1.0 - f) * fog_green;
			old_blue = f * old_blue + (1.0 - f) * fog_blue;
		}
		else
		{
			kold_start = fogex_start;
			kold_end = fogex_end;
			old_red = fog_red;
			old_green = fog_green;
			old_blue = fog_blue;
		}
	}

	fogex_start = start;
	fogex_end = end;
	fog_red = red;
	fog_green = green;
	fog_blue = blue;
	fade_time = time;
	fade_done = cl.time + time;
}
#endif

/*
=============
Fog_Update

update internal variables
=============
*/
void Fog_Update (float density, float red, float green, float blue, float time)
{
#if 1 // FogEx
	if (fogex)
	{
		Fog_ResetLocals ();
		fogex = 0;
	}
#endif
	//save previous settings for fade
	if (time > 0)
	{
		//check for a fade in progress
		if (fade_done > cl.time)
		{
			float f;

			f = (fade_done - cl.time) / fade_time;
			old_density = f * old_density + (1.0 - f) * fog_density;
			old_red = f * old_red + (1.0 - f) * fog_red;
			old_green = f * old_green + (1.0 - f) * fog_green;
			old_blue = f * old_blue + (1.0 - f) * fog_blue;
		}
		else
		{
			old_density = fog_density;
			old_red = fog_red;
			old_green = fog_green;
			old_blue = fog_blue;
		}
	}

	fog_density = density;
	fog_red = red;
	fog_green = green;
	fog_blue = blue;
	fade_time = time;
	fade_done = cl.time + time;
}

/*
=============
Fog_ParseServerMessage

handle an SVC_FOG message from server
=============
*/
void Fog_ParseServerMessage (void)
{
	float density, red, green, blue, time;

	density = MSG_ReadByte() / 255.0;
	red = MSG_ReadByte() / 255.0;
	green = MSG_ReadByte() / 255.0;
	blue = MSG_ReadByte() / 255.0;
	time = c_max(0.0, MSG_ReadShort() / 100.0);

	Fog_Update (density, red, green, blue, time);
}

#if 1 // FogEx
/*
=============
Fog_FogExCommand_f

handle the 'fogex' console command
=============
*/
void Fog_FogExCommand_f (lparse_t *line)
{
	switch (line->count)
	{
	default:
	case 1:
		Con_PrintLinef ("usage:");
		Con_PrintLinef ("   fogex <fade>");
		Con_PrintLinef ("   fogex <start> <end>");
		Con_PrintLinef ("   fogex <red> <green> <blue>");
		Con_PrintLinef ("   fogex <fade> <red> <green> <blue>");
		Con_PrintLinef ("   fogex <start> <end> <red> <green> <blue>");
		Con_PrintLinef ("   fogex <start> <end> <red> <green> <blue> <fade>");
		if (fogex)
		{
			Con_PrintLinef ("current values:");
			Con_PrintLinef ("   " QUOTEDSTR("start") " is " QUOTED_F, fogex_start);
			Con_PrintLinef ("   " QUOTEDSTR("end") " is " QUOTED_F, fogex_end);
			Con_PrintLinef ("   " QUOTEDSTR("red")" is " QUOTED_F, fog_red);
			Con_PrintLinef ("   " QUOTEDSTR("green") " is " QUOTED_F, fog_green);
			Con_PrintLinef ("   " QUOTEDSTR("blue") " is " QUOTED_F, fog_blue);
			Con_PrintLinef ("   " QUOTEDSTR("fade") " is " QUOTED_F, fade_time);
		} Con_PrintLinef ("Current fog mode is " QUOTEDSTR("classic fog") " (fog).");
/*		Con_PrintLinef ("Note: For backwards compatibility:"
		Con_PrintLinef ("In worldspawn <density> <red> <green> <blue> <start> <end>"
		Con_PrintLinef ("fogex does not use density, but a non-supporting client"
		Con_PrintLinef ("will use this" ); 
		Con_PrintLine ();
					*/
		break;
	case 2: //TEST
		FogEx_Update(fogex_start,
				   fogex_end,
				   fog_red,
				   fog_green,
				   fog_blue,
				   0.0);
		break;
	case 3:
		FogEx_Update(atof(line->args[1]),
				   atof(line->args[2]),
				   fog_red,
				   fog_green,
				   fog_blue,
				   0.0);
		break;
	case 4:
		FogEx_Update(fogex_start,
				   fogex_end,
				   CLAMP(0.0, atof(line->args[1]), 100.0),
				   CLAMP(0.0, atof(line->args[2]), 100.0),
				   CLAMP(0.0, atof(line->args[3]), 100.0),
				   0.0);
		break;
	case 5: //TEST
		FogEx_Update(fogex_start,
				   fogex_end,
				   CLAMP(0.0, atof(line->args[1]), 100.0),
				   CLAMP(0.0, atof(line->args[2]), 100.0),
				   CLAMP(0.0, atof(line->args[3]), 100.0),
				   0.0);
		break;
	case 6:
		FogEx_Update(atof(line->args[1]),
				   atof(line->args[2]),
				   CLAMP(0.0, atof(line->args[3]), 100.0),
				   CLAMP(0.0, atof(line->args[4]), 100.0),
				   CLAMP(0.0, atof(line->args[5]), 100.0),
				   0.0);
		break;
	case 7:
		FogEx_Update(atof(line->args[1]),
				   atof(line->args[2]),
				   CLAMP(0.0, atof(line->args[3]), 100.0),
				   CLAMP(0.0, atof(line->args[4]), 100.0),
				   CLAMP(0.0, atof(line->args[5]), 100.0),
				   0.0);
		break;
	}

	return;
}
#endif

void Fog_FogCommand_f (lparse_t *line)
{
	switch (line->count)
	{
	default:
	case 1:
		Con_PrintLinef ("usage:");
		Con_PrintLinef ("   fog <density>");
		Con_PrintLinef ("   fog <red> <green> <blue>");
		Con_PrintLinef ("   fog <density> <red> <green> <blue>");
#if 1 // FogEx
		if (!fogex)
		{
#endif
			Con_PrintLinef ("current values:");
			Con_PrintLinef ("   " QUOTEDSTR("density") " is " QUOTED_F, fog_density);
			Con_PrintLinef ("   " QUOTEDSTR("red") " is " QUOTED_F, fog_red);
			Con_PrintLinef ("   " QUOTEDSTR("green") " is " QUOTED_F, fog_green);
			Con_PrintLinef ("   " QUOTEDSTR("blue") " is " QUOTED_F, fog_blue);
#if 1 // FogEx
		} else Con_PrintLinef ("current fog mode is " QUOTEDSTR("extended fog") " (fogex).");
#endif
		break;
	case 2:
		Fog_Update(c_max(0.0, atof(line->args[1])),
				   fog_red,
				   fog_green,
				   fog_blue,
				   0.0);
		break;
	case 3: //TEST
		Fog_Update(c_max(0.0, atof(line->args[1])),
				   fog_red,
				   fog_green,
				   fog_blue,
				   atof(line->args[2]));
		break;
	case 4:
		Fog_Update(fog_density,
				   CLAMP(0.0, atof(line->args[1]), 1.0),
				   CLAMP(0.0, atof(line->args[2]), 1.0),
				   CLAMP(0.0, atof(line->args[3]), 1.0),
				   0.0);
		break;
	case 5:
		Fog_Update(c_max(0.0, atof(line->args[1])),
				   CLAMP(0.0, atof(line->args[2]), 1.0),
				   CLAMP(0.0, atof(line->args[3]), 1.0),
				   CLAMP(0.0, atof(line->args[4]), 1.0),
				   0.0);
		break;
	case 6: //TEST
		Fog_Update(c_max(0.0, atof(line->args[1])),
				   CLAMP(0.0, atof(line->args[2]), 1.0),
				   CLAMP(0.0, atof(line->args[3]), 1.0),
				   CLAMP(0.0, atof(line->args[4]), 1.0),
				   atof(line->args[5]));
		break;
	}
}

/*
=============
Fog_ParseWorldspawn

called at map load
=============
*/


static void Fog_ParseWorldspawn (void)
{
	const char *key = NULL;

	// Parse the fog out of this.
	if ((key = COM_CL_Worldspawn_Value_For_Key (cl.worldmodel->entities, "beta_fogex"))) {
		fogex = 1;
		sscanf(key, "%f %f %f %f %f", &fogex_start, &fogex_end, &fog_red, &fog_green, &fog_blue);
	} else if ((key = COM_CL_Worldspawn_Value_For_Key (cl.worldmodel->entities, "fog"))) {
		sscanf(key, "%f %f %f %f", &fog_density, &fog_red, &fog_green, &fog_blue);
	}

	if (key)
		c_strlcpy (level.fog_key, key);

	if ((key = COM_CL_Worldspawn_Value_For_Key (cl.worldmodel->entities, "skyfog"))) {
		// Set cvar first to avoid flag clearing.
		Cvar_SetQuick (&gl_skyfog, va("%f", gl_skyfog.value) );  // Dumb but effective.  The extra zeros means next set will not string match, triggering the change action.
		level.is_skyfog = true, level.skyfog = CLAMP(0, atof(key), 1);

	}

}

/*
=============
Fog_GetColor

calculates fog color for this frame, taking into account fade times
=============
*/
// Baker: Sky needs this.  To fog the sky correctly.
float *Fog_GetColor (float *startdist, float *enddist)
{
	static float c[4];
	float f, s, e;
	int i;

#if 1
	if (fogex)
	{
		if (fade_done > cl.time) // Kurok
		{
			f = (fade_done - cl.time) / fade_time;
			s = f * kold_start + (1.0 - f) * fogex_start;
			e = f * kold_end + (1.0 - f) * fogex_end;
			c[0] = f * old_red + (1.0 - f) * fog_red * 0.01;
			c[1] = f * old_green + (1.0 - f) * fog_green * 0.01;
			c[2] = f * old_blue + (1.0 - f) * fog_blue * 0.01;
			c[3] = 1.0;
		}
		else
		{
			s = fogex_start;
			e = fogex_end;
			c[0] = fog_red * 0.01;
			c[1] = fog_green * 0.01;
			c[2] = fog_blue * 0.01;
			c[3] = 1.0;
		}
		if (startdist) 	*startdist 	= s;
		if (enddist) 	*enddist 	= e;
	}
	else
#endif
	{
		if (fade_done > cl.time)
		{
			f = (fade_done - cl.time) / fade_time;
			c[0] = f * old_red + (1.0 - f) * fog_red;
			c[1] = f * old_green + (1.0 - f) * fog_green;
			c[2] = f * old_blue + (1.0 - f) * fog_blue;
			c[3] = 1.0;
		}
		else
		{
			c[0] = fog_red;
			c[1] = fog_green;
			c[2] = fog_blue;
			c[3] = 1.0;
		}
	}

	//find closest 24-bit RGB value, so solid-colored sky can match the fog perfectly
	for (i=0;i<3;i++)
		c[i] = (float)(c_rint(c[i] * 255)) / 255.0f;


	return c;
}

/*
=============
Fog_GetDensity

returns current density of fog
=============
*/
// Baker: non mtex uses this
float Fog_GetDensity (void)
{
	float f;

#if 1
	if (fogex)
	{
		if (fade_done > cl.time)
		{
			f = (fade_done - cl.time) / fade_time;
			return f * kold_end + (1.0 - f) * fogex_end;
		}
		return fogex_end;
	}


#endif
	if (fade_done > cl.time)
	{
		f = (fade_done - cl.time) / fade_time;
		return f * old_density + (1.0 - f) * fog_density;
	}
	else
		return fog_density;
}

/*
=============
Fog_SetupFrame

called at the beginning of each frame
=============
*/
void Fog_SetupFrame (void)
{
	float startfogdist;
	float endfogdist;


	eglFogfv(GL_FOG_COLOR, Fog_GetColor(&startfogdist, &endfogdist));
#if 1
	if (fogex)
	{
		if (startfogdist > endfogdist)
			c_swapf (&startfogdist, &endfogdist);
		eglFogi(GL_FOG_MODE, GL_LINEAR);
		eglFogf(GL_FOG_START, startfogdist);
		eglFogf(GL_FOG_END, endfogdist);
//	Con_PrintLinef ("se %g %g", startfogdist, endfogdist);
/*
		eglFogf(GL_FOG_START, fogex_start);
		eglFogf(GL_FOG_END, fogex_end);
		eglFogf(GL_FOG_DENSITY, 0.10);

cvar_t gl_fogenable = {"gl_fogenable", "0"};
cvar_t gl_fogstart = {"gl_fogstart", "50.0"};
cvar_t gl_fogend = {"gl_fogend", "800.0"};
cvar_t gl_fogdensity = {"gl_fogdensity", "0.8"};
cvar_t gl_fogred = {"gl_fogred","0.6"};
cvar_t gl_foggreen = {"gl_foggreen","0.5"};
cvar_t gl_fogblue = {"gl_fogblue","0.4"};
cvar_t gl_fogalpha = {"gl_fogalpha", "0.5"};
		eglFogi(GL_FOG_MODE, GL_LINEAR);
//		eglFogf(GL_FOG_START, startfogdist);
//		eglFogf(GL_FOG_END, endfogdist);
		eglFogf(GL_FOG_START, 50);
		eglFogf(GL_FOG_END, 800);
		eglFogf(GL_FOG_DENSITY, 0.8);
		{
			rgba4_t fogc = {0.5, 0.4, 0.5};
			eglFogfv(GL_FOG_COLOR, fogc);
		}
		*/
	}
	else
#endif

	{
		eglFogi(GL_FOG_MODE, GL_EXP2);
		eglFogf(GL_FOG_DENSITY, Fog_GetDensity() / 64.0);
	}
}

/*
=============
Fog_EnableGFog

called before drawing stuff that should be fogged
=============
*/
void Fog_EnableGFog (void)
{
	if (Fog_GetDensity() > 0)
		eglEnable(GL_FOG);
}

/*
=============
Fog_DisableGFog

called after drawing stuff that should be fogged
=============
*/
void Fog_DisableGFog (void)
{
	if (Fog_GetDensity() > 0)
		eglDisable(GL_FOG);
}

/*
=============
Fog_StartAdditive

called before drawing stuff that is additive blended -- sets fog color to black
=============
*/
// Baker: FitzQuake uses this for non-Mtex pathways
void Fog_StartAdditive (void)
{
	vec3_t color = {0,0,0};

	if (Fog_GetDensity() > 0)
		eglFogfv(GL_FOG_COLOR, color);
}

/*
=============
Fog_StopAdditive

called after drawing stuff that is additive blended -- restores fog color
=============
*/
// Baker: FitzQuake uses this for non-Mtex pathways
void Fog_StopAdditive (void)
{
	if (Fog_GetDensity() > 0)
		eglFogfv(GL_FOG_COLOR, Fog_GetColor(NULL, NULL));
}

//==============================================================================
//
//  VOLUMETRIC FOG
//
//==============================================================================

//cvar_t r_vfog = {"r_vfog", "1", CVAR_NONE}; // Baker: unused

//void Fog_DrawVFog (void){}  // Baker: unused
//void Fog_MarkModels (void){}  // Baker: unused

//==============================================================================
//
//  INIT
//
//==============================================================================

/*
=============
Fog_NewMap

called whenever a map is loaded
=============
*/
void Fog_NewMap (void)
{
	Fog_ResetLocals ();
	Fog_ParseWorldspawn (); //for global fog
//	Fog_MarkModels (); //for volumetric fog (Baker: Unused)
}

/*
=============
Fog_Init

called when quake initializes
=============
*/
void Fog_Init (void)
{
	Cmd_AddCommands (Fog_Init);
	// Baker: Fog resetting done elsewhere

	//Cvar_RegisterVariable (&r_vfog);
}


#endif // GLQUAKE specific