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

#include <juce_audio_utils/juce_audio_utils.h>

// Build configuration for Logic Pro compatibility
// When true, limits to 7.1.4 layout for Logic Pro compatibility
// When false, uses Ambisonics layouts (order 3 for Premiere, order 5 for
// others)
#ifdef ECLIPSA_LOGIC_PRO_BUILD
constexpr bool kIsLogicProBuild = true;
#else
constexpr bool kIsLogicProBuild = false;
#endif

class ProcessorBase : public juce::AudioProcessor {
 public:
  // This constructor is called by our internal processors
  // Default to a sensible wide layout for the host so internal processors
  // behave consistently. AU defaults to a named bed (7.1.2) for Logic/Atmos
  // while other formats default to Ambisonics order 5.
  ProcessorBase()
      : juce::AudioProcessor(
            BusesProperties()
                .withInput("Input", getHostWideLayout(), true)
                .withOutput("Output", getHostWideLayout(), true)) {}

  // This constructor is called by the actual plugins
  // It allows the supported channels to be explicitly stated. This is used by
  // the JUCE debugger
  ProcessorBase(juce::AudioChannelSet inputChannelSet,
                juce::AudioChannelSet outputChannelSet)
      : juce::AudioProcessor(
            BusesProperties()
                .withInput("Input", inputChannelSet, true)
                .withOutput("Output", outputChannelSet, true)) {}

  explicit ProcessorBase(const BusesProperties& ioLayouts)
      : AudioProcessor(ioLayouts) {}

  // Static helper so it can be used inside member initializer lists of
  // derived processors (cannot rely on virtual functions there).
  static inline juce::AudioChannelSet getHostWideLayout() {
    if (kIsLogicProBuild) {
      return juce::AudioChannelSet::create7point1point4();
    } else {
      if (juce::PluginHostType().isPremiere()) {
        return juce::AudioChannelSet::ambisonic(3);
      } else {
        return juce::AudioChannelSet::ambisonic(5);
      }
    }
  }

  void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    juce::ignoreUnused(sampleRate, samplesPerBlock);
  }
  void releaseResources() override {}

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override {
    return true;
  }

  bool canAddBus(bool isInput) const override {
    juce::ignoreUnused(isInput);
    return true;
  }

  bool canRemoveBus(bool isInput) const override {
    return getBusCount(isInput) > 1 ? true : false;
  }

  void getStateInformation(juce::MemoryBlock& destData) override {
    juce::ignoreUnused(destData);
  }

  void setStateInformation(const void* data, int sizeInBytes) override {
    juce::ignoreUnused(data, sizeInBytes);
  }
  virtual void reinitializeAfterStateRestore() {}

  juce::AudioProcessorEditor* createEditor() override { return nullptr; }
  bool hasEditor() const override { return false; }

  const juce::String getName() const override { return {"Base"}; }

  bool acceptsMidi() const override { return false; }

  bool producesMidi() const override { return false; }

  bool isMidiEffect() const override { return false; }

  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int index) override { juce::ignoreUnused(index); }
  const juce::String getProgramName(int index) override {
    juce::ignoreUnused(index);
    return {};
  }
  void changeProgramName(int index, const juce::String& newName) override {
    juce::ignoreUnused(index, newName);
  }
};