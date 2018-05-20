
#include "dyna1.h"


// upper design bounds

#define	MAX_MAP_HULLS		4

#define	MAX_MAP_MODELS		65536	  // was 256
#define	MAX_MAP_BRUSHES		65536	  // was 4096
#define	MAX_MAP_ENTITIES	65536	  // was 1024
#define	MAX_MAP_ENTSTRING	0x100000  // (1M) was 65536

#define	MAX_MAP_PLANES		65536	  // was 8192
#define	MAX_MAP_NODES		32767	  // because negative shorts are contents
#define	MAX_MAP_CLIPNODES	0x40000	  // (256k) was 32767
#define	MAX_MAP_LEAFS		32767	  // because negative shorts are contents
#define	MAX_MAP_VERTS		65535
#define	MAX_MAP_FACES		65535
#define	MAX_MAP_MARKSURFACES	65535	  // unsigned short
#define	MAX_MAP_TEXINFO		32767     // was 4096
#define	MAX_MAP_EDGES		0x200000  // (2M) was 256000
#define	MAX_MAP_SURFEDGES	0x400000  // (4M) was 512000
#define	MAX_MAP_MIPTEX		0x1000000 // (16M) was 0x200000 (2M)
#define	MAX_MAP_LIGHTING	0x800000  // (8M) was 0x100000 (1M)
#define	MAX_MAP_VISIBILITY	0x800000  // (8M) was 0x100000 (1M)

// key / value pair sizes

#define	MAX_KEY		32
#define	MAX_VALUE	1024

//=============================================================================


#define BSPVERSION	29

typedef struct
{
	int		fileofs, filelen;
} lump_t; // 8 byte

#define	LUMP_ENTITIES	   0 // pos in file 13
#define	LUMP_PLANES	   1 // pos in file  0
#define	LUMP_TEXTURES	   2 // pos in file 14
#define	LUMP_VERTEXES	   3 // pos in file  2
#define	LUMP_VISIBILITY	   4 // pos in file 12
#define	LUMP_NODES	   5 // pos in file  3
#define	LUMP_TEXINFO	   6 // pos in file  4
#define	LUMP_FACES	   7 // pos in file  5
#define	LUMP_LIGHTING	   8 // pos in file 11
#define	LUMP_CLIPNODES	   9 // pos in file  6
#define	LUMP_LEAFS	  10 // pos in file  1
#define	LUMP_MARKSURFACES 11 // pos in file  7
#define	LUMP_EDGES	  12 // pos in file  9
#define	LUMP_SURFEDGES	  13 // pos in file  8
#define	LUMP_MODELS	  14 // pos in file 10

#define	HEADER_LUMPS	15

typedef struct
{
	float		mins[3], maxs[3];
	float		origin[3];
	int		headnode[MAX_MAP_HULLS];
	int		visleafs;		// not including the solid leaf 0
	int		firstface, numfaces;
} dmodel_t; // 64 byte

typedef struct
{
	int		version;
	lump_t		lumps[HEADER_LUMPS];
} dheader_t; // 124 byte

typedef struct
{
	int		nummiptex;
	int		dataofs[4];		// [nummiptex]
} dmiptexlump_t; // 20 byte

#define	MIPLEVELS	4
typedef struct miptex_s
{
	char		name[16];
	unsigned	width, height;
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
} miptex_t; // 40 byte


typedef struct
{
	float	point[3];
} dvertex_t; // 12 byte


// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2

// 3-5 are non-axial planes snapped to the nearest
#define	PLANE_ANYX		3
#define	PLANE_ANYY		4
#define	PLANE_ANYZ		5

#define PlaneIsAxial(p) ((p)->type <= PLANE_Z)

typedef struct
{
	float		normal[3];
	float		dist;
	int		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dplane_t; // 20 byte



#define	CONTENTS_EMPTY		-1
#define	CONTENTS_SOLID		-2
#define	CONTENTS_WATER		-3
#define	CONTENTS_SLIME		-4
#define	CONTENTS_LAVA		-5
#define	CONTENTS_SKY		-6

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct
{
	int		planenum;
	short		children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for sphere culling
	short		maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides
} dnode_t; // 24 byte

typedef struct
{
	int		planenum;
	short		children[2];	// negative numbers are contents
} dclipnode_t; // 8 byte


typedef struct texinfo_s
{
	float		vecs[2][4];		// [s/t][xyz offset]
	int		miptex;
	int		flags;
} texinfo_t; // 40 byte

#define	TEX_SPECIAL	1		// sky or slime, no lightmap or 256 subdivision

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct
{
	unsigned short	v[2];		// vertex numbers
} dedge_t; // 4 byte

#define	MAXLIGHTMAPS	4
typedef struct
{
	short		planenum;
	short		side;

	int		firstedge;		// we must support > 64k edges
	short		numedges;
	short		texinfo;

// lighting info
	byte		styles[MAXLIGHTMAPS];
	int		lightofs;		// start of [numstyles*surfsize] samples
} dface_t; // 20 byte



#define	AMBIENT_WATER	0
#define	AMBIENT_SKY	1
#define	AMBIENT_SLIME	2
#define	AMBIENT_LAVA	3

#define	NUM_AMBIENTS	4		// automatic ambient sounds

// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
typedef struct
{
	int		contents;
	int		visofs;				// -1 = no visibility info

	short		mins[3];			// for frustum culling
	short		maxs[3];

	unsigned short	firstmarksurface;
	unsigned short	nummarksurfaces;

	byte		ambient_level[NUM_AMBIENTS];
} dleaf_t; // 28 byte

//============================================================================

// Formerly #ifndef QUAKE_GAME but I don't see this header getting shared any longer
//#ifndef QUAKE_GAME

extern	int		nummodels;
extern	dmodel_t	*dmodels;		//[MAX_MAP_MODELS];

extern	int		visdatasize;
extern	byte		*dvisdata;		//[MAX_MAP_VISIBILITY];

extern	int		lightdatasize;
extern	byte		*dlightdata;		//[MAX_MAP_LIGHTING];

extern	int		texdatasize;
extern	byte		*dtexdata;		//[MAX_MAP_MIPTEX];

extern	int		entdatasize;
extern	char		*dentdata;		//[MAX_MAP_ENTSTRING];

extern	int		numleafs;
extern	dleaf_t		*dleafs;		//[MAX_MAP_LEAFS];

extern	int		numplanes;
extern	dplane_t	*dplanes;		//[MAX_MAP_PLANES];

extern	int		numvertexes;
extern	dvertex_t	*dvertexes;		//[MAX_MAP_VERTS];

extern	int		numnodes;
extern	dnode_t		*dnodes;		//[MAX_MAP_NODES];

extern	int		numtexinfo;
extern	texinfo_t	*texinfo;		//[MAX_MAP_TEXINFO];

extern	int		numfaces;
extern	dface_t		*dfaces;		//[MAX_MAP_FACES];

extern	int		numclipnodes;
extern	dclipnode_t	*dclipnodes;		//[MAX_MAP_CLIPNODES];

extern	int		numedges;
extern	dedge_t		*dedges;		//[MAX_MAP_EDGES];

extern	int		nummarksurfaces;
extern	unsigned short	*dmarksurfaces;		//[MAX_MAP_MARKSURFACES];

extern	int		numsurfedges;
extern	int		*dsurfedges;		//[MAX_MAP_SURFEDGES];



void	LoadBSPFile (char *filename);
void	WriteBSPFile (char *filename);
void	PrintBSPFileSizes (void);

// #endif
