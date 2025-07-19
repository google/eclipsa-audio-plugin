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

#include "RendererFactory.h"

#include "substream_rdr/bed2bed_rdr/BedToBedRdr.h"
#include "substream_rdr/bin_rdr/BinauralRdr.h"
#include "substream_rdr/hoa2bed_rdr/HOAToBedRdr.h"
#include "substream_rdr/passthrough_rdr/PassthroughRdr.h"

std::unique_ptr<Renderer> createRenderer(
    const Speakers::AudioElementSpeakerLayout inputLayout,
    const Speakers::AudioElementSpeakerLayout playbackLayout,
    const int numSamples, const int sampleRate) {
  // Binaural rendering is handled by a separate renderer.
  if (playbackLayout == Speakers::kBinaural) {
    return BinauralRdr::createBinauralRdr(inputLayout, numSamples, sampleRate);
  }
  // All other rendering is Channel-based or Scene-based.
  else {
    const iamf_tools_cli_proto::AudioElementType AEType =
        inputLayout.isAmbisonics()
            ? iamf_tools_cli_proto::AUDIO_ELEMENT_SCENE_BASED
            : iamf_tools_cli_proto::AUDIO_ELEMENT_CHANNEL_BASED;

    switch (AEType) {
      case AudioElement::AudioElement_t::AUDIO_ELEMENT_CHANNEL_BASED:
        if (inputLayout != playbackLayout) {
          return BedToBedRdr::createBedToBedRdr(inputLayout, playbackLayout);
        } else {
          return PassthroughRdr::createPassthroughRdr(playbackLayout);
        }
      case AudioElement::AudioElement_t::AUDIO_ELEMENT_SCENE_BASED:
        return HOAToBedRdr::createHOAToBedRdr(inputLayout, playbackLayout);
    }
  }
}