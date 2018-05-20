
//
// "macquake.h" - MacOS X Video driver
//
// Written by:	Axel 'awe' Wefers			[mailto:awe@fruitz-of-dojo.de].
//				©2001-2012 Fruitz Of Dojo 	[http://www.fruitz-of-dojo.de].
//
// Quakeª is copyrighted by id software	[http://www.idsoftware.com].
//


// Baker: The parallel of "winquake.h" but for OS X.  At least that is my plan.

#ifndef __MACQUAKE_H__
#define __MACQUAKE_H__

#import <FDFramework/FDFramework.h>
#import "QController.h"


////////////////////////////////////////////////////////////////////
// Actual shared
////////////////////////////////////////////////////////////////////

// General ...
typedef struct
{
	FDWindow *	gVidWindow;
	FDDisplay*  gVidDisplay;


} sysplat_t;

extern sysplat_t sysplat;



#endif // ! __MACQUAKE_H__

