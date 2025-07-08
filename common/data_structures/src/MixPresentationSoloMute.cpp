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

#include "MixPresentationSoloMute.h"

#include "juce_core/system/juce_PlatformDefs.h"
#include "logger/logger.h"

MixPresentationSoloMute::MixPresentationSoloMute()
    : RepositoryItemBase(juce::Uuid::null()) {}

MixPresentationSoloMute::MixPresentationSoloMute(juce::Uuid id, bool anySoloed)
    : RepositoryItemBase(id), anySoloed_(anySoloed) {}

void MixPresentationSoloMute::addAudioElement(const juce::Uuid id,
                                              const int referenceId,
                                              const juce::String& name) {
  audioElements_.emplace_back(id, referenceId, name);
}

void MixPresentationSoloMute::removeAudioElement(const juce::Uuid id) {
  audioElements_.erase(
      std::remove_if(audioElements_.begin(), audioElements_.end(),
                     [id](const AudioElementSoloMute& audioElement) {
                       return audioElement.getId() == id;
                     }),
      audioElements_.end());
}

void MixPresentationSoloMute::setAudioElementSolo(const juce::Uuid& id,
                                                  const bool isSoloed) {
  for (auto& audioElement : audioElements_) {
    if (audioElement.getId() == id) {
      audioElement.setSoloed(isSoloed);
      LOG_ANALYTICS(0, "Audio element " + id.toString().toStdString() + " soloed: " + std::to_string(isSoloed));
      return;
    }
  }
}

void MixPresentationSoloMute::setAudioElementMute(const juce::Uuid& id,
                                                  const bool isMuted) {
  for (auto& audioElement : audioElements_) {
    if (audioElement.getId() == id) {
      audioElement.setMuted(isMuted);
      return;
    }
  }
}

AudioElementSoloMute MixPresentationSoloMute::getAudioElement(
    const juce::Uuid& id) const {
  for (auto audioElement : audioElements_) {
    if (audioElement.getId() == id) {
      return audioElement;
    }
  }
  return AudioElementSoloMute();
}

MixPresentationSoloMute MixPresentationSoloMute::fromTree(
    const juce::ValueTree tree) {
  jassert(tree.hasProperty(kId));

  MixPresentationSoloMute mixPres(juce::Uuid(tree[kId]), tree[kAnySoloed]);

  juce::ValueTree audioElementsTree = tree.getChildWithName(kAudioElements);
  for (auto audioElementTree : audioElementsTree) {
    mixPres.audioElements_.push_back(
        AudioElementSoloMute::fromTree(audioElementTree));
  }

  return mixPres;
}

juce::ValueTree MixPresentationSoloMute::toValueTree() const {
  juce::ValueTree tree(kTreeType, {{kId, id_.toString()}, {kAnySoloed, false}});

  juce::ValueTree audioElementsTree =
      tree.getOrCreateChildWithName(kAudioElements, nullptr);
  for (const auto& audioElement : audioElements_) {
    audioElementsTree.appendChild(audioElement.toValueTree(), nullptr);
    if (audioElement.isSoloed()) {
      tree.setProperty(kAnySoloed, true, nullptr);
    }
  }

  tree.appendChild(audioElementsTree, nullptr);

  return tree;
}

bool MixPresentationSoloMute::operator==(
    const MixPresentationSoloMute& other) const {
  if (other.id_ != id_) {
    return false;
  }
  for (auto audioElement = audioElements_.begin();
       audioElement != audioElements_.end(); audioElement++) {
    if (std::find(other.audioElements_.begin(), other.audioElements_.end(),
                  *audioElement) == other.audioElements_.end()) {
      return false;
    }
  }
  return true;
}

bool MixPresentationSoloMute::isAudioElementMuted(const juce::Uuid& id) const {
  for (auto& audioElement : audioElements_) {
    if (audioElement.getId() == id) {
      return audioElement.isMuted();
    }
  }
  return false;
}

bool MixPresentationSoloMute::isAudioElementSoloed(const juce::Uuid& id) const {
  for (auto& audioElement : audioElements_) {
    if (audioElement.getId() == id) {
      return audioElement.isSoloed();
    }
  }
  return false;
}