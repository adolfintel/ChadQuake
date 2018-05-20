// brush.c

#include "bsp5.h"

int			numbrushplanes;
int			CurrLine;
SetupArray(plane_t,		planes, MAX_MAP_PLANES);

int                     validfacesactive;
SetupArray(face_t*, validfaces, MAX_MAP_PLANES);

int			numbrushfaces;
mface_t			faces[MAX_FACES]; //128		// beveled clipping hull can generate many extra
entity_t 		*CurrEnt;


/*
=================
CheckFace

Note: this will not catch 0 area polygons
=================
*/
void CheckFace (face_t *f)
{
	int	i, j;
	vec_t	*p1, *p2;
	vec_t	d, edgedist;
	vec3_t	dir, edgenormal, facenormal;

	if (f->numpoints < 3)
		Message (MSGWARN, "CheckFace: Face with too few (%i) points at %s", f->numpoints, GetCoord (f->pts[0]));

	VectorCopy (planes[f->planenum].normal, facenormal);
	if (f->planeside)
	{
		VectorSubtract (tx_vec3_origin, facenormal, facenormal);
	}

	for (i=0 ; i<f->numpoints ; i++)
	{
		p1 = f->pts[i];

		for (j=0 ; j<3 ; j++)
			if (p1[j] > BOGUS_RANGE || p1[j] < -BOGUS_RANGE)
				Message (MSGERR, "CheckFace: BOGUS_RANGE: %s", GetCoord (p1));

		j = i+1 == f->numpoints ? 0 : i+1;

	// check the point is on the face plane
		d = DotProduct (p1, planes[f->planenum].normal) - planes[f->planenum].dist;
		if (d < -ON_EPSILON || d > ON_EPSILON)
		{
			Message (MSGWARN, "CheckFace: Healing point %s off plane by %.3g", GetCoord (p1), d);

			// Adjust point to plane
			tx_VectorMA (p1, -d, planes[f->planenum].normal, f->pts[i]);
			p1 = f->pts[i];
		}

	// check the edge isn't degenerate
		p2 = f->pts[j];
		VectorSubtract (p2, p1, dir);

		if (tx_VectorLength (dir) < ON_EPSILON)
		{
			Message (MSGWARN, "CheckFace: Healing degenerate edge at %s", GetCoord (p1));
			for (j=i+1; j<f->numpoints; j++)
				VectorCopy(f->pts[j], f->pts[j-1]);
			ResizeFace (f, f->numpoints - 1);
			CheckFace(f);
			break;
		}

		tx_CrossProduct (facenormal, dir, edgenormal);
		tx_VectorNormalize (edgenormal);
		edgedist = DotProduct (p1, edgenormal);
		edgedist += ON_EPSILON;

	// all other points must be on front side
		for (j=0 ; j<f->numpoints ; j++)
		{
			if (j == i)
				continue;
			d = DotProduct (f->pts[j], edgenormal);
			if (d > edgedist)
				Message (MSGERR, "CheckFace: Non-convex at %s", GetCoord (p1));
		}
	}
}


//===========================================================================

/*
=================
ClearBounds
=================
*/
void ClearBounds (brushset_t *bs)
{
	int i;

	for (i=0 ; i<3 ; i++)
	{
		bs->mins[i] = 99999;
		bs->maxs[i] = -99999;
	}
}

/*
=================
AddToBounds
=================
*/
void AddToBounds (brushset_t *bs, vec3_t v)
{
	int		i;

	for (i=0 ; i<3 ; i++)
	{
		if (v[i] < bs->mins[i])
			bs->mins[i] = v[i];
		if (v[i] > bs->maxs[i])
			bs->maxs[i] = v[i];
	}
}

//===========================================================================

int	PlaneTypeForNormal (vec3_t normal)
{
	vec_t ax, ay, az;

// NOTE: should these have an epsilon around 1.0?
	if (normal[0] == 1.0)
		return PLANE_X;
	if (normal[1] == 1.0)
		return PLANE_Y;
	if (normal[2] == 1.0)
		return PLANE_Z;
	if (normal[0] == -1.0 ||
		normal[1] == -1.0 ||
		normal[2] == -1.0)
		Message (MSGERR, "PlaneTypeForNormal: Not a canonical vector");

	ax = fabs(normal[0]);
	ay = fabs(normal[1]);
	az = fabs(normal[2]);

	if (ax >= ay && ax >= az)
		return PLANE_ANYX;
	if (ay >= ax && ay >= az)
		return PLANE_ANYY;
	return PLANE_ANYZ;
}

