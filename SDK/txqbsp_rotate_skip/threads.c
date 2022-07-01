/*  Copyright (C) 1996-1997  Id Software, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.
*/


#include "cmdlib.h"
#include "threads.h"

#define	MAX_THREADS	64

int		dispatch;
int		workcount;
int		oldf;
qboolean		pacifier;

/*
=============
GetThreadWork

=============
*/
int	GetThreadWork (void)
{
	int	r;
	int	f;

	ThreadLock ();

	if (dispatch == workcount)
	{
		ThreadUnlock ();
		return -1;
	}

	f = 10*dispatch / workcount;
	if (f != oldf)
	{
		oldf = f;
		if (pacifier)
			printf ("%i...", f);
	}

	r = dispatch;
	dispatch++;
	ThreadUnlock ();

	return r;
}



/*
===================================================================

WIN32

===================================================================
*/
#ifdef WIN32

#define	USED

#include <windows.h>

int		numthreads = 1;
CRITICAL_SECTION		crit;

void ThreadLock (void)
{
	EnterCriticalSection (&crit);
}

void ThreadUnlock (void)
{
	LeaveCriticalSection (&crit);
}

/*
=============
RunThreadsOn
=============
*/
void RunThreadsOn (int workcnt, qboolean showpacifier, void(*func)(int))
{
	int		threadid[MAX_THREADS];
	HANDLE	threadhandle[MAX_THREADS];
	int		i;

	dispatch = 0;
	workcount = workcnt;
	oldf = -1;
	pacifier = showpacifier;

	//
	// run threads in parallel
	//
	InitializeCriticalSection (&crit);
	for (i=0 ; i<numthreads ; i++)
	{
		threadhandle[i] = CreateThread(
		   NULL,	// LPSECURITY_ATTRIBUTES lpsa,
		   0,		// DWORD cbStack,
		   (LPTHREAD_START_ROUTINE)func,	// LPTHREAD_START_ROUTINE lpStartAddr,
		   (LPVOID)i,	// LPVOID lpvThreadParm,
		   0,			//   DWORD fdwCreate,
		   &threadid[i]);
	}

	for (i=0 ; i<numthreads ; i++)
		WaitForSingleObject (threadhandle[i], INFINITE);
	DeleteCriticalSection (&crit);
	if (pacifier)
		printf ("\n");
}


#endif

/*
===================================================================

OSF1

===================================================================
*/

#ifdef __osf__
#define	USED

int		numthreads = 4;

#include <pthread.h>

pthread_mutex_t	*my_mutex;

void ThreadLock (void)
{
	if (my_mutex)
		pthread_mutex_lock (my_mutex);
}

void ThreadUnlock (void)
{
	if (my_mutex)
		pthread_mutex_unlock (my_mutex);
}


/*
=============
RunThreadsOn
=============
*/
void RunThreadsOn (int workcnt, qboolean showpacifier, void(*func)(int))
{
	int		i;
	pthread_t	work_threads[MAX_THREADS];
	pthread_addr_t	status;
	pthread_attr_t	attrib;
	pthread_mutexattr_t	mattrib;

	dispatch = 0;
	workcount = workcnt;
	oldf = -1;
	pacifier = showpacifier;

	if (!my_mutex)
	{
		my_mutex = malloc (sizeof(*my_mutex));
		if (pthread_mutexattr_create (&mattrib) == -1)
			Error ("pthread_mutex_attr_create failed");
		if (pthread_mutexattr_setkind_np (&mattrib, MUTEX_FAST_NP) == -1)
			Error ("pthread_mutexattr_setkind_np failed");
		if (pthread_mutex_init (my_mutex, mattrib) == -1)
			Error ("pthread_mutex_init failed");
	}

	if (pthread_attr_create (&attrib) == -1)
		Error ("pthread_attr_create failed");
	if (pthread_attr_setstacksize (&attrib, 0x100000) == -1)
		Error ("pthread_attr_setstacksize failed");
	
	for (i=0 ; i<numthreads ; i++)
	{
  		if (pthread_create(&work_threads[i], attrib
		, (pthread_startroutine_t)func, (pthread_addr_t)i) == -1)
			Error ("pthread_create failed");
	}
		
	for (i=0 ; i<numthreads ; i++)
	{
		if (pthread_join (work_threads[i], &status) == -1)
			Error ("pthread_join failed");
	}

	if (pacifier)
		printf ("\n");
}


#endif

/*
=======================================================================

  SINGLE THREAD

=======================================================================
*/

#ifndef USED

int		numthreads = 1;

void ThreadLock (void)
{
}

void ThreadUnlock (void)
{
}

/*
=============
RunThreadsOn
=============
*/
void RunThreadsOn (int workcnt, qboolean showpacifier, void(*func)(int))
{
	int		i;

	dispatch = 0;
	workcount = workcnt;
	oldf = -1;
	pacifier = showpacifier;

	func(0);

	if (pacifier)
		printf ("\n");
}

#endif
