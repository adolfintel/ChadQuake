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
// location.c -- Loads locations from a text file (ProQuake format)

#include "quakedef.h"



#define MAX_LOCATIONS 64
typedef struct
{
	vec3_t mins_corner;		// min xyz corner
	vec3_t maxs_corner;		// max xyz corner
	vec_t sd;				// sum of dimensions
	char name[32];
} location_t;

static location_t	locations[MAX_LOCATIONS];
static int			numlocations = 0;

/*
===============
LOC_LoadLocations

Load the locations for the current level from the location file
===============
*/
// Returns a NULL on success.  A string of expected qpath on failure for download to use if it chooses.
const char *LOC_LoadLocations (void)
{
	FILE *f;
	char		*ch;
	static char	locs_filename[MAX_QPATH_64];
	char		base_map_name[MAX_QPATH_64];
	char		buff[MAX_OSPATH];
	location_t *thisloc;
	int i;
	float temp;

	numlocations = 0;
	File_URL_Copy_StripExtension (base_map_name, File_URL_SkipPath(cl.worldmodel->name) /* skip "maps/" */, sizeof(base_map_name));
	c_snprintf1 (locs_filename, "locs/%s.loc", base_map_name);

	if (COM_FOpenFile_Limited (locs_filename, &f, cl.worldmodel->loadinfo.searchpath) == -1)
		return locs_filename;  // Failed

	thisloc = locations;
	while (!feof(f) && numlocations < MAX_LOCATIONS)
	{
		if (fscanf(f, "%f, %f, %f, %f, %f, %f, ", &thisloc->mins_corner[0], &thisloc->mins_corner[1], &thisloc->mins_corner[2], &thisloc->maxs_corner[0], &thisloc->maxs_corner[1], &thisloc->maxs_corner[2]) == 6)
		{
			thisloc->sd = 0;
			for (i = 0 ; i < 3 ; i++)
			{
				if (thisloc->mins_corner[i] > thisloc->maxs_corner[i])
				{
					temp = thisloc->mins_corner[i];
					thisloc->mins_corner[i] = thisloc->maxs_corner[i];
					thisloc->maxs_corner[i] = temp;
				}
				thisloc->sd += thisloc->maxs_corner[i] - thisloc->mins_corner[i];
			}
			thisloc->mins_corner[2] -= 32.0;
			thisloc->maxs_corner[2] += 32.0;
			fgets(buff, 256, f);

			ch = strrchr(buff, '\n');	if (ch)		*ch = 0;	// Eliminate trailing newline characters
			ch = strrchr(buff, '\"');	if (ch)		*ch = 0;	// Eliminate trailing quotes

			for (ch = buff ; *ch == ' ' || *ch == '\t' || *ch == '\"' ; ch++);	// Go through the string and forward past any spaces, tabs or double quotes to find start of the name

			c_strlcpy (thisloc->name, ch);
			thisloc = &locations[++numlocations];
		}
		else
			fgets(buff, 256, f);
	}

	FS_fclose(f);
	return NULL;
}

/*
===============
LOC_GetLocation

Get the name of the location of a point
===============
*/
// return the nearest rectangle if you aren't in any (manhattan distance)
char *LOC_GetLocation (vec3_t worldposition)
{
	location_t *thisloc;
	location_t *bestloc;
	float dist, bestdist;

	if (numlocations == 0)
		return "(Not loaded)";


	bestloc = NULL;
	bestdist = 999999;
	for (thisloc = locations ; thisloc < locations + numlocations ; thisloc++)
	{
		dist =	fabs(thisloc->mins_corner[0] - worldposition[0]) + fabs(thisloc->maxs_corner[0] - worldposition[0]) +
				fabs(thisloc->mins_corner[1] - worldposition[1]) + fabs(thisloc->maxs_corner[1] - worldposition[1]) +
				fabs(thisloc->mins_corner[2] - worldposition[2]) + fabs(thisloc->maxs_corner[2] - worldposition[2]) - thisloc->sd;

		if (dist < .01)
			return thisloc->name;

		if (dist < bestdist)
		{
			bestdist = dist;
			bestloc = thisloc;
		}
	}
	if (bestloc)
		return bestloc->name;
	return "somewhere";
}




