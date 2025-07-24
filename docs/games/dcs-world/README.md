---
parent: Game Setup
---

# DCS World

## In-Game Options

Any other hand-tracking interferes with HTCC or entirely stops it from functioning.

- Turn off hand tracking in the game
- Turn off hand tracking controller emulation in any other tools except for device drivers - for example, OpenXR Toolkit
- Turn off Ultraleap in the game; if you have an Ultraleap device (including the Pimax hand tracking module), keep OpenXR support enabled in the Ultraleap driver settings, but do not enable it in game
- Unless you are using Oculus Touch controller emulation in HTCC (**not** recommended), turn off hand controller support in DCS

To test if DCS is configured correctly, *disable HTCC*, restart DCS, and move your hands; if your hands move in-game, you have some other hand tracking turned on which needs to be turned off. Once you have no hand tracking in DCS, re-enable HTCC.

## Quest via Link/AirLink

These steps are *only* for Link/AirLink, not Virtual Desktop, Steam Link, or any other tools. As of 2024-05-13, additional steps are needed for Link/AirLink because of a compatibility issue introduced into Meta's drivers.

1. Delete `dbghelp.dll` from DCS's `bin` and `bin-mt` folders; these will need to be deleted again every time DCS updates, and can be recovered by a DCS 'quick repair'
2. Follow the generic OpenXR steps below

## OpenXR

HTCC requires that DCS is using OpenXR.

1. If using controller emulation, you probably want to hide DCS's default gloved hands. Inside DCS's installation folder, find `CoreMods\characters\models\glove_L.chanimgpu` and `glove_R.chanimgpu` - inside these files, change the `scale` to `0`
2. If using a PointCTRL with custom firmware, unbind the PointCTRL X and Y axes within DCS, and unbind the buttons from all categories
3. If using touchscreen emulation (recommended), enable DCS's option to lock the mouse cursor to the window, and enable mouse support
4. If using a virtual VR controller, enable DCS's option to support touch controllers in VR settings

If using VR controller emulation, there doesn't appear to be a way to disable 'touch to interact'; be careful not to accidentally touch the controls. This is especially problematic for the fire suppression pull handles in the A10C. If accidental control interactions are a problem, switch to 'Virtual Touch Screen' mode - though you may then need to use a mouse for menus.

If hand tracking interferes with the mouse, you can temporarily disable HTCC by holding one hand above your head for a few seconds, near the top of your field of view - you should hear a few beeps. You can re-enable it by repeating the gesture.

### OpenComposite

- HTCC will no longer be tested with OpenComposite
- no support is available for HTCC with OpenComposite
- no bug reports related to OpenComposite will be worked on

That said, as of 2023-07-01, HTCC does still work with OpenComposite; there are no HTCC-specific installation or configuration steps required. If it does not work for you, use DCS's native OpenXR support instead.

## Controls

- 'left click'/index finger pinch to thumb: left click/VR thumbstick down
- 'right click'/ Middle finger pinch to thumb: right click/VR thumbstick up
- 'scroll up'/Ring finger pinch to thumb: scroll wheel/VRthumbstick left (decrease value)
- 'scroll down'/Little finger pinch to thumb: scroll wheel/VR thumbstick right (increase value)

If you're using the Quest hand tracking gestures, you will need to use the real mouse instead in some DCS menus because of DCS handling mouse differently in some menus compared to in-game; this conflicts with HTCC's attempts to reduce accidental/incorrect clicks.

### PointCTRL FCU buttons

- FCU1: left click
- FCU2: right click
- FCU3: if FCU1 was pressed more recently than FCU2, scroll down; otherwise, scroll up
