#include "dzip.h"

void dem_nop(void)
{
	copy_msg(1);
}

void dem_disconnect(void)
{
	copy_msg(1);
}

void dem_updatestat(void)
{
	copy_msg(6);
}

void dem_version(void)
{
	copy_msg(5);
}

void dem_setview(void)
{
	copy_msg(3);
}

void dem_sound(void)
{
	uchar mask = inptr[1];
	int len = 11;
	uInt chanent, c, entity;

	if (mask & 0x01) len++;
	if (mask & 0x02) len++;

	inptr[1] = 0x38+mask;
	chanent = getshort(inptr+len-9) & 0xffff;
	entity = chanent >> 3;
	chanent = (entity << 3) | ((2-(chanent & 0x07)) & 0x07);

	if (entity < MAX_ENT)
	{
		c = getshort(inptr+len-6) - oldent[entity].org0;
		c = cnvlong(c); memcpy(inptr+len-6,&c,2);
		c = getshort(inptr+len-4) - oldent[entity].org1;
		c = cnvlong(c); memcpy(inptr+len-4,&c,2);
		c = getshort(inptr+len-2) - oldent[entity].org2;
		c = cnvlong(c); memcpy(inptr+len-2,&c,2);
	}

	chanent = cnvlong(chanent);
	memcpy(inptr+len-9,&chanent,2);
	discard_msg(1);
	copy_msg(len-1);
}

void dem_time(void)
{
	long tmp = getlong(inptr+1) - dem_gametime;
	dem_gametime = getlong(inptr+1);
	tmp = cnvlong(tmp);
	memcpy(inptr+1,&tmp,4);
	if (inptr[3] || inptr[4]) { *inptr = DZ_longtime; copy_msg(5); }
	else { copy_msg(3); discard_msg(2); }
}

/* used by lots of msgs */
void dem_string(void)
{
	uchar *ptr = inptr + 1;
	while (*ptr++);
	copy_msg(ptr-inptr);
}

void dem_setangle(void)
{
	copy_msg(4);
}

void dem_serverinfo(void)
{
	uchar *ptr = inptr + 7;
	uchar *start_ptr;

	while (*ptr++);
	do {
		start_ptr = ptr;
		while (*ptr++);
	} while (ptr - start_ptr > 1);
	do {
		start_ptr = ptr;
		while (*ptr++);
	} while (ptr - start_ptr > 1);
	copy_msg(ptr-inptr);
	sble = 0;
}

void dem_lightstyle(void)
{
	uchar *ptr = inptr + 2;
	while (*ptr++);
	copy_msg(ptr-inptr);
}

void dem_updatename(void)
{
	uchar *ptr = inptr + 2;
	while (*ptr++);
	copy_msg(ptr-inptr);
}

void dem_updatefrags(void)
{
	copy_msg(4);
}

uchar bdiff(int x, int y)
{
	int d = x - y;
	if (d < 0) d += 256;
	return d & 0xff;
}

