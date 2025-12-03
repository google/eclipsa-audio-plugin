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

#include "RoutingProcessor.h"

#ifdef _WIN32
#include <windows.h>
#define sleep(sec) Sleep((sec) * 1000)
#endif

#include "data_structures/src/AudioElementPluginSyncClient.h"

RoutingProcessor::RoutingProcessor(
    AudioElementSpatialLayoutRepository* audioElementSpatialLayoutRepository,
    AudioElementPluginSyncClient* syncClient, int totalChannelCount)
    : audioElementSpatialLayoutData_(audioElementSpatialLayoutRepository),
      syncClient_(syncClient),
      firstChannel_(0),
      totalChannels_(0),
      totalChannelCount_(totalChannelCount) {
  // Register ourselves to listen for updates to the AudioElementSpatialLayout
  // and/or audio element data
  audioElementSpatialLayoutData_->registerListener(this);
  syncClient->registerListener(this);
  initializeRouting();
}

RoutingProcessor::~RoutingProcessor() {
  // Deregister listeners
  audioElementSpatialLayoutData_->deregisterListener(this);
  syncClient_->removeListener(this);
}

void RoutingProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  juce::ignoreUnused(sampleRate);
  copyBuffer_.setSize(totalChannelCount_, samplesPerBlock);
}

void RoutingProcessor::initializeRouting() {
  // Fetch the audio element to determine the first and total channel numbers
  juce::Uuid audioElementId =
      audioElementSpatialLayoutData_->get().getAudioElementId();
  auto audioElement = syncClient_->getElement(audioElementId);
  if (!audioElement.has_value()) {
    return;
  }

  firstChannel_ = audioElement->getFirstChannel();
  totalChannels_ = audioElement->getChannelCount();
}

void RoutingProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                    juce::MidiBuffer& midiMessages) {
  copyBuffer_.clear();

  // Copy data from the input channels to copy buffer, shifting the
  // audio forward by the first channel
  for (int channel = 0; channel < totalChannels_; ++channel) {
    if (channel + firstChannel_ <= buffer.getNumChannels()) {
      copyBuffer_.copyFrom(channel + firstChannel_, 0, buffer, channel, 0,
                           buffer.getNumSamples());
    }
  }

  // Now copy the data back to the original buffer
  // We can't copy in place because JUCE doesn't let you copy between the same
  // buffer
  buffer.makeCopyOf(copyBuffer_);
}