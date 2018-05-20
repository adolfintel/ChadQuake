
//
// "FDDebug.h"
//
// Written by:	Axel 'awe' Wefers			[mailto:awe@fruitz-of-dojo.de].
//				©2001-2012 Fruitz Of Dojo 	[http://www.fruitz-of-dojo.de].
//


#import "FDDefines.h"
#import <Cocoa/Cocoa.h>



typedef BOOL (*FDDebugAssertHandler) (const char *pFile, unsigned int line, const char *pFormat, ...);
typedef void (*FDDebugErrorHandler) (const char *pFormat, ...);
typedef void (*FDDebugExceptionHandler) (const char *pFormat, ...);
typedef void (*FDDebugLogHandler) (const char *pFormat, ...);



@interface FDDebug : NSObject
{
}

+ (FDDebug*) sharedDebug;
+ (BOOL) isDebuggerAttached;

- (id) init;
- (id) initWithName: (NSString*) name;

- (void) setName: (NSString*) name;
- (NSString*) name;

- (void) setAssertHandler: (FDDebugAssertHandler) assertHandler;
- (void) setErrorHandler: (FDDebugErrorHandler) errorHandler;
- (void) setExceptionHandler: (FDDebugExceptionHandler) exceptionHandler;
- (void) setLogHandler: (FDDebugLogHandler) logHandler;

- (void) logWithFormat: (NSString*) format, ...;
- (void) logWithFormat: (NSString*) format arguments: (va_list) argList;

- (void) errorWithFormat: (NSString*) format, ...;
- (void) errorWithFormat: (NSString*) format arguments: (va_list) argList;

- (void) exception: (NSException*) exception;
- (void) exceptionWithFormat: (NSString*) format, ...;
- (void) exceptionWithFormat: (NSString*) format arguments: (va_list) argList;

- (BOOL) assert: (NSString*) file line: (NSUInteger) line format: (NSString*) format, ...;
- (BOOL) assert: (NSString*) file line: (NSUInteger) line format: (NSString*) format arguments: (va_list) argList;

@end



FD_EXTERN void    FDLog (NSString* format, ...);
FD_EXTERN void    FDError (NSString* format, ...);



#if defined (DEBUG)

#define FD_ASSERT(expr)         if (!(expr))                                                                            \
                                {                                                                                       \
                                    if (![[FDDebug sharedDebug] assert: @""__FILE__ line: __LINE__ format: @""#expr])   \
                                    {                                                                                   \
                                        FD_TRAP ();                                                                     \
                                    }                                                                                   \
                                }

#else

#define FD_ASSERT(expr)         do {} while (0)

#endif // DEBUG



#define	FD_DURING               NS_DURING
#define FD_HANDLER              NS_HANDLER                                                                              \
                                {                                                                                       \
                                    [[FDDebug sharedDebug] exception: localException];                                  \
                                }                                                                                       \
                                NS_ENDHANDLER


