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

#include "MonoToSpeakerPanner.h"

inline admrender::OutputLayout AdmTypeFromPannedLayout(
    Speakers::AudioElementSpeakerLayout pannedLayout) {
  switch (pannedLayout) {
    // Note that Mono is unsupported
    case Speakers::kStereo:
      return admrender::OutputLayout::ITU_0_2_0;
    case Speakers::k3Point1Point2:
      return admrender::OutputLayout::_3p1p2;
    case Speakers::k5Point1:
      return admrender::OutputLayout::ITU_0_5_0;
    case Speakers::k5Point1Point2:
      return admrender::OutputLayout::ITU_2_5_0;
    case Speakers::k5Point1Point4:
      return admrender::OutputLayout::ITU_4_5_0;
    case Speakers::k7Point1:
      return admrender::OutputLayout::ITU_0_7_0;
    case Speakers::k7Point1Point2:
      return admrender::OutputLayout::_2_7_0;
    case Speakers::k7Point1Point4:
      return admrender::OutputLayout::ITU_4_7_0;
    case Speakers::kBinaural:
      return admrender::OutputLayout::Binaural;
    case Speakers::kExpl5Point1Point4Surround:
      return admrender::OutputLayout::ITU_4_5_0;
    case Speakers::kExplLFE:
    case Speakers::kExpl7Point1Point4SideSurround:
    case Speakers::kExpl7Point1Point4RearSurround:
    case Speakers::kExpl7Point1Point4TopFront:
    case Speakers::kExpl7Point1Point4TopBack:
    case Speakers::kExpl7Point1Point4Top:
    case Speakers::kExpl7Point1Point4Front:
      return admrender::OutputLayout::ITU_4_7_0;
    case Speakers::kExpl9Point1Point6:
    case Speakers::kExpl9Point1Point6Front:
    case Speakers::kExpl9Point1Point6Side:
    case Speakers::kExpl9Point1Point6TopSide:
    case Speakers::kExpl9Point1Point6Top:
      return admrender::OutputLayout::ITU_9_10_3;
  }
  return admrender::OutputLayout::ITU_0_2_0;
}

MonoToSpeakerPanner::MonoToSpeakerPanner(
    const Speakers::AudioElementSpeakerLayout pannedLayout,
    const int samplesPerBlock, const int sampleRate)
    : AudioPanner(pannedLayout, samplesPerBlock, sampleRate) {
  // Prepare a buffer to write the output audio to
  // For expanded layouts, we need to render to all channels and then copy out
  // the ones we need
  if (!pannedLayout.isExpandedLayout()) {
    outputAudioBufferPointers_ = new float*[kPannedLayout_.getNumChannels()];
  } else {
    // ITU 9_10_3 has 24 channels
    // See: https://www.itu.int/rec/R-REC-BS.2127-1-202311-I/en
    // For simplicity, create the widest buffer we could possibly need
    outputAudioBufferPointers_ = new float*[24];

    for (int i = 0; i < 24; i++) {
      outputAudioBufferPointers_[i] = new float[samplesPerBlock];
    }

    // Decide what channels we'd like
    // Note that 9.10.3 contains 8 additional channels over 9.1.6
    explValidChannels_ = kPannedLayout_.getExplValidChannels().value();
    if (AdmTypeFromPannedLayout(kPannedLayout_) ==
        admrender::OutputLayout::ITU_9_10_3) {
      // Configure a mapping from the 16 channels of 9.1.6 to the 24 channels
      // of 9.10.3
      explChannelPointers_.push_back(outputAudioBufferPointers_[0]);   // FL
      explChannelPointers_.push_back(outputAudioBufferPointers_[1]);   // FR
      explChannelPointers_.push_back(outputAudioBufferPointers_[2]);   // FC
      explChannelPointers_.push_back(outputAudioBufferPointers_[3]);   // LFE
      explChannelPointers_.push_back(outputAudioBufferPointers_[4]);   // BL
      explChannelPointers_.push_back(outputAudioBufferPointers_[5]);   // BR
      explChannelPointers_.push_back(outputAudioBufferPointers_[6]);   // FLc
      explChannelPointers_.push_back(outputAudioBufferPointers_[7]);   // FRc
      explChannelPointers_.push_back(outputAudioBufferPointers_[10]);  // SiL
      explChannelPointers_.push_back(outputAudioBufferPointers_[11]);  // SiR
      explChannelPointers_.push_back(outputAudioBufferPointers_[12]);  // TpFL
      explChannelPointers_.push_back(outputAudioBufferPointers_[13]);  // TpFR
      explChannelPointers_.push_back(outputAudioBufferPointers_[16]);  // TpBL
      explChannelPointers_.push_back(outputAudioBufferPointers_[17]);  // TpBR
      explChannelPointers_.push_back(outputAudioBufferPointers_[18]);  // TpSiL
      explChannelPointers_.push_back(outputAudioBufferPointers_[19]);  // TpSir
    } else {
      // The mappings are identical, so just add all the channels from the
      // panned layout
      for (int i = 0; i < kPannedLayout_.getChannelSet().size(); i++) {
        explChannelPointers_.push_back(outputAudioBufferPointers_[i]);
      }
    }
  }

  // Prepare the object metadata for panning
  objectMetadata_.trackInd = 0;
  objectMetadata_.blockLength = kSamplesPerBlock_;
  objectMetadata_.cartesian = false;
  objectMetadata_.channelLock = admrender::ChannelLock();
  objectMetadata_.channelLock->maxDistance = 0.01f;
  objectMetadata_.width = 0;
  objectMetadata_.jumpPosition.flag = true;
  objectMetadata_.screenRef = false;

  // Prepare a stream containing this object
  streamInfo_.nChannels = 1;
  streamInfo_.typeDefinition.push_back(admrender::TypeDefinition::Objects);

  // Prepate the renderer for panning
  renderer_.Configure(AdmTypeFromPannedLayout(pannedLayout), 0, kSampleRate_,
                      kSamplesPerBlock_, streamInfo_);
}

