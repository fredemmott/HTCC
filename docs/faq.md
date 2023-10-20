---
nav_order: 3
---

# Frequently Asked Questions

## I can't see my hands in DCS

If you are using controller emulation and followed the instructions, you should only see laser pointers.

If you are using touch screen/tablet emulation, you should only see mouse cursor movement.

If you do not see these, follow the setup instructions.

## Why are hands not meant to be visible in DCS?

For touch screen/tablet emulation, DCS does not draw hands for mice/touch screens/tablets.

For controller emulation, the instructions say to disable the hands because HTCC intentionally ignores hand orientation.

## Why does HTCC ignore hand orientation?

HTCC draws a line from your headset to your hand, and uses that for pointer orientation; this **greatly** increases pointing stability and accuracy.

Only Eagle Dynamics are able to make hand tracking in DCS immersive, reliable, **and** useful; third-party software like HTCC must make a trade-off, and HTCC chooses reliability and utility over immersion.

## Why does is the pointer in a line from my head?

See the previous question.