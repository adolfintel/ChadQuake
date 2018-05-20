
#include "bsp5.h"
#define __STDC__ 1 	// Kludge, otherwise conflict with cmdlib.h
#include "io.h"

int		headclipnode;
int		firstface;

//===========================================================================

/*
==================
FindFinalPlane

Used to find plane index numbers for clip nodes read from child processes
==================
*/
int FindFinalPlane (dplane_t *p)
{
	int		i;
	dplane_t	*dplane;

	for (i=0, dplane = dplanes ; i<numplanes ; i++, dplane++)
	{
		if (p->type != dplane->type)
			continue;
		if (p->dist != dplane->dist)
			continue;
		if (p->normal[0] != dplane->normal[0])
			continue;
		if (p->normal[1] != dplane->normal[1])
			continue;
		if (p->normal[2] != dplane->normal[2])
			continue;
		return i;
	}

//
// new plane
//
	ExtendArray(dplanes, numplanes);
	dplane = &dplanes[numplanes];
	*dplane = *p;
	numplanes++;

	return numplanes - 1;
}


SetupArray(int,	planemapping, MAX_MAP_PLANES);

void WriteNodePlanes_r (node_t *node)
{
	plane_t	 *plane;
	dplane_t *dplane;
	int	 i;

	if (node->planenum == -1)
		return;

	ExtendArray(planemapping, node->planenum);

	if (!planemapping[node->planenum])
	{
		plane = &planes[node->planenum];
		dplane = dplanes;

		// search for an equivalent plane
		for (i = 0; i < numplanes; i++, dplane++)
		{
			if (DotProduct(dplane->normal, plane->normal) > 1 - ANGLEEPSILON &&
			    fabs(dplane->dist - plane->dist) < DISTEPSILON &&
			    dplane->type == plane->type)
				break;
		}

		// Offset by 1 due to default value being 0 instead of -1
		planemapping[node->planenum] = i + 1;

		if (i == numplanes)
		{
			// a new plane
			ExtendArray(dplanes, numplanes);

			dplane = &dplanes[numplanes];
			dplane->normal[0] = plane->normal[0];
			dplane->normal[1] = plane->normal[1];
			dplane->normal[2] = plane->normal[2];
			dplane->dist = plane->dist;
			dplane->type = plane->type;

			numplanes++;
		}
	}

	// Offset by 1 due to default value being 0 instead of -1
	node->outputplanenum = planemapping[node->planenum] - 1;

	WriteNodePlanes_r (node->children[0]);
	WriteNodePlanes_r (node->children[1]);
}

/*
==================
WriteNodePlanes

==================
*/
void WriteNodePlanes (node_t *nodes)
{
	CreateArray(int, planemapping, MAX_MAP_PLANES);
	ClearArray(planemapping);
	WriteNodePlanes_r (nodes);
	ClearArray(planemapping);
}

//===========================================================================

/*
==================
FreeNodes_r
==================
*/
void FreeNodes_r (node_t *node)
{
	face_t	 *f, *fnext;

	if (node == NULL)
	    return;

	FreeNodes_r (node->children[0]);
	FreeNodes_r (node->children[1]);

	if (node->markfaces != NULL)
		FreeOther(node->markfaces);

	for (f = node->faces; f; f = fnext)
	{
		fnext = f->next;
		FreeFace (f);
	}

	FreeNode (node);
}

/*
==================
WriteClipNodes_r

==================
*/
int WriteClipNodes_r (node_t *node)
{
	int			i, c;
	dclipnode_t	*cn;
	int			num;

// FIXME: free more stuff?
	if (node->planenum == -1)
	{
		num = node->contents;
		return num;
	}

// emit a clipnode
	c = numclipnodes;
        ExtendArray(dclipnodes, numclipnodes);
	cn = &dclipnodes[numclipnodes];
	numclipnodes++;
	cn->planenum = node->outputplanenum;
	for (i=0 ; i<2 ; i++)
		cn->children[i] = WriteClipNodes_r(node->children[i]);

	return c;
}

