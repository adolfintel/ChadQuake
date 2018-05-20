/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2009-2014 Baker and others

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

#include "quakedef.h"

#if 0
static int CL_WebDownloadProgress( double percent )
{
	static double time, oldtime, newtime;

	cls.download.percent = percent;
	
	if (sv.active || cls.demoplayback)
		CL_KeepaliveMessage();

	newtime = Sys_FloatTime ();
	time = newtime - oldtime;

	Host_Frame (time);

	oldtime = newtime;

	return cls.download.disconnect; // abort if disconnect received
}

#endif


