// csg4.c

#include "bsp5.h"

/*

NOTES
-----
Brushes that touch still need to be split at the cut point to make a tjunction

*/


face_t	*inside, *outside;
int		brushfaces;
int		csgfaces;
int		csgmergefaces;

void	  CalcSides (winding_t *in, plane_t *split, int *sides, vec_t *dists, int counts[3], vec_t epsilon);
void	  ClipInsideNoSplit (brush_t *b);
void	  WindingSplit (winding_t *in, plane_t *plane, plane_t *split, winding_t **pfront, winding_t **pback);
winding_t *WindingClip (winding_t *in, plane_t *plane, plane_t *split, qboolean keepon, int side);
winding_t *WindingCopy (face_t *f);

void PushToPlaneAxis (vec_t *point, plane_t *plane)
{
	int t = plane->type % 3;

	point[t] = (plane->dist - plane->normal[(t + 1) % 3] * point[(t + 1) % 3] -
				  plane->normal[(t + 2) % 3] * point[(t + 2) % 3]) / plane->normal[t];
}

/*
==================
WindingCopy
==================
*/
winding_t *WindingCopy (face_t *f)
{
	winding_t *w;

	w = NewWinding (f->numpoints);
	memcpy (w->points, f->pts, sizeof(vec3_t) * w->numpoints);

	return w;
}

/*
==================
WindingResize
==================
*/
winding_t *WindingResize (winding_t *w, int numpoints)
{
	winding_t *neww;

	neww = NewWinding (numpoints);
	memcpy (neww->points, w->points, sizeof(vec3_t) * neww->numpoints);

	FreeWinding (w);

	return neww;
} 

/*
===========================
Helper for for the clipping functions
  (WindingClip, WindingSplit)
===========================
*/
void CalcSides (winding_t *in, plane_t *split, int *sides, vec_t *dists, int counts[3], vec_t epsilon)
{
	int   i;
	vec_t *p;

	counts[0] = counts[1] = counts[2] = 0;

	if (PlaneIsAxial (split))
	{
		p = in->points[0] + split->type;

		for (i = 0; i < in->numpoints; ++i, p += 3)
		{
			vec_t dot = *p - split->dist;
			dists[i] = dot;
			
			if (dot > epsilon)
				sides[i] = SIDE_FRONT;
			else if (dot < -epsilon)
				sides[i] = SIDE_BACK;
			else
				sides[i] = SIDE_ON;

			counts[sides[i]]++;
		}
	}
	else
	{
		p = in->points[0];

		for (i = 0; i < in->numpoints; ++i, p += 3)
		{
			vec_t dot = DotProduct (split->normal, p) - split->dist;
			dists[i] = dot;

			if (dot > epsilon)
				sides[i] = SIDE_FRONT;
			else if (dot < -epsilon)
				sides[i] = SIDE_BACK;
			else
				sides[i] = SIDE_ON;

			counts[sides[i]]++;
		}
	}

	sides[i] = sides[0];
	dists[i] = dists[0];
}
 
/*
==================
WindingClip

Clips the winding to the plane, returning the new winding on 'side'.
Frees the input winding.
If keepon is true, an exactly on-plane winding will be saved, otherwise
  it will be clipped away.
==================
*/
winding_t *WindingClip (winding_t *in, plane_t *plane, plane_t *split, qboolean keepon, int side)
{
	vec_t     dists[MAX_POINTS_ON_WINDING + 1];
	int       sides[MAX_POINTS_ON_WINDING + 1];
	int       counts[3];
	vec_t     dot;
	int       i, j;
	winding_t *neww;
	vec_t     *p1, *p2, *mid;
	vec_t     epsilon = ON_EPSILON;
	int       maxpts;

	CalcSides (in, split, sides, dists, counts, epsilon);

	if (keepon && !counts[SIDE_FRONT] && !counts[SIDE_BACK])
		return in;

	if (!counts[side])
	{
		FreeWinding (in);
		return NULL;
	}

	if (!counts[side ^ 1])
		return in;

	maxpts = in->numpoints + 4;

	neww = NewWinding (maxpts);
	neww->numpoints = 0;

	// FIXME: watch numpoints <= MAX_POINTS_ON_WINDING ?
	for (i = 0; i < in->numpoints; i++)
	{
		p1 = in->points[i];

		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			++neww->numpoints;
			continue;
		}

		if (sides[i] == side)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			++neww->numpoints;
		}

		if (sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i])
			continue;

		// generate a split point
		p2 = in->points[(i + 1) % in->numpoints];
		mid = neww->points[neww->numpoints++];

		dot = dists[i] / (dists[i] - dists[i + 1]);

		for (j = 0; j < 3; j++)
		{
			// avoid round off error when possible
			if (plane->normal[j] == 1.0)
			    mid[j] = plane->dist;
			else if (plane->normal[j] == -1.0)
			    mid[j] = -plane->dist;
			else if (split->normal[j] == 1.0)
			    mid[j] = split->dist;
			else if (split->normal[j] == -1.0)
			    mid[j] = -split->dist;
			else
			    mid[j] = p1[j] + dot * (p2[j] - p1[j]);
		}

		if (!PlaneIsAxial (plane))
			PushToPlaneAxis (mid, plane);
	}

	if (neww->numpoints > maxpts)
		Message (MSGERR, "WindingClip: Points exceeded estimate");

	// free the original winding
	FreeWinding (in);

	// Shrink the winding back to just what it needs ...
	neww = WindingResize (neww, neww->numpoints);

	return neww;
}

