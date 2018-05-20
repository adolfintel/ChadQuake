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
// sv_edict.c -- entity dictionary

#include "quakedef.h"

dprograms_t			*progs;
dfunction_t			*pr_functions;

char				*pr_strings;
ddef_t				*pr_fielddefs;
ddef_t				*pr_globaldefs;

#if 1
static	int			pr_stringssize;
static	const char	**pr_knownstrings;
static	int			pr_maxknownstrings;
static	int			pr_numknownstrings;
#endif // Quakespasm

cbool		pr_alpha_supported; //johnfitz

dstatement_t	*pr_statements;
globalvars_t	*pr_global_struct;
float			*pr_globals;			// same as pr_global_struct
int				pr_edict_size;	// in bytes

unsigned short pr_crc;

int type_size[8] = {
	1,		// ev_void
	1,		// ev_string  (string_t is an int)  sizeof(string_t)/4
	1,		// ev_float
	3,		// ev_vector
	1,		// ev_entity
	1,		// ev_field
	1,		// ev_function (func_t is an int) sizeof(func_t)/4
	1,		// ev_pointer (sizeof(void *)/4) isn't 64-bit friendly
};

static ddef_t *ED_FieldAtOfs (int ofs);
static cbool	ED_ParseEpair (void *base, ddef_t *key, const char *s);

#define	MAX_FIELD_LEN	64
#define GEFV_CACHESIZE	2

typedef struct {
	ddef_t	*pcache;
	char	field[MAX_FIELD_LEN];
} gefv_cache;

static gefv_cache	gefvCache[GEFV_CACHESIZE] =
{
	{NULL, ""},
	{NULL, ""}
};

// evaluation shortcuts
int	eval_gravity, eval_attack_finished, eval_items2, eval_ammo_shells1, eval_ammo_nails1;
int	eval_ammo_lava_nails, eval_ammo_rockets1, eval_ammo_multi_rockets;
int	eval_ammo_cells1, eval_ammo_plasma, eval_alpha;
int eval_idealpitch, eval_pitch_speed;

#ifdef SUPPORTS_COOP_ENHANCEMENTS
int eval_flags, eval_frags; // Baker: for coop enhancements
#endif // SUPPORTS_COOP_ENHANCEMENTS

static ddef_t *ED_FindField (const char *name);

int PR_FindFieldOffset (const char *field)
{
	ddef_t	*d;

	if (!(d = ED_FindField(field)))
		return 0;

	return d->ofs*4;
}

void PR_FindEdictFieldOffsets (void)
{
	eval_gravity = PR_FindFieldOffset ("gravity");
	eval_attack_finished = PR_FindFieldOffset ("attack_finished");
	eval_items2 = PR_FindFieldOffset ("items2");
	eval_ammo_shells1 = PR_FindFieldOffset ("ammo_shells1");
	eval_ammo_nails1 = PR_FindFieldOffset ("ammo_nails1");
	eval_ammo_lava_nails = PR_FindFieldOffset ("ammo_lava_nails");
	eval_ammo_rockets1 = PR_FindFieldOffset ("ammo_rockets1");
	eval_ammo_multi_rockets = PR_FindFieldOffset ("ammo_multi_rockets");
	eval_ammo_cells1 = PR_FindFieldOffset ("ammo_cells1");
	eval_ammo_plasma = PR_FindFieldOffset ("ammo_plasma");

	eval_alpha = PR_FindFieldOffset ("alpha");
	eval_idealpitch = PR_FindFieldOffset ("idealpitch");
	eval_pitch_speed = PR_FindFieldOffset ("pitch_speed");

#ifdef SUPPORTS_COOP_ENHANCEMENTS
	// Baker: For coop modifications
	eval_frags = PR_FindFieldOffset ("frags");
	eval_flags = PR_FindFieldOffset ("flags");
#endif // SUPPORTS_COOP_ENHANCEMENTS
}



/*
=================
ED_ClearEdict

Sets everything to NULL
=================
*/
void ED_ClearEdict (edict_t *e)
{
	memset (&e->v, 0, progs->entityfields * 4); // MH has a warning about this and FitzQuake alpha
	e->free = false;
}

/*
=================
ED_Alloc

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t *ED_Alloc (void)
{
	int			i;
	edict_t		*e;

// start on the edict after the clients
	for ( i = svs.maxclients_internal + 1 ; i < sv.num_edicts ; i++) // Because the cap can change at any time now.
	{
		e = EDICT_NUM(i);

		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (e->free && ( e->freetime < 2.0 || sv.time - e->freetime > 0.5 ) )
		{
			ED_ClearEdict (e);
			return e;
		}
	}

	if (i == sv.max_edicts) //johnfitz -- use sv.max_edicts instead of MAX_EDICTS
		Host_Error ("ED_Alloc: no free edicts (max_edicts is %d)", sv.max_edicts);

	sv.num_edicts++;
	e = EDICT_NUM(i);
	ED_ClearEdict (e);

	return e;
}

/*
=================
ED_Free

Marks the edict as free
FIXME: walk all entities and NULL out references to this entity
=================
*/
void ED_Free (edict_t *ed)
{
	SV_UnlinkEdict (ed);		// unlink from world bsp

	ed->free = true;
	ed->v.model = 0;
	ed->v.takedamage = 0;
	ed->v.modelindex = 0;
	ed->v.colormap = 0;
	ed->v.skin = 0;
	ed->v.frame = 0;
	VectorCopy (vec3_origin, ed->v.origin);
	VectorCopy (vec3_origin, ed->v.angles);
	ed->v.nextthink = -1;
	ed->v.solid = 0;
	ed->alpha = ENTALPHA_DEFAULT; //johnfitz -- reset alpha for next entity

	ed->freetime = sv.time;
}