void NormalizePlane (plane_t *dp)
{
	vec_t	ax, ay, az;

	if (dp->normal[0] == -1.0)
	{
		dp->normal[0] = 1.0;
		dp->dist = -dp->dist;
	}
	if (dp->normal[1] == -1.0)
	{
		dp->normal[1] = 1.0;
		dp->dist = -dp->dist;
	}
	if (dp->normal[2] == -1.0)
	{
		dp->normal[2] = 1.0;
		dp->dist = -dp->dist;
	}

	if (dp->normal[0] == 1.0)
	{
		dp->type = PLANE_X;
		return;
	}
	if (dp->normal[1] == 1.0)
	{
		dp->type = PLANE_Y;
		return;
	}
	if (dp->normal[2] == 1.0)
	{
		dp->type = PLANE_Z;
		return;
	}

	ax = fabs(dp->normal[0]);
	ay = fabs(dp->normal[1]);
	az = fabs(dp->normal[2]);

	if (ax >= ay && ax >= az)
		dp->type = PLANE_ANYX;
	else if (ay >= ax && ay >= az)
		dp->type = PLANE_ANYY;
	else
		dp->type = PLANE_ANYZ;
	if (dp->normal[dp->type-PLANE_ANYX] < 0)
	{
		VectorSubtract (tx_vec3_origin, dp->normal, dp->normal);
		dp->dist = -dp->dist;
	}

}

/*
===============
FindPlane

Returns a global plane number and the side that will be the front
===============
*/
int	FindPlane (plane_t *dplane, int *side)
{
	int			i;
	plane_t		*dp, pl;
	vec_t		dot;

	dot = tx_VectorLength(dplane->normal);
	if (dot < 1.0 - ANGLEEPSILON || dot > 1.0 + ANGLEEPSILON)
		Message (MSGERR, "FindPlane: Normalization error");

	pl = *dplane;
	NormalizePlane (&pl);
	if (DotProduct(pl.normal, dplane->normal) > 0)
		*side = 0;
	else
		*side = 1;

	dp = planes;
	for (i=0 ; i<numbrushplanes;i++, dp++)
	{
		dot = DotProduct (dp->normal, pl.normal);
		if (dot > 1.0 - ANGLEEPSILON
		&& fabs(dp->dist - pl.dist) < DISTEPSILON )
		{	// regular match
			return i;
		}
	}

        if (validfacesactive)
                ExtendArray(validfaces, numbrushplanes);
	ExtendArray(planes, numbrushplanes);
	planes[numbrushplanes] = pl;

	numbrushplanes++;

	return numbrushplanes-1;
}


/*
=============================================================================

			TURN BRUSHES INTO GROUPS OF FACES

=============================================================================
*/

vec3_t		brush_mins, brush_maxs;
face_t		*brush_faces;

/*
=================
FindTargetEntity
=================
*/
int FindTargetEntity(char *Target, int *BadTarget)
{
	int Ent;

	for (Ent = 0; Ent < num_entities; ++Ent)
	{
		// Require correct targetname, (classname) and origin key
		if (!stricmp(Target, ValueForKey(&entities[Ent], "targetname")))
		{
			*BadTarget = Ent;

//			if (!stricmp("info_rotate", ValueForKey(&entities[Ent], "classname")) &&
			if (strlen(ValueForKey(&entities[Ent], "origin")) != 0)
				return Ent;
		}
	}

	return -1;
}


/*
=================
FixRotateOrigin
=================
*/
void FixRotateOrigin(entity_t *Ent, vec3_t offset)
{
	int		FoundEnt = -1, BadTarget = -1;
	char		*Search, Origin[100], Str[100];
	static entity_t *PrevEnt = NULL; // Prevent multiple warnings for same entity

	Search = ValueForKey(Ent, "target");

	if (strlen(Search) != 0)
	{
		FoundEnt = FindTargetEntity(Search, &BadTarget);

		if (FoundEnt != -1)
			GetVectorForKey(&entities[FoundEnt], "origin", offset);
	}

	if (Ent != PrevEnt)
	{
		if (FoundEnt == -1)
		{
			Str[0] = '\0';

			if (BadTarget != -1)
				sprintf(Str, " (line %d)", entities[BadTarget].Line);

			Message (MSGWARN, "Bad target%s for rotation entity on line %d", Str, Ent->Line);
		}

		PrevEnt = Ent;
	}

	sprintf(Origin, "%d %d %d", (int)offset[0], (int)offset[1], (int)offset[2]);
	SetKeyValue(Ent, "origin", Origin);
}

/*
=================
CreateBrushFaces
=================
*/
#define	ZERO_EPSILON	0.001
#if 1 // Rotating brush support
entity_t *FindTargetEntityHMAP2 (char *targetname)
{
	int			entnum;

	for (entnum = 0; entnum < num_entities; entnum++)
		if (!strcmp (targetname, ValueForKey (&entities[entnum], "targetname")))
			return &entities[entnum];

	return NULL;
}

