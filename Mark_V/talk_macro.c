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
// talk_macro.c -- Translates simple macros in talk like %h into health ("say I have %h health" --> "I have 64 health")

#include "quakedef.h"


static char *Weapons_String (void)
{
	static char weapons_string[256];

	memset (weapons_string, 0, sizeof(weapons_string));

	c_strlcat (weapons_string, "(");
	if (cl.items & IT_ROCKET_LAUNCHER)  c_strlcat (weapons_string, "RL ");
	if (cl.items & IT_GRENADE_LAUNCHER) c_strlcat (weapons_string, "GL ");
	if (cl.items & IT_LIGHTNING)  		c_strlcat (weapons_string, "LG ");

	if (strlen (weapons_string) == 1) // No good weapons.
	{
		if (cl.items & IT_SUPER_NAILGUN)  	c_strlcat (weapons_string, "SNG ");
		if (cl.items & IT_SUPER_SHOTGUN)  	c_strlcat (weapons_string, "SSG ");
		if (cl.items & IT_SUPER_NAILGUN)  	c_strlcat (weapons_string, "NG ");
	}

	// Not even bad weapons
	if (strlen (weapons_string) == 1) 		c_strlcat (weapons_string, "none");

	c_strlcat (weapons_string, ")");

	return weapons_string;
}

static char *Powerups_String (void)
{
	static char powerups_string[256];

	memset (powerups_string, 0, sizeof(powerups_string));

	if (cl.items & IT_QUAD)				c_strlcat (powerups_string, "[QUAD] ");
	if (cl.items & IT_INVULNERABILITY)  c_strlcat (powerups_string, "[PENT] ");
	if (cl.items & IT_INVISIBILITY)		c_strlcat (powerups_string, "[RING] ");

	return powerups_string;
}

static char *Time_String (void)
{
	static char level_time_string[256];
	int minutes = cl.time / 60;
	int seconds = cl.time - (60 * minutes);
	minutes &= 511;

	memset (level_time_string, 0, sizeof(level_time_string));

	c_snprintf2 (level_time_string, "%d:%02d", minutes, seconds);

	return level_time_string;
}


const char *Talk_Macros_Expand (const char *string)
{
	static char modified_string[256];
	int		readpos = 0;
	int		writepos = 0;
	char	*insert_point;
	char	letter;
//	int		i, match;

	memset (modified_string, 0, sizeof(modified_string));

	// Byte by byte copy cmd_args into the buffer.

	for ( ; string[readpos] && writepos < 100; )
	{
		if (string[readpos] != '%' || !string[readpos+1] || strchr("acdhlprtw", string[readpos + 1] ) == NULL)
		{
			// Not a macro to replace or some invalid macro
			modified_string[writepos] = string[readpos];
			readpos ++;
			writepos ++;
			continue;
		}

		// We found a macro
		readpos ++; // Skip the percent
		letter = string[readpos];
		readpos ++; // Skip writing the letter too
		insert_point = &modified_string[writepos];

		// Baker: Can't reliably kill these sprintfs because c_snprintf doesn't return in the ISO fashion
			 if (letter == 'a')  writepos += sprintf(insert_point, "%d", cl.stats[STAT_ARMOR]);
		else if (letter == 'c')  writepos += sprintf(insert_point, "%d", cl.stats[STAT_CELLS]);
		else if (letter == 'd')  writepos += sprintf(insert_point, "%s", LOC_GetLocation(cl.death_location));
		else if (letter == 'h')  writepos += sprintf(insert_point, "%d", cl.stats[STAT_HEALTH]);
		else if (letter == 'l')  writepos += sprintf(insert_point, "%s", LOC_GetLocation(cl_entities[cl.viewentity_player].origin));
		else if (letter == 'p')	 writepos += sprintf(insert_point, "%s", Powerups_String (/*cl.items*/));
		else if (letter == 'r')  writepos += sprintf(insert_point, "%d", cl.stats[STAT_ROCKETS]);
		else if (letter == 't')  writepos += sprintf(insert_point, "%s", Time_String (/*cl.time*/));
		else if (letter == 'w')  writepos += sprintf(insert_point, "%s", Weapons_String (/*cl.items*/));
		else					 writepos += sprintf(insert_point, "(invalid macro '%c')", letter); // This should be unreachable
	}

	modified_string[writepos] = 0; // Null terminate the copy
	return modified_string;
}

