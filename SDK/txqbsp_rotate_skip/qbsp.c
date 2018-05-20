// bsp5.c

#include "bsp5.h"
#include "sys\types.h"
#include "sys\stat.h"

void SecToStr(time_t, char []);
void PrintFinish (void);
void *Qmalloc(int Size, char Caller[]);
void Qfree(void *Ptr, int Size);

//
// command line flags
//
options_t options;

typedef struct
{
	int	    nummodels;
	int	    numclipnodes;
	dclipnode_t *dclipnodes;
	int	    numplanes;
	dplane_t    *dplanes;
} hullinfo_t;

hullinfo_t	HullInfo[3];

brushset_t	*brushset;

int		valid;

char		bspfilename[1024];
char		pointfilename[1024];
char		portfilename[1024];
char		logfilename[] = "QBSP_LOG.TXT";


char		*argv0;	// changed after fork();

qboolean	worldmodel;

int		hullnum, TotalWarnings = 0;
int		HullOrder[] = {0, 1, 2};
double		start = 0, end;

FILE		*logfile;

//===========================================================================

/*
=======
Message
=======
*/
void Message (int MsgType, ...)
{
	va_list  argptr;
	char	 *Fmt, Warning[] = "WARNING: ", Buf[512];
	qboolean DisableScreen, IsWarning;

	va_start (argptr, MsgType);

	if (MsgType == MSGVERBOSE && !options.verbosefile)
		return;

	Fmt = va_arg (argptr, char *);

	IsWarning = MsgType == MSGWARN || MsgType == MSGWARNCRIT;

	if (IsWarning)
		++TotalWarnings;

	DisableScreen = MsgType == MSGVERBOSE && (!options.verbose || options.noverbose) ||
			MsgType == MSGNOVERBOSE && options.noverbose ||
		        MsgType == MSGWARN && options.nowarnings ||
			MsgType == MSGFILE;

	if (MsgType == MSGERR)
		Message (MSGALWAYS, "************ ERROR ************");

        vsprintf (Buf, Fmt, argptr);

	if (!DisableScreen)
	{
		ShowBar (0, -1);

		if (IsWarning)
			printf (Warning);

        	printf ("%s", Buf);

		if (MsgType != MSGALWAYSEXACT)
			printf ("\n");

	        fflush (stdout);
        }

	if (IsWarning)
		SafePrintf (logfile, logfilename, Warning);

	SafePrintf (logfile, logfilename, "%s", Buf);

	if (MsgType != MSGALWAYSEXACT)
		SafePrintf (logfile, logfilename, "\n");

	SafeFlush (logfile);

	va_end (argptr);

	if (MsgType == MSGERR)
	{
		PrintFinish ();
#ifndef QUAKE_GAME
		exit (1);
#else
#pragma message ("Should we do a longjump or something here?")
#pragma message ("Should we do a longjump or something here?")
#pragma message ("Should we do a longjump or something here?")
#pragma message ("Should we do a longjump or something here?")
#pragma message ("Should we do a longjump or something here?")
#pragma message ("Should we do a longjump or something here?")
#pragma message ("Should we do a longjump or something here?")
#pragma message ("Because we cannot do exit")
#endif
	}
}

/*
===============
GetCoord

Returns coordinates as a string
===============
*/
char *GetCoord(vec3_t v)
{
	static char Str[50];

	sprintf (Str, "(%.0f %.0f %.0f)", v[0], v[1], v[2]);

	return Str;
}

/*
=================
BaseWindingForPlane
=================
*/
winding_t *BaseWindingForPlane (plane_t *p)
{
	int		i, x;
	vec_t	max, v;
	vec3_t	org, vright, vup;
	winding_t	*w;

// find the major axis

	max = -BOGUS_RANGE;
	x = -1;
	for (i=0 ; i<3; i++)
	{
		v = fabs(p->normal[i]);
		if (v > max)
		{
			x = i;
			max = v;
		}
	}
	if (x==-1)
		Message (MSGERR, "BaseWindingForPlane: No axis found");

	VectorCopy (tx_vec3_origin, vup);
	switch (x)
	{
	case 0:
	case 1:
		vup[2] = 1;
		break;
	case 2:
		vup[0] = 1;
		break;
	}

	v = DotProduct (vup, p->normal);
	tx_VectorMA (vup, -v, p->normal, vup);
	tx_VectorNormalize (vup);

	tx_VectorScale (p->normal, p->dist, org);

	tx_CrossProduct (vup, p->normal, vright);

	tx_VectorScale (vup, 8192, vup);
	tx_VectorScale (vright, 8192, vright);

// project a really big	axis aligned box onto the plane
	w = NewWinding (4);

	VectorSubtract (org, vright, w->points[0]);
	VectorAdd (w->points[0], vup, w->points[0]);

	VectorAdd (org, vright, w->points[1]);
	VectorAdd (w->points[1], vup, w->points[1]);

	VectorAdd (org, vright, w->points[2]);
	VectorSubtract (w->points[2], vup, w->points[2]);

	VectorSubtract (org, vright, w->points[3]);
	VectorSubtract (w->points[3], vup, w->points[3]);

	return w;
}

#if 1 // Rotating brush support
winding_t *AllocWinding (int points)
{
	winding_t	*w;
	size_t		size;

	if (points > MAX_POINTS_ON_WINDING)
		Message (MSGERR, "AllocWinding: %i points", points);

	size = (size_t) ((winding_t *) 0)->points[points];
	w = malloc (size);
	memset (w, 0, size);

	return w;
}


