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

#include "RenderProcessor.h"

#include <cstddef>
#include <ranges>

#include "data_repository/implementation/AudioElementRepository.h"
#include "data_repository/implementation/RoomSetupRepository.h"
#include "data_structures/src/AudioElement.h"
#include "data_structures/src/MixPresentation.h"
#include "data_structures/src/RoomSetup.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "logger/logger.h"
#include "substream_rdr/rdr_factory/Renderer.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

AudioElementRenderer::AudioElementRenderer(
    Speakers::AudioElementSpeakerLayout inputLayout,
    Speakers::AudioElementSpeakerLayout playbackLayout, int firstInputChannel,
    int samplesPerBlock, int sampleRate, bool isBinaural)
    : inputData(inputLayout.getNumChannels(), samplesPerBlock),
      outputData(playbackLayout.getNumChannels(), samplesPerBlock),
      outputDataBinaural(Speakers::kBinaural.getNumChannels(), samplesPerBlock),
      firstChannel(firstInputChannel),
      inputLayout(inputLayout),
      kIsBinaural(isBinaural) {
  renderer = createRenderer(inputLayout, playbackLayout);
  if (kIsBinaural) {
    rendererBinaural = createRenderer(inputLayout, Speakers::kBinaural,
                                      samplesPerBlock, sampleRate);
  } else {
    rendererBinaural = createRenderer(inputLayout, Speakers::kStereo);
  }
}

//==============================================================================
RenderProcessor::RenderProcessor(ProcessorBase* hostProc,
                                 RoomSetupRepository* roomSetupData,
                                 AudioElementRepository* audioElementData,
                                 MixPresentationRepository* mixPresData,
                                 ActiveMixRepository* activeMixdata,
                                 SpeakerMonitorData& data)
    : hostProcessor_(hostProc),
      roomSetupData_(roomSetupData),
      audioElementData_(audioElementData),
      mixPresData_(mixPresData),
      activeMixPresData_(activeMixdata),
      monitorData_(data),
      currentSamplesPerBlock_(1),
      speakersOut_(1) {
  currentPlaybackLayout_ =
      roomSetupData->get().getSpeakerLayout().getRoomSpeakerLayout();

  // Call the initialization function once
  initializeRenderers();

  // Listen for updates from the UI
  audioElementData_->registerListener(this);
  roomSetupData->registerListener(this);
  mixPresData_->registerListener(this);
  activeMixPresData_->registerListener(this);
}

RenderProcessor::~RenderProcessor() {
  // Clear the current renderers
  for (auto audioElementRenderer = audioElementRenderers_.begin();
       audioElementRenderer != audioElementRenderers_.end();
       ++audioElementRenderer) {
    delete *audioElementRenderer;
  }
  audioElementRenderers_.clear();

  audioElementData_->deregisterListener(this);
  roomSetupData_->deregisterListener(this);
  mixPresData_->deregisterListener(this);
  activeMixPresData_->deregisterListener(this);
}

void RenderProcessor::initializeRenderers() {
  // Suspending processing while we update the renderers.
  hostProcessor_->suspendProcessing(true);
  const juce::SpinLock::ScopedLockType lock(renderersLock_);

  // Clear the current renderers.
  for (AudioElementRenderer* rdr : audioElementRenderers_) {
    delete rdr;
  }
  audioElementRenderers_.clear();

  // Get the active mix presentation.
  juce::Uuid activeMixID = activeMixPresData_->get().getActiveMixId();

  // If the active mix presentation is invalid, exit.
  std::optional<MixPresentation> activeMixPres = mixPresData_->get(activeMixID);
  if (!activeMixPres) {
    return;
  }

  // From the active mix presentation pull down the list of constituent audio
  // elements and construct renderers for these elements.
  mixPresentationGain_ = activeMixPres->getDefaultMixGain();
  std::vector<MixPresentationAudioElement> mixPresAEs =
      activeMixPres->getAudioElements();

  // boilerplate ensures that each MixPresentationAudioElement is in the
  // AudioElementRepository
  std::vector<AudioElement> activeAudioElements;
  for (int i = 0; i < mixPresAEs.size(); ++i) {
    std::optional<AudioElement> ae =
        audioElementData_->get(mixPresAEs[i].getId());

    if (ae) {
      activeAudioElements.push_back(ae.value());
    } else {
      LOG_ERROR(0, "Failed to retrieve mixPresentationAudioElement with ID: " +
                       mixPresAEs[i].getId().toString().toStdString() +
                       " from the audio element repository.");
    }
  }

  jassert(activeAudioElements.size() ==
          mixPresAEs.size());  // Ensure we have all audio elements.

  // Get the room's speaker layout
  auto roomSpeakerLayout = roomSetupData_->get().getSpeakerLayout();
  speakersOut_ = roomSpeakerLayout.getRoomSpeakerLayout().getNumChannels();
  currentPlaybackLayout_ = roomSpeakerLayout.getRoomSpeakerLayout();

  // Resize the internal bed mixing buffer.
  mixBuffer_.setSize(currentPlaybackLayout_.getNumChannels(),
                     currentSamplesPerBlock_);
  binauralMixBuffer_.setSize(Speakers::kBinaural.getNumChannels(),
                             currentSamplesPerBlock_, false, true, true);

  // Create a renderer for each audio element
  for (int i = 0; i < activeAudioElements.size(); ++i) {
    const AudioElement& audioElement = activeAudioElements[i];
    const MixPresentationAudioElement& mixPresAudioElement =
        mixPresAEs[i];  // Get the corresponding MixPresentationAudioElement
    Speakers::AudioElementSpeakerLayout audioElementLayout =
        audioElement.getChannelConfig();

    // Determine the number of channels used by this audio element and the
    // first channel it uses
    int firstChannel = audioElement.getFirstChannel();

    // Create the audio element renderer
    // Add the audio element renderer to our list of renderers
    audioElementRenderers_.push_back(new AudioElementRenderer(
        audioElementLayout,
        roomSetupData_->get().getSpeakerLayout().getRoomSpeakerLayout(),
        firstChannel, currentSamplesPerBlock_, currentSampleRate_,
        mixPresAudioElement.isBinaural()));
  }

  // Set up the input and output buffers
  for (auto& aeRdr : audioElementRenderers_) {
    aeRdr->inputData.setSize(
        aeRdr->inputLayout.getExplBaseLayout().getNumChannels(),
        currentSamplesPerBlock_, false, true, true);
    aeRdr->outputData.setSize(speakersOut_, currentSamplesPerBlock_, false,
                              true, true);
    aeRdr->outputDataBinaural.setSize(Speakers::kBinaural.getNumChannels(),
                                      currentSamplesPerBlock_, false, true,
                                      true);
  }

  hostProcessor_->suspendProcessing(false);
}