//===========================================================================

/*
============
ED_GlobalAtOfs
============
*/
static ddef_t *ED_GlobalAtOfs (int ofs)
{
	ddef_t		*def;
	int			i;

	for (i=0 ; i<progs->numglobaldefs ; i++)
	{
		def = &pr_globaldefs[i];

		if (def->ofs == ofs)
			return def;
	}

	return NULL;
}

/*
============
ED_FieldAtOfs
============
*/
static ddef_t *ED_FieldAtOfs (int ofs)
{
	ddef_t		*def;
	int			i;

	for (i=0 ; i<progs->numfielddefs ; i++)
	{
		def = &pr_fielddefs[i];

		if (def->ofs == ofs)
			return def;
	}

	return NULL;
}

/*
============
ED_FindField
============
*/
static ddef_t *ED_FindField (const char *name)
{
	ddef_t		*def;
	int			i;

	for (i=0 ; i<progs->numfielddefs ; i++)
	{
		def = &pr_fielddefs[i];

		if (!strcmp(PR_GetString(def->s_name), name))
			return def;
	}

	return NULL;
}


/*
============
ED_FindGlobal
============
*/
static ddef_t *ED_FindGlobal (const char *name)
{
	ddef_t		*def;
	int			i;

	for (i=0 ; i<progs->numglobaldefs ; i++)
	{
		def = &pr_globaldefs[i];

		if (!strcmp(PR_GetString(def->s_name), name))
			return def;
	}

	return NULL;
}


/*
============
ED_FindFunction
============
*/
dfunction_t *ED_FindFunction (const char *fn_name) // qcexec command needs this so no static (or move qcexec)
{
	dfunction_t		*func;
	int				i;

	for (i = 0 ; i < progs->numfunctions ; i++)
	{
		func = &pr_functions[i];

		if (!strcmp(PR_GetString(func->s_name), fn_name))
			return func;
	}

	return NULL;
}



/*
============
PR_ValueString
(etype_t type, eval_t *val)

Returns a string describing *data in a type specific manner
=============
*/
// etype_t is an int and we better use int in case future compilers 
// make enums into 64 bits or something
static const char *PR_ValueString (int type, eval_t *val) 
{
	static char	line[512];
	ddef_t		*def;
	dfunction_t	*f;

	type &= ~DEF_SAVEGLOBAL;

	switch (type)
	{
	case ev_string:
		c_snprintf1 (line, "%s", PR_GetString(val->string));
		break;
	case ev_entity:
		c_snprintf1 (line, "entity %d", NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)) );
		break;
	case ev_function:
		f = pr_functions + val->function;
		c_snprintf1 (line, "%s()", PR_GetString(f->s_name));
		break;
	case ev_field:
		def = ED_FieldAtOfs ( val->_int );
		c_snprintf1 (line, ".%s", PR_GetString(def->s_name));
		break;
	case ev_void:
		c_strlcpy (line, "void");
		break;
	case ev_float:
		c_snprintf1 (line, "%5.1f", val->_float);
		break;
	case ev_vector:
		c_snprintf3 (line, "'%5.1f %5.1f %5.1f'", val->vector[0], val->vector[1], val->vector[2]);
		break;
	case ev_pointer:
		c_strlcpy (line, "pointer");
		break;
	default:
		c_snprintf1 (line, "bad type %d", type);
		break;
	}

	return line;
}

/*
============
PR_UglyValueString
(etype_t type, eval_t *val)

Returns a string describing *data in a type specific manner
Easier to parse than PR_ValueString
=============
*/
// Keep as an int in case future compiler makes enum into 64-bits
static const char *PR_UglyValueString (int type, eval_t *val)
{
	static char	line[512];
	ddef_t		*def;
	dfunction_t	*f;

	type &= ~DEF_SAVEGLOBAL;

	switch (type)
	{
	case ev_string:
		c_snprintf1 (line, "%s", PR_GetString(val->string));
		break;
	case ev_entity:
		c_snprintf1 (line, "%d", NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)));
		break;
	case ev_function:
		f = pr_functions + val->function;
		c_snprintf1 (line, "%s", PR_GetString(f->s_name));
		break;
	case ev_field:
		def = ED_FieldAtOfs ( val->_int );
		c_snprintf1 (line, "%s", PR_GetString(def->s_name));
		break;
	case ev_void:
		c_strlcpy (line, "void");
		break;
	case ev_float:
		c_snprintf1 (line, "%f", val->_float);
		break;
	case ev_vector:
		c_snprintf3 (line, "%f %f %f", val->vector[0], val->vector[1], val->vector[2]);
		break;
	default:
		c_snprintf1 (line, "bad type %d", type);
		break;
	}

	return line;
}

/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
const char *PR_GlobalString (int ofs)
{
	static char	line[512];
	const char	*s;
	int		i;
	ddef_t	*def;
	void	*val;

	val = (void *)&pr_globals[ofs];
	def = ED_GlobalAtOfs(ofs);

	if (!def)
		c_snprintf1 (line,"%d(???)", ofs);
	else
	{
		s = PR_ValueString (def->type, (eval_t *)val);
		c_snprintf3 (line,"%d(%s)%s", ofs, PR_GetString(def->s_name), s);
	}

	i = strlen(line);

	for ( ; i < 20 ; i++)
		c_strlcat (line, " ");

	c_strlcat (line, " ");

	return line;
}

