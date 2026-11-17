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

#include "Panner3DProcessor.h"

#include <algorithm>
#include <memory>

#include "data_structures/src/AudioElementSpatialLayout.h"
#include "processors/processor_base/ProcessorBase.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"
#include "substream_rdr/surround_panner/AmbisonicPanner.h"
#include "substream_rdr/surround_panner/BinauralPanner.h"
#include "substream_rdr/surround_panner/MonoToSpeakerPanner.h"

Panner3DProcessor::Panner3DProcessor(
    ProcessorBase* hostProcessor,
    AudioElementSpatialLayoutRepository* audioElementSpatialLayoutRepository,
    AudioElementParameterTree* automationParameterTree)
    : hostProcessor_(hostProcessor),
      audioElementSpatialLayoutData_(audioElementSpatialLayoutRepository),
      automationParameterTree_(automationParameterTree) {
  outputLayout_ = audioElementSpatialLayoutRepository->get().getChannelLayout();

  xPosition_ = automationParameterTree_->getXPosition();
  yPosition_ = automationParameterTree_->getYPosition();
  zPosition_ = automationParameterTree_->getZPosition();

  automationParameterTree_->addXPositionListener(this);
  automationParameterTree_->addYPositionListener(this);
  automationParameterTree_->addZPositionListener(this);

  audioElementSpatialLayoutRepository->registerListener(this);
}

Panner3DProcessor::~Panner3DProcessor() {
  automationParameterTree_->removeXPositionListener(this);
  automationParameterTree_->removeYPositionListener(this);
  automationParameterTree_->removeZPositionListener(this);
  audioElementSpatialLayoutData_->deregisterListener(this);
}

void Panner3DProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  if (kIsAUBuild) {
    // AU Build: Use 32-sample chunks to handle variable buffer sizes
    samplesPerBlock_ = std::min(samplesPerBlock, 32);
  } else {
    // Non-AU builds (VST3, AAX, etc.): Use host buffer size directly
    samplesPerBlock_ = samplesPerBlock;
  }

  sampleRate_ = sampleRate;
  initializePanning();
}

void Panner3DProcessor::initializePanning() {
  hostProcessor_->suspendProcessing(true);

  // Check if panning is enabled
  bool isPanningEnabled =
      audioElementSpatialLayoutData_->get().isPanningEnabled();

  // Fetch the current audio element to pan to from the panner repository
  outputLayout_ = audioElementSpatialLayoutData_->get().getChannelLayout();

  // Determine the input layout for the plugin
  inputLayout_ = Speakers::AudioElementSpeakerLayout(
      hostProcessor_->getBusesLayout().getMainInputChannelSet());

  // Set up buffers as needed
  // Lock the panner data since we might be performing a process block operation
  // And also updating the panner value tree / position
  renderLock.enter();

  // Note that for all speakers we are currently just using mono inputs
  if (!isPanningEnabled || (outputLayout_ == Speakers::kMono) ||
      (outputLayout_ == Speakers::kExplLFE)) {
    // If panning is disabled, we need to set the panner to NULL
    surroundPanner_.reset();
  } else if (outputLayout_ == Speakers::kBinaural) {
    // For binaural layouts, use the Binaural Panner based on the obr library
    surroundPanner_ =
        std::make_unique<BinauralPanner>(samplesPerBlock_, sampleRate_);
  } else if (outputLayout_.isAmbisonics()) {
    // For ambisonics layouts, use the Ambisonic Panner based on the obr
    // library
    surroundPanner_ = std::make_unique<AmbisonicPanner>(
        outputLayout_, samplesPerBlock_, sampleRate_);
  } else {
    // For non-ambisonics layouts, use the 3D Panner based on libspatialaudio
    surroundPanner_ = std::make_unique<MonoToSpeakerPanner>(
        outputLayout_, samplesPerBlock_, sampleRate_);
  }
  lastSetXPosition_ = xPosition_;
  lastSetYPosition_ = yPosition_;
  lastSetZPosition_ = zPosition_;
  if (surroundPanner_) {
    surroundPanner_->setPosition(lastSetXPosition_, lastSetYPosition_,
                                 lastSetZPosition_);
  }

  outputBuffer_.setSize(outputLayout_.getNumChannels(), samplesPerBlock_);
  // The panners read only the first input channel.
  chunkInput_.setSize(1, samplesPerBlock_);
  renderLock.exit();
  hostProcessor_->suspendProcessing(false);
}

void Panner3DProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer&) {
  const int hostBufferSize = buffer.getNumSamples();

  renderLock.enter();

  if (surroundPanner_ != nullptr && hostBufferSize > 0 &&
      buffer.getNumChannels() > 0) {
    // The panners crossfade a position change across the next chunk they
    // render, so set it only when it changes.
    const int x = xPosition_;
    const int y = yPosition_;
    const int z = zPosition_;
    if (x != lastSetXPosition_ || y != lastSetYPosition_ ||
        z != lastSetZPosition_) {
      surroundPanner_->setPosition(x, y, z);
      lastSetXPosition_ = x;
      lastSetYPosition_ = y;
      lastSetZPosition_ = z;
    }

    const int chunkSize = samplesPerBlock_;
    const int numChunks = (hostBufferSize + chunkSize - 1) / chunkSize;
    const int copyChannels =
        std::min(outputBuffer_.getNumChannels(), buffer.getNumChannels());

    for (int chunk = 0; chunk < numChunks; ++chunk) {
      const int start = chunk * chunkSize;
      const int numSamples = std::min(chunkSize, hostBufferSize - start);

      // The panners always render a full chunk, so zero-pad a short one.
      chunkInput_.copyFrom(0, 0, buffer, 0, start, numSamples);
      if (numSamples < chunkSize) {
        chunkInput_.clear(0, numSamples, chunkSize - numSamples);
      }

      surroundPanner_->process(chunkInput_, outputBuffer_);

      for (int channel = 0; channel < copyChannels; ++channel) {
        buffer.copyFrom(channel, start, outputBuffer_, channel, 0, numSamples);
      }
      for (int channel = copyChannels; channel < buffer.getNumChannels();
           ++channel) {
        buffer.clear(channel, start, numSamples);
      }
    }
  }

  renderLock.exit();
}