void dem_clientdata(void)
{
	uchar buf[32];
	uchar *ptr = inptr+3;
	uInt mask = getshort(inptr+1);
	long tmp;

	memset(&newcd,0,sizeof(cdata_t));
	newcd.voz = 22;
	newcd.items = 0x4001;

	#define CFDIFF(x,def,bit) if (newcd.x == def) newcd.force |= bit;

	if (mask & 0x0001) { newcd.voz = *ptr++; CFDIFF(voz,22,0x0800); }
	if (mask & 0x0002) { newcd.pax = *ptr++; CFDIFF(pax,0,0x1000); }
	if (mask & 0x0004) { newcd.ang0 = *ptr++; CFDIFF(ang0,0,0x0100); }
	if (mask & 0x0020) { newcd.vel0 = *ptr++; CFDIFF(vel0,0,1); }
	if (mask & 0x0008) { newcd.ang1 = *ptr++; CFDIFF(ang1,0,0x0200); }
	if (mask & 0x0040) { newcd.vel1 = *ptr++; CFDIFF(vel1,0,2); }
	if (mask & 0x0010) { newcd.ang2 = *ptr++; CFDIFF(ang2,0,0x0400); }
	if (mask & 0x0080) { newcd.vel2 = *ptr++; CFDIFF(vel2,0,4); }
	newcd.items = getlong(ptr); ptr += 4;
	newcd.invbit = !(mask & 0x0200);
	newcd.uk10 = !!(mask & 0x0400);
	newcd.uk11 = !!(mask & 0x0800);
	if (mask & 0x1000) { newcd.wpf = *ptr++; CFDIFF(wpf,0,0x2000); }
	if (mask & 0x2000) { newcd.av = *ptr++; CFDIFF(av,0,0x4000); }
	if (mask & 0x4000) { newcd.wpm = *ptr++; CFDIFF(wpm,0,0x8000); }
	newcd.health = getshort(ptr); ptr += 2;
	newcd.am = *ptr++;
	newcd.sh = *ptr++;
	newcd.nl = *ptr++;
	newcd.rk = *ptr++;
	newcd.ce = *ptr++;
	newcd.wp = *ptr++;
	discard_msg(ptr-inptr);

	mask = 0x00000040;
	ptr = buf+4;

	#define CDIFF(x,b) \
		if (oldcd.x != newcd.x) \
		{ mask |= b; *ptr++ = bdiff(newcd.x,oldcd.x); }

	CDIFF(vel2,0x00000001);
	CDIFF(vel0,0x00000002);
	CDIFF(vel1,0x00000004);
	CDIFF(wpf,0x00000100);
	if (oldcd.uk10 != newcd.uk10) mask |= 0x00000200;
	CDIFF(ang0,0x00000400);
	CDIFF(am,0x00000800);
	if (oldcd.health != newcd.health)
	{
		mask |= 0x00001000;
		tmp = newcd.health - oldcd.health;
		tmp = cnvlong(tmp);
		memcpy(ptr,&tmp,2); ptr += 2;
	}
	if (oldcd.items != newcd.items)
	{
		mask |= 0x00002000;
		tmp = newcd.items ^ oldcd.items;
		tmp = cnvlong(tmp);
		memcpy(ptr,&tmp,4); ptr += 4;		
	}
	CDIFF(av,0x00004000);
	CDIFF(pax,0x00010000);
	CDIFF(sh,0x00020000);
	CDIFF(nl,0x00040000);
	CDIFF(rk,0x00080000);
	CDIFF(wpm,0x00100000);
	CDIFF(wp,0x00200000);
	if (oldcd.uk11 != newcd.uk11) mask |= 0x00400000;
	CDIFF(voz,0x01000000);
	CDIFF(ce,0x02000000);
	CDIFF(ang1,0x04000000);
	CDIFF(ang2,0x08000000);
	if (oldcd.invbit != newcd.invbit) mask |= 0x10000000;

	if (mask & 0xffffff00) mask |= 0x08;
	if (mask & 0xffff0000) mask |= 0x8000;
	if (mask & 0xff000000) mask |= 0x800000;

	tmp = cnvlong(mask);
	memcpy(buf,&tmp,4);
	if (!(mask & 0x08))
		{ memcpy(buf+1,buf+4,ptr-buf-4); ptr -= 3; }
	else if (!(mask & 0x8000))
		{ memcpy(buf+2,buf+4,ptr-buf-4); ptr -= 2; }
	else if (!(mask & 0x800000))
		{ memcpy(buf+3,buf+4,ptr-buf-4); ptr -= 1; }
	insert_msg(buf,ptr-buf);

	if (newcd.force != oldcd.force)
	{
		mask = 0x50 | (newcd.force ^ oldcd.force);
		if (mask & 0xff00) mask |= 8;
		buf[0] = mask & 0xff;
		buf[1] = (mask >> 8) & 0xff;
		insert_msg(buf,1+!!buf[1]);
	}

	oldcd = newcd;
}

void dem_stopsound(void)
{
	copy_msg(3);
}