const char *PR_GlobalStringNoContents (int ofs)
{
	static char	line[512];
	int		i;
	ddef_t	*def;
	

	def = ED_GlobalAtOfs(ofs);

	if (!def)
	{
		// more trigraph warnings here
		c_snprintf1 (line, "%d(???)", ofs);
	}
	else
		c_snprintf2 (line, "%d(%s)", ofs, PR_GetString(def->s_name));

	i = strlen(line);

	for ( ; i < 20 ; i++)
		c_strlcat (line," ");

	c_strlcat (line," ");

	return line;
}


/*
=============
ED_Print

For debugging
=============
*/
void ED_Print (edict_t *ed)
{
	ddef_t	*d;
	int		*v;
	int		i, j, l;
	const char	*name;
	int		type;

	if (ed->free)
	{
		Con_PrintLinef ("FREE");
		return;
	}

	Con_SafePrintLine ();
	Con_SafePrintLinef ("EDICT %d:", NUM_FOR_EDICT(ed)); //johnfitz -- was Con_Printf

	for (i = 1 ; i < progs->numfielddefs ; i++)
	{
		d = &pr_fielddefs[i];
		name = PR_GetString(d->s_name);
		l = strlen (name);

		if (l > 1 && name[l - 2] == '_') // Baker: Note this
			continue;	// skip _x, _y, _z vars

		v = (int *)((char *)&ed->v + d->ofs*4);

	// if the value is still all 0, skip the field
		type = d->type & ~DEF_SAVEGLOBAL;

		for (j=0 ; j<type_size[type] ; j++)
		{
			if (v[j])
				break;
		}

		if (j == type_size[type])
			continue;

		Con_SafePrintContf ("%s" ,name); //johnfitz -- was Con_Printf

		while (l++ < 15)
			Con_SafePrintContf (" "); //johnfitz -- was Con_Printf
#if 0
		if (!strcmp (name, "solid"))
		{
			
			int intval = PR_ValueString(d->type, (eval_t *)v);
			

		}
#endif
		//const char * solid_s[] = {"SOLID_NOT", "SOLID_TRIGGER", "SOLID_BBOX", "SOLID_SLIDEBOX", "SOLID_BSP", NULL};
#pragma message ("Baker: string to enum here ?  Movetype and flags too?  Effects")
		Con_SafePrintLinef ("%s", PR_ValueString(d->type, (eval_t *)v)); //johnfitz -- was Con_Printf
	}
}

/*
=============
ED_Write

For savegames
=============
*/
void ED_Write (FILE *f, edict_t *ed)
{
	ddef_t	*d;
	int		*v;
	int		i, j;
	const char	*name;
	int		type;

	fprintf (f, "{\n");

	if (ed->free)
	{
		fprintf (f, "}\n");
		return;
	}

	for (i=1 ; i<progs->numfielddefs ; i++)
	{
		d = &pr_fielddefs[i];
		name = PR_GetString(d->s_name) ;
		j = strlen (name);

		if (j > 1 && name[j - 2] == '_') // Baker: Note this
			continue;	// skip _x, _y, _z vars

		v = (int *)((char *)&ed->v + d->ofs*4);

	// if the value is still all 0, skip the field
		type = d->type & ~DEF_SAVEGLOBAL;

		for (j=0 ; j<type_size[type] ; j++)
		{
			if (v[j])
				break;
		}

		if (j == type_size[type])
			continue;

		fprintf (f, QUOTED_S " " /* <-- an important space */, name);
		fprintf (f, QUOTED_S "\n", PR_UglyValueString(d->type, (eval_t *)v));
	}

	//johnfitz -- save entity alpha manually when progs.dat doesn't know about alpha
	if (!pr_alpha_supported && ed->alpha != ENTALPHA_DEFAULT)
		fprintf (f, QUOTEDSTR("alpha") " " QUOTED_F "\n", ENTALPHA_TOSAVE(ed->alpha));
	//johnfitz

	fprintf (f, "}\n");
}

void ED_PrintNum (int ent)
{
	ED_Print (EDICT_NUM(ent));
}

/*
=============
ED_PrintEdicts

For debugging, prints all the entities in the current server
=============
*/
void ED_PrintEdicts (lparse_t *unused)
{
	int		i;

	if (!sv.active)
		return;

	Con_PrintLinef ("%d entities", sv.num_edicts);
	for (i=0 ; i<sv.num_edicts ; i++)
		ED_PrintNum (i);
}

/*
=============
ED_PrintEdict_f

For debugging, prints a single edict
=============
*/
void ED_PrintEdict_f (lparse_t *line)
{
	int		i;

	if (!sv.active)
		return;

	i = atoi (line->args[1]);
	if (i < 0 || i >= sv.num_edicts)
	{
		Con_PrintLinef ("Bad edict number");
		return;
	}

	ED_PrintNum (i);
}

/*
=============
ED_Count

For debugging
=============
*/
void ED_Count (void)
{
	edict_t	*ent;
	int		i, active, models, solid, step;

	if (!sv.active)
		return;

	active = models = solid = step = 0;

	for (i = 0 ; i < sv.num_edicts ; i++)
	{
		ent = EDICT_NUM(i);

		if (ent->free)
			continue;

		active++;

		if (ent->v.solid)
			solid++;

		if (ent->v.model)
			models++;

		if (ent->v.movetype == MOVETYPE_STEP)
			step++;
	}

	Con_PrintLinef ("num_edicts:%3d", sv.num_edicts);
	Con_PrintLinef ("active    :%3d", active);
	Con_PrintLinef ("view      :%3d", models);
	Con_PrintLinef ("touch     :%3d", solid);
	Con_PrintLinef ("step      :%3d", step);
}


/*
==============================================================================

					ARCHIVING GLOBALS

FIXME: need to tag constants, doesn't really work
==============================================================================
*/

