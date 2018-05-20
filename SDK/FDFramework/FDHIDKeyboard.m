
//
// "FDHIDKeyboard.m"
//
// Written by:  Axel 'awe' Wefers           [mailto:awe@fruitz-of-dojo.de].
//              ©2001-2012 Fruitz Of Dojo  [http://www.fruitz-of-dojo.de].
//


#import "quakedef.h"
#import "FDHIDManager.h"
#import "FDHIDDevice.h"
#import "FDHIDInternal.h"
#import "FDDebug.h"
#import "FDDefines.h"

#import <Cocoa/Cocoa.h>
#import <IOKit/hidsystem/IOHIDLib.h>
#import <IOKit/hid/IOHIDLib.h>
#import <Carbon/Carbon.h>



static void             FDHIDKeyboard_KeyHandler (id, unsigned int, IOHIDValueRef, IOHIDElementRef);
static void             FDHIDKeyboard_FnHandler (id, unsigned int, IOHIDValueRef, IOHIDElementRef);



static FDHIDButtonMap   sFDHIDKeyboardDefaultMiscMap[] = 
{
    { 0x03,                                     0x03,                   &FDHIDKeyboard_FnHandler    }, 
};



static FDHIDButtonMap   sFDHIDKeyboardDefaultButtonMap[] = 
{
    { 0,                                        0x00,                   NULL                        }, 
    { kHIDUsage_KeyboardErrorRollOver,          0x00,                   NULL                        }, 
    { kHIDUsage_KeyboardPOSTFail,               0x00,                   NULL                        }, 
    { kHIDUsage_KeyboardErrorUndefined,         0x00,                   NULL                        }, 
    { kHIDUsage_KeyboardA,                      kVK_ANSI_A,             &FDHIDKeyboard_KeyHandler   }, 
    { kHIDUsage_KeyboardB,                      kVK_ANSI_B,             &FDHIDKeyboard_KeyHandler   }, 
    { kHIDUsage_KeyboardC,                      kVK_ANSI_C,             &FDHIDKeyboard_KeyHandler   }, 
    { kHIDUsage_KeyboardD,                      kVK_ANSI_D,             &FDHIDKeyboard_KeyHandler   }, 
    { kHIDUsage_KeyboardE,                      kVK_ANSI_E,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF,                      kVK_ANSI_F,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardG,                      kVK_ANSI_G,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardH,                      kVK_ANSI_H,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardI,                      kVK_ANSI_I,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardJ,                      kVK_ANSI_J,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardK,                      kVK_ANSI_K,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardL,                      kVK_ANSI_L,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardM,                      kVK_ANSI_M,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardN,                      kVK_ANSI_N,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardO,                      kVK_ANSI_O,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardP,                      kVK_ANSI_P,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardQ,                      kVK_ANSI_Q,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardR,                      kVK_ANSI_R,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardS,                      kVK_ANSI_S,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardT,                      kVK_ANSI_T,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardU,                      kVK_ANSI_U,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardV,                      kVK_ANSI_V,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardW,                      kVK_ANSI_W,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardX,                      kVK_ANSI_X,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardY,                      kVK_ANSI_Y,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardZ,                      kVK_ANSI_Z,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keyboard1,                      kVK_ANSI_1,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keyboard2,                      kVK_ANSI_2,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keyboard3,                      kVK_ANSI_3,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keyboard4,                      kVK_ANSI_4,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keyboard5,                      kVK_ANSI_5,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keyboard6,                      kVK_ANSI_6,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keyboard7,                      kVK_ANSI_7,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keyboard8,                      kVK_ANSI_8,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keyboard9,                      kVK_ANSI_9,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keyboard0,                      kVK_ANSI_0,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardReturnOrEnter,          kVK_Return,             &FDHIDKeyboard_KeyHandler   },    
    { kHIDUsage_KeyboardEscape,                 kVK_Escape,             &FDHIDKeyboard_KeyHandler   }, 
    { kHIDUsage_KeyboardDeleteOrBackspace,      kVK_Delete,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardTab,                    kVK_Tab,                &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardSpacebar,               kVK_Space,              &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardHyphen,                 kVK_ANSI_Minus,         &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardEqualSign,              kVK_ANSI_Equal,         &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardOpenBracket,            kVK_ANSI_LeftBracket,   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardCloseBracket,           kVK_ANSI_RightBracket,  &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardBackslash,              kVK_ANSI_Backslash,     &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardNonUSPound,             0x00,                   NULL                        },
    { kHIDUsage_KeyboardSemicolon,              kVK_ANSI_Semicolon,     &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardQuote,                  kVK_ANSI_Quote,         &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardGraveAccentAndTilde,    kVK_ANSI_Grave,         &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardComma,                  kVK_ANSI_Comma,         &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardPeriod,                 kVK_ANSI_Period,        &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardSlash,                  kVK_ANSI_Slash,         &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardCapsLock,               kVK_CapsLock,           &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF1,                     kVK_F1,                 &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF2,                     kVK_F2,                 &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF3,                     kVK_F3,                 &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF4,                     kVK_F4,                 &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF5,                     kVK_F5,                 &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF6,                     kVK_F6,                 &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF7,                     kVK_F7,                 &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF8,                     kVK_F8,                 &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF9,                     kVK_F9,                 &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF10,                    kVK_F10,                &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF11,                    kVK_F11,                &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF12,                    kVK_F12,                &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardPrintScreen,            0x69,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardScrollLock,             0x00,                   NULL                        },
    { kHIDUsage_KeyboardPause,                  0x00,                   NULL                        },
    { kHIDUsage_KeyboardInsert,                 0x72,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardHome,                   kVK_Home,               &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardPageUp,                 kVK_PageUp,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardDeleteForward,          kVK_ForwardDelete,      &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardEnd,                    kVK_End,                &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardPageDown,               kVK_PageDown,           &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardRightArrow,             kVK_RightArrow,         &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardLeftArrow,              kVK_LeftArrow,          &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardDownArrow,              kVK_DownArrow,          &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardUpArrow,                kVK_UpArrow,            &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeypadNumLock,                  0x47,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeypadSlash,                    0x4b,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeypadAsterisk,                 0x43,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeypadHyphen,                   0x4e,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeypadPlus,                     0x45,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeypadEnter,                    0x4c,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keypad1,                        0x53,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keypad2,                        0x54,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keypad3,                        0x55,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keypad4,                        0x56,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keypad5,                        0x57,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keypad6,                        0x58,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keypad7,                        0x59,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keypad8,                        0x5b,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keypad9,                        0x5c,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_Keypad0,                        0x52,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeypadPeriod,                   0x41,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardNonUSBackslash,         0x00,                   NULL                        },
    { kHIDUsage_KeyboardApplication,            0x00,                   NULL                        },
    { kHIDUsage_KeyboardPower,                  0x00,                   NULL                        },
    { kHIDUsage_KeypadEqualSign,                0x51,                   &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF13,                    kVK_F13,                &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF14,                    kVK_F14,                &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF15,                    kVK_F15,                &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF16,                    kVK_F16,                &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF17,                    kVK_F17,                &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF18,                    kVK_F18,                &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF19,                    kVK_F19,                &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF20,                    kVK_F20,                &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardF21,                    0x00,                   NULL                        },
    { kHIDUsage_KeyboardF22,                    0x00,                   NULL                        },
    { kHIDUsage_KeyboardF23,                    0x00,                   NULL                        },
    { kHIDUsage_KeyboardF24,                    0x00,                   NULL                        },
    { kHIDUsage_KeyboardExecute,                0x00,                   NULL                        },
    { kHIDUsage_KeyboardHelp,                   kVK_Help,               &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardMenu,                   0x00,                   NULL                        },
    { kHIDUsage_KeyboardSelect,                 0x00,                   NULL                        },
    { kHIDUsage_KeyboardStop,                   0x00,                   NULL                        },
    { kHIDUsage_KeyboardAgain,                  0x00,                   NULL                        },
    { kHIDUsage_KeyboardUndo,                   0x00,                   NULL                        },
    { kHIDUsage_KeyboardCut,                    0x00,                   NULL                        },
    { kHIDUsage_KeyboardCopy,                   0x00,                   NULL                        },
    { kHIDUsage_KeyboardPaste,                  0x00,                   NULL                        },
    { kHIDUsage_KeyboardFind,                   0x00,                   NULL                        },
    { kHIDUsage_KeyboardMute,                   kVK_Mute,               &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardVolumeUp,               kVK_VolumeUp,           &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardVolumeDown,             kVK_VolumeDown,         &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardLockingCapsLock,        0x00,                   NULL                        },
    { kHIDUsage_KeyboardLockingNumLock,         0x00,                   NULL                        },
    { kHIDUsage_KeyboardLockingScrollLock,      0x00,                   NULL                        },
    { kHIDUsage_KeypadComma,                    0x00,                   NULL                        },
    { kHIDUsage_KeypadEqualSignAS400,           0x00,                   NULL                        },
    { kHIDUsage_KeyboardInternational1,         0x00,                   NULL                        },
    { kHIDUsage_KeyboardInternational2,         0x00,                   NULL                        },
    { kHIDUsage_KeyboardInternational3,         0x00,                   NULL                        },
    { kHIDUsage_KeyboardInternational4,         0x00,                   NULL                        },
    { kHIDUsage_KeyboardInternational5,         0x00,                   NULL                        },
    { kHIDUsage_KeyboardInternational6,         0x00,                   NULL                        },
    { kHIDUsage_KeyboardInternational7,         0x00,                   NULL                        },
    { kHIDUsage_KeyboardInternational8,         0x00,                   NULL                        },
    { kHIDUsage_KeyboardInternational9,         0x00,                   NULL                        },
    { kHIDUsage_KeyboardLANG1,                  0x00,                   NULL                        },
    { kHIDUsage_KeyboardLANG2,                  0x00,                   NULL                        },
    { kHIDUsage_KeyboardLANG3,                  0x00,                   NULL                        },
    { kHIDUsage_KeyboardLANG4,                  0x00,                   NULL                        },
    { kHIDUsage_KeyboardLANG5,                  0x00,                   NULL                        },
    { kHIDUsage_KeyboardLANG6,                  0x00,                   NULL                        },
    { kHIDUsage_KeyboardLANG7,                  0x00,                   NULL                        },
    { kHIDUsage_KeyboardLANG8,                  0x00,                   NULL                        },
    { kHIDUsage_KeyboardLANG9,                  0x00,                   NULL                        },
    { kHIDUsage_KeyboardAlternateErase,         0x00,                   NULL                        },
    { kHIDUsage_KeyboardSysReqOrAttention,      0x00,                   NULL                        },
    { kHIDUsage_KeyboardCancel,                 0x00,                   NULL                        },
    { kHIDUsage_KeyboardClear,                  0x00,                   NULL                        },
    { kHIDUsage_KeyboardPrior,                  0x00,                   NULL                        },
    { kHIDUsage_KeyboardReturn,                 kVK_Return,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardSeparator,              0x00,                   NULL                        },
    { kHIDUsage_KeyboardOut,                    0x00,                   NULL                        },
    { kHIDUsage_KeyboardOper,                   0x00,                   NULL                        },
    { kHIDUsage_KeyboardClearOrAgain,           0x00,                   NULL                        },
    { kHIDUsage_KeyboardCrSelOrProps,           0x00,                   NULL                        },
    { kHIDUsage_KeyboardExSel,                  0x00,                   NULL                        },
    { 0xA5,                                     0x00,                   NULL                        },
    { 0xA6,                                     0x00,                   NULL                        },
    { 0xA7,                                     0x00,                   NULL                        },
    { 0xA8,                                     0x00,                   NULL                        },
    { 0xA9,                                     0x00,                   NULL                        },
    { 0xAA,                                     0x00,                   NULL                        },
    { 0xAB,                                     0x00,                   NULL                        },
    { 0xAC,                                     0x00,                   NULL                        },
    { 0xAD,                                     0x00,                   NULL                        },
    { 0xAE,                                     0x00,                   NULL                        },
    { 0xAF,                                     0x00,                   NULL                        },
    { 0xB0,                                     0x00,                   NULL                        },
    { 0xB1,                                     0x00,                   NULL                        },
    { 0xB2,                                     0x00,                   NULL                        },
    { 0xB3,                                     0x00,                   NULL                        },
    { 0xB4,                                     0x00,                   NULL                        },
    { 0xB5,                                     0x00,                   NULL                        },
    { 0xB6,                                     0x00,                   NULL                        },
    { 0xB7,                                     0x00,                   NULL                        },
    { 0xB8,                                     0x00,                   NULL                        },
    { 0xB9,                                     0x00,                   NULL                        },
    { 0xBA,                                     0x00,                   NULL                        },
    { 0xBB,                                     0x00,                   NULL                        },
    { 0xBC,                                     0x00,                   NULL                        },
    { 0xBD,                                     0x00,                   NULL                        },
    { 0xBE,                                     0x00,                   NULL                        },
    { 0xBF,                                     0x00,                   NULL                        },
    { 0xC0,                                     0x00,                   NULL                        },
    { 0xC1,                                     0x00,                   NULL                        },
    { 0xC2,                                     0x00,                   NULL                        },
    { 0xC3,                                     0x00,                   NULL                        },
    { 0xC4,                                     0x00,                   NULL                        },
    { 0xC5,                                     0x00,                   NULL                        },
    { 0xC6,                                     0x00,                   NULL                        },
    { 0xC7,                                     0x00,                   NULL                        },
    { 0xC8,                                     0x00,                   NULL                        },
    { 0xC9,                                     0x00,                   NULL                        },
    { 0xCA,                                     0x00,                   NULL                        },
    { 0xCB,                                     0x00,                   NULL                        },
    { 0xCC,                                     0x00,                   NULL                        },
    { 0xCD,                                     0x00,                   NULL                        },
    { 0xCE,                                     0x00,                   NULL                        },
    { 0xCF,                                     0x00,                   NULL                        },
    { 0xD0,                                     0x00,                   NULL                        },
    { 0xD1,                                     0x00,                   NULL                        },
    { 0xD2,                                     0x00,                   NULL                        },
    { 0xD3,                                     0x00,                   NULL                        },
    { 0xD4,                                     0x00,                   NULL                        },
    { 0xD5,                                     0x00,                   NULL                        },
    { 0xD6,                                     0x00,                   NULL                        },
    { 0xD7,                                     0x00,                   NULL                        },
    { 0xD8,                                     0x00,                   NULL                        },
    { 0xD9,                                     0x00,                   NULL                        },
    { 0xDA,                                     0x00,                   NULL                        },
    { 0xDB,                                     0x00,                   NULL                        },
    { 0xDC,                                     0x00,                   NULL                        },
    { 0xDD,                                     0x00,                   NULL                        },
    { 0xDE,                                     0x00,                   NULL                        },
    { 0xDF,                                     0x00,                   NULL                        },
    { kHIDUsage_KeyboardLeftControl,            kVK_Control,            &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardLeftShift,              kVK_Shift,              &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardLeftAlt,                kVK_Option,             &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardLeftGUI,                kVK_Command,            &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardRightControl,           kVK_RightControl,       &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardRightShift,             kVK_RightShift,         &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardRightAlt,               kVK_RightOption,        &FDHIDKeyboard_KeyHandler   },
    { kHIDUsage_KeyboardRightGUI,               kVK_Command,            &FDHIDKeyboard_KeyHandler   }
};