void dem_updatecolors(void)
{
	copy_msg(3);
}

void dem_particle(void)
{
	copy_msg(12);
}

void dem_damage(void)
{
	copy_msg(9);
}

void dem_spawnstatic(void)
{
	copy_msg(14);
}

void dem_spawnbinary(void)
{
	copy_msg(1);
}

void dem_spawnbaseline(void)
{
	uchar buf[16], *ptr;
	ent_t ent;
	int index = getshort(inptr+1);
	int diff;

	memset(&ent,0,sizeof(ent_t));
	ent.modelindex = inptr[3];
	ent.frame = inptr[4];
	ent.colormap = inptr[5];
	ent.skin = inptr[6];
	ent.org0 = getshort(inptr+7);
	ent.ang0 = inptr[9];
	ent.org1 = getshort(inptr+10);
	ent.ang1 = inptr[12];
	ent.org2 = getshort(inptr+13);
	ent.ang2 = inptr[15];
	discard_msg(16);

	buf[0] = DEM_spawnbaseline;
	diff = (index - sble + 0x400) % 0x400;
	buf[1] = diff & 0xff;
	buf[2] = diff >> 8;
	ptr = buf+3;
	*ptr++ = ent.modelindex;
	if (ent.frame) { buf[2] |= 0x04; *ptr++ = ent.frame; }
	if (ent.colormap) { buf[2] |= 0x08; *ptr++ = ent.colormap; }
	if (ent.skin) { buf[2] |= 0x10; *ptr++ = ent.skin; }
	if (ent.org0 || ent.org1 || ent.org2)
	{
		int tmp;
		buf[2] |= 0x20;
		tmp = ent.org0; tmp = cnvlong(tmp); memcpy(ptr,&tmp,2);
		tmp = ent.org1; tmp = cnvlong(tmp); memcpy(ptr+2,&tmp,2);
		tmp = ent.org2; tmp = cnvlong(tmp); memcpy(ptr+4,&tmp,2);
		ptr += 6;
	}
	if (ent.ang1) { buf[2] |= 0x40; *ptr++ = ent.ang1; }
	if (ent.ang0 || ent.ang2)
	{	buf[2] |= 0x80; *ptr++ = ent.ang0; *ptr++ = ent.ang2; }
	insert_msg(buf,ptr-buf);
	base[index] = ent;
	sble = index;
	copybaseline = 1;
}

const uchar te_size[] = {8, 8,  8,  8, 8, 16, 16, 8, 8, 16,
				  8, 8, 10, 16, 8,  8, 14};

void dem_temp_entity(void)
{
	uchar entitytype = inptr[1];
	if (entitytype >= 16)
	{
		directory[numfiles].type = TYPE_NEHAHRA;
		if (entitytype == 17)
		{
			copy_msg(strlen(inptr + 2) + 17);
			return;
		}
		if (entitytype > 17) /* this should be a bailout */
			error("entitytype is %d", entitytype);
	}
	copy_msg(te_size[entitytype]);
}

void dem_setpause(void)
{
	copy_msg(2);
}

void dem_signonnum(void)
{
	copy_msg(2);
}

void dem_killedmonster(void)
{
	copy_msg(1);
}

void dem_foundsecret(void)
{
	copy_msg(1);
}

void dem_spawnstaticsound(void)
{
	copy_msg(10);
}

void dem_intermission(void)
{
	copy_msg(1);
}

void dem_cdtrack(void)
{
	copy_msg(3);
}

void dem_sellscreen(void)
{
	copy_msg(1);
}

/* nehahra */
void dem_showlmp(void)
{
	uchar *ptr = inptr + 1;
	while (*ptr++);
	while (*ptr++);
	ptr += 2;
	*inptr = DZ_showlmp;	/* DEM_showlmp (35) is used by DZ_longtime */
	copy_msg(ptr-inptr);
	directory[numfiles].type = TYPE_NEHAHRA;
}