void CreateBrushFaces2 (void)
{
	int	  i, j, k;
	vec3_t    offset, point;
	vec_t	  r, max, min;
	face_t	  *f;
	winding_t *w;
	plane_t	  clipplane, faceplane;
	mface_t	  *mf;
	qboolean  IsRotate = false;

	offset[0] = offset[1] = offset[2] = 0;
	min = brush_mins[0] = brush_mins[1] = brush_mins[2] = 9999999;
	max = brush_maxs[0] = brush_maxs[1] = brush_maxs[2] = -9999999;

#if 0
	// Hipnotic rotation
	IsRotate = !strncmp(ValueForKey(CurrEnt, "classname"), "rotate_", 7);

	if (IsRotate)
		FixRotateOrigin(CurrEnt, offset);
	else GetVectorForKey (CurrEnt, "origin", offset);
#else
	if (!strncmp (ValueForKey (CurrEnt, "classname"), "rotate_", 7))
	{
		entity_t	*FoundEntity;
		char 		*searchstring;
		char		text[20];

		searchstring = ValueForKey (CurrEnt, "target");
		FoundEntity = FindTargetEntityHMAP2 (searchstring);

		if (FoundEntity)
			GetVectorForKey (FoundEntity, "origin", offset);

		sprintf (text, "%g %g %g", offset[0], offset[1], offset[2]);
		SetKeyValue (CurrEnt, "origin", text);
		IsRotate = true;
	}

	GetVectorForKey (CurrEnt, "origin", offset);
#endif

	brush_faces = NULL;

	for (i = 0; i < numbrushfaces; i++)
	{
		mf = &faces[i];
		faceplane = mf->plane;
		w = BaseWindingForPlane (&faceplane);

		for (j = 0; j < numbrushfaces && w; j++)
		{
			clipplane = faces[j].plane;

			if (j == i)
				continue;

			// flip the plane, because we want to keep the back side
			VectorNegate (clipplane.normal, clipplane.normal);
			clipplane.dist *= -1;

			w = ClipWindingEpsilon (w, &clipplane, ON_EPSILON, true);
		}

		if (!w)
			continue;	// overcontrained plane

		// this face is a keeper
		f = AllocFace ();
		ResizeFace (f, w->numpoints);

		if (f->numpoints > MAXEDGES)
			Message (MSGERR, "f->numpoints (%d) > MAXEDGES (%d)", f->numpoints, MAXEDGES);

		for (j = 0; j < w->numpoints; j++)
		{
			for (k = 0; k < 3; k++)
			{
				point[k] = w->points[j][k] - offset[k];
				r = tx_Q_rint(point[k]);

				if (fabs(point[k] - r) < ZERO_EPSILON)
					f->pts[j][k] = r;
				else f->pts[j][k] = point[k];

				if (f->pts[j][k] < brush_mins[k]) brush_mins[k] = f->pts[j][k];
				if (f->pts[j][k] > brush_maxs[k]) brush_maxs[k] = f->pts[j][k];

				if (IsRotate)
				{
					if (f->pts[j][k] < min) min = f->pts[j][k];
					if (f->pts[j][k] > max) max = f->pts[j][k];
				}
			}
		}

		FreeWinding2 (w);

		faceplane.dist -= DotProduct (faceplane.normal, offset);

		f->texturenum = hullnum ? 0 : mf->texinfo;
		f->planenum = FindPlane (&faceplane, &f->planeside);
		f->next = brush_faces;
		brush_faces = f;

		CheckFace (f);
	}

	// Rotatable objects must have a bounding box big enough to
	// account for all its rotations
	if (DotProduct (offset, offset))
	{
		// ???? hmap2 rotation fix ????
		vec_t delta;

		delta = tx_RadiusFromBounds (brush_mins, brush_maxs);

		for (k = 0; k < 3; k++)
		{
			brush_mins[k] = -delta;
			brush_maxs[k] = delta;
		}
	}
}
#endif

void CreateBrushFaces (void)
{
	int	  i, j, k;
	vec3_t    offset, point;
	vec_t	  r, max, min;
	face_t	  *f;
	winding_t *w;
	plane_t	  plane;
	mface_t	  *mf;
	qboolean  IsRotate;

	offset[0] = offset[1] = offset[2] = 0;
	min = brush_mins[0] = brush_mins[1] = brush_mins[2] = 99999;
	max = brush_maxs[0] = brush_maxs[1] = brush_maxs[2] = -99999;

	// Hipnotic rotation
	IsRotate = !strncmp(ValueForKey(CurrEnt, "classname"), "rotate_", 7);

	if (IsRotate)
		FixRotateOrigin(CurrEnt, offset);

	brush_faces = NULL;

	for (i=0 ; i<numbrushfaces ; i++)
	{
		mf = &faces[i];

		w = BaseWindingForPlane (&mf->plane);

		for (j=0 ; j<numbrushfaces && w ; j++)
		{
			if (j == i)
				continue;
		// flip the plane, because we want to keep the back side
			VectorSubtract (tx_vec3_origin,faces[j].plane.normal, plane.normal);
			plane.dist = -faces[j].plane.dist;

			w = ClipWinding (w, &plane, false);
		}

		if (!w)
			continue;	// overcontrained plane

	// this face is a keeper
		f = AllocFace ();
		ResizeFace (f, w->numpoints);
		if (f->numpoints > MAXEDGES)
			Message (MSGERR, "f->numpoints (%d) > MAXEDGES (%d)", f->numpoints, MAXEDGES);

		for (j=0 ; j<w->numpoints ; j++)
		{
			for (k=0 ; k<3 ; k++)
			{
				point[k] = w->points[j][k] - offset[k];
				r = tx_Q_rint(point[k]);
				if (fabs(point[k] - r) < ZERO_EPSILON)
					f->pts[j][k] = r;
				else
					f->pts[j][k] = point[k];

				if (f->pts[j][k] < brush_mins[k])
					brush_mins[k] = f->pts[j][k];
				if (f->pts[j][k] > brush_maxs[k])
					brush_maxs[k] = f->pts[j][k];

				if (IsRotate)
				{
					if (f->pts[j][k] < min)
						min = f->pts[j][k];
					if (f->pts[j][k] > max)
						max = f->pts[j][k];
				}
			}
		}

		if (!IsRotate)
			plane = mf->plane;
		else
		{
			VectorCopy(mf->plane.normal, plane.normal);
			tx_VectorScale(mf->plane.normal, mf->plane.dist, point);
			VectorSubtract(point, offset, point);
			plane.dist = DotProduct(plane.normal, point);
		}

		FreeWinding (w);
		f->texturenum = hullnum ? 0 : mf->texinfo;
		f->planenum = FindPlane (&plane, &f->planeside);
		f->next = brush_faces;
		brush_faces = f;
		CheckFace (f);
	}

	// Rotatable objects must have a bounding box big enough to
	// account for all its rotations
	if (IsRotate)
	{
		vec_t delta;

		delta = fabs(max);
		if (fabs(min) > delta)
			delta = fabs(min);

		for (k=0; k<3; k++)
		{
			brush_mins[k] = -delta;
			brush_maxs[k] = delta;
		}
	}
}

