#include "dzip.h"
#include "dzipcon.h"

char flag[NUM_SWITCHES];
char AbortOp, *dzname;
uInt dzsize;
FILE *infile, *outfile, *dzfile;

/* read from file being compressed */
void Infile_Read (void *buf, uInt num)
{
	if (fread(buf, 1, num, infile) != num)
	{
		error(": error reading from input: %s", strerror(errno));
		AbortOp = 3;
	}
	make_crc(buf, num);
}

/* change file pointer in file being compressed (used for .pak) */
void Infile_Seek (uInt dest)
{
	fseek(infile, dest, SEEK_SET);
}

/* called when compression was unsuccessful (negative ratio) */
void Infile_Store (uInt size)
{
	void *buf = Dzip_malloc(32768);
	int s;

	Infile_Seek(ftell(infile) - size);
	while (size && !AbortOp)
	{
		s = (size > 32768) ? 32768 : size;
		Infile_Read(buf, s);
		dzFile_Write(buf, s);
		size -= s;
	}
	free(buf);
}

/* write to the file being extracted */
void Outfile_Write (void *buf, uInt num)
{
	if (!flag[SW_VERIFY])
	if (fwrite(buf, 1, num, outfile) != num)
	{
		error(": error writing to output: %s", strerror(errno));
		AbortOp = 1;
	}
	make_crc(buf, num);
}

/* read from dz */
void dzFile_Read (void *buf, uInt num)
{
	if (fread(buf, 1, num, dzfile) != num)
	{
		error(": error reading from %s: %s", dzname, strerror(errno));
		AbortOp = 1;
	}
}

/* write to dz */
void dzFile_Write (void *buf, uInt num)
{
	if (fwrite(buf, 1, num, dzfile) != num)
	{
		error(": error writing to %s: %s", dzname, strerror(errno));
		AbortOp = 1;
	}
}

/*	get filesize of dz, returns zero if it could not
	be determined, most likely because it was >= 4GB */
uInt dzFile_Size(void)
{	/* size was determined in DoFiles */
	return dzsize;
}

/* change file pointer of dz */
void dzFile_Seek (uInt dest)
{
#ifdef _MSC_VER
	/* do this so 2-4GB files will work for windows version */
	int hi = 0;
	fseek(dzfile, 0, SEEK_CUR); /* can't just seek out from under crt's nose */
	SetFilePointer(_get_osfhandle(fileno(dzfile)), dest, &hi, FILE_BEGIN);
#else
	fseek(dzfile, dest, SEEK_SET);
#endif
}

/* chop off end of dz; this is implementation dependent
   and may need adjusting for other platforms */
void dzFile_Truncate(void)
{
#ifdef _MSC_VER
	SetEndOfFile(_get_osfhandle(fileno(dzfile)));
#else
	ftruncate(fileno(dzfile), ftell(dzfile));
#endif
}

/* free directory and close current dz */
void dzClose(void)
{
	int i;
	for (i = 0; i < numfiles; free(directory[i++].name));
	free(directory);
	fclose(dzfile);
}

/* open a dz file, return 0 on failure */
int dzOpen (char *src, int needwrite)
{
	dzname = src;
	dzfile = fopen(src, needwrite ? "rb+" : "rb");
	if (!dzfile)
	{
		error(needwrite ? "could not open %s for writing: %s"
			: "could not open %s: %s", src, strerror(errno));
		return 0;
	}

	if (dzReadDirectory(src))
		return 1;

	fclose(dzfile);
	return 0;
}

FILE *open_create (char *src)
{
	FILE *f;

	if (!flag[SW_FORCE])
		if ((f = fopen(src, "r"))) 
		{
			error("%s exists; will not overwrite", src);
			return NULL;
		}

	f = fopen(src,"wb+");
	if (!f) error("could not create %s: %s",src, strerror(errno));
	return f;
}

/* safe set of allocation functions */
void *Dzip_malloc (uInt size)
{
	void *m = malloc(size);
	if (!m)	errorFatal("Out of memory!");
	return m;		
}

void *Dzip_realloc (void *ptr, uInt size)
{
	void *m = realloc(ptr, size);
	if (!m)	errorFatal("Out of memory!");
	return m;		
}

char *Dzip_strdup (const char *str)
{
	char *m = strdup(str);
	if (!m)	errorFatal("Out of memory!");
	return m;		
}

void *Dzip_calloc (uInt size)
{
	return memset(Dzip_malloc(size), 0, size);
}

#ifdef BIG_ENDIAN
/* byte swapping on big endian machines */
short getshort (uchar *c)
{
	return (c[1] << 8) + c[0];
}