winding_t *ClipWindingEpsilon (winding_t *in, plane_t *split, vec_t epsilon, qboolean keepon)
{
	int			i, j;
	vec_t		dists[MAX_POINTS_ON_WINDING + 1];
	int			sides[MAX_POINTS_ON_WINDING + 1];
	int			counts[3];
	vec_t		dot;
	vec_t		*p1, *p2;
	vec3_t		mid;
	winding_t	*neww;
	int			maxpts;

	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for (i = 0; i < in->numpoints; i++)
	{
		dists[i] = dot = DotProduct (in->points[i], split->normal) - split->dist;

		if (dot > epsilon)
			sides[i] = SIDE_FRONT;
		else if (dot < -epsilon)
			sides[i] = SIDE_BACK;
		else
			sides[i] = SIDE_ON;

		counts[sides[i]]++;
	}

	sides[i] = sides[0];
	dists[i] = dists[0];

	if (keepon && !counts[SIDE_FRONT] && !counts[SIDE_BACK])
		return in;

	if (!counts[SIDE_FRONT])
	{
		FreeWinding (in);
		return NULL;
	}

	if (!counts[SIDE_BACK])
		return in;

	maxpts = in->numpoints + 4;	// can't use counts[0]+2 because of fp grouping errors
	neww = AllocWinding (maxpts);

	for (i = 0; i < in->numpoints; i++)
	{
		p1 = in->points[i];

		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			neww->numpoints++;
			continue;
		}

		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			neww->numpoints++;
		}

		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		// generate a split point
		p2 = in->points[ (i+1) %in->numpoints];

		dot = dists[i] / (dists[i] - dists[i+1]);

		for (j = 0; j < 3; j++)
		{
			// avoid round off error when possible
			if (split->normal[j] == 1)
				mid[j] = split->dist;
			else if (split->normal[j] == -1)
				mid[j] = -split->dist;
			else
				mid[j] = p1[j] + dot * (p2[j] - p1[j]);
		}

		VectorCopy (mid, neww->points[neww->numpoints]);
		neww->numpoints++;
	}

	if (neww->numpoints > maxpts)
		Message (MSGERR, "ClipWinding: points exceeded estimate");

	// free the original winding
	FreeWinding2 (in);

	return neww;
}
#endif

/*
==================
ClipWinding

Clips the winding to the plane, returning the new winding on the positive side
Frees the input winding.
If keepon is true, an exactly on-plane winding will be saved, otherwise
it will be clipped away.
==================
*/
winding_t *ClipWinding (winding_t *in, plane_t *split, qboolean keepon)
{
	vec_t	  dists[MAX_POINTS_ON_WINDING];
	int	  sides[MAX_POINTS_ON_WINDING];
	int	  counts[3];
	vec_t	  dot;
	int	  i, j;
	vec_t	  *p1, *p2;
	vec3_t	  mid;
	winding_t *neww;
	int	  maxpts;

	counts[0] = counts[1] = counts[2] = 0;

// determine sides for each point
	for (i=0 ; i<in->numpoints ; i++)
	{
		dot = DotProduct (in->points[i], split->normal);
		dot -= split->dist;
		dists[i] = dot;
		if (dot > ON_EPSILON)
			sides[i] = SIDE_FRONT;
		else if (dot < -ON_EPSILON)
			sides[i] = SIDE_BACK;
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	if (keepon && !counts[0] && !counts[1])
		return in;

	if (!counts[0])
	{
		FreeWinding (in);
		return NULL;
	}
	if (!counts[1])
		return in;

	maxpts = in->numpoints+4;	// can't use counts[0]+2 because
								// of fp grouping errors
	neww = NewWinding (maxpts);
	neww->numpoints = 0;

	for (i=0 ; i<in->numpoints ; i++)
	{
		p1 = in->points[i];

		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			neww->numpoints++;
			continue;
		}

		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			neww->numpoints++;
		}

		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

	// generate a split point
		p2 = in->points[(i+1)%in->numpoints];

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

		VectorCopy (mid, neww->points[neww->numpoints]);
		neww->numpoints++;
	}

	if (neww->numpoints > maxpts)
		Message (MSGERR, "ClipWinding: Points exceeded estimate");

// free the original winding
	FreeWinding (in);

	// Shrink the winding back to just what it needs ...
	neww = WindingResize (neww, neww->numpoints);

	return neww;
}


/*
==================
DivideWinding

Divides a winding by a plane, producing one or two windings.  The
original winding is not damaged or freed.  If only on one side, the
returned winding will be the input winding.  If on both sides, two
new windings will be created.
==================
*/
void DivideWinding (winding_t *in, plane_t *split, winding_t **front, winding_t **back)
{
	vec_t	  dists[MAX_POINTS_ON_WINDING];
	int	  sides[MAX_POINTS_ON_WINDING];
	int	  counts[3];
	vec_t	  dot;
	int	  i, j;
	vec_t	  *p1, *p2;
	vec3_t	  mid;
	winding_t *f, *b;
	int	  maxpts;

	counts[0] = counts[1] = counts[2] = 0;

// determine sides for each point
	for (i=0 ; i<in->numpoints ; i++)
	{
		dot = DotProduct (in->points[i], split->normal);
		dot -= split->dist;
		dists[i] = dot;
		if (dot > ON_EPSILON)
			sides[i] = SIDE_FRONT;
		else if (dot < -ON_EPSILON)
			sides[i] = SIDE_BACK;
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	*front = *back = NULL;

	if (!counts[0])
	{
		*back = in;
		return;
	}
	if (!counts[1])
	{
		*front = in;
		return;
	}

	maxpts = in->numpoints+4;	// can't use counts[0]+2 because
								// of fp grouping errors

	*front = f = NewWinding (maxpts);
	*back = b = NewWinding (maxpts);
	f->numpoints = b->numpoints = 0;

	for (i=0 ; i<in->numpoints ; i++)
	{
		p1 = in->points[i];

		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, f->points[f->numpoints]);
			f->numpoints++;
			VectorCopy (p1, b->points[b->numpoints]);
			b->numpoints++;
			continue;
		}

		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, f->points[f->numpoints]);
			f->numpoints++;
		}
		if (sides[i] == SIDE_BACK)
		{
			VectorCopy (p1, b->points[b->numpoints]);
			b->numpoints++;
		}

		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

	// generate a split point
		p2 = in->points[(i+1)%in->numpoints];

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

		VectorCopy (mid, f->points[f->numpoints]);
		f->numpoints++;
		VectorCopy (mid, b->points[b->numpoints]);
		b->numpoints++;
	}

	if (f->numpoints > maxpts || b->numpoints > maxpts)
		Message (MSGERR, "DivideWinding: Points exceeded estimate");

	// Shrink the windings back to just what they need ...
	*front = WindingResize (f, f->numpoints);
	*back = WindingResize (b, b->numpoints);
}

