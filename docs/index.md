---
nav_order: 1
---
# Home
{: .no_toc }

This project is an OpenXR API layer aimed to make it easy to click on cockpits in VR flight simulators.

This is **not** intended to be or usable as general-purpose hand tracking support: it is designed to work well with specific games (DCS and MSFS).

1. TOC
{:toc}

## Game Compatibility

| Game | Compatible? | Notes |
|------|-------------|-------|
| MSFS | ✅ |  |
| DCS World | ✅ | Stable requires [OpenComposite] |
| All others | ❌ | |

If you have a PointCTRL:
- you must calibrate using the included calibration app instead of the standard calibration procedure
- the custom firmware is **required** for MSFS
- the custom firmware is *optional* in DCS, however if you are using the custom firmware, DCS **must** be using [OpenComposite] or native OpenXR support (Open Beta only)

To use PointCTRL in Stable DCS without OpenComposite, you must restore the standard firmware.

## Hardware Compatibility

| VR Headset   | Have a PointCTRL? | Good to go? | Notes |
|--------------|-------------------|-------------|-------|
| Quest 2      | ✓                  | ✅ | |
| Quest Pro    | ✓                  | ✅ | |
| All others   | ✓                  | ✅ | Includes HP Reverb G2 |
| Quest 2      | ✗                 | ✅ | Requires an Oculus developer account |
| Quest Pro    | ✗                 | ✅ | Requires an Oculus developer account |
| All others   | ✗                 | ❌ | Includes HP Reverb G2 |

- **The HP Reverb G2 does not have the required hardware or driver support**
- **The Varjo Aero does not have the required hardware support**

If you have a Quest 2 or Quest Pro *and* a PointCTRL, you can use either, or combine them - e.g. pointing with Quest hand tracking but clicking with the PointCTRL FCUs.

If you have neither, I recommend ordering a [PointCTRL].

### UltraLeap/Leap Motion/Pimax Hand Tracking

As long as OpenXR support is enabled in Ultraleap settings, an ultraleap can be used for *pointing*, but not clicking; you will need a separate device.

The PointCTRL finger-mounted buttons can be used (with the HMS being used as a dongle, not as a tracker).

Other devices can also be used if Windows considers them a joystick/gamepad with a VID+PID - take a look at [docs/settings.md](settings.md).

## Installation and setup

- [Getting started](getting-started.md)
- [PointCTRL](hardware/pointctrl/README.md)
- [Oculus hand tracking](hardware/oculus-hand-tracking/README.md)
- [DCS World](games/dcs-world/README.md)
- [MSFS](games/msfs/README.md)


[OpenComposite]: https://gitlab.com/znixian/OpenOVR/-/tree/openxr#downloading-and-installation
[`XR_FB_hand_tracking_aim`]: https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_FB_hand_tracking_aim
[`XR_EXT_hand_tracking`]: https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_EXT_hand_tracking
[PointCTRL]: https://pointctrl.com/
