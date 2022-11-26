# Registry Settings

All settings are in the registry, in `HKEY_LOCAL_MACHINE\SOFTWARE\FredEmmott\DCSHandTracking`.

## Enabled

DWORD 0 (disabled) or 1 (enabled); enable or disable the entire API layer.

## PointerSource

DWORD: which device is used to set the cursor location

- 0: Oculus hand tracking
- 1: PointCtrl

## PointerSink

DWORD: which device is emulated

- 0: touch screen (classic PointCtrl)
- 1: VR controllers

## VerboseDebug

DWORD: 0..n

- 0: off
- 1: some
- 2: more
- 3: every frame
- 4...n: increasingly spammy

Debug output is visible in a debugger or DebugView

## MirrorEye

DWORD 0 or 1:

- 0: left eye
- 1: right eye

This affects cursor emulation, and should match the DCS mirror settings

## PinchToClick

DWORD 0 (disabled) or 1 (enabled): use Oculus hand tracking pinch gestures to click

## PinchToScroll

DWORD 0 (disabled) or 1 (enabled): use Oculus hand tracking pinch gestures to scroll

## PointCtrlFCUClicks

DWORD 0 (disabled) or 1 (enabled): use PointCtrl FCU button clicks to click or scroll

## PointCtrlProjectionDistance

String (SZ) containing a distance (in meters) to project virtual hands when using a PointCtrl. For example, "0.6" for 60cm.

## PointCtrlCenterX, PointCtrlCenterY, PointCtrlRadiansPerUnitX, PointCtrlRadiansPerUnitY

CenterX and Y are DWORD 0..65535
PointCtrlRadiansPerUnitX and Y are floats as strings.

These values should be set with the included `PointCtrlCalibration.exe` program.

## CheckDCS (developers only)

DWORD 0 (disabled) or 1 (enabled); do nothing in games other than DCS

# UseHandTrackingAimPointFB (developers only)

DWORD 0 (disabled) or 1 (enabled): Use the FB-provided 'aim' point instead of a skeletal joint

# HandTrackingAimJoint (developers only)

DWORD: joint index for aim joint. See the OpenXR specification.

# RaycastHandTrackingPose (developers only)

DWORD 0 (disabled) or 1 (enabled): Discard the orientation/rotation of your hand, and instead rotate it to fit a laser pointer from the center of your headset. This will improve accuracy and stability, but if controller models are enabled in-game, it may feel weird. Leaving this on is recommended.