//==============================================================================
const juce::String RenderProcessor::getName() const { return {"FileOutput"}; }

//==============================================================================
void RenderProcessor::setNonRealtime(bool isNonRealtime) noexcept {}

void RenderProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  juce::ignoreUnused(sampleRate);

  currentSamplesPerBlock_ = samplesPerBlock;
  currentSampleRate_ = sampleRate;
  initializeRenderers();
}

void RenderProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  // Clear the internal buffers.
  mixBuffer_.clear();
  binauralMixBuffer_.clear();

  // Take the renderers lock to prevent the renderers from being modified by the
  // UI thread while processing.
  const juce::SpinLock::ScopedLockType lock(renderersLock_);

  // Fetch each audio element currently being played back, render it to this
  // room setup
  for (auto& aeRdr : audioElementRenderers_) {
    // Clear the buffers (may not have to clear output, unsure)
    aeRdr->inputData.clear();
    aeRdr->outputData.clear();
    aeRdr->outputDataBinaural.clear();

    // Copy Audio Element substream data from the process block buffer to the
    // AudioElementRenderer's input buffer.
    for (int ch = 0; ch < aeRdr->inputData.getNumChannels(); ++ch) {
      aeRdr->inputData.copyFrom(ch, 0, buffer, aeRdr->firstChannel + ch, 0,
                                buffer.getNumSamples());
    }

    // Always attempt to render binaural audio.
    // This renderer is never null, it is either a BinauralRdr, a BedToBedRdr or
    // a PassthroughRdr.
    if (aeRdr->rendererBinaural != nullptr) {
      aeRdr->rendererBinaural->render(aeRdr->inputData,
                                      aeRdr->outputDataBinaural);

      // Mix rendered binaural audio to the internal binaural mix buffer.
      for (int i = 0; i < Speakers::kBinaural.getNumChannels(); ++i) {
        binauralMixBuffer_.addFrom(i, 0, aeRdr->outputDataBinaural, i, 0,
                                   binauralMixBuffer_.getNumSamples());
      }

      // Render beds audio if playback is not binaural,
      // This renderer could be null if the rdrMat does not exist, so ensure the
      // renderer is not null.
      if (currentPlaybackLayout_ != Speakers::kBinaural &&
          aeRdr->renderer != nullptr) {
        aeRdr->renderer->render(aeRdr->inputData, aeRdr->outputData);
      }

      // Mix the rendered audio to the internal mix buffer.
      const int numSourceChannels = aeRdr->outputData.getNumChannels();
      for (int i = 0; i < numSourceChannels; ++i) {
        mixBuffer_.addFrom(i, 0, aeRdr->outputData, i, 0,
                           mixBuffer_.getNumSamples());
      }
    }
  }

  // Update the binaural loudness from the rendered and mixed binaural
  // buffer.
  updateBinauralLoudness(binauralMixBuffer_);

  buffer.clear();

  // If the playback is binaural, copy the mixed binaural audio to the output
  // buffer.
  if (currentPlaybackLayout_ == Speakers::kBinaural) {
    for (int i = 0; i < binauralMixBuffer_.getNumChannels(); ++i) {
      buffer.copyFrom(i, 0, binauralMixBuffer_, i, 0,
                      binauralMixBuffer_.getNumSamples());
    }
  }
  // Otherwise copy the mixed beds audio to the output buffer.
  else {
    for (int i = 0; i < mixBuffer_.getNumChannels(); ++i) {
      buffer.copyFrom(i, 0, mixBuffer_, i, 0, mixBuffer_.getNumSamples());
    }
  }
  buffer.applyGain(mixPresentationGain_);
}

void RenderProcessor::updateBinauralLoudness(
    juce::AudioBuffer<float>& rdrdAudio) {
  std::array<float, 2> loudnesses;
  if (rdrdAudio.getNumChannels() < 2) {
    loudnesses[0] = -300.f;
    loudnesses[1] = -300.f;
  } else {
    for (int i = 0; i < 2; ++i) {
      loudnesses[i] = 20.0f * std::log10(rdrdAudio.getRMSLevel(
                                  i, 0, rdrdAudio.getNumSamples()));
    }
  }

  monitorData_.binauralLoudness.update(loudnesses);
}

//==============================================================================
bool RenderProcessor::hasEditor() const { return false; }

juce::AudioProcessorEditor* RenderProcessor::createEditor() { return nullptr; }