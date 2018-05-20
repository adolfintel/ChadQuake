/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2011 O.Sezer
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
// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"


qmodel_t	*loadmodel;
char	loadname[MAX_QPATH_64];	// for hunk tags

void Mod_LoadSpriteModel (qmodel_t *mod, void *buffer);
void Mod_LoadBrushModel (qmodel_t *mod, void *buffer);
void Mod_LoadAliasModel (qmodel_t *mod, void *buffer);
qmodel_t *Mod_LoadModel (qmodel_t *mod, cbool crash);

byte	mod_novis[MAX_MAP_LEAFS/8];

#define	MAX_MOD_KNOWN	2048 /*johnfitz -- was 512 */
qmodel_t	mod_known[MAX_MOD_KNOWN];
int		mod_numknown;

#ifdef WINQUAKE_RENDERER_SUPPORT
// values for qmodel_t's needload
#define NL_PRESENT		0
#define NL_NEEDS_LOADED	1
#define NL_UNREFERENCED	2
#endif // WINQUAKE_RENDERER_SUPPORT

#ifdef GLQUAKE_RENDERER_SUPPORT
texture_t	*r_notexture_mip; //johnfitz -- moved here from r_main.c
texture_t	*r_notexture_mip2; //johnfitz -- used for non-lightmapped surfs with a missing texture
#endif // GLQUAKE_RENDERER_SUPPORT




/*
=======================
Mod_Flags_Refresh_f -- johnfitz -- called when r_nolerp_list or other list cvar changes
=======================
*/
void Mod_Flags_Refresh_f (cvar_t *var)
{
	int i;
	for (i=0; i < MAX_FITZQUAKE_MODELS; i++)
		Mod_SetExtraFlags (cl.model_precache[i]);


}

/*
=================
Mod_SetExtraFlags -- johnfitz -- set up extra flags that aren't in the mdl
=================
*/
void Mod_SetExtraFlags (qmodel_t *mod)
{
	if (!mod || !mod->name || mod->type != mod_alias)
		return;

	mod->modelflags &= (0xFF | EF_ALPHA_MASKED_MDL); //only preserve first byte (and now preserve EF_ALPHA_MASKED_MDL also as not a list element but real model flag)

	// nolerp flag
	if (COM_ListMatch (r_nolerp_list.string, mod->name) == true)
		mod->modelflags |= MOD_NOLERP;

#ifdef GLQUAKE_RENDERER_SUPPORT
	if (COM_ListMatch (gl_noshadow_list.string, mod->name) == true)
		mod->modelflags |= MOD_NOSHADOW;

	if (COM_ListMatch (gl_fbrighthack_list.string, mod->name) == true)
		mod->modelflags |= MOD_FBRIGHTHACK;


	if (COM_ListMatch (gl_nocolormap_list.string, mod->name) == true)
		mod->modelflags |= MOD_NOCOLORMAP;
#endif // GLQUAKE_RENDERER_SUPPORT

	// nolerp flag
//	if (COM_ListMatch ("progs/player.mdl", mod->name) == true)
//		mod->flags |= MOD_PLAYER;

	if (COM_ListMatch ("progs/eyes.mdl", mod->name) == true)
		mod->modelflags |= MOD_EYES;

	if (COM_ListMatch (sv_filter_gibs_list.string, mod->name) == true)
		mod->modelflags |= MOD_GIBS;

}


/*
===============
Mod_Extradata

Caches the data if needed
===============
*/
void *Mod_Extradata (qmodel_t *mod)
{
	void	*r;

	r = Cache_Check(&mod->cache);
	if (r)
		return r;

	Mod_LoadModel (mod, true);

	if (!mod->cache.data)
		Host_Error ("Mod_Extradata: caching failed");
	return mod->cache.data;
}

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t *Mod_PointInLeaf (vec3_t p, qmodel_t *model)
{
	mnode_t		*node;
	float		d;
	mplane_t	*plane;

	if (!model || !model->nodes)
		Host_Error ("Mod_PointInLeaf: bad model");

	node = model->nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mleaf_t *)node;
		plane = node->plane;
		d = DotProduct (p, plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL;	// never reached
}


/*
===================
Mod_DecompressVis
===================
*/
byte *Mod_DecompressVis (byte *in, qmodel_t *model)
{
	static byte	decompressed[MAX_MAP_LEAFS/8];
	int		c;
	byte	*out;
	int		row;

	row = (model->numleafs+7)>>3;
	out = decompressed;

#if 0
	memcpy (out, in, row);
#else
	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
#endif

	return decompressed;
}

byte *Mod_LeafPVS (mleaf_t *leaf, qmodel_t *model)
{
	if (leaf == model->leafs)
		return mod_novis;
	return Mod_DecompressVis (leaf->compressed_vis, model);
}

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll (void)
{
	int		i;
	qmodel_t	*mod;

	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
#ifdef GLQUAKE_RENDERER_SUPPORT
		if (mod->type != mod_alias)
		{
			mod->needload = true;
			TexMgr_FreeTexturesForOwner (mod); //johnfitz
		}
#endif // GLQUAKE_RENDERER_SUPPORT
#ifdef WINQUAKE_RENDERER_SUPPORT
		mod->needload = NL_UNREFERENCED;
//FIX FOR CACHE_ALLOC ERRORS:
		if (mod->type == mod_sprite)
			mod->cache.data = NULL;
#endif // WINQUAKE_RENDERER_SUPPORT
	}
}

/*
===================
Mod_ClearAll_Compact
===================
*/
void Mod_ClearAll_Compact (void)
{
#ifdef GLQUAKE_RENDERER_SUPPORT
	size_t	bufsize = sizeof(qmodel_t) * MAX_MOD_KNOWN;
	qmodel_t*  new_mod_known = calloc (sizeof(qmodel_t), MAX_MOD_KNOWN );
	int		i, newcount = 0;
	qmodel_t	*mod;

	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (mod->type != mod_alias)
		{
			mod->needload = true;
			TexMgr_FreeTexturesForOwner (mod); //johnfitz
		}

		if (mod->cache.data)
		{
			qmodel_t	*old_mod = &mod_known[i];
			qmodel_t	*new_mod = &new_mod_known[newcount];
			// Transfer it
			memcpy (new_mod, old_mod, sizeof(qmodel_t));
			newcount ++;
		}
	}

	memset (mod_known, 0, sizeof(mod_known));
	memcpy (mod_known, new_mod_known, sizeof(mod_known));

	mod_numknown = newcount;
	free (new_mod_known);
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
	Mod_ClearAll (); // Baker: I'm not sure software needs this.
#endif // WINQUAKE_RENDERER_SUPPORT
}

/*
==================
Mod_FindName

==================
*/
qmodel_t *Mod_FindName (const char *name)
{
	int		i;
	qmodel_t	*mod;
#ifdef WINQUAKE_RENDERER_SUPPORT
	qmodel_t	*avail = NULL;
#endif // WINQUAKE_RENDERER_SUPPORT
	if (!name[0])
		Host_Error ("Mod_FindName: NULL name"); //johnfitz -- was "Mod_ForName"

//
// search the currently loaded models
//
	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (!strcmp (mod->name, name) )
			break;
#ifdef WINQUAKE_RENDERER_SUPPORT
		if (mod->needload == NL_UNREFERENCED)
			if (!avail || mod->type != mod_alias)
				avail = mod;
#endif // WINQUAKE_RENDERER_SUPPORT
	}

	if (i == mod_numknown)
	{
		if (mod_numknown == MAX_MOD_KNOWN)
		{
#ifdef GLQUAKE_RENDERER_SUPPORT
			Host_Error ("mod_numknown == MAX_MOD_KNOWN");
		}

		mod->needload = true;

		mod_numknown++;
#endif // GLQUAKE_RENDERER_SUPPORT
#ifdef WINQUAKE_RENDERER_SUPPORT
			if (avail)
			{
				mod = avail;
				if (mod->type == mod_alias)
					if (Cache_Check (&mod->cache))
						Cache_Free (&mod->cache, false /* don't free textures, this is WinQuake anyways! */ );
			}
			else
			{
				Host_Error ("mod_numknown == MAX_MOD_KNOWN");
			}
		}
		else
		{
			mod_numknown++;
		}

		mod->needload = NL_NEEDS_LOADED;
#endif // WINQUAKE_RENDERER_SUPPORT

		strlcpy (mod->name, name, MAX_QPATH_64);
	}

	return mod;
}

/*
==================
Mod_TouchModel

==================
*/
void Mod_TouchModel (const char *name)
{
	qmodel_t	*mod;

	mod = Mod_FindName (name);

#ifdef GLQUAKE_RENDERER_SUPPORT
	if (!mod->needload)
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
	if (mod->needload == NL_PRESENT)
#endif // WINQUAKE_RENDERER_SUPPORT
	{
		if (mod->type == mod_alias)
			Cache_Check (&mod->cache);
	}
}

/*
==================
Mod_LoadModel

Loads a model into the cache
==================
*/
qmodel_t *Mod_LoadModel (qmodel_t *mod, cbool crash)
{
	byte	*buf;
	byte	stackbuf[1024];		// avoid dirtying the cache heap
	int	mod_type;

#ifdef GLQUAKE_RENDERER_SUPPORT
	if (!mod->needload)
	{
		if (mod->type == mod_alias)
		{
			if (Cache_Check (&mod->cache))
				return mod;
			}
		else return mod;		// not cached at all
	}
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
	if (mod->type == mod_alias)
	{
		if (Cache_Check (&mod->cache))
		{
			mod->needload = NL_PRESENT;

			return mod;
		}
	}
	else
	{
		if (mod->needload == NL_PRESENT)
			return mod;
	}
#endif // WINQUAKE_RENDERER_SUPPORT

//
// because the world is so huge, load it one piece at a time
//
	if (!crash)
	{

	}

//
// load the file
//
	buf = (byte *)COM_LoadStackFile (mod->name, stackbuf, sizeof(stackbuf));
	if (!buf)
	{
		if (crash)
			Host_Error ("Mod_LoadModel: %s not found", mod->name); //johnfitz -- was "Mod_NumForName"
		return NULL;
	}

	// Baker: You are a horrible person.
	if (com_filesrcpak == 3) {
		mod->is_original_flame_mdl = true;
	}

//
// allocate a new model
//
	COM_FileBase (mod->name, loadname, sizeof(loadname));
//	Con_PrintLinef ("Loadname is %s", loadname);
	loadmodel = mod;

	// Update the path
	c_strlcpy (mod->loadinfo.searchpath, com_filepath);

//
// fill it in
//

// call the appropriate loader
	mod->needload = false;

	mod_type = (buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24));
	switch (mod_type)
	{
	case IDPOLYHEADER:
		Mod_LoadAliasModel (mod, buf);
		break;

	case IDSPRITEHEADER:
		Mod_LoadSpriteModel (mod, buf);
		break;

	default:
		Mod_LoadBrushModel (mod, buf);
		break;
	}

	return mod;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
qmodel_t *Mod_ForName (const char *name, cbool crash)
{
	qmodel_t	*mod;

	mod = Mod_FindName (name);

	return Mod_LoadModel (mod, crash);
}


/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/

static byte	*mod_base;


/*
=================
Mod_LoadTextures
=================
*/
cbool Is_Texture_Prefix (const char *texturename, const char *prefixstring)
{
	if (prefixstring[0] == 0)
		return false; // 0 length string

	//if (strncasecmp(texturename, prefixstring, strlen(prefixstring)) == 0)
	if (String_Does_Start_With_Caseless (texturename, prefixstring))
		return true;

	return false;
}


#ifdef GLQUAKE_RENDERER_SUPPORT


/*
=================
Mod_CheckFullbrights -- johnfitz
=================
*/
cbool Mod_CheckFullbrights (byte *pixels, int count, cbool alphatex)
{
	int i;
	for (i = 0; i < count; i++)
	{
		if (pixels[i] > 223)
		{
			if (alphatex && pixels[i] == 255)
				continue; // If alphatex 255 isn't fullbright so ignore mask pixel
			else return true;
		}
	}
	return false;
}


static cbool GameHacks_Is_Game_Level (const char *stripped_name)
{
	int i;
	if (gl_external_textures.value > 1)
		return true;

	for (i = 0; i < num_quake_original_levels; i ++)
		if (strcmp (levels[i].name, stripped_name)==0)
			return true;

	return false;
}

static modhint_e GameHacks_IsSpecialQuakeAliasModel (const char *model_name)
{
	// Joszef said:  NOTE: comparing not only with player.mdl, but with all models
	// begin with "player" coz we need to support DME models as well!

	if 	    (String_Does_Start_With_Caseless (model_name, "progs/player"))	return MOD_PLAYER_1;	// Why?
	else if (String_Does_Match_Caseless (model_name, "progs/eyes.mdl")		)	return MOD_EYES_2;		// Why?
//	else if (String_Does_Match_Caseless (model_name, "progs/flame0.mdl")	)	return MOD_FLAME_3;
	else if (String_Does_Match_Caseless (model_name, "progs/flame.mdl")	)		return MOD_FLAME_3;
	else if (String_Does_Match_Caseless (model_name, "progs/flame2.mdl")	)	return MOD_FLAME_3;
	else if (String_Does_Match_Caseless (model_name, "progs/bolt.mdl")		)	return MOD_THUNDERBOLT_4;
	else if (String_Does_Match_Caseless (model_name, "progs/bolt2.mdl")	)		return MOD_THUNDERBOLT_4;
	else if (String_Does_Match_Caseless (model_name, "progs/bolt3.mdl")	)		return MOD_THUNDERBOLT_4;
	else if (String_Does_Match_Caseless (model_name, "progs/v_shot.mdl")	)	return MOD_WEAPON_5;
	else if (String_Does_Match_Caseless (model_name, "progs/v_shot2.mdl")	)	return MOD_WEAPON_5;
	else if (String_Does_Match_Caseless (model_name, "progs/v_nail.mdl")	)	return MOD_WEAPON_5;
	else if (String_Does_Match_Caseless (model_name, "progs/v_nail2.mdl")	)	return MOD_WEAPON_5;
	else if (String_Does_Match_Caseless (model_name, "progs/v_rock.mdl")	)	return MOD_WEAPON_5;
	else if (String_Does_Match_Caseless (model_name, "progs/v_rock2.mdl")	)	return MOD_WEAPON_5;
	// hipnotic weapons
	else if (String_Does_Match_Caseless (model_name, "progs/v_laserg.mdl")	)	return MOD_WEAPON_5;
	else if (String_Does_Match_Caseless (model_name, "progs/v_prox.mdl")	)	return MOD_WEAPON_5;
	// rogue weapons
	else if (String_Does_Match_Caseless (model_name, "progs/v_grpple.mdl")	)	return MOD_WEAPON_5;
	else if (String_Does_Match_Caseless (model_name, "progs/v_lava.mdl")	)	return MOD_WEAPON_5;
	else if (String_Does_Match_Caseless (model_name, "progs/v_lava2.mdl")	)	return MOD_WEAPON_5;
	else if (String_Does_Match_Caseless (model_name, "progs/v_multi.mdl")	)	return MOD_WEAPON_5;
	else if (String_Does_Match_Caseless (model_name, "progs/v_multi2.mdl") )	return MOD_WEAPON_5;
	else if (String_Does_Match_Caseless (model_name, "progs/v_plasma.mdl") )	return MOD_WEAPON_5;
	else if (String_Does_Match_Caseless (model_name, "progs/v_star.mdl")   )	return MOD_WEAPON_5;
	else if (String_Does_Match_Caseless (model_name, "progs/lavaball.mdl") )	return MOD_LAVABALL_6;

	else if (String_Does_Match_Caseless (model_name, "progs/spike.mdl")	)		return MOD_SPIKE_7;
	else if (String_Does_Match_Caseless (model_name, "progs/s_spike.mdl")	)	return MOD_SPIKE_7;
	else if (String_Does_Match_Caseless (model_name, "progs/shambler.mdl")	)	return MOD_SHAMBLER_8;
	else if (String_Does_Match_Caseless (model_name, "progs/laser.mdl")	)		return MOD_LASER_11;

	return MOD_NORMAL_0;
}

void GameHacks_InitModelnames (void)
{
	int	i;

	memset (cl_modelnames, 0, sizeof(cl_modelnames));

	cl_modelnames[mi_player] 		= "progs/player.mdl";
	cl_modelnames[mi_h_player] 		= "progs/h_player.mdl";
	cl_modelnames[mi_eyes] 			= "progs/eyes.mdl";
	cl_modelnames[mi_rocket] 		= "progs/missile.mdl";
	cl_modelnames[mi_grenade] 		= "progs/grenade.mdl";
//	cl_modelnames[mi_flame0] 		= "progs/flame0.mdl";
	cl_modelnames[mi_flame1] 		= "progs/flame.mdl";
	cl_modelnames[mi_flame2] 		= "progs/flame2.mdl";
	cl_modelnames[mi_explo1] 		= "progs/s_expl.spr";
	cl_modelnames[mi_explo2] 		= "progs/s_explod.spr";
	cl_modelnames[mi_bubble] 		= "progs/s_bubble.spr";
	cl_modelnames[mi_gib1] 			= "progs/gib1.mdl";
	cl_modelnames[mi_gib2] 			= "progs/gib2.mdl";
	cl_modelnames[mi_gib3] 			= "progs/gib3.mdl";
	cl_modelnames[mi_fish] 			= "progs/fish.mdl";
	cl_modelnames[mi_dog] 			= "progs/dog.mdl";
	cl_modelnames[mi_soldier] 		= "progs/soldier.mdl";
	cl_modelnames[mi_enforcer] 		= "progs/enforcer.mdl";
	cl_modelnames[mi_knight] 		= "progs/knight.mdl";
	cl_modelnames[mi_hknight] 		= "progs/hknight.mdl";
	cl_modelnames[mi_scrag] 		= "progs/wizard.mdl";
	cl_modelnames[mi_ogre] 			= "progs/ogre.mdl";
	cl_modelnames[mi_fiend] 		= "progs/demon.mdl";
	cl_modelnames[mi_vore] 			= "progs/shalrath.mdl";
	cl_modelnames[mi_shambler] 		= "progs/shambler.mdl";
	cl_modelnames[mi_h_dog] 		= "progs/h_dog.mdl";
	cl_modelnames[mi_h_soldier] 	= "progs/h_guard.mdl";
	cl_modelnames[mi_h_enforcer] 	= "progs/h_mega.mdl";
	cl_modelnames[mi_h_knight] 		= "progs/h_knight.mdl";
	cl_modelnames[mi_h_hknight] 	= "progs/h_hellkn.mdl";
	cl_modelnames[mi_h_scrag] 		= "progs/h_wizard.mdl";
	cl_modelnames[mi_h_ogre] 		= "progs/h_ogre.mdl";
	cl_modelnames[mi_h_fiend] 		= "progs/h_demon.mdl";
	cl_modelnames[mi_h_vore] 		= "progs/h_shal.mdl";
	cl_modelnames[mi_h_shambler] 	= "progs/h_shams.mdl";
	cl_modelnames[mi_h_zombie] 		= "progs/h_zombie.mdl";

	for (i = 0; i < modelindex_max; i++)
	{
		if (!cl_modelnames[i])
			System_Error ("cl_modelnames[%d] not initialized", i);
	}
}



enum {warp_texture, regular_texture};

