---
parent: Device Setup
---

# Other headsets

## The original Pimax Crystal with the Pimax Hand Tracking module

This is an Ultraleap tracker; see [the Ultraleap instructions](ultraleap/README.md).

## All other headsets

HTCC should be compatible with *any* OpenXR-compatible tracker that supports:

- `XR_EXT_hand_tracking`
- `XR_FB_hand_tracking_aim`

`XR_FB_hand_tracking_aim` is optional, but is required for pinch gesture recognition. Without this extension, you will
also need to build or buy a clicking device, like a PointCTRL or SlugMouse.

### Getting help

Contact your hardware manufacturer's customer support for information and support.

If you believe you have found an OpenXR correctness issue in HTCC,
please [open a GitHub issue with details](https://github.com/fredemmott/HTCC/issues).

If you represent a hardware manufacturer and require assistance implementing these OpenXR extensions or otherwise
supporting HTCC, I can be reached [via email](mailto:htcc-commerical@fred.fredemmott.com) to inquire about my
availability for contract work.

## Pimax Crystal Light, Pimax Crystal Super

See [**All** other headsets](#all-other-headsets).