/*
=================
FreeBrushFaces
=================
*/
void FreeBrushFaces(face_t *pFaceList)
{
	face_t *pFace, *pNext;

	for (pFace = pFaceList; pFace; pFace = pNext)
	{
		pNext = pFace->next;
		FreeFace(pFace);
	}
}

/*
=====================
FreeBrushsetBrushes
=====================
*/
void FreeBrushsetBrushes(void)
{
	brush_t *pBrush, *pNext;

	for (pBrush = brushset->brushes; pBrush; pBrush = pNext)
	{
		pNext = pBrush->next;
		FreeBrushFaces(pBrush->faces);
		FreeBrush(pBrush);
	}
}

/*
==============================================================================

BEVELED CLIPPING HULL GENERATION

This is done by brute force, and could easily get a lot faster if anyone cares.
==============================================================================
*/

vec3_t	hull_size[3][2] = {
{ {0, 0, 0}, {0, 0, 0} },
{ {-16,-16,-32}, {16,16,24} },
{ {-32,-32,-64}, {32,32,24} }

};

#define	MAX_HULL_POINTS	 512   //128
#define	MAX_HULL_EDGES	 1024  //256

int	num_hull_points;
vec3_t	hull_points[MAX_HULL_POINTS];
vec3_t	hull_corners[MAX_HULL_POINTS*8];
int	num_hull_edges;
int	hull_edges[MAX_HULL_EDGES][2];

/*
===============
ExpandSize
===============
*/
vec_t ExpandSize (int hullnum, int infront, int axis)
{
	return hull_size[hullnum][infront][axis] + options.HullExpansion[infront];
}

/*
===============
IsPerpendicular
===============
*/
qboolean IsPerpendicular(texinfo_t *tex, plane_t *plane)
{
	vec3_t texnormal;

// calculate a normal to the texture axis.  points can be moved along this
// without changing their S/T
	texnormal[0] = tex->vecs[1][1] * tex->vecs[0][2] - tex->vecs[1][2] * tex->vecs[0][1];
	texnormal[1] = tex->vecs[1][2] * tex->vecs[0][0] - tex->vecs[1][0] * tex->vecs[0][2];
	texnormal[2] = tex->vecs[1][0] * tex->vecs[0][1] - tex->vecs[1][1] * tex->vecs[0][0];

	tx_VectorNormalize (texnormal);

// flip it towards plane normal
	return DotProduct (texnormal, plane->normal) == 0;
}

/*
==========
GetTexInfo
==========
*/
int GetTexInfo(plane_t *plane)
{
	texinfo_t *tex;
	vec_t	  Dot, DotMax = 0;
	int	  i, j, texindex, miptex = -1, flags, NearestFace = -1;

	// First try to find the best texinfo in the other faces in brush
	// Then try any texinfo in the other faces in brush

	// Find nearest face (smallest angle)
	for (i = 0; i < numbrushfaces; ++i)
	{
		Dot = DotProduct(plane->normal, faces[i].plane.normal);

		if (Dot > DotMax)
		{
			DotMax = Dot;
			NearestFace = i;
		}
	}

	for (i = 0; i < 2; ++i)
	{
		for (j = 0; j < numbrushfaces; ++j)
		{
			if (i == 0 && j != NearestFace)
				continue;

			texindex = faces[j].texinfo;
			tex = &texinfo[texindex];

			if (miptex == -1)
			{
				// Save for later use
				miptex = tex->miptex;
				flags = tex->flags;
		    	}
			else if (miptex != tex->miptex || flags != tex->flags)
				continue; // We prefer same texture as nearest face

			if (!IsPerpendicular(tex, plane))
				return texindex; // This texture axis is NOT perpendicular to face
		}
	}

	// Then try any texinfo with same texture/flags and finally any texinfo at all
	for (i = 0; i < 2; ++i)
	{
		for (texindex = 0; texindex < numtexinfo; ++texindex)
		{
			tex = &texinfo[texindex];

			if (i == 0 && (miptex != tex->miptex || flags != tex->flags))
				continue;

			if (!IsPerpendicular(tex, plane))
				return texindex; // This texture axis is NOT perpendicular to face
		}
	}

	Message (MSGWARN, "GetTexInfo: Couldn't find non-perpendicular texture axis");

	return faces[0].texinfo; // Fail
}

