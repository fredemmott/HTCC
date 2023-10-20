---
parent: Device Setup
---

# Ultraleap

1. Make sure you have Gemini v5.16.0 (October 2023) or later installed
2. Set 'Hand tracking device' to 'OpenXR hand tracking'
3. *Optional*: enable pinch gestures
4. For v5.16.0, under 'Workarounds':
   - Turn on 'Ignore XR_FB_hand_tracking_aim_pose'
   - Turn on 'Force-enable XR_EXT_hand_tracking'
   - Turn on 'Force-enable XR_FB_hand_tracking_aim'

## DCS

You must use the multithreaded version of DCS, and remove the `LeapC.dll` from **both** the `bin` and `bin-mt` folders inside your DCS installation. Alternatively, rename them to `LeapC.dll.bak`.