/*
==================
WriteClipNodes

Called after the clipping hull is completed.  Generates a disk format
representation and frees the original memory.
==================
*/
void WriteClipNodes (node_t *nodes)
{
	headclipnode = numclipnodes;
	WriteClipNodes_r (nodes);

	FreeAllPortals (nodes);
	FreeNodes_r (nodes);
}

//===========================================================================

/*
==================
WriteLeaf
==================
*/
void WriteLeaf (node_t *node)
{
	face_t		**fp, *f;
	dleaf_t		*leaf_p;

// emit a leaf
        ExtendArray(dleafs, numleafs);
	leaf_p = &dleafs[numleafs];
	numleafs++;

	leaf_p->contents = node->contents;

//
// write bounding box info
//
	VectorCopy (node->mins, leaf_p->mins);
	VectorCopy (node->maxs, leaf_p->maxs);

	leaf_p->visofs = -1;	// no vis info yet

//
// write the marksurfaces
//
	leaf_p->firstmarksurface = nummarksurfaces;

	if (node->markfaces)
	{
		for (fp=node->markfaces ; *fp ; fp++)
		{
		// emit a marksurface
			f = *fp;
			do
			{
				ExtendArray(dmarksurfaces, nummarksurfaces);
				dmarksurfaces[nummarksurfaces] =  f->outputnumber;
				nummarksurfaces++;
				f=f->original;		// grab tjunction split faces
			} while (f);
		}
	}

	leaf_p->nummarksurfaces = nummarksurfaces - leaf_p->firstmarksurface;
}


/*
==================
WriteDrawNodes_r
==================
*/
void WriteDrawNodes_r (node_t *node)
{
	dnode_t	*n;
	int		i;

// emit a node
	ExtendArray(dnodes, numnodes);
	n = &dnodes[numnodes];
	numnodes++;

	VectorCopy (node->mins, n->mins);
	VectorCopy (node->maxs, n->maxs);

	n->planenum = node->outputplanenum;
	n->firstface = node->firstface;
	n->numfaces = node->numfaces;

//
// recursively output the other nodes
//

	for (i=0 ; i<2 ; i++)
	{
		if (node->children[i]->planenum == -1)
		{
			if (node->children[i]->contents == CONTENTS_SOLID)
				n->children[i] = -1;
			else
			{
				n->children[i] = -(numleafs + 1);
				WriteLeaf (node->children[i]);
			}
		}
		else
		{
			n->children[i] = numnodes;
			WriteDrawNodes_r (node->children[i]);
		}
	}
}

/*
==================
WriteDrawNodes
==================
*/
void WriteDrawNodes (node_t *headnode)
{
	int		i;
	int		start;
	dmodel_t	*bm;

#if 0
	if (headnode->contents < 0)
		Error ("FinishBSPModel: Empty model");
#endif

// emit a model
        ExtendArray(dmodels, nummodels);
	bm = &dmodels[nummodels];
	nummodels++;

	bm->headnode[0] = numnodes;
	bm->firstface = firstface;
	bm->numfaces = numfaces - firstface;
	firstface = numfaces;

	start = numleafs;

	if (headnode->contents < 0)
		WriteLeaf (headnode);
	else
		WriteDrawNodes_r (headnode);
	bm->visleafs = numleafs - start;

	// Check if # world vis leafs exceed normal engine max
	if (nummodels == 1)
		ChkLimit ("Vis leafs", bm->visleafs, 8192);

	for (i=0 ; i<3 ; i++)
	{
		bm->mins[i] = headnode->mins[i] + SIDESPACE + 1;	// remove the padding
		bm->maxs[i] = headnode->maxs[i] - SIDESPACE - 1;
	}
// FIXME: are all the children decendant of padded nodes?

	FreeAllPortals (headnode);
	FreeNodes_r (headnode);
}


/*
==================
BumpModel

Used by the clipping hull processes that only need to store headclipnode
==================
*/
void BumpModel (int hullnum)
{
	dmodel_t	*bm;

// emit a model
        ExtendArray(dmodels, nummodels);
	bm = &dmodels[nummodels];
	nummodels++;

	bm->headnode[hullnum] = headclipnode;
}

//=============================================================================

typedef struct
{
	char		identification[4];	// should be WAD2/WAD3
	int		numlumps;
	int		infotableofs;
} wadinfo_t;


