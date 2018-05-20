// map.c

#include "bsp5.h"

int			nummapfaces;
int			nummapbrushes;
SetupArray(mbrush_t,	mapbrushes, MAX_MAP_BRUSHES);

int			num_entities;
SetupArray(entity_t,	entities, MAX_MAP_ENTITIES);

int			nummiptex;
SetupArray(char16, 	miptex, MAX_MAP_TEXINFO);

char			MapTitle[MAX_VALUE];

//============================================================================

/*
===============
FindMiptex

===============
*/

//AR
void CleanupName (char *in, char *out)
{
	int		i;

	for (i = 0; i < sizeof(char16) - 1; i++)
	{
		if (!in[i])
			break;

		out[i] = tolower(in[i]);
	}

	for (; i < sizeof(char16); i++)
		out[i] = 0;
}

int FindMiptex (char *name)
{
	int		i;

	for (i=0 ; i<nummiptex ; i++)
	{
		if (!Q_strncasecmp(name, miptex[i], sizeof(char16)))
			return i;
	}
        ExtendArray(miptex, i);
	CleanupName (name, miptex[i]);
	nummiptex++;
	return i;
}

/*
===============
FindTexinfo

Returns a global texinfo number
===============
*/
int	FindTexinfo (texinfo_t *t)
{
	int			i, j;
	texinfo_t	*tex;

// set the special flag
	if (miptex[t->miptex][0] == '*' ||
	    !options.SolidMap && !Q_strncasecmp (miptex[t->miptex], "sky",3) )
		t->flags |= TEX_SPECIAL;


	tex = texinfo;
	for (i=0 ; i<numtexinfo;i++, tex++)
	{
		if (t->miptex != tex->miptex)
			continue;
		if (t->flags != tex->flags)
			continue;

		for (j=0 ; j<8 ; j++)
			if (t->vecs[0][j] != tex->vecs[0][j])
				break;
		if (j != 8)
			continue;

		return i;
	}

// allocate a new texture
	ExtendArray(texinfo, i);
	texinfo[i] = *t;
	numtexinfo++;

	return i;
}


//============================================================================

#define	MAXTOKEN	1024

char	token[MAXTOKEN];
qboolean	unget;
char	*script_p;
int		scriptline;
char	txcommand;

void	StartTokenParsing (char *data)
{
	scriptline = 1;
	script_p = data;
	unget = false;
}

qboolean GetToken (qboolean crossline)
{
	char    *token_p;

	if (unget)                         // is a token already waiting?
	{
		unget = false;
		return true;
	}

//
// skip space
//
skipspace:
	while (*script_p <= 32)
	{
		if (!*script_p)
		{
			if (!crossline)
				Message (MSGERR, "Line %i is incomplete",scriptline);
			return false;
		}
		if (*script_p++ == '\n')
		{
			if (!crossline)
				Message (MSGERR, "Line %i is incomplete",scriptline);
			scriptline++;
		}
	}

	if (script_p[0] == '/' && script_p[1] == '/')	// comment field
	{
		if (!crossline)
			Message (MSGERR, "Line %i is incomplete",scriptline);
		if (script_p[2] == 'T' && script_p[3] == 'X')  // AR: "//TX" command
			txcommand = script_p[4];
		while (*script_p++ != '\n')
			if (!*script_p)
			{
				if (!crossline)
					Message (MSGERR, "Line %i is incomplete",scriptline);
				return false;
			}

		scriptline++;
		goto skipspace;
	}

//
// copy token
//
	token_p = token;

	if (*script_p == '"')
	{
		script_p++;
		while ( *script_p != '"' )
		{
			if (!*script_p)
				Message (MSGERR, "EOF inside quoted token");
			*token_p++ = *script_p++;
			if (token_p > &token[MAXTOKEN-1])
				Message (MSGERR, "Token too large on line %i",scriptline);
		}
		script_p++;
	}
	else while ( *script_p > 32 )
	{
		*token_p++ = *script_p++;
		if (token_p > &token[MAXTOKEN-1])
			Message (MSGERR, "Token too large on line %i",scriptline);
	}

	*token_p = 0;

	return true;
}

void UngetToken ()
{
	unget = true;
}


//============================================================================

entity_t *mapent;
int	 PlayerStarts = 0;

