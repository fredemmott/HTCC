---
parent: Game Setup
---

# DCS World

## "PointCTRL Classic" (v0.2 and above)

If you are using a PointCTRL and don't want to use any OpenXR features (e.g. Quest or Ultraleap hand tracking, or controller emulation), you can use the 'HTCC PointCTRL Classic' entry in the start menu.

This will:
- always use PointCTRL, regardless of your settings
- always use the Virtual Touch Screen/Tablet, regardless of your settings
- **only** work with DCS

To launch, hit the Windows key, and type 'HTCC PointCTRL Classic', and run the item. You can then ignore the rest of this file.

"PointCTRL Classic" is mostly useful for people who want to keep using their PointCTRL with DCS without using OpenXR, but also want to use it with DCS.

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