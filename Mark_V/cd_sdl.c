#ifdef CORE_SDL

/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2009-2014 Baker and others
Copyright (C) 2018-2022 Federico Dossena

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
// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

// cd_sdl.c

#include "quakedef.h"
#include "environment.h"

#ifdef PLATFORM_LINUX

#include "SDL2/SDL.h"
#include "SDL2/SDL_mixer.h"

Mix_Music *Music=NULL;

void CDAudio_Play (byte track, cbool looping)
{
    if(Music!=NULL){
        CDAudio_Stop();
    }
    char path[MAX_OSPATH];
    FILE* temp;
    sprintf(path,"%s/music/track%02d.mp3",com_gamedir,track);
    if(temp=fopen(path,"rb")){
        fclose(temp);
    }else{
        sprintf(path,"%s/id1/music/track%02d.mp3",com_basedir,track);
        if(temp=fopen(path,"rb")){
            fclose(temp);
        }else{
            return;
        }
    }
    Music=Mix_LoadMUS(path);
    if(Music!=NULL){
        Mix_PlayMusic(Music, looping?-1:1);
        CDAudio_Update();
    }
}


void CDAudio_Stop(void)
{
    if(Music!=NULL){
        Mix_HaltMusic();
        Mix_FreeMusic(Music);
        Music=NULL;
    }
}


void CDAudio_Resume(void)
{
    if(Music!=NULL){
        Mix_ResumeMusic();
    }
}

cvar_t *musicvol=NULL;
void CDAudio_Update(void)
{
    if(Music!=NULL){
        if(musicvol==NULL){
            musicvol=Cvar_Find("bgmvolume");
        }
        Mix_VolumeMusic((int)((musicvol->value/3)*128));
    }
}

cbool mixAudioInit=false;
void CDAudio_Init(void)
{
    if(!mixAudioInit){
        Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 );
        mixAudioInit=true;
    }

}


void CDAudio_Shutdown(void)
{
    if(Music!=NULL){
        CDAudio_Stop();
    }
    Mix_CloseAudio();
}


void CDAudio_Pause () {
    if(Music!=NULL){
        Mix_PauseMusic();
    }
}

void CD_f () {}


void external_music_toggle_f (cvar_t* var)
{
}

#else
void CDAudio_Play (byte track, cbool looping)
{
    printf("SDL CDAudio not implemented on this platform\n");
}


void CDAudio_Stop(void)
{
    printf("SDL CDAudio not implemented on this platform\n");
}


void CDAudio_Resume(void)
{
    printf("SDL CDAudio not implemented on this platform\n");
}

void CDAudio_Update(void)
{
    printf("SDL CDAudio not implemented on this platform\n");
}

void CDAudio_Init(void)
{
    printf("SDL CDAudio not implemented on this platform\n");
}


void CDAudio_Shutdown(void)
{
    printf("SDL CDAudio not implemented on this platform\n");
}


void CDAudio_Pause () {
    printf("SDL CDAudio not implemented on this platform\n");
}

void CD_f () {}


void external_music_toggle_f (cvar_t* var)
{
}
#endif // PLATFORM_LINUX

#endif // CORE_SDL
