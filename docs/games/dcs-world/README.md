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

## OpenXR/OpenComposite

You **must** either use DCS's native OpenXR support, or OpenComposite. **If you are using the multithreaded beta, you can only use native OpenXR**.

1.If you are using [OpenComposite](https://gitlab.com/znixian/OpenOVR) install it, and enable it for DCS World. If you need help, go to [the OpenComposite Discord](https://discord.gg/sQ2jwSb62J)
2. If using controller emulation, you probably want to hide DCS's default gloved hands. Inside DCS's installation folder, find `CoreMods\characters\models\glove_L.chanimgpu` and `glove_R.chanimgpu` - inside these files, change the `scale` to `0`
3. If using a PointCTRL with custom firmware, unbind the PointCTRL X and Y axes within DCS
4. Even though you won't be using SteamVR, to use OpenComposite you need to launch DCS with the `--force_steam_VR` option, or if you're using Skatezilla's launcher, select SteamVR. You must do this even if you're not using Steam, and even if you have an Oculus headset, as OpenComposite works by pretending to be SteamVR. If SteamVR launches, OpenComposite is not set up correctly.
5. If using touchscreen emulation (recommended), enable DCS's option to lock the mouse cursor to the window, and enable mouse support
6. If using a virtual VR controller, enable DCS's option to support touch controllers in VR settings

![Skatezilla's SteamVR option](skatezilla-steamvr.png).

If using VR controller emulation, there doesn't appear to be a way to disable 'touch to interact'; be careful not to accidentally touch the controls. This is especially problematic for the fire suppression pull handles in the A10C. If accidental control interactions are a problem, switch to 'Virtual Touch Screen' mode - though you may then need to use a mouse for menus.

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