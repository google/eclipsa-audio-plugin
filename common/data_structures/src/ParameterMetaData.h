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

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include <memory>

#include "data_structures/src/RealtimeDataType.h"

namespace AutoParamMetaData {

// create strings for automation parameters
const std::string volumeId{"PannerVolume"};
const std::string unmuteId{"PannerMute"};
const juce::Identifier kTreeType{"AutomationParams"};

// ranges
const std::pair<int, int> positionRange_({-50.f, 50.f});
const std::pair<int, int> rotationRange_({-180, 180});
const std::pair<int, int> spreadRange_({0, 100});

// position
static const juce::String xPosition{"X"};
static const juce::String yPosition{"Y"};
static const juce::String zPosition{"Z"};
static const juce::String rotation{"Rotation"};
static const juce::String size{"Size"};

// spread
static const juce::String width{"Width"};
static const juce::String height{"Height"};
static const juce::String depth{"Depth"};

// LFE
const juce::String lfeName = "LFE";
const std::pair<int, int> lfeRange_ = spreadRange_;
const int lfeInitialValue = 50;

static juce::ParameterID getParameterIDFromName(
    const juce::String& parameterName) {
  return juce::ParameterID(parameterName, 2);
}

static std::unique_ptr<juce::AudioParameterInt> createIntParameter(
    const juce::String& parameterName, const std::pair<int, int>& range,
    int initialValue) {
  return std::make_unique<juce::AudioParameterInt>(
      getParameterIDFromName(parameterName), parameterName, range.first,
      range.second, initialValue);
}

static std::unique_ptr<juce::AudioParameterFloat> createFloatParameter(
    const juce::String& parameterName, const std::pair<float, float>& range,
    float initialValue) {
  return std::make_unique<juce::AudioParameterFloat>(
      getParameterIDFromName(parameterName), parameterName,
      juce::NormalisableRange<float>{range.first, range.second}, initialValue);
}

static juce::AudioProcessorValueTreeState::ParameterLayout
CreateStaticParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  // this function is multi-step for readability
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      AutoParamMetaData::getParameterIDFromName(AutoParamMetaData::volumeId),
      AutoParamMetaData::volumeId, juce::NormalisableRange<float>{-100.f, 12.f},
      0.f));

  layout.add(std::make_unique<juce::AudioParameterBool>(
      AutoParamMetaData::getParameterIDFromName(AutoParamMetaData::unmuteId),
      AutoParamMetaData::unmuteId, true));

  // create position control parameters
  layout.add(createIntParameter(xPosition, positionRange_, 0));
  layout.add(createIntParameter(yPosition, positionRange_, 0));
  layout.add(createIntParameter(zPosition, positionRange_, 0));

  return layout;
}
}  // namespace AutoParamMetaData
