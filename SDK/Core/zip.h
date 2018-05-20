/*
Copyright (C) 2013-2014 Baker

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
// zip.h -- pak file header

#ifndef __ZIP_H__
#define __ZIP_H__


cbool Zip_Has_File (const char *zipfile_url, const char *filename);
void Zip_List_Print (const char * zipfile_url);
clist_t * Zip_List_Alloc (const char* zipfile_url);

cbool Zip_Add_File (const char *zipfile_url, const char *inside_pak_filename, const char *srcfile_url);
cbool Zip_Extract_File (const char *zipfile_url, const char *inside_pak_filename, const char *destfile_url);
int Zip_Unzip (const char *zipfile_url, const char *dest_folder_url);
int Zip_Zip_Folder (const char *zipfile_url, const char *source_folder_url);
cbool Zip_Remove_File (const char *zipfile_url, const char *inside_pak_filename);
cbool Zip_Rename_File (const char *zipfile_url, const char *inside_pak_filename, const char *new_name);
void Zip_Replace_File (const char *zipfile_url, const char *inside_pak_filename, const char *srcfile_url);

clist_t * Zip_List_Details_Alloc (const char *zipfile_url, const char *delimiter);

#endif // ! __ZIP_H__