/*
==================
MidpointWinding
==================
*/
void MidpointWinding(winding_t *w, vec3_t v)
{
	int i, j;

	VectorCopy(tx_vec3_origin, v);
	for (i=0; i<w->numpoints; i++)
		for (j=0; j<3; j++)
			v[j] += w->points[i][j];

	if (w->numpoints > 0)
		for (j=0; j<3; j++)
			v[j] /= w->numpoints;
}

//===========================================================================

int			c_activefaces, c_peakfaces;
int			c_activesurfaces, c_peaksurfaces;
int			c_activewindings, c_peakwindings;
int			c_activeportals, c_peakportals;
int			c_activenodes, c_peaknodes;
int			c_activebrushes, c_peakbrushes;
int			c_activeother, c_peakother;
int			c_activetotalbytes, c_peaktotalbytes;

static void SecToStr(time_t Sec, char Str[])

{
	int Hour;

	Hour = Sec / 3600;

	if (Hour == 0)
		sprintf(Str, "%d:%02d", (Sec / 60) % 60, Sec % 60);
	else
		sprintf(Str, "%d:%02d:%02d", Hour, (Sec / 60) % 60, Sec % 60);
}

void PrintFinish (void)
{
	int  c_peakfacebytes;
	int  c_peaksurfacebytes;
	int  c_peakwindingbytes;
	int  c_peakportalbytes;
	int  c_peaknodebytes;
	int  c_peakbrushbytes;
	char Str[20];

	if (start == 0)
		return;

	if (TotalWarnings > 0)
		Message (MSGALWAYS, "\n%d warning%s", TotalWarnings, TotalWarnings > 1 ? "s" : "");

	end = I_FloatTime ();

	SecToStr((time_t)(end - start + 0.5), Str);

	Message (MSGALWAYS, "\nElapsed time : %s", Str);

	c_peakfacebytes = c_peakfaces * sizeof(face_t);
	c_peaksurfacebytes = c_peaksurfaces * sizeof(surface_t);
	c_peakwindingbytes = c_peakwindings * sizeof(winding_t);
	c_peakportalbytes = c_peakportals * sizeof(portal_t);
	c_peaknodebytes = c_peaknodes * sizeof(node_t);
	c_peakbrushbytes = c_peakbrushes * sizeof(brush_t);

	if (options.allverbose || options.verbosemem)
	{
		Message (MSGFILE, "\nObject     Active      Peak   kbyte");
		Message (MSGFILE, "Faces     %7d  %8d  %6d", c_activefaces, c_peakfaces, c_peakfacebytes / 1024);
		Message (MSGFILE, "Surfaces  %7d  %8d  %6d", c_activesurfaces, c_peaksurfaces, c_peaksurfacebytes / 1024);
		Message (MSGFILE, "Windings  %7d  %8d  %6d", c_activewindings, c_peakwindings, c_peakwindingbytes / 1024);
		Message (MSGFILE, "Portals   %7d  %8d  %6d", c_activeportals, c_peakportals, c_peakportalbytes / 1024);
		Message (MSGFILE, "Nodes     %7d  %8d  %6d", c_activenodes, c_peaknodes, c_peaknodebytes / 1024);
		Message (MSGFILE, "Brushes   %7d  %8d  %6d", c_activebrushes, c_peakbrushes, c_peakbrushbytes / 1024);
		Message (MSGFILE, "Other(kb) %7d  %8d  %6d", c_activeother / 1024, c_peakother / 1024, c_peakother / 1024);
		Message (MSGFILE, "Virt data %7s  %8s  %6d", "", "", total_used_memory_peak / 1024);
	}

	Message (MSGALWAYS, "\nPeak memory used : %.1f MB", (float)c_peaktotalbytes / (1024 * 1024));

	SafeClose (logfile, NULL);
}

void AddBytes(int Size)
{
	c_activetotalbytes += Size;

	if (c_activetotalbytes > c_peaktotalbytes)
		c_peaktotalbytes = c_activetotalbytes;
}

void SubBytes(int Size)
{
	c_activetotalbytes -= Size;
}

void *Qmalloc(int Size, char Caller[])
{
	void *Ptr;

	Ptr = malloc(Size);

	if (Ptr == NULL)
		Message (MSGERR, "Memory allocation of %d bytes failed in %s", Size, Caller);

	AddBytes(Size);

	return(Ptr);
}

void Qfree(void *Ptr, int Size)
{
	SubBytes(Size);
#if 1 // Rotating brush support .. just an MH comment
	// crashes - no big deal anyway as it's just memory
#endif
	free(Ptr);
}

/*
==================
NewWinding
==================
*/
winding_t *NewWinding (int points)
{
	winding_t *w;
	int	  size;

	if (points > MAX_POINTS_ON_WINDING)
		Message (MSGERR, "NewWinding: %i points", points);

	c_activewindings++;
	if (c_activewindings > c_peakwindings)
		c_peakwindings = c_activewindings;

	size = (int)((winding_t *)0)->points[points];
	size += sizeof(int);

	w = Qmalloc (size, "NewWinding");
	memset (w, 0, size);

	// Save total size just before the user area
	*(int *)w = size;
	w = (winding_t *)((char *)w + sizeof(int));

	w->numpoints = points;

	return w;
}