MonoToSpeakerPanner::~MonoToSpeakerPanner() {
  if (kPannedLayout_.isExpandedLayout()) {
    for (int i = 0; i < kPannedLayout_.getNumChannels(); i++) {
      delete[] outputAudioBufferPointers_[i];
    }
  }
  delete outputAudioBufferPointers_;
}

void MonoToSpeakerPanner::positionUpdated() {
  objectMetadata_.position.polarPosition().azimuth = currPos_.azimuth;
  objectMetadata_.position.polarPosition().elevation = currPos_.elevation;
  objectMetadata_.position.polarPosition().distance = currPos_.distance;
}

void MonoToSpeakerPanner::process(juce::AudioBuffer<float>& inputBuffer,
                                  juce::AudioBuffer<float>& outputBuffer) {
  // Add the object to the stream
  // Note that GetRendereredAudio basically resets the renderer, so objects must
  // be added every time
  //
  // I don't like using getWritePointer here, but getReadPointer returns a const
  // and libspatialaudio doesn't take a const, and we'd
  // rather not perform a copy
  float* inputAudio = inputBuffer.getWritePointer(0);
  renderer_.AddObject(inputAudio, kSamplesPerBlock_, objectMetadata_);
  outputBuffer.clear();
  // For non-expanded layouts, just write directly to the output buffer
  if (!kPannedLayout_.isExpandedLayout()) {
    // Same issue here, the write pointers are const but GetRenderedAudio isn't
    // Convert to non-const pointers pointing to the various output channels
    for (int i = 0; i < kPannedLayout_.getNumChannels(); i++) {
      outputAudioBufferPointers_[i] = outputBuffer.getWritePointer(i);
    }

    renderer_.GetRenderedAudio(outputAudioBufferPointers_, kSamplesPerBlock_);
  } else {
    // For expanded layouts, we need to copy out the channels we want
    // First, render to 9_10_3
    renderer_.GetRenderedAudio(outputAudioBufferPointers_, kSamplesPerBlock_);

    // Now copy out the desired channels
    for (int i = 0; i < explValidChannels_.size(); i++) {
      outputBuffer.copyFrom(i, 0, explChannelPointers_[explValidChannels_[i]],
                            kSamplesPerBlock_);
    }
  }
}