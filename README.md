**THIS IS AN EXPERIMENT - IT IS NOT READY FOR USE**. Do not ask for help using or installing it, etc.

# EXPERIMENT: Hand-tracked cockpit clicking for VR flight simulators

This project is an OpenXR API layer aimed to make it easy to click on cockpits in VR flight simulators. Currently only DCS World is supported.

Pointer input can either come from:
- OpenXR hand tracking (any OpenXR-compatible hand tracking, including Quest 2 or Quest Pro headsets)
- [PointCTRL] with custom firmware

If you already have OpenXR hand tracking, you have everything you need.

A PointCTRL will work with any VR headset, and also often has better tracking when your hands are higher than your head, especially on a Quest 2. The importance here varies by aircraft - for example, it's much more important in a Ka-50 than an A-10C due to the location of the countermeasures system.

Interactions (mouse clicks etc) can either come from:
- Hand tracking **on Oculus headsets**, including Quest 2 or Quest pro
- PointCTRL with custom firmware

Oculus hand tracking gesture recognition does not require additional hardware, however PointCTRL clicks are more reliable, and give a better tacticle feel.

You can mix-and-match, e.g. use hand tracking for position, but PointCTRL FCUs for clicks.

Game support is provided either with:
- a virtual touchscreen
- a virtual Oculus Touch controller

The virtual touch screen feels fairly close to a PointCTRL with default firmware; the key limitations are that it only works with DCS, and it is not possible to click on things at the bottom of your field of view.

The virtual Oculus Touch controller is designed to be used as a 'laser pointer', and intentionally does not line up precisely with your hand: instead, the pointing direction is in a straight line between the center of your view and the PointCTRL or the first knuckle of your hand. This gives much better precision and stability, however you are likely to want to disable in-game controller models.

## DCS

[OpenComposite] is required, and DCS World must be launched in SteamVR mode.

In controller emulation mode, DCS will show gloved hands; you probably want to hide these by setting the 'scale' to 0 in `CoreMods\characters\models\glove_L.chanimgpu` and `glove_R.chanimgpu`.

If using a PointCTRL with custom firmware, unbind the PointCTRL axes in DCS control bindings.

### Oculus hand tracking gesture controls

- Index finger pinch to thumb: left click/tVR thumbstick down
- Middle finger pinch to thumb: right click/VR thumbstick up
- Ring finger pinch to thumb: scroll wheel up/VR thumbstick left
- Little finger pinch to thumb: scroll wheel down/VR thumbstick right

## FAQ

### Why do pinch gestures require Oculus headsets?

The [`XR_FB_hand_tracking_aim`] OpenXR extension is required, which is currently only supported by Oculus headsets.

Other headsets would work if other OpenXR runtimes add support for `XR_FB_hand_tracking_aim`, or if other projects emulate it using [`XR_EXT_hand_tracking`] or vendor-specific APIs. I am not aware of any projects doing this at present.

Standard OpenXR hand tracking via `XR_EXT_hand_tracking` does not provide enough similar precise gestures.

### Why only DCS World?

I don't want the complexity of making the control scheme configurable; this project disables itself for all other programs as the thumbstick mapping is unlikely to be useful, and it may interfere with gameplay.

### Why is OpenComposite and SteamVR mode needed, when DCS World has native support for Oculus headsets?

Hand tracking is only supported by Meta in OpenXR games, so OpenComposite must be used to run DCS using OpenXR.

DCS must be launched in SteamVR mode because OpenComposite works by pretending to be SteamVR.

## License

This project is licensed under the [MIT license].

[OpenComposite]: https://gitlab.com/znixian/OpenOVR/-/tree/openxr#downloading-and-installation
[`XR_FB_hand_tracking_aim`]: https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_FB_hand_tracking_aim
[`XR_EXT_hand_tracking`]: https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_EXT_hand_tracking
[MIT license]: LICENSE
[PointCTRL]: https://pointctrl.com/
