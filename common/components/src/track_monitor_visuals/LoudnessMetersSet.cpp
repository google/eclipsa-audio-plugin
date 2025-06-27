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

#include "LoudnessMetersSet.h"

#include "../../../audioelementplugin/src/AudioElementPluginProcessor.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

LoudnessMetersSet::LoudnessMetersSet(
    AudioElementPluginSyncClient* syncClient,
    AudioElementPluginRepositoryCollection audioElementPluginRepo,
    Speakers::AudioElementSpeakerLayout& pbLayout)
    : repos_(audioElementPluginRepo),
      syncClient_(syncClient),
      audioElementSpatialLayoutRepository_(
          &audioElementPluginRepo.audioElementSpatialLayoutRepository_),
      spkrData_(audioElementPluginRepo.monitorData_),
      playbackMSRepo_(audioElementPluginRepo.msRespository_),
      pbLayout_(pbLayout) {
  if (!pbLayout_.isAmbisonics()) {
    pbLayout_ = audioElementSpatialLayoutRepository_->get().getChannelLayout();
    std::vector<juce::String> chLabels = pbLayout_.getSpeakerLabels();
    createLoudnessMeters(chLabels);
  }
}

LoudnessMetersSet::~LoudnessMetersSet() { setLookAndFeel(nullptr); }

void LoudnessMetersSet::paint(juce::Graphics& g) { drawLoudnessMeters(g); }

void LoudnessMetersSet::resetSoloMutes() {
  PlaybackMS muteSoloState = playbackMSRepo_.get();
  muteSoloState.reset();
  playbackMSRepo_.update(muteSoloState);

  for (const auto& meter : meters_) {
    meter->resetSoloMute();
  }
}

void LoudnessMetersSet::createLoudnessMeters(
    const std::vector<juce::String>& chLabels) {
  meters_.clear();
  for (int i = 0; i < chLabels.size(); ++i) {
    meters_.push_back(
        std::make_unique<LoudnessMeter>(chLabels[i], i, playbackMSRepo_));
    addAndMakeVisible(meters_[i].get());
  }
}

void LoudnessMetersSet::drawLoudnessMeters(juce::Graphics& g) {
  auto bounds = getLocalBounds();

  // Draw meters.
  std::vector<float> loudnesses;

  // Draw ambisonics visualizer
  spkrData_.playbackLoudness.read(loudnesses);
  for (int i = 0; i < meters_.size(); ++i) {
    bounds.removeFromLeft(kMeterOffset);
    if (i < loudnesses.size()) {
      meters_[i]->setloudness(loudnesses[i]);
    }
    meters_[i]->setBounds(bounds.removeFromLeft(kMeterWidth - kMeterOffset));
  }
}

void LoudnessMetersSet::updateMeters() {
  pbLayout_ = audioElementSpatialLayoutRepository_->get().getChannelLayout();
  std::vector<juce::String> chLabels = pbLayout_.getSpeakerLabels();
  createLoudnessMeters(chLabels);

  resetSoloMutes();

  repaint();
}