/*
==================
WindingSplit

Splits a winding by a plane, producing one or two windings.  The
original winding is not damaged or freed.  If only on one side, the
returned winding will be the input winding.  If on both sides, two
new windings will be created.
==================
*/
void WindingSplit(winding_t *in, plane_t *plane, plane_t *split, winding_t **pfront, winding_t **pback)
{
	vec_t     dists[MAX_POINTS_ON_WINDING + 1];
	int       sides[MAX_POINTS_ON_WINDING + 1];
	int       counts[3];
	vec_t     dot;
	int       i, j;
	winding_t *front, *back;
	vec_t     *p1, *p2, *mid;
	int       maxpts;

	CalcSides (in, split, sides, dists, counts, ON_EPSILON);

	if (!counts[0])
	{
		*pfront = NULL;
		*pback = in;
		return;
	}
	
	if (!counts[1])
	{
		*pfront = in;
		*pback = NULL;
		return;
	}

	maxpts = in->numpoints + 4;
	front = NewWinding (maxpts);
	back = NewWinding (maxpts);
	front->numpoints = back->numpoints = 0;

	for (i = 0; i < in->numpoints; i++)
	{
		p1 = in->points[i];

		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, front->points[front->numpoints]);
			++front->numpoints;
			VectorCopy (p1, back->points[back->numpoints]);
			++back->numpoints;
			continue;
		}

		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, front->points[front->numpoints]);
			++front->numpoints;
		}
		else if (sides[i] == SIDE_BACK)
		{
			VectorCopy (p1, back->points[back->numpoints]);
			++back->numpoints;
		}

		if (sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i])
			continue;

		// generate a split point
		p2 = in->points[(i + 1) % in->numpoints];
		mid = front->points[front->numpoints++];

		dot = dists[i] / (dists[i] - dists[i + 1]);
		
		for (j = 0; j < 3; j++)
		{
			// avoid round off error when possible
			if (plane->normal[j] == 1.0)
				mid[j] = plane->dist;
			else if (plane->normal[j] == -1.0)
				mid[j] = -plane->dist;
			else if (split->normal[j] == 1.0)
				mid[j] = split->dist;
			else if (split->normal[j] == -1.0)
				mid[j] = -split->dist;
			else
				mid[j] = p1[j] + dot * (p2[j] - p1[j]);
		}

		if (!PlaneIsAxial (plane))
			PushToPlaneAxis (mid, plane);

		VectorCopy(mid, back->points[back->numpoints]);
		++back->numpoints;
	}

	if (front->numpoints > maxpts || back->numpoints > maxpts)
		Message (MSGERR, "WindingSplit: Points exceeded estimate");

	// Shrink the windings back to just what they need ...
	*pfront = WindingResize (front, front->numpoints);
	*pback = WindingResize (back, back->numpoints);
}
 
/*
==================
ClipInsideNoSplit
==================
*/
void ClipInsideNoSplit(brush_t *b)
{
	face_t	  *f, *bf, *next;
	face_t	  *insidelist;
	winding_t *w;
	plane_t	  *plane, *clip;

	insidelist = NULL;
	f = inside;

	while (f)
	{
		next = f->next;

		plane = &planes[f->planenum];
		w = WindingCopy (f);

		for (bf = b->faces; bf; bf = bf->next)
		{
			clip = &planes[bf->planenum];
			w = WindingClip (w, plane, clip, true, bf->planeside ^ 1);
			
			if (!w)
				break;
		}

		if (!w)
		{
			f->next = outside;
			outside = f;
		}
		else
		{
			f->next = insidelist;
			insidelist = f;
			FreeWinding (w);
		}

		f = next;
	}
	
	inside = insidelist;
}