static FDHIDElementMap  sFDHIDKeyboardDefaultMap[] =
{
    { kIOHIDElementTypeInput_Misc,   FD_SIZE_OF_ARRAY (sFDHIDKeyboardDefaultMiscMap),   &(sFDHIDKeyboardDefaultMiscMap[0]) },
    { kIOHIDElementTypeInput_Button, FD_SIZE_OF_ARRAY (sFDHIDKeyboardDefaultButtonMap), &(sFDHIDKeyboardDefaultButtonMap[0]) }
};



FDHIDDeviceDesc gFDHIDKeyboardMap[] =
{
    { -1, -1, &(sFDHIDKeyboardDefaultMap[0]), FD_SIZE_OF_ARRAY (sFDHIDKeyboardDefaultMap), 0 }
};



FDHIDUsageToDevice gFDHIDKeyboardUsageMap[] =
{
    { kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard, &(gFDHIDKeyboardMap[0]), FD_SIZE_OF_ARRAY (gFDHIDKeyboardMap), 0 }
};


#include "keys.h"
const UInt8     gInSpecialKey[] =
{
    K_UPARROW,              K_DOWNARROW,            K_LEFTARROW,            K_RIGHTARROW,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      K_INSERT,
    K_DELETE,                  K_HOME,                 0,                      K_END,
    K_PAGEUP,                 K_PAGEDOWN,                 0,                      0,
    K_PAUSE,                0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      K_NUMLOCK,           0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      K_INSERT
};



