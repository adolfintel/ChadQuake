
//
// "FDHIDGamePad.m"
//
// Written by:  Axel 'awe' Wefers           [mailto:awe@fruitz-of-dojo.de].
//              ©2001-2012 Fruitz Of Dojo  [http://www.fruitz-of-dojo.de].
//


#import "FDHIDManager.h"
#import "FDHIDInternal.h"
#import "FDDebug.h"
#import "FDDefines.h"

#import <Cocoa/Cocoa.h>
#import <IOKit/hidsystem/IOHIDLib.h>
#import <IOKit/hid/IOHIDLib.h>



static float    FDHIDGamePad_GetValue (IOHIDValueRef, IOHIDElementRef);
static void     FDHIDGamePad_AxisHandler (id device, unsigned int, IOHIDValueRef, IOHIDElementRef);
static void     FDHIDGamePad_ButtonHandler (id device, unsigned int, IOHIDValueRef, IOHIDElementRef);



static FDHIDButtonMap   sFDHIDDualPsxAxisMap[] =
{
    { kHIDUsage_GD_X,           eFDHIDGamePadAxisLeftX,     &FDHIDGamePad_AxisHandler   },
    { kHIDUsage_GD_Y,           eFDHIDGamePadAxisLeftY,     &FDHIDGamePad_AxisHandler   },
    { kHIDUsage_GD_Z,           eFDHIDGamePadAxisRightY,    &FDHIDGamePad_AxisHandler   },
    { kHIDUsage_GD_Rx,          0,                          NULL                        },
    { kHIDUsage_GD_Ry,          0,                          NULL                        },
    { kHIDUsage_GD_Rz,          eFDHIDGamePadAxisRightX,    &FDHIDGamePad_AxisHandler   }
};

static FDHIDButtonMap    sFDHIDDualPsxButtonMap[] =
{
    { kHIDUsage_Button_1,       12,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 1,   13,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 2,   14,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 3,   15,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 4,   8,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 5,   9,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 6,   10,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 7,   11,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 8,   0,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 9,   3,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 10,  1,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 11,  2,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 12,  4,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 13,  5,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 14,  6,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 15,  7,                          &FDHIDGamePad_ButtonHandler }
};

static FDHIDElementMap  sFDHIDDualPsxMap[] =
{
    { kIOHIDElementTypeInput_Misc,      FD_SIZE_OF_ARRAY (sFDHIDDualPsxAxisMap),     &(sFDHIDDualPsxAxisMap[0])   },
    { kIOHIDElementTypeInput_Button,    FD_SIZE_OF_ARRAY (sFDHIDDualPsxButtonMap),   &(sFDHIDDualPsxButtonMap[0]) }
};



static FDHIDButtonMap   sFDHIDDualShock3AxisMap[] =
{
    { kHIDUsage_GD_X,           eFDHIDGamePadAxisLeftX,     &FDHIDGamePad_AxisHandler   },
    { kHIDUsage_GD_Y,           eFDHIDGamePadAxisLeftY,     &FDHIDGamePad_AxisHandler   },
    { kHIDUsage_GD_Z,           eFDHIDGamePadAxisRightX,    &FDHIDGamePad_AxisHandler   },
    { kHIDUsage_GD_Rx,          0,                          NULL                        },
    { kHIDUsage_GD_Ry,          0,                          NULL                        },
    { kHIDUsage_GD_Rz,          eFDHIDGamePadAxisRightY,    &FDHIDGamePad_AxisHandler   }
};

static FDHIDButtonMap   sFDHIDDualShock3ButtonMap[] =
{
    { kHIDUsage_Button_1 + 0,   0,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 1,   1,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 2,   2,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 3,   3,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 4,   4,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 5,   5,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 6,   6,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 7,   7,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 8,   8,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 9,   9,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 10,  10,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 11,  11,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 12,  12,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 13,  13,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 14,  14,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 15,  15,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 16,  16,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 17,  17,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 18,  18,                         &FDHIDGamePad_ButtonHandler }
};

