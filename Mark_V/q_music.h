/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
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

#ifndef __CDAUDIO_H__
#define __CDAUDIO_H__

void CDAudio_Init(void);
void CDAudio_Play (byte track, cbool looping);
void CDAudio_Stop(void);
void CDAudio_Pause(void);
void CDAudio_Resume(void);
void CDAudio_Shutdown(void);
void CDAudio_Update(void);
#define MAX_MUSIC_MAPS_100 100

#ifdef SUPPORTS_MP3_MUSIC // Baker change
void MediaPlayer_Shutdown (void);
void MediaPlayer_Pause (void);
void MediaPlayer_Force_Off (void);
void MediaPlayer_Message (int looping);
void MediaPlayer_Update (void);
void MediaPlayer_Resume (void);
void MediaPlayer_Command_Line_Disabled (void);

cbool MediaPlayer_Play (int tracknum, cbool looping);
cbool MediaPlayer_Force_On (void);

//cmd void Set_Music_f (void);

extern char musicmaps[100][MAX_QPATH_64];
#endif // SUPPORTS_MP3_MUSIC Baker change +


#endif // ! __CDAUDIO_H__
