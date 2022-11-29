# Getting Started

## What's this 'OpenXR' thing anyway?

OpenXR is a specification - a language - that components *can* use to talk to each other; both components need to speak OpenXR to work together. OpenXR is not something you can install - but you can install 'translation' projects to translate between OpenXR and other systems.

The most important of these are:
- OpenComposite: this turns SteamVR games into OpenXR games, by pretending to be SteamVR.
- PimaxXR: this makes it possible for OpenXR games - or SteamVR games using OpenComposite - to use Pimax headsets.

These other projects are not needed, but the names are often confused:
- OpenXR Tools for Windows Mixed Reality: offers basic settings and diagnostics. Primarily used for WMR headsets like the HP Reverb G2
- OpenXR Toolkit: advanced settings for any OpenXR game - or SteamVR game using OpenComposite - on any OpenXR headset
- "OpenXR Tools": not a separate project; sometimes refers to "OpenXR Tools for Windows Mixed Reality", sometimes refers to "OpenXR Toolkit"

As DCS World does not support OpenXR (regardless of headset), OpenComposite is needed to use OpenXR software, like Hand Tracked Cockpit Clicking or OpenXR Toolkit.

## Configuring your devices

- [PointCTRL]
- [Oculus Hand Tracking]

## Configuring your games

- [DCS World]
- [MSFS]

[Oculus Hand Tracking]: oculus-hand-tracking/README.md
[DCS World]: dcs-world/README.md
[PointCTRL]: pointctrl/README.md
[MSFS]: msfs/README.md
