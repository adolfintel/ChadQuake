
#include "bsp5.h"


extern qboolean SameContent(int Cont0, int Cont1);
extern void	NumberLeafs_r (node_t *node);

int		num_visleafs;	// leafs the player can be in
int		num_visportals;

node_t	outside_node;		// portals outside the world face this

//=============================================================================

/*
=============
AddPortalToNodes
=============
*/
void AddPortalToNodes (portal_t *p, node_t *front, node_t *back)
{
	if (p->nodes[0] || p->nodes[1])
		Message (MSGERR, "AddPortalToNodes: Already included near %s", GetCoord(p->winding->points[0]));

	p->nodes[0] = front;
	p->next[0] = front->portals;
	front->portals = p;

	p->nodes[1] = back;
	p->next[1] = back->portals;
	back->portals = p;
}


/*
=============
RemovePortalFromNode
=============
*/
void RemovePortalFromNode (portal_t *portal, node_t *l)
{
	portal_t	**pp, *t;

// remove reference to the current portal
	pp = &l->portals;
	while (1)
	{
		t = *pp;
		if (!t)
			Message (MSGERR, "RemovePortalFromNode: Portal not in leaf near %s", GetCoord(portal->winding->points[0]));

		if ( t == portal )
			break;

		if (t->nodes[0] == l)
			pp = &t->next[0];
		else if (t->nodes[1] == l)
			pp = &t->next[1];
		else
			Message (MSGERR, "RemovePortalFromNode: Portal not bounding leaf near %s", GetCoord(portal->winding->points[0]));
	}

	if (portal->nodes[0] == l)
	{
		*pp = portal->next[0];
		portal->nodes[0] = NULL;
	}
	else if (portal->nodes[1] == l)
	{
		*pp = portal->next[1];
		portal->nodes[1] = NULL;
	}
}

//============================================================================

/*
================
MakeHeadnodePortals

The created portals will face the global outside_node
================
*/
void MakeHeadnodePortals (node_t *node)
{
	vec3_t		bounds[2];
	int			i, j, n;
	portal_t	*p, *portals[6];
	plane_t		bplanes[6], *pl;
	int			side;

// pad with some space so there will never be null volume leafs
	for (i=0 ; i<3 ; i++)
	{
		bounds[0][i] = brushset->mins[i] - SIDESPACE;
		bounds[1][i] = brushset->maxs[i] + SIDESPACE;
	}

	outside_node.contents = CONTENTS_SOLID;
	outside_node.portals = NULL;

	for (i=0 ; i<3 ; i++)
		for (j=0 ; j<2 ; j++)
		{
			n = j*3 + i;

			p = AllocPortal ();
			portals[n] = p;

			pl = &bplanes[n];
			memset (pl, 0, sizeof(*pl));
			if (j)
			{
				pl->normal[i] = -1;
				pl->dist = -bounds[j][i];
			}
			else
			{
				pl->normal[i] = 1;
				pl->dist = bounds[j][i];
			}
			p->planenum = FindPlane (pl, &side);

			p->winding = BaseWindingForPlane (pl);
			if (side)
				AddPortalToNodes (p, &outside_node, node);
			else
				AddPortalToNodes (p, node, &outside_node);
		}

// clip the basewindings by all the other planes
	for (i=0 ; i<6 ; i++)
	{
		for (j=0 ; j<6 ; j++)
		{
			if (j == i)
				continue;
			portals[i]->winding = ClipWinding (portals[i]->winding, &bplanes[j], true);
		}
	}
}

//============================================================================

void PlaneFromWinding (winding_t *w, plane_t *plane)
{
	vec3_t		v1, v2;

// calc plane
	VectorSubtract (w->points[2], w->points[1], v1);
	VectorSubtract (w->points[0], w->points[1], v2);
	tx_CrossProduct (v2, v1, plane->normal);
	tx_VectorNormalize (plane->normal);
	plane->dist = DotProduct (w->points[0], plane->normal);
}