void dem_hidelmp(void)
{
	uchar *ptr = inptr + 1;
	while (*ptr++);
	copy_msg(ptr-inptr);
	directory[numfiles].type = TYPE_NEHAHRA;
}

void dem_skybox(void)
{
	uchar *ptr = inptr + 1;
	while (*ptr++);
	copy_msg(ptr-inptr);
	directory[numfiles].type = TYPE_NEHAHRA;
}
/* end nehahra */

void dem_copy_ue(void)
{
	uchar mask = inptr[0] & 0x7f;
	uchar topmask;
	int len = 1;

	topmask = (mask & 0x01)? inptr[len++] : 0x00;
	if (topmask & 0x40) len += 2; else len++;
	if (topmask & 0x04) len++;
	if (mask & 0x40) len++;
	if (topmask & 0x08) len++;
	if (topmask & 0x10) len++;
	if (topmask & 0x20) len++;
	if (mask & 0x02) len += 2;
	if (topmask & 0x01) len++;
	if (mask & 0x04) len += 2;
	if (mask & 0x10) len++;
	if (mask & 0x08) len += 2;
	if (topmask & 0x02) len++;
	if (topmask & 0x80) /* this should be a bailout */
		error("dem_copy_ue(): topmask & 0x80");
	copy_msg(len);
}

void diff_entities(void);

void dem_updateentity(void)
{
	uchar mask = *inptr & 0x7f;
	uchar topmask;
	short entity;
	static short lastentity;
	int len = 1;

	if (!dem_updateframe)
	{
		dem_updateframe = 1;
		lastentity = 0;
	}

	if (dem_updateframe == 2)
	{
		dem_copy_ue();
		return;
	}

	topmask = (mask & 0x01)? inptr[len++] : 0;
	if (topmask & 0x40) {
		entity = getshort(inptr+len);
		len += 2;
	} else {
		entity = inptr[len++];
	}

	if (entity <= lastentity)
	{
		diff_entities();
		dem_updateframe = 2;
		dem_copy_ue();
		return;
	}
	lastentity = entity;

	memcpy(newent + entity, base + entity, sizeof(ent_t));
	if (topmask & 0x40 && entity <= 0xff)
		newent[entity].force |= 0x400000;

	#define FDIFF(x,bit) if (newent[entity].x == base[entity].x) \
					newent[entity].force |= bit;
	if (topmask & 0x04)
	    { newent[entity].modelindex = inptr[len++];
	      FDIFF(modelindex,0x040000); }
	if (mask & 0x40)
	    { newent[entity].frame = inptr[len++]; FDIFF(frame,0x4000); }
	if (topmask & 0x08)
	    { newent[entity].colormap=inptr[len++]; FDIFF(colormap,0x080000); }
	if (topmask & 0x10)
	    { newent[entity].skin = inptr[len++]; FDIFF(skin,0x100000); }
	if (topmask & 0x20)
	    { newent[entity].effects = inptr[len++]; FDIFF(effects,0x200000); }
	if (mask & 0x02)
	    { newent[entity].org0 = getshort(inptr+len);
	      len += 2; FDIFF(org0,0x010000); }
	if (topmask & 0x01)
	    { newent[entity].ang0 = inptr[len++]; FDIFF(ang0,0x0800); }
	if (mask & 0x04)
	    { newent[entity].org1 = getshort(inptr+len);
	      len += 2; FDIFF(org1,0x0400); }
	if (mask & 0x10)
	    { newent[entity].ang1 = inptr[len++]; FDIFF(ang1,0x1000); }
	if (mask & 0x08)
	    { newent[entity].org2 = getshort(inptr+len);
	      len += 2; FDIFF(org2,0x020000); }
	if (topmask & 0x02)
	    { newent[entity].ang2 = inptr[len++]; FDIFF(ang2,0x2000); }
/* nehahra */
	if (topmask & 0x80)
	{
		float tmp = getfloat(inptr + len);
		directory[numfiles].type = TYPE_NEHAHRA;
		if (tmp != 2 && tmp != 1) /* this should be a bailout */
			error("nehahra: tmp is %f\n", tmp);
		newent[entity].alpha = getfloat(inptr + len + 4);
		len += 8;
		if (tmp == 2)
		{
			newent[entity].fullbright = 1 + (int)getfloat(inptr + len);
			if (newent[entity].fullbright != 2 && newent[entity].fullbright != 1)
			/* this should be a bailout */
				error("nehahra: fullbright is %f\n", getfloat(inptr + len));
			len += 4;
		}
		else newent[entity].fullbright = 0;
		newent[entity].force |= 0x800000;
	}
	newent[entity].newbit = mask & 0x20;
	newent[entity].present = 1;

	newent[entity].od0 = newent[entity].org0 - oldent[entity].org0;
	newent[entity].od1 = newent[entity].org1 - oldent[entity].org1;
	newent[entity].od2 = newent[entity].org2 - oldent[entity].org2;

	while (entlink[lastent] <= entity) lastent = entlink[lastent];
	if (lastent < entity)
	{
		entlink[entity] = entlink[lastent];
		entlink[lastent] = entity;
	}

	discard_msg(len);
}

