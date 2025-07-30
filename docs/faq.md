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

## I'm a security researcher and found a problem, what do I do?

If the potential issue is in this list, please do not report it until you have confirmed that the problem is a genuine issue in HTCC, not a false-positive:

- an antivirus says it found something
- VirusTotal says one or more antivirus programs found something
- the autoupdater can download and run code from the internet (yes, it's an autoupdater)
- the autoupdater contains some suspicious words (this is the dictionary for Google's Brotli compression, found in appendix A of RFC 7932)
- an LLM or any other tool says it found an issue
- any other heuristic

If the cause is *not* in the list above, or you have confirmed that it is *not* a false-positive:
- If you believe that opening a public GitHub issue puts users at risk, please [email me](mailto:security@fred.fredemmott.com)
- Otherwise, please open a GitHub issue

## I'm a game developer, how do I add/improve HTCC support?

HTCC is a band-aid; everything it does can be done *better*, *more easily*, and *more reliably* in the game/engine - without HTCC. Adding/improving HTCC support in your game should not be a goal; instead, add/improve support for `XR_EXT_hand_tracking` in your game.

If you would like assistance implementing hand tracking in your game/engine without HTCC, I can be reached [via email](mailto:htcc-commerical@fred.fredemmott.com) to inquire about my
availability for contract work. 

## I'm a gamer; can you support an additional game?

I do not currently intend to add support for any additional games to HTCC.

The game developers can do everything that HTCC does *better*, *more easily*, and *more reliably* in the game itself, without HTCC.  I do not want to give game developers a reason not to implement hand tracking directly in the game; long-term, this may be a net loss to users and the broader ecosystem.

You can:

- ask the developers of the game to improve the hand tracking experience without HTCC
- ask relevant hardware manufacturers to make hand tracking part of their relationship with their software partners