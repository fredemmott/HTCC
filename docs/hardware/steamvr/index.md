---
parent: Device Setup
nav_order: 3
---

# SteamVR Hand Tracking

As of July 2025:

- Stable SteamVR (v2.11.2) requires HTCC beta v1.3.6
- SteamVR beta (v2.12.6 or above) can be used with either stable or beta versions of HTCC

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