
#include "bsp5.h"


//=============================================================================

int			nummodels;
SetupArray(dmodel_t,	dmodels, MAX_MAP_MODELS);

int			visdatasize;
SetupArray(byte,	dvisdata, MAX_MAP_VISIBILITY);

int			lightdatasize;
SetupArray(byte,	dlightdata, MAX_MAP_LIGHTING);

int			texdatasize;
SetupArray(byte,	dtexdata, MAX_MAP_MIPTEX); // (dmiptexlump_t)

int			entdatasize;
SetupArray(char,	dentdata, MAX_MAP_ENTSTRING);

int			numleafs;
SetupArray(dleaf_t,	dleafs, MAX_MAP_LEAFS);

int			numplanes;
SetupArray(dplane_t,	dplanes, MAX_MAP_PLANES);

int			numvertexes;
SetupArray(dvertex_t,	dvertexes, MAX_MAP_VERTS);

int			numnodes;
SetupArray(dnode_t,	dnodes, MAX_MAP_NODES);

int			numtexinfo;
SetupArray(texinfo_t,	texinfo, MAX_MAP_TEXINFO);

int			numfaces;
SetupArray(dface_t,	dfaces, MAX_MAP_FACES);

int			numclipnodes;
SetupArray(dclipnode_t, dclipnodes, MAX_MAP_CLIPNODES);

int			numedges;
SetupArray(dedge_t,	dedges, MAX_MAP_EDGES);

int			nummarksurfaces;
SetupArray(unsigned short,dmarksurfaces, MAX_MAP_MARKSURFACES);

int			numsurfedges;
SetupArray(int, 	dsurfedges, MAX_MAP_SURFEDGES);

//=============================================================================