typedef struct
{
	int		filepos;
	int		disksize;
	int		size;			// uncompressed
	char		type;
	char		compression;
	char		pad1, pad2;
	char		name[16];		// must be null terminated
} lumpinfo_t;

typedef struct
{
	FILE	   *texfile;
	char	   *texfilename;
	wadinfo_t  wadinfo;
	lumpinfo_t *lumpinfo;
	qboolean   Wad3;
} wadlist_t;

wadlist_t *wadlist;
int	  NoOfWads;

/*
=================
TEX_InitFromWad
=================
*/
void TEX_InitFromWad (void)
{
	wadlist_t *CWad;
	int	  i, Wad;

	for (Wad = 0; Wad < NoOfWads; ++Wad)
	{
		CWad = &wadlist[Wad];

		CWad->wadinfo.numlumps = tx_LittleLong (CWad->wadinfo.numlumps);
		CWad->wadinfo.infotableofs = tx_LittleLong (CWad->wadinfo.infotableofs);
		SafeSeek (CWad->texfile, CWad->texfilename, CWad->wadinfo.infotableofs);
		CWad->lumpinfo = AllocOther (CWad->wadinfo.numlumps * sizeof(lumpinfo_t));
		SafeRead (CWad->texfile, CWad->texfilename, CWad->lumpinfo, CWad->wadinfo.numlumps * sizeof(lumpinfo_t));

		for (i = 0; i < CWad->wadinfo.numlumps; i++)
		{
			CWad->lumpinfo[i].filepos = tx_LittleLong (CWad->lumpinfo[i].filepos);
			CWad->lumpinfo[i].disksize = tx_LittleLong (CWad->lumpinfo[i].disksize);
		}
	}
}

/*
==================
LoadLump
==================
*/
int LoadLump (char *name, byte *dest, byte *array)
{
	wadlist_t *CWad;
	miptex_t  *MipTex;
	int	  i, Wad, PalOffset;

	for (Wad = 0; Wad < NoOfWads; ++Wad)
	{
		CWad = &wadlist[Wad];

		for (i = 0; i < CWad->wadinfo.numlumps; i++)
		{
			if (!Q_strncasecmp(name, CWad->lumpinfo[i].name, 16))    //AR
			{
				if (dest == NULL)
				{
					// Only checking for missing textures
					if (options.allverbose || options.verbosetex)
						Message (MSGVERBOSE, "Texture %s found in %s", name, CWad->texfilename);

					return 0;
				}

				NeedArrayBytes (array, dest + CWad->lumpinfo[i].disksize - array);
				SafeSeek (CWad->texfile, CWad->texfilename, CWad->lumpinfo[i].filepos);
				SafeRead (CWad->texfile, CWad->texfilename, dest, CWad->lumpinfo[i].disksize);
				CleanupName (dest, dest);

				if (CWad->Wad3)
				{
					// Check if any palette and reduce texture size accordingly
					MipTex = (miptex_t *)dest;

					PalOffset = MipTex->offsets[MIPLEVELS - 1] + MipTex->width * MipTex->height / 64;

					if (PalOffset < CWad->lumpinfo[i].disksize)
						CWad->lumpinfo[i].disksize = CWad->lumpinfo[i].size = PalOffset;
				}

				return CWad->lumpinfo[i].disksize;
			}
		}
	}

	if (dest == NULL)
		Message (MSGWARN, "Texture %s not found", name);

	return 0;
}


/*
==================
AddAnimatingTextures
==================
*/
void AddAnimatingTextures (void)
{
	int	base;
	int	i, j, k, Wad;
	char	name[32];

	base = nummiptex;

	for (i=0 ; i<base ; i++)
	{
		if (miptex[i][0] != '+')
			continue;
		strcpy (name, miptex[i]);

		for (j=0 ; j<20 ; j++)
		{
			if (j < 10)
				name[1] = '0'+j;
			else
				name[1] = 'A'+j-10;		// alternate animation


		// see if this name exists in the wadfile
			for (Wad = 0; Wad < NoOfWads; ++Wad)
			{
				for (k = 0; k < wadlist[Wad].wadinfo.numlumps; k++)
				{
					if (!Q_strncasecmp(name, wadlist[Wad].lumpinfo[k].name, 16))
					{
						FindMiptex (name);	// add to the miptex list
						break;
					}
				}
			}
		}
	}

	if (nummiptex > base)
		Message (MSGNOVERBOSE, "Added %i texture frames", nummiptex - base);
}