static const char *Mod_LoadExternalTexture (qmodel_t* mod, const char *texture_name, unsigned** data, int* fwidth, int* fheight)
{
	char plainname[MAX_QPATH_64];

	if (!gl_external_textures.value)
		return NULL;

	if (mod->bspversion == BSPVERSION_HALFLIFE)
		return NULL;

	c_strlcpy (plainname, File_URL_SkipPath (mod->name));
	File_URL_Edit_Remove_Extension (plainname);

	{
#define NUMDATA_SOURCES 4
		static char filename_success[MAX_OSPATH];
		cbool original_level = GameHacks_Is_Game_Level (plainname);
		char *fname0 = va ("texturepointer/%s", texture_name); // Remove me?  texturepointer cut/paste/temps
		char *fname1 = va ("textures/%s/%s", plainname, texture_name); // Map named variant
		char *fname2 = original_level ? va ("textures/exmy/%s", texture_name) : NULL;
		char *fname3 = va ("textures/%s", texture_name);
		char *filename[NUMDATA_SOURCES] = {fname0, fname1, fname2, fname3 };

		int i ;

		for (i = 0; i < NUMDATA_SOURCES; i ++)
		{
			char *current_filename = filename[i];
			char *ch;

			if (!current_filename)
				continue; // exmy might be nulled out if not an original level.

			// Remove asterisks
			for (ch = current_filename; *ch; ch++)
				if (*ch == '*')
					*ch = '#';

			// Try the texture
			*data = Image_Load_Limited (current_filename, fwidth, fheight, mod->loadinfo.searchpath);
			if (*data)
			{
				c_strlcpy (filename_success, current_filename);
				return filename_success;
			}
		}
	}

	return NULL;
}


// Baker -- I could calc pixels data (tx+1) and wad3paldata (tx+1)+numpixels but this = better.
void Mod_Load_Brush_Model_Texture (qmodel_t* ownermod, int bsp_texnum, texture_t * tx, byte *tx_qpixels, byte *tx_wad3_palette)
{
	int			pixelcount = tx->width * tx->height;

	int			mark = Hunk_LowMark(); // Both paths do this
	int			fwidth, fheight;
	unsigned	*data;
	const char *external_filename =  Mod_LoadExternalTexture (ownermod, tx->name,&data, &fwidth, &fheight);

	char		warp_prefix = ownermod->bspversion != BSPVERSION_HALFLIFE ? '*' : '!';
	int			texture_type = (tx->name[0] == warp_prefix) ? warp_texture : regular_texture;
	int			extraflags = 0;
	const char *warp_texturename;

#pragma message ("Baker: If this isn't right, we will find out")
	if (tx->gltexture) 	 tx->gltexture = TexMgr_FreeTexture (tx->gltexture);
	if (tx->fullbright)  tx->fullbright= TexMgr_FreeTexture (tx->fullbright);
	if (tx->warpimage)   tx->warpimage = TexMgr_FreeTexture (tx->warpimage);

	tx->update_warp = false;

	switch (texture_type)
	{
	case warp_texture:
		//if external texture found then
		//now load whatever we found
		if (external_filename) //load external image
		{
			tx->gltexture = TexMgr_LoadImage (
				ownermod, bsp_texnum,	// owner, bsp texture num
				external_filename, 		// description of source
				fwidth, fheight,		// width, height
				SRC_RGBA, 				// type of bytes
				data, 					// the bytes
				external_filename,		// data source file
				0, 						// offset into file
				TEXPREF_NONE			// processing flags
			);
			warp_texturename = va ("%s_warp", external_filename);
		}
		else if (ownermod->bspversion == BSPVERSION_HALFLIFE)
		{
			//use the texture from the bsp file
			const char *in_model_texturename = va ("%s:%s", ownermod->name, tx->name);
			tx->gltexture = TexMgr_LoadImage_SetPal (
				ownermod, bsp_texnum, in_model_texturename,
				tx->width, tx->height,
				SRC_INDEXED_WITH_PALETTE, tx_qpixels, tx_wad3_palette,
				"", (src_offset_t)tx_qpixels, (src_offset_t)tx_wad3_palette,
				TEXPREF_NONE
			);
			warp_texturename = va ("%s_warp", in_model_texturename);
		}
		else
		{
			//use the texture from the bsp file
			const char *in_model_texturename = va ("%s:%s", ownermod->name, tx->name);

			tx->gltexture = TexMgr_LoadImage (
				ownermod, bsp_texnum, 	// owner, bsp texnum
				in_model_texturename, 	// description of source
				tx->width, tx->height,	// width, height
				SRC_INDEXED, 			// type of bytes
				tx_qpixels, 			// the bytes
				"", 					// data source file
				(src_offset_t)tx_qpixels,// offset into file
				TEXPREF_NONE			// processing flags
			);
			warp_texturename = va ("%s_warp", in_model_texturename);
		}
#ifdef DIRECT3D8_WRAPPER // dx8 - no oldwater
				// no warp updates
				tx->update_warp = false;
#else // NOT DIRECT3D8_WRAPPER ... (oldwater)
		{
			extern byte *hunk_base;

			//now create the warpimage, using dummy data from the hunk to create the initial image
			Hunk_AllocName (gl_warpimagesize*gl_warpimagesize * 4, "warp_tx"); //make sure hunk is big enough so we don't reach an illegal address
			Hunk_FreeToLowMark (mark);
			tx->warpimage = TexMgr_LoadImage (
					ownermod, -1 /* texturepointer won't use this*/, warp_texturename,
					gl_warpimagesize, gl_warpimagesize,
					SRC_RGBA, hunk_base, "",
					(src_offset_t)hunk_base,
					TEXPREF_NOPICMIP | TEXPREF_WARPIMAGE
			);
			tx->update_warp = true;
		}
#endif // ! DIRECT3D8_WRAPPER (oldwater)
		// Warp texture we already cleared the hunk
		break;

	case regular_texture:

		// Checking for fence texture
//		switch (ownermod->bspversion)
//		{
//		case BSPVERSION_HALFLIFE:
			if (tx->name[0] == '{')
				extraflags |=  TEXPREF_ALPHA;
//			break;
//		default:
//			if (Is_Texture_Prefix (tx->name, r_texprefix_fence.string))
//				extraflags |=  TEXPREF_ALPHA;
//			break;
//		}


		//now load whatever we found
		if (external_filename) //load external image
		{
			const char *glow_name = va ("%s_glow", File_URL_SkipPath(external_filename));
			const char *luma_name = va ("%s_luma", File_URL_SkipPath(external_filename));

			tx->gltexture = TexMgr_LoadImage (
				ownermod, bsp_texnum,
				external_filename, fwidth, fheight,
				SRC_RGBA,
				data,
				external_filename,
				0,
				TEXPREF_MIPMAP | extraflags
			);

			//now try to load glow/luma image from the same place
			Hunk_FreeToLowMark (mark);
			external_filename = Mod_LoadExternalTexture (ownermod, glow_name,&data, &fwidth, &fheight);
			if (external_filename == NULL)
				external_filename = Mod_LoadExternalTexture (ownermod, luma_name, &data, &fwidth, &fheight);

			if (external_filename == NULL)
				break; // Bust out of our switch statement

			// Load the glow or luma we found
			tx->fullbright = TexMgr_LoadImage (
				ownermod, bsp_texnum,
				external_filename,
				fwidth, fheight,
				SRC_RGBA,
				data,
				external_filename,
				0,
				TEXPREF_MIPMAP | extraflags
			);
		}
		else
		{
			// No external texture so
			// use the texture from the bsp file
			const char *in_model_texturename = va ("%s:%s", ownermod->name, tx->name);
			if (ownermod->bspversion == BSPVERSION_HALFLIFE)
			{
				tx->gltexture = TexMgr_LoadImage_SetPal (
					ownermod, bsp_texnum, in_model_texturename,
					tx->width, tx->height,
					SRC_INDEXED_WITH_PALETTE, tx_qpixels, tx_wad3_palette,
					"", (src_offset_t)tx_qpixels, (src_offset_t)tx_wad3_palette,
					TEXPREF_MIPMAP | extraflags
				);
			}
			else
			// Quake texture no external, check for fullbrights
			if (Mod_CheckFullbrights (tx_qpixels, pixelcount, extraflags))
			{
				// Quake fullbright texture
				const char *in_model_glow_texturename = va ("%s:%s_glow", ownermod->name, tx->name);
				tx->gltexture = TexMgr_LoadImage (
					ownermod, bsp_texnum, in_model_texturename,
					tx->width, tx->height,
					SRC_INDEXED, tx_qpixels,
					"", (src_offset_t)tx_qpixels,
					TEXPREF_MIPMAP | TEXPREF_NOBRIGHT | extraflags
				);
				tx->fullbright = TexMgr_LoadImage (
					ownermod, bsp_texnum, in_model_glow_texturename,
					tx->width, tx->height,
					SRC_INDEXED, tx_qpixels,
					"", (src_offset_t)tx_qpixels,
					TEXPREF_MIPMAP | TEXPREF_FULLBRIGHT | extraflags
				);
			}
			else
			{
				// Quake non-fullbright texture
				tx->gltexture = TexMgr_LoadImage (
					ownermod, bsp_texnum, in_model_texturename,
					tx->width, tx->height,
					SRC_INDEXED, tx_qpixels,
					"", (src_offset_t)tx_qpixels,
					TEXPREF_MIPMAP | extraflags
				);
			}
		}
		// Regular texture clears to low mark
		Hunk_FreeToLowMark (mark);
		break;
	} // End of switch statement

}

void Mod_Brush_ReloadTextures (qmodel_t* mod)
{
	int i;

	// Baker --- (-2) last 2 textures are missing textures
	// We could do  "i < mod->numtextures - 2"
	// A missing texture will render as checkered

	for (i = 0 ; i < mod->numtextures; i++)
	{
		cbool	is_wad3 = (mod->bspversion == BSPVERSION_HALFLIFE);
		texture_t	*tx = mod->textures[i];
		if (tx) { // Missing texture
			int			pixelcount = tx->width * tx->height;
			byte		*tx_qpixels = (byte*)(tx + 1);
			byte		*tx_wad3_palette = is_wad3 ? (byte*)tx_qpixels + pixelcount : NULL;

			if (mod->type == mod_alias)

			// Except don't do sky
			if (mod->bspversion != BSPVERSION_HALFLIFE)

				if (!strncasecmp(tx->name,"sky",3))
					continue;

			// Now what?
			Mod_Load_Brush_Model_Texture (mod, i, tx, tx_qpixels, tx_wad3_palette);
		} else 
			Con_DPrintLinef ("No texture %s # %d", mod->name, i);
	}
}

void External_Textures_Change_f  (cvar_t* var)
{
	int i;

	if (!cl.worldmodel)
		return;

	Mod_Brush_ReloadTextures (cl.worldmodel);

	for (i = 1; i < MAX_FITZQUAKE_MODELS && cl.model_precache[i] ; i++)
	{
		qmodel_t* mod = cl.model_precache[i];

		if (mod == cl.worldmodel)
			continue; // Don't do world twice.

		switch (mod->type)
		{
		case mod_brush:
			// If world submodel, it shares textures with world --- no need so skip
			if (mod->name[0] != '*')
				Mod_Brush_ReloadTextures (mod);
			break;
		default:
			// Not supported at this time (alias, sprites)
			break;
		}
	}

	// Handles the alias models.  Is supposed to handle sprites.
	Cache_Flush (NULL);

	Draw_NewGame ();	// Clears scrap
	R_NewGame ();		// Clears skins
	// Reload the skins?
	{
		int i;
		entity_t *ent;
		for (i = 0, ent = cl_entities ; i < cl.num_entities ; i++, ent++) {
			if (ent->colormap && ent->coloredskin)
				ent->coloredskin = NULL; // It's gone!
		}
	}


#pragma message ("Do we say anything about gamedir change required")
#pragma message ("Baker: What do you mean???  Gamedir change isn't required is it?")
#pragma message ("Baker: Looks like sprites doesn't reload.  I think alias models do")
}

#endif // GLQUAKE_RENDERER_SUPPORT

static void Mod_LoadTextures (lump_t *l)
{
	dmiptexlump_t	*m = NULL;
	int				nummiptex=  0;
	int		i;

#ifdef GLQUAKE_RENDERER_SUPPORT
	//johnfitz -- need 2 dummy texture chains for missing textures
	loadmodel->numtextures = 2;
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
	loadmodel->numtextures = 0;
#endif // WINQUAKE_RENDERER_SUPPORT

	if (l->filelen)
	{
		m = (dmiptexlump_t *)(mod_base + l->fileofs);
		nummiptex = m->nummiptex = LittleLong (m->nummiptex);
		loadmodel->numtextures += nummiptex;
	}
	else
	{
		//johnfitz -- GL: don't return early if no textures; still need to create dummy texture
		Con_PrintLinef ("Mod_LoadTextures: no textures in bsp file");
#ifdef WINQUAKE_RENDERER_SUPPORT
		loadmodel->textures = NULL;
		return;
#endif // WINQUAKE_RENDERER_SUPPORT
	}

	loadmodel->textures = (texture_t **)Hunk_AllocName (loadmodel->numtextures * sizeof(*loadmodel->textures) , loadname);

	for (i=0 ; i< nummiptex ; i++)
	{
		miptex_t	*mt;
		texture_t	*tx;
		byte		*tx_qpixels;
		int			pixelcount;
#ifdef GLQUAKE_RENDERER_SUPPORT
		src_offset_t into_file_offset;
		byte		*tx_wad3_palette;
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
		int			pixmip0;
		int			j;
#endif // WINQUAKE_RENDERER_SUPPORT

		if ((m->dataofs[i] = LittleLong(m->dataofs[i])) == -1)
			continue;
		do
		{
			mt = (miptex_t *)((byte *)m + m->dataofs[i]);

			mt->width = LittleLong (mt->width);
			mt->height = LittleLong (mt->height);
#ifdef GLQUAKE_RENDERER_SUPPORT
			// Baker --- offsets 1,2,3 = on disk mipmaps = aren't used in GLQuake
			mt->offsets[0] = LittleLong (mt->offsets[0]);
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
			for (j=0 ; j<MIPLEVELS ; j++)
				mt->offsets[j] = LittleLong (mt->offsets[j]);
#endif // WINQUAKE_RENDERER_SUPPORT

			if ( (mt->width & 15) || (mt->height & 15) )
				Host_Error ("Mod_LoadTextures: Texture %s is not 16 aligned", mt->name);

#ifdef GLQUAKE_RENDERER_SUPPORT
			into_file_offset = (src_offset_t)(mt + 1) - (src_offset_t)mod_base;
			pixelcount = mt->width * mt->height; ///64*85;
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
			pixelcount = mt->width*mt->height/64*85; ///64*85;
			pixmip0 = mt->width*mt->height;
#endif // WINQUAKE_RENDERER_SUPPORT

			if (loadmodel->bspversion != BSPVERSION_HALFLIFE)
				break; // Get out of this segment

		} while (0);

		// Populate the texture_t
		do
		{
#ifdef GLQUAKE_RENDERER_SUPPORT
			// Baker: if Half-Life texture store the palette too, so tx is full of all data we need
			// And so we do not need mt anymore.
			cbool do_palette_too = (loadmodel->bspversion == BSPVERSION_HALFLIFE);

			int pixelcount_plus = pixelcount + (do_palette_too ? PALETTE_SIZE_768 : 0);
			tx = loadmodel->textures[i] = (texture_t *) Hunk_AllocName (sizeof(texture_t) + pixelcount_plus, loadname );
			memcpy (tx->name, mt->name, sizeof(tx->name));
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
			tx = loadmodel->textures[i] = Hunk_AllocName (sizeof(texture_t) + pixelcount, loadname );

			memcpy (tx->name, mt->name, sizeof(tx->name)); // Baker: Leave as memcpy, might be funky
#endif // WINQUAKE_RENDERER_SUPPORT

			// Baker: so unnamed ones can have external textures
			if (!tx->name[0])
			{
				c_snprintf1 (tx->name, "unnamed%d", i);
				Con_DWarningLine ("unnamed texture in %s, renaming to %s", loadmodel->name, tx->name);
			}

			// Fill in the data
			tx->width = mt->width;
			tx->height = mt->height;
			tx_qpixels = (byte*)(tx + 1);

#ifdef GLQUAKE_RENDERER_SUPPORT
			tx->offset0 = mt->offsets[0] + sizeof(texture_t) - sizeof(miptex_t);
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
			for (j=0 ; j<MIPLEVELS ; j++)
				tx->offsets[j] = mt->offsets[j] + sizeof(texture_t) - sizeof(miptex_t);
#endif // WINQUAKE_RENDERER_SUPPORT

			// copy the data over,  the pixels immediately follow the structures
			memcpy ( tx_qpixels, mt+1, pixelcount);

#ifdef GLQUAKE_RENDERER_SUPPORT
			if (do_palette_too)
			{
				// Determine WAD3 palette location then copy over
				byte *mt_wad3_palette = ((byte *) ((byte *) mt + mt->offsets[0])) + ((pixelcount * 85) >> 6) + 2;
				tx_wad3_palette = (byte*)tx_qpixels + pixelcount;
				memcpy (tx_wad3_palette, mt_wad3_palette, PALETTE_SIZE_768);

				//	wad3_palette_into_file_offset = (src_offset_t)wad3_palette - (src_offset_t)mod_base;
			}

			// Baker: If we are going to store the original pixels off, let's do this here ... instead of gl_texmgr
			// Why? If you are going to store the original pixels off, do it right and ...
			// I might get bored make a live toggle between replacement and original texture someday
			if (!strcmp(tx->name, "shot1sid") && tx->width == 32 && tx->height == 32 && CRC_Block(tx_qpixels, pixelcount) == 65393)
			{   // This texture in b_shell1.bsp has some of the first 32 pixels painted white.
				// They are invisible in software, but look really ugly in GL. So we just copy
				// 32 pixels from the bottom to make it look nice.
				memcpy (tx_qpixels, tx_qpixels + 32 * 31, 32);
			}
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
			if (loadmodel->bspversion == BSPVERSION_HALFLIFE)
			{
				// Baker: Process Half-Life textures
				byte *mt_wad3_palette = ((byte *) ((byte *) mt + mt->offsets[0])) + ((pixmip0 * 85) >> 6) + 2;
				Image_Convert_Palette_To_Palette (tx_qpixels, pixelcount, mt_wad3_palette, vid.basepal);
			}
#endif // WINQUAKE_RENDERER_SUPPORT

		} while (0);

		if (isDedicated)
			continue;  // Baker: dedicated doesn't upload textures

		// Upload textures phase

		if (loadmodel->bspversion != BSPVERSION_HALFLIFE && !strncasecmp(tx->name,"sky",3)) //sky texture //also note -- was Q_strncmp, changed to match qbsp
		{
			Sky_LoadTexture (tx);
			continue; // We're done for this texture
		}


#ifdef GLQUAKE_RENDERER_SUPPORT
		Mod_Load_Brush_Model_Texture (loadmodel, i, tx, tx_qpixels, tx_wad3_palette);

// FUNC_ILLUSIONARY_MIRRORS
#if 1 // For debugging only.  Allows us to track the rendering.
		if (loadmodel->isworldmodel /* prevent healthboxes from having mirrors*/ &&  GL_Mirrors_Is_TextureName_Mirror(tx->name)) {
			level.mirror = true; // Set to true, but never set this to false.
			// We will need to scan static entities
			// If this is true
			// If this is true and other is false then
			//   there are only mirrors on func_illusionary
			tx->gltexture->is_mirror = true;
		}
#endif

#endif // GLQUAKE_RENDERER_SUPPORT

	}

#ifdef GLQUAKE_RENDERER_SUPPORT
//
// johnfitz -- last 2 slots in array should be filled with dummy textures
//
	loadmodel->textures[loadmodel->numtextures-2] = r_notexture_mip; //for lightmapped surfs
	loadmodel->textures[loadmodel->numtextures-1] = r_notexture_mip2; //for SURF_DRAWTILED surfs
#endif // GLQUAKE_RENDERER_SUPPORT

//
// sequence the animations
//
	for (i=0 ; i<nummiptex ; i++)
	{
		int j, num, maxanim, altmax;
		texture_t	*ptx, *ptx2;
		texture_t	*anims[10];
		texture_t	*altanims[10];

		ptx = loadmodel->textures[i];
		if (!ptx || ptx->name[0] != '+')
			continue;

		if (ptx->anim_next)
			continue;	// already sequenced

	// find the number of frames in the animation
		memset (anims, 0, sizeof(anims));
		memset (altanims, 0, sizeof(altanims));

		maxanim = ptx->name[1];
		altmax = 0;
		if (maxanim >= 'a' && maxanim <= 'z')
			maxanim -= 'a' - 'A';

		if (maxanim >= '0' && maxanim <= '9')
		{
			maxanim -= '0';
			altmax = 0;
			anims[maxanim] = ptx;
			maxanim++;
		}
		else if (maxanim >= 'A' && maxanim <= 'J')
		{
			altmax = maxanim - 'A';
			maxanim = 0;
			altanims[altmax] = ptx;
			altmax++;
		}
		else
		{
			Host_Error ("Mod_LoadTextures: Bad animating texture %s", ptx->name);
		}

		for (j=i+1 ; j<nummiptex ; j++)
		{
			ptx2 = loadmodel->textures[j];
			if (!ptx2 || ptx2->name[0] != '+')
				continue;
			if (strcmp (ptx2->name+2, ptx->name+2)) // Baker: +1 to skip prefix, but why +2
				continue;

			num = ptx2->name[1];
			if (num >= 'a' && num <= 'z')
				num -= 'a' - 'A';
			if (num >= '0' && num <= '9')
			{
				num -= '0';
				anims[num] = ptx2;
				if (num+1 > maxanim)
					maxanim = num + 1;
			}
			else if (num >= 'A' && num <= 'J')
			{
				num = num - 'A';
				altanims[num] = ptx2;
				if (num+1 > altmax)
					altmax = num+1;
			}
			else
			{
				Host_Error ("Mod_LoadTextures: Bad animating texture %s", ptx->name);
			}
		}

#define	ANIM_CYCLE	2
	// link them all together
		for (j=0 ; j<maxanim ; j++)
		{
			ptx2 = anims[j];
			if (!ptx2)
				Host_Error ("Mod_LoadTextures: Missing frame %d of %s",j, ptx->name);
			ptx2->anim_total = maxanim * ANIM_CYCLE;
			ptx2->anim_min = j * ANIM_CYCLE;
			ptx2->anim_max = (j+1) * ANIM_CYCLE;
			ptx2->anim_next = anims[ (j+1)%maxanim ];
			if (altmax)
				ptx2->alternate_anims = altanims[0];
		}

		for (j=0 ; j<altmax ; j++)
		{
			ptx2 = altanims[j];
			if (!ptx2)
				Host_Error ("Mod_LoadTextures: Missing frame %d of %s",j, ptx->name);
			ptx2->anim_total = altmax * ANIM_CYCLE;
			ptx2->anim_min = j * ANIM_CYCLE;
			ptx2->anim_max = (j+1) * ANIM_CYCLE;
			ptx2->anim_next = altanims[ (j+1)%altmax ];
			if (maxanim)
				ptx2->alternate_anims = anims[0];
		}
	}
}