const UInt8     gInSpecialKeyFn[] =
{
    K_PAGEUP,                 K_PAGEDOWN,                 K_HOME,                 K_END,
    K_F1,                   K_F2,                   K_F3,                   K_F4,
    K_F5,                   K_F6,                   K_F7,                   K_F8,
    K_F9,                   K_F10,                  K_F11,                  K_F12,
    0 /*K_F13*/,             0 /*     K_F14*/,      0          /*  K_F15*/,                  0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      K_INSERT,
    K_DELETE,                  K_HOME,                 0,                      K_END,
    K_PAGEUP,                 K_PAGEDOWN,                 0,                      0,
    K_PAUSE,                0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      K_NUMLOCK,           0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      K_INSERT
};



const UInt8     gInNumPadKey[] = 
{   
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0,                      0,                      0,
    0,                      0 /*KeyPadPeriod*/,     0,                      K_NUMPAD_MULTIPLY /*K_KP_STAR*/,
    0,                      K_NUMPAD_PLUS /*K_KP_PLUS*/,              0,                      0,
    0,                      0,                      0,                      0,
    K_ENTER /*K_KP_ENTER*/,             K_NUMPAD_PERIOD /*K_KP_SLASH*/,             K_NUMPAD_MINUS /*K_KP_MINUS*/,             0,
    0,                      0 /*KeyPadEqual*/,      K_NUMPAD_0 /*K_KP_INS*/,               K_NUMPAD_1 /*K_KP_END*/,
    K_NUMPAD_2 /*K_KP_DOWNARROW*/,         K_NUMPAD_3 /*K_KP_PGDN*/,              K_NUMPAD_4 /*K_KP_LEFTARROW*/,         K_NUMPAD_5,
    K_NUMPAD_6 /*K_KP_RIGHTARROW*/,        K_NUMPAD_7 /*K_KP_HOME*/,              0,                      K_NUMPAD_8 /*K_KP_UPARROW*/,
    K_NUMPAD_9 /*K_KP_PGUP*/
};