static FDHIDElementMap  sFDHIDDualShock3Map[] =
{
    { kIOHIDElementTypeInput_Misc,      FD_SIZE_OF_ARRAY (sFDHIDDualShock3AxisMap),    &(sFDHIDDualShock3AxisMap[0])   },
    { kIOHIDElementTypeInput_Button,    FD_SIZE_OF_ARRAY (sFDHIDDualShock3ButtonMap),  &(sFDHIDDualShock3ButtonMap[0]) }
};



static FDHIDButtonMap   sFDHIDXbox360AxisMap[] =
{
    { kHIDUsage_GD_X,           eFDHIDGamePadAxisLeftX,     &FDHIDGamePad_AxisHandler   },
    { kHIDUsage_GD_Y,           eFDHIDGamePadAxisLeftY,     &FDHIDGamePad_AxisHandler   },
    { kHIDUsage_GD_Z,           8,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_GD_Rx,          eFDHIDGamePadAxisRightX,    &FDHIDGamePad_AxisHandler   },
    { kHIDUsage_GD_Ry,          eFDHIDGamePadAxisRightY,    &FDHIDGamePad_AxisHandler   },
    { kHIDUsage_GD_Rz,          9,                          &FDHIDGamePad_ButtonHandler }
};

static FDHIDButtonMap   sFDHIDXbox360ButtonMap[] =
{
    { kHIDUsage_Button_1 + 0,   14,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 1,   13,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 2,   15,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 3,   12,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 4,   10,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 5,   11,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 6,   1,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 7,   2,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 8,   3,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 9,   0,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 10,  16,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 11,  4,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 12,  6,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 13,  7,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 14,  5,                          &FDHIDGamePad_ButtonHandler },
};

static FDHIDElementMap  sFDHIDXbox360Map[] =
{
    { kIOHIDElementTypeInput_Misc,      FD_SIZE_OF_ARRAY (sFDHIDXbox360AxisMap),      &(sFDHIDXbox360AxisMap[0]) },
    { kIOHIDElementTypeInput_Button,    FD_SIZE_OF_ARRAY (sFDHIDXbox360ButtonMap),    &(sFDHIDXbox360ButtonMap[0]) }
};



static FDHIDButtonMap   sFDHIDDefaultGamePadAxisMap[] =
{
    { kHIDUsage_GD_X,           eFDHIDGamePadAxisLeftX,     &FDHIDGamePad_AxisHandler   },
    { kHIDUsage_GD_Y,           eFDHIDGamePadAxisLeftY,     &FDHIDGamePad_AxisHandler   },
    { kHIDUsage_GD_Z,           eFDHIDGamePadAxisLeftZ,     &FDHIDGamePad_AxisHandler   },
    { kHIDUsage_GD_Rx,          eFDHIDGamePadAxisRightX,    &FDHIDGamePad_AxisHandler   },
    { kHIDUsage_GD_Ry,          eFDHIDGamePadAxisRightY,    &FDHIDGamePad_AxisHandler   },
    { kHIDUsage_GD_Rz,          eFDHIDGamePadAxisRightZ,    &FDHIDGamePad_AxisHandler   }
};

static FDHIDButtonMap   sFDHIDDefaultGamePadButtonMap[] =
{
    { kHIDUsage_Button_1 + 0,   0,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 1,   1,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 2,   2,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 3,   3,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 4,   4,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 5,   5,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 6,   6,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 7,   7,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 8,   8,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 9,   9,                          &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 10,  10,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 11,  11,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 12,  12,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 13,  13,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 14,  14,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 15,  15,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 16,  16,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 17,  17,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 18,  18,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 19,  19,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 20,  20,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 21,  21,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 22,  22,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 23,  23,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 24,  24,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 25,  25,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 26,  26,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 27,  27,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 28,  28,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 29,  29,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 30,  30,                         &FDHIDGamePad_ButtonHandler },
    { kHIDUsage_Button_1 + 31,  31,                         &FDHIDGamePad_ButtonHandler }
};

static FDHIDElementMap  sFDHIDDefaultGamePadMap[] =
{
    { kIOHIDElementTypeInput_Misc,   FD_SIZE_OF_ARRAY (sFDHIDDefaultGamePadAxisMap),   &(sFDHIDDefaultGamePadAxisMap[0])   },
    { kIOHIDElementTypeInput_Button, FD_SIZE_OF_ARRAY (sFDHIDDefaultGamePadButtonMap), &(sFDHIDDefaultGamePadButtonMap[0]) }
};



