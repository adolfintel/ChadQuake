
//
// "FDHIDInternal.h"
//
// Written by:	Axel 'awe' Wefers			[mailto:awe@fruitz-of-dojo.de].
//				©2001-2012 Fruitz Of Dojo 	[http://www.fruitz-of-dojo.de].
//


#import "FDHIDDevice.h"
#import "FDHIDManager.h"
//#import "FDHIDActuator.h"

#import <Cocoa/Cocoa.h>
#import <ForceFeedback/ForceFeedback.h>
#import <IOKit/hidsystem/IOHIDLib.h>
#import <IOKit/hid/IOHIDLib.h>



typedef struct
{
    uint32_t                mUsage;
    uint32_t                mButton;
    void                    (*mpEventHandler)(id, unsigned int, IOHIDValueRef, IOHIDElementRef);
} FDHIDButtonMap;



typedef struct
{
    uint32                  mType;
    uint32                  mNumButtons;
    FDHIDButtonMap*         mpButtons;
} FDHIDElementMap;



typedef struct
{
    SInt32                  mVendorId;
    SInt32                  mProductId;
    FDHIDElementMap*        mpElements;
    uint32_t                mNumElements;
    uint32_t                mPadding;
} FDHIDDeviceDesc;



typedef struct
{
    uint32_t                mUsagePage;
    uint32_t                mUsage;
    FDHIDDeviceDesc*        mDeviceDesc;
    uint32_t                mNumDeviceDesc;
    uint32_t                m_Padding;
} FDHIDUsageToDevice;



@interface _FDHIDDevice : FDHIDDevice
{
    IOHIDDeviceRef          mpIOHIDDevice;
    NSString*               mVendorName;
    NSString*               mProductName;
 //   FDHIDActuator*          mActuator;
    const FDHIDDeviceDesc*  mpDeviceDesc;
    FDHIDManager*           mDelegate;
}

+ (NSArray*) matchingDictionaries: (const FDHIDUsageToDevice*) usageMap withCount: (NSUInteger) numUsages;
+ (FDHIDDevice*) deviceWithDevice: (IOHIDDeviceRef) pDevice
                         usageMap: (const FDHIDUsageToDevice*) pUsageMap
                            count: (NSUInteger) numUsages;

- (id) initWithDevice: (IOHIDDeviceRef) pDevice deviceDescriptors: (const FDHIDDeviceDesc*) deviceDescriptors;

- (void) setDelegate: (FDHIDManager*) delegate;
- (void) pushEvent: (const FDHIDEvent*) pEvent;

- (IOHIDDeviceRef) iohidDeviceRef;
- (void) handleInput: (IOHIDValueRef) pValue;
- (FDHIDElementMap*) elementMap;
- (NSUInteger) elementCount;
- (void) flush;

@end
#if 0


@interface _FDHIDActuator : FDHIDActuator
{
    io_service_t            mIoService;
    FFDeviceObjectReference mpDevice;
    FFEffectObjectReference mpEffect;
    FFEFFECT                mEffectParams;
    FFENVELOPE              mEffectEnvelope;
    FFPERIODIC              mEffectPeriodic;
    DWORD                   mEffectAxes[32];
    LONG                    mEffectDirection[2];

}

- (id) initWithDevice: (_FDHIDDevice*) device;

@end
#endif

