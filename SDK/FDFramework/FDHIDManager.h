
//
// "FDHIDManager.h" - HID input
//
// Written by:	Axel 'awe' Wefers			[mailto:awe@fruitz-of-dojo.de].
//				©2001-2012 Fruitz Of Dojo 	[http://www.fruitz-of-dojo.de].
//


#import "FDDefines.h"
#import "FDHIDDevice.h"

#import <Cocoa/Cocoa.h>
#import <IOKit/hid/IOHIDLib.h>



FD_EXTERN NSString*    FDHIDDeviceGamePad;
FD_EXTERN NSString*    FDHIDDeviceKeyboard;
FD_EXTERN NSString*    FDHIDDeviceMouse;



enum FDHIDEventType
{
    eFDHIDEventTypeGamePadAxis,
    eFDHIDEventTypeGamePadButton,
    eFDHIDEventTypeKeyboard,
    eFDHIDEventTypeMouseAxis,
    eFDHIDEventTypeMouseButton
};






enum FDHIDMouseAxis
{
    eFDHIDMouseAxisX,
    eFDHIDMouseAxisY,
    eFDHIDMouseAxisWheel
};



enum FDHIDGamePadAxis
{
    eFDHIDGamePadAxisLeftX,
    eFDHIDGamePadAxisLeftY,
    eFDHIDGamePadAxisLeftZ,
    eFDHIDGamePadAxisRightX,
    eFDHIDGamePadAxisRightY,
    eFDHIDGamePadAxisRightZ
};



typedef struct
{
    FDHIDDevice*        mDevice;
    enum FDHIDEventType mType;
    unsigned int        mButton;

    union
    {
        float           mFloatVal;
        signed int      mIntVal;
        BOOL            mBoolVal;
    };

    unsigned int        mPadding;
} FDHIDEvent;



@interface FDHIDManager : NSObject
{
}

+ (FDHIDManager*) sharedHIDManager;
+ (void) checkForIncompatibleDevices;

- (void) setDeviceFilter: (NSArray*) deviceTypes;
- (NSArray*) devices;
- (const FDHIDEvent*) nextEvent;

@end


