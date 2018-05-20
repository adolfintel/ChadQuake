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
// interface.h -- platform interface

#ifndef __INTERFACE_H__
#define __INTERFACE_H__


//typedef void* sys_handle_t;  // Clang says multiple def

///////////////////////////////////////////////////////////////////////////////
//  URL QUERY
///////////////////////////////////////////////////////////////////////////////

const char * File_Binary_URL (void); // to executable
const char * Folder_Binary_Folder_URL (void); // folder executable resides in
const char * Folder_Caches_URL (void); // uses global appname

///////////////////////////////////////////////////////////////////////////////
//  OS AND FOLDER INTERACTION: Baker
///////////////////////////////////////////////////////////////////////////////

cbool Folder_Open (const char *path_to_file);
cbool Folder_Open_Highlight (const char *path_to_file);
cbool Folder_Open_Highlight_Binary (void);


///////////////////////////////////////////////////////////////////////////////
//  CLIPBOARD -- Baker
///////////////////////////////////////////////////////////////////////////////

unsigned *Clipboard_Get_Image_Alloc (int *outwidth, int *outheight); // RGBA
char *Clipboard_Get_Text_Line (void);
char* Clipboard_Get_Text_Alloc (void);
cbool Clipboard_Set_Image (unsigned *rgba, int width, int height);
cbool Clipboard_Set_Text (const char * text_to_clipboard);




#endif // ! __INTERFACE_H__