/*
=============
ED_WriteGlobals
=============
*/
void ED_WriteGlobals (FILE *f)
{
	ddef_t		*def;
	int			i;
	const char		*name;
	int			type;

	fprintf (f,"{\n");

	for (i=0 ; i<progs->numglobaldefs ; i++)
	{
		def = &pr_globaldefs[i];
		type = def->type;

		if ( !(def->type & DEF_SAVEGLOBAL) )
			continue;

		type &= ~DEF_SAVEGLOBAL;

		if (type != ev_string && type != ev_float && type != ev_entity)
			continue;

		name = PR_GetString(def->s_name);
		fprintf (f, QUOTED_S " " /* <--- an important space */, name);
		fprintf (f, QUOTED_S "\n", PR_UglyValueString(type, (eval_t *)&pr_globals[def->ofs]));
	}

	fprintf (f,"}\n");
}

/*
=============
ED_ParseGlobals
=============
*/
void ED_ParseGlobals (const char *data)
{
	char	keyname[64];
	ddef_t	*key;

	while (1)
	{
	// parse key
		data = COM_Parse (data);

		if (com_token[0] == '}')
			break;

		if (!data)
			Host_Error ("ED_ParseEntity: EOF without closing brace");

		c_strlcpy (keyname, com_token);

	// parse value
		data = COM_Parse (data);

		if (!data)
			Host_Error ("ED_ParseEntity: EOF without closing brace");

		if (com_token[0] == '}')
			Host_Error ("ED_ParseEntity: closing brace without data");

		key = ED_FindGlobal (keyname);

		if (!key)
		{
			Con_PrintLinef ("'%s' is not a global", keyname);
			continue;
		}

		if (!ED_ParseEpair ((void *)pr_globals, key, com_token))
			Host_Error ("ED_ParseGlobals: parse error");
	}
}

//============================================================================