long getlong (uchar *c)
{
	return (c[3] << 24) + (c[2] << 16) + (c[1] << 8) + c[0];
}

float getfloat (uchar *c)
{
	long l = getlong(c);
	return *(float*)(&l);
}

#endif

//#ifdef QUAKE_GAME
//#include <setjmp.h>
//#endif // QUAKE_GAME
void Q_ErrorOut (int errz)
{
#ifdef QUAKE_GAME
	extern void Host_Error (const char *error, ...);
//	extern jmp_buf host_dzfail;
//	longjmp (host_dzfail, 1);
	Host_Error ("Dzip couldn't open file.");
#else
	exit (1);
#endif
}

/* does not return if -e was specified on cmd line */
void error (const char *msg, ...)
{
	va_list	the_args;
	va_start(the_args,msg);
	if (flag[SW_HALT]) {
		if (*msg == ':')
		{
			Q_fprintf(stderr, "\nERROR: ");
			msg += 2;
		}
		else
			Q_fprintf(stderr,"ERROR: ");
	} /* stupid compilers bitch about ambigious else */

	vfprintf(stderr,msg,the_args);
	Q_fprintf(stderr,"\n");
	va_end(the_args);
	if (flag[SW_HALT]) // Baker: -e and we aren't using that so be lazy
		Q_ErrorOut (1); // exit (1)
}



/* does not return ever */
void errorFatal (const char *msg, ...)
{
	va_list	the_args;
	va_start(the_args,msg);
	Q_fprintf(stderr,"ERROR: ");
	Q_vfprintf(stderr,msg,the_args);
	Q_fprintf(stderr,"\n");

	va_end(the_args);
	Q_ErrorOut (1); // Q_exit(1);
}

struct { char *name; int flag; }
optname[] =
	{ { "-l", SW_LIST },
	  { "-x", SW_EXTRACT },
	  { "-v", SW_VERIFY },
	  { "-a", SW_ADD },
	  { "-d", SW_DELETE },
	  { "-f", SW_FORCE },
	  { "-e", SW_HALT },
	  { NULL, 0 }
	};

void parse_switch (char *arg)
{
	int i;

	if (strlen(arg) == 2 && arg[1] >= '0' && arg[1] <= '9')
	{
		zlevel = arg[1] - '0';
		return;
	}

	for (i = 0; optname[i].name; i++)
		if (!strcmp(optname[i].name,arg))
		{
			flag[optname[i].flag]++;
			return;
		}
	errorFatal("unknown switch %s",arg);
}

void usage(void)
{
    Q_fprintf(stderr,
	"Dzip v%u.%u: specializing in Quake demo compression\n\n"

	"Compression:         dzip <filenames> [-o <outputfile>]\n"
	"Add to existing dz:  dzip -a <dzfile> <filenames>\n"
	"Delete from a dz:    dzip -d <dzfile> <filenames>\n"
	"Decompression:       dzip -x <filenames>\n"
	"Verify dz file:      dzip -v <filenames>\n"
	"List dz file:        dzip -l <filenames>\n\n"

	"Options:\n"
	"  -0 to -9: set compression level\n"
	"  -e: quit program on first error\n"
	"  -f: overwrite existing files\n\n"

	"Copyright 2000-2002 Stefan Schwoon, Nolan Pflug\n",
	MAJOR_VERSION, MINOR_VERSION
    );

    Q_exit(1);
}

void DoFiles (char *fname, void (*func)(char *));

void DoDirectory (char *dname)
{
	char fullname[260], *ptr;

	if (!strcmp(dname, ".") || !strcmp(dname, ".."))
		return;
	ptr = &fullname[sprintf(fullname, "%s\\", dname)];
	dzAddFolder(fullname);

	/* scan directory, more system specific stuff */
	{
#ifdef WIN32
	HANDLE f;
	WIN32_FIND_DATA fd;

	*(short *)ptr = '*';
	f = FindFirstFile(fullname, &fd);
	if (f == INVALID_HANDLE_VALUE)
		return;	/* happens if user doesn't have browse permission */

	do {
		if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, ".."))
			continue;
		strcpy(ptr, fd.cFileName);
		DoFiles(fullname, NULL);
	} while (!AbortOp && FindNextFile(f, &fd));

	FindClose(f);
#else
	struct dirent *e;
	DIR *d = opendir(dname);
	if (!d) return;
	ptr[-1] = '/';

	while (!AbortOp && (e = readdir(d)))
	{
		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
			continue;
		strcpy(ptr, e->d_name);
		DoFiles(fullname, NULL);
	}

	closedir(d);
#endif
	}
}

