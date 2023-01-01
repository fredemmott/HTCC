# Hand-tracked cockpit clicking for VR flight simulators

This project is an OpenXR API layer aimed to make it easy to click on cockpits in VR flight simulators.

This is **not** intended to be or usable as general-purpose hand tracking support: it is designed to work well with specific games (DCS and MSFS).

## Game Compatibility

| Game | Compatible? | Notes |
|------|-------------|-------|
| MSFS | ✅ |  |
| DCS World | ✅ | Requires [OpenComposite] |
| All others | ❌ | |

If you have a PointCTRL:
- you must calibrate using the included calibration app instead of the standard calibration procedure
- the custom firmware is **required** for MSFS
- the custom firmware is *optional* in DCS, however if you are using the custom firmware, DCS **must** be using [OpenComposite]

To use PointCTRL in DCS without OpenComposite, you must restore the standard firmware.

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

Other devices can also be used if Windows considers them a joystick/gamepad with a VID+PID - take a look at [docs/settings.md](docs/settings.md).

## Installation and setup

- [Downloads](https://github.com/fredemmott/hand-tracked-cockpit-clicking/releases/latest)
- [Getting started](docs/getting%20started.md)
- [PointCTRL](docs/pointctrl/README.md)
- [Oculus hand tracking](docs/oculus-hand-tracking/README.md)
- [DCS World](docs/dcs-world/README.md)
- [MSFS](docs/msfs/README.md)

## Getting help

**This is an experiment**; you should only use this if you're already comfortable with OpenComposite and other required projects, I have very limited time to support.

I make this for my own use, and I share this in the hope others find it useful; I'm not able to commit to support, bug fixes, or feature development.

First, check the documentation links above; if you still need help, support may be available from the community via the #help channel in [my Discord]. I am not able to respond to 1:1 requests for help via any means, including GitHub, Discord, Twitter, Reddit, or email.

## License

This project is licensed under the [MIT license].

[OpenComposite]: https://gitlab.com/znixian/OpenOVR/-/tree/openxr#downloading-and-installation
[`XR_FB_hand_tracking_aim`]: https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_FB_hand_tracking_aim
[`XR_EXT_hand_tracking`]: https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_EXT_hand_tracking
[MIT license]: LICENSE
[PointCTRL]: https://pointctrl.com/
[my Discord]: https://go.fredemmott.com/discord
