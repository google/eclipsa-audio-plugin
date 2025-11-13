/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace EclipsaColours {
const static juce::Colour green = juce::Colour(117, 250, 76);
const static juce::Colour red = juce::Colour(234, 50, 35);
const static juce::Colour yellow = juce::Colour(254, 252, 90);
const static juce::Colour orange = juce::Colour(235, 159, 56);
const static juce::Colour selectCyan = juce::Colour(128, 213, 212);
const static juce::Colour onButtonGrey = juce::Colour(50, 75, 75);
const static juce::Colour semiOnButtonGrey = onButtonGrey.withAlpha(0.5f);
const static juce::Colour indicatorBorderGrey = juce::Colour(111, 121, 121);
const static juce::Colour inactiveGrey = juce::Colour(26, 33, 33);
const static juce::Colour headingGrey = juce::Colour(221, 228, 227);
const static juce::Colour tabTextGrey = juce::Colour(190, 201, 200);
const static juce::Colour drawButtonGrey = juce::Colour(204, 232, 231);
const static juce::Colour tableAlternateGrey = juce::Colour(22, 29, 29);
// temporarily use default black
const static juce::Colour backgroundOffBlack = juce::Colour(14, 21, 20);
const static juce::Colour transparentBlack = juce::Colours::transparentBlack;
const static juce::Colour textWhite = juce::Colour(255, 219, 202);
const static juce::Colour rolloverGrey = juce::Colour(63, 73, 72);
const static juce::Colour gridLine = juce::Colour(42, 48, 47);
const static juce::Colour lightGridLine = juce::Colour(78, 84, 83);
const static juce::Colour controlBlue = juce::Colour(27, 159, 231);

const static juce::Colour buttonOutlineGrey = juce::Colour(190, 201, 200);
const static juce::Colour buttonTextColour = juce::Colour(0, 55, 55);
const static juce::Colour buttonRolloverColour = juce::Colour(63, 73, 72);
const static juce::Colour buttonRolloverTextColour =
    juce::Colour(128, 213, 212);
const static juce::Colour buttonMSTextColour = juce::Colour(136, 147, 146);

const static juce::Colour iconWhite = juce::Colour(230, 224, 233);
const static juce::Colour selectionToggleBorderGrey =
    juce::Colour(136, 147, 146);
const static juce::Colour ambisonicsFillGrey = juce::Colour(37, 43, 43);
// Room View Colours.
const static juce::Colour roomviewLightWall = juce::Colour(66, 73, 72);
const static juce::Colour roomviewDarkWall = juce::Colour(35, 42, 41);
const static juce::Colour roomviewLightGrid = juce::Colour(77, 84, 83);
const static juce::Colour roomviewDarkGrid = juce::Colour(41, 48, 47);
const static juce::Colour roomviewTranslucentWall =
    roomviewDarkWall.withAlpha(0.5f);
const static juce::Colour roomviewIsoTransparentWall =
    EclipsaColours::roomviewLightWall.brighter().withAlpha(0.06f);
const static juce::Colour speakerOutline = juce::Colour(147, 143, 153);
const static juce::Colour speakerSilentFill = juce::Colour(85, 92, 91);
const static juce::Colour playerBlue = juce::Colour(96, 156, 247);
}  // namespace EclipsaColours