#define MAX_WADPATHSIZE 0x40000 // 256K

SetupArray(char, WadPath, MAX_WADPATHSIZE);

/*
==================
ExpandWadPath
==================
*/
char *ExpandWadPath (char *Path)
{
	struct _finddata_t FindInfo;
	long		   Handle;
	int		   i, j, Result, Lth, Lth2;
	char		   Src[1024], FullPath[1024], Drv[_MAX_DRIVE], Dir[_MAX_DIR];
	char		   *PathBegin;
	qboolean	   WildCard = false;

	if (strchr (Path, '*') == NULL)
		return Path; // No wildcards

	CreateArray (char, WadPath, MAX_WADPATHSIZE);

	Lth = strlen (Path);
	PathBegin = NULL;

	for (i = j = 0; i < Lth + 1; ++i)
	{
		if (PathBegin == NULL)
			PathBegin = &Path[i]; // Beginning of new wad path

		NeedArrayBytes (WadPath, j);
		WadPath[j++] = Path[i];

		if (Path[i] == '*')
			WildCard = true;

		if (Path[i] != ';' && Path[i] != '\0')
			continue;

		// New wad path
		if (!WildCard)
		{
			PathBegin = NULL;
			continue; // No wildcard in this path
		}

		// New wildcard wad path
		WildCard = false;
		Lth2 = &Path[i] - PathBegin;
		memcpy (Src, PathBegin, Lth2);
		Src[Lth2] = '\0';
		PathBegin = NULL;

		j -= Lth2 + 1;
		WadPath[j] = '\0';

		_splitpath (Src, Drv, Dir, NULL, NULL);

		Handle = _findfirst (Src, &FindInfo);

		if (Handle == -1L)
			continue; // No file match

		Result = 0;

		while (Result == 0)
		{
			_makepath (FullPath, Drv, Dir, FindInfo.name, NULL);

			Lth2 = strlen (FullPath);

			// Add filename to list
			NeedArrayBytes (WadPath, j + Lth2 + 1);
			memcpy (&WadPath[j], FullPath, Lth2);
			j += Lth2;
			WadPath[j++] = ';';
			WadPath[j] = '\0';

			Result = _findnext (Handle, &FindInfo);
		}

		_findclose (Handle);
	}

	return WadPath;
}

