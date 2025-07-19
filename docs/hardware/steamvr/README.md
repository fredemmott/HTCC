---
parent: Device Setup
---

# SteamVR Hand Tracking

Starting with HTCC v1.3.6 (currently in beta), HTCC is compatible with SteamVR's OpenXR runtime. As of July 2025,
future versions of SteamVR are expected to increase compatibility with stable versions of HTCC.

## Requirements

- Your game must be using OpenXR, *NOT* OpenVR
- As of July 2025, SteamVR does not support pinch gesture recognition
    - This would require Valve to implement the `XR_FB_hand_tracking_aim` OpenXR extension
- As of July 2025, despite SteamVR itself supporting OpenXR hand tracking:
    - the vast majority of SteamVR *headsets* do not support hand tracking
    - the vast majority of SteamVR *drivers* do not support hand tracking

In practice, this means that to use HTCC on SteamVR, you will probably need *both*:

- A Quest-series headset using the Steam Link app
- A clicking device like a PointCTRL or SlugMouse

As SteamVR does not support pinch gesture recognition, you will not be able to interact with cockpits without a suitable
clicking device.