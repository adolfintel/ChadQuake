/* Demo compression program
   May/June 2000 by Stefan Schwoon
   (schwoon@in.tum.de)
   Basic dem parsing source
   January 1999 by Anthony Bailey
   (anthony@planetquake.com)
   GUI by Nolan Pflug
   (radix@planetquake.com)
*/

#include "dzip.h"

direntry_t *directory;
int numfiles;
uInt totalsize;

uchar *inblk, *outblk, *tmpblk, *inptr;
long outlen;

cdata_t oldcd, newcd;
ent_t base[MAX_ENT], oldent[MAX_ENT], newent[MAX_ENT];
int entlink[MAX_ENT];
uchar dem_updateframe, copybaseline;
long dem_gametime;
int maxent, lastent, sble;
int maj_ver, min_ver;

z_stream zs;
uchar *zbuf;
uInt ztotal;
int zlevel = Z_DEFAULT_COMPRESSION;

void dzWrite (void *ptr, int len)
{
	zs.next_in = ptr;
	zs.avail_in = len;

	while (zs.avail_in)
	{
		zs.next_out = zbuf;
		zs.avail_out = Z_BUFFER_SIZE;
		deflate(&zs, Z_NO_FLUSH);
		len = Z_BUFFER_SIZE - zs.avail_out;
		if (!len) continue;	/* avoid wasted write call of 0 bytes */
		dzFile_Write(zbuf, len);
		totalsize += len;
	}
}

int dzRead (int inlen)
{	/* inlen is the number of bytes that are already used in
	   inblk because they were moved back from the last block
	   which is only going to happen for quake demos */
	int toread, bsize;

	toread = p_blocksize - inlen;
	zs.next_out = inblk + inlen;
	zs.avail_out = toread;
	totalsize += toread;

	while (zs.avail_out)
	{
		if (!zs.avail_in && ztotal)
		{
			bsize = (ztotal > Z_BUFFER_SIZE) ? Z_BUFFER_SIZE : ztotal;
			ztotal -= bsize;
			dzFile_Read(zbuf, bsize);
			zs.next_in = zbuf;
			zs.avail_in = bsize;
		}
		bsize = inflate(&zs, Z_NO_FLUSH);
		if (bsize == Z_DATA_ERROR)
			return 0;
		if (bsize == Z_STREAM_END)
			return 1;
	}
	return 1;
}

int dzReadDirectoryEntry (direntry_t *de)
{
	char *s;
	int size = DE_DISK_SIZE;
	if (maj_ver == 1) size -= 8; 

	dzFile_Read(de, size);
	de->ptr = cnvlong(de->ptr);
	de->size = cnvlong(de->size);
	de->real = cnvlong(de->real);
	de->len = cnvlong(de->len) & 0xffff;
	if (de->len > 257)
		return 0;
	de->pak = cnvlong(de->pak) & 0xffff;
	de->crc = cnvlong(de->crc);
	de->type = cnvlong(de->type);
	de->date = cnvlong(de->date);
	de->inter = cnvlong(de->inter);

	if (de->type == TYPE_DEMV1 && maj_ver > 1)
		return 0;

	s = Dzip_malloc(de->len);
	dzFile_Read(s, de->len);
	de->name = s;
	if (de->pak && de->type != TYPE_PAK)
		return 1;	/* dont mess with dirchar inside pakfiles */
	do
		if (*s == WRONGCHAR) *s = DIRCHAR;
	while (*s++);
	return 1;
}

/* read in all the stuff from a dz file, return 0 if not valid */
int dzReadDirectory (char *dzname)
{
	int i;
	uInt fsize;
	struct {
		uInt id, offset, numfiles;
	} dzheader;

	fsize = dzFile_Size();
	if (fsize < 12)
	{
		error("%s is not a valid dz file", dzname);
		return 0;
	}

	dzFile_Read(&dzheader, 12);
	dzheader.id = cnvlong(dzheader.id);
	dzheader.offset = cnvlong(dzheader.offset);
	numfiles = cnvlong(dzheader.numfiles);

	if ((dzheader.id & 0xffff) != 'D' + ('Z' << 8))
	{
		error("%s is not a valid dz file", dzname);
		return 0;
	}
	maj_ver = (dzheader.id >> 16) & 0xff;
	min_ver = (dzheader.id >> 24) & 0xff;
	if (maj_ver > MAJOR_VERSION)
	{
		error("%s was compressed with version %u ",
			"but I'm only version %u", dzname,
			maj_ver, MAJOR_VERSION);
		return 0;
	}

	if (numfiles & 0xF8000000 || dzheader.offset >= fsize ||
		(uInt)numfiles * (DE_DISK_SIZE - 8 * (maj_ver == 1)) > fsize)
	{
		error("%s is corrupt: missing directory", dzname);
		return 0;
	}

	directory = Dzip_malloc(numfiles * sizeof(direntry_t));
	dzFile_Seek(dzheader.offset);
	
#ifndef GUI
	if (maj_ver == 1) ztotal = dzheader.offset - 12;
#endif
	for (i = 0; i < numfiles; i++)
		if (!dzReadDirectoryEntry(directory + i))
		{
			error("%s is corrupt: bad directory entry", dzname);
			return 0;
		}
	
	return 1;
}

void dzWriteDirectoryEntry (direntry_t *de)
{
	de->ptr = cnvlong(de->ptr);
	de->size = cnvlong(de->size);
	de->real = cnvlong(de->real);
	de->len = cnvlong(de->len) & 0xffff;
	de->pak = cnvlong(de->pak) & 0xffff;
	de->crc = cnvlong(de->crc);
	de->type = cnvlong(de->type);
	de->date = cnvlong(de->date);
	de->inter = cnvlong(de->inter);
	dzFile_Write(de, DE_DISK_SIZE);
	de->len = cnvlong(de->len);
	dzFile_Write(de->name, de->len);
}

void dzWriteDirectory(void)
{
	int i, j;

	j = directory[numfiles - 1].ptr + directory[numfiles - 1].size;
	dzFile_Seek(4);
	i = cnvlong(j); dzFile_Write(&i, 4);
	i = cnvlong(numfiles); dzFile_Write(&i, 4);
	dzFile_Seek(j);

	for (i = 0; i < numfiles; i++)
		dzWriteDirectoryEntry(directory+i);
}

void copy_msg (uInt i)
{
	memcpy(outblk + outlen, inptr, i);
	outlen += i;
	inptr += i;
}

void insert_msg (void *msg, size_t i)
{
	memcpy(outblk + outlen, msg, i);
	outlen += i;
}

int get_filetype (char *name)
{
	char *ext = FileExtension(name);
	if (!strcasecmp(ext, ".dem"))
		return TYPE_DEM;
	if (!strcasecmp(ext, ".pak"))
		return TYPE_PAK;
	return TYPE_NORMAL;
}

/* returns pointer to filename from a full path */
char *GetFileFromPath (char *in)
{
	char *x = in - 1;

	while (*++x)
		if (*x == '/' || *x == '\\')
			in = x + 1;
	return in;
}

/* returns a pointer directly to the period of the extension,
   or it there is none, to the nul character at the end */
char *FileExtension (char *in)
{
	char *e = in + strlen(in);

	in = GetFileFromPath(in);
	while ((in = strchr(in, '.')))
		e = in++;

	return e;
}