/*
==================
NewFaceFromFace

Duplicates the non point information of a face, used by SplitFace and
MergeFace.
==================
*/
face_t *NewFaceFromFace (face_t *in)
{
	face_t	*newf;
	
	newf = AllocFace ();

	newf->planenum = in->planenum;
	newf->texturenum = in->texturenum;	
	newf->planeside = in->planeside;
	newf->original = in->original;
	newf->contents[0] = in->contents[0];
	newf->contents[1] = in->contents[1];
	
	return newf;
}

/*
==================
SplitFace

==================
*/
void SplitFace (face_t *in, plane_t *split, face_t **front, face_t **back)
{
	vec_t	dists[MAXEDGES+1];
	int	sides[MAXEDGES+1];
	int	counts[3];
	vec_t	dot;
	int	i, j, numpoints;
	face_t	*newf, *new2;
	vec_t	*p1, *p2;
	vec3_t	mid;
	
	if (in->numpoints < 0)
		Message (MSGERR, "SplitFace: freed face");
        if (in->numpoints > MAXEDGES)
                Message (MSGERR, "in->numpoints (%d) > MAXEDGES (%d)", in->numpoints, MAXEDGES);
	counts[0] = counts[1] = counts[2] = 0;

// determine sides for each point
	for (i=0 ; i<in->numpoints ; i++)
	{
		dot = DotProduct (in->pts[i], split->normal);
		dot -= split->dist;
		dists[i] = dot;
		if (dot > ON_EPSILON)
			sides[i] = SIDE_FRONT;
		else if (dot < -ON_EPSILON)
			sides[i] = SIDE_BACK;
		else
			sides[i] = SIDE_ON;
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];
	
	if (!counts[0])
	{
		*front = NULL;
		*back = in;
		return;
	}
	if (!counts[1])
	{
		*front = in;
		*back = NULL;
		return;
	}
	
	*back = newf = NewFaceFromFace (in);
	*front = new2 = NewFaceFromFace (in);
	ResizeFace (newf, MAXEDGES);
	ResizeFace (new2, MAXEDGES);
	newf->numpoints = new2->numpoints = 0;
	
// distribute the points and generate splits

	for (i=0 ; i<in->numpoints ; i++)
	{
		if (newf->numpoints >= MAXEDGES || new2->numpoints >= MAXEDGES)
		{
			numpoints = newf->numpoints;

			if (new2->numpoints > numpoints)
				numpoints = new2->numpoints;

			Message (MSGERR, "SplitFace: numpoints (%d) > MAXEDGES (%d)", numpoints, MAXEDGES);
		}

		p1 = in->pts[i];
		
		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, newf->pts[newf->numpoints]);
			++newf->numpoints;
			VectorCopy (p1, new2->pts[new2->numpoints]);
			++new2->numpoints;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, new2->pts[new2->numpoints]);
			++new2->numpoints;
		}
		else
		{
			VectorCopy (p1, newf->pts[newf->numpoints]);
			++newf->numpoints;
		}
		
		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
	// generate a split point
		p2 = in->pts[(i+1)%in->numpoints];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{	// avoid round off error when possible
			if (split->normal[j] == 1)
				mid[j] = split->dist;
			else if (split->normal[j] == -1)
				mid[j] = -split->dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}
	
		VectorCopy (mid, newf->pts[newf->numpoints]);
		++newf->numpoints;
		VectorCopy (mid, new2->pts[new2->numpoints]);
		++new2->numpoints;
	}

	if (newf->numpoints > MAXEDGES || new2->numpoints > MAXEDGES)
	{
		numpoints = newf->numpoints;

		if (new2->numpoints > numpoints)
			numpoints = new2->numpoints;

		Message (MSGERR, "SplitFace: numpoints (%d) > MAXEDGES (%d)", numpoints, MAXEDGES);
	}

// free the original face now that is represented by the fragments
	FreeFace (in); 

	// Shrink the faces back to just what they need ...
	ResizeFace (newf, newf->numpoints);
	ResizeFace (new2, new2->numpoints);
}