/*
=================
ParseEpair
=================
*/
void ParseEpair (void)
{
	epair_t	*e;

	e = AllocOther (sizeof(epair_t));
	e->next = mapent->epairs;
	mapent->epairs = e;

	if (strlen(token) >= MAX_KEY-1)
		Message (MSGERR, "Entity key or value too long on line %d", scriptline);
	e->key = copystring(token);
	GetToken (false);
	if (strlen(token) >= MAX_VALUE-1)
		Message (MSGERR, "Entity key or value too long on line %d", scriptline);
	e->value = copystring(token);

	if (!stricmp(e->key, "classname") && !stricmp(e->value, "info_player_start"))
	{
		if (++PlayerStarts > 1)
			Message (MSGWARN, "Multiple info_player_start entities on line %d", scriptline);
	}
}

//============================================================================


/*
==================
textureAxisFromPlane
==================
*/
vec3_t	baseaxis[18] =
{
{0,0,1}, {1,0,0}, {0,-1,0},			// floor
{0,0,-1}, {1,0,0}, {0,-1,0},		// ceiling
{1,0,0}, {0,1,0}, {0,0,-1},			// west wall
{-1,0,0}, {0,1,0}, {0,0,-1},		// east wall
{0,1,0}, {1,0,0}, {0,0,-1},			// south wall
{0,-1,0}, {1,0,0}, {0,0,-1}			// north wall
};

void TextureAxisFromPlane (plane_t *pln, vec3_t xv, vec3_t yv)
{
	int		bestaxis;
	vec_t	dot,best;
	int		i;

	best = 0;
	bestaxis = 0;

	for (i=0 ; i<6 ; i++)
	{
		dot = DotProduct (pln->normal, baseaxis[i*3]);
		if (dot > best ||
		   (dot == best && options.AltAxis))
		{
			best = dot;
			bestaxis = i;
		}
	}

	VectorCopy (baseaxis[bestaxis*3+1], xv);
	VectorCopy (baseaxis[bestaxis*3+2], yv);
}


//=============================================================================

#define ScaleCorrection	(1.0/128.0)

static int Weight2 (vec_t n)
{
	if (fabs(n - 1) < 0.9)
		return(0);

	if (fabs(n + 1) < 0.9)
		return(1);

	return(2);
}

static int Weight (vec3_t n)
{
	return(Weight2(n[0]) * 100 + Weight2(n[1]) * 10 + Weight2(n[2]));
}

static qboolean Phase1;

static int CmpFace (const void *arg1, const void *arg2)
{
	mface_t *f1, *f2;
	int	Cmp;

	f1 = *(mface_t **)arg1;
	f2 = *(mface_t **)arg2;

	Cmp = Weight(f1->plane.normal) - Weight(f2->plane.normal);

	if (Phase1)
		Cmp += (int)(1000 * (f1->plane.dist - f2->plane.dist)); // To get determinism, we need this

	return(Cmp);
}

static void SortFaces (mbrush_t *b)
{
	mface_t	*Face[MAX_FACES], *Curr;
	int	Faces, I;

	for (Curr = b->faces, Faces = 0; Curr != NULL; Curr = Curr->next)
	{
		Face[Faces++] = Curr;

		if (Faces == MAX_FACES)
			return; // Too many faces
	}

	if (Faces < 2)
		return; // Too few faces

	Phase1 = true;

	// 1st sort in a deterministic order
	qsort(Face, Faces, sizeof(mface_t *), CmpFace);

	Phase1 = false;

	// 2nd sort in a "neat" order
	qsort(Face, Faces, sizeof(mface_t *), CmpFace);

	b->faces = Face[0];

	for (I = 0; I < Faces - 1; ++I)
		Face[I]->next = Face[I + 1];

	Face[I]->next = NULL;
}

static void DropBrush (mbrush_t	*b)
{
	mface_t	*f, *f2;

	for (f = b->faces; f != NULL; f = f2)
	{
		f2 = f->next;
		FreeOther (f);
	}

	b->faces = NULL;
}

