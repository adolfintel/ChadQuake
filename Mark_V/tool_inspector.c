#ifdef GLQUAKE // GLQUAKE specific

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
// tool_inspector.c -- Entity inspector.  Room for improvement.



#include "quakedef.h"

static cbool inspector_on;

void Tool_Inspector_f (lparse_t *line)
{
	const char *svactivehint = "Note: tool_inspector only works when serving a game (single player, host, etc.)";

	switch (line->count)
	{
	case 2:
		inspector_on = !!atoi(line->args[1]);
		break;
	case 1:
		inspector_on = !inspector_on;
		break;
	}

	Con_PrintLinef ("entity inspector is %s", inspector_on ? "ON" : "OFF");
	if (!sv.active)
		Con_PrintLinef ("%s", svactivehint);

}


void Entity_Inspector_Init (void)
{
	Cmd_AddCommands (Entity_Inspector_Init);
}




/*

  Entity Inspector

// 0: Model + Frame		(No model then skip)
// 1: Classname			(Everything)
// 2: Server edict		(Everything)
// 4: Flags				(Everything)
// 8: Target -->		(Everything)
//16. --> Targetname	(Everything)
//32: Origin/Angles		(Everything)
//64: Nextthink time	(Everything)

 */

static void Entity_Inspector_Draw (void)
{
	extern		edict_t *sv_player;
	edict_t		*ed;
	int			i;
	entity_t	*client_entity;

	// Cycle through the server entities  TODO: Sort by distance?

	for (i = 1, ed = NEXT_EDICT(sv.edicts) ; i < sv.num_edicts ; i ++, ed = NEXT_EDICT(ed))
	{
		const char *my_classname = PR_GetString(ed->v.classname);
		rgba4_t	boxcolor;

		if (ed->free)
			continue;	// Free edict, just skip

		client_entity = NULL;
		if (i < cl.num_entities)		// Baker: Not all server ents get communicated to client.
			client_entity = cl_entities + i;

		if (ed == sv_player && chase_active.value == 0)
			continue;	// If entity is the player and chase cam isn't on ... skip it.

		// Screen out stuff

		switch (cl.stats[STAT_ACTIVEWEAPON])
		{
		case 0:		if (!ed->v.model || strlen(PR_GetString( ed->v.model)) == 0)
					continue;	// Have no model.

//		case 16:	if (ed->v.health <= 0)
//					continue;	// Have no health

		case 32:	break; // For the moment
		default:	break;

		}


		// Obtain Quake specific super-special color
		do
		{
			int special_entity = 0;

			if      (strncasecmp (my_classname, "info_player_start", 17) == 0) 	 special_entity = 1;
			else if (strcasecmp (my_classname, "info_player_coop") == 0) 		 special_entity = 2;
			else if (strcasecmp (my_classname, "info_player_deathmatch") == 0) special_entity = 3;
			else if (strncasecmp (my_classname, "light", 5) == 0) special_entity = 4;

			switch (special_entity)
			{
			case 1:		VectorSetColor4f (boxcolor, 1.0f, 0.0f, 0.0f, 0.40f);	break;
			case 2:		VectorSetColor4f (boxcolor, 1.0f, 1.0f, 0.0f, 0.30f);	break;
			case 3:		VectorSetColor4f (boxcolor, 1.0f, 0.0f, 0.0f, 0.40f); break;
			case 4:		VectorSetColor4f (boxcolor, 1.0f, 1.0f, 0.0f, 0.70f); break;
			}

			// If we already determined a color, exit stage left ...
			if (special_entity)
				continue;

			// normal color selection
			switch ((int)ed->v.solid)
			{
			case SOLID_NOT:      VectorSetColor4f (boxcolor, 1.0f, 1.0f, 1.0f, 0.05f); break;
			case SOLID_TRIGGER:  VectorSetColor4f (boxcolor, 1.0f, 0.0f, 1.0f, 0.10f); break;
			case SOLID_BBOX:     VectorSetColor4f (boxcolor, 0.0f, 1.0f, 0.0f, 0.10f); break;
			case SOLID_SLIDEBOX: VectorSetColor4f (boxcolor, 1.0f, 0.0f, 0.0f, 0.10f); break;
			case SOLID_BSP:      VectorSetColor4f (boxcolor, 0.0f, 0.0f, 1.0f, 0.05f); break;
			default:             VectorSetColor4f (boxcolor, 0.0f, 0.0f, 0.0f, 0.50f); break;
			}

		} while (0);

		// Render box phase

		do
		{
			cbool isPointEntity  = VectorCompare (ed->v.mins, ed->v.maxs); // point entities have 0 size

			if (isPointEntity)
			{
				cbool bRenderTopMost = false;
				cbool bRenderLines   = false;
				float size_adjust = 4.0f; // Give it a little bit of size

				R_EmitBox (boxcolor, ed->v.origin, NULL, NULL, bRenderTopMost, bRenderLines, size_adjust);
			}
			else
			{
				cbool bRenderTopMost = false;
				cbool bRenderLines   = false;
				float size_adjust	  = -0.10f; // Pull size in just a little

				R_EmitBox (boxcolor, ed->v.origin, ed->v.mins, ed->v.maxs, bRenderTopMost, bRenderLines, size_adjust);
			}
		} while (0);

		// Render Caption phase

		do
		{
			extern cbool SV_InvisibleToClient (edict_t *viewer, edict_t *seen);

			extern vec3_t currentbox_mins, currentbox_maxs;	// Last box rendering result
			vec3_t caption_location;
			char buffer[1024];
//			char *fieldstring;

			if (SV_InvisibleToClient (sv_player, ed))
				continue; //don't draw if entity cannot be seen

			// Averaging the vectors gives the center
			VectorAverage (currentbox_mins, currentbox_maxs, caption_location);

			// Except we want the caption to be at the top
			caption_location[2] = currentbox_maxs[2];

			// Add a couple of height units to make it overhead
			caption_location[2] = caption_location[2] + 20.0f;


/*

  Entity Inspector


// 0: Model + Frame		(No model then skip)
// 1: Classname			(Everything)
// 2: Server edict		(Everything)
// 4: Flags				(Everything)
// 8: Target -->		(Everything)
//16. --> Targetname	(Everything)
//32: Origin/Angles		(Everything)
//64: Nextthink time	(Everything)


 */

			switch (cl.stats[STAT_ACTIVEWEAPON])
/*weapon*/	{
/* 1 */		case 0:		c_snprintf3 (buffer, "\bframe\b %d \bindex:\b %d\n%s", (int)ed->v.frame, (int)ed->v.modelindex, PR_GetString( ed->v.model));
						break;

/* 3 */		case 2:		if (((int)cl.time % 10) < 3)
							c_snprintf1 (buffer, "\btype\b edict %d\n\bin console\nfor more info\b", i); // So someone can learn the right words.
						else
							c_snprintf1 (buffer, "\bserver edict # \b%d", i);
						break;

/* 7 */		case 32:	c_snprintf6 (buffer, "\borigin:\b %d %d %d\n\bangles:\b %d %d %d", (int)ed->v.origin[0], (int)ed->v.origin[1], (int)ed->v.origin[2], (int)ed->v.angles[0], (int)ed->v.angles[1], (int)ed->v.angles[2] );
						break;

/* 8 */		case 64:	c_snprintf2 (buffer, "\bnextthink time:\b\n\babs:\b %4.2f \bnet:\b%s\n", ed->v.nextthink, ed->v.nextthink < 1 ? "never" : va("%4.2f", ed->v.nextthink - sv.time) );
						break;

/* 4 */		case 4:		c_snprintf1 (buffer, "\bflags:\b  %d", (int)ed->v.flags);
						break;

/* 5 */		case 8:		if (strlen(PR_GetString( ed->v.target)) > 0)
							c_snprintf1 (buffer, "\btarget -->:\b %s", PR_GetString( ed->v.target));
						else
							c_strlcpy (buffer, "no entity targeted");
						break;


/* 6 */		case 16:	if (strlen(PR_GetString( ed->v.targetname)) > 0)
							c_snprintf1 (buffer, "--> \btargetname:\b %s", PR_GetString( ed->v.targetname));
						else
							c_strlcpy (buffer, "none");
						break;

/* 2 */		case 1:
			default:	if (((int)cl.time % 20) == 0)
						{
							c_strlcpy  (buffer, "\bentity classname is ...\b"); // So someone can learn the right words.
						}
						else c_snprintf1 (buffer, "%s", my_classname);
						break;
			}

			R_EmitCaption  (caption_location, buffer, true);
		} while (0);

		// End of this entity
	}  // For Loop closing brace

}



void Entity_Inspector_Think (void)
{
	if (!sv.active)
		return; // Only works if client is the server too

	if (!inspector_on)
		return;

	Entity_Inspector_Draw ();
}

#endif // GLQUAKE specific