@interface _FDHIDDeviceKeyboard : _FDHIDDevice
{
    BOOL    mFnKeyIsDown;
}

+ (NSArray*) matchingDictionaries;

+ (NSUInteger) usagePage;
+ (NSUInteger) usage;

- (id) initWithDevice: (IOHIDDeviceRef) pDevice deviceDescriptors: (const FDHIDDeviceDesc*) pDeviceDescriptors;

- (void) setFnKeyState: (BOOL) isDown;
- (void) evaluateEvent: (NSEvent*) event;

@end



@implementation _FDHIDDeviceKeyboard

+ (NSArray*) matchingDictionaries
{
    return [self matchingDictionaries: gFDHIDKeyboardUsageMap withCount: FD_SIZE_OF_ARRAY (gFDHIDKeyboardUsageMap)]; 
}



+ (FDHIDDevice*) deviceWithDevice: (IOHIDDeviceRef) pDevice
{
    const NSUInteger            numUsages   = FD_SIZE_OF_ARRAY (gFDHIDKeyboardUsageMap);
    const FDHIDUsageToDevice*   pUsageMap   = &(gFDHIDKeyboardUsageMap[0]);
    
    return [self deviceWithDevice: pDevice usageMap: pUsageMap count: numUsages];
}
            


- (id) initWithDevice: (IOHIDDeviceRef) pDevice deviceDescriptors: (const FDHIDDeviceDesc*) pDeviceDescriptors
{
    self = [super initWithDevice: pDevice deviceDescriptors: pDeviceDescriptors];
    
    if (self != nil)
    {
        mFnKeyIsDown = NO;
    }
    
    return self;
}