/*
================
CutNodePortals_r

================
*/
void CutNodePortals_r (node_t *node)
{
	plane_t 	*plane, clipplane;
	node_t		*f, *b, *other_node;
	portal_t	*p, *new_portal, *next_portal;
	winding_t	*w, *frontwinding, *backwinding;
	int		side;

//
// seperate the portals on node into it's children
//
	if (node->contents)
	{
		return;			// at a leaf, no more dividing
	}

	plane = &planes[node->planenum];

	f = node->children[0];
	b = node->children[1];

//
// create the new portal by taking the full plane winding for the cutting plane
// and clipping it by all of the planes from the other portals
//
	new_portal = AllocPortal ();
	new_portal->planenum = node->planenum;

	w = BaseWindingForPlane (&planes[node->planenum]);
	side = 0;	// shut up compiler warning
	for (p = node->portals ; p ; p = p->next[side])
	{
		clipplane = planes[p->planenum];
		if (p->nodes[0] == node)
			side = 0;
		else if (p->nodes[1] == node)
		{
			clipplane.dist = -clipplane.dist;
			VectorSubtract (tx_vec3_origin, clipplane.normal, clipplane.normal);
			side = 1;
		}
		else
			Message (MSGERR, "CutNodePortals_r: Mislinked portal near %s", GetCoord(p->winding->points[0]));

		w = ClipWinding (w, &clipplane, true);
		if (!w)
		{
			FreePortal (new_portal);
			new_portal = NULL;
			Message (MSGWARN, "CutNodePortals_r: New portal was clipped away near %s", GetCoord(p->winding->points[0]));
			break;
		}
	}

	if (w)
	{
	// if the plane was not clipped on all sides, there was an error
		new_portal->winding = w;
		AddPortalToNodes (new_portal, f, b);
	}

//
// partition the portals
//
	for (p = node->portals ; p ; p = next_portal)
	{
		if (p->nodes[0] == node)
			side = 0;
		else if (p->nodes[1] == node)
			side = 1;
		else
			Message (MSGERR, "CutNodePortals_r: Mislinked portal near %s", GetCoord(p->winding->points[0]));
		next_portal = p->next[side];

		other_node = p->nodes[!side];
		RemovePortalFromNode (p, p->nodes[0]);
		RemovePortalFromNode (p, p->nodes[1]);

//
// cut the portal into two portals, one on each side of the cut plane
//
		DivideWinding (p->winding, plane, &frontwinding, &backwinding);

		if (!frontwinding)
		{
			if (side == 0)
				AddPortalToNodes (p, b, other_node);
			else
				AddPortalToNodes (p, other_node, b);
			continue;
		}
		if (!backwinding)
		{
			if (side == 0)
				AddPortalToNodes (p, f, other_node);
			else
				AddPortalToNodes (p, other_node, f);
			continue;
		}

	// the winding is split
		new_portal = AllocPortal ();
		*new_portal = *p;
		new_portal->winding = backwinding;
		FreeWinding (p->winding);
		p->winding = frontwinding;

		if (side == 0)
		{
			AddPortalToNodes (p, f, other_node);
			AddPortalToNodes (new_portal, b, other_node);
		}
		else
		{
			AddPortalToNodes (p, other_node, f);
			AddPortalToNodes (new_portal, other_node, b);
		}
	}

	CutNodePortals_r (f);
	CutNodePortals_r (b);

}


/*
==================
PortalizeWorld

Builds the exact polyhedrons for the nodes and leafs
==================
*/
void PortalizeWorld (node_t *headnode, qboolean WriteFile)
{
	if (WriteFile)
		Message (MSGNOVERBOSE, "------ Portalize ------");

	MakeHeadnodePortals (headnode);
	CutNodePortals_r (headnode);

// set the visleafnum field in every leaf and count the total number of portals
	num_visleafs = 0;
	num_visportals = 0;
	NumberLeafs_r (headnode);
}


/*
==================
FreeAllPortals

==================
*/
void FreeAllPortals (node_t *node)
{
	portal_t	*p, *nextp;

	if (!node->contents)
	{
		FreeAllPortals (node->children[0]);
		FreeAllPortals (node->children[1]);
	}

	for (p=node->portals ; p ; p=nextp)
	{
		if (p->nodes[0] == node)
			nextp = p->next[0];
		else
			nextp = p->next[1];
		RemovePortalFromNode (p, p->nodes[0]);
		RemovePortalFromNode (p, p->nodes[1]);
		FreeWinding (p->winding);
		FreePortal (p);
	}
	node->portals = NULL;
}

