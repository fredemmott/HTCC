---
parent: Device Setup
nav_order: 5
---

# Other Headsets

## The original Pimax Crystal with the Pimax Hand Tracking module

This is an Ultraleap tracker; see [the Ultraleap instructions](ultraleap/README.md).

## All other headsets

HTCC should be compatible with *any* OpenXR-compatible tracker running on a conformant OpenXR runtime, as long as the combination supports:

- `XR_EXT_hand_tracking`
- `XR_FB_hand_tracking_aim`

`XR_FB_hand_tracking_aim` is optional, but is required for pinch gesture recognition. Without this extension, you will
also need to build or buy a clicking device, like a PointCTRL or SlugMouse.

## Pimax Crystal Light, Pimax Crystal Super

See [*All* other headsets](#all-other-headsets).

### Getting help

Contact your hardware manufacturer's customer support for information and support.

If you believe you have found an OpenXR correctness issue in HTCC,
please [open a GitHub issue with details](https://github.com/fredemmott/HTCC/issues).


### Hardware manufacturers

If you represent a hardware manufacturer and would like assistance implementing these OpenXR extensions or otherwise
supporting HTCC, I can be reached [via email](mailto:htcc-commerical@fred.fredemmott.com) to inquire about my
availability for contract work.

HTCC requires [the extensions mentioned above](#all-other-headsets); additionally, *runtimes* and *API layers* **MUST** return an error from `xrCreateApiLayerInstance()`/`xrCreateInstance()` if an unsupported extension is requested.

This is because if the API layer is not the *last* API layer, OpenXR only allows API layers to query what extensions are provided by the *next* API layer, not the runtime, and not any other API layers. The only way an API layer can reliably determine if an extension is available is by trying to enable it in `xrCreateApiLayerInstance()`, then checking for failure.

You *can not* test this error behvaior with a normal OpenXR application - you *must* test this with an API layer, or by directly testing your runtime's dll without using the OpenXR loader. This is because when an *application* calls `xrCreateInstance()`, the OpenXR loader will read the JSON manifest files, and if an unsupported extension is requested, the loader will return `XR_ERROR_EXTENSION_NOT_PRESENT` *without calling the runtime implementation*. However, as API layers are lower down the stack than the loader, when API layers attempt this, the runtime's implementation *is* invoked, instead of the OpenXR loader's implementation.