static qboolean InvalidBrush (mbrush_t *b, int *PFaces)
{
	mface_t	 *f;
	vec3_t	 Sum;
	int	 i;
	qboolean NonAxial = false;

	Sum[0] = Sum[1] = Sum[2] = *PFaces = 0;

	for (f = b->faces; f != NULL; f = f->next, ++(*PFaces))
	{
		for (i = 0; i < 3; ++i)
		{
			if (f->plane.normal[i] != 0 && fabs(f->plane.normal[i]) != 1)
				NonAxial = true;

			Sum[i] += f->plane.normal[i];
		}
	}

	// Less than minimum or axial brush with less than 6 faces?
	if (*PFaces < 4 || *PFaces < 6 && !NonAxial)
	{
		Message (MSGWARN, "Too few (%d) faces in brush on line %d", *PFaces, b->Line);
		return true;
	}

	if (NonAxial)
		return false; // Don't know how to check more

	// Axial brush with more than 6 faces?
	if (*PFaces > 6)
	{
		Message (MSGWARN, "Too many (%d) faces in brush on line %d", *PFaces, b->Line);
		return false; // Not critical
	}

	// Axial brush => trivial test for closed
	if (Sum[0] != 0 || Sum[1] != 0 || Sum[2] != 0)
	{
		Message (MSGWARN, "Brush not closed on line %d", b->Line);
		return true;
	}

	return false;
}

static void GetValveTex (vec3_t VAxis)
{
	int i;

	// Skip [
	GetToken (false);

	for (i = 0; i < 3; i++)
	{
		VAxis[i] = atof(token);
		GetToken (false);
	}
}

static void Q2ToQ1Tex (char *Name)
{
	char *LastSlash;

	LastSlash = strrchr (Name, '/');

	if (LastSlash)
		memmove (Name, LastSlash + 1, strlen(LastSlash + 1) + 1);

	if (Name[0] == '#' || Name[0] == '%')
		Name[0] = '*';
}