void FreeWinding (winding_t *w)
{
	c_activewindings--;

	// Total size is saved just before the user area
	w = (winding_t *)((char *)w - sizeof(int));

	Qfree (w, *(int *)w);
}


#if 1 // Rotating brush support ... Baker: Err?  Ok then
void FreeWinding2 (winding_t *w)
{
	//c_activewindings--;

	// Total size is saved just before the user area
	// w = (winding_t *)((char *)w - sizeof(int));

	// this leak is intentional to avoid a crash elsewhere
	// Qfree (w, *(int *)w);
}
#endif


/*
===========
AllocFace
===========
*/
face_t *AllocFace (void)
{
	face_t	*f;

	c_activefaces++;
	if (c_activefaces > c_peakfaces)
		c_peakfaces = c_activefaces;

	f = Qmalloc (sizeof(face_t), "AllocFace");
	memset (f, 0, sizeof(face_t));
	f->planenum = -1;

	return f;
}

void FreeFace (face_t *f)
{
	ResizeFace (f, 0);

	c_activefaces--;
	Qfree (f, sizeof(face_t));
}

void ResizeFace (face_t *f, int numpoints)
{
	// Note: f->numpoints may not be correct here
	f->pts = ReAllocOther(f->pts, numpoints * sizeof(vec3_t));
	f->numpoints = numpoints;
}

void CopyFace (face_t *dst, face_t *src)
{
	vec3_t *Save;

	ResizeFace (dst, src->numpoints);

	Save = dst->pts;
	*dst = *src;
	dst->pts = Save;

	memcpy(dst->pts, src->pts, dst->numpoints * sizeof(vec3_t));
}


/*
===========
AllocSurface
===========
*/
surface_t *AllocSurface (void)
{
	surface_t	*s;

	c_activesurfaces++;
	if (c_activesurfaces > c_peaksurfaces)
		c_peaksurfaces = c_activesurfaces;

	s = Qmalloc (sizeof(surface_t), "AllocSurface");
	memset (s, 0, sizeof(surface_t));

	return s;
}

void FreeSurface (surface_t *s)
{
	c_activesurfaces--;
	Qfree (s, sizeof(surface_t));
}

/*
===========
AllocPortal
===========
*/
portal_t *AllocPortal (void)
{
	portal_t	*p;

	c_activeportals++;
	if (c_activeportals > c_peakportals)
		c_peakportals = c_activeportals;

	p = Qmalloc (sizeof(portal_t), "AllocPortal");
	memset (p, 0, sizeof(portal_t));

	return p;
}

void FreePortal (portal_t *p)
{
	c_activeportals--;
	Qfree (p, sizeof(portal_t));
}


/*
===========
AllocNode
===========
*/
node_t *AllocNode (void)
{
	node_t	*n;

	c_activenodes++;
	if (c_activenodes > c_peaknodes)
		c_peaknodes = c_activenodes;

	n = Qmalloc (sizeof(node_t), "AllocNode");
	memset (n, 0, sizeof(node_t));

	return n;
}

void FreeNode (node_t *n)
{
	c_activenodes--;
	Qfree (n, sizeof(node_t));
}

/*
===========
AllocBrush
===========
*/
brush_t *AllocBrush (void)
{
	brush_t	*b;

	c_activebrushes++;
	if (c_activebrushes > c_peakbrushes)
		c_peakbrushes = c_activebrushes;

	b = Qmalloc (sizeof(brush_t), "AllocBrush");
	memset (b, 0, sizeof(brush_t));

	return b;
}

void FreeBrush (brush_t *b)
{
	c_activebrushes--;
	Qfree (b, sizeof(brush_t));
}

/*
===========
AllocOther
===========
*/
void *AllocOther (size_t Size)
{
	void *o;

	Size += sizeof(int);

	o = Qmalloc(Size, "AllocOther");
	memset (o, 0, Size);

	// Save total size just before the user area
	*(int *)o = Size;
	o = (char *)o + sizeof(int);

	c_activeother += Size;
	if (c_activeother > c_peakother)
		c_peakother = c_activeother;

	return(o);
}

void FreeOther (void *o)
{
	// Total size is saved just before the user area
	o = (char *)o - sizeof(int);
	c_activeother -= *(int *)o;

	Qfree (o, *(int *)o);
}

void *ReAllocOther (void *o, size_t Size)
{
	void *o2;
	int  OldSize;

	o2 = Size == 0 ? NULL : AllocOther (Size);

	if (o != NULL)
	{
		if (o2 != NULL)
		{
			// Total size is saved just before the user area
			OldSize = *(int *)((char *)o - sizeof(int)) - sizeof(int);
			memcpy(o2, o, (int)Size < OldSize ? Size : OldSize);
		}

		FreeOther (o);
	}

	return(o2);
}

//===========================================================================

