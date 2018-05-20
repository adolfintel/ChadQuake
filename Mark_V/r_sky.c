#ifndef GLQUAKE // WinQuake Software renderer

/*
Copyright (C) 1996-1997 Id Software, Inc.
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
// r_sky.c

#include "quakedef.h" // Baker: It's just sky stuff obviously


int		iskyspeed = 8;
int		iskyspeed2 = 2;
float	skyspeed, skyspeed2;

float		skytime;


// TODO: clean up these routines

byte	*skyunderlay, *skyoverlay; // Manoel Kasimier - smooth sky
byte	bottomalpha[128*131]; // Manoel Kasimier - translucent sky

// Manoel Kasimier - skyboxes - begin
// Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
extern	mtexinfo_t		r_skytexinfo[SKYBOX_SIDES_COUNT_6];
//extern	cbool		r_drawskybox;
byte					r_skypixels[SKYBOX_SIDES_COUNT_6][SKYBOX_MAX_SIZE*SKYBOX_MAX_SIZE]; // Manoel Kasimier - edited
texture_t				r_skytextures[SKYBOX_SIDES_COUNT_6];
char					skybox_name[MAX_QPATH_64]; //name of current skybox, or "" if no skybox
char					last_skybox_name[MAX_QPATH_64];


//==============================================================================
//
//  INIT
//
//==============================================================================

/*
=============
Sky_LoadTexture

A sky texture is 256*128, with the left side being a masked overlay
==============
*/
void Sky_LoadTexture (texture_t *mt)
{
	// Baker: Warn.
	if (mt->width != 256 || mt->height != 128) // Leave this.
		Con_WarningLinef ("Standard sky texture %s expected to be 256 x 128 but is %d by %d ", mt->name, mt->width, mt->height);

	skyoverlay = (byte *)mt + mt->offsets[0]; // Manoel Kasimier - smooth sky
	skyunderlay = skyoverlay+128; // Manoel Kasimier - smooth sky
}

///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////

cbool R_LoadSkybox (const char *name);

/*
==================
Sky_LoadSkyBox
==================
*/
void Sky_LoadSkyBox (const char *name)
{
	if (strcmp (skybox_name, name) == 0)
		return; //no change

	// turn off skybox if sky is set to ""
	if (name[0] == 0)
	{
		// If there is a user sky, load that now, otherwise clear
		if (!cl_sky.string[0])
		{
			// No user sky box, so clear the skybox
			skybox_name[0] = 0;
			return;
		}

		// User has a skybox, load that and continue ...
		name = cl_sky.string;
	}

	// Baker: If name matches, we already have the pixels loaded and we don't
	// actually need to reload
	if (strcmp (name, last_skybox_name))
	{
		if (cl.worldmodel && !R_LoadSkybox (name))
		{
			skybox_name[0] = 0;
			if (name == cl_sky.string)
				Cvar_SetQuick (&cl_sky, ""); // If the user chose it, nuke it
			return;
		}
	}


	c_strlcpy(skybox_name, name);
	c_strlcpy(last_skybox_name, skybox_name);

}

///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////


////////////////////////////////////////////////
////////////////////////////////////////////////
////////////////////////////////////////////////
////////////////////////////////////////////////
////////////////////////////////////////////////
////////////////////////////////////////////////
////////////////////////////////////////////////
////////////////////////////////////////////////

