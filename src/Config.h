/*
 * MIT License
 *
 * Copyright (c) 2022 Fred Emmott <fred@fredemmott.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once

#include <cinttypes>

namespace DCSQuestHandTracking {
enum class PointerSource : DWORD {
  OculusHandTracking = 0,
  PointCtrl = 1,
};
}

namespace DCSQuestHandTracking::Config {

// Enable or disable the entire API layer
extern bool Enabled;

// Do nothing unless running inside "DCS.exe"
extern bool CheckDCS;

// 0 = Oculus hand tracking, 1 = PointCtrl
extern DCSQuestHandTracking::PointerSource PointerSource;

// 0 = off, 1 = some, 2 = more, 3 = every frame
extern uint8_t VerboseDebug;

// 0 = left, 1 == right
extern uint8_t MirrorEye;
// Use Quest hand tracking pinch gestures for mouse clicks (index and middle
// finger)
extern bool PinchToClick;
// Use Quest hand tracking pinch gestures for wheel events (ring and little
// finger)
extern bool PinchToScroll;
// currently requires a custom PointCtrl firmware exposing all buttons and axis
// as a joystick instead of a mouse
extern bool PointCtrlFCUClicks;

// Run the calibration app for these :)
extern uint16_t PointCtrlCenterX;
extern uint16_t PointCtrlCenterY;
extern float PointCtrlRadiansPerUnitX;
extern float PointCtrlRadiansPerUnitY;

void Load();

}// namespace DCSQuestHandTracking::Config
