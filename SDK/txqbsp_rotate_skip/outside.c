
#include "bsp5.h"

int	 outleafs;
int	 numleaks;
qboolean LeakFileGenerated = false;
portal_t **pLeaks = NULL;
FILE	 *leakfile;

/*
===========
PointInLeaf
===========
*/
node_t	*PointInLeaf (node_t *node, vec3_t point)
{
	vec_t	d;

	if (node->contents)
		return node;

	d = DotProduct (planes[node->planenum].normal, point) - planes[node->planenum]. dist;

	if (d > 0)
		return PointInLeaf (node->children[0], point);

	return PointInLeaf (node->children[1], point);
}

/*
===========
PlaceOccupant
===========
*/
qboolean PlaceOccupant (int num, vec3_t point, node_t *headnode)
{
	node_t	*n;

	n = PointInLeaf (headnode, point);
	if (n->contents == CONTENTS_SOLID)
		return false;
	n->occupied = num;
	return true;
}

/*
==============
PrintLeakTrail
==============
*/
void PrintLeakTrail (vec3_t p1, vec3_t p2)
{
	int	i;
	vec3_t	dir;
	vec_t	len;

	VectorSubtract (p2, p1, dir);
	len = tx_VectorLength (dir);
	tx_VectorNormalize (dir);

	while (len > options.LeakDist)
	{
		SafePrintf (leakfile, pointfilename, "%f %f %f\n", p1[0], p1[1], p1[2]);

		for (i = 0; i < 3; ++i)
			p1[i] += dir[i] * options.LeakDist;

		len -= options.LeakDist;
	}
}

/*
==============
MarkLeakTrail
==============
*/
void MarkLeakTrail (portal_t *n2)
{
	vec3_t	 p1, p2;
	portal_t *n1;

	if (LeakFileGenerated || options.ReqLeakHull != -1 && options.ReqLeakHull != hullnum)
		return;

	if (numleaks > num_visportals)
		Message (MSGERR, "MarkLeakTrail: numleaks (%d) > num_visportals (%d)", numleaks, num_visportals);

	pLeaks[numleaks] = n2;
	numleaks++;

	MidpointWinding(n2->winding, p1);

	if (numleaks < 2 || !options.OldLeak)
		return;

	n1 = pLeaks[numleaks - 2];

	MidpointWinding(n1->winding, p2);

	PrintLeakTrail (p1, p2);
}

vec3_t v1, v2;