cbool R_LoadSkybox (const char *name)
{
	int		i;
	char	pathname[MAX_QPATH_64];
	byte	*pic;
	char	*suf[SKYBOX_SIDES_COUNT_6] = {"rt", "bk", "lf", "ft", "up", "dn"};
	int		r_skysideimage[SKYBOX_SIDES_COUNT_6] = {5, 2, 4, 1, 0, 3};
	int		width, height;
	int		mark;


	if (!name || !name[0])
	{
		skybox_name[0] = 0;
		return false;
	}

	// the same skybox we are using now
	if (!strcmp (name, skybox_name))
		return true;

	c_strlcpy (skybox_name, name);



	mark = Hunk_LowMark ();

	Image_Quick_Palette_256_Alloc (vid.altblack);

	for (i = 0 ; i < SKYBOX_SIDES_COUNT_6 ; i++)
	{
		c_snprintf2 (pathname, "gfx/env/%s%s", skybox_name, suf[r_skysideimage[i]]);
		pic = Image_Load_Convert_RGBA_To_Palette (pathname, &width, &height, vid.basepal);

		if (!pic)
		{
			Con_PrintLinef ("Couldn't load %s", pathname);
			return false;
		}
		// Manoel Kasimier - hi-res skyboxes - begin
		switch (width)
		{
		case 1024:	// falls through
		case 512:	// falls through
		case 256:
			// We're good!
			break;
		default:
			// We aren't good
			Con_PrintLinef ("skybox width (%d) for %s must be 256, 512, 1024", width, pathname);
			Hunk_FreeToLowMark (mark);
			return false;
		}

		switch (height)
		{
		case 1024:	// falls through
		case 512:	// falls through
		case 256:
			// We're good!
			break;
		default:
			Con_PrintLinef ("skybox height (%d) for %s must be 256, 512, 1024", height, pathname);
			Hunk_FreeToLowMark (mark);
			return false;
		}
		// Manoel Kasimier - hi-res skyboxes - end

		r_skytexinfo[i].texture = &r_skytextures[i];
		r_skytexinfo[i].texture->width = width; // Manoel Kasimier - hi-res skyboxes - edited
		r_skytexinfo[i].texture->height = height; // Manoel Kasimier - hi-res skyboxes - edited
		r_skytexinfo[i].texture->offsets[0] = i;

		// Manoel Kasimier - hi-res skyboxes - begin
		{
			extern vec3_t box_vecs[SKYBOX_SIDES_COUNT_6][2];
			extern vec3_t box_bigvecs[SKYBOX_SIDES_COUNT_6][2];
			extern vec3_t box_bigbigvecs[SKYBOX_SIDES_COUNT_6][2];
			extern msurface_t *r_skyfaces;

			switch (width)
			{
			case 1024:	VectorCopy (box_bigbigvecs[i][0], r_skytexinfo[i].vecs[0]); break;
			case 512:	VectorCopy (box_bigvecs[i][0], r_skytexinfo[i].vecs[0]); break;
			default:	VectorCopy (box_vecs[i][0], r_skytexinfo[i].vecs[0]); break;
			}

			switch (height)
			{
			case 1024:	VectorCopy (box_bigbigvecs[i][1], r_skytexinfo[i].vecs[1]); break;
			case 512:	VectorCopy (box_bigvecs[i][1], r_skytexinfo[i].vecs[1]); break;
			default:	VectorCopy (box_vecs[i][1], r_skytexinfo[i].vecs[1]); break;
			}

			// This is if one is already loaded and the size changed
			if (r_skyfaces)
			{
				r_skyfaces[i].texturemins[0] = -(width/2);
				r_skyfaces[i].texturemins[1] = -(height/2);
				r_skyfaces[i].extents[0] = width;
				r_skyfaces[i].extents[1] = height;
			} 
			else Con_DPrintLinef ("Warning: No surface to load yet for WinQuake skybox");

		}
		// Manoel Kasimier - hi-res skyboxes - end
		memset (&r_skypixels[i], 0, SKYBOX_MAX_SIZE * SKYBOX_MAX_SIZE); // Baker: Nuke it
		memcpy (r_skypixels[i], pic, width*height); // Manoel Kasimier - hi-res skyboxes - edited
		Hunk_FreeToLowMark (mark);
	}

	Hunk_FreeToLowMark (mark);

	Image_Quick_Palette_256_Free ();
	return true;
}


/*
=============
R_SetSkyFrame
==============
*/
void R_SetSkyFrame (void)
{
	int		g, s1, s2;
	float	temp;

	skyspeed = iskyspeed;
	skyspeed2 = iskyspeed2;

	g = GreatestCommonDivisor (iskyspeed, iskyspeed2);
	s1 = iskyspeed / g;
	s2 = iskyspeed2 / g;
	temp = SKYSIZE * s1 * s2;

	skytime = cl.ctime - ((int)(cl.ctime / temp) * temp);
}

#endif // !GLQUAKE - WinQuake Software renderer