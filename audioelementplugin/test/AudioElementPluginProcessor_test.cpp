// Copyright 2025 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "../src/AudioElementPluginProcessor.h"

#include <gtest/gtest.h>

namespace {
void sendHostTrackName(AudioElementPluginProcessor& processor,
                       const juce::String& name) {
  juce::AudioProcessor::TrackProperties properties;
  properties.name = name;
  processor.updateTrackProperties(properties);
}

AudioElementSpatialLayout storedLayout(AudioElementPluginProcessor& processor) {
  return processor.getRepositories().audioElementSpatialLayoutRepository_.get();
}

juce::MemoryBlock savedStateWithCustomName(const juce::String& name) {
  juce::MemoryBlock state;
  AudioElementPluginProcessor source;
  sendHostTrackName(source, "Track 1");
  source.setUserTrackName(name);
  source.getStateInformation(state);
  return state;
}

void loadState(AudioElementPluginProcessor& processor,
               const juce::MemoryBlock& state) {
  processor.setStateInformation(state.getData(),
                                static_cast<int>(state.getSize()));
}
}  // namespace

// A custom name survives a reload when the host sends its track name first.
TEST(AudioElementPluginProcessorTrackName,
     custom_name_survives_reload_host_name_before_load) {
  const juce::MemoryBlock state = savedStateWithCustomName("Lead Vocal");

  AudioElementPluginProcessor restored;
  sendHostTrackName(restored, "Track 1");
  loadState(restored, state);

  EXPECT_EQ(storedLayout(restored).getName(), "Lead Vocal");
  EXPECT_TRUE(storedLayout(restored).isNameCustom());
}

// A custom name survives a reload when the host sends its track name after.
TEST(AudioElementPluginProcessorTrackName,
     custom_name_survives_reload_host_name_after_load) {
  const juce::MemoryBlock state = savedStateWithCustomName("Lead Vocal");

  AudioElementPluginProcessor restored;
  loadState(restored, state);
  sendHostTrackName(restored, "Track 1");

  EXPECT_EQ(storedLayout(restored).getName(), "Lead Vocal");
  EXPECT_TRUE(storedLayout(restored).isNameCustom());
}

// A name the user never typed follows the host's track name, live and after a
// reload.
TEST(AudioElementPluginProcessorTrackName, host_name_followed_when_not_custom) {
  juce::MemoryBlock state;
  {
    AudioElementPluginProcessor source;
    sendHostTrackName(source, "Track 1");
    EXPECT_EQ(storedLayout(source).getName(), "Track 1");
    sendHostTrackName(source, "Drums");
    EXPECT_EQ(storedLayout(source).getName(), "Drums");
    EXPECT_FALSE(storedLayout(source).isNameCustom());
    source.getStateInformation(state);
  }

  AudioElementPluginProcessor restored;
  sendHostTrackName(restored, "Renamed In DAW");
  loadState(restored, state);
  EXPECT_EQ(storedLayout(restored).getName(), "Renamed In DAW");

  sendHostTrackName(restored, "Renamed Again");
  EXPECT_EQ(storedLayout(restored).getName(), "Renamed Again");
}

// Clearing the custom name restores the host's track name and resumes
// following it.
TEST(AudioElementPluginProcessorTrackName,
     clearing_custom_name_restores_host_name) {
  AudioElementPluginProcessor processor;
  sendHostTrackName(processor, "Track 1");
  processor.setUserTrackName("Lead Vocal");
  sendHostTrackName(processor, "Track 2");
  ASSERT_EQ(storedLayout(processor).getName(), "Lead Vocal");

  processor.setUserTrackName("");

  EXPECT_EQ(storedLayout(processor).getName(), "Track 2");
  EXPECT_FALSE(storedLayout(processor).isNameCustom());
  sendHostTrackName(processor, "Track 3");
  EXPECT_EQ(storedLayout(processor).getName(), "Track 3");
}