/*
=============
ED_NewString
=============
*/
// string_t is an int and needs to stay that way
static string_t ED_NewString (const char *string) 
{
	char		*new_p;
	int			i,l;
	string_t	num;

	l = strlen(string) + 1;
	num = PR_AllocString (l, &new_p);

	for (i=0 ; i< l ; i++)
	{
		if (string[i] == '\\' && i < l-1)
		{
			i++;

			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}

	return num;
}


/*
=============
ED_ParseEval

Can parse either fields or globals
returns false if error
=============
*/
static cbool ED_ParseEpair (void *base, ddef_t *key, const char *s)
{
	int		i;
	char	string[128];
	ddef_t	*def;
	char	*v, *w;
	void	*d;
	dfunction_t	*func;

	d = (void *)((int *)base + key->ofs);

	switch (key->type & ~DEF_SAVEGLOBAL)
	{
	case ev_string:
		*(string_t *)d = ED_NewString(s);
		break;

	case ev_float:
		*(float *)d = atof (s);
		break;

	case ev_vector:
		c_strlcpy (string, s);
		v = string;
		w = string;

		for (i=0 ; i<3 ; i++)
		{
			while (*v && *v != ' ')
				v++;

			*v = 0;
			((float *)d)[i] = atof (w);
			w = v = v+1;
		}

		break;

	case ev_entity:
		*(int *)d = EDICT_TO_PROG(EDICT_NUM(atoi (s)));
		break;

	case ev_field:
		def = ED_FindField (s);

		if (!def)
		{
			//johnfitz -- HACK -- suppress error becuase fog/sky fields might not be mentioned in defs.qc
			if (strncmp(s, "sky", 3) && strcmp(s, "fog"))
				Con_DPrintLinef ("Can't find field %s", s);

			return false;
		}

		*(int *)d = G_INT(def->ofs);
		break;

	case ev_function:
		func = ED_FindFunction (s);

		if (!func)
		{
			Con_PrintLinef ("Can't find function %s", s);
			return false;
		}

		*(func_t *)d = func - pr_functions;
		break;

	default:
		break;
	}

	return true;
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
Used for initial level load and for savegames.
====================
*/
const char *ED_ParseEdict (const char *data, edict_t *ent, cbool *out_angle_hacked)
{
	ddef_t		*key;
	char		keyname[256];
	cbool	anglehack, init;
	int			n;
	*out_angle_hacked = false;

	init = false;

// clear it
	if (ent != sv.edicts)	// hack
		memset (&ent->v, 0, progs->entityfields * 4);

// go through all the dictionary pairs
	while (1)
	{
	// parse key
		data = COM_Parse (data);

		if (com_token[0] == '}')
			break;

		if (!data)
			Host_Error ("ED_ParseEntity: EOF without closing brace");

		// anglehack is to allow QuakeEd to write single scalar angles
		// and allow them to be turned into vectors. (FIXME...)
		if (!strcmp(com_token, "angle"))
		{
			c_strlcpy (com_token, "angles");
			*out_angle_hacked = anglehack = true;
		}
		else anglehack = false;

		// FIXME: change light to _light to get rid of this hack
		if (!strcmp(com_token, "light"))
			c_strlcpy (com_token, "light_lev");	// hack for single light def

		c_strlcpy (keyname, com_token);

		// another hack to fix keynames with trailing spaces
		n = strlen(keyname);

		while (n && keyname[n-1] == ' ')
		{
			keyname[n-1] = 0;
			n--;
		}

		// parse value
		data = COM_Parse (data);
		if (!data)
			Host_Error ("ED_ParseEntity: EOF without closing brace");

		if (com_token[0] == '}')
			Host_Error ("ED_ParseEntity: closing brace without data");

		init = true;

		// keynames with a leading underscore are used for utility comments,
		// and are immediately discarded by quake
		if (keyname[0] == '_')
			continue;

		//johnfitz -- hack to support .alpha even when progs.dat doesn't know about it
		if (!strcmp(keyname, "alpha")) 
			ent->alpha = ENTALPHA_ENCODE(atof(com_token));
		//johnfitz

		key = ED_FindField (keyname);

		if (!key)
		{
			//johnfitz -- HACK -- suppress error becuase fog/sky/alpha fields might not be mentioned in defs.qc
			if (strncmp(keyname, "sky", 3) && strcmp(keyname, "fog") && strcmp(keyname, "alpha"))
				Con_DPrintLinef (QUOTED_S " is not a field", keyname); //johnfitz -- was Con_Printf

			continue;
		}

		if (anglehack)
		{
			char	temp[32];
			c_strlcpy (temp, com_token);
			c_snprintf1 (com_token, "0 %s 0", temp);
		}

		if (!ED_ParseEpair ((void *)&ent->v, key, com_token))
			Host_Error ("ED_ParseEdict: parse error");
	}

	if (!init)
		ent->free = true;

	return data;
}


/*
================
ED_LoadFromFile

The entities are directly placed in the array, rather than allocated with
ED_Alloc, because otherwise an error loading the map would have entity
number references out of order.

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.

Used for both fresh maps and savegame loads.  A fresh map would also need
to call ED_CallSpawnFunctions () to let the objects initialize themselves.
================
*/
void ED_LoadFromFile (const char *data)
{
	dfunction_t	*func;
	edict_t		*ent = NULL;
	int			inhibit = 0;
	int			oldcount;
	cbool		anglehack;

	pr_global_struct->time = sv.time;
	sv.fish_counted = false;

// parse ents
	while (1)
	{
// parse the opening brace
		data = COM_Parse (data);
		if (!data)
			break;

		if (com_token[0] != '{')
			Host_Error ("ED_LoadFromFile: found %s when expecting {",com_token);

		if (!ent)
			ent = EDICT_NUM(0);
		else ent = ED_Alloc ();

		data = ED_ParseEdict (data, ent, &anglehack);

// remove things from different skill levels or deathmatch
#ifdef SUPPORTS_PQ_RQUAKE // Baker change (implementation)
		if (pr_deathmatch.value && !pq_rquake.value)
		{
			if (((int)ent->v.spawnflags & SPAWNFLAG_NOT_DEATHMATCH))
			{
				ED_Free (ent);	
				inhibit++;
				continue;
			}
		}
#endif // Baker change + SUPPORTS_PQ_RQUAKE
		else if ((sv.current_skill == 0 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_EASY))
				|| (sv.current_skill == 1 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_MEDIUM))
				|| (sv.current_skill >= 2 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_HARD)) )
		{
			ED_Free (ent);
			inhibit++;
			continue;
		}
#if 1

		if (sv_fix_func_train_angles.value && anglehack)
			if (!ent->v.angles[0] && ent->v.angles[1] < 0 && !ent->v.angles[2])
			{
//				const char *classname = PR_GetString(ent->v.classname);
				// -1 or -2 key (up or down)
				if (!strcmp(PR_GetString(ent->v.classname), "func_train"))
					ent->v.angles[1] = 0;
			}
#endif

// immediately call spawn function
		if (!ent->v.classname)
		{
			Con_SafePrintLinef ("No classname for:"); //johnfitz -- was Con_Printf
			ED_Print (ent);
			ED_Free (ent);
			continue;
		}

	// look for the spawn function
		func = ED_FindFunction (PR_GetString(ent->v.classname));

		if (!func)
		{
			Con_SafePrintLinef ("No spawn function for:"); //johnfitz -- was Con_Printf
			ED_Print (ent);
			ED_Free (ent);
			continue;
		}

		pr_global_struct->self = EDICT_TO_PROG(ent);

		if (!sv.fish_counted && vm_fishfix.value && !strcmp(PR_GetString(func->s_name), "monster_fish"))
		{
			oldcount = pr_global_struct->total_monsters;
		}
		else oldcount = -1;

		PR_ExecuteProgram (func - pr_functions);

		if (oldcount != -1)
		{
			if ((int)(pr_global_struct->total_monsters) > oldcount)
				sv.fish_counted = true;
		}

	}

	Con_DPrintLinef ("%d entities inhibited", inhibit);
}

/*
============
PR_FindFunction
============
*/
dfunction_t *PR_FindNextFunction (int *start, const char *name, int flags)
{
	dfunction_t	*func;
	int			len, i;
	int			(*cmpfunc)(const char *, const char *, size_t);

	cmpfunc = (flags & PRFF_IGNORECASE ? strncasecmp : strncmp);

	len = strlen (name);

	i = (start ? c_max(*start, 0) : 0);
	for ( ; i<progs->numfunctions ; i++)
	{
		func = &pr_functions[i];
		if ((flags & PRFF_NOBUILTINS) && (func->first_statement < 0))
			continue;

		if ((flags & PRFF_NOPARAMS) && func->numparms)
			continue;

		if (!cmpfunc( PR_GetString(func->s_name), name, len))
		{
			if ((flags & PRFF_NOPARTIALS) && PR_GetString(func->s_name)[len] )
				continue;
			if (start)
				*start = i;
			return func;
		}
	}

	return NULL;
}


