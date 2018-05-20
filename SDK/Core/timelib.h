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
// timelib.h -- time functions


#ifndef __TIMELIB_H__
#define __TIMELIB_H__

#include "environment.h"

// Need precise ticks time?  See: double Platform_MachineTime (void) // no set metric


///////////////////////////////////////////////////////////////////////////////
//  CORE: Time functions
///////////////////////////////////////////////////////////////////////////////

int Time_Minutes (int seconds); // Takes a number and returns the minutes.  For example 64 is 1 because 64 seconds is 1 minute.
int Time_Seconds (int seconds); // Takes a number and returns the seconds.  For example 64 = 4 because 64 is 1 minute and 4 seconds.

double Time_Now (void); // seconds precision time since 1970
void Time_Wait (double seconds); // Waits a certain number of seconds.  Not terrible, but why would we use it?  And Sleep on Windows is imprecise.
//void Time_Sleep_Milliseconds (unsigned long milliseconds); // Like above, I'm not seeing the value of this.
// Double self-initializes now
//double Time_Precise_Now (void); // no set metric
//#define Time_Now_Precise System_Time_Now_Precise

// Platform_MachineTime <-------------------------------

///////////////////////////////////////////////////////////////////////////////
//  CORE: Epoch Time
///////////////////////////////////////////////////////////////////////////////

char *Time_To_String (double seconds_since_1970); // static buffer versions
char *Time_To_String_GMT (double seconds_since_1970); 

char *Time_To_Buffer (double seconds_since_1970, char *buf, size_t bufsize); // buffer fill versions
char *Time_To_Buffer_GMT (double seconds_since_1970, char *buf, size_t bufsize);

double Time_String_To_Time (const char *s);

char *Time_Now_To_String (void);
char *Time_Now_To_String_GMT (void);

///////////////////////////////////////////////////////////////////////////////
//  CORE: time clock
///////////////////////////////////////////////////////////////////////////////




#endif	// ! __TIMELIB_H__