/*
==================
GetWadName
==================
*/
void GetWadName (void)
{
	wadlist_t *wadtemp, *CWad;
	int	  Lth, Wad, i;
	char	  *path, *spath, *Ptr, *Ptr2;
	char	  fullpath[1024];
	qboolean  Exist, WadSizeOk, Wad2;

	/* Check worldmodel _wad/wad keys */
	path = ValueForKey (&entities[0], "_wad");

	if (!path[0])
	{
		path = ValueForKey (&entities[0], "wad");

		if (!path[0])
		{
			Message (MSGWARN, "No wadfiles specified in worldmodel");
			path = "";
		}
	}

	// Expand wildcards in wad path, return path might be reallocated
	path = ExpandWadPath (spath = path);

	Lth = strlen (path);

	/* Count WAD file specifications */
	NoOfWads = 1;

	for (i = 0; i < Lth; ++i)
	{
		if (path[i] == ';')
			++NoOfWads;
	}

	wadlist = (wadlist_t *)AllocOther (NoOfWads * sizeof(wadlist_t));

	Ptr = path;

	for (i = Wad = 0; i < NoOfWads; ++i)
	{
		CWad = &wadlist[Wad];

		Ptr2 = Ptr;

	    	/* Extract current WAD file name */
		while (*Ptr2 != ';' && *Ptr2 != '\0')
			++Ptr2;

		memset (fullpath, 0, sizeof(fullpath));

		if (Ptr2 - Ptr < sizeof(fullpath))
			memcpy (fullpath, Ptr, Ptr2 - Ptr);

		if (*Ptr2 != '\0')
			Ptr = Ptr2 + 1;

		/* Check existence */
		if (strlen (fullpath) == 0)
			Exist = false;
		else
			Exist = (CWad->texfile = SafeOpen (fullpath, "rb", false, &CWad->texfilename)) != NULL;

		if (!Exist && Wad == 0 && i == NoOfWads - 1)
		{
			/* Check default WAD name */
			strcpy (fullpath, bspfilename);

			StripExtension (fullpath);
			DefaultExtension (fullpath, ".wad");

			Exist = (CWad->texfile = SafeOpen (fullpath, "rb", false, &CWad->texfilename)) != NULL;

			if (Exist)
				Message (MSGNOVERBOSE, "Using default WAD: %s", fullpath);
		}

		if (Exist)
		{
			WadSizeOk = filelength (CWad->texfile) >= sizeof(wadinfo_t);

			if (WadSizeOk)
			{
				SafeRead (CWad->texfile, CWad->texfilename, &CWad->wadinfo, sizeof(wadinfo_t));

				Wad2 = !strncmp (CWad->wadinfo.identification, "WAD2", 4);
				CWad->Wad3 = !strncmp (CWad->wadinfo.identification, "WAD3", 4);
			}

			/* Check contents */
			if (WadSizeOk && (Wad2 || CWad->Wad3))
				++Wad;
			else
			{
				Message (MSGWARN, "%s isn't a wadfile", fullpath);
				SafeClose (CWad->texfile, CWad->texfilename);
			}
		}
	}

	if (path != spath)
		ClearArray (path);

	if (Wad == 0)
	{
		wadtemp = NULL;
		Message (MSGWARNCRIT, "No valid wadfiles in worldmodel");
	}
	else
	{
		// Remove invalid wads from memory
		wadtemp = (wadlist_t *)AllocOther (Wad * sizeof(wadlist_t));
		memcpy (wadtemp, wadlist, Wad * sizeof(wadlist_t));
	}

	FreeOther (wadlist);
	wadlist = wadtemp;
	NoOfWads = Wad;

	if (NoOfWads > 0)
	{
		// Report missing textures early on
		TEX_InitFromWad ();
		AddAnimatingTextures ();

		for (i = 0; i < nummiptex; ++i)
			LoadLump (miptex[i], NULL, NULL);
	}
}

/*
==================
WriteMiptex
==================
*/
void WriteMiptex (void)
{
	int		i, len;
	byte		*data;
	dmiptexlump_t	*l;

	if (NoOfWads == 0)
	{
		texdatasize = 0;
		return;
	}

	l = (dmiptexlump_t *)dtexdata;
	data = (byte *)&l->dataofs[nummiptex];
	l->nummiptex = nummiptex;
	for (i=0 ; i<nummiptex ; i++)
	{
		l->dataofs[i] = data - (byte *)l;
		len = LoadLump (miptex[i], data, dtexdata);
		if (data + len - dtexdata >= MAX_MAP_MIPTEX)
			Message (MSGERR, "Textures exceeded MAX_MAP_MIPTEX (%d)", MAX_MAP_MIPTEX);
		if (!len)
			l->dataofs[i] = -1;	// didn't find the texture
		data += len;
	}

	/* Free WAD list memory */
	for (i = 0; i < NoOfWads; ++i)
	{
		FreeOther (wadlist[i].lumpinfo);
		SafeClose (wadlist[i].texfile, wadlist[i].texfilename);
	}

	FreeOther (wadlist);

	texdatasize = data - dtexdata;
}

//===========================================================================


/*
==================
BeginBSPFile
==================
*/
void BeginBSPFile (void)
{
// edge 0 is not used, because 0 can't be negated
	numedges = 1;

// leaf 0 is common solid with no faces
	numleafs = 1;
	dleafs[0].contents = CONTENTS_SOLID;

	firstface = 0;
}


/*
==================
FinishBSPFile
==================
*/
void FinishBSPFile (void)
{
	Message (MSGNOVERBOSE, "\n------ FinishBSPFile ------");
	Message (MSGNOVERBOSE, "WriteBSPFile: %s", bspfilename);

	WriteMiptex ();

	PrintBSPFileSizes ();

	WriteBSPFile (bspfilename);
}