/*
============
AddBrushPlane
=============
*/
void AddBrushPlane (plane_t *plane, qboolean CopyTexInfo)
{
	int		i;
	plane_t	*pl;
	vec_t	l;

	if (numbrushfaces == MAX_FACES)
		Message (MSGERR, "AddBrushPlane: numbrushfaces == MAX_FACES (%d) on line %d", MAX_FACES, CurrLine);
	l = tx_VectorLength (plane->normal);
	if (l < 0.999 || l > 1.001)
		Message (MSGERR, "AddBrushPlane: Bad normal");

	for (i=0 ; i<numbrushfaces ; i++)
	{
		pl = &faces[i].plane;
		if (tx_VectorCompare (pl->normal, plane->normal)
		&& fabs(pl->dist - plane->dist) < ON_EPSILON )
			return;
	}
	faces[i].plane = *plane;
	faces[i].texinfo = CopyTexInfo ? faces[0].texinfo : GetTexInfo(plane);
	numbrushfaces++;
}


/*
============
TestAddPlane

Adds the given plane to the brush description if all of the original brush
vertexes can be put on the front side
=============
*/
void TestAddPlane (plane_t *plane)
{
	int	i, c;
	vec_t	d;
	vec_t	*corner;
	plane_t	flip;
	vec3_t	inv;
	int	counts[3];
	plane_t	*pl;

// see if the plane has already been added
	for (i=0 ; i<numbrushfaces ; i++)
	{
		pl = &faces[i].plane;
		if (tx_VectorCompare (plane->normal, pl->normal) && fabs(plane->dist - pl->dist) < ON_EPSILON)
			return;
		VectorSubtract (tx_vec3_origin, plane->normal, inv);
		if (tx_VectorCompare (inv, pl->normal) && fabs(plane->dist + pl->dist) < ON_EPSILON)
			return;
	}

// check all the corner points
	counts[0] = counts[1] = counts[2] = 0;
	c = num_hull_points * 8;

	corner = hull_corners[0];
	for (i=0 ; i<c ; i++, corner += 3)
	{
		d = DotProduct (corner, plane->normal) - plane->dist;
		if (d < -ON_EPSILON)
		{
			if (counts[0])
				return;
			counts[1]++;
		}
		else if (d > ON_EPSILON)
		{
			if (counts[1])
				return;
			counts[0]++;
		}
		else
			counts[2]++;
	}

// the plane is a seperator

	if (counts[0])
	{
		VectorSubtract (tx_vec3_origin, plane->normal, flip.normal);
		flip.dist = -plane->dist;
		plane = &flip;
	}

	AddBrushPlane (plane, hullnum);
}

/*
============
AddHullPoint

Doesn't add if duplicated
=============
*/
int AddHullPoint (vec3_t p, int hullnum)
{
	int		i;
	vec_t	*c;
	int		x,y,z;

	for (i=0 ; i<num_hull_points ; i++)
		if (tx_VectorCompare (p, hull_points[i]))
			return i;

	VectorCopy (p, hull_points[num_hull_points]);

	c = hull_corners[i*8];

	for (x=0 ; x<2 ; x++)
		for (y=0 ; y<2 ; y++)
			for (z=0; z<2 ; z++)
			{
				c[0] = p[0] + ExpandSize (hullnum, x, 0);
				c[1] = p[1] + ExpandSize (hullnum, y, 1);
				c[2] = p[2] + ExpandSize (hullnum, z, 2);
				c += 3;
			}

	num_hull_points++;   //AR

	if (num_hull_points == MAX_HULL_POINTS)
		Message (MSGERR, "MAX_HULL_POINTS (%d) exceeded", MAX_HULL_POINTS);

	return i;
}

#if 1 // Rotating brush support
int AddHullPoint2 (vec3_t p, int hullnum)
{
	int		i;
	vec_t	*c;
	int		x, y, z;

	for (i = 0; i < num_hull_points; i++)
		if (tx_VectorCompare (p, hull_points[i]))
			return i;

	VectorCopy (p, hull_points[num_hull_points]);

	c = hull_corners[i*8];

	for (x = 0; x < 2; x++)
		for (y = 0; y < 2; y++)
			for (z = 0; z < 2; z++)
			{
				c[0] = p[0] - ExpandSize (hullnum, x^1, 0);
				c[1] = p[1] - ExpandSize (hullnum, y^1, 1);
				c[2] = p[2] - ExpandSize (hullnum, z^1, 2);
				c += 3;
			}

	if (num_hull_points == MAX_HULL_POINTS)
		Message (MSGERR, "AddHullPoint: MAX_HULL_POINTS");

	num_hull_points++;

	return i;
}
#endif

