---
nav_order: 1
---
# Home
{: .no_toc }

This project is an OpenXR API layer aimed to make it easy to click on cockpits in VR flight simulators.

This is **not** intended to be:
- usable as general-purpose hand tracking support: it is designed to work well with specific games (DCS and MSFS).
- a way to see your hands in VR: it priorities ability to point-and-click reliably over immersion. In-game hands/gloves will not line up with your hands, as this conflicts with the goal of providing reliable pointing - the instructions for DCS include making the hands/gloves invisible. **This can only be solved by game developers adding/improving built-in hand-tracking support or deeper modifications to the game itself. It can not be improved by HTCC without sacrificing usability.**

1. TOC
{:toc}

## Game Compatibility

| Game | Compatible? | Notes |
|------|-------------|-------|
| MSFS | ✅ | |
| DCS World | ✅ | Must use OpenXR |
| HelloXR | ✅ | Use [hello_xr.reg] |
| All others | ❌ | |

If you have a PointCTRL:
- HTCC is **optional** for DCS
- HTCC is **required** for MSFS
- if you are using HTCC:
  - the custom firwmare is **required**
  - you must calibrate using the included calibration app instead of the standard calibration procedure
- if you are not using HTCC, you **must not** use the custom firmware


## Hardware Compatibility

The following hand trackers are supported:

| Tracker                    | Compatible? | Notes                                             |
|----------------------------|-------------|---------------------------------------------------|
| Quest 2                    | ✅          | Requires Oculus Developer Mode or Virtual Desktop |
| Quest Pro                  | ✅          | Requires Oculus Developer Mode or Virtual Desktop |
| Quest 3                    | ✅          | Requires Oculus Developer Mode or Virtual Desktop |
| PointCTRL                  | ✅          | Requires custom firmware                          |
| Ultraleap                  | ✅          | Includes Pimax hand tracking modules              |
| Other OpenXR hand trackers | 🧪          | untested, but *should* work                       |
| All others                 | ❌          | Includes HP Reverb G2, Varjo Aero                 |

- **The HP Reverb G2 does not have the required hardware or driver support**
- **The Varjo Aero does not have the required hardware support**
- Unlisted hand trackers *may* work if they support `XR_EXT_hand_tracking`
- All headsets (including the G2 and Aero) can be used with HTCC if you have additional hardware that is compatible, such as an Ultraleap or PointCtrl.

PointCTRL can be used by themselves, or in combination with any other device; for example, you can point with Quest hand tracking, but clickwith the PointCTRL FCUs.

If you have neither, I recommend ordering a [PointCTRL].


- [Getting started](getting-started.md)
- [PointCTRL](hardware/pointctrl/README.md)
- [Oculus hand tracking](hardware/oculus-hand-tracking/README.md)
- [Ultraleap](hardware/ultraleap/README.md)
- [DCS World](games/dcs-world/README.md)
- [MSFS](games/msfs/README.md)


[OpenComposite]: https://gitlab.com/znixian/OpenOVR/-/tree/openxr#downloading-and-installation
[`XR_FB_hand_tracking_aim`]: https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_FB_hand_tracking_aim
[`XR_EXT_hand_tracking`]: https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_EXT_hand_tracking
[PointCTRL]: https://pointctrl.com/
[hello_xr.reg]: https://github.com/fredemmott/HTCC/blob/master/reg/hello_xr.reg