/*
=================
ParseBrush
=================
*/
void ParseBrush (void)
{
	mbrush_t  *b;
	mface_t	  *f, *f2;
	vec3_t	  planepts[3];
	vec3_t	  t1, t2, t3, VAxis[2];
	int	  i, j, Faces;
	texinfo_t tx;
	vec_t	  d;
	vec_t	  shift[2], rotate, scale[2];
	qboolean  ok, Valve220;

        ExtendArray(mapbrushes, nummapbrushes);
	b = &mapbrushes[nummapbrushes];
	b->Line = scriptline;

	ok = GetToken (true);

	while (ok)
	{
		txcommand = 0;
		if (!strcmp (token, "}") )
		{
			if (!options.onlyents)
			{
				if (InvalidBrush(b, &Faces))
				{
					// Too few faces in brush or not closed; drop it
					DropBrush (b);
					nummapfaces -= Faces;
					// Note: texinfo array is not corrected here
					return;
				}
			}

			if (options.SortFace)
				SortFaces(b);

			break;
		}

	// read the three point plane definition
		for (i=0 ; i<3 ; i++)
		{
			if (i != 0)
				GetToken (true);
			if (strcmp (token, "(") )
				Message (MSGERR, "Invalid brush plane format on line %d", scriptline);

			for (j=0 ; j<3 ; j++)
			{
				GetToken (false);
				planepts[i][j] = atof(token);	// AR: atof
			}

			GetToken (false);
			if (strcmp (token, ")") )
				Message (MSGERR, "Invalid brush plane format on line %d", scriptline);

		}

	// read the texturedef
		memset (&tx, 0, sizeof(tx));
		GetToken (false);

		if (options.Q2Map)
			Q2ToQ1Tex (token);

		// Check texture name length
		if (strlen(token) > sizeof(char16) - 1)
			Message (MSGERR, "Texture name \"%s\" too long on line %d", token, scriptline);

		if (options.SolidMap && num_entities == 1 && token[0] == '*' ||
		    options.noents && num_entities > 1)
		{
			// No liquid worldbrushes or only worldbrushes allowed; drop brush
			DropBrush (b);

			while (GetToken(true) && strcmp(token, "}"))
				;

			return;
		}

		Valve220 = false;

		tx.miptex = FindMiptex (token);
		GetToken (false);

		if (!strcmp (token, "["))
		{
			// Valve 220 map import
			Valve220 = true;
			GetValveTex (VAxis[0]);
		}

		shift[0] = atoi(token);
		GetToken (false);

		if (Valve220)
		{
			// Skip ]
			GetToken (false);
			GetValveTex (VAxis[1]);
		}

		shift[1] = atoi(token);
		GetToken (false);

		if (Valve220)
			GetToken (false); // Skip ]

		rotate = atoi(token);
		GetToken (false);
		scale[0] = atof(token);
		GetToken (false);
		scale[1] = atof(token);

		if (options.Q2Map)
		{
			// Skip extra Q2 style face info
			GetToken (false);
			GetToken (false);
			GetToken (false);
		}

		ok = GetToken (true); // Note : scriptline normally gets advanced here

		// if the three points are all on a previous plane, it is a
		// duplicate plane
		for (f2 = b->faces ; f2 ; f2=f2->next)
		{
			for (i=0 ; i<3 ; i++)
			{
				d = DotProduct(planepts[i],f2->plane.normal) - f2->plane.dist;
				if (d < -ON_EPSILON || d > ON_EPSILON)
					break;
			}
			if (i==3)
				break;
		}
		if (f2)
		{
			Message (MSGWARN, "Brush with duplicate plane on line %d", scriptline - 1);
			continue;
		}

		f = AllocOther(sizeof(mface_t));

	// convert to a vector / dist plane
		for (j=0 ; j<3 ; j++)
		{
			t1[j] = planepts[0][j] - planepts[1][j];
			t2[j] = planepts[2][j] - planepts[1][j];
			t3[j] = planepts[1][j];
		}

		tx_CrossProduct(t1,t2, f->plane.normal);
		if (tx_VectorCompare (f->plane.normal, tx_vec3_origin))
		{
			Message (MSGWARN, "Brush plane with no normal on line %d", scriptline - 1);
			FreeOther (f);
			continue;
		}
		tx_VectorNormalize (f->plane.normal);
		f->plane.dist = DotProduct (t3, f->plane.normal);

		f->next = b->faces;
		b->faces = f;

		if (txcommand=='1' || txcommand=='2')
		{		// from QuArK, the texture vectors are given directly from the three points
			vec3_t	TexPt[2];
			int		k;
			vec_t	dot22, dot23, dot33, mdet, aa, bb, dd;

			k = txcommand-'0';
			for (j=0; j<3; j++)
				TexPt[1][j] = (planepts[k][j] - planepts[0][j]) * ScaleCorrection;
			k = 3-k;
			for (j=0; j<3; j++)
				TexPt[0][j] = (planepts[k][j] - planepts[0][j]) * ScaleCorrection;

			dot22 = DotProduct (TexPt[0], TexPt[0]);
			dot23 = DotProduct (TexPt[0], TexPt[1]);
			dot33 = DotProduct (TexPt[1], TexPt[1]);
			mdet = dot22*dot33-dot23*dot23;
			if (mdet<1E-6 && mdet>-1E-6)
			{
				aa = bb = dd = 0;
				Message (MSGWARN, "Degenerate QuArK-style brush texture on line %d", scriptline - 1);
			}
			else
			{
				mdet = 1.0/mdet;
      			aa = dot33*mdet;
      			bb = -dot23*mdet;
				//cc = -dot23*mdet;     // cc = bb
				dd = dot22*mdet;
			}

			for (j=0; j<3; j++)
			{
				tx.vecs[0][j] = aa * TexPt[0][j] + bb * TexPt[1][j];
				tx.vecs[1][j] = -(/*cc*/ bb * TexPt[0][j] + dd * TexPt[1][j]);
			}

			tx.vecs[0][3] = -DotProduct(tx.vecs[0], planepts[0]);
			tx.vecs[1][3] = -DotProduct(tx.vecs[1], planepts[0]);
		}
		else if (Valve220)
		{
			// Valve 220 texturedef
			vec3_t vec;
			vec_t  tscale;

			// Prevent division by zero
			if (!scale[0])
				scale[0] = 1;
			if (!scale[1])
				scale[1] = 1;

			// FIXME: If origin brushes are in use, this is where to fix their tex alignment!!!
			for (i = 0; i < 2; ++i)
			{
				tscale = 1 / scale[i];
				tx_VectorScale(VAxis[i], tscale, vec);

				for (j = 0; j < 3; ++j)
					tx.vecs[i][j] = (float)vec[j];

				tx.vecs[i][3] = (float)shift[i] + DotProduct(tx_vec3_origin, vec);
			}
		}
		else
	//
	// fake proper texture vectors from QuakeEd style
	//
		{
			vec3_t	vecs[2];
			int		sv, tv;
			vec_t	ang, sinv, cosv;
			vec_t	ns, nt;

			TextureAxisFromPlane(&f->plane, vecs[0], vecs[1]);

			if (!scale[0])
				scale[0] = 1;
			if (!scale[1])
				scale[1] = 1;

		// rotate axis
			if (rotate == 0)
				{ sinv = 0 ; cosv = 1; }
			else if (rotate == 90)
				{ sinv = 1 ; cosv = 0; }
			else if (rotate == 180)
				{ sinv = 0 ; cosv = -1; }
			else if (rotate == 270)
				{ sinv = -1 ; cosv = 0; }
			else
			{
				ang = rotate / 180 * Q_PI;
				sinv = sin(ang);
				cosv = cos(ang);
			}

			if (vecs[0][0])
				sv = 0;
			else if (vecs[0][1])
				sv = 1;
			else
				sv = 2;

			if (vecs[1][0])
				tv = 0;
			else if (vecs[1][1])
				tv = 1;
			else
				tv = 2;

			for (i=0 ; i<2 ; i++)
			{
				ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
				nt = sinv * vecs[i][sv] +  cosv * vecs[i][tv];
				vecs[i][sv] = ns;
				vecs[i][tv] = nt;
			}

			for (i=0 ; i<2 ; i++)
				for (j=0 ; j<3 ; j++)
					tx.vecs[i][j] = vecs[i][j] / scale[i];

			tx.vecs[0][3] = shift[0];
			tx.vecs[1][3] = shift[1];
		}

	// unique the texinfo
		f->texinfo = FindTexinfo (&tx);
		nummapfaces++;
	};

	nummapbrushes++;
	b->next = mapent->brushes;
	mapent->brushes = b;
}

