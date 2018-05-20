// cmdlib.h

#ifndef __CMDLIB__
#define __CMDLIB__

#pragma warning( disable : 4244 4305)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>


#ifndef __BYTEBOOL__
#define __BYTEBOOL__
typedef enum {false, true} qboolean;
typedef unsigned char byte;
#endif

// the dec offsetof macro doesn't work very well...
#define myoffsetof(type,identifier) ((size_t)&((type *)0)->identifier)


#define Q_strncasecmp strnicmp
#define Q_strcasecmp stricmp

int filelength (FILE *f);

double I_FloatTime (void);

void	TXError (char *error, ...);

FILE	*SafeOpen (char *filename, char *Mode, qboolean Abort, char **SaveName);
void	SafeClose (FILE *f, char *filename);
void	SafeFlush (FILE *f);
void	SafeSeek (FILE *f, char *filename, long pos);
void	SafeRead (FILE *f, char *filename, void *buffer, int count);
void	SafeWrite (FILE *f, char *filename, void *buffer, int count);
void	SafePrintf (FILE *f, char *filename, char *Format, ...);

int	LoadFile (char *filename, void **bufferptr);
void	SaveFile (char *filename, void *buffer, int count);

void	DefaultExtension (char *path, char *extension);
void	StripExtension (char *path);

int	ParseNum (char *str);

short	tx_BigShort (short l);
#define tx_LittleShort(a) a
int	tx_BigLong (int l);
#define tx_LittleLong(a) a
float	tx_BigFloat (float l);
#define tx_LittleFloat(a) a


char    *copystring(char *s);

void	SetQPriority(int Priority);
void	ShowBar(int Current, int Total);

#endif
