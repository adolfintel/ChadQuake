

#define	MAX_FACES	512 //128
typedef struct mface_s
{
	struct mface_s	*next;
	plane_t		plane;
	int		texinfo;
} mface_t;

typedef struct mbrush_s
{
	struct mbrush_s	*next;
	mface_t		*faces;
	int		Line;
} mbrush_t;

typedef struct epair_s
{
	struct epair_s	*next;
	char		*key;
	char		*value;
} epair_t;

typedef struct
{
#if 1 // Rotating brush support ... err ... actually just an MH enhancement
	qboolean	worldspawn;	// mh - remove fraudulent angles key from worldspawn
#endif
	vec3_t		origin;
	mbrush_t	*brushes;
	epair_t		*epairs;
	int		Line;
} entity_t;

typedef char		char16[16];

extern	int		nummapfaces;
extern	int		nummapbrushes;
extern	mbrush_t	*mapbrushes;                //[MAX_MAP_BRUSHES];

extern	int		num_entities;
extern	entity_t	*entities;                  //[MAX_MAP_ENTITIES];

extern	int		nummiptex;
extern	char16		*miptex;                    //[MAX_MAP_TEXINFO];

void	LoadMapFile (char *filename);

int	FindMiptex (char *name);

void	PrintEntity (entity_t *ent);
char	*ValueForKey (entity_t *ent, char *key);
void	SetKeyValue (entity_t *ent, char *key, char *value);
vec_t	FloatForKey (entity_t *ent, char *key);
void	GetVectorForKey (entity_t *ent, char *key, vec3_t vec);

void	WriteEntitiesToString (void);
