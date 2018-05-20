/*
Copyright (C) 2009-2014 Baker and others

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
// music.h - mp3 player


#ifndef __MUSIC_H__
#define __MUSIC_H__


	typedef struct musicplayer_s
	{
	// required public function, must be top
		__OBJ_REQUIRED__

	// public functions
		void (*AttachVolume) (struct musicplayer_s *me, float *pvolume);
		cbool (*Play) (struct musicplayer_s *me, const char *path_to_file, cbool looping);
		void (*Update) (struct musicplayer_s *me);
		void (*Stop) (struct musicplayer_s *me);
		void (*Pause) (struct musicplayer_s *me);
		void (*Resume) (struct musicplayer_s *me);

	// public variables
		cbool			playing;
		cbool			paused;
		char			filename[MAX_OSPATH];
		cbool			looping;
		float			volumepct;
		float			*getvolumepct;

	// private members
		void*			_local;
	} musicplayer_t;

	musicplayer_t *MP3_Instance (void);
	musicplayer_t *CD_Instance (void);

#endif // __MUSIC_H__

