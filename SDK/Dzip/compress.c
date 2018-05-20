#include "dzip.h"

void end_zlib_compression(void)
{
	int r;
	do {
		zs.next_out = zbuf;
		zs.avail_out = Z_BUFFER_SIZE;
		r = deflate(&zs, Z_FINISH);
		dzFile_Write(zbuf,Z_BUFFER_SIZE - zs.avail_out);
		totalsize += Z_BUFFER_SIZE - zs.avail_out;
	} while (r != Z_STREAM_END);

	deflateEnd(&zs);
}

void startdirentry (char *name, uInt real, uInt type, uInt filetime)
{
	direntry_t *de;
	directory = Dzip_realloc(directory,(numfiles+1)*sizeof(direntry_t));
	de = directory + numfiles;
	de->ptr = totalsize;
	de->len = strlen(name)+1;
	de->pak = 0;
	de->real = real;
	de->type = type;
	de->date = filetime;
	de->name = Dzip_strdup(name);
	crcval = INITCRC;

	if (type != TYPE_PAK && type != TYPE_DIR)
		deflateInit(&zs, zlevel);	/* deflateInit won't fail with modified zlib */

	Q_fflush(stdout);
}

void finishdirentry(void)
{
	direntry_t *de = directory + numfiles;

	if (AbortOp)
	{
		dzFile_Seek(de->ptr);
		return;
	}

	if (de->type == TYPE_DIR)
		de->inter = de->size = de->crc = 0;
	else
	{
		end_zlib_compression();

		de->crc = crcval;
		de->inter = zs.total_in;
		de->size = zs.total_out;
	}

	if (de->size > de->real)
	{	/* store file if we got a negative ratio */
#ifdef GUI
		GuiProgressMsg("storing %s", de->name);
#endif
		Q_printf(": negative compression; storing at 0%%\n");
		dzFile_Seek(de->ptr);
		Infile_Store(de->real);
		totalsize = de->ptr + de->real;
		de->size = de->real;
		de->type = TYPE_STORE;
	}
	else if (de->type != TYPE_DIR)
		Q_printf(" (%2.1f%%)\n",100 - 100 * (double)de->size / (double)de->real);
	numfiles++;
}

void normal_compress (uInt filesize)
{
	uInt bsize;
	while (filesize && !AbortOp)
	{
		bsize = (filesize > p_blocksize) ? p_blocksize : filesize;
		Infile_Read(inblk, bsize);
		dzWrite(inblk, bsize);
		filesize -= bsize;
	}
}

void dzCompressFile (char *name, uInt filesize, uInt filedate)
{
	int filetype = get_filetype(name);

	if (filetype == TYPE_PAK)
	{
		uInt i, paksize;
		direntry_t *pakde;
		pakentry_t entry;
		struct {
			int id;
			uInt offset;
			uInt size;
		} pakheader;

		if (filesize < 12)	/* no way is it a pak! */
			goto notapak;
		Infile_Read(&pakheader, 12);
		if (pakheader.id != pakid)
		{
			Q_fprintf(stderr, "warning: %s is not a pak file\n", name);
			goto notapak;
		}
		pakheader.offset = cnvlong(pakheader.offset);
		pakheader.size = cnvlong(pakheader.size);

		if ((pakheader.offset + pakheader.size != filesize) 
			|| (pakheader.size & 0x3f))
		{
			Q_fprintf(stderr, "warning: %s has problems\n", name);
			goto notapak;
		}
		if (pakheader.size >= 65536 * 64)
		{
			Q_fprintf(stderr, "warning: too many files in %s\n", name);
			goto notapak;
		}
		Infile_Seek(pakheader.offset);

		Q_printf("processing pak file %s\n", name);
		paksize = 12 + pakheader.size;

		startdirentry(name, pakheader.offset + pakheader.size, filetype, filedate);
		pakheader.size /= 64;
		directory[numfiles].pak = (unsigned short)pakheader.size;
		numfiles++;

		for (i = 1; i <= pakheader.size; i++)
		{
			Infile_Read(&entry, 64);
			entry.ptr = cnvlong(entry.ptr);
			entry.len = cnvlong(entry.len);
			Infile_Seek(entry.ptr);
			filetype = get_filetype(entry.name);
			if (filetype == TYPE_PAK)
			{
				Q_fprintf(stderr, "warning: %s inside %s will be "
					"treated as a normal file\n",
					entry.name, directory[numfiles - i].name);
				filetype = TYPE_NORMAL;
			}
			startdirentry(entry.name, entry.len, filetype, filedate);
			directory[numfiles].pak = i;

		#ifdef GUI
			GuiProgressMsg("compressing %s [%s]", name, entry.name);
		#endif
			Q_printf(" compressing %s", entry.name);
			if (filetype == TYPE_DEM)
				dem_compress(entry.ptr, entry.len + entry.ptr);
			else
				normal_compress(entry.len);
			if (AbortOp)
			{	/* if a pakfile is aborted I need to remove all the
				   direntry_t that have been made so far */
				pakde = directory + numfiles;
				free(pakde->name);
				do 
				{
					pakde--;
					free(pakde->name);
					numfiles--;
				} while (pakde->type != TYPE_PAK);
				dzFile_Seek(pakde->ptr);
				return;
			}
			paksize += directory[numfiles].real;
			finishdirentry();
			Infile_Seek(pakheader.offset + 64 * i);
		}
		pakde = directory + numfiles - i;
		pakde->size = totalsize - pakde->ptr;
		pakde->real = paksize;
		return;
notapak:
		filetype = TYPE_NORMAL;
		Infile_Seek(0);
	}

	startdirentry(name, filesize, filetype, filedate);
	Q_printf("compressing %s", name);
	if (filetype == TYPE_DEM)
		dem_compress(0, filesize);
	else
		normal_compress(filesize);
	finishdirentry();
}

void dzAddFolder (char *name)
{
	startdirentry(name, 0, TYPE_DIR, 0);
	finishdirentry();
	Q_printf("added %s\n", name);
}