/*
===============
PR_CheckRevCycle (JDH)
  checks whether progs handles impulse 12 (usually used for CycleWeaponReverseCommand)
===============
*/
cbool PR_CheckRevCycle (void)
{
	char			*name = "ImpulseCommands";
	dfunction_t		*func;
	dstatement_t	*st;
	int				ofs_impulse, temp_impulse;
	eval_t			*arg2;

	func = PR_FindFunction (name, PRFF_NOBUILTINS | PRFF_NOPARTIALS);
	if (!func)
		return true;		// if there's no ImpulseCommands func, it's customized code that I won't mess with

	ofs_impulse = PR_FindFieldOffset ("impulse") / 4;
	temp_impulse = 0;

	for (st = &pr_statements[func->first_statement]; st->op; st++)
	{
		arg2 = (eval_t *) &pr_globals[(unsigned short)st->b];

		if (st->op == OP_LOAD_F)
		{
			//eval_t *arg1 = (eval_t *) &pr_globals[(unsigned short)st->a];
			//edict_t *ed = PROG_TO_EDICT(arg1->edict);

			if (arg2->_int == ofs_impulse)
				temp_impulse = st->c;		// local var that self.impulse is loaded into
		}
		else if (st->op == OP_EQ_F)
		{
			if ((st->a == temp_impulse) && (arg2->_float == 12.0))
				return true;
		}
	}

	return false;
}

#ifdef SUPPORTS_COOP_ENHANCEMENTS

void PR_In_Killed_Check (dfunction_t *f, int depth)
{
	// Baker: Find out who attacker is and if we need to increment
	// Save attackers score and increment it upon leaving function.

	// parm 1 is target
	// parm 2 is killer
	int the_dead_addy = ((int *)pr_globals)[f->parm_start];
	int the_murderer_addy = ((int *)pr_globals)[f->parm_start + 1];

	// Determine the entity number
	int the_dead_entnum = the_dead_addy / pr_edict_size;
	int the_murderer_entnum = the_murderer_addy / pr_edict_size;
	edict_t *dead_edict = EDICT_NUM(the_dead_entnum);
	edict_t *murderer_edict = EDICT_NUM(the_murderer_entnum);
	eval_t	*dead_flags_ev = GETEDICTFIELDVALUE(dead_edict, eval_flags);
	eval_t	*murderer_frag_ev = GETEDICTFIELDVALUE(murderer_edict, eval_frags);
	cbool dead_is_monster = dead_flags_ev ? (int)dead_flags_ev->_float & FL_MONSTER : false;
//	int murderer_start_frags = murderer_frag_ev ? (int)murderer_frag_ev->_float : 99999;

	// Make sure dead entity is a monster
	if (!dead_is_monster)
		return;

	// Make sure the killer is a player
	if (the_murderer_entnum < 1 || svs.maxclients_internal < the_murderer_entnum) // Because the cap can change at any time now.
		return;

	// Make sure isn't a suicide or self-kill
	if (the_murderer_entnum == the_dead_entnum)
		return;

	sv.pr_in_killed = depth;
	sv.pr_in_killed_murderer_frags_ev = murderer_frag_ev;
	sv.pr_in_killed_start_frags = murderer_frag_ev->_float;
	//Con_SafePrintLinef ("Entered killed");
}

void PR_In_Killed_Finish (void)
{
	sv.pr_in_killed = 0;
	//Con_SafePrintLinef ("Exit killed");

	if (!sv.pr_in_killed_murderer_frags_ev)
		return; // Error?

	// Frags didn't change, so we will do it.
	if (sv.pr_in_killed_murderer_frags_ev->_float == sv.pr_in_killed_start_frags)
		sv.pr_in_killed_murderer_frags_ev->_float ++;
}

dfunction_t* PR_Check_Coop_Protection (void)
{
	if (vm_coop_enhancements.value && pr_coop.value /*pr_global_struct->coop hasn't been set yet */)
		return PR_FindFunction ("PutClientInServer", PRFF_NOBUILTINS | PRFF_NOPARTIALS); // PutClientInServer is a required QuakeC function.

	return NULL;
}

dfunction_t* PR_Check_Marcher_Key_Touch (void)
{
	if (vm_coop_enhancements.value && pr_coop.value && !strcasecmp(gamedir_shortname(), "marcher")/*pr_global_struct->coop hasn't been set yet */)
		return PR_FindFunction ("key_touch", PRFF_NOBUILTINS | PRFF_NOPARTIALS);

	return NULL;
}



dfunction_t* PR_Check_Coop_Kills (void)
{
	// Reset this on progs load in case of pr error
	sv.pr_in_killed = 0;
	if (vm_coop_enhancements.value && pr_coop.value /*pr_global_struct->coop hasn't been set yet */)
	{
		dfunction_t* testfunc = PR_FindFunction ("Killed", PRFF_NOBUILTINS | PRFF_NOPARTIALS);
		if (testfunc && testfunc->numparms == 2 && testfunc->parm_size[0] == 1 && testfunc->parm_size[1])
		{
//			Con_SafePrintLinef ("Per player coop scores initialized");
			return testfunc; // Looks good!
		}
	}
	return NULL;
}
#endif // SUPPORTS_COOP_ENHANCEMENTS