FDHIDDeviceDesc gFDHIDGamePadMap[] =
{
    { 1118, 654,    &(sFDHIDXbox360Map[0]),         FD_SIZE_OF_ARRAY (sFDHIDXbox360Map),        0 },
    { -1,   -1,     &(sFDHIDDefaultGamePadMap[0]),  FD_SIZE_OF_ARRAY (sFDHIDDefaultGamePadMap), 0 }
};



FDHIDDeviceDesc gFDHIDJoystickMap[] =
{
    { 1356, 616,    &(sFDHIDDualShock3Map[0]),      FD_SIZE_OF_ARRAY (sFDHIDDualShock3Map),     0 },
    { 2883, 3,      &(sFDHIDDualPsxMap[0]),         FD_SIZE_OF_ARRAY (sFDHIDDualPsxMap),        0 },
    { -1,   -1,     &(sFDHIDDefaultGamePadMap[0]),  FD_SIZE_OF_ARRAY (sFDHIDDefaultGamePadMap), 0 }
};



FDHIDUsageToDevice gFDHIDGamePadUsageMap[] =
{
    { kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad,  &(gFDHIDGamePadMap[0]),  FD_SIZE_OF_ARRAY (gFDHIDGamePadMap),  0 },
    { kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick, &(gFDHIDJoystickMap[0]), FD_SIZE_OF_ARRAY (gFDHIDJoystickMap), 0 }
};



@interface _FDHIDDeviceGamePad : _FDHIDDevice
{
}

+ (NSArray*) matchingDictionaries;
+ (FDHIDDevice*) deviceWithDevice: (IOHIDDeviceRef) pDevice;

@end



@implementation _FDHIDDeviceGamePad

+ (NSArray*) matchingDictionaries
{
    return [self matchingDictionaries: gFDHIDGamePadUsageMap withCount: FD_SIZE_OF_ARRAY (gFDHIDGamePadUsageMap)]; 
}



+ (FDHIDDevice*) deviceWithDevice: (IOHIDDeviceRef) pDevice
{
    const NSUInteger            numUsages   = FD_SIZE_OF_ARRAY (gFDHIDGamePadUsageMap);
    const FDHIDUsageToDevice*   pUsageMap   = &(gFDHIDGamePadUsageMap[0]);
    
    return [self deviceWithDevice: pDevice usageMap: pUsageMap count: numUsages];
}

@end



inline float   FDHIDGamePad_GetValue (IOHIDValueRef pValue, IOHIDElementRef pElement)
{
    FD_ASSERT (pValue != nil);
    FD_ASSERT (pElement != nil);
    
    const double physValue  = IOHIDValueGetScaledValue (pValue, kIOHIDValueScaleTypePhysical);
    const double physMin    = IOHIDElementGetPhysicalMin (pElement);
    const double physMax    = IOHIDElementGetPhysicalMax (pElement);
    
    return (physValue - physMin) / (physMax - physMin);
}



void FDHIDGamePad_AxisHandler (id device, unsigned int axis, IOHIDValueRef pValue, IOHIDElementRef pElement)
{
    FD_ASSERT (pValue != nil);
    FD_ASSERT (pElement != nil);
    
    FDHIDEvent  event = { 0 };
    
    event.mDevice   = device;
    event.mType     = eFDHIDEventTypeGamePadAxis;
    event.mButton   = axis;
    event.mFloatVal = (FDHIDGamePad_GetValue (pValue, pElement) - 0.5f) * 2.0f;
    
    [device pushEvent: &event];
}



void FDHIDGamePad_ButtonHandler (id device, unsigned int button, IOHIDValueRef pValue, IOHIDElementRef pElement)
{
    FD_ASSERT (pValue != nil);
    FD_ASSERT (pElement != nil);
    
    FDHIDEvent  event = { 0 };
    
    event.mDevice   = device;
    event.mType     = eFDHIDEventTypeGamePadButton;
    event.mButton   = button;
    event.mBoolVal  = (FDHIDGamePad_GetValue (pValue, pElement) > 0.5f);
    
    [device pushEvent: &event];
}