/*
=============
SwapBSPFile

Byte swaps all data in a bsp file.
=============
*/
void SwapBSPFile (qboolean todisk)
{
	int		i, j, c;
	dmodel_t	*d;
	dmiptexlump_t	*mtl;


// models
	for (i=0 ; i<nummodels ; i++)
	{
		d = &dmodels[i];

		for (j=0 ; j<MAX_MAP_HULLS ; j++)
			d->headnode[j] = tx_LittleLong (d->headnode[j]);

		d->visleafs = tx_LittleLong (d->visleafs);
		d->firstface = tx_LittleLong (d->firstface);
		d->numfaces = tx_LittleLong (d->numfaces);

		for (j=0 ; j<3 ; j++)
		{
			d->mins[j] = tx_LittleFloat(d->mins[j]);
			d->maxs[j] = tx_LittleFloat(d->maxs[j]);
			d->origin[j] = tx_LittleFloat(d->origin[j]);
		}
	}

//
// vertexes
//
	for (i=0 ; i<numvertexes ; i++)
	{
		for (j=0 ; j<3 ; j++)
			dvertexes[i].point[j] = tx_LittleFloat (dvertexes[i].point[j]);
	}

//
// planes
//
	for (i=0 ; i<numplanes ; i++)
	{
		for (j=0 ; j<3 ; j++)
			dplanes[i].normal[j] = tx_LittleFloat (dplanes[i].normal[j]);
		dplanes[i].dist = tx_LittleFloat (dplanes[i].dist);
		dplanes[i].type = tx_LittleLong (dplanes[i].type);
	}

//
// texinfos
//
	for (i=0 ; i<numtexinfo ; i++)
	{
		for (j=0 ; j<8 ; j++)
			texinfo[i].vecs[0][j] = tx_LittleFloat (texinfo[i].vecs[0][j]);
		texinfo[i].miptex = tx_LittleLong (texinfo[i].miptex);
		texinfo[i].flags = tx_LittleLong (texinfo[i].flags);
	}

//
// faces
//
	for (i=0 ; i<numfaces ; i++)
	{
		dfaces[i].texinfo = tx_LittleShort (dfaces[i].texinfo);
		dfaces[i].planenum = tx_LittleShort (dfaces[i].planenum);
		dfaces[i].side = tx_LittleShort (dfaces[i].side);
		dfaces[i].lightofs = tx_LittleLong (dfaces[i].lightofs);
		dfaces[i].firstedge = tx_LittleLong (dfaces[i].firstedge);
		dfaces[i].numedges = tx_LittleShort (dfaces[i].numedges);
	}

//
// nodes
//
	for (i=0 ; i<numnodes ; i++)
	{
		dnodes[i].planenum = tx_LittleLong (dnodes[i].planenum);
		for (j=0 ; j<3 ; j++)
		{
			dnodes[i].mins[j] = tx_LittleShort (dnodes[i].mins[j]);
			dnodes[i].maxs[j] = tx_LittleShort (dnodes[i].maxs[j]);
		}
		dnodes[i].children[0] = tx_LittleShort (dnodes[i].children[0]);
		dnodes[i].children[1] = tx_LittleShort (dnodes[i].children[1]);
		dnodes[i].firstface = tx_LittleShort (dnodes[i].firstface);
		dnodes[i].numfaces = tx_LittleShort (dnodes[i].numfaces);
	}

//
// leafs
//
	for (i=0 ; i<numleafs ; i++)
	{
		dleafs[i].contents = tx_LittleLong (dleafs[i].contents);
		for (j=0 ; j<3 ; j++)
		{
			dleafs[i].mins[j] = tx_LittleShort (dleafs[i].mins[j]);
			dleafs[i].maxs[j] = tx_LittleShort (dleafs[i].maxs[j]);
		}

		dleafs[i].firstmarksurface = tx_LittleShort (dleafs[i].firstmarksurface);
		dleafs[i].nummarksurfaces = tx_LittleShort (dleafs[i].nummarksurfaces);
		dleafs[i].visofs = tx_LittleLong (dleafs[i].visofs);
	}

//
// clipnodes
//
	for (i=0 ; i<numclipnodes ; i++)
	{
		dclipnodes[i].planenum = tx_LittleLong (dclipnodes[i].planenum);
		dclipnodes[i].children[0] = tx_LittleShort (dclipnodes[i].children[0]);
		dclipnodes[i].children[1] = tx_LittleShort (dclipnodes[i].children[1]);
	}

//
// miptex
//
	if (texdatasize)
	{
		mtl = (dmiptexlump_t *)dtexdata;
		if (todisk)
			c = mtl->nummiptex;
		else
			c = tx_LittleLong(mtl->nummiptex);
		mtl->nummiptex = tx_LittleLong (mtl->nummiptex);
		for (i=0 ; i<c ; i++)
			mtl->dataofs[i] = tx_LittleLong(mtl->dataofs[i]);
	}

//
// marksurfaces
//
	for (i=0 ; i<nummarksurfaces ; i++)
		dmarksurfaces[i] = tx_LittleShort (dmarksurfaces[i]);

//
// surfedges
//
	for (i=0 ; i<numsurfedges ; i++)
		dsurfedges[i] = tx_LittleLong (dsurfedges[i]);

//
// edges
//
	for (i=0 ; i<numedges ; i++)
	{
		dedges[i].v[0] = tx_LittleShort (dedges[i].v[0]);
		dedges[i].v[1] = tx_LittleShort (dedges[i].v[1]);
	}
}


dheader_t	*header;

int CopyLump (int lump, void *dest, int size)
{
	int	length, ofs;

	length = header->lumps[lump].filelen;
	ofs = header->lumps[lump].fileofs;

	if (length % size)
		Message (MSGERR, "LoadBSPFile: Odd lump size");

	NeedArrayBytes(dest, length);
	memcpy (dest, (byte *)header + ofs, length);

	return length / size;
}

