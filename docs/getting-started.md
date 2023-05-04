---
nav_order: 2
---
# Getting Started

## What's this 'OpenXR' thing anyway?

OpenXR is a specification - a language - that components *can* use to talk to each other; both components need to speak OpenXR to work together. OpenXR is not something you can install - but you can install 'translation' projects to translate between OpenXR and other systems.

The most important of these are:
- OpenComposite (not needed for DCS Open Beta): this turns SteamVR games into OpenXR games, by pretending to be SteamVR.
- PimaxXR: this makes it possible for OpenXR games - or SteamVR games using OpenComposite - to use Pimax headsets.

These other projects are not needed, but the names are often confused:
- OpenXR Tools for Windows Mixed Reality: offers basic settings and diagnostics. Primarily used for WMR headsets like the HP Reverb G2
- OpenXR Toolkit: advanced settings for any OpenXR game - or SteamVR game using OpenComposite - on any OpenXR headset
- "OpenXR Tools": not a separate project; sometimes refers to "OpenXR Tools for Windows Mixed Reality", sometimes refers to "OpenXR Toolkit"

As DCS World Stable does not support OpenXR (regardless of headset), OpenComposite is needed to use OpenXR software, like Hand Tracked Cockpit Clicking or OpenXR Toolkit; for Open Beta, DCS's native OpenXR support can be used.

## Installing HTCC

Download and install the MSI from [the latest release](https://github.com/fredemmott/hand-tracked-cockpit-clicking/releases/latest), then configure your devices and games.

## Configuring your devices

- [PointCTRL](hardware/pointctrl/README.md)
- [Oculus Hand Tracking](hardware/oculus-hand-tracking/README.md)

## Configuring your games

- [DCS World](games/dcs-world/README.md)
- [MSFS](games/msfs/README.md)
