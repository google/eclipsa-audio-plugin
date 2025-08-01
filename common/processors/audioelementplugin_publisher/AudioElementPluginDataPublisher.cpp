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

#include "AudioElementPluginDataPublisher.h"

#include <cmath>
#include <memory>

#include "data_structures/src/AudioElementCommunication.h"
#include "processors/audioelementplugin_publisher/MessagingThread.h"

AudioElementPluginDataPublisher::AudioElementPluginDataPublisher(
    AudioElementSpatialLayoutRepository* audioElementSpatialLayoutRepository,
    AudioElementParameterTree* automationParameterTree)
    : audioElementSpatialLayoutData_(audioElementSpatialLayoutRepository),
      automationParameterTree_(automationParameterTree),
      messagingThread_(std::make_unique<MessagingThread>(
          juce::String("AudioElementPublisherThread"))) {
  // Set up the initial data
  localData_.x = automationParameterTree_->getXPosition();
  localData_.y = automationParameterTree_->getYPosition();
  localData_.z = automationParameterTree_->getZPosition();

  avgLoudness_.update(-70.f);
  jassert(messagingThread_ != nullptr);
  // Update any information from the repository
  updateData();

  automationParameterTree_->addXPositionListener(this);
  automationParameterTree_->addYPositionListener(this);
  automationParameterTree_->addZPositionListener(this);

  audioElementSpatialLayoutRepository->registerListener(this);
}

AudioElementPluginDataPublisher::~AudioElementPluginDataPublisher() {}

void AudioElementPluginDataPublisher::prepareToPlay(double sampleRate,
                                                    int samplesPerBlock) {}

void AudioElementPluginDataPublisher::updateData() {
  // Fetch the audio element plugin name from the repository
  strncpy(localData_.name,
          audioElementSpatialLayoutData_->get().getName().toRawUTF8(),
          sizeof(localData_.name));
  localData_.name[sizeof(localData_.name) - 1] =
      '\0';  // Ensure null-terminated
  channels_ =
      audioElementSpatialLayoutData_->get().getChannelLayout().getNumChannels();

  std::memcpy(localData_.uuid.data(),
              audioElementSpatialLayoutData_->get().getId().getRawData(),
              localData_.uuid.size());

  float loudness(-70.f);

  avgLoudness_.read(loudness);  // Reset the average loudness

  localData_.loudness = loudness;

  messagingThread_->pushAudioElementUpdateData(localData_);
}

void AudioElementPluginDataPublisher::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
  float loudness = 0;
  for (int i = 0; i < channels_; ++i) {
    float chLoud =
        20.0f * std::log10(buffer.getRMSLevel(i, 0, buffer.getNumSamples()));
    // Clamp the loudness to -70 dB since some tracks will be -Inf
    loudness += std::max(chLoud, -70.0f);
  }
  loudness = loudness / channels_;
  avgLoudness_.update(loudness);

  if (loudness != localData_.loudness) {
    localData_.loudness = loudness;
    messagingThread_->pushAudioElementUpdateData(localData_);
  }
}
