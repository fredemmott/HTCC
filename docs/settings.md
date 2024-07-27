---
nav_order: 7
---

# Registry Settings
{: .no_toc }

All supported settings are in the HTCC settings app; this page is for advanced users only.

**ALL REGISTRY SETTINGS ARE UNSUPPORTED AND NO HELP IS AVAILABLE** - they are documented primarily for my own reference, but may be useful for advanced users.

All the settings are in the registry, in `HKEY_LOCAL_MACHINE\SOFTWARE\Fred Emmott\HandTrackedCockpitClicking`; per-app overrides are in `HKEY_LOCAL_MACHINE\SOFTWARE\Fred Emmott\HandTrackedCockpitClicking\AppOverrides\EXECUTABLE_NAME.exe\`, e.g. `AppOverrides\DCS.exe\`

Defaults are in [Config.h](https://github.com/fredemmott/HTCC/blob/master/src/lib/Config.h) and change between versions.

## Table of Contents
{: .no_toc, .text-delta }

1. TOC
{:toc}

## OpenXR hand tracking wake/sleep/false clicks

OpenXR hand tracking is not perfectly accurate, so HTCC ignores movement and gestures unless conditions are met. These settings can tweak the conditions.

### HandTrackingWakeHFOV, HandTrackingWakeVFOV

STRING

Field of view - in radians - that the hand must be in for hand tracking to activate.

### HandTrackingActionHFOV, HandTrackingActionVFOV

STRING

Field of view - in radians - that the hand must be in for the start of a gesture to be recognized.

### HandTrackingWakeMilliseconds

DWORD

Number of milliseconds that the hand must be in the Active FOV for hand tracking to activate.

### HandTrackingSleepMilliseconds

DWORD

Number of milliseconds that the hand must be outside of the Sleep FOV for hand tracking to deactivate.

### HandTrackingWakeSleepBeeps

DWORD

Play beeps when hand tracking wakes up or sleeps. This is useful when tweaking the wake/sleep settings,
but generally not for normal use.

- 0: No wake/sleep beeps
- 1: Beep 'hi-lo' when sleeping, 'lo-hi' when waking

## OpenXR Hand Tracking Hibernate Settings

'Hibernate' lets you completely disable OpenXR Hand Tracking until you re-enable it. To toggle hibernation on/off, hold one hand near the top of your field of view until you hear the beeps and the input stops.

### HandTrackingHibernateGestureEnabled

DWORD

- 1 (default): The hibernate gesture is recognized
- 0: the hibernate gesture will be ignored

This setting requires HTCC v1.1 or above, which will also include an option in the settings app.

### HandTrackingHibernateCutoff

STRING

Angle above 'straight ahead' (in radians) that the hand needs to be above to enable hibernation.

### HandTrackingHibernateMilliseconds

DWORD

Time that the hibernate gesture must be held.

### HandTrackingHibernateIntervalMilliseconds

DWORD

After a hibernate on/off gesture is recognized, HTCC will ignore the gesture for this amount of time. This is to avoid accidentally double-toggling it.

### HandTrackingHibernateBeeps

DWORD

- 0: No hibernation beeps
- 1: beep 'hi-lo-hi-lo' when hibernating, 'lo-hi-lo-hi' when waking from hibernation

## Other OpenXR Hand Tracking Settings

### HandTrackingHands

DWORD

- 0: both hands are tracked
- 1: only the left hand is tracked
- 2: only the right hand is tracked

## Scroll behavior

### ScrollWheelDelayMilliseconds

DWORD, number of milliseconds between first and second wheel tick.

### ScrollWheelIntervalMilliseconds

DWORD, number of milliseconds between subsequent wheel ticks.

### VRControllerScrollAccelerationDelayMilliseconds 

DWORD, number of milliseconds to wait before speeding up the scroll acceleration when using VR controllers.

## PointCTRL settings

### PointCtrlFCUMapping

DWORD:

- 0: Disabled
- 1: Classic
- 2: Modal (intended for MSFS, but no longer required due to other changes)
- 3: Modal, but long-pressing buttons 1 & 2 together enters scroll lock mode but with  button 1 held
- 4: For custom devices; FCU3 is ignored, `GameControllerWheelUpButton` and `GameControllerWheelDownButton` are used instead

### ShortPressLongPressMilliseconds

DWORD, number of milliseconds for the boundary between a short press and a long press; this affects the PointCTRL 'modal' bindings.

### ProjectionDistance

String (SZ) containing a distance in meters to project virtual hands when using a PointCtrl. For example, `0.6` for 60cm.

### PointCtrlCenterX, PointCtrlCenterY, PointCtrlRadiansPerUnitX, PointCtrlRadiansPerUnitY

CenterX and Y are DWORD 0..65535
PointCtrlRadiansPerUnitX and Y are floats as strings.

These values should be set with the included `PointCtrlCalibration.exe` program.

## General settings

Most of these are in the settings app.

### Enabled

DWORD 0 (disabled) or 1 (enabled); enable or disable the entire API layer.

This should usually be 0 to disable for the majority of games, but overriden to 1 in `AppOverrides\DCS.exe`.

### PointerSource

DWORD: which device is used to set the cursor location

- 0: Oculus hand tracking
- 1: PointCtrl

### PointerSink

DWORD: which device is emulated for pointing

- 0: touch screen (classic PointCtrl)
- 1: VR controllers

### ClickActionSink

DWORD: which device is emulated for clicking

- 0: match PointerSink
- 1: touch screen/mouse
- 2: VR controllers

### ScrollActionSink

DWORD: which device is emulated for scrolling

- 0: match PointerSink
- 1: touch screen/mouse
- 2: VR controllers

### VerboseDebug

DWORD: 0..n

- 0: off
- 1: verbose initialization, major state changes
- 2: periodic changes
- 3: every frame
- 4...n: increasingly spammy

Debug output is visible in a debugger or DebugView

### PinchToClick

DWORD 0 (disabled) or 1 (enabled): use Oculus hand tracking pinch gestures to click

### PinchToScroll

DWORD 0 (disabled) or 1 (enabled): use Oculus hand tracking pinch gestures to scroll

### OneHandOnly

DWORD 0 (disabled) or 1 (enabled): only render one controller at a time.

Some games (including DCS) assume that you pick up one controller at a time to point at one specific thing; with real controllers like DCS is designed for, this works well and is reasonable. For hands, its' common to have both visible, but DCS will always use the left controller for pointing if two controllers are active, even if only one is visible.

With this option enabled (default), it will prefer:
1. whichever is performing any gestures, if any
2. whichever is currently closer to the center of the field of view

### VRControllerActionSinkMapping 

DWORD:

- 0: [DCS](dcs-world/README.md)
- 1: [MSFS](msfs/README.md)

Which game's mappings are used for VR controllers.

## SmoothingFactor

STRING

Value between `0.0` and `1.0` weighing the two most recent inputs. A value of `1.0` is equivalent to no smoothing, and a value of `0.5` averages the last two frames (maximum smoothing). `0.0` entirely uses the previous frame's data,
and is not generally useful.

## Rendering offset

### VRVerticalOffset

String (SZ) containing a distance in meters, e.g. `-0.04` (4cm down).

The emulated VR controller is positioned slightly below your hand, and angled slightly up to make the 'laser pointer' more visible. This adjusts how far below it is.

The pointing angle is based on this distance, combined with the `VRFarDistance` setting.

### VRFarDistance

String (SZ) containing a distance in meters, e.g. `0.8` (80cm away).

Rough distance from your head to the majority of things you want to interact with. Combined with `VRVerticalOffset`, this sets the upwards rotation of the pointer; a shorter 'far distance' will increase the amount of rotation, a longer one will reduce it:

![upwards tilt = atan(offset / (far distance - hand to headset distance)](tilt-angle.png)

## Controller emulation tweaks

### VRControllerGripSquueze

DWORD

- 0: Never
- 1: When tracking

Some games (such as DCS) require the grip to be squeezed to consider the controller active. Others consider it an active input.

### VRControllerPointerSinkWorldLock 

DWORD

- 0: clicking does not affect tracking
- 1 - orientation: any click/wheel action locks the angle of the pointer, but not the position
- 2 - hard-lock orientation, soft-lock position: as above, however the position will also be unlocked until your hand moves a certain distance

### VRControllerPointerSinkSoftWorldLockDistance

STRING

Distance in meters (e.g. "0.05" for 5cm). While 'clicking', the virtual controllers will stay locked in position until your hand moves this far from the original position.

### VRControllerActionSinkSecondsPerRotation

STRING

Number of seconds for a mouse wheel event to trigger a full rotation of the controller when using MSFS bindings. For example, "4.0"

## Custom button boxes

If you don't have a PointCTRL, but have a different button box that appears as a USB joystick, it can also be used.

- pointing: main X Y axis
- clicking: buttons

Devices can be used for pointing, clicking, or both.

### PointCtrlVID, PointCtrlPID

DWORDs containing 16-bit USB vendor and product IDs.

Change the USB vendor ID and product ID that this project looks for when trying to find a PointCTRL device.

### PointCtrlFCUButtonL1-L3, PointCtrlFCUButtonR1-R3

DWORD button indices starting at 0.

FCU L1 is the button farthest from your wrist on your left hand, L3 is closest. R1-R3 are the buttons on your right hand.

### GameControllerLWheelUpButton, GameControllerRWheelUpButton

DWORD button indices starting at 0

If you want to use separate buttons for 'wheel up' and 'wheel down' instead of emulating a pointctrl, set `PointCtrlFCUMapping` to `4` (dedicated scroll buttons), and this to a desired button index.

The naming of 'wheel up' is based on mouse events, where 'up' means correlates with moving the top of the mouse wheel away from you - traditionally, scrolling up. In-game, 'scrolling up' will usually decrease a value, but 'scrolling down' will usually increase a value.

### GameControllerLWheelDownButton, GameControllerRWheelDownButton

DWORD button indices starting at 0

If you want to use separate buttons for 'wheel up' and 'wheel down' instead of emulating a pointctrl, set `PointCtrlFCUMapping` to `4` (dedicated scroll buttons), and this to a desired button index.

The naming of 'wheel up' is based on mouse events, where 'up' means correlates with moving the top of the mouse wheel away from you - traditionally, scrolling up. In-game, 'scrolling up' will usually decrease a value, but 'scrolling down' will usually increase a value.

## Developers only

Changing these settings can have unexpected effects; parts of HTCC may assume they're at their default values and break if they're changed.

### UseHandTrackingAimPointFB

DWORD 0 (disabled) or 1 (enabled): Use the FB-provided 'aim' point instead of a skeletal joint.

### HandTrackingAimJoint

DWORD: joint index for aim joint. See the OpenXR specification.

### HandTrackingOrientation

DWORD:

- 0 (raw): The hand joint orientation/rotation reported by OpenXR is used unmodified
- 1 (ray cast): Discard the orientation/rotation of your hand, and instead rotate it to fit a laser pointer from the center of your headset. This will improve accuracy and stability, but if controller models are enabled in-game, it may feel weird. Leaving this on is recommended.
- 2 (ray cast with reprojection): ray cast, then re-project to `ProjectionDistance`. Workaround for DCS 'touch to interact' being forced-on.

### EnableFBOpenXRExtensions

DWORD 0 (disabled) or 1 (enabled): Enable the use of Facebook/Meta/Oculus-specific OpenXR extensions for hand tracking. This is required for the pinch gestures.

To disable pinch gestures, use the `PinchToClick` and `PinchToScroll` settings instead. This is primarily intended for checking that this project can function without them.

### VirtualControllerInteractionProfilePath

STRING: OpenXR path to the interaction profile of the emulated controllers.

### Quirk_Conformance_ExtensionCount

DWORD:
- 1: enabled (default)
- 0: disabled

As of 2024-07-22, the Meta Link PTC runtime has a bug where it inappropriately returns `XR_ERROR_SIZE_INSUFFICIENT` from `xrEnumerateInstanceExtensionProperties()` when `propertyCapacityInput` is 0; this registry setting enables/disables a workaround, which (incorrectly) makes HTCC consider `XR_ERROR_SIZE_INSUFFICIENT` to also indicate success for this call.