/*
===============
ProcessEntity
===============
*/
void ProcessEntity (int entnum)
{
	entity_t *ent;
	char	mod[80];
	surface_t	*surfs;
	node_t		*nodes;
	brushset_t	*bs;

	ent = &entities[entnum];
	if (!ent->brushes)
		return;		// non-bmodel entity

	if (entnum > 0)
	{
		worldmodel = false;
		if (entnum == 1)
			Message (MSGVERBOSE, "------ Internal Entities ------");
		sprintf (mod, "*%i", nummodels);

		if (options.verbose)
		{
			PrintEntity (ent);

			if (hullnum == 0)
				Message (MSGVERBOSE, "MODEL: %s", mod);
		}

		SetKeyValue (ent, "model", mod);
	}
	else
		worldmodel = true;


//
// take the brush_ts and clip off all overlapping and contained faces,
// leaving a perfect skin of the model with no hidden faces
//
	if (worldmodel)
		ShowBar(0, 0);

	bs = Brush_LoadEntity (ent, hullnum);

	if (!bs->brushes)
		Message (MSGERR, "Entity with no valid brushes or textures on line %d", ent->Line);

	if (worldmodel)
		ShowBar(0, 0);

	brushset = bs;
	surfs = CSGFaces (bs);

	FreeBrushsetBrushes();

	if (hullnum != 0)
	{
		nodes = SolidBSP (surfs, true, false, false);
		if (worldmodel && !options.nofill)	// assume non-world bmodels are simple
		{
			PortalizeWorld (nodes, false);
			if (FillOutside (nodes))
			{
				FreeAllPortals (nodes);
				surfs = GatherNodeFaces (nodes);
				nodes = SolidBSP (surfs, false, false, false);	// make a really good tree
			}
		}
		WriteNodePlanes (nodes);
		WriteClipNodes (nodes);
		BumpModel (hullnum);
	}
	else
	{
	//
	// SolidBSP generates a node tree
	//
	// if not the world, make a good tree first
	// the world is just going to make a bad tree
	// because the outside filling will force a regeneration later
		nodes = SolidBSP (surfs, worldmodel, false, false);

	//
	// build all the portals in the bsp tree
	// some portals are solid polygons, and some are paths to other leafs
	//
		if (worldmodel && !options.nofill) // assume non-world bmodels are simple
		{
			PortalizeWorld (nodes, false);

			if (FillOutside (nodes))
			{
				FreeAllPortals (nodes);

			// get the remaining faces together into surfaces again
				surfs = GatherNodeFaces (nodes);

			// merge polygons
				MergeAll (surfs);

				ShowBar(0, 0);

			// make a really good tree
				nodes = SolidBSP (surfs, false, !options.noverbose, true);

			// make the real portals for vis tracing
				PortalizeWorld (nodes, true);

			// save portal file for vis tracing
				WritePortalfile (nodes);

			// fix tjunctions
				tjunc (nodes);
			}
			FreeAllPortals (nodes);
		}

		WriteNodePlanes (nodes);

		if (worldmodel)
			ShowBar(0, 0);

		MakeFaceEdges (nodes);
		WriteDrawNodes (nodes);
	}
}

/*
=================
UpdateEntLump

=================
*/
void UpdateEntLump (void)
{
	int 	 Models, entnum;
	char	 mod[80];
	vec3_t	 temp;
	entity_t *Ent;

	Message (MSGALWAYS, "Updating entities lump...");

	VectorCopy(tx_vec3_origin, temp);
	Models = 1; // World model is always there
	for (entnum = 1 ; entnum < num_entities ; entnum++)
	{
		Ent = &entities[entnum];

		if (!Ent->brushes)
			continue;
		sprintf (mod, "*%i", Models);
		SetKeyValue (Ent, "model", mod);
		Models++;

		// Do extra work for rotating entities if necessary
		if (!strncmp(ValueForKey(Ent, "classname"), "rotate_", 7))
			FixRotateOrigin(Ent, temp);
	}

	LoadBSPFile (bspfilename);

	// Has # brush models changed ?
	if (Models != nummodels)
		Message (MSGWARNCRIT, "UpdateEntLump: Bsp has %i models, map has %i", nummodels, Models);

	WriteEntitiesToString();
	WriteBSPFile (bspfilename);
}

/*
=================
WriteClipHull

Write the clipping hull out to a text file so the parent process can get it
=================
*/
void WriteClipHull (void)
{
	hullinfo_t *Hull;

	Hull = &HullInfo[hullnum];

	Hull->nummodels = nummodels;

	Hull->numclipnodes = numclipnodes;
	Hull->dclipnodes = AllocOther(numclipnodes * sizeof(dclipnode_t));
	memcpy(Hull->dclipnodes, dclipnodes, numclipnodes * sizeof(dclipnode_t));

	Hull->numplanes = numplanes;
	Hull->dplanes = AllocOther(numplanes * sizeof(dplane_t));
	memcpy(Hull->dplanes, dplanes, numplanes * sizeof(dplane_t));
}

/*
=================
ReadClipHull

Read the files written out by the child processes
=================
*/
void ReadClipHull (int hullnum)
{
	int	    i, j;
	int	    firstclipnode;
	dplane_t    p;
	dclipnode_t *d;
	hullinfo_t  *Hull;
	vec3_t	    norm;

	Hull = &HullInfo[hullnum];

	if (hullnum == 0)
	{
		// Just restore data
		nummodels = Hull->nummodels;
		numclipnodes = Hull->numclipnodes;
		numplanes = Hull->numplanes;
		memcpy(dclipnodes, Hull->dclipnodes, numclipnodes * sizeof(dclipnode_t));
		memcpy(dplanes, Hull->dplanes, numplanes * sizeof(dplane_t));
	}
	else
	{
		ExtendArray(dclipnodes, numclipnodes + Hull->numclipnodes);
		memcpy(&dclipnodes[numclipnodes], Hull->dclipnodes, Hull->numclipnodes * sizeof(dclipnode_t));

		if (Hull->nummodels != nummodels)
			Message (MSGERR, "ReadClipHull: Hull had %i models, base had %i", Hull->nummodels, nummodels);

		if (numclipnodes != 0)
		{
			for (i = 0; i < nummodels; i++)
				dmodels[i].headnode[hullnum] += numclipnodes;
		}

		firstclipnode = numclipnodes;

		for (i = 0; i < Hull->numclipnodes; i++)
		{
			d = &dclipnodes[numclipnodes];
			numclipnodes++;

			p = Hull->dplanes[Hull->dclipnodes[i].planenum];

			for (j = 0; j < 3; ++j)
				norm[j] = p.normal[j];

			p.type = PlaneTypeForNormal (norm); // vec_t precision

			if (firstclipnode != 0)
			{
				for (j = 0; j < 2; ++j)
				{
					if (d->children[j] >= 0)
						d->children[j] += firstclipnode;
				}
			}

			d->planenum = FindFinalPlane (&p);
		}
	}

	FreeOther(Hull->dclipnodes);
	FreeOther(Hull->dplanes);
}

