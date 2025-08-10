---
parent: Device Setup
nav_order: 1
---

# Oculus Hand Tracking

As of 2025-08-10, Meta appear to have removed support for hand tracking via Link, even in developer mode. Use Virtual Desktop or Steam Link instead.

## Virtual Desktop

If you choose to use Virtual Desktop instead of Link/AirLink:

- you must use VDXR
- turn on 'send tracking data' so that your PC - and HTCC - receive the hand tracking data
- leave hand tracking in Virtual Desktop turned *on*
- if you have a Quest Pro, as of 2024-11-26, Virtual Desktop can not enable hand tracking unless face and body tracking
  are also enabled on your headset

## Sleep/wake

Hand tracking and gesture recognition is not perfectly reliable; it will hopefully improve with future Oculus firmware
updates.

HTCC reduces the changes of accidental clicks by:

- adding sleep/wake mode
- ignoring gestures unless you're looking close to your hand

'Wake' hand tracking by waving your hand close to the center of your view. Tracking will 'sleep' for that hand after a
few seconds of little movement.

The limits can be tweaked [in the registry](../../settings.md) if needed.

## FAQ

### Why do pinch gestures require Oculus headsets?

The [`XR_FB_hand_tracking_aim`] OpenXR extension is required, which is currently only supported by Oculus headsets.

Other headsets would work if other OpenXR runtimes add support for `XR_FB_hand_tracking_aim`, or if other projects
emulate it using [`XR_EXT_hand_tracking`] or vendor-specific APIs. I am not aware of any projects doing this at present.

Standard OpenXR hand tracking via `XR_EXT_hand_tracking` does not provide enough similar precise gestures.