void (* const dem_message[])(void) = {
	dem_nop, dem_disconnect, dem_updatestat, dem_version,
	dem_setview, dem_sound, dem_time, dem_string, dem_string,
	dem_setangle, dem_serverinfo, dem_lightstyle, dem_updatename,
	dem_updatefrags, dem_clientdata, dem_stopsound, dem_updatecolors,
	dem_particle, dem_damage, dem_spawnstatic, dem_spawnbinary,
	dem_spawnbaseline, dem_temp_entity, dem_setpause, dem_signonnum,
	dem_string, dem_killedmonster, dem_foundsecret,
	dem_spawnstaticsound, dem_intermission, dem_string,
	dem_cdtrack, dem_sellscreen, dem_string,
	dem_showlmp, dem_hidelmp, dem_skybox	/* nehahra */
};

void update_activate(int i, int *baseval, uchar *buf)
{
	uchar *ptr = buf;
	while (i - *baseval >= 0xff) { *ptr++ = 0xff; *baseval += 0xfe; }
	*ptr++ = i - *baseval;
	insert_msg(buf,ptr-buf);
}

void diff_entities(void)
{
	uchar buf[32];
	uchar *ptr;
	long tmp;
	int i, prev, mask;
	int firstent;
	int baseval = 0;
	uchar *tptr = tmpblk;

	buf[0] = 0x30; insert_msg(buf,1);

	for (prev = 0, i = entlink[0]; i < MAX_ENT; i = entlink[i])
	{
		ent_t n = newent[i], o = oldent[i];

		if (!n.present)
		{
			*tptr++ = 0x80;
			if (!o.active) update_activate(i,&baseval,buf);
			oldent[i] = base[i];
			entlink[prev] = entlink[i];
			continue;
		}

		if (!o.present) update_activate(i,&baseval,buf);

		prev = i;
		ptr = tptr+4;
		mask = 0;
		
		if (o.od2 != n.od2)
		{
			int diff = n.od2 - o.od2;
			if (diff >= -128 && diff <= 127)
			{   if (diff < 0) diff += 256;
			    mask |= 0x000001; *ptr++ = diff;
			} else {
			    mask |= 0x000800; tmp = cnvlong(n.org2);
			    memcpy(ptr,&tmp,2); ptr += 2;
			}
		}
		if (o.od1 != n.od1)
		{
			int diff = n.od1 - o.od1;
			if (diff >= -128 && diff <= 127)
			{   if (diff < 0) diff += 256;
			    mask |= 0x000002; *ptr++ = diff;
			} else {
			    mask |= 0x000400; tmp = cnvlong(n.org1);
			    memcpy(ptr,&tmp,2); ptr += 2;
			}
		}
		if (o.od0 != n.od0)
		{
			int diff = n.od0 - o.od0;
			if (diff >= -128 && diff <= 127)
			{   if (diff < 0) diff += 256;
			    mask |= 0x000004; *ptr++ = diff;
			} else {
			    mask |= 0x000200; tmp = cnvlong(n.org0);
			    memcpy(ptr,&tmp,2); ptr += 2;
			}
		}
		if (o.ang0 != n.ang0)
			{ mask |= 0x000008; *ptr++ = bdiff(n.ang0,o.ang0); }
		if (o.ang1 != n.ang1)
			{ mask |= 0x000010; *ptr++ = bdiff(n.ang1,o.ang1); }
		if (o.ang2 != n.ang2)
			{ mask |= 0x000020; *ptr++ = bdiff(n.ang2,o.ang2); }
		if (o.frame != n.frame)
			{ if (n.frame - o.frame == 1) mask |= 0x000040;
			  else { mask |= 0x000100;
				 *ptr++ = bdiff(n.frame,o.frame); }
			}
		if (o.effects != n.effects)
			{ mask |= 0x001000; *ptr++ = n.effects; }
		if (o.modelindex != n.modelindex)
			{ mask |= 0x002000; *ptr++ = n.modelindex; }
		if (o.newbit != n.newbit) mask |= 0x004000;
		if (o.colormap != n.colormap)
			{ mask |= 0x010000; *ptr++ = n.colormap; }
		if (o.skin != n.skin)
			{ mask |= 0x020000; *ptr++ = n.skin; }

	/* nehahra */
		if (n.alpha != o.alpha)
		{
			float ftmp;
			ftmp = getfloat((uchar *)&n.alpha);
			memcpy(ptr, &ftmp, 4);
			ptr += 4;
			mask |= 0x040000;
		}
		if (n.fullbright != o.fullbright)
			{ mask |= 0x080000;	*ptr++ = n.fullbright; }

		if ((mask & 0xffff00) && !(mask & 0x0000ff))
		{
			mask |= 0x01;
			tptr[3] = 0;
		}
		else
		{
			memcpy(tptr+3,tptr+4,ptr-tptr-4);
			ptr--;
		}
		if (mask & 0xffff00) mask |= 0x80;
		if (mask & 0xff0000) mask |= 0x8000;

		if (!mask)
		{
			if (o.present && !o.active) continue;
			*tptr++ = 0x00;
			continue;
		}

		newent[i].active = 1;
		if (!o.active && o.present) update_activate(i,&baseval,buf);

		tmp = cnvlong(mask);
		memcpy(tptr,&tmp,3);

		if (!(mask & 0xffff00))
			{ memcpy(tptr+1,tptr+3,ptr-tptr-3); ptr -= 2; }
		else if (!(mask & 0xff0000))
			{ memcpy(tptr+2,tptr+3,ptr-tptr-3); ptr--; }
		tptr = ptr;
	}

	buf[0] = 0; insert_msg(buf,1);
	insert_msg(tmpblk,tptr-tmpblk);

	firstent = -1;
	for (i = entlink[0]; i < MAX_ENT; i = entlink[i])
	{
		if (oldent[i].force == newent[i].force) continue;
		if (firstent < 0)
		{
			firstent = i;
			buf[0] = 0x31;
			insert_msg(buf,1);
		}
		mask = i | (oldent[i].force ^ newent[i].force);
		if (mask & 0xff0000) mask |= 0x8000;
		buf[0] = mask & 0xff;
		buf[1] = (mask >> 8) & 0xff;
		buf[2] = (mask >> 16) & 0xff;
		insert_msg(buf,2+!!buf[2]);
	}

	if (firstent > 0)
	{
		buf[0] = buf[1] = 0;
		insert_msg(buf,2);
	}

	for (i = entlink[0]; i < MAX_ENT; i = entlink[i])
	{
		oldent[i] = newent[i];
		newent[i].present = 0;
	}
}

