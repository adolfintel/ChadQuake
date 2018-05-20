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
// memchain.h -- memory chain for temp allocs

#ifndef __MEMCHAIN_H__
#define __MEMCHAIN_H__

/*
A slightly different version is back in environment.h now
#define __OBJ_REQUIRED__			\
	const char * _cname;			\
	struct cobj_s * _parent;		\
	struct cobj_s * _child;			\
	void *(*Shutdown) (void *);
*/

typedef struct sysobj_s
{
	__OBJ_REQUIRED__ // The header.
} sysobj_t;

typedef struct memchain_s
{
// required public function, must be top
	__OBJ_REQUIRED__

// public functions
	void *(*Malloc) (struct memchain_s *me, size_t n, const char *tag);
	void *(*Calloc) (struct memchain_s *me, size_t len, size_t n, const char *tag);
	char *(*Strdup) (struct memchain_s *me, const char *s, const char *tag);
	void *(*Free)   (struct memchain_s *me, const void *ptr, const char *hint); // Ignores attempts to free NULL
	void *(*Realloc)(struct memchain_s *me, const void *ptr, size_t len, const char *tag);
	void *(*Memdup) (struct memchain_s *me, const void *src, size_t len, const char *tag);

	void (*Flush)	(struct memchain_s *me);

// public variables

// private members
	void*			_local;
} memchain_t;



memchain_t *Memchain_Instance (void);



// These are a delight ...
#define memfreenull(_var) _var = m->memchain->Free (m->memchain, _var, #_var)
#define memcalloc(_var,_size) _var = m->memchain->Calloc (m->memchain, _size, 1, #_var)
#define memdup(_var,_src,_size)  if (_var) _var = m->memchain->Free (m->memchain, _var, #_var); _var = m->memchain->Memdup (m->memchain, _src, _size, #_var)
#define memstrdupe(_var,_str)  if (_var) _var = m->memchain->Free (m->memchain, _var, #_var); _var = m->memchain->Strdup (m->memchain, _str ? _str : empty_string, #_var)

#define Mem_Shutdown(_var) (_var = _var->Shutdown(_var))
#define Mem_Initialize(_var) (_var = Memchain_Instance ())


#endif // ! __MEMCHAIN_H__



