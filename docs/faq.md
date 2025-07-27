---
nav_order: 5
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

## How do I improve gesture recognition?

- Make sure your area is well-lit
- Make sure your hand tracker can see the gesture

Gesture recognition is provided by your hand trackers' driver; there is nearly no scope for improvement within HTCC.

## Why does the cursor move when I move my head in DCS?

This is a limitation of DCS's mouse handling.

## My anti-virus says it found something, what do I do?

Report to your anti-virus vendor as a false positive.

I do not spend my free time (and in some cases, money) trying to fix bugs in other people's software, which this practically always is.

## I'm a security researcher and actually found a problem, what do I do?

If you have reasons beyond:

- an antivirus says it found something (assume false positive unless evidence to the contrary)
- VirusTotal says an antivirus found something (the same, but multiplied)
- The autoupdater can download and run code from the internet (yes, it's an autoupdater)
- The autoupdater contains some bad/suspicious words (this is the dictionary for Google's Brotli compression, found in appendix A of RFC 7932)

Please open a GitHub issue with details.