/*
=================
CreateSingleHull

=================
*/
void CreateSingleHull (void)
{
	int			entnum;

	Message (MSGALWAYS, "Processing hull %d...", hullnum);

	options.verbose = options.verbosefile = true;

	if (!options.allverbose)
	{
		// Old variant : print only statistics for hull 1
		// Normally    : print only statistics for hull 0
		if (options.oldprint && hullnum != 1 || !options.oldprint && hullnum != 0)
			options.verbose = options.verbosefile = false;

		if (options.noverbose)
			options.verbose = false;
	}

	nummodels = numplanes = numclipnodes = 0;

// for each entity in the map file that has geometry
	for (entnum = 0 ; entnum < num_entities ; entnum++)
	{
		ProcessEntity (entnum);
		if (!options.allverbose)
			options.verbose = options.verbosefile = false; // Don't print rest of entities for this hull
	}

	WriteClipHull ();
}

/*
=================
CreateHulls

=================
*/
void CreateHulls (void)
{
// create the hulls sequentially
	Message (MSGALWAYS, "\nBuilding hulls sequentially...");

	hullnum = HullOrder[0];
	CreateSingleHull ();

	hullnum = HullOrder[1];
	CreateSingleHull ();

	hullnum = HullOrder[2];
	CreateSingleHull ();
}

/*
==================
BuildName
==================
*/
static void BuildName(char Name[], char Orig[], char Ext[])
{
	strcpy (Name, Orig);
	StripExtension (Name);
	strcat (Name, Ext);
}

/*
==================
ChkLimit
==================
*/
void ChkLimit (char Object[], int Value, int Limit)
{
	if (Value > Limit)
		Message (MSGWARNCRIT, "%s %d exceed normal engine max %d", Object, Value, Limit);
}

/*
=================
ProcessFile

=================
*/
void ProcessFile (char *sourcebase, char *bspfilename1)
{
	char hullfilename[1024];

// create filenames
	BuildName(bspfilename, bspfilename1, ".bsp");
	BuildName(hullfilename, bspfilename1, ".h1");
	BuildName(portfilename, bspfilename1, ".prt");
	BuildName(pointfilename, bspfilename1, ".pts");

	if (!options.onlyents)
	{
		remove (bspfilename);
		remove (hullfilename);
		hullfilename[strlen(hullfilename)-1] = '2';
		remove (hullfilename);
		remove (portfilename);
		remove (pointfilename);
	}

// load brushes and entities
	LoadMapFile (sourcebase);
	if (options.onlyents)
	{
		UpdateEntLump ();
		return;
	}

	GetWadName(); // Display WAD problems early

// init the tables to be shared by all models
	BeginBSPFile ();

// the clipping hulls will be written out to text files by forked processes
	CreateHulls ();

	ReadClipHull (0);
	ReadClipHull (1);
	ReadClipHull (2);

	// Check various engine limits
	ChkLimit ("Models", nummodels, 256);
	ChkLimit ("Planes", numplanes, 32767);
	ChkLimit ("Nodes", numnodes, MAX_MAP_NODES);
	ChkLimit ("Clipnodes", numclipnodes, 32767);
	ChkLimit ("Leafs", numleafs, MAX_MAP_LEAFS);
	ChkLimit ("Vertexes", numvertexes, MAX_MAP_VERTS);
	ChkLimit ("Faces", numfaces, 32767); // normal engine bug
	ChkLimit ("Marksurfaces", nummarksurfaces, 32767); // normal engine bug
	ChkLimit ("Texinfo", numtexinfo, MAX_MAP_TEXINFO);

	WriteEntitiesToString();
	FinishBSPFile ();

	if (LeakFileGenerated)
		remove (portfilename);
}

/*
==================
CreateGlobalArrays

==================
*/
void CreateGlobalArrays()
{
	CreateArray(face_t*,	    validfaces,	   MAX_MAP_PLANES);
	CreateArray(mbrush_t,	    mapbrushes,	   MAX_MAP_BRUSHES);
	CreateArray(entity_t,	    entities,	   MAX_MAP_ENTITIES);
	CreateArray(char16,	    miptex,	   MAX_MAP_TEXINFO);
	CreateArray(plane_t,	    planes,	   MAX_MAP_PLANES);
	CreateArray(face_t_p2,	    edgefaces,	   MAX_MAP_EDGES);
	CreateArray(dmodel_t,	    dmodels,	   MAX_MAP_MODELS);
	CreateArray(byte,	    dvisdata,	   MAX_MAP_VISIBILITY);
	CreateArray(byte,	    dlightdata,	   MAX_MAP_LIGHTING);
	CreateArray(byte,	    dtexdata,	   MAX_MAP_MIPTEX); // (dmiptexlump_t)
	CreateArray(char,	    dentdata,	   MAX_MAP_ENTSTRING);
	CreateArray(dleaf_t,	    dleafs,	   MAX_MAP_LEAFS);
	CreateArray(dplane_t,	    dplanes,	   MAX_MAP_PLANES);
	CreateArray(dvertex_t,	    dvertexes,	   MAX_MAP_VERTS);
	CreateArray(dnode_t,	    dnodes,	   MAX_MAP_NODES);
	CreateArray(texinfo_t,	    texinfo,	   MAX_MAP_TEXINFO);
	CreateArray(dface_t,	    dfaces,	   MAX_MAP_FACES);
	CreateArray(dclipnode_t,    dclipnodes,	   MAX_MAP_CLIPNODES);
	CreateArray(dedge_t,	    dedges,	   MAX_MAP_EDGES);
	CreateArray(unsigned short, dmarksurfaces, MAX_MAP_MARKSURFACES);
	CreateArray(int,	    dsurfedges,	   MAX_MAP_SURFEDGES);
}


