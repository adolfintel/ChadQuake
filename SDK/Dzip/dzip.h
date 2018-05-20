
#if _MSC_VER > 1400 // Baker: VS 2008
	#define _CRT_SECURE_NO_WARNINGS // Get rid of error messages about secure functions
	#define POINTER_64 __ptr64 // VS2008+ include order involving DirectX SDK can cause this not to get defined PVOID64 stupidity
	#pragma warning(disable:4996) // VS2008+ do not like fileno, stricmp, strdup but we aren't using threads and Microsoft only functions = NO
#endif // Baker

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zlib/zlib.h"

typedef unsigned char uchar;

#define MAX_ENT 1024
#define MAJOR_VERSION 2
#define MINOR_VERSION 9
#define INITCRC 0xffffffff

enum { TYPE_NORMAL, TYPE_DEMV1, TYPE_TXT, TYPE_PAK, TYPE_DZ, TYPE_DEM,
	TYPE_NEHAHRA, TYPE_DIR, TYPE_STORE, TYPE_LAST };

enum {
	DEM_bad, DEM_nop, DEM_disconnect, DEM_updatestat, DEM_version,
	DEM_setview, DEM_sound, DEM_time, DEM_print, DEM_stufftext,
	DEM_setangle, DEM_serverinfo, DEM_lightstyle, DEM_updatename,
	DEM_updatefrags, DEM_clientdata, DEM_stopsound, DEM_updatecolors,
	DEM_particle, DEM_damage, DEM_spawnstatic, DEM_spawnbinary,
	DEM_spawnbaseline, DEM_temp_entity, DEM_setpause, DEM_signonnum,
	DEM_centerprint, DEM_killedmonster, DEM_foundsecret,
	DEM_spawnstaticsound, DEM_intermission, DEM_finale,
	DEM_cdtrack, DEM_sellscreen, DEM_cutscene, DZ_longtime,
/* nehahra */
	DEM_showlmp = 35, DEM_hidelmp, DEM_skybox, DZ_showlmp
};

typedef struct {
	uchar voz, pax;
	uchar ang0, ang1, ang2;
	uchar vel0, vel1, vel2;
	long items;
	uchar uk10, uk11, invbit;
	uchar wpf, av, wpm;
	int health;
	uchar am, sh, nl, rk, ce, wp;
	int force;
} cdata_t;

typedef struct { 
	uInt ptr;	/* start of file inside dz */
	uInt size;	/* v1: intermediate size. v2: compressed size */
	uInt real;	/* uncompressed size */
	unsigned short len;	/* length of name */
	unsigned short pak;
	uInt crc;
	uInt type;
	uInt date;
	uInt inter;	/* v2: intermediate size */
	char *name;
} direntry_t;
#define DE_DISK_SIZE 32

typedef struct {
	uchar modelindex, frame;
	uchar colormap, skin;
	uchar effects;
	uchar ang0, ang1, ang2;
	uchar newbit, present, active;
	uchar fullbright;	/* nehahra */
	int org0, org1, org2;
	int od0, od1, od2;
	int force;
	float alpha;		/* nehahra */
} ent_t;

typedef struct {
	char name[56];
	uInt ptr;
	uInt len;
} pakentry_t;

int bplus (int, int);
void copy_msg (uInt);
void create_clientdata_msg (void);
void crc_init (void);
void dem_compress (uInt, uInt);
void dem_copy_ue (void);
uInt dem_uncompress (uInt);
void dem_uncompress_init (int);
void demv1_clientdata (void);
void demv1_updateentity (void);
void demv1_dxentities (void);
void dzAddFolder (char *);
void dzCompressFile (char *, uInt, uInt);
void dzDeleteFiles  (uInt *, int, void (*)(uInt, uInt));
void dzExtractFile (uInt, int);
int dzRead (int);
int dzReadDirectory (char *);
void dzFile_Read (void *, uInt);
void dzFile_Write (void *, uInt);
uInt dzFile_Size (void);
void dzFile_Seek (uInt);
void dzFile_Truncate (void);
void dzWrite (void *, int);
void dzWriteDirectory (void);
void *Dzip_malloc (uInt);
void *Dzip_realloc (void *, uInt);
char *Dzip_strdup (const char *);
void end_zlib_compression (void);
void error (const char *, ...);
char *FileExtension (char *);
int get_filetype (char *);
char *GetFileFromPath (char *);
void Infile_Read (void *, uInt);
void Infile_Seek (uInt);
void Infile_Store (uInt);
void insert_msg (void *, size_t); // Baker uint to size_t
void make_crc (uchar *, int);
void normal_compress (uInt);
void Outfile_Write (void *, uInt);

#define pakid *(int *)"PACK"
#define discard_msg(x) inptr += x

#ifndef SFXVAR
	#define SFXVAR extern
#endif // !SFXVAR

extern uchar dem_updateframe;
SFXVAR uchar copybaseline;
SFXVAR int maxent, lastent, sble;
extern int maj_ver, min_ver;	/* of the current dz file */
#define p_blocksize 32768
extern int numfiles;
extern uInt totalsize;
SFXVAR int entlink[MAX_ENT];
SFXVAR long dem_gametime;
SFXVAR long outlen;
SFXVAR long cam0, cam1, cam2;
SFXVAR uchar *inblk, *outblk, *inptr;
extern uchar *tmpblk;
extern char AbortOp;
extern unsigned long crcval;
SFXVAR cdata_t oldcd, newcd;
SFXVAR ent_t base[MAX_ENT], oldent[MAX_ENT], newent[MAX_ENT];
extern direntry_t *directory;

#ifndef BIG_ENDIAN

	#define getshort(x) (*(short*)(x))
	#define getlong(x) (*(long*)(x))
	#define getfloat(x) (*(float*)(x))
	#define cnvlong(x) (x)

#else // ...

	short getshort (uchar *);
	long getlong (uchar *);
	float getfloat (uchar *);
	#define cnvlong(x) getlong((uchar*)(&x))

#endif // BIG_ENDIAN

#define Z_BUFFER_SIZE 16384
extern z_stream zs;
extern uchar *zbuf;
extern uInt ztotal;
extern int zlevel;

#ifdef GUI

	void GuiProgressMsg(const char *, ...);
	#define printf
	#define fprintf
	#define fflush

#endif // GUI

#ifdef _WIN32
	#define DIRCHAR '\\'
	#define WRONGCHAR '/'
	#define strcasecmp stricmp
#else
	#define DIRCHAR '/'
	#define WRONGCHAR '\\'
#endif // _WIN32

// Baker: Addition



#ifdef QUAKE_GAME
static void UnusedX (const void *x, ...) {}
	#define Q_exit_return return  
	#define Q_exit					// Turn this into a host error or something
	#define Q_vfprintf UnusedX
	#define Q_fprintf UnusedX
	#define Q_fflush UnusedX
	#define Q_printf UnusedX
#else
	#define Q_exit exit
	#define Q_vfprintf vfprintf
	#define Q_fprintf fprintf
	#define Q_fflush fflush
	#define Q_printf printf



#endif 