void DoFiles (char *fname, void (*func)(char *))
{
	uInt filetime;

/* handle wildcards for Win32, assume that
   other systems will handle them for me! */
#ifdef WIN32
	HANDLE f;
	WIN32_FIND_DATA fd;
	char fullname[260];
	int p = GetFileFromPath(fname) - fname;

	f = FindFirstFile(fname, &fd);
	if (f == INVALID_HANDLE_VALUE)
	{	/* file not found, try adding .dem if no wildcards */
		if (!*FileExtension(fname) && !strpbrk(fname, "*?"))
		{
			strcat(fname, ".dem");
			f = FindFirstFile(fname, &fd);
			if (f == INVALID_HANDLE_VALUE)
				*FileExtension(fname) = 0;
		}
		if (f == INVALID_HANDLE_VALUE)
		{
			error("could not open %s", fname);
			return;
		}
	}

	do
	{
		strncpy(fullname, fname, p);
		strcpy(fullname + p, fd.cFileName);

		if (func)
		{	/* either dzList or dzUncompress */
			if (fd.nFileSizeHigh)
				dzsize = 0;
			else
				dzsize = fd.nFileSizeLow;
			func(fullname);
			continue;
		}

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			DoDirectory(fullname);
			continue;
		}

		/* cant add the current file to itself */
		if (!strcasecmp(fullname, dzname))
			continue;

		if (fd.nFileSizeHigh)
		{
			error("%s is 4GB or more and can't be compressed", fullname);
			return;
		}

		infile = fopen(fullname, "rb");
		if (!infile)
		{
			error("could not open %s: %s", fullname, strerror(errno));
			continue;
		}

		FileTimeToLocalFileTime(&fd.ftLastWriteTime, &fd.ftLastAccessTime);
		FileTimeToDosDateTime(&fd.ftLastAccessTime, (short *)&filetime + 1, (short *)&filetime);

		dzCompressFile(fullname, fd.nFileSizeLow, filetime - (1 << 21));
		fclose(infile);
		if (AbortOp == 3)	/* had a problem reading from infile */
			AbortOp = 0;	/* but still try the rest of the files */

	} while (!AbortOp && FindNextFile(f, &fd));
	FindClose(f);

#else

	struct stat64 filestats;
	struct tm *trec;

	if (stat64(fname, &filestats))
	{	/* file probably doesn't exist if this failed */
		error("could not open %s: %s", fname, strerror(errno));
		return;
	}

	if (func)
	{	/* either dzList or dzUncompress */
		if (filestats.st_size > 0xFFFFFFFF)
			dzsize = 0;
		else
			dzsize = filestats.st_size;
		func(fname);
		return;
	}

	if (filestats.st_mode & S_IFDIR)
	{
		DoDirectory(fname);
		return;
	}

	/* cant add the current file to itself */
	if (!strcasecmp(fname, dzname))
		return;

	if (filestats.st_size > 0xFFFFFFFF)
	{
		error("%s is 4GB or more and can't be compressed", fname);
		return;
	}

	infile = fopen(fname, "rb");
	if (!infile)
	{
		error("could not open %s: %s", fname, strerror(errno));
		return;
	}

	trec = localtime(&filestats.st_mtime);
	filetime = (trec->tm_sec >> 1) + (trec->tm_min << 5)
		+ (trec->tm_hour << 11) + (trec->tm_mday << 16)
		+ (trec->tm_mon << 21) + ((trec->tm_year - 80) << 25);

	dzCompressFile(fname, filestats.st_size, filetime);
	fclose(infile);
	if (AbortOp == 3)	/* had a problem reading from infile */
		AbortOp = 0;	/* but still try the rest of the files */

#endif
}