/*
==============
PrintOptions
==============
*/
void PrintOptions(void)
{
	Message (MSGALWAYS, "TxQBSP performs geometric level processing of Quake .MAP files to create");
	Message (MSGALWAYS, "Quake .BSP files.\n");
	Message (MSGALWAYS, "txqbsp [options] sourcefile [destfile]\n");
	Message (MSGALWAYS, "Options:");
	Message (MSGALWAYS, "   -notjunc        Disable tjunc calculations");
	Message (MSGALWAYS, "   -nofill         Disable outside filling");
	Message (MSGALWAYS, "   -nofillclip     Disable outside filling in hull 1");
	Message (MSGALWAYS, "   -nofillvis      Disable outside filling in hull 0");
	Message (MSGALWAYS, "   -fill           Force outside filling in hulls 1/2 if they leak");
	Message (MSGALWAYS, "   -hilimit        Enable extreme capacity");
#if 1 // Rotating brush support (well not really but still ...
	Message (MSGALWAYS, "   -noskip         Disable skip tool");
	Message (MSGALWAYS, "   -hiprotate      Use old (Hipnotic) rotation");
#endif
	Message (MSGALWAYS, "   -onlyents       Only update .MAP entities");
	Message (MSGALWAYS, "   -noents         Remove all entities except world and players");
	Message (MSGALWAYS, "   -verbose        Print out more .MAP information");
	Message (MSGALWAYS, "   -verbosemem     Print out more memory information");
	Message (MSGALWAYS, "   -verbosetex     Print out more texture information");
	Message (MSGALWAYS, "   -noverbose      Print out almost no information at all");
	Message (MSGALWAYS, "   -nowarnings     Disable warning printouts");
	Message (MSGALWAYS, "   -quiet          Short for noverbose, nowarnings and nopercent");
	Message (MSGALWAYS, "   -nowatervis     Disable portal information for transparent water");
	Message (MSGALWAYS, "   -solid          Remove liquids and treat sky as solid");
	Message (MSGALWAYS, "   -q2map          Enable Q2 style map format");
	Message (MSGALWAYS, "   -group          Enable func_group parsing");
	Message (MSGALWAYS, "   -oldprint       Print hull 1 information instead of hull 0");
	Message (MSGALWAYS, "   -oldleak        Create an old-style QBSP .PTS file");
	Message (MSGALWAYS, "   -nosortface     Disable face sorting before processing");
	Message (MSGALWAYS, "   -oldcsg         Disable new CSG processing method");
	Message (MSGALWAYS, "   -oldexpand      Disable new brush expansion method");
	Message (MSGALWAYS, "   -altaxis        Enable alternate texture axis handling");
	Message (MSGALWAYS, "   -nopercent      Disable percent completion information");
	Message (MSGALWAYS, "   -percent        Enable percent completion information");
	Message (MSGALWAYS, "   -numpercent     Numerical percent completion information");
	Message (MSGALWAYS, "   -leak [n]       Hull # to generate leakfile for (default -1 = any)");
	Message (MSGALWAYS, "   -leakdist [n]   Distance between leakfile points (default 4)");
	Message (MSGALWAYS, "   -simpdist [n]   Simplification distance in leakfile (default 100)");
	Message (MSGALWAYS, "   -hull [n]       Hull # to make visible (default 0)");
	Message (MSGALWAYS, "   -starthull [n]  Hull # to start processing with (default 0)");
	Message (MSGALWAYS, "   -expand [n]     Hull expansion (default 0.0)");
	Message (MSGALWAYS, "   -epsilon [n]    Plane comparison, use 0.05 for old-style (default 0.01)");
	Message (MSGALWAYS, "   -subdivide [n]  Use different texture subdivision (default 240)");
	Message (MSGALWAYS, "   -priority [n]   Set thread priority 0-2 (below/normal/above, default 1)");
	Message (MSGALWAYS, "   sourcefile      .MAP file to process");
	Message (MSGALWAYS, "   destfile        .BSP file to output");

	SafeClose (logfile, NULL);

	exit(1);
}

/*
==============
ChkArgument
==============
*/
void ChkArgument(char Arg[], char *NextOption)
{
	if (NextOption == NULL)
		Message (MSGERR, "Missing '%s' argument", Arg);
}

/*
==============
GetArgument
==============
*/
int GetArgument(char Arg[], char *NextOption)
{
	ChkArgument(Arg, NextOption);
	return(atoi(NextOption));
}

/*
================
GetFloatArgument
================
*/
vec_t GetFloatArgument(char Arg[], char *NextOption)
{
	ChkArgument(Arg, NextOption);
	return(atof(NextOption));
}

/*
==================
main

==================
*/
#ifndef QUAKE_GAME
#define bsp_main main
#endif // QUAKE_GAME

