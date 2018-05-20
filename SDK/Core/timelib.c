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

#define CORE_LOCAL
#include "core.h"


///////////////////////////////////////////////////////////////////////////////
//  CORE: Time functions
///////////////////////////////////////////////////////////////////////////////

// Returns seconds since 1970
double Time_Now (void)
{
	time_t t1 = time (NULL);
	return (double) t1;
}

int Time_Minutes (int seconds)
{
	return seconds / 60;
}

int Time_Seconds (int seconds)
{
	return seconds %60;
}



void Time_Wait (double seconds)
{
	double end_time = Time_Now () + seconds;
	
	while (Time_Now() < end_time)
	{
		Platform_Sleep_Milliseconds (1);
	}
}

///////////////////////////////////////////////////////////////////////////////
//  CORE: Time functions
///////////////////////////////////////////////////////////////////////////////

int un_asctime(struct tm *timeptr, const char *buffer) 
{
	static const char wday_name[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static const char mon_name[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	char wday[4];
	char mon[4];
	int n;

	int cnt = sscanf(buffer, "%3s%3s%d%d:%d:%d%d%n", wday, mon,
		  &timeptr->tm_mday, &timeptr->tm_hour,
		  &timeptr->tm_min, &timeptr->tm_sec,
		  &timeptr->tm_year, &n);

	if (cnt != 7 || buffer[n + 1] != '\0' ) 
		return 1;

	timeptr->tm_year -= 1900;
	timeptr->tm_isdst = -1;  // unknown DST setting

	for (timeptr->tm_wday = 0; timeptr->tm_wday < 7; timeptr->tm_wday++) 
	{
		if (strcmp(wday, wday_name[timeptr->tm_wday]) == 0)
			break;
	}

	for (timeptr->tm_mon = 0; timeptr->tm_mon < 12; timeptr->tm_mon++) 
	{
		if (strcmp(mon, mon_name[timeptr->tm_mon]) == 0)
			break;
	}
	
	if (timeptr->tm_wday == 7 || timeptr->tm_mon == 12)
		return 1;
	
	timeptr->tm_yday = 0; // TBD
	return 0;
}

static char *Time_To_Buffer_ (double seconds_since_1970, char *buf, size_t bufsize, cbool doGMT)
{
	if (seconds_since_1970 < 0) {
		logd ("Invalid time < 0");
		return NULL;
	} else {
		time_t secs = seconds_since_1970; //  time_t is a long.  on 32 bit this is going to be very short. Year 2038 problem.
		struct tm *tm_info = doGMT ? gmtime (&secs) : localtime (&secs);
		int len;
			const char *_timestamp = asctime (tm_info); // asctime has static buffer, suffers from race conditions.
		strlcpy (buf, _timestamp, bufsize); 
		
		len = strlen(buf);
		if (len > 0 && buf[len - 1] == '\n')
			buf[len - 1] = 0; // Sigh.  Why is new line in there?  Sheesh?
	}

	return buf;
}

char *Time_To_Buffer (double seconds_since_1970, char *buf, size_t bufsize)
{
	return Time_To_Buffer_ (seconds_since_1970, buf, bufsize, false /* no gmt */);
}

char *Time_To_Buffer_GMT (double seconds_since_1970, char *buf, size_t bufsize)
{
	return Time_To_Buffer_ (seconds_since_1970, buf, bufsize, true /* wants gmt */);
}

char *Time_Now_To_String (void)
{
	static char buf[26];

	return Time_To_Buffer (Time_Now(), buf, sizeof(buf));
}

char *Time_Now_To_String_GMT (void)
{
	static char buf[26];

	return Time_To_Buffer_GMT (Time_Now(), buf, sizeof(buf));
}

char *Time_To_String (double seconds_since_1970)
{
	static char buf[26];

	return Time_To_Buffer (seconds_since_1970, buf, sizeof(buf));
}

char *Time_To_String_GMT (double seconds_since_1970)
{
	static char buf[26];

	return Time_To_Buffer_GMT (seconds_since_1970, buf, sizeof(buf));
}

// Returns -1 on failure
double Time_String_To_Time (const char *s)
{
	struct tm  tm_info;
	
	int failed = un_asctime (&tm_info, s);

	if (failed)
		return -1;

	return mktime (&tm_info); // -1 is returned if failed.
}




