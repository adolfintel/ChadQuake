
// bsp5.h

#include "cmdlib.h"
#include "txmathlib.h"
#include "txbspfile.h"

typedef struct
{
	vec3_t	normal;
	vec_t	dist;
	int	type;
} plane_t;


#include "map.h"

#define	MAX_THREADS	4

#define	ON_EPSILON	options.on_epsilon
#define	BOGUS_RANGE	18000

#define	DISTEPSILON	0.01
#define	ANGLEEPSILON	0.00001

// the exact bounding box of the brushes is expanded some for the headnode
// volume.  is this still needed?
#define	SIDESPACE	24

//============================================================================


typedef struct
{
	int	numpoints;
	vec3_t	points[8];			// variable sized
} winding_t;

#define MAX_POINTS_ON_WINDING	64

winding_t *BaseWindingForPlane (plane_t *p);
void	  CheckWinding (winding_t *w);
winding_t *NewWinding (int points);
void	  FreeWinding (winding_t *w);
#if 1 // Rotating brush support
void FreeWinding2 (winding_t *w);
#endif
winding_t *CopyWinding (winding_t *w);
winding_t *ClipWinding (winding_t *in, plane_t *split, qboolean keepon);
void	  DivideWinding (winding_t *in, plane_t *split, winding_t **front, winding_t **back);
void	  MidpointWinding(winding_t *w, vec3_t v);
#if 1 // Rotating brush support
winding_t *ClipWindingEpsilon (winding_t *in, plane_t *split, vec_t epsilon, qboolean keepon);
#endif

//============================================================================

#define	MAXEDGES  64 // This limit was expensive to raise in previous versions
#define	MAXPOINTS 28 // don't let a base face get past this
		     // because it can be split more later

typedef struct visfacet_s
{
	struct visfacet_s	*next;

	int			planenum;
	int			planeside;	// which side is the front of the face
	int			texturenum;
	int			contents[2];	// 0 = front side

	struct visfacet_s	*original;	// face on node
	int			outputnumber;	// only valid for original faces after
						// write surfaces
	int			numpoints;
	vec3_t			*pts;		// FIXME: change to use winding_t
	int			*edges;
} face_t;


typedef struct surface_s
{
	struct surface_s	*next;
	struct surface_s	*original;	// before BSP cuts it up
	int			planenum;
	int			outputplanenum;	// only valid after WriteSurfacePlanes
	vec3_t			mins, maxs;
	qboolean		onnode;		// true if surface has already been used
						// as a splitting node
	face_t			*faces;		// links to all the faces on either side of the surf
} surface_t;


//
// there is a node_t structure for every node and leaf in the bsp tree
//
#define	PLANENUM_LEAF		-1

typedef struct node_s
{
	vec3_t			mins,maxs;	// bounding volume, not just points inside

// information for decision nodes
	int			planenum;	// -1 = leaf node
	int			outputplanenum;	// only valid after WriteNodePlanes
	int			firstface;	// decision node only
	int			numfaces;	// decision node only
	struct node_s		*children[2];	// only valid for decision nodes
	face_t			*faces;		// decision nodes only, list for both sides

// information for leafs
	int			contents;	// leaf nodes (0 for decision nodes)
	face_t			**markfaces;	// leaf nodes only, point to node faces
	struct portal_s		*portals;
	int			visleafnum;	// -1 = solid
	int			valid;		// for flood filling
	int			occupied;	// light number in leaf for outside filling
} node_t;

//=============================================================================

// brush.c

#define	NUM_HULLS	2				// normal and +16

#define	NUM_CONTENTS	2				// solid and water

typedef struct brush_s
{
	struct brush_s	*next;
	vec3_t		mins, maxs;
	face_t		*faces;
	int		contents;
} brush_t;

typedef struct
{
	vec3_t		mins, maxs;
	brush_t		*brushes;		// NULL terminated list
} brushset_t;

extern	int		numbrushplanes;
extern	plane_t		*planes;                //[MAX_MAP_PLANES];

brushset_t *Brush_LoadEntity (entity_t *ent, int hullnum);
void	   FixRotateOrigin(entity_t *Ent, vec3_t offset);
void	   FreeBrushsetBrushes(void);
int	   PlaneTypeForNormal (vec3_t normal);
int	   FindPlane (plane_t *dplane, int *side);

//=============================================================================

// csg4.c

// build surfaces is also used by GatherNodeFaces
extern  int     validfacesactive;
extern  int     csgmergefaces;
extern	face_t	**validfaces;     //[MAX_MAP_PLANES];
surface_t	*BuildSurfaces (void);

winding_t *WindingResize (winding_t *w, int numpoints);
face_t	  *NewFaceFromFace (face_t *in);
surface_t *CSGFaces (brushset_t *bs);
void	  SplitFace (face_t *in, plane_t *split, face_t **front, face_t **back);

//=============================================================================

// solidbsp.c

void   DivideFacet (face_t *in, plane_t *split, face_t **front, face_t **back);
void   CalcSurfaceInfo (surface_t *surf);
void   SubdivideFace (face_t *f, face_t **prevptr);
node_t *SolidBSP (surface_t *surfhead, qboolean midsplit, qboolean ForcePrint, qboolean Progress);

//=============================================================================

// merge.c

void   MergePlaneFaces (surface_t *plane);
face_t *MergeFaceToList (face_t *face, face_t *list);
face_t *FreeMergeListScraps (face_t *merged);
void   MergeAll (surface_t *surfhead);

//=============================================================================

// surfaces.c

typedef face_t *face_t_p2[2];

extern	face_t_p2	*edgefaces;    //[MAX_MAP_EDGES];

extern	int		firstmodeledge;
extern	int		firstmodelface;

