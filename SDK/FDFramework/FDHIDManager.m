
//
// "FDHIDManager.m"
//
// Written by:  Axel 'awe' Wefers           [mailto:awe@fruitz-of-dojo.de].
//              ©2001-2012 Fruitz Of Dojo  [http://www.fruitz-of-dojo.de].
//


#import "FDHIDManager.h"
#import "FDHIDInternal.h"
#import "FDDebug.h"
#import "FDDefines.h"
#import "FDPreferences.h"

#import <IOKit/IOKitLib.h>
#import <IOKit/hidsystem/IOHIDLib.h>
#import <IOKit/hid/IOHIDLib.h>



#define FD_HID_DEVICE_GAME_PAD      @"_FDHIDDeviceGamePad"
#define FD_HID_DEVICE_KEYBOARD      @"_FDHIDDeviceKeyboard"
#define FD_HID_DEVICE_MOUSE         @"_FDHIDDeviceMouse"

#define FD_HID_LCC_IDENTIFIER       @"com.Logitech.Control Center.Daemon"
#define FD_HID_LCC_SUPPRESS_WARNING @"LCCSuppressWarning"



static NSString*        sDeviceFactories[]      = {
                                                    FD_HID_DEVICE_GAME_PAD,
                                                    FD_HID_DEVICE_KEYBOARD,
                                                    FD_HID_DEVICE_MOUSE
                                                  };



NSString*               FDHIDDeviceGamePad      = FD_HID_DEVICE_GAME_PAD;
NSString*               FDHIDDeviceKeyboard     = FD_HID_DEVICE_KEYBOARD;
NSString*               FDHIDDeviceMouse        = FD_HID_DEVICE_MOUSE;



static dispatch_once_t  sFDHIDManagerPredicate  = 0;
static FDHIDManager*    sFDHIDManagerInstance   = nil;



static void             FDHIDManager_InputHandler (void*, IOReturn, void*, IOHIDValueRef);  
static void             FDHIDManager_DeviceMatchingCallback (void*, IOReturn, void*, IOHIDDeviceRef);
static void             FDHIDManager_DeviceRemovalCallback (void*, IOReturn, void*, IOHIDDeviceRef);



@interface _FDHIDManager : FDHIDManager
{
@private
    IOHIDManagerRef     mpIOHIDManager;
    NSMutableArray*     mDevices;
    
    FDHIDEvent*         mpEvents;
    NSUInteger          mReadEvent;
    NSUInteger          mWriteEvent;
    NSUInteger          mMaxEvents;
}

- (id) initSharedHIDManager;
- (void) applicationWillResignActive: (NSNotification*) notification;
- (void) registerDevice: (IOHIDDeviceRef) pDevice;
- (void) unregisterDevice: (IOHIDDeviceRef) pDevice;

@end



@implementation _FDHIDManager

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



- (id) initSharedHIDManager
{
    self = [super init];
    
    if (self != nil)
    {
        BOOL success = YES;
        
        if (success)
        {
            mpIOHIDManager  = IOHIDManagerCreate (kCFAllocatorDefault, kIOHIDManagerOptionNone);
            success         = (mpIOHIDManager != NULL);
        }
        
        if (success)
        {
            success = (IOHIDManagerOpen (mpIOHIDManager, kIOHIDManagerOptionNone) == kIOReturnSuccess);
        }
        
        if (success)
        {
            mDevices = [[NSMutableArray alloc] initWithCapacity: 3];
            success  = (mDevices != nil);
        }
        
        if (success)
        {
            NSNotificationCenter* notificationCenter = [NSNotificationCenter defaultCenter];
            
            [notificationCenter addObserver: self
                                   selector: @selector (applicationWillResignActive:)
                                       name: NSApplicationWillResignActiveNotification 
                                     object: nil];
        }
        
        if (!success)
        {
            [self release];
            self = nil;
        }
    }
    
    return self;    
}



- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver: self];
    [mDevices release];
    
    if (mpIOHIDManager)
    {
        IOHIDManagerRegisterDeviceMatchingCallback (mpIOHIDManager, NULL, NULL);
        IOHIDManagerRegisterDeviceRemovalCallback (mpIOHIDManager, NULL, NULL);
        IOHIDManagerUnscheduleFromRunLoop (mpIOHIDManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        IOHIDManagerClose (mpIOHIDManager, kIOHIDManagerOptionNone);
    }
    
    if (mpEvents)
    {
        free (mpEvents);
    }
    
    sFDHIDManagerInstance = nil;
    
    [super dealloc];
}



- (void) applicationWillResignActive: (NSNotification*) notification
{
    FD_UNUSED (notification);
    
    for (_FDHIDDevice* device in mDevices)
    {
        [device flush];
    }
    
    mReadEvent  = 0;
    mWriteEvent = 0;
}



- (void) setDeviceFilter: (NSArray*) devices
{
    NSMutableArray* matchingArray = nil;
    
    if (devices != nil)
    {
        matchingArray = [NSMutableArray array];

        for (NSString* deviceName in devices)
        {
            Class   device = NSClassFromString (deviceName);
            
            if (device != nil)
            {
                NSArray* dicts = [device matchingDictionaries];
                
                if (dicts != nil)
                {
                    [matchingArray addObjectsFromArray: dicts];
                }
            }
        }
            
        if ([matchingArray count] == 0)
        {
            matchingArray = nil;
        }
     }
    
    IOHIDManagerSetDeviceMatchingMultiple (mpIOHIDManager, (CFMutableArrayRef) matchingArray);
    IOHIDManagerRegisterDeviceMatchingCallback (mpIOHIDManager, FDHIDManager_DeviceMatchingCallback, self);
    IOHIDManagerRegisterDeviceRemovalCallback (mpIOHIDManager, FDHIDManager_DeviceRemovalCallback, self);
    IOHIDManagerScheduleWithRunLoop (mpIOHIDManager, CFRunLoopGetCurrent (), kCFRunLoopDefaultMode);
}



- (NSArray*) devices
{
    return mDevices;
}



- (const FDHIDEvent*) nextEvent
{
    const FDHIDEvent* pEvent = nil;
     
    if (mReadEvent < mWriteEvent)
    {
        pEvent = &(mpEvents[mReadEvent++]);
    }
    else
    {
        mReadEvent  = 0;
        mWriteEvent = 0;
    }
     
    return pEvent;
}



- (void) pushEvent: (const FDHIDEvent*) pEvent
{
    if ([NSApp isActive] == YES)
    {
        if (mWriteEvent == mMaxEvents)
        {
            mMaxEvents   = (mMaxEvents + 1) << 1;
            mpEvents     = realloc (mpEvents, mMaxEvents * sizeof (FDHIDEvent));
            
            if (mpEvents == NULL)
            {
                mReadEvent  = 0;
                mWriteEvent = 0;
                mMaxEvents  = 0;
            }
        }
        
        if (mWriteEvent < mMaxEvents)
        {
            mpEvents[mWriteEvent++] = *pEvent;
        }
    }
}



- (void) registerDevice: (IOHIDDeviceRef) pDevice
{
    for (NSUInteger i = 0; i < FD_SIZE_OF_ARRAY (sDeviceFactories); ++i)
    {
        Class factory = NSClassFromString (sDeviceFactories[i]);
        
        if (factory != nil)
        {
            FD_ASSERT ([factory isSubclassOfClass: [_FDHIDDevice class]]);
            FD_ASSERT ([factory respondsToSelector: @selector (deviceWithDevice:)]);
            
            id device = [factory deviceWithDevice: pDevice];
            
            if (device != nil)
            {
                [device setDelegate: self];
                [mDevices addObject: device];
                
                IOHIDDeviceRegisterInputValueCallback (pDevice, &FDHIDManager_InputHandler, device);
                
                break;
            }
        }
    }
}



- (void) unregisterDevice: (IOHIDDeviceRef) pDevice
{
    IOHIDDeviceRegisterInputValueCallback (pDevice, NULL, NULL);
 
    for (_FDHIDDevice* device in mDevices)
    {
        if ([device iohidDeviceRef] == pDevice)
        {
            [mDevices removeObject: device];
            break;
        }
    }
}

