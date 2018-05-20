// divide.h

#include "bsp5.h"


surface_t	newcopy_t;

/*
a surface has all of the faces that could be drawn on a given plane

the outside filling stage can remove some of them so a better bsp can be generated

*/

int	Face, TotalFaces;


/*
===============
SubdivideFace

If the face is >256 in either texture direction, carve a valid sized
piece off and insert the remainder in the next link
===============
*/
void SubdivideFace (face_t *f, face_t **prevptr)
{
	vec_t		mins, maxs;
	vec_t		v;
	int			axis, i;
	plane_t		plane;
	face_t		*front, *back, *next;
	texinfo_t	*tex;

// special (non-surface cached) faces don't need subdivision
	tex = &texinfo[f->texturenum];

	if ( tex->flags & TEX_SPECIAL)
		return;


	for (axis = 0 ; axis < 2 ; axis++)
	{
		while (1)
		{
			mins = 9999;
			maxs = -9999;

			for (i=0 ; i<f->numpoints ; i++)
			{
				v = DotProduct (f->pts[i], tex->vecs[axis]);
				if (v < mins)
					mins = v;
				if (v > maxs)
					maxs = v;
			}

			if (maxs - mins <= options.subdivide_size)
				break;

		// split it
			VectorCopy (tex->vecs[axis], plane.normal);
			v = tx_VectorLength (plane.normal);
			tx_VectorNormalize (plane.normal);
			plane.dist = (mins + options.subdivide_size - 16)/v;
			next = f->next;
			SplitFace (f, &plane, &front, &back);

			if (!front || !back)
				Message (MSGERR, "SubdivideFace: Didn't split the polygon near %s, %s",
					 GetCoord (f->pts[0]), miptex[texinfo[f->texturenum].miptex]);

			*prevptr = back;
			back->next = front;
			front->next = next;
			f = back;
		}
	}
}

/*
=============================================================================

GatherNodeFaces

Frees the current node tree and returns a new chain of the surfaces that
have inside faces.
=============================================================================
*/

void GatherNodeFaces_r (node_t *node)
{
	face_t	*f, *next;

	if (node->planenum != PLANENUM_LEAF)
	{
//
// decision node
//
		for (f=node->faces ; f ; f=next)
		{
			next = f->next;
			if (!f->numpoints)
			{	// face was removed outside
				FreeFace (f);
			}
			else
			{
				f->next = validfaces[f->planenum];
				validfaces[f->planenum] = f;
			}
		}

		GatherNodeFaces_r (node->children[0]);
		GatherNodeFaces_r (node->children[1]);

		FreeNode (node);
	}
	else
	{
//
// leaf node
//
		if (node->markfaces != NULL)
			FreeOther(node->markfaces);

		FreeNode (node);
	}
}

/*
================
GatherNodeFaces

================
*/
surface_t *GatherNodeFaces (node_t *headnode)
{
	ClearArray(validfaces);
        validfacesactive = 1;
	ExtendArray(validfaces, numbrushplanes-1);
	GatherNodeFaces_r (headnode);
	return BuildSurfaces ();
}

//===========================================================================

typedef struct hashvert_s
{
	struct hashvert_s	*next;
	vec3_t	point;
	int		num;
	int		numplanes;		// for corner determination
	int		planenums[2];
	int		numedges;
} hashvert_t;

#define	POINT_EPSILON	0.01

SetupArray(hashvert_t,	hvertex, MAX_MAP_VERTS);
hashvert_t	*hvert_p;

SetupArray(face_t_p2, edgefaces, MAX_MAP_EDGES);
int		firstmodeledge = 1;
int		firstmodelface;

//============================================================================

#define	NUM_HASH	4096

hashvert_t	*hashverts[NUM_HASH];

static	vec3_t	hash_min, hash_scale;

static	void InitHash (void)
{
	vec3_t	size;
	vec_t	volume;
	vec_t	scale;
	int		newsize[2];
	int		i;

	memset (hashverts, 0, sizeof(hashverts));

	for (i=0 ; i<3 ; i++)
	{
		hash_min[i] = -8000;
		size[i] = 16000;
	}

	volume = size[0]*size[1];

	scale = sqrt(volume / NUM_HASH);

	newsize[0] = size[0] / scale;
	newsize[1] = size[1] / scale;

	hash_scale[0] = newsize[0] / size[0];
	hash_scale[1] = newsize[1] / size[1];
	hash_scale[2] = newsize[1];

	CreateArray(hashvert_t, hvertex, MAX_MAP_VERTS);
	hvert_p = hvertex;
}

static	unsigned HashVec (vec3_t vec)
{
	unsigned	h;

	h =	hash_scale[0] * (vec[0] - hash_min[0]) * hash_scale[2]
		+ hash_scale[1] * (vec[1] - hash_min[1]);
	if ( h >= NUM_HASH)
		return NUM_HASH - 1;
	return h;
}


