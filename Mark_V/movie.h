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
// movie.h

#ifndef __MOVIE_H__
#define __MOVIE_H__

void Movie_Init (void);

#ifdef SUPPORTS_AVI_CAPTURE // Baker change

cbool Movie_GetSoundtime (void);
cbool Movie_IsActive (void);
void Movie_TransferStereo16 (void);
void Movie_StopPlayback (void);
void Movie_UpdateScreen (void);
double Movie_FrameTime (void);

// Platform
void AVI_LoadLibrary (void);
void ACM_LoadLibrary (void);
int Capture_Open (const char *filename, const char *usercodec, cbool silentish);
void Capture_WriteVideo (byte *pixel_buffer);
void Capture_WriteAudio (int samples, byte *sample_buffer);
void Capture_Close (void);

extern char movie_codec[12];

#endif // Baker change +



#endif // ! __MOVIE_H__