# MSFS

No special setup needed; just be sure to follow the setup instructions for your hardware before installing Hand Tracked Cockpit Clicking:

- [Oculus Hand Tracking](../oculus-hand-tracking/README.md)
- [PointCTRL](../pointctrl/README.md)

## Controls

Touchscreen/mouse emulation is not supported for MSFS.

### Virtual Controller Mapping

- 'left click'/index finger pinch: vr controller trigger (primary interaction)
- 'right click'/middle finger pinch: push the vr controller away (push button)
- 'scroll up'/ring finger pinch: rotate controller counter-clockwise
- 'scroll down'/little finger pinch: rotate controller clockwise

### PointCTRL FCU buttons

The standard mapping doesn't work well for MSFS, so when running MSFS, there's two modes:

- normal mode - short tap FCU3, and default
- scroll mode - long-press FCU3

#### Normal Mode

- short-tap FCU1: left click
- long-hold FCU1: hold left click
  - short-tap FCU2 while holding FCU1: tap right click
  - long-press FCU2 while holding FCU1: switch to scroll mode, but with left click staying held down even once you release FCU1, until you go back to normal mode
- FCU2: right click
- short-tap FCU3: switch to normal mode (already here, so does nothing)
- long-press FCU3: switch to scroll mode

#### Scroll Mode

- FCU1: scroll up
- FCU2: scroll down
- short-tap FCU3: switch to normal mode
- long-press FCU3: switch to scroll mode (already here, so does nothing)