void dem_compress (uInt start, uInt stop)
{
	unsigned int inlen, pos, crc_cheat;
	int cfields, clen;
	char cdstring[12];
	uchar *ptr;
	long a1,a2,a3,o1,o2,o3;

	#define bail(s) { fprintf(stderr,"\nwarning: %s\n",s); goto bailout; }

	/* 12 is the max length of a real quake-demo's cd string */
	if (stop - start > 13)
	for (clen = 0; clen < 12; clen++)
	{
		Infile_Read(cdstring + clen, 1);
		if (cdstring[clen] == '\n')
		{
			dzWrite(cdstring, clen + 1);
			goto tryit;
		}
		else if (cdstring[clen] == '-')
			/* ok */;
		else if (cdstring[clen] < '0' || cdstring[clen] > '9')
			break;
	}
	/* failed the test */
	directory[numfiles].type = TYPE_NORMAL;
	Infile_Seek(start);
	crcval = INITCRC;
	normal_compress(stop - start);
	return;
	
tryit:
	memset(&base,0,sizeof(ent_t)*MAX_ENT);
	memset(&oldent,0,sizeof(ent_t)*MAX_ENT);
	memset(&oldcd,0,sizeof(cdata_t));
	oldcd.voz = 22;
	oldcd.items = 0x4001;
	entlink[0] = MAX_ENT;
	o1 = o2 = o3 = 0;
	copybaseline = 0;
	dem_gametime = 0;
	sble = 0;

	for (pos = start + clen + 1; pos < stop; pos += 4 + inlen)
	{
		cfields = 0, clen = 0;
		crc_cheat = crcval;
		if (pos + 4 > stop) bail("unexpected EOF");
		Infile_Read(&inlen,4);
		inlen = cnvlong(inlen) + 12;
		if (inlen > p_blocksize / 2) bail("block too long");
		if (pos + inlen + 4 > stop) bail("unexpected EOF");
		Infile_Read(inblk, inlen);
		outlen = 1;
		ptr = inptr = inblk;
		a1 = getlong(inptr);
		a2 = getlong(inptr + 4);
		a3 = getlong(inptr + 8);
		o1 = a1 - o1; o1 = cnvlong(o1);
		o2 = a2 - o2; o2 = cnvlong(o2);
		o3 = a3 - o3; o3 = cnvlong(o3);
		if (o1) { cfields |= 1; memcpy(ptr,&o1,4); ptr += 4; }
		if (o2) { cfields |= 2; memcpy(ptr,&o2,4); ptr += 4; }
		if (o3) { cfields |= 4; memcpy(ptr,&o3,4); ptr += 4; }

		o1 = a1; o2 = a2; o3 = a3; clen = ptr - inptr;
		copy_msg(clen); discard_msg(12-clen);
		
		dem_updateframe = lastent = 0;
		while (inptr < inblk + inlen)
		{
			if (*inptr && *inptr <= DEM_skybox)
			{
				if (dem_updateframe == 1)
				{
					diff_entities();
					dem_updateframe = 2;
				}
				dem_message[(int)(*inptr - 1)]();
			}
			else if (*inptr & 0x80) dem_updateentity();
			else bail("bad message encountered");
		}
		if (dem_updateframe == 1) diff_entities();

		if (inptr > inblk + inlen) bail("parse error");
		if (outlen > p_blocksize / 2) bail("block too large in output");
		*outblk = cfields;
		cfields = 0; insert_msg(&cfields,1);
		dzWrite(outblk,outlen);
		if (copybaseline)
		{
			memcpy(oldent,base,sizeof(ent_t)*MAX_ENT);
			copybaseline = 0;
		}
		if (AbortOp)
			return;
	}

	return;

bailout:
	Infile_Seek(pos);
	inlen = stop - pos;
	crcval = crc_cheat;

	while (inlen && !AbortOp)
	{
		int toread = (inlen > p_blocksize-5)? p_blocksize-5 : inlen;
		toread = cnvlong(toread);
		*inblk = 0xff;
		memcpy(inblk+1,&toread,4);
		toread = cnvlong(toread);
		Infile_Read(inblk+5,toread);
		dzWrite(inblk,toread+5);
		inlen -= toread;
	}
}
