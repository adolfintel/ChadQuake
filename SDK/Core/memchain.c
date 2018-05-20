/*
Copyright (C) 2012-2014 Baker

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
// memchain.c -- memory chain for temp allocs

#include "core.h"
#include "memchain.h"

///////////////////////////////////////////////////////////////////////////////
//  LOCAL SETUP
///////////////////////////////////////////////////////////////////////////////

#define MLOCAL(_m) struct mlocal *_m = (struct mlocal *) me->_local

// Begin ...

static const char * const _tag = "memchain";  	// TAG
#define mobj_t memchain_t						// object type
#define mlocal _memchain_local					// private data struct

typedef struct mitem_s
{
__LIST_UNSORTED_ITEM_REQUIRED__ (struct mitem_s) // Supplies *prev, *next
	char   tag[32];
	size_t bytes;
	void   *data;
} mitem_t;

typedef struct memtrack_s
{
__LIST_UNSORTED_LIST_REQUIRED__ (struct mitem_s) // Supplies *first, *last
	int			count;
	int			bytes;
} memlist_t;

struct mlocal							// private data
{
	// Make this
	memlist_t	memlist;
};


///////////////////////////////////////////////////////////////////////////////
//  INTERNAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

static void sLink (memlist_t *memlist, mitem_t *item)
{
	List_Unsorted_Add (memlist, item);

	memlist->count ++;
	memlist->bytes += item->bytes;
}


static void sUnlink (memlist_t *memlist, mitem_t *item)
{
	List_Unsorted_Remove (memlist, item);

	memlist->count --;
	memlist->bytes -= item->bytes;
}


static mitem_t *sFindLink (memlist_t *memlist, const void *ptr)
{
	mitem_t *cur;

	for (cur = memlist->first; cur; cur = cur->next)
		if (cur->data == ptr)
			return cur;

	return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//  PUBLIC OBJECT FUNCTIONS:
///////////////////////////////////////////////////////////////////////////////

static void *Malloc (mobj_t *me, size_t len, const char *tag)
{
	MLOCAL(m);

	mitem_t *item = core_malloc (sizeof(mitem_t));
	c_strlcpy (item->tag, tag);
	item->bytes = len;
	item->data = core_malloc (len);


	sLink (&m->memlist, item);
	return item->data;
}

static void *Memdup (mobj_t *me, const void *src, size_t len, const char *tag)
{
	MLOCAL(m);

	mitem_t *item = core_malloc (sizeof(mitem_t));
	c_strlcpy (item->tag, tag);
	item->bytes = len;
	item->data = core_malloc (len);

	memcpy (item->data, src, len);
	sLink (&m->memlist, item);
	return item->data;
}


static void *Calloc (mobj_t *me, size_t len, size_t n, const char *tag)
{
	MLOCAL(m);

	mitem_t *item = core_malloc (sizeof(mitem_t));
	c_strlcpy (item->tag, tag);
	item->bytes = len * n;
	item->data = core_calloc (len, n);

	sLink (&m->memlist, item);
	return item->data;
}

static char *Strdup (mobj_t *me, const char *s, const char *tag)
{
	MLOCAL(m);

	mitem_t *item = core_malloc (sizeof(mitem_t));
	c_strlcpy (item->tag, tag);
	item->bytes = strlen (s) + 1;
	item->data = core_strdup (s);

	sLink (&m->memlist, item);
	return (char *)item->data;
}

static void *Free (mobj_t *me, const void *ptr, const char *hint)
{
	MLOCAL(m);

	if (ptr)
	{
		mitem_t *item = sFindLink (&m->memlist, ptr);

		if (!item)
		{
			log_fatal ("Missing Link");
			return NULL;
		}


		sUnlink (&m->memlist, item);

		core_free (item->data);
		core_free (item);
	}
	return NULL;
}

static void *Realloc (mobj_t *me, const void *ptr, size_t len, const char *tag)
{
	MLOCAL(m);

	mitem_t *item = sFindLink (&m->memlist, ptr);

	if (item == NULL)
		log_fatal ("Missing Link");

	sUnlink (&m->memlist, item);

	c_strlcpy (item->tag, tag);
	item->bytes = len;
	item->data = realloc (item->data, len);

	sLink (&m->memlist, item);

	return item->data;
}

static void Flush (mobj_t *me)
{
	MLOCAL(m);

	mitem_t *cur, *nextcur;

	for (cur = m->memlist.first; cur && TRUISM(nextcur = cur->next); cur = nextcur)
	{
		sUnlink (&m->memlist, cur);
		cur->data = core_free (cur->data);
		cur = core_free (cur);
	}
}

static cbool Initialize (mobj_t *me)
{
	MLOCAL(m);

	return true;
}


///////////////////////////////////////////////////////////////////////////////
//  PUBLIC GLOBAL FUNCTIONS:  Creation and freeing of object
///////////////////////////////////////////////////////////////////////////////


// Shutdown and Free
static void *Shutdown (mobj_t *me)
{
	MLOCAL(m);

#ifdef _DEBUG
	mitem_t *cur;

	// Warn of memory we didn't shutdown ourselves
	for (cur = m->memlist.first; cur; cur = cur->next)
		alert ("Mem remained %s", cur->tag);
#endif


	Flush (me);

	core_free (me->_local);
	core_free (me);

	return NULL;
}



memchain_t *Memchain_Instance (void)
{
	cbool result;

	mobj_t *nobj = core_calloc (sizeof(mobj_t), 1);
	nobj->_local = core_calloc (sizeof(struct mlocal), 1);

	// Hook up functions
	#define FUNCTION_HOOKUP(_x) nobj->_x = _x

	FUNCTION_HOOKUP(Malloc);
	FUNCTION_HOOKUP(Calloc);
	FUNCTION_HOOKUP(Strdup);
	FUNCTION_HOOKUP(Free);

	FUNCTION_HOOKUP(Realloc);
	FUNCTION_HOOKUP(Memdup);

	FUNCTION_HOOKUP(Flush);
	#undef FUNCTION_HOOKUP

	// Run initializer
	result = Initialize (nobj);

	if (!result)
	{
		nobj = Shutdown (nobj);
	}

	// Required information
	OBJ_REQUIRED_HOOKUP(nobj) // Sets parent, _tag, Shutdown

	return nobj;
}

#undef mobj_t
#undef mlocal