/*
=================
LineIntersect_r

Returns true if the line segment v1, v2 does not intersect any of the faces
in the node, false if it does.
=================
*/
qboolean LineIntersect_r(node_t *n)
{
	face_t	*f, **fp;
	vec_t   dist1, dist2;
	vec3_t  dir;
	vec3_t  mid, mins, maxs;
	plane_t *p;
	int     i, j;

	// Process this node's faces if leaf node
	if (n->contents)
	{
		if (n->markfaces)
		{
		 	for (fp=n->markfaces; *fp; fp++)
		 	{
		 		for (f=*fp; f; f=f->original)
		 		{
		 			p = &planes[f->planenum];
		 			dist1 = DotProduct(v1, p->normal) - p->dist;
		 			dist2 = DotProduct(v2, p->normal) - p->dist;

		 			// Line segment doesn't cross the plane
		 			if (dist1 < -ON_EPSILON && dist2 < -ON_EPSILON ||
		 		 	     dist1 > ON_EPSILON && dist2 > ON_EPSILON)
		 		 		continue;

		 			if (fabs(dist1) < ON_EPSILON)
		 			{
		 		 		VectorCopy(v1, mid);

		 		 		if (fabs(dist2) < ON_EPSILON)
		 		 			return false; // Line too short, don't risk it...
		 			}
		 			else if (fabs(dist2) < ON_EPSILON)
		 			{
		 		 		VectorCopy(v2, mid);
		 			}
		 			else
		 			{
		 		 		// Find the midpoint on the plane of the face
		 		 		VectorSubtract(v2, v1, dir);
		 		 		tx_VectorMA(v1, dist1 / (dist1-dist2), dir, mid);
		 			}

		 			// Do fast (bounding box) test for point outside polygon
		 			VectorCopy(f->pts[0], mins);
		 			VectorCopy(f->pts[0], maxs);

		 			for (i=1; i<f->numpoints; i++)
		 		 		for (j=0; j<3; j++)
		 		 		{
		 		 			if (f->pts[i][j] < mins[j])
		 		 		 		mins[j] = f->pts[i][j];
		 		 			if (f->pts[i][j] > maxs[j])
		 		 		 		maxs[j] = f->pts[i][j];
		 		 		}

		 			if (mid[0] < mins[0] - ON_EPSILON || mid[0] > maxs[0] + ON_EPSILON ||
		 		 	    mid[1] < mins[1] - ON_EPSILON || mid[1] > maxs[1] + ON_EPSILON ||
		 		 	    mid[2] < mins[2] - ON_EPSILON || mid[2] > maxs[2] + ON_EPSILON)
		 		 		continue;
					else
		 				return false;
				}
			}
		}
	}
	else
	{
		p = &planes[n->planenum];

		dist1 = DotProduct(v1, p->normal) - p->dist;
		dist2 = DotProduct(v2, p->normal) - p->dist;

		if (dist1 < -ON_EPSILON && dist2 < -ON_EPSILON)
			return LineIntersect_r(n->children[1]);
		if (dist1 > ON_EPSILON && dist2 > ON_EPSILON)
			return LineIntersect_r(n->children[0]);

		if (!LineIntersect_r(n->children[1]))
			return false;
		if (!LineIntersect_r(n->children[0]))
			return false;
	}

	return true;
}

/*
=================
OptimizeLeakline
=================
*/
void OptimizeLeakline(node_t *headnode, qboolean Final)
{
	int	 i, j, k;
	portal_t *p1, *p2;

	i = 0;

	while (i < numleaks - 1)
	{
		while (pLeaks[i] == NULL)
			++i;

		p1 = pLeaks[i];
		MidpointWinding(p1->winding, v1);

		if (Final)
			j = numleaks - 1; // Make final cleanup
		else
		{
			// Simplify one bit at a time to speed things up
			j = i + options.SimpDist;

			if (j > numleaks - 1)
				j = numleaks - 1;
		}

		while (j > i + 1)
		{
			p2 = pLeaks[j];
			MidpointWinding(p2->winding, v2);

			if (LineIntersect_r(headnode))
			{
				// Shortcut found; Remove leak portals inbetween
				for (k = i + 1; k < j; ++k)
					pLeaks[k] = NULL;

				break;
			}
			else
			{
				// Shortcut not found; Try a closer portal
				do
					--j;
				while (j > i + 1 && pLeaks[j] == NULL);
			}
		}

		i = j;
	}
}

/*
=================
SimplifyLeakline
=================
*/
void SimplifyLeakline(node_t *headnode)
{
	int	 i, j;
	portal_t *p1, *p2;

	if (numleaks < 2)
		return;

	OptimizeLeakline(headnode, false);

	if (options.SimpDist < numleaks - 1)
		OptimizeLeakline(headnode, true);

	i = 0;

	while (i < numleaks - 1)
	{
		p1 = pLeaks[i];
		MidpointWinding(p1->winding, v1);

		j = i + 1;

		while (pLeaks[j] == NULL)
			++j;

		p2 = pLeaks[j];
		MidpointWinding(p2->winding, v2);

		PrintLeakTrail (v1, v2);

		i = j;
	}
}


/*
==================
RecursiveFillOutside

If fill is false, just check, don't fill
Returns true if an occupied leaf is reached
==================
*/
int hit_occupied;