/*
============
AddHullEdge

Creates all of the hull planes around the given edge, if not done allready
=============
*/
void AddHullEdge (vec3_t p1, vec3_t p2, int hullnum)
{
	int	pt1, pt2;
	int	i;
	int	a, b, c, d, e;
	vec3_t	edgevec, planeorg, planevec;
	plane_t	plane;
	vec_t	l;

	pt1 = AddHullPoint (p1, hullnum);
	pt2 = AddHullPoint (p2, hullnum);

	for (i=0 ; i<num_hull_edges ; i++)
		if ( (hull_edges[i][0] == pt1 && hull_edges[i][1] == pt2)
		|| (hull_edges[i][0] == pt2 && hull_edges[i][1] == pt1) )
			return;	// allread added

	if (num_hull_edges == MAX_HULL_EDGES)
		Message (MSGERR, "MAX_HULL_EDGES (%d) exceeded", MAX_HULL_EDGES);

	hull_edges[i][0] = pt1;
	hull_edges[i][1] = pt2;
	num_hull_edges++;

	VectorSubtract (p1, p2, edgevec);
	tx_VectorNormalize (edgevec);

	for (a=0 ; a<3 ; a++)
	{
		VectorCopy (tx_vec3_origin, planevec);
		planevec[a] = 1;

		tx_CrossProduct (planevec, edgevec, plane.normal);
		l = tx_VectorLength (plane.normal);

		if (options.OldExpand)
		{
			if (l < 1-ANGLEEPSILON || l > 1+ANGLEEPSILON)
				continue;
		}
		else
		{
			// If this edge is almost parallel to the hull edge, skip it
			if (l < ANGLEEPSILON)
				continue;

			tx_VectorScale (plane.normal, 1 / l, plane.normal);
		}

		b = (a+1)%3;
		c = (a+2)%3;
		for (d=0 ; d<=1 ; d++)
			for (e=0 ; e<=1 ; e++)
			{
				VectorCopy (p1, planeorg);
				planeorg[b] += ExpandSize (hullnum, d, b);
				planeorg[c] += ExpandSize (hullnum, e, c);

				plane.dist = DotProduct (planeorg, plane.normal);
				TestAddPlane (&plane);
			}
	}
}

#if 1 // Rotating brush support
void AddHullEdge2 (vec3_t p1, vec3_t p2, int hullnum)
{
	int		pt1, pt2;
	int		i;
	int		a, b, c, d, e;
	vec3_t	edgevec, planeorg, planevec;
	plane_t	plane;
	vec_t	length;

	pt1 = AddHullPoint2 (p1, hullnum);
	pt2 = AddHullPoint2 (p2, hullnum);

	for (i = 0; i < num_hull_edges; i++)
		if ((hull_edges[i][0] == pt1 && hull_edges[i][1] == pt2) || (hull_edges[i][0] == pt2 && hull_edges[i][1] == pt1))
			return;	// already added

	if (num_hull_edges == MAX_HULL_EDGES)
		Message (MSGERR, "AddHullEdge: MAX_HULL_EDGES");

	hull_edges[num_hull_edges][0] = pt1;
	hull_edges[num_hull_edges][1] = pt2;
	num_hull_edges++;

	VectorSubtract (p1, p2, edgevec);
	tx_VectorNormalize (edgevec);

	for (a = 0; a < 3; a++)
	{
		b = (a + 1) % 3;
		c = (a + 2) % 3;

		planevec[a] = 1;
		planevec[b] = 0;
		planevec[c] = 0;
		tx_CrossProduct (planevec, edgevec, plane.normal);
		length = tx_VectorNormalize (plane.normal);

#define ANGLE_EPSILON      ((vec_t)0.000001)

		/* If this edge is almost parallel to the hull edge, skip it. */
		if (length < ANGLE_EPSILON)
			continue;

		for (d = 0; d < 2; d++)
		{
			for (e = 0; e < 2; e++)
			{
				// ???? does this expand in the wrong direction ????
				VectorCopy (p1, planeorg);
				planeorg[b] -= ExpandSize (hullnum, d^1, b);
				planeorg[c] -= ExpandSize (hullnum, e^1, c);
				plane.dist = DotProduct (planeorg, plane.normal);

				TestAddPlane (&plane);
			}
}
	}
}
#endif


