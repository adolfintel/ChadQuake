#include "dzip.h"
#include "dzipcon.h"

/* this file deals with uncompressing files made by
   version 1.x of Dzip and can be omitted without much harm! */

#define CPLUS(x,bit) if (mask & bit) { newcd.x = bplus(*ptr++,oldcd.x); }

void demv1_clientdata(void)
{
	uchar *ptr = inptr;
	int mask = *ptr++;

	if (mask & 0x01) mask += *ptr++ << 8;
	if (mask & 0x0100) mask += *ptr++ << 16;
	if (mask & 0x010000) mask += *ptr++ << 24;

	CPLUS(voz,0x01000000);
	CPLUS(pax,0x00100000);
	CPLUS(ang0,0x08000000);
	CPLUS(ang1,0x04000000);
	CPLUS(ang2,0x02000000);
	CPLUS(vel0,0x00000008);
	CPLUS(vel1,0x00000004);
	CPLUS(vel2,0x00000002);
	if (mask & 0x00008000) newcd.uk10 = !oldcd.uk10;
	if (mask & 0x00400000) newcd.uk11 = !oldcd.uk11;
	if (mask & 0x10000000) newcd.invbit = !oldcd.invbit;
	if (mask & 0x00200000)
	{
		newcd.items += getlong(ptr);
		ptr += 4;
	}
	CPLUS(wpf,0x00004000);
	CPLUS(av,0x00080000);
	CPLUS(wpm,0x00020000);
	if (mask & 0x00040000)
	{
		newcd.health += getshort(ptr);
		ptr += 2;
	}
	CPLUS(am,0x00002000);
	CPLUS(sh,0x00001000);
	CPLUS(nl,0x00000800);
	CPLUS(rk,0x00000400);
	CPLUS(ce,0x00800000);
	CPLUS(wp,0x00000200);

	discard_msg(ptr-inptr);

	if ((*ptr & 0xf0) == 0xe0)
	{
		mask = *ptr++;
		if (mask & 0x08) mask |= *ptr++ << 8;
		newcd.force ^= mask & 0xff07;
		discard_msg(ptr-inptr);
	}
	
	create_clientdata_msg();
}

void demv1_updateentity(void)
{
	uchar *ptr = inptr+1;
	uchar code = *inptr;
	int mask, entity;
	ent_t n, o;

	dem_updateframe = 1;

	if (code == 0x82) { discard_msg(1); return; }

	if (code == 0x83)
	{
		while ((entity = getshort(ptr)))
		{
			ptr += 2;
			memcpy(newent+entity,base+entity,sizeof(ent_t));
			memcpy(oldent+entity,base+entity,sizeof(ent_t));
		}
		discard_msg(ptr-inptr+2);
		return;
	}

	if (code == 0x84)
	{
		while ((mask = getshort(ptr)))
		{
			ptr += 2;
			mask &= 0xffff;
			if (mask & 0x8000) mask |= *ptr++ << 16;
			entity = mask & 0x3ff;
			if (entity > maxent) maxent = entity;
			newent[entity].force ^= mask & 0xfffc00;
		}
		discard_msg(ptr-inptr+2);
		return;
	}

	for (;;)
	{
		if (code == 0x81)
		{
			mask = (*ptr++ << 8) + 1;
			code = 0x80;
		}
		else
		{
			mask = getshort(ptr) & 0xffff;
			ptr += 2;
		}

		if (mask & 0x8000) mask += (*ptr++) << 16;
		if (mask & 0x800000) mask += (*ptr++) << 24;

		entity = mask & 0x1ff;
		if (mask & 0x08000000) entity += 0x200; 
		if (entity > maxent) maxent = entity;
		if (!entity) break;

		n = newent[entity];
		o = oldent[entity];
		n.present = 1;
		if (mask & 0x010000) n.modelindex = *ptr++;
		if (mask & 0x0200) n.frame = o.frame+1;
		if (mask & 0x080000) n.frame = bplus(*ptr++,o.frame);
		if (mask & 0x01000000) n.colormap = *ptr++;
		if (mask & 0x02000000) n.skin = *ptr++;
		if (mask & 0x04000000) n.effects = *ptr++;
		if (mask & 0x0400) n.org0 = bplus(*ptr++,o.org0);
		if (mask & 0x100000) { n.org0 = getshort(ptr); ptr += 2; }
		if (mask & 0x2000) n.ang0 = bplus(*ptr++,o.ang0);
		if (mask & 0x0800) n.org1 = bplus(*ptr++,o.org1);
		if (mask & 0x200000) { n.org1 = getshort(ptr); ptr += 2; }
		if (mask & 0x4000) n.ang1 = bplus(*ptr++,o.ang1);
		if (mask & 0x1000) n.org2 = bplus(*ptr++,o.org2);
		if (mask & 0x400000) { n.org2 = getshort(ptr); ptr += 2; }
		if (mask & 0x020000) n.ang2 = bplus(*ptr++,o.ang2);
		if (mask & 0x040000) n.newbit = !o.newbit;
		newent[entity] = n;
	}

	discard_msg(ptr-inptr);
}

