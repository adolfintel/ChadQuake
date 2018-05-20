/*
Copyright (C) 2012-2014 Baker

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// system.c -- totally neutralized system functions

#define CORE_LOCAL
#define FILE_LOCAL

#include "core.h"

#include "file.h"
#include "stringlib.h"

#include <stdio.h> // fopen, etc.
#include <stdlib.h> // malloc, etc.


int msgbox (const char *_title, const char *fmt, ...)
{
	const char *title = _title ? _title : "Alert";
	// We are intentionally cutting this off at 1024 otherwise Windows is REAL slow with mega sized ones.
	// NO TOO SLOW WITH SUPERSIZED ONES: NVA_EXPAND_ALLOC (text, length, bufsiz, fmt);
	VA_EXPAND (text, MAX_MSGBOX_TEXT_SIZE_BECAUSE_UNLIMITED_IS_SLOW_2048 /* maxsize */, fmt);
	_Platform_MessageBox (title, text);
	// NO TOO SLOW WITH SUPERSIZED ONES: (text);
	return 0;
}

int alert (const char *fmt, ...)
{
	VA_EXPAND_ALLOC (text, length, bufsiz, fmt); // We are variably sized, but msgbox is 2048 sized because huge msgbox is slow as hell, at least on Windows
//	VA_EXPAND (text, SYSTEM_STRING_SIZE_1024, fmt);

	msgbox (NULL, "%s", text);
	free (text); // VA_EXPAND_ALLOC free
	return 0;
}




