---
parent: Device Setup
nav_order: 4
---

# Ultraleap

1. Make sure you have Gemini v5.16.0 (October 2023) or later (including Hyperion) installed
2. Set 'Hand tracking device' to 'OpenXR hand tracking'
3. *Optional*: enable pinch gestures
4. For v5.16.0, under 'Workarounds':
    - Turn on 'Ignore XR_FB_hand_tracking_aim_pose'
    - Turn on 'Force-enable XR_EXT_hand_tracking'
    - Turn on 'Force-enable XR_FB_hand_tracking_aim'

## DCS

You must use the multithreaded version of DCS, and remove the `LeapC.dll` from **both** the `bin` and `bin-mt` folders
inside your DCS installation. Alternatively, rename them to `LeapC.dll.bak`.

You may need to repeat these steps after any DCS update or repair.

## Adjusting the position

See [Ultraleap's documentation](https://docs.ultraleap.com/openxr/configuration/index.html#configuration-file), and
contact Ultraleap support if you need assistance.

### Pimax's hand tracking module

This should show up in Ultraleap's software as a "Stereo IR 170", and your hands likely appear ~ 10-15cm too high; this
can be fixed by following Ultraleap's documentation above; you should start with a tilt angle of 25 degrees, e.g:

```json
{
  "pos": [
    0.0,
    0.0,
    0.0
  ],
  "tilt_angle": 25
}
```

You can also [download this `api_layer_config.json`](pimax/api_layer_config.json); follow Ultraleap's documentation,
create `%PROGRAMDATA%\Ultraleap\OpenXR` if necessary (this is *usually* equivalent to
`C:\ProgramData\Ultraleap\OpenXR\api_layer_config.json`), and put the .json file there. **If you need assistance,
contact Ultraleap support.**

## Troubleshooting

- Run [`OpenXR-API-Layers-Win64-HKLM.exe`](https://github.com/fredemmott/OpenXR-API-Layers-GUI/releases/latest) -
  download the zip linked in the description there.
- make sure that the Ultraleap layer is after HTCC; the 'Fix it!' buttons should do this
- you probably want to fix any other issues the tool finds
- if the game fails to start in VR with the 'Force-enable' options on, **you need to contact Ultraleap support** - the
  Ultraleap driver is not working correctly.

### Disabled by environment variable

As of 2025-09-10, if the environment variable `DISABLE_XR_APILAYER_ULTRALEAP_HAND_TRACKING_1` is set to *any* value (including `0`, `false`, `no`, etc), the Ultraleap layer will be disabled. This is part of OpenXR and the Ultraleap driver, not HTCC.

If the HTCC settings app says that Ultraleap is disabled by environment variable, you can try removing the environment variable from Windows settings; if it is not set there, it is possible for OpenXR runtimes to set the environment variable; contact your VR headset manufacturer for assistance.