@end



@implementation FDHIDManager

+ (id) allocWithZone: (NSZone*) zone
{
    return NSAllocateObject ([_FDHIDManager class], 0, zone);
}



+ (FDHIDManager*) sharedHIDManager
{
    dispatch_once (&sFDHIDManagerPredicate, ^{ sFDHIDManagerInstance = [[_FDHIDManager alloc] initSharedHIDManager]; });
    
    return sFDHIDManagerInstance;
}



+ (void) checkForIncompatibleDevices
{
    [[FDPreferences sharedPrefs] registerDefaultObject: [NSNumber numberWithBool: NO] forKey: FD_HID_LCC_SUPPRESS_WARNING];
    
    // check for Logitech Control Center. LCC installs its own kext and blocks HID events from Logitech devices
    if ([[NSWorkspace sharedWorkspace] absolutePathForAppBundleWithIdentifier: FD_HID_LCC_IDENTIFIER] != nil)
    {
        if ([[FDPreferences sharedPrefs] boolForKey: FD_HID_LCC_SUPPRESS_WARNING] == NO)
        {
            NSAlert*    alert   = [[[NSAlert alloc] init] autorelease];
            NSString*   appName = [[NSRunningApplication currentApplication] localizedName];
            NSString*   message = [NSString stringWithFormat: @"An installation of the Logitech Control Center software "
                                                              @"has been detected. This software is not compatible with %@.",
                                                              appName];
            NSString*   informative = [NSString stringWithFormat: @"Please uninstall the Logitech Control Center software "
                                                                  @"if you want to use a Logitech input device with %@.",
                                                                  appName];
            
            [alert setMessageText: message];
            [alert setInformativeText: informative];
            [alert setAlertStyle: NSCriticalAlertStyle];
            [alert setShowsSuppressionButton: YES];
            [alert runModal];
            
            [[FDPreferences sharedPrefs] setObject: [alert suppressionButton] forKey: FD_HID_LCC_SUPPRESS_WARNING];
        }
    }
    else
    {
        // reset the warning in case LCC was uninstalled
        [[FDPreferences sharedPrefs] setObject: [NSNumber numberWithBool: NO] forKey: FD_HID_LCC_SUPPRESS_WARNING];
    }
}



- (void) setDeviceFilter: (NSArray*) devices
{
    FD_UNUSED (devices);

    [self doesNotRecognizeSelector: _cmd];
}



- (NSArray*) devices
{
    [self doesNotRecognizeSelector: _cmd];
    
    return nil;
}



- (const FDHIDEvent*) nextEvent
{
    [self doesNotRecognizeSelector: _cmd];
    
    return nil;
}



- (void) pushEvent: (const FDHIDEvent*) pEvent
{
    FD_UNUSED (pEvent);
    
    [self doesNotRecognizeSelector: _cmd];
}

@end



void FDHIDManager_InputHandler (void* pContext, IOReturn result, void* pSender, IOHIDValueRef pValue)
{
    FD_UNUSED (result, pSender);
    FD_ASSERT (pContext != nil);
    FD_ASSERT (pValue != nil);
    
    if ([NSApp isActive] == YES)
    {
        _FDHIDDevice* device = (_FDHIDDevice*) pContext;
        
        [device handleInput: pValue];
    }
}



void FDHIDManager_DeviceMatchingCallback (void* pContext, IOReturn result, void* pSender, IOHIDDeviceRef pDevice)
{
    FD_UNUSED (result, pSender);
    FD_ASSERT (pContext == sFDHIDManagerInstance);
    FD_ASSERT (pDevice != nil);

    [((_FDHIDManager*) pContext) registerDevice: pDevice];
}



void FDHIDManager_DeviceRemovalCallback (void* pContext, IOReturn result, void* pSender, IOHIDDeviceRef pDevice) 
{
    FD_UNUSED (result, pSender);
    FD_ASSERT (pContext == sFDHIDManagerInstance);
    FD_ASSERT (pDevice != nil);
    
    [((_FDHIDManager*) pContext) unregisterDevice: pDevice];
}


