// in_null.c -- for systems without a mouse

#include "quakedef.h"

void IN_Init (void)
{
}

void IN_Shutdown (void)
{
}

void IN_Commands (void)
{
}

void IN_Move (usercmd_t *cmd)
{
}

/*
===========
IN_ModeChanged
===========
*/
void IN_ModeChanged (void)
{
}

void Input_Local_Init () {}

void Input_Local_Joystick_Commands () {}


void Input_Local_Deactivate () {}

void Input_Local_Shutdown () {}


void Input_Mouse_Move () {}

cbool Input_Local_Joystick_Startup () { return 0; }

void Input_Local_SendKeyEvents (void ) {}
float sInJoyValues[JOY_MAX_AXES];

void Utilities_Init (void) {}