/*
==============================================================================

PORTAL FILE GENERATION

==============================================================================
*/

#define	PORTALFILE	"PRT1"

FILE	*pf;

void WriteFloat (vec_t v)
{
	if ( fabs(v - tx_Q_rint(v)) < 0.001 )
		SafePrintf (pf, portfilename, "%i ", (int)tx_Q_rint(v));
	else
		SafePrintf (pf, portfilename, "%f ", v);
}

static qboolean SameContent(int Cont0, int Cont1)
{
	if (Cont0 == Cont1 && Cont0 != CONTENTS_SKY ||
	    options.watervis && (Cont0 >= CONTENTS_LAVA && Cont0 <= CONTENTS_WATER && Cont1 == CONTENTS_EMPTY ||
				 Cont0 == CONTENTS_EMPTY && Cont1 >= CONTENTS_LAVA && Cont1 <= CONTENTS_WATER))
		return(true);

	return(false);
}

void WritePortalFile_r (node_t *node)
{
	int		i;
	portal_t	*p;
	winding_t	*w;
	plane_t		*pl, plane2;

	if (!node->contents)
	{
		WritePortalFile_r (node->children[0]);
		WritePortalFile_r (node->children[1]);
		return;
	}

	if (node->contents == CONTENTS_SOLID)
		return;

	for (p = node->portals ; p ; )
	{
		w = p->winding;
		if (w && p->nodes[0] == node && SameContent(p->nodes[0]->contents, p->nodes[1]->contents))
		{
			// write out to the file

			// sometimes planes get turned around when they are very near
			// the changeover point between different axis.  interpret the
			// plane the same way vis will, and flip the side orders if needed
			pl = &planes[p->planenum];
			PlaneFromWinding (w, &plane2);
			if ( DotProduct (pl->normal, plane2.normal) < 0.99 )
			{	// backwards...
				SafePrintf (pf, portfilename, "%i %i %i ", w->numpoints, p->nodes[1]->visleafnum, p->nodes[0]->visleafnum);
			}
			else
				SafePrintf (pf, portfilename, "%i %i %i ", w->numpoints, p->nodes[0]->visleafnum, p->nodes[1]->visleafnum);

			for (i=0 ; i<w->numpoints ; i++)
			{
				SafePrintf (pf, portfilename, "(");
				WriteFloat (w->points[i][0]);
				WriteFloat (w->points[i][1]);
				WriteFloat (w->points[i][2]);
				SafePrintf (pf, portfilename, ") ");
			}
			SafePrintf (pf, portfilename, "\n");
		}

		if (p->nodes[0] == node)
			p = p->next[0];
		else
			p = p->next[1];
	}

}

/*
================
NumberLeafs_r
================
*/
void NumberLeafs_r (node_t *node)
{
	portal_t	*p;

	if (!node->contents)
	{	// decision node
		node->visleafnum = -99;
		NumberLeafs_r (node->children[0]);
		NumberLeafs_r (node->children[1]);
		return;
	}

	if (node->contents == CONTENTS_SOLID)
	{	// solid block, viewpoint never inside
		node->visleafnum = -1;
		return;
	}

	node->visleafnum = num_visleafs++;

	for (p = node->portals ; p ; )
	{
		if (p->nodes[0] == node)		// only write out from first leaf
		{
			if (SameContent(p->nodes[0]->contents, p->nodes[1]->contents))
				num_visportals++;

			p = p->next[0];
		}
		else
			p = p->next[1];
	}

}


/*
================
WritePortalfile
================
*/
void WritePortalfile (node_t *headnode)
{
// set the visleafnum field in every leaf and count the total number of portals
	num_visleafs = 0;
	num_visportals = 0;
	NumberLeafs_r (headnode);

// write the file
	pf = SafeOpen (portfilename, "w", true, NULL);

	SafePrintf (pf, portfilename, "%s\n", PORTALFILE);
	SafePrintf (pf, portfilename, "%i\n", num_visleafs);
	SafePrintf (pf, portfilename, "%i\n", num_visportals);

	WritePortalFile_r (headnode);

	SafeClose (pf, NULL);

	Message (MSGNOVERBOSE, "%6i vis leafs", num_visleafs);
	Message (MSGNOVERBOSE, "%6i vis portals", num_visportals);
}


