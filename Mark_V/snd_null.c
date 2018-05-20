/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// snd_null.c -- include this instead of all the other snd_* files to have
// no sound code whatsoever

#include "quakedef.h"


 
void S_Init (void)
{
}

void S_AmbientOff (void)
{
}

void S_AmbientOn (void)
{
}

void S_Shutdown (void)
{
}

void S_TouchSound (const char *sample)
{
}

void S_ClearBuffer (void)
{
}

void S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation)
{
}

void S_StartSound (int entnum, int entchannel, sfx_t *sfx, const vec3_t origin, float fvol,  float attenuation)
{
}

void S_StopSound (int entnum, int entchannel)
{
}

sfx_t *S_PrecacheSound (const char *sample)
{
	return NULL;
}

void S_ClearPrecache (void)
{
}

void S_Update (const vec3_t origin, const vec3_t v_forward, const vec3_t v_right, const vec3_t v_up)
{	
}

void S_StopAllSounds (cbool clear)
{
}

void S_BeginPrecaching (void)
{
}

void S_EndPrecaching (void)
{
}

void S_ExtraUpdate (void)
{
}

void S_LocalSound (const char *s)
{
}

void S_Play2_f (void)
{
	
}

void S_PlayVol(void)
{}

void S_BlockSound () {}

const char * S_Sound_ListExport () { return NULL; }

void S_SoundInfo_f () {}

void S_UnblockSound (void) {}

void Sound_Toggle_Mute_f() {}

void Neh_ResetSFX () {}


void S_SoundList () {}

void S_Play_f () {}

void Neh_Reset_Sfx_Count (void) {};

void S_StopAllSoundsC (void) {}



void S_Snd_Speed_Notify_f (cvar_t *var)
{
}

void external_music_toggle_f (cvar_t *var)
{
}