/*
============
ExpandBrush
=============
*/
void ExpandBrush (int hullindex)
{
	int	i, x, s;
	vec3_t	corner;
	face_t	*f;
	plane_t	plane, *p;

	num_hull_points = 0;
	num_hull_edges = 0;

// create all the hull points

	for (f=brush_faces ; f ; f=f->next)
		for (i=0 ; i<f->numpoints ; i++)
			AddHullPoint (f->pts[i], hullindex);

// expand all of the planes
	for (i=0 ; i<numbrushfaces ; i++)
	{
		p = &faces[i].plane;

		VectorCopy (tx_vec3_origin, corner);
		for (x=0 ; x<3 ; x++)
		{
			if (p->normal[x] > 0)
				corner[x] = ExpandSize (hullindex, 1, x);
			else if (p->normal[x] < 0)
				corner[x] = ExpandSize (hullindex, 0, x);
		}
		p->dist += DotProduct (corner, p->normal);
	}

// add any axis planes not contained in the brush to bevel off corners
	for (x=0 ; x<3 ; x++)
		for (s=-1 ; s<=1 ; s+=2)
		{
		// add the plane
			VectorCopy (tx_vec3_origin, plane.normal);
			plane.normal[x] = s;
			if (s == -1)
				plane.dist = -brush_mins[x] - ExpandSize (hullindex, 0, x);
			else
				plane.dist = brush_maxs[x] + ExpandSize (hullindex, 1, x);
			AddBrushPlane (&plane, hullnum);
		}

// add all of the edge bevels
	for (f=brush_faces ; f ; f=f->next)
		for (i=0 ; i<f->numpoints ; i++)
			AddHullEdge (f->pts[i], f->pts[(i+1)%f->numpoints], hullindex);
}

#if 1 // Rotating brush support
void AddPointToBounds (vec3_t v, vec3_t mins, vec3_t maxs)
{
	int		i;

	for (i = 0; i < 3; i++)
	{
		if (v[i] < mins[i])
			mins[i] = v[i];

		if (v[i] > maxs[i])
			maxs[i] = v[i];
	}
}


void ExpandBrush2 (int hullnum)
{
	int			i, x, s;
	vec3_t		corner;
	winding_t	*w;
	plane_t		plane;

	int				j, k, numwindings;
	vec_t			r;
	winding_t		**windings;
	plane_t			clipplane, faceplane;
	mface_t			*mf;
	vec3_t			point;
	vec3_t		mins, maxs;

	if (!numbrushfaces)
		return;

	num_hull_points = 0;
	num_hull_edges = 0;

	mins[0] = mins[1] = mins[2] = 999999;
	maxs[0] = maxs[1] = maxs[2] = -999999;

	// generate windings and bounds data
	numwindings = 0;
	windings = calloc (numbrushfaces, sizeof (*windings));

	for (i = 0; i < numbrushfaces; i++)
	{
		mf = &faces[i];
		windings[i] = NULL;

		faceplane = mf->plane;
		w = BaseWindingForPlane (&faceplane);

		for (j = 0; j < numbrushfaces && w; j++)
		{
			clipplane = faces[j].plane;

			if (j == i)
				continue;

			// flip the plane, because we want to keep the back side
			VectorNegate (clipplane.normal, clipplane.normal);
			clipplane.dist *= -1;

			w = ClipWindingEpsilon (w, &clipplane, ON_EPSILON, true);
		}

		if (!w)
			continue;	// overcontrained plane

		for (j = 0; j < w->numpoints; j++)
		{
			for (k = 0; k < 3; k++)
			{
				point[k] = w->points[j][k];
				r = tx_Q_rint (point[k]);

				if (fabs (point[k] - r) < ZERO_EPSILON)
					w->points[j][k] = r;
				else
					w->points[j][k] = point[k];

				// check for incomplete brushes
				if (w->points[j][k] >= BOGUS_RANGE || w->points[j][k] <= -BOGUS_RANGE)
					return;
			}

			AddPointToBounds (w->points[j], mins, maxs);
		}

		windings[i] = w;
	}

	// add all of the corner offsets
	for (i = 0; i < numwindings; i++)
	{
		w = windings[i];

		for (j = 0; j < w->numpoints; j++)
			AddHullPoint2 (w->points[j], hullnum);
	}

	// expand the face planes
	for (i = 0; i < numbrushfaces; i++)
	{
		mf = &faces[i];

		for (x = 0; x < 3; x++)
		{
			if (mf->plane.normal[x] > 0)
				corner[x] = -ExpandSize (hullnum, 0, x);
			else if (mf->plane.normal[x] < 0)
				corner[x] = -ExpandSize (hullnum, 1, x);
		}

		mf->plane.dist += DotProduct (corner, mf->plane.normal);
	}

	// add any axis planes not contained in the brush to bevel off corners
	for (x = 0; x < 3; x++)
		for (s = -1; s <= 1; s += 2)
		{
			// add the plane
			plane.normal[0] = plane.normal[1] = plane.normal[2] = 0;
			plane.normal[x] = s;

			if (s == -1)
				plane.dist = -mins[x] + ExpandSize (hullnum, 1, x);
			else
				plane.dist = maxs[x] + -ExpandSize (hullnum, 0, x);

			AddBrushPlane (&plane, true);
		}

	// add all of the edge bevels
	for (i = 0; i < numwindings; i++)
	{
		w = windings[i];

		for (j = 0; j < w->numpoints; j++)
			AddHullEdge2 (w->points[j], w->points[(j+1) %w->numpoints], hullnum);
	}

	// free the windings as we no longer need them
	for (i = 0; i < numwindings; i++)
		if (windings[i])
			FreeWinding (windings[i]);

	free (windings);
}
#endif

//============================================================================