+ (NSUInteger) usagePage
{
    return kHIDPage_GenericDesktop;
}



+ (NSUInteger) usage
{
    return kHIDUsage_GD_Keyboard;
}



- (void) setFnKeyState: (BOOL) isDown
{
    mFnKeyIsDown = isDown;
}



- (void) flush
{
    [self setFnKeyState: NO];
}



- (void) evaluateEvent: (NSEvent*) event
{
    const NSEventType   eventType = [event type];
    FDHIDEvent          keyEvent = { 0 };
    
    keyEvent.mDevice    = self;
    keyEvent.mType      = eFDHIDEventTypeKeyboard;
    
    switch (eventType)
    {
        case NSKeyDown:
        case NSKeyUp:
            {
                NSString*           characters      = [event charactersIgnoringModifiers];
                const NSUInteger    numCharacters   = [characters length];
                
                keyEvent.mBoolVal = (eventType == NSKeyDown);
                
                for (NSUInteger i = 0; i < numCharacters; ++i)
                {
                    NSUInteger  flags       = [event modifierFlags];
                    unichar     character   = [characters characterAtIndex: i];
                    
                    if ((character & 0xFF00) ==  0xF700)
                    {
                        character -= 0xF700;
                        
                        if (character < FD_SIZE_OF_ARRAY (gInSpecialKey))
                        {
                            if (mFnKeyIsDown == YES)
                            {
                                keyEvent.mButton = gInSpecialKeyFn[character];
                            }
                            else
                            {
                                keyEvent.mButton = gInSpecialKey[character];
                            }
                            
                            [self pushEvent: &keyEvent];
                        }
                    }
                    else
                    {
                        if (flags & NSNumericPadKeyMask)
                        {
                            UInt16 keyPad = [event keyCode];
                            
                            if (keyPad < FD_SIZE_OF_ARRAY (gInNumPadKey))
                            {
                                if (gInNumPadKey[keyPad] != 0)
                                {
                                    keyEvent.mButton = gInNumPadKey[keyPad];
                                    
                                    [self pushEvent: &keyEvent];
                                    break;
                                }
                            }
                        }
                        
                        if (character < 0x80)
                        {
                            if ((character >= 'A') && (character <= 'Z'))
                            {
                                character += 'a' - 'A';
                            }
                            
                            keyEvent.mButton = character;
                            
                            [self pushEvent: &keyEvent];
                        }
                    }
                }
            }
            break;
            
        case NSFlagsChanged:
            {
                static NSUInteger   lastFlags       = 0;
                static int command_down;
                static int option_down;
                static int shift_down;
                static int control_down;
                const NSUInteger    flags           = [event modifierFlags];
                const NSUInteger    filteredFlags   = flags ^ lastFlags;
                unsigned short scancode = [event keyCode];
#ifdef DEBUG_EVENTS 
                Con_Printf ("Scancode is %i", (int)scancode);
#endif
                lastFlags = flags;

#ifdef DEBUG_EVENTS 
                unsigned long checkoflags = [[NSApp currentEvent]modifierFlags];
                unsigned long commandz = (checkoflags & NSCommandKeyMask);
                unsigned long optionz = (checkoflags & NSAlternateKeyMask);
                unsigned long controlz = (checkoflags & NSControlKeyMask);
                unsigned long shiftz = (checkoflags & NSShiftKeyMask);

                Con_Printf ("Command %i Control %i options %i scancode %i\n", (int)commandz, (int)controlz, (int)optionz, (int)scancode);
#endif // DEBUG_EVENTS 

                switch (scancode)
                {                   
                case 55: // Command
                        keyEvent.mButton = K_ALT;
                        keyEvent.mBoolVal = command_down = !command_down;
#ifdef DEBUG_EVENTS 
                        Con_Printf ("Key ALT is %i\n", keyEvent.mBoolVal);
#endif // DEBUG_EVENTS
                        [self pushEvent: &keyEvent];
                        break;
                case 56: // Shift
                        keyEvent.mButton = K_SHIFT;
                        keyEvent.mBoolVal = shift_down = !shift_down;
#ifdef DEBUG_EVENTS 
                        Con_Printf ("Key SHIFT is %i\n", keyEvent.mBoolVal);
#endif // DEBUG_EVENTS
                        [self pushEvent: &keyEvent];
                        break;
#if 0
                case 57: // Capslock
                        keyEvent.mButton = eFDHIDKeyCapsLock;
                        keyEvent.mBoolVal = (flags & NSAlphaShiftKeyMask) ? YES : NO;
                        
                        [self pushEvent: &keyEvent];
                        break;
#endif
                case 58: // Option
                        keyEvent.mButton = K_WINDOWS;
                        keyEvent.mBoolVal = option_down = !option_down;
#ifdef DEBUG_EVENTS 
                        Con_Printf ("Key OPTION is %i\n", keyEvent.mBoolVal);
#endif // DEBUG_EVENTS
                        [self pushEvent: &keyEvent];
                        break;
                case 59: // Control
                        keyEvent.mButton = K_CTRL;
                        keyEvent.mBoolVal = control_down = !control_down;
#ifdef DEBUG_EVENTS 
                        Con_Printf ("Key CONTROL is %i\n", keyEvent.mBoolVal);
#endif // DEBUG_EVENTS
                        [self pushEvent: &keyEvent];
                        break;
                        
                case 60: // Right shift
                    break; // Not supported
                case 61: // Other option
                    break; // Not supported
                case 62: // Other control
                    break; // Not supported
                default:
                    break;
                }
            }
            break;
            
        default:
            break;
    }
}