qboolean RecursiveFillOutside (node_t *l, qboolean fill)
{
	portal_t	*p;
	int			s;

	if (l->contents == CONTENTS_SOLID || l->contents == CONTENTS_SKY)
		return false;

	if (l->valid == valid)
		return false;

	if (l->occupied && (!fill || !options.ForcedFill || hullnum == 0))
	{
		hit_occupied = l->occupied;
		return true;
	}

	l->valid = valid;

// fill it and it's neighbors
	if (fill)
	{
		l->contents = CONTENTS_SOLID;
		outleafs++;
	}

	for (p=l->portals ; p ; )
	{
		s = (p->nodes[0] == l);

		if (RecursiveFillOutside (p->nodes[s], fill) )
		{	// leaked, so stop filling
			MarkLeakTrail (p);
			return true;
		}
		p = p->next[!s];
	}

	return false;
}

/*
==================
ClearOutFaces

==================
*/
void ClearOutFaces (node_t *node)
{
	face_t	**fp;

	if (node->planenum != -1)
	{
		ClearOutFaces (node->children[0]);
		ClearOutFaces (node->children[1]);
		return;
	}
	if (node->contents != CONTENTS_SOLID)
		return;

	if (node->markfaces)
	{
		for (fp=node->markfaces ; *fp ; fp++)
		{
		// mark all the original faces that are removed
			ResizeFace (*fp, 0);
		}
	}
	node->faces = NULL;
}


//=============================================================================

/*
===========
FillOutside

===========
*/
qboolean FillOutside (node_t *node)
{
	int	 s;
	int	 i;
	qboolean inside, LeakOccurred = false;

	Message (MSGVERBOSE, "------ FillOutside ------");

	inside = false;
	for (i=1 ; i<num_entities ; i++)
	{
		if (!tx_VectorCompare (entities[i].origin, tx_vec3_origin))
		{
			if (PlaceOccupant (i, entities[i].origin, node))
				inside = true;
		}
	}

	if (!inside && !options.noents)
	{
		Message (MSGWARNCRIT, "No entities in empty space -- no filling performed");
		return false;
	}

	s = !(outside_node.portals->nodes[1] == &outside_node);

// first check to see if an occupied leaf is hit
	outleafs = 0;
	numleaks = 0;
	valid++;

	if (!LeakFileGenerated)
	{
		pLeaks = (portal_t **)AllocOther (sizeof(portal_t *) * num_visportals);
		leakfile = SafeOpen (pointfilename, "w", true, NULL);
	}

	if (RecursiveFillOutside (outside_node.portals->nodes[s], false))
	{
		LeakOccurred = true;

		Message (MSGWARNCRIT, "Reached occupant at %s, %s", GetCoord (entities[hit_occupied].origin), ValueForKey(&entities[hit_occupied], "classname"));

		if (!LeakFileGenerated && (options.ReqLeakHull == -1 || options.ReqLeakHull == hullnum))
		{
			if (!options.OldLeak)
			{
				Message (MSGALWAYSEXACT, "Simplifying ... ");
				SimplifyLeakline (node);
			}

			Message (MSGALWAYS, "Leak file written to %s", pointfilename);
			LeakFileGenerated = true;

			// Get rid of .prt file if .pts file is generated
			remove (portfilename);
		}
	}

	if (pLeaks != NULL)
	{
		FreeOther (pLeaks);
		pLeaks = NULL;
		SafeClose (leakfile, NULL);

		if (numleaks == 0)
			remove (pointfilename); // Get rid of 0-byte .pts file
	}

	if (LeakOccurred)
	{
		if (!options.ForcedFill || hullnum == 0)
			return false;

		Message (MSGALWAYS, "Force filling");
	}
	else if (options.NoFillVis && hullnum == 0 ||
		 options.NoFillClip && hullnum == 1)
	{
		// No filling of visible (0) or player clipping (1) hull
		Message (MSGALWAYS, "No filling");
		return false;
	}

// now go back and fill things in
	valid++;
	RecursiveFillOutside (outside_node.portals->nodes[s], true);

// remove faces from filled in leafs
	ClearOutFaces (node);

	Message (MSGVERBOSE, "%6i outleafs", outleafs);
	return true;
}


