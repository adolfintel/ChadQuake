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
// r_efrag.c

#include "quakedef.h" // Baker: There are no modifications of interest in this file.


mnode_t	*r_pefragtopnode;


//===========================================================================

/*
===============================================================================

					ENTITY FRAGMENT FUNCTIONS

===============================================================================
*/

efrag_t		**lastlink;

vec3_t		r_emins, r_emaxs;

entity_t	*r_addent;


/*
================
R_RemoveEfrags

Call when removing an object from the world or moving it to another position

from MH: note that this never gets properly used in GL, and only exists for compatibility
(it could just as easily be a stub)
================
*/
void R_RemoveEfrags (entity_t *ent)
{
	efrag_t		*ef, *old, *walk, **prev;

	ef = ent->efrag;

	while (ef)
	{
		prev = &ef->leaf->efrags;

		while (1)
		{
			walk = *prev;

			if (!walk)
				break;

			if (walk == ef)
			{
				// remove this fragment
				*prev = ef->leafnext;
				break;
			}
			else prev = &walk->leafnext;
		}

		old = ef;
		ef = ef->entnext;

	// put it on the free list
		old->entnext = cl.free_efrags;
		cl.free_efrags = old;
	}

	ent->efrag = NULL;
}

/*
===================
R_SplitEntityOnNode
===================
*/
void R_SplitEntityOnNode (mnode_t *node)
{
	efrag_t		*ef;
	mplane_t	*splitplane;
	mleaf_t		*leaf;
	int			sides;

	if (node->contents == CONTENTS_SOLID)
	{
		return;
	}

// add an efrag if the node is a leaf

	if ( node->contents < 0)
	{
		if (!r_pefragtopnode)
			r_pefragtopnode = node;

		leaf = (mleaf_t *)node;

// grab an efrag off the free list
		ef = cl.free_efrags;
		if (!ef)
		{
			//johnfitz -- less spammy overflow message
			if (!dev_overflows.efrags || dev_overflows.efrags + CONSOLE_RESPAM_TIME < realtime )
			{
				Con_PrintLinef ("Too many efrags!");
				dev_overflows.efrags = realtime;
			}
			//johnfitz
			return;		// no free fragments...
		}
		cl.free_efrags = cl.free_efrags->entnext;

		ef->entity = r_addent;

// add the entity link
		*lastlink = ef;
		lastlink = &ef->entnext;
		ef->entnext = NULL;

// set the leaf links
		ef->leaf = leaf;
		ef->leafnext = leaf->efrags;
		leaf->efrags = ef;

		return;
	}

// NODE_MIXED

	splitplane = node->plane;
	sides = BOX_ON_PLANE_SIDE(r_emins, r_emaxs, splitplane);

	if (sides == 3)
	{
	// split on this plane
	// if this is the first splitter of this bmodel, remember it
		if (!r_pefragtopnode)
			r_pefragtopnode = node;
	}

// recurse down the contacted sides
	if (sides & 1)
		R_SplitEntityOnNode (node->children[0]);

	if (sides & 2)
		R_SplitEntityOnNode (node->children[1]);
}

/*
===========
R_CheckEfrags -- johnfitz -- check for excessive efrag count
===========
*/
void R_CheckEfrags (void)
{
	efrag_t		*ef;
	int			count;

	if (cls.signon < 2)
		return; //don't spam when still parsing signon packet full of static ents

	for (count = MAX_MARK_V_EFRAGS, ef = cl.free_efrags; ef; count--, ef = ef->entnext)
		;

//	if (count > 640 && dev_peakstats.efrags <= 640)
//		Con_WarningLinef ("%d efrags exceeds standard limit of 640.", count);

	if (count > MAX_FITZQUAKE_EFRAGS && dev_peakstats.efrags <= MAX_WINQUAKE_EFRAGS)
		Con_DWarningLine ("%d efrags exceeds FitzQuake limit of %d.", count, MAX_FITZQUAKE_EFRAGS);

	if (count > MAX_WINQUAKE_EFRAGS && dev_peakstats.efrags <= MAX_WINQUAKE_EFRAGS)
		Con_DWarningLine ("%d efrags exceeds standard limit of %d.", count, MAX_WINQUAKE_EFRAGS);

	dev_stats.efrags = count;
	dev_peakstats.efrags = c_max(count, dev_peakstats.efrags);
}


#ifdef WINQUAKE_RENDERER_SUPPORT
/*
===================
R_SplitEntityOnNode2
===================
*/
// Called by R_DrawBEntitiesOnList in r_main.c
void R_SplitEntityOnNode2 (mnode_t *node)
{
	mplane_t	*splitplane;
	int			sides;

	if (node->visframe != cl.r_visframecount)
		return;

	if (node->contents < 0)
	{
		if (node->contents != CONTENTS_SOLID)
			r_pefragtopnode = node; // we've reached a non-solid leaf, so it's
									//  visible and not BSP clipped
		return;
	}

	splitplane = node->plane;
	sides = BOX_ON_PLANE_SIDE(r_emins, r_emaxs, splitplane);

	if (sides == 3)
	{
	// remember first splitter
		r_pefragtopnode = node;
		return;
	}

// not split yet; recurse down the contacted side
	if (sides & 1)
		R_SplitEntityOnNode2 (node->children[0]);
	else
		R_SplitEntityOnNode2 (node->children[1]);
}
#endif // WINQUAKE_RENDERER_SUPPORT


/*
===========
R_AddEfrags
===========
*/
void R_AddEfrags (entity_t *ent)
{
	qmodel_t		*entmodel;
	int			i;

	if (!ent->model)
		return;

	r_addent = ent;

	lastlink = &ent->efrag;
	r_pefragtopnode = NULL;

	entmodel = ent->model;

	for (i = 0 ; i < 3 ; i++)
	{
		r_emins[i] = ent->origin[i] + entmodel->mins[i];
		r_emaxs[i] = ent->origin[i] + entmodel->maxs[i];
	}

	R_SplitEntityOnNode (cl.worldmodel->nodes);

	ent->topnode = r_pefragtopnode;

	R_CheckEfrags (); //johnfitz
}


/*
================
R_StoreEfrags -- johnfitz -- pointless switch statement removed.
================
*/
void R_StoreEfrags (efrag_t **ppefrag)
{
	entity_t	*pent;
	efrag_t		*pefrag;

	while ((pefrag = *ppefrag) != NULL)
	{
		pent = pefrag->entity;

		if ((pent->visframe != cl.r_framecount) && (cl.numvisedicts < MAX_MARK_V_VISEDICTS))
		{
			cl.visedicts[cl.numvisedicts++] = pent;

			// mark that we've recorded this entity for this frame
				pent->visframe = cl.r_framecount;
		}

		ppefrag = &pefrag->leafnext;
	}
}