#ifdef GLQUAKE_RENDERER_SUPPORT
// Baker: The below is from FTE
byte lmgamma[256];
void LightNormalize (byte *litdata, const byte *normal, int bsp_lit_size)
{
	static cbool built_table = false;
	float prop;
	int i;

	if (!built_table)
	{
		Image_Build_Gamma_Table (1, 1, lmgamma);
		built_table = true;
	}

	//force it to the same intensity. (or less, depending on how you see it...)
	for (i = 0; i < bsp_lit_size; i++)
	{
		#define m(a, b, c) (a>(b>c?b:c)?a:(b>c?b:c))
		prop = (float)m(litdata[0],  litdata[1], litdata[2]);

		if (!prop)
		{
			litdata[0] = lmgamma[*normal];
			litdata[1] = lmgamma[*normal];
			litdata[2] = lmgamma[*normal];
		}
		else
		{
			prop = lmgamma[*normal] / prop;
			litdata[0] *= prop;
			litdata[1] *= prop;
			litdata[2] *= prop;
		}

		normal++;
		litdata+=3;
	}
}


/*
=================
Mod_LoadLighting -- johnfitz -- replaced with lit support code via lordhavoc
=================
*/
static void Mod_LoadLighting (lump_t *l)
{
	int mark = Hunk_LowMark();

	loadmodel->lightdata = NULL;

	if (loadmodel->bspversion == BSPVERSION_HALFLIFE)
	{
		if (!l->filelen)
			return; // Because we won't be checking for external lits with Half-Life, this is automatic fail

		loadmodel->lightdata = Hunk_AllocName(l->filelen, loadname);
		Con_DPrintLinef ("lighting data at %p with length %d", mod_base + l->fileofs, l->filelen);
		memcpy (loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
		return;
	}


	// External lit phase
	do
	{
		char litfilename[MAX_OSPATH];
		int qlitversion;
		byte *data, *bsp_light_data;

		if (!gl_external_lits.value)
			break;

		// LordHavoc: check for a .lit file
		c_strlcpy (litfilename, loadmodel->name);
		File_URL_Edit_Remove_Extension (litfilename);
		c_strlcat (litfilename, ".lit");

		data = (byte*) COM_LoadHunkFile_Limited (litfilename, loadmodel->loadinfo.searchpath);
//		Con_PrintLinef ("com_filesize is %d", com_filesize);

		if (!data)
			break;

		if (l->filelen && (com_filesize - 8) != l->filelen * 3)
		{
			//Con_PrintLinef ("Corrupt .lit file (old version?), ignoring");
			Con_PrintLinef ("Invalid .lit file (data length mismatch against map), ignoring");
			break;
		}

		if (memcmp (data, "QLIT", 4) !=0 )
		{
			Con_PrintLinef ("Corrupt .lit file (old version?), ignoring");
			break;
		}


		qlitversion = LittleLong(((int *)data)[1]);
		if (qlitversion != 1)
		{
			Con_PrintLinef ("Unknown .lit file version (%d)", qlitversion);
			break;
		}

		// Commmitted
		loadmodel->lightdata = data + 8;

		// Final act
		bsp_light_data = mod_base + l->fileofs;
		LightNormalize (loadmodel->lightdata, bsp_light_data, l->filelen);

		// Success
		return;

	} while (0);

	// No lit, regular data
	Hunk_FreeToLowMark (mark);

	if (l->filelen)
	{
		int i;
		byte *in, *out;
		byte d;

		// Expand the white lighting data to color ...
		loadmodel->lightdata = (byte *) Hunk_AllocName (l->filelen * 3, "qlightdata");

		// place the file at the end, so it will not be overwritten until the very last write
		in = loadmodel->lightdata + l->filelen * 2;
		memcpy (in, mod_base + l->fileofs, l->filelen);

		for (i = 0, out = loadmodel->lightdata; i < l->filelen; i ++, out +=3)
		{
			d = *in++;
			out[0] = out[1] = out[2] = d;
		}
	}
}
#endif // GLQUAKE_RENDERER_SUPPORT


#ifdef WINQUAKE_RENDERER_SUPPORT
/*
=================
Mod_LoadLighting
=================
*/
static void Mod_LoadLighting (lump_t *l)
{
	if (!l->filelen)
	{
		loadmodel->lightdata = NULL;
		return;
	}

	if (loadmodel->bspversion == BSPVERSION_HALFLIFE)
	{
		int i;
		byte *in,*out;

		loadmodel->lightdata = Hunk_AllocName ( l->filelen/3, loadname);
		for (i = 0, in = mod_base + l->fileofs, out=loadmodel->lightdata; i < l->filelen /3; i++, in+=3, out++)
		{
			*out= (byte)  (   ( (float)in[0] + in[1] + in[2]  )/3   );
		}
		return;
	}

	loadmodel->lightdata = Hunk_AllocName ( l->filelen, loadname);
	memcpy (loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
}
#endif // WINQUAKE_RENDERER_SUPPORT

/*
=================
Mod_LoadVisibility
=================
*/
static void Mod_LoadVisibility (lump_t *l)
{
	if (!l->filelen)
	{
		Con_DPrintLinef ("No vis for model");
		loadmodel->visdata = NULL;
		return;
	}
	loadmodel->visdata = (byte *)Hunk_AllocName ( l->filelen, loadname);
	memcpy (loadmodel->visdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadEntities
=================
*/
static void Mod_LoadEntities (lump_t *l)
{
	if (loadmodel->isworldmodel && model_external_ents.value)
	{
		char entfilename[MAX_QPATH_64], *entstring;

		c_strlcpy (entfilename, loadmodel->name);
		File_URL_Edit_Remove_Extension (entfilename);
		c_strlcat (entfilename, ".ent");

		if ( (entstring = (char *)COM_LoadHunkFile_Limited (entfilename, loadmodel->loadinfo.searchpath )) )
		{
			loadmodel->entities = entstring;
			Con_PrintLinef ("External .ent found: %s", entfilename);
			return;
		}
	}

	if (!l->filelen)
	{
		loadmodel->entities = NULL;
		return;
	}

	loadmodel->entities = (char *)Hunk_AllocName ( l->filelen, loadname);
	memcpy (loadmodel->entities, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadVertexes
=================
*/
static void Mod_LoadVertexes (lump_t *l)
{
	dvertex_t	*in;
	mvertex_t	*out;
	int			i, count;

	in = (dvertex_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadVertexes: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);
#ifdef WINQUAKE_SOFTWARE_SKYBOX
	out = (mvertex_t *)Hunk_AllocName ( (count+8)*sizeof(*out), loadname); // Manoel Kasimier - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
#else
	out = (mvertex_t *)Hunk_AllocName ( count*sizeof(*out), loadname);
#endif // WINQUAKE_SOFTWARE_SKYBOX

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->position[0] = LittleFloat (in->point[0]);
		out->position[1] = LittleFloat (in->point[1]);
		out->position[2] = LittleFloat (in->point[2]);
	}
}

/*
=================
Mod_LoadEdges
=================
*/
static void Mod_LoadEdges (lump_t *l, int bsp2)
{
	medge_t *out;
	int 	i, count;

	if (bsp2)
	{
		dledge_t *in = (dledge_t *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadEdges: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);

#ifdef WINQUAKE_SOFTWARE_SKYBOX
	out = (medge_t *)Hunk_AllocName ( (count + 13) * sizeof(*out), loadname); // Manoel Kasimier - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
#else
	out = (medge_t *)Hunk_AllocName ( (count + 1) * sizeof(*out), loadname);
#endif // WINQUAKE_SOFTWARE_SKYBOX
		loadmodel->edges = out;
		loadmodel->numedges = count;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			out->v[0] = LittleLong(in->v[0]);
			out->v[1] = LittleLong(in->v[1]);
		}
	}
	else
	{
		dsedge_t *in = (dsedge_t *)(mod_base + l->fileofs);

		if (l->filelen % sizeof(*in))
			Host_Error ("Mod_LoadEdges: funny lump size in %s",loadmodel->name);

		count = l->filelen / sizeof(*in);
#ifdef WINQUAKE_SOFTWARE_SKYBOX
		out = (medge_t *)Hunk_AllocName ( (count + 13) * sizeof(*out), loadname); // Manoel Kasimier - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
#else
		out = (medge_t *)Hunk_AllocName ( (count + 1) * sizeof(*out), loadname);
#endif // WINQUAKE_SOFTWARE_SKYBOX

		loadmodel->edges = out;
		loadmodel->numedges = count;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			out->v[0] = (unsigned short)LittleShort(in->v[0]);
			out->v[1] = (unsigned short)LittleShort(in->v[1]);
		}
	}
}

/*
=================
Mod_LoadTexinfo
=================
*/
static void Mod_LoadTexinfo (lump_t *l)
{
	texinfo_t *in;
	mtexinfo_t *out;
	int 	i, j, count, miptex;
#ifdef WINQUAKE_RENDERER_SUPPORT
	float	len1, len2;
#endif // WINQUAKE_RENDERER_SUPPORT
	int missing = 0; //johnfitz

	in = (texinfo_t *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadTexinfo: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);

#ifdef WINQUAKE_SOFTWARE_SKYBOX
	out = (mtexinfo_t *)Hunk_AllocName ( (count+6)*sizeof(*out), loadname); // Manoel Kasimier - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
#else
	out = (mtexinfo_t *)Hunk_AllocName ( count*sizeof(*out), loadname);
#endif // WINQUAKE_SOFTWARE_SKYBOX

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<4 ; j++)
		{
			out->vecs[0][j] = LittleFloat (in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat (in->vecs[1][j]);
		}
#ifdef WINQUAKE_RENDERER_SUPPORT
		len1 = VectorLength (out->vecs[0]);
		len2 = VectorLength (out->vecs[1]);
		len1 = (len1 + len2)/2;
		if (len1 < 0.32)
			out->mipadjust = 4;
		else if (len1 < 0.49)
			out->mipadjust = 3;
		else if (len1 < 0.99)
			out->mipadjust = 2;
		else
			out->mipadjust = 1;
#endif // WINQUAKE_RENDERER_SUPPORT

		miptex = LittleLong (in->miptex);
		out->flags = LittleLong (in->flags);

		//johnfitz -- rewrote this section
#ifdef GLQUAKE_RENDERER_SUPPORT
		if (miptex >= loadmodel->numtextures-1 || !loadmodel->textures[miptex])
		{
			if (out->flags & TEX_SPECIAL)
				out->texture = loadmodel->textures[loadmodel->numtextures-1];
			else
				out->texture = loadmodel->textures[loadmodel->numtextures-2];

			out->flags |= TEX_MISSING;
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
		if (miptex >= loadmodel->numtextures || !loadmodel->textures[miptex])
		{
			out->texture = r_notexture_mip;	// checkerboard texture
			out->flags = 0;
#endif // WINQUAKE_RENDERER_SUPPORT
			missing++;
		}
		else
		{
			out->texture = loadmodel->textures[miptex];
		}

		//johnfitz
	}

	//johnfitz: report missing textures
	if (missing && loadmodel->numtextures > 1)
		Con_PrintLinef ("Mod_LoadTexinfo: %d texture(s) missing from BSP file", missing);

	//johnfitz
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
static void CalcSurfaceExtents (msurface_t *s)
{
	float	mins[2], maxs[2], val;
	int		i, j, e;
	mvertex_t	*v;
	mtexinfo_t	*tex;
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;

	for (i=0 ; i<s->numedges ; i++)
	{
		e = loadmodel->surfedges[s->firstedge+i];

		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

		for (j=0 ; j<2 ; j++)
		{
#if 1  // Quakespasm, ericw -- for 64-bit
			val =   ((double)v->position[0] * (double)tex->vecs[j][0]) +
					((double)v->position[1] * (double)tex->vecs[j][1]) +
					((double)v->position[2] * (double)tex->vecs[j][2]) +
					(double)tex->vecs[j][3];
#else
			val = v->position[0] * tex->vecs[j][0] +
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
#endif

			if (val < mins[j])
				mins[j] = val;

			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i = 0 ; i < 2 ; i++)
	{
		bmins[i] = floor(mins[i]/16);
		bmaxs[i] = ceil(maxs[i]/16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;

		if ( !(tex->flags & TEX_SPECIAL) && s->extents[i] > MAX_FITZQUAKE_SURFACE_EXTENTS) //johnfitz -- was 512 in glquake, 256 in winquake
			Host_Error ("CalcSurfaceExtents: Bad surface extents %d (MAX: %d), texture is %s.", s->extents[i], MAX_FITZQUAKE_SURFACE_EXTENTS, tex->texture->name);

		if ( !(tex->flags & TEX_SPECIAL) && s->extents[i] > MAX_WINQUAKE_SURFACE_EXTENTS /* 256*/ )
			Con_DWarningLine ("%d surface extents exceed standard limit of %d.", s->extents[i], MAX_WINQUAKE_SURFACE_EXTENTS);

	}

	s->smax = (s->extents[0] >> 4) + 1;
	s->tmax = (s->extents[1] >> 4) + 1;
}

#ifdef GLQUAKE_RENDERER_SUPPORT
/*
================
Mod_PolyForUnlitSurface -- johnfitz -- creates polys for unlightmapped surfaces (sky and water)

TODO: merge this into BuildSurfaceDisplayList?
================
*/
void Mod_PolyForUnlitSurface (msurface_t *fa)
{
	vec3_t		verts[64];
	int			numverts, i, lindex;
	float		*vec;
	glpoly_t	*poly;
	float		texscale;

	if (fa->flags & (SURF_DRAWTURB | SURF_DRAWSKY))
		texscale = (1.0/128.0); //warp animation repeats every 128
	else
		texscale = (1.0/32.0); //to match r_notexture_mip

	// convert edges back to a normal polygon
	numverts = 0;
	for (i=0 ; i<fa->numedges ; i++)
	{
		lindex = loadmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
			vec = loadmodel->vertexes[loadmodel->edges[lindex].v[0]].position;
		else
			vec = loadmodel->vertexes[loadmodel->edges[-lindex].v[1]].position;
		VectorCopy (vec, verts[numverts]);
		numverts++;
	}

	//create the poly
	poly = (glpoly_t *)Hunk_Alloc (sizeof(glpoly_t) + (numverts-4) * VERTEXSIZE*sizeof(float));
	poly->next = NULL;
	fa->polys = poly;
	poly->numverts = numverts;
	for (i=0, vec=(float *)verts; i<numverts; i++, vec+= 3)
	{
		VectorCopy (vec, poly->verts[i]);
		poly->verts[i][3] = DotProduct(vec, fa->texinfo->vecs[0]) * texscale;
		poly->verts[i][4] = DotProduct(vec, fa->texinfo->vecs[1]) * texscale;
	}
}
#endif // GLQUAKE_RENDERER_SUPPORT

/*
=================
Mod_CalcSurfaceBounds -- johnfitz -- calculate bounding box for per-surface frustum culling
=================
*/
void Mod_CalcSurfaceBounds (msurface_t *s)
{
	int			i, e;
	mvertex_t	*v;

	s->mins[0] = s->mins[1] = s->mins[2] =  9999999;
	s->maxs[0] = s->maxs[1] = s->maxs[2] = -9999999;

	for (i=0 ; i<s->numedges ; i++)
	{
		e = loadmodel->surfedges[s->firstedge+i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

		if (s->mins[0] > v->position[0])
			s->mins[0] = v->position[0];
		if (s->mins[1] > v->position[1])
			s->mins[1] = v->position[1];
		if (s->mins[2] > v->position[2])
			s->mins[2] = v->position[2];

		if (s->maxs[0] < v->position[0])
			s->maxs[0] = v->position[0];
		if (s->maxs[1] < v->position[1])
			s->maxs[1] = v->position[1];
		if (s->maxs[2] < v->position[2])
			s->maxs[2] = v->position[2];
	}
}


/*
=================
Mod_LoadFaces
=================
*/
static void Mod_LoadFaces (lump_t *l, cbool bsp2)
{
	dsface_t	*ins;
	dlface_t	*inl;
	msurface_t 	*out;
	int			i, count, surfnum, lofs;
	int			planenum, side, texinfon;

	if (bsp2)
	{
		ins = NULL;
		inl = (dlface_t *)(mod_base + l->fileofs);
		if (l->filelen % sizeof(*inl))
		Host_Error ("Mod_LoadFaces: funny lump size in %s",loadmodel->name);
		count = l->filelen / sizeof(*inl);
	}
	else
	{
		ins = (dsface_t *)(mod_base + l->fileofs);
		inl = NULL;
		if (l->filelen % sizeof(*ins))
			Host_Error ("Mod_LoadFaces: funny lump size in %s",loadmodel->name);
		count = l->filelen / sizeof(*ins);
	}

#ifdef WINQUAKE_SOFTWARE_SKYBOX
	out = (msurface_t *)Hunk_AllocName ( (count+6)*sizeof(*out), loadname); // Manoel Kasimier - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
#else
	out = (msurface_t *)Hunk_AllocName ( count*sizeof(*out), loadname);
#endif // WINQUAKE_SOFTWARE_SKYBOX

	//johnfitz -- warn mappers about exceeding old limits
//	if (count > 32767)
//		Con_DWarningLine ("%d faces exceeds standard limit of 32767.", count);

	if (count > MAX_WINQUAKE_MAP_FACES && !bsp2)
		Con_DWarningLine ("%d faces exceeds standard limit of %d.", count, MAX_WINQUAKE_MAP_FACES);

	//johnfitz

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	for (surfnum = 0 ; surfnum < count ; surfnum++, out++)
	{
		if (bsp2)
		{
			out->firstedge = LittleLong(inl->firstedge);
			out->numedges = LittleLong(inl->numedges);
			planenum = LittleLong(inl->planenum);
			side = LittleLong(inl->side);
			texinfon = LittleLong (inl->texinfo);
			for (i = 0 ; i < MAXLIGHTMAPS ; i++)
				out->styles[i] = inl->styles[i];
			lofs = LittleLong(inl->lightofs);
			inl++;
		}
		else
		{
			out->firstedge = LittleLong(ins->firstedge);
			out->numedges = LittleShort(ins->numedges);
			planenum = LittleShort(ins->planenum);
			side = LittleShort(ins->side);
			texinfon = LittleShort (ins->texinfo);
			for (i=0 ; i<MAXLIGHTMAPS ; i++)
				out->styles[i] = ins->styles[i];

#ifdef GLQUAKE_RENDERER_SUPPORT // Colored lighting actually ...
			lofs = LittleLong(ins->lightofs);
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
			if (loadmodel->bspversion == BSPVERSION_HALFLIFE)
				lofs = LittleLong(ins->lightofs)/3;
			else lofs = LittleLong(ins->lightofs);
#endif // WINQUAKE_RENDERER_SUPPORT
			ins++;
		}

		out->flags = 0;

		if (side)
			out->flags |= SURF_PLANEBACK;

		out->plane = loadmodel->planes + planenum;

		out->texinfo = loadmodel->texinfo + texinfon;

		CalcSurfaceExtents (out);

		Mod_CalcSurfaceBounds (out); //johnfitz -- for per-surface frustum culling

	// lighting info
		if (lofs == -1)
			out->samples = NULL;
#ifdef GLQUAKE_RENDERER_SUPPORT // Colored lighting actually ...
		else if (loadmodel->bspversion == BSPVERSION_HALFLIFE)
			out->samples = loadmodel->lightdata + lofs;
		else
			out->samples = loadmodel->lightdata + (lofs * 3); //johnfitz -- lit support via lordhavoc (was "+ i")
#endif // GLQUAKE_RENDERER_SUPPORT
#ifdef WINQUAKE_RENDERER_SUPPORT
		else out->samples =  loadmodel->lightdata + lofs;
#endif // WINQUAKE_RENDERER_SUPPORT

	// set the drawing flags flag
		if (!strncasecmp(out->texinfo->texture->name, "sky", 3)) // sky surface  //also note -- was Q_strncmp, changed to match qbsp
		{
			out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);
			level.sky = true;
#ifdef GLQUAKE_RENDERER_SUPPORT
			Mod_PolyForUnlitSurface (out); //no more subdivision
#endif // GLQUAKE_RENDERER_SUPPORT
			continue;
		}
		else if (out->texinfo->texture->name[0] == '*')		// warp surface
		{
			out->flags |= (SURF_DRAWTURB | SURF_DRAWTILED);
			if (!strncmp (out->texinfo->texture->name, "*lava", 5))
				level.lava = true, out->flags |= SURF_DRAWLAVA;
			else if (!strncmp (out->texinfo->texture->name, "*slime", 6))
				level.slime = true, out->flags |= SURF_DRAWSLIME;
			else if (Is_Texture_Prefix (out->texinfo->texture->name, r_texprefix_tele.string))
				level.teleporter = true, out->flags |= SURF_DRAWTELE;  // *tele ... so that r_wateralpha doesn't affect teleporters.
			else level.water = true, out->flags |= SURF_DRAWWATER;
#ifdef GLQUAKE_RENDERER_SUPPORT
			if (!isDedicated) { // Baker: Duh -- GL/DX8 dedicated server.
				Mod_PolyForUnlitSurface (out);
				GL_SubdivideSurface (out);
			}
#endif // GLQUAKE_RENDERER_SUPPORT
#ifdef WINQUAKE_RENDERER_SUPPORT
			for (i = 0;  i < 2; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
#endif // WINQUAKE_RENDERER_SUPPORT
			continue; // Baker: Done with warp surface
		}
#ifdef GLQUAKE_RENDERER_SUPPORT
		else if (out->texinfo->flags & TEX_MISSING) // texture is missing from bsp
		{
			if (out->samples) //lightmapped
			{
				out->flags |= SURF_NOTEXTURE;
			}
			else // not lightmapped
			{
				out->flags |= (SURF_NOTEXTURE | SURF_DRAWTILED);
				Mod_PolyForUnlitSurface (out);
			}
			continue;
		}
#endif // GLQUAKE_RENDERER_SUPPORT

		// Normal texture
		if (out->texinfo->texture->name[0] == '{') // alpha masked surface
			out->flags |= SURF_DRAWFENCE;

#ifdef GLQUAKE_RENDERER_SUPPORT
		if (loadmodel->isworldmodel /* prevent healthboxes mirrors*/ && renderer.gl_stencilbits && GL_Mirrors_Is_TextureName_Mirror (out->texinfo->texture->name)) {
			level.mirror = true;
			out->flags |= SURF_DRAWMIRROR; // mirror_		// Yes submodel surfaces get marked here.
		}

		if (Is_Texture_Prefix (out->texinfo->texture->name, gl_texprefix_envmap.string))
			out->flags |= SURF_DRAWENVMAP; // env_  Shiny
		if (Is_Texture_Prefix (out->texinfo->texture->name, gl_texprefix_scrollx.string))
			out->flags |= SURF_SCROLLX; // scrollx_
		if (Is_Texture_Prefix (out->texinfo->texture->name, gl_texprefix_scrolly.string))
			out->flags |= SURF_SCROLLY; // scrolly_
#endif // GLQUAKE_RENDERER_SUPPORT
	}
}


/*
=================
Mod_SetParent
=================
*/
static void Mod_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;

	if (node->contents < 0)
		return;

	Mod_SetParent (node->children[0], node);
	Mod_SetParent (node->children[1], node);
}

/*
=================
Mod_LoadNodes
=================
*/
static void Mod_LoadNodes_S (lump_t *l)
{
	int			i, j, count, p;
	dsnode_t	*in;
	mnode_t 	*out;

	in = (dsnode_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadNodes: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = (mnode_t *)Hunk_AllocName ( count*sizeof(*out), loadname);

	//johnfitz -- warn mappers about exceeding old limits
//	if (count > 32767)
//		Con_DWarningLine ("%d nodes exceeds standard limit of 32767.", count);

	if (count > MAX_WINQUAKE_MAP_NODES)
		Con_DWarningLine ("%d nodes exceeds standard limit of %d.", count, MAX_WINQUAKE_MAP_NODES);

	//johnfitz

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

		out->firstsurface = (unsigned short)LittleShort (in->firstface); //johnfitz -- explicit cast as unsigned short
		out->numsurfaces = (unsigned short)LittleShort (in->numfaces); //johnfitz -- explicit cast as unsigned short

		for (j=0 ; j<2 ; j++)
		{
			//johnfitz -- hack to handle nodes > 32k, adapted from darkplaces
			p = (unsigned short)LittleShort(in->children[j]);
			if (p < count)
				out->children[j] = loadmodel->nodes + p;
			else
			{
				p = 0xffff /*65535*/ - p; //note this uses 65535 intentionally, -1 is leaf 0
				if (p < loadmodel->numleafs)
					out->children[j] = (mnode_t *)(loadmodel->leafs + p);
				else
				{
					Con_PrintLinef ("Mod_LoadNodes: invalid leaf index %d (file has only %d leafs)", p, loadmodel->numleafs);
					out->children[j] = (mnode_t *)(loadmodel->leafs); //map it to the solid leaf
				}
			}
			//johnfitz
		}
	}
}

static void Mod_LoadNodes_L1 (lump_t *l)
{
	int			i, j, count, p;
	dl1node_t	*in;
	mnode_t 	*out;

	in = (dl1node_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadNodes: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = (mnode_t *)Hunk_AllocName ( count*sizeof(*out), loadname);

	//johnfitz -- warn mappers about exceeding old limits
//	if (count > 32767)
//		Con_DWarningLine ("%d nodes exceeds standard limit of 32767.", count);

	if (count > MAX_WINQUAKE_MAP_NODES)
		Con_DWarningLine ("%d nodes exceeds standard limit of %d.", count, MAX_WINQUAKE_MAP_NODES);

	//johnfitz

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

		out->firstsurface = LittleLong (in->firstface); //johnfitz -- explicit cast as unsigned short
		out->numsurfaces = LittleLong (in->numfaces); //johnfitz -- explicit cast as unsigned short

		for (j=0 ; j<2 ; j++)
		{
			//johnfitz -- hack to handle nodes > 32k, adapted from darkplaces
			p = LittleLong(in->children[j]);
			if (p >= 0 && p < count)
				out->children[j] = loadmodel->nodes + p;
			else
			{
				p = 0xffffffff - p; //note this uses 65535 intentionally, -1 is leaf 0
				if (p >= 0 && p < loadmodel->numleafs)
					out->children[j] = (mnode_t *)(loadmodel->leafs + p);
				else
				{
					Con_PrintLinef ("Mod_LoadNodes: invalid leaf index %d (file has only %d leafs)", p, loadmodel->numleafs);
					out->children[j] = (mnode_t *)(loadmodel->leafs); //map it to the solid leaf
				}
			}
			//johnfitz
		}
	}
}

static void Mod_LoadNodes_L2 (lump_t *l)
{
	int			i, j, count, p;
	dl2node_t	*in;
	mnode_t 	*out;

	in = (dl2node_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadNodes: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = (mnode_t *)Hunk_AllocName ( count*sizeof(*out), loadname);

	//johnfitz -- warn mappers about exceeding old limits
//	if (count > 32767)
//		Con_DWarningLine ("%d nodes exceeds standard limit of 32767.", count);

	if (count > MAX_WINQUAKE_MAP_NODES)
		Con_DWarningLine ("%d nodes exceeds standard limit of %d.", count, MAX_WINQUAKE_MAP_NODES);

	//johnfitz

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleFloat (in->mins[j]);
			out->minmaxs[3+j] = LittleFloat (in->maxs[j]);
		}

		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

		out->firstsurface = LittleLong (in->firstface); //johnfitz -- explicit cast as unsigned short
		out->numsurfaces = LittleLong (in->numfaces); //johnfitz -- explicit cast as unsigned short

		for (j=0 ; j<2 ; j++)
		{
			//johnfitz -- hack to handle nodes > 32k, adapted from darkplaces
			p = LittleLong(in->children[j]);
			if (p > 0 && p < count)
				out->children[j] = loadmodel->nodes + p;
			else
			{
				p = 0xffffffff - p; //note this uses 65535 intentionally, -1 is leaf 0
				if (p >= 0 && p < loadmodel->numleafs)  // Baker: Quakespasm has p >=0
					out->children[j] = (mnode_t *)(loadmodel->leafs + p);
				else
				{
					Con_PrintLinef ("Mod_LoadNodes: invalid leaf index %d (file has only %d leafs)", p, loadmodel->numleafs);
					out->children[j] = (mnode_t *)(loadmodel->leafs); //map it to the solid leaf
				}
			}
			//johnfitz
		}
	}
}

static void Mod_LoadNodes (lump_t *l, int bsp2)
{
	if (bsp2 == 2)
		Mod_LoadNodes_L2(l);
	else if (bsp2)
		Mod_LoadNodes_L1(l);
	else
		Mod_LoadNodes_S(l);

	Mod_SetParent (loadmodel->nodes, NULL);	// Spike commented this out?
}

static void Mod_ProcessLeafs_S (dsleaf_t *in, int filelen)
{
	mleaf_t 	*out;
	int			i, j, count, p;

	if (filelen % sizeof(*in))
		Host_Error ("Mod_ProcessLeafs: funny lump size in %s", loadmodel->name);

	count = filelen / sizeof(*in);

	out = (mleaf_t *) Hunk_AllocName (count * sizeof(*out), loadname);


	if (count > MAX_MAP_LEAFS)
		Host_Error ("Mod_LoadLeafs: %d leafs exceeds limit of %d.", count, MAX_MAP_LEAFS);
	if (count > MAX_WINQUAKE_MAP_LEAFS)
		Con_DWarningLine ("Mod_LoadLeafs: %d leafs exceeds standard limit of %d.", count, MAX_WINQUAKE_MAP_LEAFS);

	loadmodel->leafs		= out;
	loadmodel->numleafs		= count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		p					= LittleLong(in->contents);
		out->contents		= p;

		out->firstmarksurface = loadmodel->marksurfaces + (unsigned short)LittleShort(in->firstmarksurface); //johnfitz -- unsigned short
		out->nummarksurfaces = (unsigned short)LittleShort(in->nummarksurfaces); //johnfitz -- unsigned short

		p					= LittleLong(in->visofs);
		if (p == -1)
			out->compressed_vis = NULL;
		else
			out->compressed_vis = loadmodel->visdata + p;
		out->efrags			= NULL;

		for (j=0 ; j<4 ; j++)
			out->ambient_sound_level[j] = in->ambient_level[j];

		// gl underwater warp. Baker: This marks the surface as underwater
		if (out->contents != CONTENTS_EMPTY)
		{
			for (j=0 ; j < out->nummarksurfaces ; j++)
			{
				msurface_t** surf = &out->firstmarksurface[j];
				if (!surf || !(*surf) || !(*surf)->texinfo || !(*surf)->texinfo->texture)
				{
					continue;
				}
				out->firstmarksurface[j]->flags |= SURF_UNDERWATER; // This screws up?
			}
		}
	}

}


static void Mod_ProcessLeafs_L1 (dl1leaf_t *in, int filelen)
{
	mleaf_t 	*out;
	int			i, j, count, p;

	if (filelen % sizeof(*in))
		Host_Error ("Mod_ProcessLeafs: funny lump size in %s", loadmodel->name);

	count = filelen / sizeof(*in);

	out = (mleaf_t *) Hunk_AllocName (count * sizeof(*out), loadname);


	if (count > MAX_MAP_LEAFS)
		Host_Error ("Mod_LoadLeafs: %d leafs exceeds limit of %d.", count, MAX_MAP_LEAFS);
	if (count > MAX_WINQUAKE_MAP_LEAFS)
		Con_DWarningLine ("Mod_LoadLeafs: %d leafs exceeds standard limit of %d.", count, MAX_WINQUAKE_MAP_LEAFS);

	loadmodel->leafs		= out;
	loadmodel->numleafs		= count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		p					= LittleLong(in->contents);
		out->contents		= p;

		out->firstmarksurface = loadmodel->marksurfaces + LittleLong(in->firstmarksurface); //johnfitz -- unsigned short
		out->nummarksurfaces = LittleLong(in->nummarksurfaces); //johnfitz -- unsigned short

		p					= LittleLong(in->visofs);
		if (p == -1)
			out->compressed_vis = NULL;
		else
			out->compressed_vis = loadmodel->visdata + p;
		out->efrags			= NULL;

		for (j=0 ; j<4 ; j++)
			out->ambient_sound_level[j] = in->ambient_level[j];

		// gl underwater warp. Baker: This marks the surface as underwater
		if (out->contents != CONTENTS_EMPTY)
		{
			for (j=0 ; j < out->nummarksurfaces ; j++)
			{
				msurface_t** surf = &out->firstmarksurface[j];
				if (!surf || !(*surf) || !(*surf)->texinfo || !(*surf)->texinfo->texture)
				{
					continue;
				}
				out->firstmarksurface[j]->flags |= SURF_UNDERWATER; // This screws up?
			}
		}
	}

}

static void Mod_ProcessLeafs_L2 (dl2leaf_t *in, int filelen)
{
	mleaf_t 	*out;
	int			i, j, count, p;

	if (filelen % sizeof(*in))
		Host_Error ("Mod_ProcessLeafs: funny lump size in %s", loadmodel->name);

	count = filelen / sizeof(*in);

	out = (mleaf_t *) Hunk_AllocName (count * sizeof(*out), loadname);


	if (count > MAX_MAP_LEAFS)
		Host_Error ("Mod_LoadLeafs: %d leafs exceeds limit of %d.", count, MAX_MAP_LEAFS);
	if (count > MAX_WINQUAKE_MAP_LEAFS)
		Con_DWarningLine ("Mod_LoadLeafs: %d leafs exceeds standard limit of %d.", count, MAX_WINQUAKE_MAP_LEAFS);

	loadmodel->leafs		= out;
	loadmodel->numleafs		= count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleFloat (in->mins[j]);
			out->minmaxs[3+j] = LittleFloat (in->maxs[j]);
		}

		p					= LittleLong(in->contents);
		out->contents		= p;

		out->firstmarksurface = loadmodel->marksurfaces + LittleLong(in->firstmarksurface); //johnfitz -- unsigned short
		out->nummarksurfaces = LittleLong(in->nummarksurfaces); //johnfitz -- unsigned short

		p					= LittleLong(in->visofs);
		if (p == -1)
			out->compressed_vis = NULL;
		else
			out->compressed_vis = loadmodel->visdata + p;
		out->efrags			= NULL;

		for (j=0 ; j<4 ; j++)
			out->ambient_sound_level[j] = in->ambient_level[j];

		// gl underwater warp. Baker: This marks the surface as underwater
		if (out->contents != CONTENTS_EMPTY)
		{
			for (j=0 ; j < out->nummarksurfaces ; j++)
			{
				msurface_t** surf = &out->firstmarksurface[j];
				if (!surf || !(*surf) || !(*surf)->texinfo || !(*surf)->texinfo->texture)
				{
					continue;
				}
				out->firstmarksurface[j]->flags |= SURF_UNDERWATER; // This screws up?
			}
		}
	}

}


/*
=================
Mod_LoadLeafs
=================
*/
static void Mod_LoadLeafs (lump_t *l, int bsp2)
{
	void 	*in = (void *)(mod_base + l->fileofs);

	if (bsp2 == 2)
		Mod_ProcessLeafs_L2 ((dl2leaf_t *)in, l->filelen);
	else if (bsp2)
		Mod_ProcessLeafs_L1 ((dl1leaf_t *)in, l->filelen);
	else
		Mod_ProcessLeafs_S ((dsleaf_t *)in, l->filelen);
}


/*
=================
Mod_LoadClipnodes
=================
*/
static void Mod_LoadClipnodes (lump_t *l, cbool bsp2)
{
	dsclipnode_t *ins;
	dlclipnode_t *inl;

	mclipnode_t *out; //johnfitz -- was dclipnode_t
	int			i, count;
	hull_t		*hull;

	if (bsp2)
	{
		ins = NULL;
		inl = (dlclipnode_t *)(mod_base + l->fileofs);
		if (l->filelen % sizeof(*inl))
		Host_Error ("Mod_LoadClipnodes: funny lump size in %s",loadmodel->name);

		count = l->filelen / sizeof(*inl);
	}
	else
	{
		ins = (dsclipnode_t *)(mod_base + l->fileofs);
		inl = NULL;
		if (l->filelen % sizeof(*ins))
			Host_Error ("Mod_LoadClipnodes: funny lump size in %s",loadmodel->name);

		count = l->filelen / sizeof(*ins);
	}
	out = (mclipnode_t *)Hunk_AllocName ( count*sizeof(*out), loadname);

	//johnfitz -- warn about exceeding old limits
//	if (count > 32767)
//		Con_DWarningLine ("%d clipnodes exceeds standard limit of 32767.", count);
	if (count > MAX_WINQUAKE_MAP_CLIPNODES && !bsp2)
		Con_DWarningLine ("%d clipnodes exceeds standard limit of %d.", count, MAX_WINQUAKE_MAP_CLIPNODES);
	//johnfitz

	loadmodel->clipnodes = out;
	loadmodel->numclipnodes = count;

	// Player Hull
	hull = &loadmodel->hulls[1];
	{
		hull->available			= true;
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;

		VectorSet (hull->clip_mins, -16, -16, -24);
		VectorSet (hull->clip_maxs,  16,  16,  32);

		if (loadmodel->bspversion == BSPVERSION_HALFLIFE)
		{	// Player hull is taller
			hull->clip_mins[2]	= -36;
			hull->clip_maxs[2]	= 36;
		}
	}

	// Monster hull
	hull = &loadmodel->hulls[2];
	{
		hull->available			= true;
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;

		VectorSet (hull->clip_mins, -32, -32, -24);
		VectorSet (hull->clip_maxs,  32,  32,  64);

		if (loadmodel->bspversion == BSPVERSION_HALFLIFE)
		{ // Monster hull is shorter
			hull->clip_mins[2]	= -32;
			hull->clip_maxs[2]	= 32;
		}
	}

	// Half-Life crouch hull
	hull = &loadmodel->hulls[3];
	{
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;

		VectorSet (hull->clip_mins, -16, -16,  -6);
		VectorSet (hull->clip_maxs,  16,  16,  30);

		hull->available = false;

		if (loadmodel->bspversion == BSPVERSION_HALFLIFE)
		{
			hull->clip_mins[2]	= -18;
			hull->clip_maxs[2]	= 18;
			hull->available		= true;
		}
	}

	if (bsp2)
	{
		for (i=0 ; i<count ; i++, out++, inl++)
		{
			out->planenum = LittleLong(inl->planenum);

			//johnfitz -- bounds check
			if (out->planenum < 0 || out->planenum >= loadmodel->numplanes)
				Host_Error ("Mod_LoadClipnodes: planenum out of bounds");
			//johnfitz

			out->children[0] = LittleLong(inl->children[0]);
			out->children[1] = LittleLong(inl->children[1]);
			//Spike: FIXME: bounds check
		}
	}
	else
	{
		for (i=0 ; i<count ; i++, out++, ins++)
		{
				out->planenum = LittleLong(ins->planenum);

			//johnfitz -- bounds check
			if (out->planenum < 0 || out->planenum >= loadmodel->numplanes)
				Host_Error ("Mod_LoadClipnodes: planenum out of bounds");
			//johnfitz

			//johnfitz -- support clipnodes > 32k
				out->children[0] = (unsigned short)LittleShort(ins->children[0]);
				out->children[1] = (unsigned short)LittleShort(ins->children[1]);

			if (out->children[0] >= count)
				out->children[0] -= 65536;
			if (out->children[1] >= count)
				out->children[1] -= 65536;
			//johnfitz
		}
	}
}

/*
=================
Mod_MakeHull0

Duplicate the drawing hull structure as a clipping hull
=================
*/
static void Mod_MakeHull0 (void)
{
	hull_t		*hull = &loadmodel->hulls[0];
	mnode_t		*in= loadmodel->nodes;
	int			count = loadmodel->numnodes;
	mclipnode_t *out = (mclipnode_t *)Hunk_AllocName ( count*sizeof(*out), loadname);

	mnode_t *child;
	int			i, j;

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->planenum = in->plane - loadmodel->planes;
		for (j=0 ; j<2 ; j++)
		{
			child = in->children[j];
			if (child->contents < 0)
				out->children[j] = child->contents;
			else
				out->children[j] = child - loadmodel->nodes;
		}
	}
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
static void Mod_LoadMarksurfaces (lump_t *l, int bsp2)
{
	int		i, j, count;
	msurface_t **out;
	if (bsp2)
	{
		unsigned int		*in = (unsigned int *)(mod_base + l->fileofs);

		if (l->filelen % sizeof(*in))
			Host_Error ("Mod_LoadMarksurfaces: funny lump size in %s",loadmodel->name);

		count = l->filelen / sizeof(*in);
		out = (msurface_t **)Hunk_AllocName ( count*sizeof(*out), loadname);

		loadmodel->marksurfaces = out;
		loadmodel->nummarksurfaces = count;

		//johnfitz -- warn mappers about exceeding old limits
	//	if (count > 32767)
	//		Con_DWarningLine ("%d marksurfaces exceeds standard limit of 32767.", count);

		if (count > MAX_WINQUAKE_MAP_MARKSURFACES && !bsp2)
			Con_DWarningLine ("%d marksurfaces exceeds standard limit of %d.", count, MAX_WINQUAKE_MAP_MARKSURFACES);

		//johnfitz

		for ( i=0 ; i<count ; i++)
		{
			j = LittleLong(in[i]);
			if (j >= loadmodel->numsurfaces)
				Host_Error ("Mod_LoadMarksurfaces: bad surface number");
			out[i] = loadmodel->surfaces + j;
		}
	}
	else
	{
		short		*in = (short *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadMarksurfaces: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = (msurface_t **)Hunk_AllocName ( count*sizeof(*out), loadname);

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	//johnfitz -- warn mappers about exceeding old limits
//	if (count > 32767)
//		Con_DWarningLine ("%d marksurfaces exceeds standard limit of 32767.", count);

	if (count > MAX_WINQUAKE_MAP_MARKSURFACES)
		Con_DWarningLine ("%d marksurfaces exceeds standard limit of %d.", count, MAX_WINQUAKE_MAP_MARKSURFACES);

	//johnfitz

	for ( i=0 ; i<count ; i++)
	{
		j = (unsigned short)LittleShort(in[i]); //johnfitz -- explicit cast as unsigned short
		if (j >= loadmodel->numsurfaces)
			Host_Error ("Mod_LoadMarksurfaces: bad surface number");
		out[i] = loadmodel->surfaces + j;
	}
}
}

/*
=================
Mod_LoadSurfedges
=================
*/
static void Mod_LoadSurfedges (lump_t *l)
{
	int	*in = (int *)(mod_base + l->fileofs);

	int	i, count, *out;

	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadSurfedges: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);

#ifdef WINQUAKE_SOFTWARE_SKYBOX
	out = (int *)Hunk_AllocName ( (count+24)*sizeof(*out), loadname); // Manoel Kasimier - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
#else
	out = (int *)Hunk_AllocName ( count*sizeof(*out), loadname);
#endif // WINQUAKE_SOFTWARE_SKYBOX

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for ( i=0 ; i<count ; i++)
		out[i] = LittleLong (in[i]);
}


/*
=================
Mod_LoadPlanes
=================
*/
static void Mod_LoadPlanes (lump_t *l)
{
	int		i, j, count, bits;
	mplane_t	*out;
	dplane_t 	*in = (dplane_t *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadPlanes: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);

#ifdef WINQUAKE_SOFTWARE_SKYBOX
	out = (mplane_t *)Hunk_AllocName ( (count+6)*2*sizeof(*out), loadname); // Manoel Kasimier - skyboxes - extra for skybox // Code taken from the ToChriS engine - Author: Vic (vic@quakesrc.org) (http://hkitchen.quakesrc.org/)
#else
	out = (mplane_t *)Hunk_AllocName ( count*2*sizeof(*out), loadname);
#endif // WINQUAKE_SOFTWARE_SKYBOX

	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		bits = 0;
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);
		out->signbits = bits;
	}
}