/*
===============
PR_LoadProgs
===============
*/
void PR_LoadProgs (const char *__progs_name)
{
	int		i;
	cbool use_progs_name = __progs_name && __progs_name[0] && String_Is_Only_Alpha_Numeric_Plus_Charcode (__progs_name, '.');
	c_strlcpy (sv.progs_name, use_progs_name ? __progs_name : DEFAULT_PROGS_DAT_NAME);

// flush the non-C variable lookup cache
	for (i = 0 ; i < GEFV_CACHESIZE ; i++)
		gefvCache[i].field[0] = 0;

	CRC_Init (&pr_crc);

	progs = (dprograms_t *)COM_LoadHunkFile (sv.progs_name);
	if (!progs)
		Host_Error ("PR_LoadProgs: couldn't load " QUOTED_S, sv.progs_name);
	Con_DPrintLinef ("Programs occupy %d K.", (int)Math_KiloBytesDouble(com_filesize) );

	for (i=0 ; i<com_filesize ; i++)
		CRC_ProcessByte (&pr_crc, ((byte *)progs)[i]);

// byte swap the header
	for (i=0 ; i< (int) sizeof(*progs )/ 4 ; i++)
		((int *)progs)[i] = LittleLong ( ((int *)progs)[i] );

	if (progs->version != PROG_VERSION)
		Host_Error ("%s has wrong version number (%d should be %d)", sv.progs_name, progs->version, PROG_VERSION);
	if (progs->crc != PROGHEADER_CRC)
		Host_Error ("%s system vars have been modified, progdefs.h is out of date", sv.progs_name);

	pr_functions = (dfunction_t *)((byte *)progs + progs->ofs_functions);
	pr_strings = (char *)progs + progs->ofs_strings;
#if 1
	if (progs->ofs_strings + progs->numstrings >= com_filesize)
		Host_Error ("progs.dat strings go past end of file");
#endif

#if 1
	// initialize the strings
	pr_numknownstrings = 0;
	pr_maxknownstrings = 0;
	pr_stringssize = progs->numstrings;
	if (pr_knownstrings)
		Z_Free ((void *)pr_knownstrings);
	pr_knownstrings = NULL;
	PR_SetEngineString("");
#endif 

	pr_globaldefs = (ddef_t *)((byte *)progs + progs->ofs_globaldefs);
	pr_fielddefs = (ddef_t *)((byte *)progs + progs->ofs_fielddefs);
	pr_statements = (dstatement_t *)((byte *)progs + progs->ofs_statements);

	pr_global_struct = (globalvars_t *)((byte *)progs + progs->ofs_globals);
	pr_globals = (float *)pr_global_struct;

// byte swap the lumps
	for (i=0 ; i<progs->numstatements ; i++)
	{
		pr_statements[i].op = LittleShort(pr_statements[i].op);
		pr_statements[i].a = LittleShort(pr_statements[i].a);
		pr_statements[i].b = LittleShort(pr_statements[i].b);
		pr_statements[i].c = LittleShort(pr_statements[i].c);
	}

	for (i=0 ; i<progs->numfunctions; i++)
	{
	pr_functions[i].first_statement = LittleLong (pr_functions[i].first_statement);
	pr_functions[i].parm_start = LittleLong (pr_functions[i].parm_start);
	pr_functions[i].s_name = LittleLong (pr_functions[i].s_name);
	pr_functions[i].s_file = LittleLong (pr_functions[i].s_file);
	pr_functions[i].numparms = LittleLong (pr_functions[i].numparms);
	pr_functions[i].locals = LittleLong (pr_functions[i].locals);
	}

	for (i=0 ; i<progs->numglobaldefs ; i++)
	{
		pr_globaldefs[i].type = LittleShort (pr_globaldefs[i].type);
		pr_globaldefs[i].ofs = LittleShort (pr_globaldefs[i].ofs);
		pr_globaldefs[i].s_name = LittleLong (pr_globaldefs[i].s_name);
	}

	pr_alpha_supported = false; //johnfitz

	for (i=0 ; i<progs->numfielddefs ; i++)
	{
		pr_fielddefs[i].type = LittleShort (pr_fielddefs[i].type);
		if (pr_fielddefs[i].type & DEF_SAVEGLOBAL)
			Host_Error ("PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");
		pr_fielddefs[i].ofs = LittleShort (pr_fielddefs[i].ofs);
		pr_fielddefs[i].s_name = LittleLong (pr_fielddefs[i].s_name);

		//johnfitz -- detect alpha support in progs.dat
		if (!strcmp(PR_GetString(pr_fielddefs[i].s_name), "alpha"))
			pr_alpha_supported = true;
		//johnfitz
	}

	for (i=0 ; i<progs->numglobals ; i++)
		((int *)pr_globals)[i] = LittleLong (((int *)pr_globals)[i]);

	pr_edict_size = progs->entityfields * 4 + sizeof (edict_t) - sizeof(entvars_t);

	// Quakespasm
	// round off to next highest whole word address (esp for Alpha)
	// this ensures that pointers in the engine data area are always
	// properly aligned
	pr_edict_size += sizeof(void *) - 1;
	pr_edict_size &= ~(sizeof(void *) - 1);

	// JDHack / Requiem ...
	sv.pr_handles_imp12 = PR_CheckRevCycle ();
	sv.pr_imp12_override = false;

	if (!sv.pr_handles_imp12 && vm_imp12hack.value && vm_imp12hack.value < 2 && COM_ListMatch (vm_imp12hack_exceptions.string, gamedir_shortname()) )
	{
		Con_DPrintLinef ("Mod " QUOTED_S " is on impulse 12 exception list", gamedir_shortname());
	}
	else
	{
		sv.pr_imp12_override = vm_imp12hack.value >=2 || ( !sv.pr_handles_imp12 && vm_imp12hack.value) ;
			 if (!sv.pr_handles_imp12 && !sv.pr_imp12_override)  Con_VerbosePrintLinef ("Progs.dat does not appear to support impulse 12");
		else if (!sv.pr_handles_imp12 &&  sv.pr_imp12_override)  Con_VerbosePrintLinef ("Overriding missing impulse 12 support");
		else if ( sv.pr_handles_imp12 &&  sv.pr_imp12_override)  Con_VerbosePrintLinef ("Overriding existing impulse 12 support");
		else if ( sv.pr_handles_imp12 && !sv.pr_imp12_override)  Con_DPrintLinef ("Mod appears to support impulse 12");
	}

#ifdef SUPPORTS_COOP_ENHANCEMENTS
	sv.pr_handles_killed = PR_Check_Coop_Kills ();
	sv.pr_putclientinserver = PR_Check_Coop_Protection ();
	sv.pr_marcher_key_touch = PR_Check_Marcher_Key_Touch ();
#endif // SUPPORTS_COOP_ENHANCEMENTS


	// JoeQuake ...
	PR_FindEdictFieldOffsets ();
}


