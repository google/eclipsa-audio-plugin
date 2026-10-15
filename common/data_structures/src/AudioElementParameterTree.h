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
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#include <atomic>
#include <cmath>

#include "ParameterMetaData.h"

class AudioElementParameterTree : public juce::AudioProcessorValueTreeState {
 public:
  AudioElementParameterTree(juce::AudioProcessor& panner)
      : juce::AudioProcessorValueTreeState(
            panner, nullptr, AutoParamMetaData::kTreeType,
            AutoParamMetaData::CreateStaticParameterLayout()) {};

  int getXPosition() {
    return (int)std::lround(read(AutoParamMetaData::xPosition));
  }

  int getYPosition() {
    return (int)std::lround(read(AutoParamMetaData::yPosition));
  }

  int getZPosition() {
    return (int)std::lround(read(AutoParamMetaData::zPosition));
  }

  float getVolume() { return read(AutoParamMetaData::volumeId); }

  bool getUnmute() { return read(AutoParamMetaData::unmuteId) > 0.5f; }

  void setXPosition(int value) {
    write(AutoParamMetaData::xPosition, (float)value);
  }

  void setYPosition(int value) {
    write(AutoParamMetaData::yPosition, (float)value);
  }

  void setZPosition(int value) {
    write(AutoParamMetaData::zPosition, (float)value);
  }

  void setVolume(float value) { write(AutoParamMetaData::volumeId, value); }

  void setUnmute(bool value) {
    write(AutoParamMetaData::unmuteId, value ? 1.f : 0.f);
  }

  // For a control that carries its parameter's name rather than a fixed axis.
  void setParameterValue(const juce::String& parameterName, const float value) {
    write(parameterName, value);
  }

  void addXPositionListener(
      juce::AudioProcessorValueTreeState::Listener* listener) {
    addParameterListener(AutoParamMetaData::xPosition, listener);
  }

  void addYPositionListener(
      juce::AudioProcessorValueTreeState::Listener* listener) {
    addParameterListener(AutoParamMetaData::yPosition, listener);
  }

  void addZPositionListener(
      juce::AudioProcessorValueTreeState::Listener* listener) {
    addParameterListener(AutoParamMetaData::zPosition, listener);
  }

  void addVolumeListener(
      juce::AudioProcessorValueTreeState::Listener* listener) {
    addParameterListener(AutoParamMetaData::volumeId, listener);
  }

  void addUnmuteListener(
      juce::AudioProcessorValueTreeState::Listener* listener) {
    addParameterListener(AutoParamMetaData::unmuteId, listener);
  }

  void removeXPositionListener(
      juce::AudioProcessorValueTreeState::Listener* listener) {
    removeParameterListener(AutoParamMetaData::xPosition, listener);
  }

  void removeYPositionListener(
      juce::AudioProcessorValueTreeState::Listener* listener) {
    removeParameterListener(AutoParamMetaData::yPosition, listener);
  }

  void removeZPositionListener(
      juce::AudioProcessorValueTreeState::Listener* listener) {
    removeParameterListener(AutoParamMetaData::zPosition, listener);
  }

  void removeVolumeListener(
      juce::AudioProcessorValueTreeState::Listener* listener) {
    removeParameterListener(AutoParamMetaData::volumeId, listener);
  }

  void removeUnmuteListener(
      juce::AudioProcessorValueTreeState::Listener* listener) {
    removeParameterListener(AutoParamMetaData::unmuteId, listener);
  }

 private:
  // Reads the parameter itself, not the value tree behind getParameterAsValue.
  // AudioProcessorValueTreeState refreshes that tree from its own timer, which
  // idles at half a second, so a tree read can lag the parameter by that much
  // -- including inside a parameter listener, which runs before the refresh.
  // The tree is still what gets persisted; copyState flushes it first.
  float read(const juce::String& parameterName) const {
    const std::atomic<float>* kValue = getRawParameterValue(parameterName);
    return kValue == nullptr ? 0.f : kValue->load();
  }

  // Writes the parameter for the same reason, and because setting a tree
  // property is a no-op when the lagging property already holds `value`,
  // leaving the parameter at what the caller meant to replace.
  void write(const juce::String& parameterName, const float value) {
    juce::RangedAudioParameter* parameter = getParameter(parameterName);
    if (parameter != nullptr) {
      parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }
  }
};