surface_t *GatherNodeFaces (node_t *headnode);
void	  MakeFaceEdges (node_t *headnode);

//=============================================================================

// portals.c

typedef struct portal_s
{
	int		planenum;
	node_t		*nodes[2];	// [0] = front side of planenum
	struct portal_s	*next[2];
	winding_t	*winding;
} portal_t;

extern	node_t	outside_node;		// portals outside the world face this
extern	int	num_visportals;

void PortalizeWorld (node_t *headnode, qboolean WriteFile);
void WritePortalfile (node_t *headnode);
void FreeAllPortals (node_t *node);

//=============================================================================

// region.c

void GrowNodeRegions (node_t *headnode);

//=============================================================================

// tjunc.c

void tjunc (node_t *headnode);

//=============================================================================

// writebsp.c

void WriteNodePlanes (node_t *headnode);
void WriteClipNodes (node_t *headnode);
void WriteDrawNodes (node_t *headnode);

void BumpModel (int hullnum);
int  FindFinalPlane (dplane_t *p);

void BeginBSPFile (void);
void FinishBSPFile (void);
void GetWadName (void);

//=============================================================================

// draw.c

void Draw_ClearBounds (void);
void Draw_AddToBounds (vec3_t v);
void Draw_DrawFace (face_t *f);
void Draw_ClearWindow (void);
void Draw_SetRed (void);
void Draw_SetGrey (void);
void Draw_SetBlack (void);
void DrawPoint (vec3_t v);

void Draw_SetColor (int c);
void SetColor (int c);
void DrawPortal (portal_t *p);
void DrawLeaf (node_t *l, int color);
void DrawBrush (brush_t *b);

void DrawWinding (winding_t *w);
void DrawTri (vec3_t p1, vec3_t p2, vec3_t p3);

//=============================================================================

// outside.c

extern qboolean LeakFileGenerated;
qboolean	FillOutside (node_t *node);

//=============================================================================

// qbsp.c

#define MSGVERBOSE 0
#define MSGNOVERBOSE 1
#define MSGALWAYS 2
#define MSGALWAYSEXACT 3
#define MSGFILE 4
#define MSGWARN 5
#define MSGWARNCRIT 6
#define MSGERR 7

typedef struct options_s
{
	qboolean	nofill;		  // Don't fill any hulls at all
	qboolean	ForcedFill;	  // Force fill hulls 1/2 even if they leak
	qboolean	NoFillClip;	  // Don't fill hull 1 even if it's sealed
	qboolean	NoFillVis;	  // Don't fill hull 0
	qboolean	OldLeak;	  // Disable leak line simplification
	qboolean	SortFace;	  // Sort faces before processing
	qboolean	OldCsg;		  // Old style CSG handling
	qboolean	OldExpand;	  // Old style brush expansion
	qboolean	AltAxis;	  // Alternate texture axis handling
	qboolean	notjunc;	  // Disable tjunction calculations
	qboolean	onlyents;	  // Compile only entities
	qboolean	noents;		  // Compile without entities (only world and players)
	qboolean	verbose;	  // Normally TRUE only during first hull
	qboolean	verbosefile;	  // Kludge
	qboolean	noverbose;	  // Less detailed printouts
	qboolean	nowarnings;	  // Disable most warnings to screen
	qboolean	verbosemem;	  // Verbose memory printout to log file
	qboolean	verbosetex;	  // Verbose texture printout
	qboolean	allverbose;	  // More detailed printouts
	qboolean	oldprint;	  // Print details for hull 1 instead of 0
	qboolean        watervis;	  // Enable transparent water
	qboolean        SolidMap;	  // No liquids allowed & sky will be solid
	qboolean        Q2Map;		  // Q2 style map format
	qboolean	group;		  // Enable func_group parsing
	qboolean	NoPercent;	  // No progress counter
	qboolean	NumPercent;	  // Numerical progress counter
	qboolean	HiLimit;	  // Enable extreme capacity
#if 1 // Rotating brush support ... not really but still ...
	qboolean	noskip;			// disable noskip tool
	qboolean	hiprotate;
#endif 
	vec_t		HullExpansion[2]; // Hull expansion value
	vec_t		on_epsilon;	  // ON_EPSILON
	int		subdivide_size;   // Face subdivision value
	int		visiblehull;	  // Which hull will be visible
	int		LeakDist;	  // Distance between points in leak file
	int		SimpDist;	  // Simplification distance (lower value => faster processing but longer trail)
	int		ReqLeakHull;	  // Which hull to generate pointfile for (-1 = any hull)
} options_t;

extern options_t	options;

extern	int		hullnum;

extern	brushset_t	*brushset;

void Message (int MsgType, ...);
char *GetCoord(vec3_t v);

extern	int		valid;

extern	char		portfilename[1024];
extern	char		bspfilename[1024];
extern	char		pointfilename[1024];

extern	qboolean	worldmodel;

//=============================================================================

// map.c

void	  CleanupName (char *in, char *out);

//=============================================================================

// misc functions

face_t	  *AllocFace (void);
void	  FreeFace (face_t *f);
void	  ResizeFace (face_t *f, int numpoints);
void	  CopyFace (face_t *dst, face_t *src);

portal_t  *AllocPortal (void);
void	  FreePortal (portal_t *p);

surface_t *AllocSurface (void);
void	  FreeSurface (surface_t *s);

node_t	  *AllocNode (void);
void	  FreeNode (node_t *n);

brush_t   *AllocBrush (void);
void	  FreeBrush (brush_t *b);

void	  *AllocOther (size_t Size);
void	  FreeOther (void *o);
void	  *ReAllocOther (void *o, size_t Size);

void	  ChkLimit(char Object[], int Value, int Limit);

//=============================================================================