// 2001-12-28 .VIS support by Maddes  start
/*
=================
Mod_LoadExternalVisibility
=================
*/
static void Mod_LoadVisibilityExternal (FILE **fhandle)
{
	long	filelen = 0;

	// get visibility data length
	filelen = 0;
	fread (&filelen, 1, 4, *fhandle);
	filelen = LittleLong(filelen);

	Con_DPrintLinef ("...%d bytes visibility data", (int)filelen);

	// load visibility data
	if (!filelen)
	{
		loadmodel->visdata = NULL;
		return;
	}
	loadmodel->visdata = Hunk_AllocName (filelen, "EXT_VIS");
	fread (loadmodel->visdata, 1, filelen, *fhandle);
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds (vec3_t mins, vec3_t maxs)
{
	int		i;
	vec3_t	corner;

	for (i=0 ; i<3 ; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return VectorLength (corner);
}

/*
=================
Mod_LoadSubmodels
=================
*/
static void Mod_LoadSubmodels (lump_t *l)
{
	dmodel_t	*in, *out;
	int			i, j, count;

	in = (dmodel_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_Error ("Mod_LoadSubmodels: funny lump size in %s",loadmodel->name);

	count = l->filelen / sizeof(*in);
	if (count > MAX_FITZQUAKE_MODELS)
		Host_Error ("Mod_LoadSubmodels: count > MAX_MODELS");

	out = (dmodel_t *)Hunk_AllocName ( count*sizeof(*out), loadname);

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = LittleFloat (in->origin[j]);
		}
		for (j=0 ; j<MAX_MAP_HULLS ; j++)
			out->headnode[j] = LittleLong (in->headnode[j]);
		out->visleafs = LittleLong (in->visleafs);
		out->firstface = LittleLong (in->firstface);
		out->numfaces = LittleLong (in->numfaces);
	}

	// johnfitz -- check world visleafs -- adapted from bjp
	out = loadmodel->submodels;

	if (out->visleafs > MAX_MAP_LEAFS)
		Host_Error ("Mod_LoadSubmodels: too many visleafs (%d, max = %d) in %s", out->visleafs, MAX_MAP_LEAFS, loadmodel->name);

//	if (out->visleafs > 8192)
//		Con_DWarningLine ("%d visleafs exceeds standard limit of 8192.", out->visleafs);

	if (out->visleafs > MAX_WINQUAKE_MAP_LEAFS)
		Con_DWarningLine ("%d visleafs exceeds standard limit of %d.", out->visleafs, MAX_WINQUAKE_MAP_LEAFS);

	//johnfitz
}


/*
=================
Mod_LoadExternalLeafs
=================
*/
static void Mod_LoadLeafsExternal (FILE **fhandle)
{
	dsleaf_t 	*in;
	long	filelen;

	// get leaf data length
	filelen = 0;
	fread (&filelen, 1, 4, *fhandle);
	filelen = LittleLong(filelen);

	Con_DPrintLinef ("...%d bytes leaf data", (int)filelen);

	// load leaf data
	if (!filelen)
	{
		loadmodel->leafs = NULL;
		loadmodel->numleafs = 0;
		return;
	}
	in = Hunk_AllocName (filelen, "EXT_LEAF");
	fread  (in, 1, filelen, *fhandle);

	Mod_ProcessLeafs_S (in, filelen);
}

// 2001-12-28 .VIS support by Maddes  start
#define VISPATCH_MAPNAME_LENGTH	32

typedef struct vispatch_s
{
	char	mapname[VISPATCH_MAPNAME_LENGTH];	// Baker: DO NOT CHANGE THIS to MAX_QPATH_64, must be 32
	int		filelen;		// length of data after VisPatch header (VIS+Leafs)
} vispatch_t;
// 2001-12-28 .VIS support by Maddes  end
#define VISPATCH_HEADER_LEN_36 36

static cbool Mod_FindVisibilityExternal (FILE **filehandle)
{
	char		visfilename[MAX_QPATH_64];
	c_snprintf1 (visfilename, "maps/%s.vis", loadname);
	COM_FOpenFile_Limited (visfilename, filehandle, loadmodel->loadinfo.searchpath); // COM_FOpenFile (name, &cls.demofile);

	if (!*filehandle)
	{
		Con_VerbosePrintLinef ("Standard vis, %s not found", visfilename);
		return false; // None
	}

	Con_VerbosePrintLinef ("External .vis found: %s", visfilename);
	{
		int			i, pos;
		vispatch_t	header;
		const char* shortname = File_URL_SkipPath(loadmodel->name); // start.bsp, e1m1.bsp, etc.

		pos = 0;

		while ((i = fread (&header, 1, VISPATCH_HEADER_LEN_36, *filehandle))) // i will be length of read, continue while a read
		{
			header.filelen = LittleLong(header.filelen);	// Endian correct header.filelen
			pos += i;										// Advance the length of the break

			if (!strcasecmp (header.mapname, shortname))
				break;

			pos += header.filelen;							// Advance the length of the filelength
			fseek (*filehandle, pos, SEEK_SET);
		}

		if (i != VISPATCH_HEADER_LEN_36)
		{
			FS_fclose (*filehandle);
			return false;
		}
	}

	return true;

}
// 2001-12-28 .VIS support by Maddes  end

#if 0 // Baker: This isn't called in code.
/*
=================
Mod_BoundsFromClipNode -- johnfitz

update the model's clipmins and clipmaxs based on each node's plane.

This works because of the way brushes are expanded in hull generation.
Each brush will include all six axial planes, which bound that brush.
Therefore, the bounding box of the hull can be constructed entirely
from axial planes found in the clipnodes for that hull.
=================
*/
void Mod_BoundsFromClipNode (qmodel_t *mod, int hull, int nodenum)
{
	mplane_t	*plane;
	mclipnode_t	*node;

	if (nodenum < 0)
		return; //hit a leafnode

	node = &mod->clipnodes[nodenum];
	plane = mod->hulls[hull].planes + node->planenum;
	switch (plane->type)
	{

	case PLANE_X_0:
		if (plane->signbits == 1)
			mod->clipmins[0] = c_min (mod->clipmins[0], -plane->dist - mod->hulls[hull].clip_mins[0]);
		else
			mod->clipmaxs[0] = c_max (mod->clipmaxs[0], plane->dist - mod->hulls[hull].clip_maxs[0]);
		break;
	case PLANE_Y_1:
		if (plane->signbits == 2)
			mod->clipmins[1] = c_min (mod->clipmins[1], -plane->dist - mod->hulls[hull].clip_mins[1]);
		else
			mod->clipmaxs[1] = c_max (mod->clipmaxs[1], plane->dist - mod->hulls[hull].clip_maxs[1]);
		break;
	case PLANE_Z_2:
		if (plane->signbits == 4)
			mod->clipmins[2] = c_min (mod->clipmins[2], -plane->dist - mod->hulls[hull].clip_mins[2]);
		else
			mod->clipmaxs[2] = c_max (mod->clipmaxs[2], plane->dist - mod->hulls[hull].clip_maxs[2]);
		break;
	default:
		//skip nonaxial planes; don't need them
		break;
	}

	Mod_BoundsFromClipNode (mod, hull, node->children[0]);
	Mod_BoundsFromClipNode (mod, hull, node->children[1]);
}
#endif

/*
=================
Mod_LoadBrushModel
=================
*/

void Mod_LoadBrushModel (qmodel_t *mod, void *buffer)
{
	int			i, j;
	dheader_t	*header;
	dmodel_t 	*bm;
	float		radius; //johnfitz
	int			bsp2;
//	qmodel_t	*modcl = mod; // Baker: Because submodel loader likes to change mod and loadmodel

	loadmodel->type = mod_brush;

	header = (dheader_t *)buffer;

	mod->bspversion = LittleLong (header->version);

	switch(mod->bspversion)
	{
	case BSPVERSION:
	case BSPVERSION_HALFLIFE:
		bsp2 = false;
		break;
	case BSP2VERSION_2PSB:
		bsp2 = 1;	//first iteration
		break;
	case BSP2VERSION_BSP2:
		bsp2 = 2;	//sanitised revision
		break;
	default:
		Host_Error ("Mod_LoadBrushModel: %s has wrong version number (%d should be %d (Quake) or %d (HalfLife))", mod->name, mod->bspversion, BSPVERSION, BSPVERSION_HALFLIFE);
		break;
	}

	{// cl.worldmodel
		cbool servermatch = sv.modelname[0] && !strcasecmp (loadname, sv.name);
		cbool clientmatch = cl.worldname[0] && !strcasecmp (loadname, cl.worldname);
		Con_DPrintLinef ("loadname     = " QUOTED_S " ", loadname);
		Con_DPrintLinef ("sv.modelname = " QUOTED_S " ", sv.modelname);
		Con_DPrintLinef ("cl.modelname = " QUOTED_S " (this can be blank for first pass)", cl.worldname);
		loadmodel->isworldmodel = servermatch || clientmatch;
	}

// swap all the lumps
	mod_base = (byte *)header;

	for (i=0 ; i<(int)sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

// load into heap
	Mod_LoadVertexes (&header->lumps[LUMP_VERTEXES]);
	Mod_LoadEdges (&header->lumps[LUMP_EDGES], bsp2);
	Mod_LoadSurfedges (&header->lumps[LUMP_SURFEDGES]);
	Mod_LoadTextures (&header->lumps[LUMP_TEXTURES]);
	Mod_LoadLighting (&header->lumps[LUMP_LIGHTING]);
	Mod_LoadPlanes (&header->lumps[LUMP_PLANES]);
	Mod_LoadTexinfo (&header->lumps[LUMP_TEXINFO]);
	Mod_LoadFaces (&header->lumps[LUMP_FACES], bsp2);
	Mod_LoadMarksurfaces (&header->lumps[LUMP_MARKSURFACES], bsp2);

	do
	{
		// 2001-12-28 .VIS support by Maddes  start
		FILE		*visfhandle;	// Baker: try to localize this var

		loadmodel->visdata = NULL;
		loadmodel->leafs = NULL;
		loadmodel->numleafs = 0;

		if (loadmodel->isworldmodel && model_external_vis.value)
		{
			Con_DPrintLinef ("trying to open external vis file");

			if (Mod_FindVisibilityExternal (&visfhandle /* We should be passing infos here &visfilehandle*/) )
			{
				// File exists, valid and open
				Con_DPrintLinef ("found valid external .vis file for map");
				Mod_LoadVisibilityExternal (&visfhandle);
				Mod_LoadLeafsExternal (&visfhandle);

				FS_fclose  (visfhandle);

				if (loadmodel->visdata && loadmodel->leafs && loadmodel->numleafs)
				{
					break; // skip standard vis
				}
				Con_PrintLinef ("External VIS data are invalid! Doing standard vis.");
			}
		}
		// Extern vis didn't exist or was invalid ..

		//
		// standard vis
		//
		Mod_LoadVisibility (&header->lumps[LUMP_VISIBILITY]);
		Mod_LoadLeafs (&header->lumps[LUMP_LEAFS], bsp2);
	} while (0);

	Mod_LoadNodes (&header->lumps[LUMP_NODES], bsp2);
	Mod_LoadClipnodes (&header->lumps[LUMP_CLIPNODES], bsp2);
	Mod_LoadEntities (&header->lumps[LUMP_ENTITIES]);
	Mod_LoadSubmodels (&header->lumps[LUMP_MODELS]);

	Mod_MakeHull0 ();

	mod->numframes = 2;		// regular and alternate animation

//
// set up the submodels (FIXME: this is confusing)
//

	// johnfitz -- okay, so that i stop getting confused every time i look at this loop, here's how it works:
	// we're looping through the submodels starting at 0.  Submodel 0 is the main model, so we don't have to
	// worry about clobbering data the first time through, since it's the same data.  At the end of the loop,
	// we create a new copy of the data to use the next time through.
	for (i=0 ; i<mod->numsubmodels ; i++)
	{
		bm = &mod->submodels[i];

		mod->hulls[0].firstclipnode = bm->headnode[0];
		for (j=1 ; j<MAX_MAP_HULLS ; j++)
		{
			mod->hulls[j].firstclipnode = bm->headnode[j];
			mod->hulls[j].lastclipnode = mod->numclipnodes-1;
		}

		mod->firstmodelsurface = bm->firstface;
		mod->nummodelsurfaces = bm->numfaces;

		VectorCopy (bm->maxs, mod->maxs);
		VectorCopy (bm->mins, mod->mins);

#ifdef WINQUAKE_RENDERER_SUPPORT
#pragma message ("Baker: WinQuake ... correct R_BmodelCheckBBox to use this and kill off radius")
		mod->radius =  // continued ... mod->radius = ... radius = ...

#endif // WINQUAKE_RENDERER_SUPPORT

		//johnfitz -- calculate rotate bounds and yaw bounds
		radius = RadiusFromBounds (mod->mins, mod->maxs);
		mod->rmaxs[0] = mod->rmaxs[1] = mod->rmaxs[2] = mod->ymaxs[0] = mod->ymaxs[1] = mod->ymaxs[2] = radius;
		mod->rmins[0] = mod->rmins[1] = mod->rmins[2] = mod->ymins[0] = mod->ymins[1] = mod->ymins[2] = -radius;
		//johnfitz

		//johnfitz -- correct physics cullboxes so that outlying clip brushes on doors and stuff are handled right
		if (i > 0 || strcmp(mod->name, sv.modelname) != 0) //skip submodel 0 of sv.worldmodel, which is the actual world
		{
			// start with the hull0 bounds
			VectorCopy (mod->maxs, mod->clipmaxs);
			VectorCopy (mod->mins, mod->clipmins);

			// process hull1 (we don't need to process hull2 becuase there's
			// no such thing as a brush that appears in hull2 but not hull1)
			//Mod_BoundsFromClipNode (mod, 1, mod->hulls[1].firstclipnode); // (disabled for now becuase it fucks up on rotating models)
		}
		//johnfitz

		mod->numleafs = bm->visleafs;

		if (i < mod->numsubmodels - 1)
		{	// duplicate the basic information
			char	name[10];

			c_snprintf1 (name, "*%d", i + 1);
			loadmodel = Mod_FindName (name);
			*loadmodel = *mod;
			c_strlcpy (loadmodel->name, name);
			mod = loadmodel;
		}
	}


}

// Returns count of the number of mirror surfaces
#ifdef GLQUAKE_RENDERER_SUPPORT	
void Mirror_Scan_SubModels (qmodel_t *world_model)
{
	int Mirror_Scan_Model (qmodel_t *mdl);
#define MODELINDEX_WORLDMODEL_1				1							// 
#define MODELINDEX_SUBMODEL_START_2			(MODELINDEX_WORLDMODEL_1 + 1)
#define MODELINDEX_SUBMODEL_LAST			(MODELINDEX_SUBMODEL_START_2 + world_model->numsubmodels - 1)
	int first_submodel = MODELINDEX_SUBMODEL_START_2;
	int last_submodel = MODELINDEX_SUBMODEL_LAST;
	int j;
	for (j = first_submodel; j <= last_submodel; j++) {
		qmodel_t *mdl = cl.model_precache[j];

		Mirror_Scan_Model (mdl);
	}
}


int Mirror_Scan_Model (qmodel_t *mdl)
{
	void GL_Mirrors_Scan_Surface (msurface_t *surf, int surfnum);

	//if (!in_range (first_submodel, ent->modelindex, last_submodel))
	//	return 0;

	//if (ent->model->mirror_numsurfaces)
	//	return 0; // Already scanned.  This means negke like trickery or something where someone duplicated a door or made a healthbox inside the map model.

	//if (!ent->model || ent->model->type != mod_brush)
	//	return 0; // Should never happen.

	//if (ent->origin[0] || ent->origin[1] || ent->origin[2])
	//	return 0; // Moved.  Disqualified.

	//if (ent->angles[0] || ent->angles[1] || ent->angles[2])
	//	return 0; // Angles.  Disqualified.

	//if (ent->model->firstmodelsurface == 0)
	//	return 0; // Health box or something.  It isn't part of the world.


	msurface_t *surf = &mdl->surfaces[mdl->firstmodelsurface];  int j, count; //    &ent->model->surfaces[ent->model->firstmodelsurface];  
	for (count = 0, j = 0 ;  j < mdl->nummodelsurfaces ; j++, surf++) {
		if (Flag_Check (surf->flags, SURF_DRAWMIRROR)) {
			GL_Mirrors_Scan_Surface (surf, j);
#if 1
			count ++;
			mdl->mirror_numsurfaces = count;
			mdl->mirror_only_surface = surf;
			mdl->mirror_plane = surf->plane; // Since this is a static, it's ok to set the entity.  Right?  Nah.  And let's use ent->static_mirror_numsurfs
			return 1; // Yes
#endif
			
		}
		// Mark the model mod->mirror_surfaces too
	}

	// Should only be reached if 0 at this time.
	return count;
}
#endif // GLQUAKE_RENDERER_SUPPORT



// Baker: I should move alias model loading into different file
// with one for GL and one for WinQuake

#ifdef GLQUAKE_RENDERER_SUPPORT
/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

aliashdr_t	*pheader;

stvert_t	stverts[MAXALIASVERTS_3984];
mtriangle_t	triangles[MAXALIASTRIS_4096];

// a pose is a single set of vertexes.  a frame may be
// an animating sequence of poses
trivertx_t	*poseverts[MAXALIASFRAMES_256];
int			posenum;

byte		**player_8bit_texels_tbl;
byte		*player_8bit_texels;

/*
=================
Mod_LoadAliasFrame
=================
*/
void * Mod_LoadAliasFrame (void * pin, maliasframedesc_t *frame)
{
	trivertx_t		*pinframe;
	int				i;
	daliasframe_t	*pdaliasframe;

	pdaliasframe = (daliasframe_t *)pin;

	c_strlcpy (frame->name, pdaliasframe->name);
	frame->firstpose = posenum;
	frame->numposes = 1;

	for (i=0 ; i<3 ; i++)
	{
		// these are byte values, so we don't have to worry about
		// endianness
		frame->bboxmin.v[i] = pdaliasframe->bboxmin.v[i];
		frame->bboxmax.v[i] = pdaliasframe->bboxmax.v[i];
	}


	pinframe = (trivertx_t *)(pdaliasframe + 1);

	poseverts[posenum] = pinframe;
	posenum++;

	pinframe += pheader->numverts;

	return (void *)pinframe;
}


/*
=================
Mod_LoadAliasGroup
=================
*/
void *Mod_LoadAliasGroup (void * pin,  maliasframedesc_t *frame)
{
	daliasgroup_t		*pingroup;
	int					i, numframes;
	daliasinterval_t	*pin_intervals;
	void				*ptemp;

	pingroup = (daliasgroup_t *)pin;

	numframes = LittleLong (pingroup->numframes);

	frame->firstpose = posenum;
	frame->numposes = numframes;

	for (i=0 ; i<3 ; i++)
	{
		// these are byte values, so we don't have to worry about endianness
		frame->bboxmin.v[i] = pingroup->bboxmin.v[i];
		frame->bboxmax.v[i] = pingroup->bboxmax.v[i];
	}


	pin_intervals = (daliasinterval_t *)(pingroup + 1);

	frame->interval = LittleFloat (pin_intervals->interval);

	pin_intervals += numframes;

	ptemp = (void *)pin_intervals;

	for (i=0 ; i<numframes ; i++)
	{
		poseverts[posenum] = (trivertx_t *)((daliasframe_t *)ptemp + 1);
		posenum++;

		ptemp = (trivertx_t *)((daliasframe_t *)ptemp + 1) + pheader->numverts;
	}

	return ptemp;
}

//=========================================================


/*
=================
Mod_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes - Ed
=================
*/

typedef struct
{
	short		x, y;
} floodfill_t;

// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
do {								\
	if (pos[off] == fillcolor) \
	{ \
		pos[off] = 255; \
		fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
		inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
	} \
	else if (pos[off] != 255) fdc = pos[off]; \
} while (0)

void Mod_FloodFillSkin( byte *skin, int skinwidth, int skinheight )
{
	byte				fillcolor = *skin; // assume this is the pixel to fill
	floodfill_t			fifo[FLOODFILL_FIFO_SIZE];
	int					inpt = 0, outpt = 0;
	int					filledcolor = -1;
	int					i;

	if (filledcolor == -1)
	{
		filledcolor = 0;
		// attempt to find opaque black
		for (i = 0; i < 256; ++i)
			if (vid.d_8to24table[i] == (255 << 0)) // alpha 1.0
			{
				filledcolor = i;
				break;
			}
	}

	// can't fill to filled color or to transparent color (used as visited marker)
	if ((fillcolor == filledcolor) || (fillcolor == 255))
	{
		//printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
		return;
	}

	fifo[inpt].x = 0, fifo[inpt].y = 0;
	inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

	while (outpt != inpt)
	{
		int			x = fifo[outpt].x, y = fifo[outpt].y;
		int			fdc = filledcolor;
		byte		*pos = &skin[x + skinwidth * y];

		outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

		if (x > 0)				FLOODFILL_STEP( -1, -1, 0 );
		if (x < skinwidth - 1)	FLOODFILL_STEP( 1, 1, 0 );
		if (y > 0)				FLOODFILL_STEP( -skinwidth, 0, -1 );
		if (y < skinheight - 1)	FLOODFILL_STEP( skinwidth, 0, 1 );
		skin[x + skinwidth * y] = fdc;
	}
}


void Mod_LoadSkinTexture (cbool is_alphamasked, gltexture_t **tex, gltexture_t **fb, char *name, char *fbr_mask_name, char *extname, byte *data, int width, int height, unsigned int offset)
{
	cbool maybe_texpref_alpha = is_alphamasked ? TEXPREF_ALPHA : 0;
	char extload[MAX_OSPATH] = {0};
	int extwidth = 0;
	int extheight = 0;
	unsigned *extdata;
	int hunkmark = Hunk_LowMark ();
	char texpath[MAX_QPATH_64];

	if (gl_external_textures.value)
	{
		c_strlcpy (texpath, loadmodel->name);
		File_URL_Edit_Reduce_To_Parent_Path (texpath); // Baker: strips file name

		c_snprintf2 (extload, "%s/%s", texpath, extname);
		extdata = Image_Load_Limited (extload, &extwidth, &extheight, loadmodel->loadinfo.searchpath);

		if (extdata)
		{
			tex[0] = TexMgr_LoadImage (loadmodel, -1 /*not bsp texture*/, extload, extwidth, extheight,
				SRC_RGBA, extdata, extload, 0,  /*TEXPREF_PAD |*/ TEXPREF_NOBRIGHT | TEXPREF_FLOODFILL | maybe_texpref_alpha);

			fb[0] = NULL;

			// now try to load glow/luma image from the same place
			Hunk_FreeToLowMark (hunkmark);

			c_snprintf2 (extload, "%s/%s_glow", texpath, extname);
			extdata = Image_Load_Limited (extload, &extwidth, &extheight, loadmodel->loadinfo.searchpath);

			if (!extdata)
			{
				c_snprintf2 (extload, "%s/%s_luma", texpath, extname);
				extdata = Image_Load_Limited (extload, &extwidth, &extheight, loadmodel->loadinfo.searchpath);
			}

			if (extdata)
			{
				fb[0] = TexMgr_LoadImage (loadmodel, -1 /*not bsp texture*/, extload, extwidth, extheight, SRC_RGBA, extdata, extload, 0,  /*TEXPREF_PAD |*/ TEXPREF_NOBRIGHT /*  | TEXPREF_FLOODFILL*/ | maybe_texpref_alpha);
				Hunk_FreeToLowMark (hunkmark);
			}

			return;
		}

	}
	// hurrah! for readable code
	if (Mod_CheckFullbrights (data, width * height, is_alphamasked)) // EF_ALPHA_MASKED_MDL
	{
		tex[0] = TexMgr_LoadImage (loadmodel, -1 /*not bsp texture*/, name, width, height, SRC_INDEXED, data, loadmodel->name, offset, TEXPREF_PAD | TEXPREF_NOBRIGHT | maybe_texpref_alpha);
		fb[0] = TexMgr_LoadImage (loadmodel, -1 /*not bsp texture*/, fbr_mask_name, width, height, SRC_INDEXED, data, loadmodel->name, offset, TEXPREF_PAD | TEXPREF_FULLBRIGHT | maybe_texpref_alpha);
	}
	else
	{
		tex[0] = TexMgr_LoadImage (loadmodel, -1 /*not bsp texture*/, name, width, height, SRC_INDEXED, data, loadmodel->name, offset, TEXPREF_PAD | maybe_texpref_alpha);
		fb[0] = NULL;
	}

	// just making sure...
	Hunk_FreeToLowMark (hunkmark);
}

/*
===============
Mod_LoadAllSkins
===============
*/


void *Mod_LoadAllSkins (int numskins, daliasskintype_t *pskintype, cbool is_alphamasked)
{
	int						i, j, k, size, groupskins;
	char					name[MAX_OSPATH];	// 32 this isn;t long enough for some mods
	byte					*skin, *texels;
	daliasskingroup_t		*pinskingroup;
	daliasskininterval_t	*pinskinintervals;
	char					fbr_mask_name[MAX_OSPATH]; // johnfitz -- added for fullbright support
	char					extname[MAX_OSPATH];
	src_offset_t			offset; //johnfitz

	skin = (byte *)(pskintype + 1);

	if (numskins < 1 || numskins > MAX_SKINS)
		Host_Error ("Mod_LoadAliasModel: Invalid # of skins: %d", numskins);

	size = pheader->skinwidth * pheader->skinheight;

	for (i=0 ; i<numskins ; i++)
	{
		if (pskintype->type == ALIAS_SKIN_SINGLE)
		{
//			Mod_FloodFillSkin( skin, pheader->skinwidth, pheader->skinheight );

			// save 8 bit texels for the player model to remap
			texels = (byte *) Hunk_AllocName(size, loadname);
			pheader->texels[i] = texels - (byte *)pheader;
			memcpy (texels, (byte *)(pskintype + 1), size);

			//johnfitz -- rewritten
			c_snprintf2 (name, "%s:frame%d", loadmodel->name, i);
			offset = (src_offset_t) (pskintype + 1) - (src_offset_t) mod_base;
			c_snprintf2 (extname, "%s_%d", loadmodel->name, i);
			c_snprintf2 (fbr_mask_name, "%s:frame%d_glow", loadmodel->name, i);


			Mod_LoadSkinTexture
			(
				is_alphamasked,
				&pheader->gltextures[i][0],
				&pheader->fbtextures[i][0],
				name,
				fbr_mask_name,
				&extname[6],
				(byte *) (pskintype + 1),
				pheader->skinwidth,
				pheader->skinheight,
				offset
			);

			pheader->gltextures[i][3] = pheader->gltextures[i][2] = pheader->gltextures[i][1] = pheader->gltextures[i][0];
			pheader->fbtextures[i][3] = pheader->fbtextures[i][2] = pheader->fbtextures[i][1] = pheader->fbtextures[i][0];
			//johnfitz

			pskintype = (daliasskintype_t *)((byte *)(pskintype+1) + size);
		}
		else
		{
			// animating skin group.  yuck.
			pskintype++;
			pinskingroup = (daliasskingroup_t *)pskintype;
			groupskins = LittleLong (pinskingroup->numskins);
			pinskinintervals = (daliasskininterval_t *)(pinskingroup + 1);

			pskintype = (daliasskintype_t *)(pinskinintervals + groupskins);

			for (j=0 ; j<groupskins ; j++)
			{
//				Mod_FloodFillSkin( skin, pheader->skinwidth, pheader->skinheight );
				if (j == 0)
				{
					texels = (byte *)Hunk_AllocName(size, loadname);
					pheader->texels[i] = texels - (byte *)pheader;
					memcpy (texels, (byte *)(pskintype), size);
				}

				//johnfitz -- rewritten
				c_snprintf3 (name, "%s:frame%d_%d", loadmodel->name, i,j);
				offset = (src_offset_t)(pskintype) - (src_offset_t)mod_base; //johnfitz
				c_snprintf3 (extname, "%s_%d_%d", loadmodel->name, i, j);
				c_snprintf3 (fbr_mask_name, "%s:frame%d_%d_glow", loadmodel->name, i, j);

				Mod_LoadSkinTexture
				(
					is_alphamasked,
					&pheader->gltextures[i][j & 3],
					&pheader->fbtextures[i][j & 3],
					name,
					fbr_mask_name,
					extname,
					(byte *) (pskintype),
					pheader->skinwidth,
					pheader->skinheight,
					offset
				);
				//johnfitz

				pskintype = (daliasskintype_t *)((byte *)(pskintype) + size);
			}
			k = j;
			for (/* */; j < 4; j++)
				pheader->gltextures[i][j&3] =
				pheader->gltextures[i][j - k];
		}
	}

	return (void *)pskintype;
}

//=========================================================================


/*
=================
Mod_CalcAliasBounds -- johnfitz -- calculate bounds of alias model for nonrotated, yawrotated, and fullrotated cases
=================
*/
void Mod_CalcAliasBounds (aliashdr_t *a)
{
	int			i,j,k;
	float		dist, yawradius, radius;
	vec3_t		v;

	//clear out all data
	for (i=0; i<3;i++)
	{
		loadmodel->mins[i] = loadmodel->ymins[i] = loadmodel->rmins[i] = 999999;
		loadmodel->maxs[i] = loadmodel->ymaxs[i] = loadmodel->rmaxs[i] = -999999;
		radius = yawradius = 0;
	}

	//process verts
	for (i=0 ; i<a->numposes; i++)
		for (j=0; j<a->numverts; j++)
		{
			for (k=0; k<3;k++)
				v[k] = poseverts[i][j].v[k] * pheader->scale[k] + pheader->scale_origin[k];

			for (k=0; k<3;k++)
			{
				loadmodel->mins[k] = c_min (loadmodel->mins[k], v[k]);
				loadmodel->maxs[k] = c_max (loadmodel->maxs[k], v[k]);
			}
			dist = v[0] * v[0] + v[1] * v[1];
			if (yawradius < dist)
				yawradius = dist;
			dist += v[2] * v[2];
			if (radius < dist)
				radius = dist;
		}

	//rbounds will be used when entity has nonzero pitch or roll
	radius = sqrt(radius);
	loadmodel->rmins[0] = loadmodel->rmins[1] = loadmodel->rmins[2] = -radius;
	loadmodel->rmaxs[0] = loadmodel->rmaxs[1] = loadmodel->rmaxs[2] = radius;

	//ybounds will be used when entity has nonzero yaw
	yawradius = sqrt(yawradius);
	loadmodel->ymins[0] = loadmodel->ymins[1] = -yawradius;
	loadmodel->ymaxs[0] = loadmodel->ymaxs[1] = yawradius;
	loadmodel->ymins[2] = loadmodel->mins[2];
	loadmodel->ymaxs[2] = loadmodel->maxs[2];

/*
	Con_PrintLinef ("%s: radius %g rmins %g %g %g rmaxes %g %g %g ymins %g %g %g ymaxes %g %g %g",
		loadmodel->name, radius,
		loadmodel->rmins[0], loadmodel->rmins[1], loadmodel->rmins[2],
		loadmodel->rmaxs[0], loadmodel->rmaxs[1], loadmodel->rmaxs[2],
		loadmodel->ymins[0], loadmodel->ymins[1], loadmodel->ymins[2],
		loadmodel->ymaxs[0], loadmodel->ymaxs[1], loadmodel->ymaxs[2]
		);
*/
}


/*
=================
Mod_LoadAliasModel
=================
*/
void Mod_LoadAliasModel (qmodel_t *mod, void *buffer)
{ // GLQUAKE
	int					i, j;
	mdl_t				*pinmodel;
	stvert_t			*pinstverts;
	dtriangle_t			*pintriangles;
	int					version, numframes;
	int					size;
	daliasframetype_t	*pframetype;
	daliasskintype_t	*pskintype;
	int					start, end, total;

	start = Hunk_LowMark ();

	pinmodel = (mdl_t *)buffer;
	mod_base = (byte *)buffer; //johnfitz

	version = LittleLong (pinmodel->version);
	if (version != ALIAS_VERSION)
		Host_Error ("Mod_LoadAliasModel: %s has wrong version number (%d should be %d)",
				 mod->name, version, ALIAS_VERSION);

#ifdef GLQUAKE_SUPPORTS_QMB
	mod->modhint = GameHacks_IsSpecialQuakeAliasModel (mod->name);
#endif // GLQUAKE_SUPPORTS_QMB

//
// allocate space for a working header, plus all the data except the frames,
// skin and group info
//
	size = 	sizeof (aliashdr_t) +
			(LittleLong (pinmodel->numframes) - 1) * sizeof (pheader->frames[0])
			;
	pheader = (aliashdr_t *)Hunk_AllocName (size, loadname);

	// Note: EF_ALPHA_MASKED_MDL is 16384 and requires no action for us to use if QME 3.1 was used to set the flag
	mod->modelflags = LittleLong (pinmodel->flags);

//
// endian-adjust and copy the data, starting with the alias model header
//
	pheader->boundingradius = LittleFloat (pinmodel->boundingradius);
	pheader->numskins = LittleLong (pinmodel->numskins);
	pheader->skinwidth = LittleLong (pinmodel->skinwidth);
	pheader->skinheight = LittleLong (pinmodel->skinheight);

	if (pheader->skinheight > MAX_LBM_HEIGHT)
		Host_Error ("Mod_LoadAliasModel: model %s has a skin taller than %d", mod->name,
				   MAX_LBM_HEIGHT);

	pheader->numverts = LittleLong (pinmodel->numverts);

	if (pheader->numverts <= 0)
		Host_Error ("Mod_LoadAliasModel: model %s has no vertices", mod->name);

	if (pheader->numverts > MAXALIASVERTS_3984)
		Host_Error ("Mod_LoadAliasModel: model %s has too many vertices (%d, max = %d)", mod->name, pheader->numverts, MAXALIASVERTS_3984);	// GLQUAKE

	pheader->numtris = LittleLong (pinmodel->numtris);

	if (pheader->numtris <= 0)
		Host_Error ("Mod_LoadAliasModel: model %s has no triangles", mod->name);

	if (pheader->numtris > MAXALIASTRIS_4096) // Baker: This technically does not apply to WinQuake only GLQuake.
		Host_Error ("Mod_LoadAliasModel: model %s has too many triangles (%d, max = %d)", mod->name, pheader->numtris, MAXALIASTRIS_4096);  //Spike -- added this check, because I'm segfaulting out.

	pheader->numframes = LittleLong (pinmodel->numframes);
	numframes = pheader->numframes;
	if (numframes < 1)
		Host_Error ("Mod_LoadAliasModel: Invalid # of frames: %d", numframes);

	pheader->size = LittleFloat (pinmodel->size) * ALIAS_BASE_SIZE_RATIO;
	mod->synctype = (synctype_t) LittleLong (pinmodel->synctype);
	mod->numframes = pheader->numframes;

	for (i=0 ; i<3 ; i++)
	{
		pheader->scale[i] = LittleFloat (pinmodel->scale[i]);
		pheader->scale_origin[i] = LittleFloat (pinmodel->scale_origin[i]);
		pheader->eyeposition[i] = LittleFloat (pinmodel->eyeposition[i]);
	}


//
// load the skins
//
	pskintype = (daliasskintype_t *)&pinmodel[1];
	pskintype = (daliasskintype_t *) Mod_LoadAllSkins (pheader->numskins, pskintype, mod->modelflags & EF_ALPHA_MASKED_MDL);

//
// load base s and t vertices
//
	pinstverts = (stvert_t *)pskintype;

	for (i=0 ; i<pheader->numverts ; i++)
	{
		stverts[i].onseam = LittleLong (pinstverts[i].onseam);
		stverts[i].s = LittleLong (pinstverts[i].s);
		stverts[i].t = LittleLong (pinstverts[i].t);
	}

//
// load triangle lists
//
	pintriangles = (dtriangle_t *)&pinstverts[pheader->numverts];

	for (i=0 ; i<pheader->numtris ; i++)
	{
		triangles[i].facesfront = LittleLong (pintriangles[i].facesfront);

		for (j=0 ; j<3 ; j++)
		{
			triangles[i].vertindex[j] =
					LittleLong (pintriangles[i].vertindex[j]);
		}
	}

//
// load the frames
//
	posenum = 0;
	pframetype = (daliasframetype_t *)&pintriangles[pheader->numtris];

	for (i=0 ; i<numframes ; i++)
	{
		aliasframetype_t	frametype;
		frametype = (aliasframetype_t) LittleLong (pframetype->type);
		if (frametype == ALIAS_SINGLE)
			pframetype = (daliasframetype_t *) Mod_LoadAliasFrame (pframetype + 1, &pheader->frames[i]);
		else
			pframetype = (daliasframetype_t *) Mod_LoadAliasGroup (pframetype + 1, &pheader->frames[i]);
	}

	pheader->numposes = posenum;

	mod->type = mod_alias;

	Mod_SetExtraFlags (mod); //johnfitz

	Mod_CalcAliasBounds (pheader); //johnfitz

	//
	// build the draw lists
	//
	GL_MakeAliasModelDisplayLists (mod, pheader);

//
// move the complete, relocatable alias model to the cache
//
	end = Hunk_LowMark ();
	total = end - start;

	Cache_Alloc (&mod->cache, total, loadname);
	if (!mod->cache.data)
		return;
	memcpy (mod->cache.data, pheader, total);

	Hunk_FreeToLowMark (start);
}
#endif // GLQUAKE_RENDERER_SUPPORT


#ifdef WINQUAKE_RENDERER_SUPPORT
/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

aliashdr_t	*pheader;

// a pose is a single set of vertexes.  a frame may be
// an animating sequence of poses
trivertx_t	*poseverts[MAXALIASFRAMES_256];
int			posenum;

/*
=================
Mod_LoadAliasFrame
=================
*/
void * Mod_LoadAliasFrame (void * pin, int *pframeindex, int numv, trivertx_t *pbboxmin, trivertx_t *pbboxmax, aliashdr_t *pheader, char *name, maliasframedesc_t *frame, int recursed)
{
	int				i, j;
	trivertx_t		*pframe, *pinframe;
	daliasframe_t	*pdaliasframe;

	pdaliasframe = (daliasframe_t *)pin;

	strlcpy (name, pdaliasframe->name, 16 /* maliasframedesc_t name size */);
#if 1
	if (!recursed)
	{
		frame->firstpose = posenum;
		frame->numposes = 1;
	}
#endif

	for (i=0 ; i<3 ; i++)
	{
	// these are byte values, so we don't have to worry about
	// endianness
#if 1
		frame->bboxmin.v[i] = pdaliasframe->bboxmin.v[i];
		frame->bboxmax.v[i] = pdaliasframe->bboxmax.v[i];
#else
		pbboxmin->v[i] = pdaliasframe->bboxmin.v[i];
		pbboxmax->v[i] = pdaliasframe->bboxmax.v[i];
#endif
	}


	pinframe = (trivertx_t *)(pdaliasframe + 1);
	pframe = Hunk_AllocName (numv * sizeof(*pframe), loadname);

	*pframeindex = (byte *)pframe - (byte *)pheader;
	poseverts[posenum] = pinframe;
	posenum++;
	for (j=0 ; j<numv ; j++)
	{
		int		k;

	// these are all byte values, so no need to deal with endianness
		pframe[j].lightnormalindex = pinframe[j].lightnormalindex;

		for (k=0 ; k<3 ; k++)
			pframe[j].v[k] = pinframe[j].v[k];
		}

	pinframe += numv;

	return (void *)pinframe;
}


/*
=================
Mod_LoadAliasGroup
=================
*/
void * Mod_LoadAliasGroup (void * pin, int *pframeindex, int numv, trivertx_t *pbboxmin, trivertx_t *pbboxmax, aliashdr_t *pheader, char *name, maliasframedesc_t *frame)
{
	daliasgroup_t		*pingroup;
	int					i, numframes;
	daliasinterval_t	*pin_intervals;
	void				*ptemp;
	float				*poutintervals;

	maliasgroup_t		*paliasgroup;

	pingroup = (daliasgroup_t *)pin;

	numframes = LittleLong (pingroup->numframes);

	frame->firstpose = posenum;
	paliasgroup = Hunk_AllocName (sizeof (maliasgroup_t) + (numframes - 1) * sizeof (paliasgroup->frames[0]), loadname);
	paliasgroup->numframes =
		frame->numposes = numframes;

	for (i=0 ; i<3 ; i++)
	{
	// these are byte values, so we don't have to worry about endianness
#if 1
		frame->bboxmin.v[i] = pingroup->bboxmin.v[i];
		frame->bboxmax.v[i] = pingroup->bboxmax.v[i];
#else
		pbboxmin->v[i] = pingroup->bboxmin.v[i];
		pbboxmax->v[i] = pingroup->bboxmax.v[i];
#endif
	}

	*pframeindex = (byte *)paliasgroup - (byte *)pheader;
	pin_intervals = (daliasinterval_t *)(pingroup + 1);
#if 1
	frame->interval = LittleFloat (pin_intervals->interval);
#endif

	poutintervals = Hunk_AllocName (numframes * sizeof (float), loadname);

	paliasgroup->intervals = (byte *)poutintervals - (byte *)pheader;

	for (i=0 ; i<numframes ; i++)
	{
		*poutintervals = LittleFloat (pin_intervals->interval);
		if (*poutintervals <= 0.0)
			System_Error ("Mod_LoadAliasGroup: interval<=0");

		poutintervals++;
		pin_intervals++;
	}

	ptemp = (void *)pin_intervals;

	for (i=0 ; i<numframes ; i++)
	{
		// posenum ++; Baker: No, unlike GL this calls Mod_LoadAliasFrame
		ptemp = Mod_LoadAliasFrame (ptemp, &paliasgroup->frames[i].frame, numv,
				&paliasgroup->frames[i].bboxmin,
				&paliasgroup->frames[i].bboxmax, pheader, name, frame, 1);
	}

	return ptemp;
}

//=========================================================


/*
=================
Mod_LoadAliasSkin
=================
*/
void * Mod_LoadAliasSkin (void * pin, int *pskinindex, int skinsize, aliashdr_t *pheader)
{
	byte	*pskin, *pinskin;

	pskin = Hunk_AllocName (skinsize /** r_pixbytes*/, loadname);
	pinskin = (byte *)pin;
	*pskinindex = (byte *)pskin - (byte *)pheader;

	memcpy (pskin, pinskin, skinsize);

	pinskin += skinsize;

	return ((void *)pinskin);
}


/*
=================
Mod_LoadAliasSkinGroup
=================
*/
void * Mod_LoadAliasSkinGroup (void * pin, int *pskinindex, int skinsize, aliashdr_t *pheader)
{
	daliasskingroup_t		*pinskingroup;
	maliasskingroup_t		*paliasskingroup;
	int						i, numskins;
	daliasskininterval_t	*pinskinintervals;
	float					*poutskinintervals;
	void					*ptemp;

	pinskingroup = (daliasskingroup_t *)pin;

	numskins = LittleLong (pinskingroup->numskins);

	paliasskingroup = Hunk_AllocName (sizeof (maliasskingroup_t) +
			(numskins - 1) * sizeof (paliasskingroup->skindescs[0]),
			loadname);

	paliasskingroup->numskins = numskins;

	*pskinindex = (byte *)paliasskingroup - (byte *)pheader;

	pinskinintervals = (daliasskininterval_t *)(pinskingroup + 1);

	poutskinintervals = Hunk_AllocName (numskins * sizeof (float),loadname);

	paliasskingroup->intervals = (byte *)poutskinintervals - (byte *)pheader;

	for (i=0 ; i<numskins ; i++)
	{
		*poutskinintervals = LittleFloat (pinskinintervals->interval);
		if (*poutskinintervals <= 0)
			System_Error ("Mod_LoadAliasSkinGroup: interval<=0");

		poutskinintervals++;
		pinskinintervals++;
	}

	ptemp = (void *)pinskinintervals;

	for (i=0 ; i<numskins ; i++)
		ptemp = Mod_LoadAliasSkin (ptemp, &paliasskingroup->skindescs[i].skin, skinsize, pheader);

	return ptemp;
}

//=========================================================================


/*
=================
Mod_CalcAliasBounds -- johnfitz -- calculate bounds of alias model for nonrotated, yawrotated, and fullrotated cases
=================
*/
void Mod_CalcAliasBounds (aliashdr_t *a)
{
	int			i,j,k;
	float		dist, yawradius, radius;
	vec3_t		v;

	//clear out all data
	for (i=0; i<3;i++)
	{
		loadmodel->mins[i] = loadmodel->ymins[i] = loadmodel->rmins[i] = 999999;
		loadmodel->maxs[i] = loadmodel->ymaxs[i] = loadmodel->rmaxs[i] = -999999;
		radius = yawradius = 0;
	}

	//process verts
	for (i=0 ; i<a->numposes; i++)
		for (j=0; j<a->numverts; j++)
		{
			for (k=0; k<3;k++)
				v[k] = poseverts[i][j].v[k] * pheader->scale[k] + pheader->scale_origin[k];

			for (k=0; k<3;k++)
			{
				loadmodel->mins[k] = c_min (loadmodel->mins[k], v[k]);
				loadmodel->maxs[k] = c_max (loadmodel->maxs[k], v[k]);
			}
			dist = v[0] * v[0] + v[1] * v[1];
			if (yawradius < dist)
				yawradius = dist;
			dist += v[2] * v[2];
			if (radius < dist)
				radius = dist;
		}
/*
	//rbounds will be used when entity has nonzero pitch or roll
	radius = sqrt(radius);
	loadmodel->rmins[0] = loadmodel->rmins[1] = loadmodel->rmins[2] = -radius;
	loadmodel->rmaxs[0] = loadmodel->rmaxs[1] = loadmodel->rmaxs[2] = radius;

	//ybounds will be used when entity has nonzero yaw
	yawradius = sqrt(yawradius);
	loadmodel->ymins[0] = loadmodel->ymins[1] = -yawradius;
	loadmodel->ymaxs[0] = loadmodel->ymaxs[1] = yawradius;
	loadmodel->ymins[2] = loadmodel->mins[2];
	loadmodel->ymaxs[2] = loadmodel->maxs[2];
		Con_PrintLinef ("%s: radius %g rmins %g %g %g rmaxes %g %g %g ymins %g %g %g ymaxes %g %g %g",
		loadmodel->name, radius,
		loadmodel->rmins[0], loadmodel->rmins[1], loadmodel->rmins[2],
		loadmodel->rmaxs[0], loadmodel->rmaxs[1], loadmodel->rmaxs[2],
		loadmodel->ymins[0], loadmodel->ymins[1], loadmodel->ymins[2],
		loadmodel->ymaxs[0], loadmodel->ymaxs[1], loadmodel->ymaxs[2]
		);
*/
}


/*
=================
Mod_LoadAliasModel
=================
*/
void Mod_LoadAliasModel (qmodel_t *mod, void *buffer)
{
	int					i, numskins;
	mdl_t				*pmodel, *pinmodel;
	stvert_t			*pstverts, *pinstverts;
//	aliashdr_t			*pheader;
	mtriangle_t			*ptri;
	dtriangle_t			*pintriangles;
	int					version, numframes;
	int					size;
	daliasframetype_t	*pframetype;
	daliasskintype_t	*pskintype;
	maliasskindesc_t	*pskindesc;
	int					skinsize;
	int					start, end, total;

	start = Hunk_LowMark ();

	pinmodel = (mdl_t *)buffer;
	mod_base = (byte *)buffer; //johnfitz

	version = LittleLong (pinmodel->version);
	if (version != ALIAS_VERSION)
		Host_Error ("Mod_LoadAliasModel: %s has wrong version number (%d should be %d)",
			mod->name, version, ALIAS_VERSION);

//
// allocate space for a working header, plus all the data except the frames,
// skin and group info
//
	size = 	sizeof (aliashdr_t) +
		 (LittleLong (pinmodel->numframes) - 1) * sizeof (pheader->frames[0])
 			+ sizeof (mdl_t) + LittleLong (pinmodel->numverts)
				* sizeof (stvert_t)
			+ LittleLong (pinmodel->numtris) * sizeof (mtriangle_t)
				;
	pheader = (aliashdr_t *)Hunk_AllocName (size, loadname);
	pmodel = (mdl_t *) ((byte *)&pheader[1] + (LittleLong (pinmodel->numframes) - 1) * sizeof (pheader->frames[0]));

	pmodel->flags = mod->modelflags = LittleLong (pinmodel->flags); // EF_ALPHA_MASKED_MDL

//
// endian-adjust and copy the data, starting with the alias model header
//
	pmodel->boundingradius =
		pheader->boundingradius = LittleFloat (pinmodel->boundingradius);
	pmodel->numskins =
		pheader->numskins = LittleLong (pinmodel->numskins);
	pmodel->skinwidth =
		pheader->skinwidth = LittleLong (pinmodel->skinwidth);
	pmodel->skinheight =
		pheader->skinheight = LittleLong (pinmodel->skinheight);

	if (pheader->skinheight > MAX_LBM_HEIGHT)
		Host_Error ("Mod_LoadAliasModel: model %s has a skin taller than %d", mod->name,
			MAX_LBM_HEIGHT);

	pmodel->numverts =
		pheader->numverts = LittleLong (pinmodel->numverts);

	if (pheader->numverts <= 0)
		Host_Error ("Mod_LoadAliasModel: model %s has no vertices", mod->name);

	if (pheader->numverts > MAXALIASVERTS_3984)
		Host_Error ("Mod_LoadAliasModel: model %s has too many vertices (%d, max = %d)", mod->name, pheader->numverts, MAXALIASVERTS_3984);	//WINQUAKE
	
	if (pheader->numtris > MAXALIASTRIS_4096) // Baker: This technically does not apply to WinQuake only GLQuake ... but for consistency ...
		Host_Error ("Mod_LoadAliasModel: model %s has too many triangles (%d, max = %d)", mod->name, pheader->numtris, MAXALIASTRIS_4096);  //Spike -- added this check, because I'm segfaulting out.

	R_CheckAliasVerts (pmodel->numverts);

	pmodel->numtris =
		pheader->numtris = LittleLong (pinmodel->numtris);

	if (pheader->numtris <= 0)
		Host_Error ("Mod_LoadAliasModel: model %s has no triangles", mod->name);

	if (pheader->numtris > MAXALIASTRIS_4096)
	{	//Spike -- added this check, because I'm segfaulting out.
		Host_Error ("model %s has too many triangles (%d > %d)", mod->name, pheader->numtris, MAXALIASTRIS_4096);
	}


	pmodel->numframes =
		pheader->numframes = LittleLong (pinmodel->numframes);

	numframes = pheader->numframes;
	if (numframes < 1)
		Host_Error ("Mod_LoadAliasModel: Invalid # of frames: %d", numframes);

	pmodel->size =
		pheader->size = LittleFloat (pinmodel->size) * ALIAS_BASE_SIZE_RATIO;
	mod->synctype = (synctype_t)LittleLong (pinmodel->synctype);
	mod->numframes = pheader->numframes;

	for (i=0 ; i<3 ; i++)
	{
		pmodel->scale[i] =
			pheader->scale[i] = LittleFloat (pinmodel->scale[i]);
		pmodel->scale_origin[i] =
			pheader->scale_origin[i] = LittleFloat (pinmodel->scale_origin[i]);
		pmodel->eyeposition[i] =
			pheader->eyeposition[i] = LittleFloat (pinmodel->eyeposition[i]);
	}

	numskins = pmodel->numskins;

	if (pmodel->skinwidth & 0x03)
		Host_Error ("Mod_LoadAliasModel: skinwidth not multiple of 4");

	pheader->model = (byte *)pmodel - (byte *)pheader;

//
// load the skins
//
	skinsize = pmodel->skinheight * pmodel->skinwidth;

	if (numskins < 1)
		Host_Error ("Mod_LoadAliasModel: Invalid # of skins: %d", numskins);

	pskintype = (daliasskintype_t *)&pinmodel[1];
	pskindesc = Hunk_AllocName (numskins * sizeof (maliasskindesc_t), loadname);

	pheader->skindesc = (byte *)pskindesc - (byte *)pheader;

	for (i=0 ; i<numskins ; i++)
	{
		aliasskintype_t	skintype;

		skintype = LittleLong (pskintype->type);
		pskindesc[i].type = skintype;

		if (skintype == ALIAS_SKIN_SINGLE)
		{
			pskintype = (daliasskintype_t *) Mod_LoadAliasSkin (pskintype + 1,
						&pskindesc[i].skin, skinsize, pheader);
		}
		else
		{
			pskintype = (daliasskintype_t *) Mod_LoadAliasSkinGroup (pskintype + 1,
											&pskindesc[i].skin, skinsize, pheader);
		}
	}

//
// load base s and t vertices
//

// set base s and t vertices
	pstverts = (stvert_t *)&pmodel[1];
	pinstverts = (stvert_t *)pskintype;

	pheader->stverts = (byte *)pstverts - (byte *)pheader;

	for (i=0 ; i<pheader->numverts ; i++)
	{
		pstverts[i].onseam = LittleLong (pinstverts[i].onseam);
	// put s and t in 16.16 format
		pstverts[i].s = LittleLong (pinstverts[i].s) << 16;
		pstverts[i].t = LittleLong (pinstverts[i].t) << 16;
	}

//
// load triangle lists
//

// set up the triangles
	ptri = (mtriangle_t *)&pstverts[pmodel->numverts];
	pintriangles = (dtriangle_t *)&pinstverts[pmodel->numverts];

	pheader->triangles = (byte *)ptri - (byte *)pheader;

	for (i=0 ; i<pheader->numtris ; i++)
	{
		int		j;
		ptri[i].facesfront = LittleLong (pintriangles[i].facesfront);

		for (j=0 ; j<3 ; j++)
		{
			ptri[i].vertindex[j] =
				LittleLong (pintriangles[i].vertindex[j]);
		}
	}

//
// load the frames
//
	posenum = 0;
	pframetype = (daliasframetype_t *)&pintriangles[pmodel->numtris];

	for (i=0 ; i<numframes ; i++)
	{
		aliasframetype_t	frametype;
		frametype = (aliasframetype_t)LittleLong (pframetype->type);
		pheader->frames[i].type = frametype;

		if (frametype == ALIAS_SINGLE)
			pframetype = (daliasframetype_t *) Mod_LoadAliasFrame (pframetype + 1, &pheader->frames[i].frame, pmodel->numverts, &pheader->frames[i].bboxmin, &pheader->frames[i].bboxmax, pheader, pheader->frames[i].name, &pheader->frames[i],0);
		else
			pframetype = (daliasframetype_t *) Mod_LoadAliasGroup (pframetype + 1, &pheader->frames[i].frame, pmodel->numverts, &pheader->frames[i].bboxmin, &pheader->frames[i].bboxmax, pheader, pheader->frames[i].name, &pheader->frames[i]);
	}

	pheader->numposes = posenum; // Baker: Watch this and compare vs. Mark V

	mod->type = mod_alias;

	Mod_SetExtraFlags (mod); //johnfitz

	Mod_CalcAliasBounds (pheader); //johnfitz

#if 0 // Baker: That skeleton map shows this flaw, doesn't it?
// FIXME: do this right
	mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
	mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;
#endif

//
// move the complete, relocatable alias model to the cache
//
	end = Hunk_LowMark ();
	total = end - start;

	Cache_Alloc (&mod->cache, total, loadname);
	if (!mod->cache.data)
		return;
	memcpy (mod->cache.data, pheader, total);

	Hunk_FreeToLowMark (start);
}
#endif // WINQUAKE_RENDERER_SUPPORT


//=============================================================================

static	int	sprite_version;
/*
=================
Mod_LoadSpriteFrame
=================
*/
void * Mod_LoadSpriteFrame (void * pin, mspriteframe_t **ppframe, int framenum)
{
	dspriteframe_t		*pinframe;
	mspriteframe_t		*pspriteframe;
	int					width, height, size, origin[2];
#ifdef GLQUAKE_RENDERER_SUPPORT
	char				name[MAX_QPATH_64];
	src_offset_t			offset; //johnfitz
#endif // GLQUAKE_RENDERER_SUPPORT
#ifdef WINQUAKE_RENDERER_SUPPORT
	byte *input_source = NULL;
	byte *usage_source = NULL;
#endif // WINQUAKE_RENDERER_SUPPORT

	pinframe = (dspriteframe_t *)pin;

	width = LittleLong (pinframe->width);
	height = LittleLong (pinframe->height);

#ifdef GLQUAKE_RENDERER_SUPPORT
	size = (sprite_version == SPRITE32_VERSION) ? width * height * 4 : width * height;
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
	input_source = (byte *)(pinframe + 1);
	size = width * height;

	if (sprite_version == SPRITE32_VERSION)
	{
		// Baker: Time to perform surgery
		// Convert the pixels to QPAL
		// Then use the alpha channel as a mask
		// If alpha == 0, make the QPAL for the pixel 255
		usage_source = Image_Buffer_RGBA_To_Palette_Alpha_Threshold_Alloc ((unsigned *)input_source, width, height, vid.basepal, 30);
	}
	else usage_source = input_source;
#endif // WINQUAKE_RENDERER_SUPPORT

	pspriteframe = (mspriteframe_t *)Hunk_AllocName (sizeof (mspriteframe_t)
#ifdef WINQUAKE_RENDERER_SUPPORT
		+ size
#endif // WINQUAKE_RENDERER_SUPPORT
		, loadname);

	*ppframe = pspriteframe;

	pspriteframe->width = width;
	pspriteframe->height = height;
	origin[0] = LittleLong (pinframe->origin[0]);
	origin[1] = LittleLong (pinframe->origin[1]);

	pspriteframe->up = origin[1];
	pspriteframe->down = origin[1] - height;
	pspriteframe->left = origin[0];
	pspriteframe->right = width + origin[0];

#ifdef GLQUAKE_RENDERER_SUPPORT
	c_snprintf2 (name, "%s:frame%d", loadmodel->name, framenum);
	offset = (src_offset_t)(pinframe+1) - (src_offset_t)mod_base; //johnfitz

	if (!isDedicated && gl_external_textures.value)
	{
		// Prepare name
		int hunkmark = Hunk_LowMark ();
		unsigned *extdata;
		char extload[MAX_OSPATH];
		int extwidth, extheight;

		c_snprintf2 (extload, "%s_%d", loadmodel->name, framenum);
		extdata = Image_Load_Limited (extload, &extwidth, &extheight, loadmodel->loadinfo.searchpath);

		if (extdata)
		{
			pspriteframe->gltexture = TexMgr_LoadImage (loadmodel, -1 /*not bsp texture*/, name, extwidth, extheight,
				SRC_RGBA, extdata, extload, 0,  TEXPREF_PAD | TEXPREF_ALPHA | TEXPREF_NOPICMIP);

			// Baker: Set smax and tmax to reflect correct size based on padding
			// Not using pad conditional because non-power-of-two capability so we will just
			// check after image was uploaded.
			pspriteframe->smax = (float)pspriteframe->gltexture->source_width/(float)pspriteframe->gltexture->width;
			pspriteframe->tmax = (float)pspriteframe->gltexture->source_height/(float)pspriteframe->gltexture->height;

			// just making sure...
			Hunk_FreeToLowMark (hunkmark);
			goto sprite_tex_done; // Baker: Could avoid goto using ugly while 0 loop with break here, nah ....
		}
	}

	//johnfitz -- image might be padded
	pspriteframe->smax = (float)width/(float)TexMgr_PadConditional(width);
	pspriteframe->tmax = (float)height/(float)TexMgr_PadConditional(height);
	//johnfitz

	if (sprite_version == SPRITE32_VERSION)
		pspriteframe->gltexture =
		TexMgr_LoadImage (loadmodel, -1 /*not bsp texture*/, name, width, height, SRC_RGBA,
			(byte *)(pinframe + 1), loadmodel->name, offset,
			TEXPREF_PAD | TEXPREF_ALPHA | TEXPREF_NOPICMIP); //johnfitz -- TexMgr
	else
		pspriteframe->gltexture =
		TexMgr_LoadImage (loadmodel, -1 /*not bsp texture*/, name, width, height, SRC_INDEXED,
			(byte *)(pinframe + 1), loadmodel->name, offset,
			TEXPREF_PAD | TEXPREF_ALPHA | TEXPREF_NOPICMIP); //johnfitz -- TexMgr

sprite_tex_done:

	// Baker: Note that for sprite frames, the original pixel data gets put on the hunk
	// Maybe in the future use this for external_textures toggle.
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
	memcpy (&pspriteframe->pixels[0], usage_source, size);

	if (sprite_version == SPRITE32_VERSION)
		free (usage_source);
#endif // WINQUAKE_RENDERER_SUPPORT

	return (void *)((byte *)pinframe + sizeof (dspriteframe_t) + size);
}


/*
=================
Mod_LoadSpriteGroup
=================
*/
void * Mod_LoadSpriteGroup (void * pin, mspriteframe_t **ppframe, int framenum)
{
	dspritegroup_t		*pingroup;
	mspritegroup_t		*pspritegroup;
	int					i, numframes;
	dspriteinterval_t	*pin_intervals;
	float				*poutintervals;
	void				*ptemp;

	pingroup = (dspritegroup_t *)pin;

	numframes = LittleLong (pingroup->numframes);

	pspritegroup = (mspritegroup_t *)Hunk_AllocName (sizeof (mspritegroup_t) +
		 (numframes - 1) * sizeof (pspritegroup->frames[0]), loadname);

	pspritegroup->numframes = numframes;

	*ppframe = (mspriteframe_t *)pspritegroup;

	pin_intervals = (dspriteinterval_t *)(pingroup + 1);

	poutintervals = (float *)Hunk_AllocName (numframes * sizeof (float), loadname);

	pspritegroup->intervals = poutintervals;

	for (i=0 ; i<numframes ; i++)
	{
		*poutintervals = LittleFloat (pin_intervals->interval);
		if (*poutintervals <= 0.0)
			Host_Error ("Mod_LoadSpriteGroup: interval %f <= 0 in %s", *poutintervals, loadmodel->name);

		poutintervals++;
		pin_intervals++;
	}

	ptemp = (void *)pin_intervals;

	for (i=0 ; i<numframes ; i++)
	{
		ptemp = Mod_LoadSpriteFrame (ptemp, &pspritegroup->frames[i], framenum * 100 + i);
	}

	return ptemp;
}


/*
=================
Mod_LoadSpriteModel
=================
*/
void Mod_LoadSpriteModel (qmodel_t *mod, void *buffer)
{
	int					i;
	dsprite_t			*pin;
	msprite_t			*psprite;
	int					numframes;
	int					size;
	dspriteframetype_t	*pframetype;

	pin = (dsprite_t *)buffer;
	mod_base = (byte *)buffer; //johnfitz

	sprite_version = LittleLong (pin->version);

	if (sprite_version != SPRITE_VERSION && sprite_version != SPRITE32_VERSION)
		Host_Error ("Mod_LoadSpriteModel: %s has wrong version number (%d should be %d or %d)", mod->name, sprite_version, SPRITE_VERSION, SPRITE32_VERSION);


	numframes = LittleLong (pin->numframes);

	size = sizeof (msprite_t) +	(numframes - 1) * sizeof (psprite->frames);

	psprite = (msprite_t *)Hunk_AllocName (size, loadname);

	mod->cache.data = psprite;

	psprite->version = sprite_version;
	psprite->type = LittleLong (pin->type);
	psprite->maxwidth = LittleLong (pin->width);
	psprite->maxheight = LittleLong (pin->height);
	psprite->beamlength = LittleFloat (pin->beamlength);
	psprite->sprite_render_type = sprite_render_normal;

	mod->synctype = (synctype_t)LittleLong (pin->synctype);
	psprite->numframes = numframes;

	mod->mins[0] = mod->mins[1] = -psprite->maxwidth/2;
	mod->maxs[0] = mod->maxs[1] = psprite->maxwidth/2;
	mod->mins[2] = -psprite->maxheight/2;
	mod->maxs[2] = psprite->maxheight/2;

//
// load the frames
//
	if (numframes < 1)
		Host_Error ("Mod_LoadSpriteModel: Invalid # of frames %d in %s", numframes, mod->name);

	mod->numframes = numframes;

	pframetype = (dspriteframetype_t *)(pin + 1);

	for (i=0 ; i<numframes ; i++)
	{
		spriteframetype_t	frametype;

		frametype = (spriteframetype_t)LittleLong (pframetype->type);
		psprite->frames[i].type = frametype;

		if (frametype == SPR_SINGLE)
		{
			pframetype = (dspriteframetype_t *)
				Mod_LoadSpriteFrame (pframetype + 1, &psprite->frames[i].frameptr,i );
		}
		else
		{
			pframetype = (dspriteframetype_t *)
				Mod_LoadSpriteGroup (pframetype + 1, &psprite->frames[i].frameptr, i);
		}
	}

	mod->type = mod_sprite;
}

//=============================================================================

/*
================
Mod_Print
================
*/

static int Mod_ModelIndex (const char *model_name)
{
	if (cls.state == ca_connected) {
		int j; for (j = 1; j < MAX_FITZQUAKE_MODELS; j++) {
			if (!cl.model_precache[j])
				break;

			if (String_Does_Match_Caseless(cl.model_precache[j]->name, model_name))
				return j;
		}
	}
	return - 1;
}


void Mod_Print (lparse_t *unused)
{
	int		i;
	qmodel_t	*mod;

	Con_SafePrintLinef ("Cached models: (column 2 is modelindex)"); //johnfitz -- safeprint instead of print
	for (i=0, mod=mod_known ; i < mod_numknown ; i++, mod++)
	{
#ifdef GLQUAKE_RENDERER_SUPPORT
		Con_SafePrintLinef ("%8p %04d: %s %s", mod->cache.data, Mod_ModelIndex(mod->name), mod->name, mod->needload ? "(Need load)" : "");
#endif // GLQUAKE_RENDERER_SUPPORT

#ifdef WINQUAKE_RENDERER_SUPPORT
		Con_SafePrintContf ("%8p : %s", mod->cache.data, mod->name);
		if (mod->needload & NL_UNREFERENCED)
			Con_SafePrintContf (" (!R)");
		if (mod->needload & NL_NEEDS_LOADED)
			Con_SafePrintContf (" (!P)");
		Con_SafePrintLine ();
#endif // WINQUAKE_RENDERER_SUPPORT
	}
	Con_SafePrintLinef ("%d models", mod_numknown); //johnfitz -- print the total too
}


/*
================
Mod_Print
================
*/
void Mod_PrintEx (lparse_t *unused)
{
	int		i, count;
	qmodel_t	*mod;

	Con_SafePrintLinef ("Cached models:"); //johnfitz -- safeprint instead of print
	for (i = 0, mod = mod_known, count = 0 ; i < mod_numknown ; i++, mod++)
	{
		if (mod->cache.data == NULL)
			continue; // It isn't loaded

		if (mod->name[0] == '*')
			continue; // Don't list map submodels

		count++;
		Con_SafePrintLinef ("%-3i : %s", count, mod->name); //johnfitz -- safeprint instead of print
	}
}


/*
===============
Mod_Init
===============
*/

void Mod_Init (void)
{
	memset (mod_novis, 0xff, sizeof(mod_novis));

#ifdef GLQUAKE_RENDERER_SUPPORT
	//johnfitz -- create notexture miptex
	r_notexture_mip = (texture_t *)Hunk_AllocName (sizeof(texture_t), "r_notexture_mip");
	c_strlcpy (r_notexture_mip->name, "notexture");
	r_notexture_mip->height = r_notexture_mip->width = 32;

	r_notexture_mip2 = (texture_t *)Hunk_AllocName (sizeof(texture_t), "r_notexture_mip2");
	c_strlcpy (r_notexture_mip2->name, "notexture2");
	r_notexture_mip2->height = r_notexture_mip2->width = 32;
	//johnfitz
#endif // GLQUAKE_RENDERER_SUPPORT

	Cmd_AddCommands (Mod_Init);
}