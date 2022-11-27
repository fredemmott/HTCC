# EXPERIMENT: Hand-tracked cockpit clicking for VR flight simulators

This project is an OpenXR API layer aimed to make it easy to click on cockpits in VR flight simulators. Currently only DCS World is supported.

## Installation and setup

**DONT**. This is an experiment and not ready for general use.

- [Downloads](https://github.com/fredemmott/hand-tracked-cockpit-clicking/releases/latest)
- [Getting started](docs/getting%20started.md)
- [PointCTRL](docs/pointctrl/README.md)
- [Oculus hand tracking](docs/oculus-hand-tracking/README.md)
- [DCS World](docs/dcs-world/README.md)

## Getting help

**This is an experiment**; you should only use this if you're already comfortable with OpenComposite and other required projects, I have very limited time to support.

I make this for my own use, and I share this in the hope others find it useful; I'm not able to commit to support, bug fixes, or feature development.

First, check the documentation links above; if you still need help, support may be available from the community via the #hand-tracking channel in [my Discord]. I am not able to respond to 1:1 requests for help via any means, including GitHub, Discord, Twitter, Reddit, or email.

## Features

Pointer input can either come from:
- OpenXR hand tracking (any OpenXR-compatible hand tracking, including Quest 2 or Quest Pro headsets)
- [PointCTRL] with custom firmware

If you already have OpenXR hand tracking, you have everything you need.

A PointCTRL will work with any VR headset, and also often has better tracking when your hands are higher than your head, especially on a Quest 2. The importance here varies by aircraft - for example, it's much more important in a Ka-50 than an A-10C due to the location of the countermeasures system.

Interactions (mouse clicks etc) can either come from:
- Hand tracking **on Oculus headsets**, including Quest 2 or Quest pro
- PointCTRL with custom firmware

Oculus hand tracking gesture recognition does not require additional hardware, however PointCTRL clicks are more reliable, and give a better tacticle feel.

You can mix-and-match, e.g. use hand tracking for position, but PointCTRL FCUs for clicks.

Game support is provided either with:
- a virtual touchscreen
- a virtual Oculus Touch controller

The virtual touch screen feels fairly close to a PointCTRL with default firmware; the key limitations are that it only works with DCS, and it is not possible to click on things at the bottom of your field of view.

The virtual Oculus Touch controller is designed to be used as a 'laser pointer', and intentionally does not line up precisely with your hand: instead, the pointing direction is in a straight line between the center of your view and the PointCTRL or the first knuckle of your index finger. This gives much better precision and stability, however you are likely to want to disable in-game controller models, and DCS will not hold buttons (e.g. signal lamp test buttons) if you move the pointer away from them.

## License

This project is licensed under the [MIT license].

[OpenComposite]: https://gitlab.com/znixian/OpenOVR/-/tree/openxr#downloading-and-installation
[`XR_FB_hand_tracking_aim`]: https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_FB_hand_tracking_aim
[`XR_EXT_hand_tracking`]: https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_EXT_hand_tracking
[MIT license]: LICENSE
[PointCTRL]: https://pointctrl.com/
[my Discord]: https://go.fredemmott.com/discord
