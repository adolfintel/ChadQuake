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

#ifndef __TOOL_TEXTUREPOINTER_H__
#define __TOOL_TEXTUREPOINTER_H__

void TexturePointer_Init (void); // Registers command
void TexturePointer_Think (void); // Updates + Draws
void TexturePointer_Draw (void); // Draws 2D string
void TexturePointer_Reset (void);


void TexturePointer_Clear_Caches (void); // Clears temp replaced files (gamedir/texturepointer/mapname)


extern cbool texturepointer_on;

void TexturePointer_ClipboardCopy (void);	// CTRL-C
void TexturePointer_ClipboardPaste (void);	// CTRL-V




#endif	// ! __TOOL_TEXTUREPOINTER_H__