/*
===============
LoadBrush

Converts a mapbrush to a bsp brush
===============
*/
brush_t *LoadBrush (mbrush_t *mb, int hullnum)
{
	brush_t		*b;
	int		contents, NoOfTex = 0, I, TexNo[3], MipTex1, MipTex2, Hull;
	char		*name, Str[512];
	mface_t		*f;

	CurrLine = mb->Line;

//
// check texture name for attributes
//
	name = miptex[MipTex1 = texinfo[mb->faces->texinfo].miptex];

	if (!Q_strcasecmp(name, "clip") && hullnum == 0)
		return NULL;		// "clip" brushes don't show up in the draw hull

	if (name[0] == '*' && worldmodel)		// entities never use water merging
	{
		if (!Q_strncasecmp(name+1,"lava",4))
			contents = CONTENTS_LAVA;
		else if (!Q_strncasecmp(name+1,"slime",5))
			contents = CONTENTS_SLIME;
		else
			contents = CONTENTS_WATER;
	}
	else if (!options.SolidMap && !Q_strncasecmp (name, "sky",3) && worldmodel && hullnum == 0)
		contents = CONTENTS_SKY;
	else
		contents = CONTENTS_SOLID;

	if (hullnum && contents != CONTENTS_SOLID && contents != CONTENTS_SKY)
		return NULL;		// water brushes don't show up in clipping hulls

// no seperate textures on clip hull

//
// create the faces
//
	brush_faces = NULL;

	numbrushfaces = 0;
	for (f=mb->faces ; f ; f=f->next)
	{
		faces[numbrushfaces] = *f;
		if (hullnum)
			faces[numbrushfaces].texinfo = 0;
		numbrushfaces++;

		if (numbrushfaces == MAX_FACES)
			Message (MSGERR, "LoadBrush: numbrushfaces == MAX_FACES (%d) on line %d", MAX_FACES, CurrLine);
	}

	CreateBrushFaces ();

	if (!brush_faces)
	{
		strcpy(Str, name);

		// Find max 3 extra unique texture names
		for (f = mb->faces; f; f = f->next)
		{
			MipTex2 = texinfo[f->texinfo].miptex;

			if (NoOfTex < 3 && MipTex2 != MipTex1)
			{
				for (I = 0; I < NoOfTex; ++I)
				{
					if (MipTex2 == TexNo[I])
						break;
				}

				if (I == NoOfTex)
					TexNo[NoOfTex++] = MipTex2;
			}
		}

		for (I = 0; I < NoOfTex; ++I)
		{
		        strcat(Str, " ");
		        strcat(Str, miptex[TexNo[I]]);
		}

		Message (MSGWARN, "Couldn't create brush on line %d with %d faces, %s", CurrLine, numbrushfaces, Str);
		return NULL;
	}

	Hull = hullnum == 0 ? options.visiblehull : hullnum;

	if (Hull || options.HullExpansion[1] > 0)
	{
#if 1 // Rotating brush support
		if (!options.hiprotate && !strncmp (ValueForKey (CurrEnt, "classname"), "rotate_", 7))
		{
			ExpandBrush2 (Hull);
			FreeBrushFaces(brush_faces);
			CreateBrushFaces2 ();
		}
		else
		{
#endif
		ExpandBrush (Hull);
		FreeBrushFaces(brush_faces);
		CreateBrushFaces ();
#if 1 // Rotating brush support
		}
#endif
	}

//
// create the brush
//
	b = AllocBrush ();

	b->contents = contents;
	b->faces = brush_faces;
	VectorCopy (brush_mins, b->mins);
	VectorCopy (brush_maxs, b->maxs);

	return b;
}

//=============================================================================

/*
============
Brush_LoadEntity
============
*/
brushset_t *Brush_LoadEntity (entity_t *ent, int hullnum)
{
	brush_t		*b, *next, *water, *other;
	mbrush_t	*mbr;
	int		numbrushes, TotalBrushes = 0;
	brushset_t	*bset;

	CurrEnt = ent;

	bset = AllocOther (sizeof(brushset_t));
	ClearBounds (bset);

	numbrushes = 0;
	other = water = NULL;

	Message (MSGVERBOSE, "------ Brush_LoadEntity ------");

	for (mbr = ent->brushes; mbr; mbr = mbr->next)
		++TotalBrushes;

	for (mbr = ent->brushes ; mbr ; mbr=mbr->next)
	{
		b = LoadBrush (mbr, hullnum);
		if (!b)
			continue;

		numbrushes++;

		if (b->contents != CONTENTS_SOLID)
		{
			b->next = water;
			water = b;
		}
		else
		{
			b->next = other;
			other = b;
		}

		AddToBounds (bset, b->mins);
		AddToBounds (bset, b->maxs);

		ShowBar(numbrushes, TotalBrushes);
	}

	ShowBar(-1, -1);

// add all of the water textures at the start
	for (b=water ; b ; b=next)
	{
		next = b->next;
		b->next = other;
		other = b;
	}

	bset->brushes = other;

	brushset = bset;

	Message (MSGVERBOSE, "%6i brushes read",numbrushes);

	return bset;
}


