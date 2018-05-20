/*
Copyright (C) 2011-2014 Baker

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
// pthreads_core.h -- pthreads

#ifndef __PTHREADS_CORE_H__
#define __PTHREADS_CORE_H__

// Note: #define CORE_PTHREADS to use this file (should probably do at project level)


#include "environment.h"
#include <pthread.h>
#ifdef PLATFORM_WINDOWS
	#ifdef __VISUAL_STUDIO_6__
		#pragma comment (lib, "pthreadVC2.lib")
	#else
	//	#ifdef _DEBUG
	//		#pragma comment (lib, "pthreadVCd2.lib")
	//	#else
			#pragma comment (lib, "pthreadVC2.lib")
//		#endif
	#endif
#endif

#endif // __PTHREADS_CORE_H__


//
// #include <pthread.h>
// #pragma comment (lib, "pthreadVC2.lib")
// static build for WIN32: define

// VC6:    ../SDK/pthreads-w32-2-9-1-release/include
// VS2008: ../SDK/pthread-static-msvc2010/include			(Visual Studio 2008 ... note: debug build available!)
//         PTW32_STATIC_LIB --- only works for Visual Studio 2008, probably should only use for release builds.
// pthreadVC2.lib and pthreadVCd2.lib (<--- debug lib)












