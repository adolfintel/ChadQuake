
//
// "FDDefines.h"
//
// Written by:	Axel 'awe' Wefers			[mailto:awe@fruitz-of-dojo.de].
//				©2001-2012 Fruitz Of Dojo 	[http://www.fruitz-of-dojo.de].
//


#define FD_SIZE_OF_ARRAY(array)             (sizeof (array) / sizeof (array[0]))



#define FD_MEMCPY(pDst, pSrc, numBytes)     __builtin_memcpy ((pDst), (pSrc), (numBytes))
#define FD_MEMSET(pDst, val, numBytes)      __builtin_memset ((pDst), (val), (numBytes))
#define FD_TRAP()                           __builtin_trap ()



#define FD_PRAGMA(literal)                  _Pragma (#literal)
#define FD_UNUSED(var,...)                  FD_PRAGMA(unused(var, ## __VA_ARGS__))



#if defined (__cplusplus)

#define FD_EXTERN                           extern "C"

#else

#define FD_EXTERN                           extern

#endif