/*
=================
ClipInside

Clips all of the faces in the inside list, possibly moving them to the
outside list or spliting it into a piece in each list.

Faces exactly on the plane will stay inside unless overdrawn by later brush

frontside is the side of the plane that holds the outside list
=================
*/
void ClipInside (int splitplane, int frontside, qboolean precedence)
{
	face_t	*f, *next;
	face_t	*frags[2];
	face_t	*insidelist;
	plane_t *split;

	split = &planes[splitplane];

	insidelist = NULL;
	for (f=inside ; f ; f=next)
	{
		next = f->next;

		if (f->planenum == splitplane)
		{	// exactly on, handle special
			if ( frontside != f->planeside || precedence )
			{	// allways clip off opposite faceing
				frags[frontside] = NULL;
				frags[!frontside] = f;
			}
			else
			{	// leave it on the outside
				frags[frontside] = f;
				frags[!frontside] = NULL;
			}
		}
		else
		{	// proper split
			SplitFace (f, split, &frags[0], &frags[1]);
		}

		if (frags[frontside])
		{
			frags[frontside]->next = outside;
			outside = frags[frontside];
		}
		if (frags[!frontside])
		{
			frags[!frontside]->next = insidelist;
			insidelist = frags[!frontside];
		}
	}

	inside = insidelist;
}

/*
==================
SaveOutside

Saves all of the faces in the outside list to the bsp plane list
==================
*/
void SaveOutside (qboolean mirror)
{
	face_t 	*f, *next, *newf;
	int	i;
	int	planenum;
		
	for (f=outside ; f ; f=next)
	{
		next = f->next;
		csgfaces++;

		planenum = f->planenum;
		
		if (mirror)
		{
			newf = NewFaceFromFace (f);
			
			ResizeFace (newf, f->numpoints);
			newf->planeside = f->planeside ^ 1;	// reverse side
			newf->contents[0] = f->contents[1];
			newf->contents[1] = f->contents[0];
	
			for (i=0 ; i<f->numpoints ; i++)	// add points backwards
			{
				VectorCopy (f->pts[f->numpoints-1-i], newf->pts[i]);
			}
		}
		else
			newf = NULL;

		validfaces[planenum] = MergeFaceToList(f, validfaces[planenum]);
		if (newf)
			validfaces[planenum] = MergeFaceToList(newf, validfaces[planenum]);

		validfaces[planenum] = FreeMergeListScraps (validfaces[planenum]);
	}
}

/*
==================
FreeInside

Free all the faces that got clipped out
==================
*/
void FreeInside (int contents)
{
	face_t	*f, *next;
	
	for (f=inside ; f ; f=next)
	{
		next = f->next;
		
		if (contents != CONTENTS_SOLID)
		{
			f->contents[0] = contents;
			f->next = outside;
			outside = f;
		}
		else
			FreeFace (f);
	}
}

//==========================================================================

/*
==================
BuildSurfaces

Returns a chain of all the external surfaces with one or more visible
faces.
==================
*/
surface_t *BuildSurfaces (void)
{
	face_t			**f;
	face_t			*count;
	int				i;
	surface_t		*s;
	surface_t		*surfhead;
	
	surfhead = NULL;
	
	f = validfaces;
	for (i=0 ; i<numbrushplanes ; i++, f++)
	{
		if (!*f)
			continue;	// nothing left on this plane

// create a new surface to hold the faces on this plane
		s = AllocSurface ();
		s->planenum = i;
		s->next = surfhead;
		surfhead = s;
		s->faces = *f;
		for (count = s->faces ; count ; count=count->next)
			csgmergefaces++;
		CalcSurfaceInfo (s);	// bounding box and flags
	}

	ClearArray(validfaces);
        validfacesactive = 0;
	return surfhead;
}

//==========================================================================

/*
==================
CopyFacesToOutside
==================
*/
void CopyFacesToOutside (brush_t *b)
{
	face_t		*f, *newf;
	
	outside = NULL;
	
	for (f=b->faces ; f ; f=f->next)
	{
		brushfaces++;
		newf = AllocFace ();
		CopyFace (newf, f);
		newf->next = outside;
		newf->contents[0] = CONTENTS_EMPTY;
		newf->contents[1] = b->contents;
		outside = newf;
	}
}