/*
================
ParseEntity
================
*/
qboolean ParseEntity (void)
{
	epair_t	   *ep;
	entity_t   *world;
	mbrush_t   *b, *next;
	int	   Line, i;
	char	   *Classname;
	static int WorldSpawns = 0;

	if (!GetToken (true))
		return false;

	Line = scriptline;

	if (strcmp (token, "{") )
		Message (MSGERR, "Invalid entity format, { not found on line %d", Line);

	ExtendArray(entities, num_entities);
	mapent = &entities[num_entities];
	mapent->Line = Line;
	num_entities++;

	do
	{
		if (!GetToken (true))
			Message (MSGERR, "ParseEntity: EOF without closing brace");
		if (!strcmp (token, "}") )
			break;
		if (!strcmp (token, "{") )
			ParseBrush ();
		else
			ParseEpair ();
	} while (1);

	Classname = ValueForKey(mapent, "classname");

	if (strlen(Classname) == 0)
		Message (MSGERR, "No classname in entity on line %d", Line); // Missing classname
#if 1 // Rotating brush support ... err ... actually just an MH enhancement
	// mh - remove fraudulent angle key from worldspawn
	mapent->worldspawn = false;
#endif
	if (!stricmp(Classname, "worldspawn"))
	{
#if 1 // Rotating brush support ... err ... actually just an MH enhancement
		// mh - remove fraudulent angle key from worldspawn
		mapent->worldspawn = true;
#endif
		if (++WorldSpawns > 1)
			Message (MSGERR, "Multiple world entities on line %d", Line); // Multiple worlds

		if (!options.onlyents && !mapent->brushes)
			Message (MSGERR, "No world brushes on line %d", Line); // No world brushes

		// Get map title
		strcpy(MapTitle, ValueForKey(mapent, "message"));

		// Translate into simplified Quake character set
		for (i = 0; MapTitle[i] != '\0'; ++i)
		{
			MapTitle[i] &= 0x7F; // Ignore colour bit

			if (MapTitle[i] >= 0x12 && MapTitle[i] <= 0x1B)
				MapTitle[i] += 0x1E; // Extra 0-9 area
			else if (MapTitle[i] == 0x10)
				MapTitle[i] = '['; // Extra bracket
			else if (MapTitle[i] == 0x11)
				MapTitle[i] = ']'; // Extra bracket

			if (!isprint(MapTitle[i] & 0xFF))
				MapTitle[i] = ' ';
		}
	}
	else if (options.noents && strnicmp(Classname, "info_player_", 12))
	{
		// Only world and players allowed; drop entity

		for (ep = mapent->epairs; ep; ep = ep->next)
			FreeOther (ep->value);

		memset (mapent, 0, sizeof(entity_t));

		--num_entities;

		return true;
	}
	else if (options.group && !stricmp(Classname, "func_group"))
	{
		// Move entity brushes into world
		world = &entities[0];

		for (b = mapent->brushes; b; b = next)
		{
			next = b->next;
			b->next = world->brushes;
			world->brushes = b;
		}

		for (ep = mapent->epairs; ep; ep = ep->next)
			FreeOther (ep->value);

		memset (mapent, 0, sizeof(entity_t));

		--num_entities;

		return true;
	}

	if (num_entities == 1 && WorldSpawns == 0)
		Message (MSGERR, "World is not first entity on line %d", Line); // World is not first entity

	GetVectorForKey (mapent, "origin", mapent->origin);
	return true;
}

