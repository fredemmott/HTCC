# Quest Handtracking for DCS World

This project emulates mouse events in DCS using hand tracking on Oculus headsets.

[OpenComposite] is required, and DCS World must be launched in SteamVR mode.

## Control mapping

- Index finger pinch: left click
- Middle finger pinch: right click

## FAQ

### Why only Oculus headsets?

The [`XR_FB_hand_tracking_aim`] OpenXR extension is required, which is currently only supported by Oculus headsets. I will not be adding support for other headsets.

Other headsets would work if other OpenXR runtimes add support for `XR_FB_hand_tracking_aim`, or if other projects emulate it using [`XR_EXT_hand_tracking`] or vendor-specific APIs. I am not aware of any projects doing this at present.

### Why only DCS World?

I don't want the complexity of making the control scheme configurable; this project disables itself for all other programs as the thumbstick mapping is unlikely to be useful, and it may interfere with gameplay.

### Why is OpenComposite and SteamVR mode needed, when DCS World has native support for Oculus headsets?

Hand tracking is only supported by Meta in OpenXR games, so OpenComposite must be used to run DCS using OpenXR.

DCS must be launched in SteamVR mode because OpenComposite works by pretending to be SteamVR.

## Why emulating a mouse instead of a controller?

DCS draws hands instead of a controller; it feels *really* weird.

DCS's mouse support also works nicely when turning away from a control while you're clicking it.

## License

This project is licensed under the [MIT license].

[OpenComposite]: https://gitlab.com/znixian/OpenOVR/-/tree/openxr#downloading-and-installation
[`XR_FB_hand_tracking_aim`]: https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_FB_hand_tracking_aim
[`XR_EXT_hand_tracking`]: https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_EXT_hand_tracking
[MIT license]: LICENSE