#ifndef QUAKE_GAME
int main(int argc, char **argv)
#else
// Outstanding issues.  
// 1) Use our proc to make the argc/argv
// 2) What about current directory?
// 3) need to remove exit (0) and replace with ... what does Host_Error do?  Maybe call hosterror?
// 4) Stuff that prints to console ... hmmm.  printf, stderr, error, etc.
// 5) String_Command_String_To_Argv (lpCmdLine, &host_parms.argc, pArgv, MAX_NUM_ARGVS);  See system_osx.m
int dzip_runner(int argc, char **argv)
#endif
{
	int i, j, oflag = 0;
	int fileargs = 0;
	char *optr = NULL;
	char **files;
	char *fname;

	if (argc < 2) usage();

	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i],"-o"))
			oflag = 1;
		else if (*argv[i] == '-')
			parse_switch(argv[i]);
		else if (oflag)
			optr = argv[i];
		else
			fileargs++;

	if (!fileargs) errorFatal("no input files");
	j = !!flag[SW_LIST] + !!flag[SW_EXTRACT]
		+ !!flag[SW_VERIFY] + !!flag[SW_ADD] + !!flag[SW_DELETE];
	if (j > 1) errorFatal("conflicting switches");
	if (oflag && j == 1) errorFatal("-o invalid with other switches");
	if (oflag && !optr) errorFatal("-o without argument");

	crc_init();
	inblk = Dzip_malloc(p_blocksize * 3);
	outblk = inblk + p_blocksize;
	tmpblk = outblk + p_blocksize / 2;
	zbuf = tmpblk + p_blocksize / 2;

	files = Dzip_malloc(fileargs * 4);
	for (i = 1, j = 0; i < argc; i++)
	{
		if (*argv[i] != '-') files[j++] = argv[i];
		if (!strcmp(argv[i],"-o")) i++;
	}

	zs.zalloc = Dzip_calloc;
	zs.zfree = free;

	if (flag[SW_LIST] || flag[SW_EXTRACT] || flag[SW_VERIFY])
	{
		void (*func)(char *) = flag[SW_LIST] ? dzList : dzUncompress;
		for (i = 0; i < fileargs && !AbortOp; i++)
		{
			fname = Dzip_malloc(strlen(files[i])+4);
			strcpy(fname,files[i]);
#ifdef WIN32 /* only add extension automatically on windows */
			if (!*FileExtension(fname))
				strcat(fname, ".dz");
#endif
			DoFiles(fname, func);
			free(fname);
		}
		Q_exit_return(0);
	}

	if (!flag[SW_ADD] && !flag[SW_DELETE])
	{	/* create a new dz and compress files */
		if (!optr)
		{
			optr = Dzip_malloc(strlen(files[0]) + 4);
			strcpy(optr, files[0]);
			strcpy(FileExtension(optr), ".dz");
		}
	#ifdef WIN32
		else if (!*FileExtension(optr))
		{
			char *t = Dzip_malloc(strlen(optr) + 4);
			sprintf(t, "%s.dz", optr);
			optr = t;
		}
	#endif
		dzname = optr;
		dzfile = open_create(optr);
		if (!dzfile) Q_exit_return(1);
		i = 'D' + ('Z' << 8) + (MAJOR_VERSION<<16) + (MINOR_VERSION<<24);
		i = cnvlong(i);
		dzFile_Write(&i, 12);
		totalsize = 12;

		directory = Dzip_malloc(sizeof(direntry_t));

		for (i = 0; i < fileargs; i++) 
		{
			fname = Dzip_malloc(strlen(files[i])+5);
			strcpy(fname,files[i]);
			DoFiles(fname, NULL);
			free(fname);
		}

		free(files);
		dzWriteDirectory();
		dzClose();
		if (!numfiles)
			remove(dzname);
		Q_exit_return(0);
	}

	/* add or delete */
	if (fileargs < 2)
		errorFatal("no input files");
	fname = Dzip_malloc(strlen(files[0])+4);
	strcpy(fname,files[0]);
#ifdef WIN32
	if (!*FileExtension(fname))
		strcat(fname, ".dz");
#endif
	dzsize = 0;
	/* figure out dzsize */
	{
#ifdef WIN32
	WIN32_FIND_DATA fd;

	FindClose(FindFirstFile(fname, &fd));
	if (!fd.nFileSizeHigh)
		dzsize = fd.nFileSizeLow;
#else
	struct stat64 filestats;

	if (!stat64(fname, &filestats))
		if (!(filestats.st_size > 0xFFFFFFFF))
			dzsize = filestats.st_size;
#endif
	}

	if (!dzOpen(fname, 1))	/* add & delete need write access */
		Q_exit_return(0);

	if (flag[SW_ADD])
	{
		dzFile_Seek(4);
		dzFile_Read(&totalsize, 4);
		dzFile_Seek(totalsize);
		for (i = 1; i < fileargs; i++) 
		{
			fname = Dzip_malloc(strlen(files[i])+5);
			strcpy(fname,files[i]);
			DoFiles(fname, NULL);
			free(fname);
		}
		dzWriteDirectory();
		dzClose();
	}
	else
		dzDeleteFiles_MakeList(files + 1, fileargs - 1);
	free(files);
	Q_exit_return(0);
}



#if 0
#include <stdio.h>
int main(int argc, const char * argv[])
{
	
	// insert code here...
	Q_printf("Hello, World!\n");
    return 0;
}
#endif

//#ifndef _WIN32
// Dec 2016 - apparently defining "DEBUG" activated this.
int z_verbose; // Baker: Hmmm.  Apparently Mac uses this function.

void z_error (char * msg)
{
	Q_fprintf (stderr, msg);
	Q_exit (1); // Failure
}
//#endif // _WIN32