/*
================
LoadMapFile
================
*/
void LoadMapFile (char *filename)
{
	char *buf;

	Message (MSGVERBOSE, "------ LoadMapFile ------");

	LoadFile (filename, (void **)&buf);

	StartTokenParsing (buf);

	num_entities = 0;

	while (ParseEntity ())
	{
	}

	FreeOther (buf);

	if (num_entities == 0)
		Message(MSGERR, "No entities in map");

	if (PlayerStarts == 0)
		Message (MSGWARNCRIT, "No info_player_start entity in level");

	if (strlen(MapTitle) != 0)
		Message (MSGALWAYS, "Title: \"%s\"", MapTitle);

	Message (MSGVERBOSE, "%6i faces", nummapfaces);
	Message (MSGVERBOSE, "%6i brushes", nummapbrushes);
	Message (MSGVERBOSE, "%6i entities", num_entities);
	Message (MSGVERBOSE, "%6i miptex", nummiptex);
	Message (MSGVERBOSE, "%6i texinfo", numtexinfo);
}

void PrintEntity (entity_t *ent)
{
	epair_t	*ep;

	for (ep=ent->epairs ; ep ; ep=ep->next)
		Message (MSGVERBOSE, "%20s : %s", ep->key, ep->value);
}


char 	*ValueForKey (entity_t *ent, char *key)
{
	epair_t	*ep;

	for (ep=ent->epairs ; ep ; ep=ep->next)
		if (!strcmp (ep->key, key) )
			return ep->value;
	return "";
}

void 	SetKeyValue (entity_t *ent, char *key, char *value)
{
	epair_t	*ep;

	for (ep=ent->epairs ; ep ; ep=ep->next)
		if (!strcmp (ep->key, key) )
		{
			FreeOther (ep->value);
			ep->value = copystring(value);
			return;
		}
	ep = AllocOther (sizeof(*ep));
	ep->next = ent->epairs;
	ent->epairs = ep;
	ep->key = copystring(key);
	ep->value = copystring(value);
}

vec_t	FloatForKey (entity_t *ent, char *key)
{
	char	*k;

	k = ValueForKey (ent, key);
	return atof(k);
}

void 	GetVectorForKey (entity_t *ent, char *key, vec3_t vec)
{
	char	*k;
	double	v1, v2, v3;

	// Warning: No check for key existence here !
	k = ValueForKey (ent, key);
	v1 = v2 = v3 = 0;
// scanf into doubles, then assign, so it is vec_t size independent
	sscanf (k, "%lf %lf %lf", &v1, &v2, &v3);
	vec[0] = v1;
	vec[1] = v2;
	vec[2] = v3;
}


void WriteEntitiesToString (void)
{
	char	*buf, *end;
	epair_t	*ep;
	char	line[MAX_KEY + MAX_VALUE + 10];
	int	i, len;

	buf = dentdata;
	end = buf;
	NeedArrayBytes(dentdata, 2 + 1); // Extra for "{\n" below
	*end = 0;

	for (i=0 ; i<num_entities ; i++)
	{
		ep = entities[i].epairs;
		if (!ep)
			continue;	// ent got removed

		strcat (end, "{\n");
		end += 2;

		for (ep = entities[i].epairs ; ep ; ep=ep->next)
		{
#if 1 // Rotating brush support ... err ... actually just an MH enhancement
			// mh - remove fraudulent angle key from worldspawn
			if (entities[i].worldspawn && (!strcmp (ep->key, "angle") || !strcmp (ep->key, "angles")))
				continue;
#endif
			len = sprintf (line, "\"%s\" \"%s\"\n", ep->key, ep->value);

			if (len > 128)
			{
				strcpy(&line[128 - 2], "\"\n"); // Cut off string at 128
				len = 128;
			}

			NeedArrayBytes(dentdata, end - buf + len + 4 + 1); // Extra for "}\n" below and "{\n" above
			strcat (end, line);
			end += len;
		}
		strcat (end, "}\n");
		end += 2;
	}
	entdatasize = end - buf + 1;
}

