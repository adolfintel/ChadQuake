/*
Copyright (C) 2009-2013 Baker

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
// recent_file.h

#ifndef __RECENT_FILE_H__
#define __RECENT_FILE_H__

void Recent_File_Init (void);
void Recent_File_NewGame (void); // Done on a gamedir change.
void Recent_File_Set_FullPath (const char *path_to_file); // An absolute path
void Recent_File_Set_QPath (const char *qpath_filename); // A qpath

const char *Recent_File_Get (void);

#endif // ! __RECENT_FILE_H__