/*
=============
LoadBSPFile
=============
*/
void	LoadBSPFile (char *filename)
{
	int	i;

//
// load the file header
//
	LoadFile (filename, (void **)&header);

// swap the header
	for (i=0 ; i< sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = tx_LittleLong ( ((int *)header)[i]);

	if (header->version != BSPVERSION)
		Message (MSGERR, "%s is version %i, not %i", filename, header->version, BSPVERSION);

	nummodels = CopyLump (LUMP_MODELS, dmodels, sizeof(dmodel_t));
	numvertexes = CopyLump (LUMP_VERTEXES, dvertexes, sizeof(dvertex_t));
	numplanes = CopyLump (LUMP_PLANES, dplanes, sizeof(dplane_t));
	numleafs = CopyLump (LUMP_LEAFS, dleafs, sizeof(dleaf_t));
	numnodes = CopyLump (LUMP_NODES, dnodes, sizeof(dnode_t));
	numtexinfo = CopyLump (LUMP_TEXINFO, texinfo, sizeof(texinfo_t));
	numclipnodes = CopyLump (LUMP_CLIPNODES, dclipnodes, sizeof(dclipnode_t));
	numfaces = CopyLump (LUMP_FACES, dfaces, sizeof(dface_t));
	nummarksurfaces = CopyLump (LUMP_MARKSURFACES, dmarksurfaces, sizeof(dmarksurfaces[0]));
	numsurfedges = CopyLump (LUMP_SURFEDGES, dsurfedges, sizeof(dsurfedges[0]));
	numedges = CopyLump (LUMP_EDGES, dedges, sizeof(dedge_t));

	texdatasize = CopyLump (LUMP_TEXTURES, dtexdata, 1);
	visdatasize = CopyLump (LUMP_VISIBILITY, dvisdata, 1);
	lightdatasize = CopyLump (LUMP_LIGHTING, dlightdata, 1);
	entdatasize = CopyLump (LUMP_ENTITIES, dentdata, 1);

	FreeOther (header);		// everything has been copied out

//
// swap everything
//
	SwapBSPFile (false);
}

//============================================================================

FILE	  *wadfile;
dheader_t outheader;
char	  *wadfilename;

void AddLump (int lumpnum, void *data, int len)
{
	lump_t *lump;
	int    extra;
	byte   padd[4] = {0, 0, 0, 0};

	lump = &header->lumps[lumpnum];

	lump->fileofs = tx_LittleLong( ftell(wadfile) );
	lump->filelen = tx_LittleLong(len);
	SafeWrite (wadfile, wadfilename, data, len);

	extra = ((len + 3) & ~3) - len;

	if (extra > 0)
		SafeWrite (wadfile, wadfilename, padd, extra); // Padd with zeroes to even 4-byte boundary
}

#if 1 // Rotating brush support ... err ... actually just an MH enhancement
int skipcount;

void SkipBSPFile (void)
{
	int i, j, k;
//	unsigned int *leafmarks;
// Baker: ....
	unsigned short *leafmarks;
	dleaf_t *leaf;
	dface_t *face;
	texinfo_t *ti;
	miptex_t *textures, *texture;
	char *name;
	dmiptexlump_t *miptexlump;
	dmodel_t *model;
	dface_t *modfaces;

	miptexlump = (dmiptexlump_t *) dtexdata;
	textures = (miptex_t *) dtexdata;
	skipcount = 0;

	for (i = 0, leaf = dleafs; i < numleafs; i++, leaf++)
	{
		leafmarks = dmarksurfaces + leaf->firstmarksurface;

		for (j = 0; j < leaf->nummarksurfaces;)
		{
			face = dfaces + leafmarks[j];
			ti = texinfo + face->texinfo;
			texture = (miptex_t *) ((byte *) textures + miptexlump->dataofs[ti->miptex]);
			name = texture->name;

			if (!strcmp (name, "skip") ||
				!strcmp (name, "*waterskip") ||
				!strcmp (name, "*slimeskip") ||
				!strcmp (name, "*lavaskip"))
			{
				// copy each remaining marksurface to previous slot
				for (k = j; k < leaf->nummarksurfaces - 1; k++)
					leafmarks[k] = leafmarks[k + 1];

				// reduce marksurface count by one
				leaf->nummarksurfaces--;

				skipcount++;
			}
			else j++;
		}
	}

	// loop through all the models, editing surface lists (this takes care of brush entities)
	for (i = 0, model = dmodels; i < nummodels; i++, model++)
	{
		if (i == 0) continue; // model 0 is worldmodel

		modfaces = dfaces + model->firstface;

		for (j = 0; j < model->numfaces;)
		{
			face = modfaces + j;
			ti = texinfo + face->texinfo;
			texture = (miptex_t *) ((byte *) textures + miptexlump->dataofs[ti->miptex]);
			name = texture->name;

			if (!strcmp (name, "skip") ||
				!strcmp (name, "*waterskip") ||
				!strcmp (name, "*slimeskip") ||
				!strcmp (name, "*lavaskip"))
			{
				// copy each remaining face to previous slot
				for (k = j; k < model->numfaces - 1; k++)
					modfaces[k] = modfaces[k + 1];

				// reduce face count by one
				model->numfaces--;

				skipcount++;
			}
			else j++;
		}
	}

	Message (MSGALWAYS, "Skipped %i surfaces", skipcount);
}
#endif

/*
=============
WriteBSPFile

Swaps the bsp file in place, so it should not be referenced again
=============
*/
void	WriteBSPFile (char *filename)
{
	header = &outheader;
	memset (header, 0, sizeof(dheader_t));
#if 1 // Rotating brush support ... err ... actually just an MH enhancement
	if (!options.noskip) SkipBSPFile ();
#endif

	SwapBSPFile (true);

	header->version = tx_LittleLong (BSPVERSION);

	wadfile = SafeOpen (filename, "wb", true, &wadfilename);
	SafeWrite (wadfile, wadfilename, header, sizeof(dheader_t)); // overwritten later

	AddLump (LUMP_PLANES, dplanes, numplanes*sizeof(dplane_t));
	AddLump (LUMP_LEAFS, dleafs, numleafs*sizeof(dleaf_t));
	AddLump (LUMP_VERTEXES, dvertexes, numvertexes*sizeof(dvertex_t));
	AddLump (LUMP_NODES, dnodes, numnodes*sizeof(dnode_t));
	AddLump (LUMP_TEXINFO, texinfo, numtexinfo*sizeof(texinfo_t));
	AddLump (LUMP_FACES, dfaces, numfaces*sizeof(dface_t));
	AddLump (LUMP_CLIPNODES, dclipnodes, numclipnodes*sizeof(dclipnode_t));
	AddLump (LUMP_MARKSURFACES, dmarksurfaces, nummarksurfaces*sizeof(dmarksurfaces[0]));
	AddLump (LUMP_SURFEDGES, dsurfedges, numsurfedges*sizeof(dsurfedges[0]));
	AddLump (LUMP_EDGES, dedges, numedges*sizeof(dedge_t));
	AddLump (LUMP_MODELS, dmodels, nummodels*sizeof(dmodel_t));

	AddLump (LUMP_LIGHTING, dlightdata, lightdatasize);
	AddLump (LUMP_VISIBILITY, dvisdata, visdatasize);
	AddLump (LUMP_ENTITIES, dentdata, entdatasize);
	AddLump (LUMP_TEXTURES, dtexdata, texdatasize);

	SafeSeek (wadfile, wadfilename, 0);
	SafeWrite (wadfile, wadfilename, header, sizeof(dheader_t));
	SafeClose (wadfile, wadfilename);
}

//============================================================================

/*
=============
PrintBSPFileSizes

Dumps info about current file
=============
*/
void PrintBSPFileSizes (void)
{
	Message (MSGNOVERBOSE, "%6i planes      %7i"
		,numplanes, (int)(numplanes*sizeof(dplane_t)));
	Message (MSGNOVERBOSE, "%6i vertexes    %7i"
		,numvertexes, (int)(numvertexes*sizeof(dvertex_t)));
	Message (MSGNOVERBOSE, "%6i nodes       %7i"
		,numnodes, (int)(numnodes*sizeof(dnode_t)));
	Message (MSGNOVERBOSE, "%6i texinfo     %7i"
		,numtexinfo, (int)(numtexinfo*sizeof(texinfo_t)));
	Message (MSGNOVERBOSE, "%6i faces       %7i"
		,numfaces, (int)(numfaces*sizeof(dface_t)));
	Message (MSGNOVERBOSE, "%6i clipnodes   %7i"
		,numclipnodes, (int)(numclipnodes*sizeof(dclipnode_t)));
	Message (MSGNOVERBOSE, "%6i leafs       %7i"
		,numleafs, (int)(numleafs*sizeof(dleaf_t)));
	Message (MSGNOVERBOSE, "%6i marksurfaces%7i"
		,nummarksurfaces, (int)(nummarksurfaces*sizeof(dmarksurfaces[0])));
	Message (MSGNOVERBOSE, "%6i surfedges   %7i"
		,numsurfedges, (int)(numsurfedges*sizeof(dsurfedges[0])));
	Message (MSGNOVERBOSE, "%6i edges       %7i"
		,numedges, (int)(numedges*sizeof(dedge_t)));

	Message (MSGNOVERBOSE, "%6i textures  %9i", texdatasize ? ((dmiptexlump_t *)dtexdata)->nummiptex : 0, texdatasize);
	Message (MSGNOVERBOSE, "%6s lightdata %9i", "", lightdatasize);
	Message (MSGNOVERBOSE, "%6s visdata   %9i", "", visdatasize);
	Message (MSGNOVERBOSE, "%6s entdata   %9i", "", entdatasize);
}


