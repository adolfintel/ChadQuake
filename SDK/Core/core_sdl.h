
// core_sdl.h
//#pragma message("Document the precise instructions to build a new SDL version")
// 1) We need to detect the SDL library version.  Maybe.  Static build on Linux would probably solve.
// SDL_version compiled;
// SDL_version linked;
// SDL_VERSION(&compiled);
// SDL_GetVersion(&linked);
// printlinef ("We compiled against SDL version %d.%d.%d ...",
//       compiled.major, compiled.minor, compiled.patch);
// printlinef ("But we are linking against SDL version %d.%d.%d.",
//       linked.major, linked.minor, linked.patch);
//
// Windows.  Go to DropBox/SDK and unpack 2.0.4 to its own folder.  Edit the makeproj bundle paths.
//           Since this is built-in to makeproj, alter makeproj to have a $PATH_SDL_WINDOWS var? (easy ish)
//           Since this is built-in to makeproj, alter makeproj to have a $PATH_SDL_MINGW var?	(easy ish)
//           Since this is built-in to makeproj, alter makeproj to have a $PATH_SDL_IOS var?	(how do we recompile? where is the shit?)
//           Since this is built-in to makeproj, alter makeproj to have a $PATH_SDL_LINUX var?	 (how do we update in Ubuntu)
//                  sudo apt-get install libsdl2-2.0.4 ?
//                  sudo apt-get install libsdl2-dev
//           Since this is built-in to makeproj, alter makeproj to have a $PATH_SDL_ANDROID var?   (recompile)
//                  C:\Dropbox\SDK\SDL2-android-so-compile
//				    Looks like we go to C:\Dropbox\SDK\SDL2-android-so-compile\jni\SDL2
//						and just copy the headers into include and the src in jni\src
//           Since this is built-in to makeproj, alter makeproj to have a $PATH_SDL_MAC var?	(how does we macports install it)?
//                  MacPorts (<--S) will install library into /opt/.
//                  we copy it
//                  what is macport command line?  sudo port install sdl2 ?  Looks like: sudo port install libsdl2
//                  copy from opt into C:\Dropbox\SDK



#ifndef __CORE_SDL_H__
#define __CORE_SDL_H__

#ifdef _MSC_VER
	//#pragma warning( disable : 4142 ) // SDL likes to define int8_t, uintptr_t, etc.
	#pragma comment (lib, "sdl2.lib")
	#pragma comment (lib, "sdl2main.lib")
	#include <SDL2/SDL.h>
	

	//#include <core_windows.h> // LLWinKeyHook
#else
	// We are using /opt/local/include
	#ifdef PLATFORM_ANDROID
		#include <SDL.h>
	#else
		#include <SDL2/SDL.h>
	#include <SDL2/SDL_syswm.h>
	#endif
#endif

//  defined in core.h
//  typedef void (*plat_dispatch_fn_t) (SDL_Event *sdl_event); 

#endif // !__CORE_SDL_H__

