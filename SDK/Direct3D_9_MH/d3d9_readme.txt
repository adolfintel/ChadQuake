
NEW FAKE GL WRAPPER - MH 2016

Uses Direct3D 9 for an (almost) seamless replacement of OpenGL.  Right now it's just restricted to the GL Calls used by
Quake, but there's no reason why it can't be expanded if need be.

It is assumed that your OpenGL code is reasonably correct so far as OpenGL is concerned, and therefore a certain layer
of error checking and validation that *could* be included is actually omitted.  It is recommended that you test any
OpenGL code you write using native OpenGL calls before sending it through this wrapper in order to ensure this
correctness.

Wherever possible the OpenGL specifications at http://www.opengl.org were used for guidance and interpretation of
OpenGL calls.  The Direct3D 9 SDK help files and several sample aplications and other sources were used for the D3D
portion of the code.  Every effort has been made to ensure that my interpretation of things is conformant with
the published specs of each API.  It has been known for published specs to have bugs, and it has also been known
for my interpretation to be incorrect, however; so let me know if there is anything that needs fixing.

Compilation should be relatively seamless, although there are some changes you will need to make to your application
in order to get the best experience from it.  These include the following items (listed with a GLQuake-derived engine
in mind):

- define DIRECT3D9_WRAPPER somewhere in your code; I suggest either at the top of quakedef.h or glquake.h (preprocessor options
  in your project properties is also a good place).

- remove the opengl32.lib and glu32.lib library imports from your linker options

- replace the includes for gl.h and glu.h with something like the following:
	#ifndef DIRECT3D9_WRAPPER
	#include <gl/gl.h>
	#include <GL/glu.h>
	#pragma comment (lib, "opengl32.lib")
	#pragma comment (lib, "glu32.lib")
	#else
	#include "d3d9_publicapi.h"
	#endif

- Replace the SwapBuffers call in gl_vidnt.c with Direct3D9_SwapBuffers

- Replace the SetPixelFormat call in gl_vidnt.c with Direct3D9_SetPixelFormat

- Likewise, replace all calls to wglCreateContext, wglDeleteContext, wglGetCurrentContext, wglGetCurrentDC, wglMakeCurrent
  and wglGetProcAddress with Direct3D9_wglCreateContext, Direct3D9_wglDeleteContext, Direct3D9_wglGetCurrentContext, Direct3D9_wglGetCurrentDC,
  Direct3D9_wglMakeCurrent and Direct3D9_wglGetProcAddress.

- depending on your code you may also need to make certain other changes; see the examples given in sbar.c and
  gl_screen.h for an idea of the kind of thing you might need (just search for #ifdef DIRECT3D9_WRAPPER).

YOU WILL NEED THE DIRECTX 9 SDK (ANY VERSION) TO COMPILE THIS.

It should compile OK on Visual C++ 2008 (including Express) or better, but there are some steps to complete first.

- Get an unmodified GLQuake compiling and running; there are tutorials on inside3d and/or quakeone for this.

- Add the DirectX 9 SDK headers and libs to your Visual C++ directories.  I suggest you place them at the very top of the
  list, so that they take priority over other include or lib paths that may contain the same files.

For running, there is a dependency on the D3DX DLLs that were released in subsequent post-D3D9 updates, but which may not be
fully up to date on all Windows PCs.  As a general rule, if you have installed and run any other game that uses Direct3D 9,
then you probably have at least one of these DLLs installed too, and you can quite comfortably ignore the rest of this readme.

If you don't have them, download and run the latest DirectX redistributable from microsoft.com: THIS APPLIES EVEN IF YOU ALREADY
HAVE A MORE RECENT VERSION OF DIRECTX THAN VERSION 9; DirectX 10, 11 or 12 may not include these D3DX DLLs.  I don't recommend
grabbing these DLLs from random DLL download sites; the proper way is to get them from Microsoft, and you use any other site
entirely at your own risk.

To remove dependencies on specific versions of these DLLs, the wrapper will attempt to dynamically load D3DX, preferring to use
the most recent version it can find.  At the time of writing the matrix code still has a dependency on one of the D3DX headers
(even though it's not actually using any D3DX calls) but this will not affect actually running the thing.


OPTIONAL ENGINE FEATURES
------------------------

In the grand tradition of the original GLQuake a number of optional "fun" engine features were added.  These are clearly marked
with #defines in the code and have been tested with both the original GL build and with the D3D9 wrapper, so they should be
quite straightforward to port over to stock FitzQuake or any other Fitz-derived engine, without dependencies on the D3D9 wrapper.

Search the code for them to locate the necessary changes.

#define DYNAMIC_LIGHT_OPTIMIZATION
Batch-uploads all modified lightmaps at the end of a frame, rather than doing lots of tiny uploads.  This was mainly implemented
for my own testing purposes and so that the poor lightmap handling of stock Fitz wouldn't colour my performance metrics.  It is
the simplistic/naive variant that lags one frame behind; like I said: testing and metrics rather than correctness was the goal here.

#define UNDERWATER_WARP
Performs a software-Quake-like underwater warp.  Looking reasonably similar was considered more important than being identical.
There's plenty of room for tuning and cvar-izing hard-coded values in this.

#define DIRECTINPUT_VERSION
#define DIRECTSOUND_VERSION
These two specify using the old DirectX 3 interfaces for DirectInput and DirectSound, despite using Direct3D 9 for the renderer.
For maximum compatibility with the original code.

#define MLOOK_VIA_CVAR
Adds a freelook cvar (default 1) for mouselooking, like in Quake II.

#define DINPUT_MWHEEL
Enables the mousewheel when running with -dinput

#define DOWN_WITH_THIS_SORT_OF_THING
This is not defined by default and certain miscellaneous minor annoyances/etc that don't affect anything else are removed from the
engine.  Define it to put them back.

#define CALC_WIDESCREEN_FOV
Adjust FOV for widescreen aspects; this uses my preferred method (sourced from http://www.emsai.net/projects/widescreen/fovcalc/ and
http://www.gamedev.net/topic/431111-perspective-math-calculating-horisontal-fov-from-vertical/) which gives slightly different results
to that used by QuakeSpasm.

#define VIEW_TIMER_CORRECTION
Some of the time-based stuff in view.c is based on host_frametime; in certain situations the client may actually be running at a
different speed to that at which host_frametime advances (host_timescale, timedemo, paused, etc), so this basis is actually invalid.
This define corrects it to use (cl.time - cl.oldtime) instead.

#define CORRECT_DLIGHT_ORIGINS
Dynamic lights don't take moving brush models into account: if a brush model moves it's lit as though it were still at it's original
location.  This define corrects that (and despite the name also corrects for rotation).

#define SHADER_BASED_GAMMA
Enables shader-based gamma and contrast (stock Fitz doesn't have contrast so I just set it to 1).

#define D3D_RESIZABLE_WINDOW
Enables resizing of the game window in D3D windowed modes.

#define R_TRUE_LIGHTPOINT
Include burush models in lightpoint calcs so that if you stand on a plat, train, etc you pick up light from that bmodel rather than from the
underlying world geometry.

#define EXTRA_DLIGHTS
Corrects stock FitzQuake support for > 32 dlights, adds extra dlights to selected effects, adds colout to dlights.