@end



void FDHIDKeyboard_KeyHandler (id device, unsigned int virtualKey, IOHIDValueRef pValue, IOHIDElementRef pElement)
{
    FD_UNUSED (pElement);
    FD_ASSERT (pValue != nil);
    
    const BOOL  isDown  = (IOHIDValueGetIntegerValue (pValue) != 0);
    CGEventRef  cgEvent = CGEventCreateKeyboardEvent (NULL, virtualKey, isDown);
    NSEvent*    event   = [NSEvent eventWithCGEvent: cgEvent];
        
    [device evaluateEvent: event];
        
    CFRelease( cgEvent );
}



void FDHIDKeyboard_FnHandler (id device, unsigned int keycode, IOHIDValueRef pValue, IOHIDElementRef pElement)
{
    FD_UNUSED (keycode, pElement);
    FD_ASSERT (pValue != nil);
    
    FDHIDEvent  keyEvent = { 0 };
    
    keyEvent.mDevice    = device;
    keyEvent.mType      = eFDHIDEventTypeKeyboard;
    keyEvent.mBoolVal   = NO;
    
    [device setFnKeyState: (IOHIDValueGetIntegerValue (pValue) != 0)];
    
    for (uint32_t i = 0; i < FD_SIZE_OF_ARRAY (gInSpecialKey); ++i)
    {
        if (gInSpecialKey[i] != 0)
        {
            keyEvent.mButton = gInSpecialKey[i];
            
            [device pushEvent: &keyEvent];
        }
    }
    
    for (uint32_t i = 0; i < FD_SIZE_OF_ARRAY (gInSpecialKeyFn); ++i)
    {
        if (gInSpecialKeyFn[i] != 0)
        {
            keyEvent.mButton = gInSpecialKeyFn[i];
            
            [device pushEvent: &keyEvent];
        }
    }
}