/*
==================
PR_QC_Exec

Execute QC commands from the console
==================
*/
void PR_QC_Exec (lparse_t *line)
{
	dfunction_t *f;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer (line);
		return;
	}

	if (!sv.active)
	{
		Con_PrintLinef ("Not running local game");
		return;
	};

	if (!developer.value)
	{
		Con_PrintLinef ("Only available in developer mode");
		return;
	};

	f = 0;
	if ((f = ED_FindFunction(line->args[1])) != NULL)
	{

		pr_global_struct->self = EDICT_TO_PROG(sv_player);
		PR_ExecuteProgram ((func_t)(f - pr_functions));
	}
	else
		Con_PrintLinef ("bad function");

}

/*
============
ED_FindFunction
============
*/
void PR_Listfunctions_f (lparse_t *line)
{
	cbool doall = line->count > 1 && !strcmp(line->args[1], "all");
	cbool doclassnames = line->count > 1 && !strcmp(line->args[1], "classnames");
	if (sv.active)
	{
		int				i;
		Con_PrintLinef ("QuakeC Functions:");
		for (i = 0 ; i < progs->numfunctions ; i++)
		{
			dfunction_t	*func = &pr_functions[i];
			const char *func_name = PR_GetString(func->s_name);

			do
			{
				// classnames are lower case a-z
				if (doclassnames)
				{
					if (!islower(func_name[0]) )
						break;

					if (!String_Does_Start_With (func_name, "monster_") && !String_Does_Start_With (func_name, "item_"))
						break;
				}

				// regular function names (uppercase) A-Z
				if (!doall && !doclassnames && !isupper (func_name[0]) )
					break;

				if (doclassnames) // Just print them
					Con_PrintLinef ("%s", PR_GetString(func->s_name) );
				else Con_PrintLinef ("%05i: %s", i, PR_GetString(func->s_name) );

				break;
			} while (0);
		}

	} else Con_PrintLinef ("No active server");
}



/*
===============
PR_Init
===============
*/
void PR_Init (void)
{
	Cmd_AddCommands (PR_Init);

}

edict_t *EDICT_NUM(int n)
{
	if (n < 0 || n >= sv.max_edicts)
		Host_Error ("EDICT_NUM: bad number %d", n);
	return (edict_t *)((byte *)sv.edicts+ (n)*pr_edict_size);
}

int NUM_FOR_EDICT(const edict_t *e)
{
	int		b;

	b = (byte *)e - (byte *)sv.edicts;
	b = b / pr_edict_size;

	if (b < 0 || b >= sv.num_edicts)
		Host_Error ("NUM_FOR_EDICT: bad pointer");
	return b;
}

//===========================================================================


#define	PR_STRING_ALLOCSLOTS	256

static void PR_AllocStringSlots (void)
{
	pr_maxknownstrings += PR_STRING_ALLOCSLOTS;
	Con_DPrintLinef ("PR_AllocStringSlots: realloc'ing for %d slots", pr_maxknownstrings);
	pr_knownstrings = (const char **) Z_Realloc ((void *)pr_knownstrings, pr_maxknownstrings * sizeof(char *));
}

const char *PR_GetString (int num)
{
	if (num >= 0 && num < pr_stringssize)
		return pr_strings + num;
	else if (num < 0 && num >= -pr_numknownstrings)
	{
		if (!pr_knownstrings[-1 - num])
		{
			Host_Error ("PR_GetString: attempt to get a non-existant string %d", num);
			return "";
		}
		return pr_knownstrings[-1 - num];
	}
	else
	{
		return pr_strings;
		Host_Error ("PR_GetString: invalid string offset %d", num);
		return "";
	}
}

int PR_SetEngineString (const char *s)
{
	int		i;

	if (!s)
		return 0;
#if 0	/* can't: sv.model_precache & sv.sound_precache points to pr_strings */
	if (s >= pr_strings && s <= pr_strings + pr_stringssize)
		Host_Error ("PR_SetEngineString: " QUOTED_S " in pr_strings area", s);
#else
	if (s >= pr_strings && s <= pr_strings + pr_stringssize - 2)
		return (int)(s - pr_strings);
#endif
	for (i = 0; i < pr_numknownstrings; i++)
	{
		if (pr_knownstrings[i] == s)
			return -1 - i;
	}
	// new unknown engine string
	//Con_DPrintLinef ("PR_SetEngineString: new engine string %p", s);
#if 0
	for (i = 0; i < pr_numknownstrings; i++)
	{
		if (!pr_knownstrings[i])
			break;
	}
#endif
//	if (i >= pr_numknownstrings)
//	{
		if (i >= pr_maxknownstrings)
			PR_AllocStringSlots();
		pr_numknownstrings++;
//	}
	pr_knownstrings[i] = s;
	return -1 - i;
}

int PR_AllocString (int size, char **ptr)
{
	int		i;

	if (!size)
		return 0;
	for (i = 0; i < pr_numknownstrings; i++)
	{
		if (!pr_knownstrings[i])
			break;
	}
//	if (i >= pr_numknownstrings)
//	{
		if (i >= pr_maxknownstrings)
			PR_AllocStringSlots();
		pr_numknownstrings++;
//	}
	pr_knownstrings[i] = (char *)Hunk_AllocName(size, "string");
	if (ptr)
		*ptr = (char *) pr_knownstrings[i];
	return -1 - i;
}