int bsp_main (int argc, char **argv)
{
	int  i;
	char sourcename[1024], destname[1024], *Option, *NextOption;

        logfile = SafeOpen (logfilename, "w", false, NULL);

        Message (MSGALWAYS, "TxQBSP 1.13 -- Modified by Bengt Jardrup\n");

	if (logfile == NULL)
		Message (MSGWARNCRIT, "Unable to open %s: %s", logfilename, strerror(errno));

	// Default options
	memset(&options, 0, sizeof(options_t));
	options.SortFace = true;
	options.verbose = true;
	options.verbosefile = true;
	options.watervis = true;
	options.on_epsilon = 0.01;	// Default ON_EPSILON
	options.subdivide_size = 240;
	options.LeakDist = 4;
	options.SimpDist = 100;
	options.ReqLeakHull = -1;
#if 1 // Rotating brush support ... not really ... but still
	options.noskip = false;
	options.hiprotate = false;
#endif

//
// check command line flags
//
	for (i=1 ; i<argc ; i++)
	{
		Option = argv[i];
		NextOption = i + 1 < argc ? argv[i + 1] : NULL;

		if (Option[0] != '-')
			break;

		++Option;

		if (!stricmp (Option, "notjunc"))
			options.notjunc = true;
		else if (!stricmp (Option, "nofill"))
			options.nofill = true;
		else if (!stricmp (Option, "nofillclip"))
			options.NoFillClip = true;
		else if (!stricmp (Option, "nofillvis"))
			options.NoFillVis = true;
		else if (!stricmp (Option, "fill"))
			options.ForcedFill = true;
		else if (!stricmp (Option, "hilimit"))
			options.HiLimit = true;
#if 1 // Rotating brush support ... not really but still
		else if (!stricmp (Option, "noskip"))
			options.noskip = true;
		else if (!stricmp (Option, "hiprotate"))
			options.hiprotate = true;
#endif
		else if (!stricmp (Option, "onlyents"))
			options.onlyents = options.HiLimit = true;
		else if (!stricmp (Option, "noents"))
			options.noents = true;
		else if (!stricmp (Option, "verbose"))
			options.allverbose = true;
		else if (!stricmp (Option, "verbosemem"))
			options.verbosemem = true;
		else if (!stricmp (Option, "verbosetex"))
			options.verbosetex = true;
		else if (!stricmp (Option, "noverbose"))
		{
			options.verbose = false;
			options.noverbose = true;
		}
		else if (!stricmp (Option, "nowarnings"))
			options.nowarnings = true;
		else if (!stricmp (Option, "quiet"))
		{
			options.verbose = false;
			options.noverbose = options.nowarnings = options.NoPercent = true;
		}
		else if (!stricmp (Option, "nowatervis"))
			options.watervis = false;
		else if (!stricmp (Option, "solid"))
			options.SolidMap = true;
		else if (!stricmp (Option, "q2map"))
			options.Q2Map = true;
		else if (!stricmp (Option, "group"))
			options.group = true;
		else if (!stricmp (Option, "oldprint"))
			options.oldprint = true;
		else if (!stricmp (Option, "oldleak"))
			options.OldLeak = true;
		else if (!stricmp (Option, "nosortface"))
			options.SortFace = false;
		else if (!stricmp (Option, "oldcsg"))
			options.OldCsg = true;
		else if (!stricmp (Option, "oldexpand"))
			options.OldExpand = true;
		else if (!stricmp (Option, "altaxis"))
			options.AltAxis = true;
		else if (!stricmp (Option, "nopercent"))
			options.NoPercent = true;
		else if (!stricmp (Option, "percent"))
			options.NoPercent = false;
		else if (!stricmp (Option, "numpercent"))
		{
			options.NoPercent = false;
			options.NumPercent = true;
		}
		else if (!stricmp (Option, "leak"))
		{
			options.ReqLeakHull = GetArgument(Option, NextOption);
			i++;
		}
		else if (!stricmp (Option, "leakdist"))
		{
			options.LeakDist = GetArgument(Option, NextOption);

			if (options.LeakDist < 1)
				Message (MSGERR, "Leak distance must be > 0");

			i++;
		}
		else if (!stricmp (Option, "simpdist"))
		{
			options.SimpDist = GetArgument(Option, NextOption);

			if (options.SimpDist < 2)
				Message (MSGERR, "Simplification distance must be > 1");

			i++;
		}
		else if (!stricmp (Option, "hull"))
		{
			options.visiblehull = GetArgument(Option, NextOption);

			if (options.visiblehull < 0 || options.visiblehull > 2)
				Message (MSGERR, "Valid hulls are 0-2");

			i++;
		}
		else if (!stricmp (Option, "starthull"))
		{
			HullOrder[0] = GetArgument(Option, NextOption);

			if (HullOrder[0] < 0 || HullOrder[0] > 2)
				Message (MSGERR, "Valid hulls are 0-2");

			if (HullOrder[0] == 1)
			{
				// Old order
				HullOrder[1] = 2;
				HullOrder[2] = 0;
			}
			else if (HullOrder[0] == 2)
			{
				HullOrder[1] = 1;
				HullOrder[2] = 0;
			}

			i++;
		}
		else if (!stricmp (Option, "expand"))
		{
			options.HullExpansion[1] = GetFloatArgument(Option, NextOption);

			if (options.HullExpansion[1] < 0)
				Message (MSGERR, "Hull expansion must be >= 0");

			options.HullExpansion[0] = -options.HullExpansion[1];
			i++;
		}
		else if (!stricmp (Option, "epsilon"))
		{
			options.on_epsilon = GetFloatArgument(Option, NextOption);

			if (options.on_epsilon <= 0)
				Message (MSGERR, "Epsilon must be > 0");

			i++;
		}
		else if (!stricmp (Option, "subdivide"))
		{
			options.subdivide_size = GetArgument(Option, NextOption);
			i++;
		}
		else if (!stricmp (Option, "priority"))
		{
			SetQPriority(GetArgument(Option, NextOption));
			i++;
		}
		else if (!stricmp (Option, "?") || !stricmp (Option, "help"))
			PrintOptions();
		else
			Message (MSGERR, "Unknown option '%s'", Option);
	}

	if (i != argc - 2 && i != argc - 1)
		PrintOptions();

//
// let forked processes change name for ps status
//
	argv0 = argv[0];


//
// create destination name if not specified
//
	strcpy (sourcename, argv[i]);
	DefaultExtension (sourcename, ".map");

	if (i != argc - 2)
	{
		strcpy (destname, argv[i]);
		StripExtension (destname);
		strcat (destname, ".bsp");
	}
	else
		strcpy (destname, argv[i+1]);

	Message (MSGALWAYS, "Inputfile: %s", sourcename);
	Message (MSGVERBOSE, "Outputfile: %s", destname);

	if (!stricmp(strrchr(sourcename, '.'), ".bsp"))
		Message (MSGERR, "Sourcefile %s may not have bsp extension", sourcename);

//
// do it!
//
	start = I_FloatTime ();

        CreateGlobalArrays();

	ProcessFile (sourcename, destname);

	PrintFinish ();

	return 0;
}
