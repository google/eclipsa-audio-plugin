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

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class BinauralMixSource {
 public:
  virtual ~BinauralMixSource() = default;

  // The current block's 2-channel binaural mix, mix gain applied. Valid until
  // the source processes its next block.
  virtual const juce::AudioBuffer<float>& getBinauralMix() const = 0;
};
