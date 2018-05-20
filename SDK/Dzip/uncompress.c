#include "dzip.h"

void dzExtractFile (uInt filepos, int testing)
{
	int inlen, blocksize;
	uInt eofptr, readptr;
	direntry_t *de = directory + filepos;
	char demomode;
#ifdef GUI
	char *action = testing ? "testing" : "extracting";
#else
	char *action = testing ? "checking" : "extracting";
#endif

	static pakentry_t *pakdir;
	static uInt pakptr;

	crcval = INITCRC;
	dzFile_Seek(de->ptr);
	
	if (de->type == TYPE_PAK)
	{
		struct {
			int id;
			uInt offset;
			uInt size;
		} pakheader;
		int i;
		
		Q_printf("%s %s:\n", action, de->name);
		pakdir = Dzip_malloc(de->pak * sizeof(pakentry_t));
		memset(pakdir, 0, de->pak * sizeof(pakentry_t));
		pakptr = 12;
	/* write pak header */
		pakheader.id = pakid;
		pakheader.size = de->pak * sizeof(pakentry_t);
		pakheader.offset = de->real - pakheader.size;
		Outfile_Write(&pakheader, 12);
		for (i = 0; i < de->pak; i++)
		{
			Q_printf(" %s", de[i + 1].name);
			dzExtractFile(filepos + i + 1, testing);
			if (AbortOp)
			{
				pakptr = 0;
				return;
			}
		}
		Outfile_Write(pakdir, de->pak * sizeof(pakentry_t));	
		free(pakdir);
		pakptr = 0;
		return;
	}

	if (de->type >= TYPE_LAST)
	{
		error("%s is unknown type of file: %d", de->name, de->type);
		return;
	}
	demomode = (de->type == TYPE_DEM || de->type == TYPE_NEHAHRA);

	if (pakptr)
	{
		strcpy(pakdir[de->pak - 1].name, de->name);
		pakdir[de->pak - 1].len = de->real;
		pakdir[de->pak - 1].ptr = pakptr;
		pakptr += de->real;
	}

#ifdef GUI
	if (pakptr || (testing && de->pak))
		GuiProgressMsg("%s %s [%s]", action, de[-de->pak].name, de->name);
	else
		GuiProgressMsg("%s %s", action, de->name);
#endif

	if (!de->pak)
		Q_printf("%s %s", action, de->name);

	if (de->type == TYPE_STORE)
	{
		totalsize = de->real;
		while (totalsize && !AbortOp)
		{
			blocksize = (totalsize > p_blocksize * 2) ? p_blocksize * 2 : totalsize;
			dzFile_Read(inblk, blocksize);
			Outfile_Write(inblk, blocksize);
			totalsize -= blocksize;
		}
	}
	else
	{
		inlen = readptr = totalsize = 0;
		eofptr = de->inter;
		ztotal = de->size;
		inflateInit(&zs);	/* cant possibly fail with my modified zlib */
		zs.avail_in = 0;

		if (demomode)
			dem_uncompress_init(de->type);
		
		while (readptr < eofptr && !AbortOp)
		{
			if (!dzRead(inlen))
				break;	/* corrupt compressed stream */

			if (demomode)
			{
				blocksize = dem_uncompress(eofptr - readptr);
				if (!blocksize)
					break;
			}
			else
			{
				blocksize = totalsize - readptr;
				if (totalsize >= eofptr)
					blocksize = eofptr - readptr;
				Outfile_Write(inblk, blocksize);
			}
			if (blocksize != p_blocksize)
				memcpy(inblk, inblk + blocksize, p_blocksize - blocksize);
			readptr += blocksize;
			inlen = p_blocksize - blocksize;
		}
		inflateEnd(&zs);
	}

	if (!AbortOp && crcval != de->crc) 
	{
#ifdef GUI
		error("CRC error in %s", de->name);
#else
		error(": CRC checksum error! Archive is broken!");
#endif
	} else if (testing) Q_printf(": ok\n");
	else Q_printf("\n");
}

#ifndef GUI

#include "dzipcon.h"

void setfiledate (char *filename, uInt date)
{
#ifdef WIN32
	FILETIME ft1, ft2;
	HANDLE h = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0, NULL);
	DosDateTimeToFileTime(HIWORD(date + (1 << 21)), LOWORD(date), &ft1);
	LocalFileTimeToFileTime(&ft1, &ft2);
	SetFileTime(h, NULL, NULL, &ft2);
	CloseHandle(h);
#else
	struct tm timerec;
	struct utimbuf tbuf;

	timerec.tm_hour = (date >> 11) & 0x1f;
	timerec.tm_min = (date >> 5) & 0x3f;
	timerec.tm_sec = (date & 0x1f) << 1;
	timerec.tm_mday = (date >> 16) & 0x1f;
	timerec.tm_mon = (date >> 21) & 0x0f;
	timerec.tm_year = ((date >> 25) & 0x7f) + 80;
	timerec.tm_isdst = -1;
	tbuf.actime = tbuf.modtime = mktime(&timerec);
	utime(filename,&tbuf);
#endif
}

void dzUncompress (char *src)
{
	direntry_t *de;
	int i;

	if (!dzOpen(src, 0))
		return;
	Q_printf("%s created using v%u.%u\n", src, maj_ver, min_ver);

	if (maj_ver == 1)
		dzUncompressV1(flag[SW_VERIFY]);
	else
	for (i = 0; i < numfiles; i += de->pak + 1)
	{
		de = directory + i;
		if (de->type == TYPE_DIR)
		{
			if (!flag[SW_VERIFY])
			{
				Q_printf("creating %s\n", de->name);
				CreateDir(de);
			}
			continue;
		}

		if (flag[SW_VERIFY])
		{
			dzExtractFile(i, 1);
			continue;
		}

		outfile = open_create(de->name);
		if (!outfile)
			continue;

		dzExtractFile(i, 0);
		fclose(outfile);
		if (AbortOp)
		{	/* problem writing file */
			remove(de->name);
			break;
		}

		setfiledate(de->name, de->date);
	}
	Q_printf("\n");
	dzClose();
}
#endif