void demv1_dxentities(void)
{
	uchar buf[32];
	uchar *ptr;
	long tmp;
	int i, mask;

	for (i = 1; i <= maxent; i++)
	{
		ent_t n = newent[i], b = base[i];

		if (!n.present) continue;
		ptr = buf+2;
		mask = 0x80;

		if (i > 0xff || (n.force & 0x400000))
		{
			tmp = cnvlong(i);
			memcpy(ptr,&tmp,2);
			ptr += 2;
			mask |= 0x4000;
		}
		else
			*ptr++ = i;

		#define BDIFF(x,bit,bit2) \
			if (n.x != b.x || n.force & bit2) \
				{ *ptr++ = n.x; mask |= bit; }

		BDIFF(modelindex,0x0400,0x040000);
		BDIFF(frame,0x0040,0x4000);
		BDIFF(colormap,0x0800,0x080000);
		BDIFF(skin,0x1000,0x100000);
		BDIFF(effects,0x2000,0x200000);
		if (n.org0 != b.org0 || n.force & 0x010000)
		    { mask |= 0x0002; tmp = cnvlong(n.org0); 
		      memcpy(ptr,&tmp,2); ptr += 2; }
		BDIFF(ang0,0x0100,0x0800);
		if (n.org1 != b.org1 || n.force & 0x0400)
		    { mask |= 0x0004; tmp = cnvlong(n.org1); 
		      memcpy(ptr,&tmp,2); ptr += 2; }
		BDIFF(ang1,0x0010,0x1000);
		if (n.org2 != b.org2 || n.force & 0x020000)
		    { mask |= 0x0008; tmp = cnvlong(n.org2); 
		      memcpy(ptr,&tmp,2); ptr += 2; }
		BDIFF(ang2,0x0200,0x2000);
		if (n.newbit) mask |= 0x20;

		if (mask & 0xff00) mask |= 0x01;
		buf[0] = mask & 0xff;
		buf[1] = (mask & 0xff00) >> 8;
		if (!(mask & 0x01)) { memcpy(buf+1,buf+2,ptr-buf-2); ptr--; }
		insert_msg(buf,ptr-buf);
		memcpy(oldent+i,newent+i,sizeof(ent_t));
	}

}

void dzUncompressV1 (int testing)
{
	int i, inlen = 0;
	uInt eofptr, blocksize, readptr = 0;
	char demomode;
	direntry_t *de;
	char *action = testing ? "checking" : "extracting";

	readptr = totalsize = 12;
	dzFile_Seek(12);
	inflateInit(&zs);	/* cant possibly fail with my modified zlib */
	zs.avail_in = 0;

	if (testing)
		outfile = NULL;

	for (i = 0; i < numfiles; i++)
	{
		de = directory + i;
		crcval = INITCRC;
		Q_printf("%s %s",action,de->name);
		Q_fflush(stdout);

		if (de->type == TYPE_DEMV1)
			demomode = 1;
		else if (de->type == TYPE_NORMAL || de->type == TYPE_TXT)
			demomode = 0;
		else
		{
			error("%s has invalid type %d", de->name, de->type);
			break;
		}

		if (!testing)
			outfile = open_create(de->name);

		if (demomode)
			dem_uncompress_init(de->type);

		eofptr = de->ptr + de->size;

		while (readptr < eofptr)
		{
			if (!dzRead(inlen))
				break;

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
				Outfile_Write(inblk,blocksize);
			}
			if (blocksize != p_blocksize)
				memcpy(inblk,inblk+blocksize,p_blocksize-blocksize);
			readptr += blocksize;
			inlen = p_blocksize - blocksize;
		}

		if (crcval != de->crc) 
			error("CRC checksum error! Archive is broken!");
		else if (testing)
			Q_printf(": ok\n");
		else
			Q_printf("\n");

		if (outfile)
			fclose(outfile);
	}
	inflateEnd(&zs);
}


