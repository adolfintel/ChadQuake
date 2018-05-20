
//
// "FDPreferences.m"
//
// Written by:  Axel 'awe' Wefers           [mailto:awe@fruitz-of-dojo.de].
//              ©2001-2012 Fruitz Of Dojo  [http://www.fruitz-of-dojo.de].
//


#import "FDPreferences.h"
#import "FDDebug.h"
#import "FDDisplay.h"
#import "FDDisplayMode.h"



static dispatch_once_t  sFDPreferecncesPredicate = 0;
static FDPreferences*   sFDPreferencesInstance  = nil;



@interface FDPreferences ()

- (id) init;
- (id) initSharedPreferences;

@end



@implementation FDPreferences



+ (FDPreferences*) sharedPrefs
{
    dispatch_once (&sFDPreferecncesPredicate, ^{ sFDPreferencesInstance = [[self alloc] initSharedPreferences]; });
    
    return sFDPreferencesInstance;
}



- (id) init
{
    self = [super init];
    
    if (self != nil)
    {
        [self doesNotRecognizeSelector: _cmd];
        [self release];
    }
    
    return nil;
}



- (id) initSharedPreferences
{
    self = [super init];
    
    return self;
}



- (id) serializableFromObject: (id) object
{
    id serializable = nil;
    
    if ([object isKindOfClass: [FDDisplay class]] == YES)
    {
        serializable = [object description];
    }
    else if ([object isKindOfClass: [FDDisplayMode class]] == YES)
    {
        serializable = [object description];
    }
    else if ([object isKindOfClass: [NSTextField class]] == YES)
    {
        serializable = [object stringValue];
    }
    else if ([object isKindOfClass: [NSPopUpButton class]] == YES)
    {
        serializable = [self serializableFromObject: [[object selectedItem] representedObject]];
        
        if (serializable == nil)
        {
            serializable = [NSNumber numberWithLong: [object selectedTag]];
        }
    }
    else if ([object isKindOfClass: [NSButton class]] == YES)
    {
        serializable = [NSNumber numberWithBool: ([object state] == NSOnState)];
    }
    else if ([object isKindOfClass: [NSString class]] == YES)
    {
        serializable = object;
    }
    else if ([object isKindOfClass: [NSNumber class]] == YES)
    {
        serializable = object;
    }
    else if ([object isKindOfClass: [NSArray class]] == YES)
    {
        serializable = object;
    }
    else if ([object isKindOfClass: [NSDictionary class]] == YES)
    {
        serializable = object;
    }
    
    return serializable;
}



- (void) registerDefaults: (NSDictionary*) dictionary
{
    [[NSUserDefaults standardUserDefaults] registerDefaults: dictionary];
}



- (void) registerDefaultObject: (NSObject*) object forKey: (NSString*) key
{
    [self registerDefaults: [NSDictionary dictionaryWithObject: [self serializableFromObject: object] forKey: key]];
}



- (void) setObject: (id) object forKey: (NSString*) key
{
    id serializable = [self serializableFromObject: object];
    
    if (serializable != nil)
    {
        [[NSUserDefaults standardUserDefaults] setObject: serializable forKey: key];
    }
    else
    {
        FDLog (@"FDPreferences: cannot serialize class of type: %@!", [object class]);
    }
}

- (void) setBool: (BOOL) value forKey: (NSString*) key
{
    [[NSUserDefaults standardUserDefaults] setBool: value forKey: key];
}

- (BOOL) boolForKey: (NSString*) key
{
    return [[NSUserDefaults standardUserDefaults] boolForKey: key];
}



- (NSInteger) integerForKey: (NSString*) key
{
    return [[NSUserDefaults standardUserDefaults] integerForKey: key];
}



- (NSString*) stringForKey: (NSString*) key
{
    if ([[NSUserDefaults standardUserDefaults] stringForKey: key] == nil)
        return @"";
    return [[NSUserDefaults standardUserDefaults] stringForKey: key];
}



- (NSArray*) arrayForKey: (NSString*) key
{   
    NSArray*    array   = nil;
    id          object  = [[NSUserDefaults standardUserDefaults] objectForKey: key];
    
    if ([object isKindOfClass: [NSArray class]] == YES)
    {
        array = object;
    }
    
    return array;
}



- (BOOL) synchronize
{
    return [[NSUserDefaults standardUserDefaults] synchronize];
}

@end

//---------------------------------------------------------------------------------------------------------------------------
