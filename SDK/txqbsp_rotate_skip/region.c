// region.h

#include "bsp5.h"

/*

input
-----
vertexes
edges
faces

output
------
smaller set of vertexes
smaller set of edges
regions
? triangulated regions
face to region mapping numbers

*/

/*
==============
CountRealNumbers
==============
*/
void CountRealNumbers (void)
{
	int		i;
	int		c;

	Message (MSGVERBOSE, "%6i regions", numfaces-firstmodelface);

	c = 0;
	for (i=firstmodelface ; i<numfaces ; i++)
		c += dfaces[i].numedges;
	Message (MSGVERBOSE, "%6i real marksurfaces", c);

	c = 0;
	for (i=firstmodeledge ; i<numedges ; i++)
		if (edgefaces[i][0])
			c++;		// not removed

	Message (MSGVERBOSE, "%6i real edges", c);
}

//=============================================================================

/*
==============
GrowNodeRegion_r
==============
*/
void GrowNodeRegion_r (node_t *node)
{
	dface_t		*r;
	face_t		*f;
	int			i;

	if (node->planenum == PLANENUM_LEAF)
		return;

	node->firstface = numfaces;

	for (f=node->faces ; f ; f=f->next)
	{
//		if (f->outputnumber != -1)
//			continue;	// allready grown into an earlier region

	// emit a region

		ExtendArray(dfaces, numfaces);
		f->outputnumber = numfaces;
		r = &dfaces[numfaces];

		r->planenum = node->outputplanenum;
		r->side = f->planeside;
		r->texinfo = f->texturenum;
		for (i=0 ; i<MAXLIGHTMAPS ; i++)
			r->styles[i] = 255;
		r->lightofs = -1;

	// add the face and mergable neighbors to it

		r->firstedge = numsurfedges;
		for (i=0 ; i<f->numpoints ; i++)
		{
			ExtendArray(dsurfedges, numsurfedges);
			dsurfedges[numsurfedges] = f->edges[i];
			numsurfedges++;
		}

		FreeOther(f->edges);

		r->numedges = numsurfedges - r->firstedge;

		numfaces++;
	}

	node->numfaces = numfaces - node->firstface;

	GrowNodeRegion_r (node->children[0]);
	GrowNodeRegion_r (node->children[1]);
}


/*
==============
GrowNodeRegions
==============
*/
void GrowNodeRegions (node_t *headnode)
{
	if (worldmodel)
		Message (MSGNOVERBOSE, "------ GrowRegions ------");

	GrowNodeRegion_r (headnode);

	if (options.allverbose)
		CountRealNumbers ();
}

/*
===============================================================================

Turn the faces on a plane into optimal non-convex regions
The edges may still be split later as a result of tjunctions

typedef struct
{
	vec3_t	dir;
	vec3_t	origin;
	vec3_t	p[2];
}
for all faces
	for all edges
		for all edges so far
			if overlap
				split


===============================================================================
*/













