// cmdlib.c

#include "cmdlib.h"
#include <sys/types.h>
#include <sys/timeb.h>
#include "bsp5.h"


#ifdef WIN32
#include <direct.h>

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include "windows.h"
#endif

#ifdef NeXT
#include <libc.h>
#endif

#define PATHSEPERATOR   '/'

char *copystring(char *s)
{
	char	*b;
	b = AllocOther(strlen(s)+1);
	strcpy (b, s);
	return b;
}



/*
================
I_FloatTime
================
*/
double I_FloatTime (void)
{
	struct _timeb timebuffer;

	_ftime(&timebuffer);

	return (double)timebuffer.time + (timebuffer.millitm / 1000.0);
}

/*
=============================================================================

						MISC FUNCTIONS

=============================================================================
*/

/*
================
filelength
================
*/
int filelength (FILE *f)
{
	int pos;
	int end;

	if (!f)
		return 0;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}


void SafeSeek (FILE *f, char *filename, long pos)
{
	if (!f)
		return;

	if (pos > filelength (f) || fseek (f, pos, SEEK_SET) != 0)
		Message (MSGERR, "File seek failure to %d in %s", pos, filename);
}


FILE *SafeOpen (char *filename, char *Mode, qboolean Abort, char **SaveName)
{
	FILE *f;

	f = fopen(filename, Mode);

	if (!f && Abort)
		Message (MSGERR, "Error opening %s: %s", filename, strerror(errno));

	if (f && SaveName != NULL)
		*SaveName = copystring (filename);

	return f;
}


void SafeClose (FILE *f, char *filename)
{
	if (!f)
		return;

	fclose (f);

	if (filename != NULL)
		FreeOther (filename);
}


void SafeFlush (FILE *f)
{
	if (!f)
		return;

	fflush (f);
}


void SafeRead (FILE *f, char *filename, void *buffer, int count)
{
	if (!f)
		return;

	if ( fread (buffer, 1, count, f) != (size_t)count)
		Message (MSGERR, "File read failure in %s", filename);
}


void SafeWrite (FILE *f, char *filename, void *buffer, int count)
{
	if (!f)
		return;

	if (fwrite (buffer, 1, count, f) != (size_t)count)
		Message (MSGERR, "File write failure in %s", filename);
}

void SafePrintf (FILE *f, char *filename, char *Format, ...)
{
	va_list argptr;

	if (!f)
		return;

	va_start (argptr, Format);

	if (vfprintf (f, Format, argptr) < 0)
		Message (MSGERR, "File write failure in %s", filename);

	va_end (argptr);
}


/*
==============
LoadFile
==============
*/
int    LoadFile (char *filename, void **bufferptr)
{
	FILE	*f;
	int    length;
	void    *buffer;

	f = SafeOpen (filename, "rb", true, NULL);
	length = filelength (f);
	buffer = AllocOther (length+1);
	((char *)buffer)[length] = 0;
	SafeRead (f, filename, buffer, length);
	SafeClose (f, NULL);

	*bufferptr = buffer;
	return length;
}


/*
==============
SaveFile
==============
*/
void	SaveFile (char *filename, void *buffer, int count)
{
	FILE	*f;

	f = SafeOpen (filename, "wb", true, NULL);
	SafeWrite (f, filename, buffer, count);
	SafeClose (f, NULL);
}



void DefaultExtension (char *path, char *extension)
{
	char    *src;
//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != PATHSEPERATOR && *src != '\\' && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	strcat (path, extension);
}

void	StripExtension (char *path)
{
	int             length;

	length = strlen(path)-1;
	while (length > 0 && path[length] != '.')
	{
		length--;
		if (path[length] == PATHSEPERATOR || path[length] == '\\')
			return;		// no extension
	}
	if (length)
		path[length] = 0;
}

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

#ifdef _SGI_SOURCE
#define	__BIG_ENDIAN__
#endif

#ifdef __BIG_ENDIAN__

short	tx_LittleShort (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short	tx_BigShort (short l)
{
	return l;
}


int    tx_LittleLong (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int    tx_BigLong (int l)
{
	return l;
}


float	tx_LittleFloat (float l)
{
	union {byte b[4]; float f;} in, out;

	in.f = l;
	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];

	return out.f;
}

float	tx_BigFloat (float l)
{
	return l;
}


#else


short	tx_BigShort (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

int    tx_BigLong (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

float	tx_BigFloat (float l)
{
	union {byte b[4]; float f;} in, out;

	in.f = l;
	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];

	return out.f;
}
#endif

void SetQPriority(int Priority)
#ifdef WIN32
{
	if (Priority == 0)
		Priority = THREAD_PRIORITY_BELOW_NORMAL;
	else if (Priority == 1)
		Priority = THREAD_PRIORITY_NORMAL;
	else if (Priority == 2)
		Priority = THREAD_PRIORITY_ABOVE_NORMAL;
	else
		Message (MSGERR, "Priority must be 0-2");

	SetThreadPriority(GetCurrentThread(), Priority);
}
#else
{
}
#endif

/*
============
ShowBar
============
*/

void ShowBar(int Current, int Total)

{
	double	      Time;
	int	      New;
	static int    Prev = -1;
	static double PrevTime = 0;

	if (Total == 0)
	{
		if (!options.NoPercent)
			Prev = 0; // Enable

		return;
	}

	if (Prev == -1)
		return; // Disabled

	if (Total == -1)
	{
		if (Current == 0)
		{
			// Disrupted
			if (Prev > 0)
			{
				printf("\\\n");

				// Make sure bar will be repainted next time
				Prev = 0;
				PrevTime = -1;
			}

			return;
		}

		// Finish and disable

		if (options.NumPercent)
			printf("\r%3d%%\n", 100);
		else
		{
			// Finish bar
			while (Prev < 9)
				printf("%c", ++Prev == 5 ? '+' : '-');

			printf("+\n");
		}

		fflush(stdout);

		Prev = -1;
		PrevTime = 0;

		return;
	}

	if (options.NumPercent)
	{
		// Update every 20th percent or once each second
		New = Current * 100 / Total;

		if (New <= Prev || New > 99)
			return;

		Time = I_FloatTime();

		if (New / 20 == Prev / 20 && Time < PrevTime + 1)
			return;

		Prev = New;
		PrevTime = Time;
		printf("\r%3d%%", New);
	}
	else
	{
		// Update every 10th percent
		New = Current * 10 / Total;

		if (New <= Prev || New > 9)
			return;

		// Make sure bar reaches current value
		do
			printf("%c", ++Prev == 5 ? '+' : '-');
		while (Prev < New);
	}

	fflush(stdout);
}