/*
============
BoxPlaneSide
============
*/
int BoxPlaneSide(vec3_t mins, vec3_t maxs, plane_t *split)
{
	int    i;
	vec3_t corners[2];
	vec_t  dist1, dist2;

	if (PlaneIsAxial(split))
	{
		if (maxs[split->type] < split->dist - ON_EPSILON)
			return SIDE_BACK;
		else if (mins[split->type] > split->dist + ON_EPSILON)
			return SIDE_FRONT;
		else
			return SIDE_ON;
	}
	else
	{
		// create the proper leading and trailing verts for the box
		for (i=0 ; i<3 ; i++)
		{
			 if (split->normal[i] < 0)
			 {
		 		corners[0][i] = mins[i];
		 		corners[1][i] = maxs[i];
			 }
			 else
			 {
		 		corners[1][i] = mins[i];
		 		corners[0][i] = maxs[i];
			 }
		}

		dist1 = DotProduct (split->normal, corners[0]) - split->dist;
		dist2 = DotProduct (split->normal, corners[1]) - split->dist;

		if (dist1 > ON_EPSILON && dist2 > ON_EPSILON)
			return SIDE_FRONT;
		if (dist1 < -ON_EPSILON && dist2 < -ON_EPSILON)
			return SIDE_BACK;

		return SIDE_ON;
	}
}

/*
==================
CSGFaces

Returns a list of surfaces containing aall of the faces
==================
*/
surface_t *CSGFaces (brushset_t *bs)
{
	brush_t	  *b1, *b2;
	int	  i, BrushNo = 0, TotalBrushes = 0;
	qboolean  overwrite;
	face_t	  *f;
	surface_t *surfhead;

	Message (MSGVERBOSE, "------ CSGFaces ------");

	ClearArray(validfaces);
        validfacesactive = 1;
        ExtendArray(validfaces, numbrushplanes-1);

	csgfaces = brushfaces = csgmergefaces = 0;
//
// do the solid faces
//
	for (b1 = bs->brushes; b1; b1 = b1->next)
		++TotalBrushes;

	for (b1=bs->brushes ; b1 ; b1 = b1->next)
	{
	// set outside to a copy of the brush's faces
		CopyFacesToOutside (b1);

		overwrite = false;

		for (b2=bs->brushes ; b2 ; b2 = b2->next)
		{
		// check bounding box first
			for (i=0 ; i<3 ; i++)
			{
				if (b1->mins[i] > b2->maxs[i] + ON_EPSILON ||
				    b1->maxs[i] < b2->mins[i] - ON_EPSILON)
					break;
			}

			if (i<3)
				continue;

			// if b2 is completely inside b1, nothing to clip away
			// only do thorough test if bounds test ok
			for (i = 0; i < 3; i++)
			{
		 		if (b2->mins[i] < b1->mins[i] + ON_EPSILON ||
		 		    b2->maxs[i] > b1->maxs[i] - ON_EPSILON)
		 			break;
			}

			if (i == 3)
			{
		 		qboolean b2_inside_b1 = true;

		 		for (f = b1->faces; f; f = f->next)
				{
		 			if (BoxPlaneSide(b2->mins, b2->maxs, &planes[f->planenum]) != (f->planeside ^ 1))
					{
		 		 		b2_inside_b1 = false;
		 		 		break;
		 			}
		 		}
		 		
				if (b2_inside_b1)
		 			continue;
			}
			
		// see if b2 needs to clip a chunk out of b1

			if (b1==b2)
			{
				overwrite = true;	// later brushes now overwrite
				continue;
			}

		// divide faces by the planes of the new brush

			inside = outside;
			outside = NULL;

			if (!options.OldCsg)
				ClipInsideNoSplit (b2); // Put faces that definitely won't get split outside

			for (f=b2->faces ; f ; f=f->next)
				ClipInside (f->planenum, f->planeside, overwrite);

		// these faces are continued in another brush, so get rid of them
			if (b1->contents == CONTENTS_SOLID && b2->contents <= CONTENTS_WATER)
				FreeInside (b2->contents);
			else
				FreeInside (CONTENTS_SOLID);
		}

	// all of the faces left in outside are real surface faces
		if (b1->contents != CONTENTS_SOLID)
			SaveOutside (true);	// mirror faces for inside view
		else
			SaveOutside (false);

		ShowBar(++BrushNo, TotalBrushes);
	}

	ShowBar(-1, -1);

	surfhead = BuildSurfaces ();
	
	Message (MSGVERBOSE, "%6i brushfaces", brushfaces);
	Message (MSGVERBOSE, "%6i csgfaces", csgfaces);
	Message (MSGVERBOSE, "%6i mergedfaces", csgmergefaces);

	return surfhead;
}


