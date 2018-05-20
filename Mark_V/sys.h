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

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

// sys.h -- non-portable functions



///////////////////////////////////////////////////////////////////////////////
//  SYSTEM IO
///////////////////////////////////////////////////////////////////////////////

#if id386
void System_MakeCodeWriteable (unsigned long startaddr, unsigned long len);
#endif // id386

///////////////////////////////////////////////////////////////////////////////
//  DEDICATED SERVER CONSOLE
///////////////////////////////////////////////////////////////////////////////

void Dedicated_Init (void);
int Dedicated_Printf (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2))); // send text to the console
int Dedicated_PrintLinef (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2))); // send text to the console
const char *Dedicated_ConsoleInput (void); // Console input

///////////////////////////////////////////////////////////////////////////////
//  SYSTEM ASM
///////////////////////////////////////////////////////////////////////////////

void System_MakeCodeWriteable (unsigned long startaddr, unsigned long length);

///////////////////////////////////////////////////////////////////////////////
//  SYSTEM OBSOLETE FUNCTIONS:  REMOVE THEM WHEN POSSIBLE
///////////////////////////////////////////////////////////////////////////////

// returns the file size or -1 if file is not present.
// the file should be in BINARY mode for stupid OSs that care
int System_FileOpenRead (const char *path, int *hndl);
int System_FileOpenWrite (const char *path);
void System_FileClose (int handle);
void System_FileSeek (int handle, int position);
int System_FileRead (int handle, void *dest, int count);
int System_FileWrite (int handle, const void *data, int count);


///////////////////////////////////////////////////////////////////////////////
//  SYSTEM
///////////////////////////////////////////////////////////////////////////////

void System_Init (void); // ASM
void System_Quit (void) __core_attribute__((__noreturn__));
int System_Error (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2), __noreturn__));
//int System_MessageBox (const char *_title, const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));

//double System_DoubleTime (void);
#define System_DoubleTime Platform_MachineTime 
void System_SleepUntilInput (int time);


void System_Process_Messages_Sleep_100 (void); // Baker: I use on Windows after vid_restart
void System_SendKeyEvents (void);  // Perform Key_Event () callbacks until the input que is empty

#endif	// ! __SYSTEM_H__