/*
=============
GetVertex
=============
*/
int	GetVertex (vec3_t in, int planenum)
{
	int			h;
	int			i;
	hashvert_t	*hv;
	vec3_t		vert;

	for (i=0 ; i<3 ; i++)
	{
		if ( fabs(in[i] - tx_Q_rint(in[i])) < 0.001)
			vert[i] = tx_Q_rint(in[i]);
		else
			vert[i] = in[i];
	}

	h = HashVec (vert);

	for (hv=hashverts[h] ; hv ; hv=hv->next)
	{
		if ( fabs(hv->point[0]-vert[0])<POINT_EPSILON
		&& fabs(hv->point[1]-vert[1])<POINT_EPSILON
		&& fabs(hv->point[2]-vert[2])<POINT_EPSILON )
		{
			hv->numedges++;
			if (hv->numplanes == 3)
				return hv->num;		// allready known to be a corner
			for (i=0 ; i<hv->numplanes ; i++)
				if (hv->planenums[i] == planenum)
					return hv->num;		// allready know this plane
			if (hv->numplanes != 2)
				hv->planenums[hv->numplanes] = planenum;
			hv->numplanes++;
			return hv->num;
		}
	}

	ExtendArray(hvertex, hvert_p-hvertex);
	hv = hvert_p;
	hv->numedges = 1;
	hv->numplanes = 1;
	hv->planenums[0] = planenum;
	hv->next = hashverts[h];
	hashverts[h] = hv;
	VectorCopy (vert, hv->point);
	hv->num = numvertexes;
	hvert_p++;

// emit a vertex
	ExtendArray(dvertexes, numvertexes);

	dvertexes[numvertexes].point[0] = vert[0];
	dvertexes[numvertexes].point[1] = vert[1];
	dvertexes[numvertexes].point[2] = vert[2];
	numvertexes++;

	return hv->num;
}

//===========================================================================


/*
==================
GetEdge

Don't allow four way edges
==================
*/
int GetEdge (vec3_t p1, vec3_t p2, face_t *f)
{
	int		v1, v2;
	dedge_t	*edge;
	int		i;

	if (!f->contents[0])
		Message (MSGERR, "GetEdge: 0 contents");

	v1 = GetVertex (p1, f->planenum);
	v2 = GetVertex (p2, f->planenum);
	for (i=firstmodeledge ; i < numedges ; i++)
	{
		edge = &dedges[i];
		if (v1 == edge->v[1] && v2 == edge->v[0]
		&& !edgefaces[i][1]
		&& edgefaces[i][0]->contents[0] == f->contents[0])
		{
			edgefaces[i][1] = f;
			return -i;
		}
	}

// emit an edge
	ExtendArray(edgefaces, numedges);
	ExtendArray(dedges, numedges);
	edge = &dedges[numedges];
	numedges++;
	edge->v[0] = v1;
	edge->v[1] = v2;
	edgefaces[i][0] = f;

	return i;
}


/*
==================
FindFaceEdges
==================
*/
void FindFaceEdges (face_t *face)
{
	int		i;

	face->outputnumber = -1;
	if (face->numpoints > MAXEDGES)
		Message (MSGERR, "WriteFace: %i points", face->numpoints);

	face->edges = (int *)AllocOther(face->numpoints * sizeof(int));
	for (i=0; i<face->numpoints ; i++)
		face->edges[i] =  GetEdge
		(face->pts[i], face->pts[(i+1)%face->numpoints], face);
}

/*
================
MakeFaceEdges_r
================
*/
void MakeFaceEdges_r (node_t *node)
{
	face_t	*f;

	ShowBar(++Face, TotalFaces);

	if (node->planenum == PLANENUM_LEAF)
		return;

	for (f=node->faces ; f ; f=f->next)
		FindFaceEdges (f);

	MakeFaceEdges_r (node->children[0]);
	MakeFaceEdges_r (node->children[1]);
}

void MakeFaceEdges_c (node_t *node)
{
	++TotalFaces;

	if (node->planenum == PLANENUM_LEAF)
		return;

	MakeFaceEdges_c (node->children[0]);
	MakeFaceEdges_c (node->children[1]);
}

/*
================
MakeFaceEdges
================
*/
void MakeFaceEdges (node_t *headnode)
{
	if (worldmodel)
		Message (MSGNOVERBOSE, "------ MakeFaceEdges ------");

	InitHash ();
	Face = TotalFaces = 0;

	MakeFaceEdges_c (headnode);
	MakeFaceEdges_r (headnode);

	ShowBar(-1, -1);

	GrowNodeRegions (headnode);

	firstmodeledge = numedges;
	firstmodelface = numfaces;
}

