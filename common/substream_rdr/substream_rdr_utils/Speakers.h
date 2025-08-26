/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>

#include "audio_element.pb.h"
#include "audio_frame.pb.h"
#include "ear/ear.hpp"
#include "mix_presentation.pb.h"

using BaseLayout = iamf_tools_cli_proto::LoudspeakerLayout;
using ExpandedLayout = iamf_tools_cli_proto::ExpandedLoudspeakerLayout;

namespace Speakers {

typedef juce::AudioChannelSet ChannelSet;
typedef juce::AudioChannelSet::ChannelType ChannelType;
typedef juce::AudioBuffer<float> FBuffer;

// Utility functions for channel layout validation
inline bool isNamedBed(const juce::AudioChannelSet& s) {
  return s == juce::AudioChannelSet::stereo() ||
         s == juce::AudioChannelSet::create5point1() ||
         s == juce::AudioChannelSet::create7point1() ||
         s == juce::AudioChannelSet::create7point1point2() ||
         s == juce::AudioChannelSet::create7point1point4();
}

inline bool isSymmetricDiscrete(const juce::AudioChannelSet& s) {
  if (!s.isDiscreteLayout()) return false;
  const int n = s.size();
  return n >= 1 && n <= 16;  // covers 1,2,6,8,10,12 etc.
}

struct ChGainMap {
  int destIdx, srcIdx;
  float gain;
};

class AudioElementSpeakerLayout {
 public:
  constexpr AudioElementSpeakerLayout() : index_(0) {}
  constexpr explicit AudioElementSpeakerLayout(int index) : index_(index) {}
  explicit AudioElementSpeakerLayout(ChannelSet fromChannelSet);
  constexpr operator int() const { return index_; }

  BaseLayout getIamfLayout() const;
  int getNumChannels() const;
  juce::AudioChannelSet getChannelSet() const;
  juce::String toString() const;
  bool isAmbisonics() const;
  int getCoupledChannelCount() const;
  int getUncoupledChannelCount() const;
  std::vector<iamf_tools_cli_proto::ChannelLabel> getIamfChannelLabels() const;
  std::vector<juce::String> getSpeakerLabels() const;
  std::string getItuString() const;
  const juce::Array<ChannelType> getITUChannelOrdering() const;

  /**
   * @brief Checks whether this layout is an expanded loudspeaker layout, and
   * will therefore require different handling.
   *
   * @return bool
   */
  bool isExpandedLayout() const;

  /**
   * @brief Get the underlying loudspeaker layout for the current expanded
   * layout e.g., 7.1.4 for Stereo-Rs or 9.1.6 for SiL/SiR. If the layout is not
   * an expanded layout, simply returns the current layout.
   *
   * @return AudioElementSpeakerLayout
   */
  AudioElementSpeakerLayout getExplBaseLayout() const;

  /**
   * @brief Get the channels indices containing audio data for an expanded
   * layout. These indices reference the channels of the expanded layout's
   * *base* layout. As an example, the expanded layout Stereo-Rs
   * (kExpl7Point1Point4RearSurround) contains 2 channels worth of audio data,
   * which are mapped to channels 6 and 7 of the base layout 7.1.4.
   *
   * @return const std::vector<int>
   */
  const std::optional<std::vector<int>> getExplValidChannels() const;

  /**
   * @brief For expanded layouts and layouts that require downmixing to generate
   * (Mono, 3.1.2, and 7.1.2, any expanded layout with 9.1.6 as its base
   * layout), returns a set that contains the destination channel, its source
   * channel, and the gain to be applied to the source channel to generate the
   * dest channel.
   *
   * @return const std::vector<ChGainMap>
   */
  const std::vector<ChGainMap> getChGainMap() const;

  /**
   * @brief Returns the IAMF label for this expanded loudspeaker layout, or
   * INVALID if this loudspeaker layout is not an expanded loudspeaker layout.
   *
   */
  ExpandedLayout getIamfExpl() const;

 private:
  int index_;
};

/******************************
Standard Audio Element layouts
******************************/
constexpr int firstStandardLayout = 0;
constexpr AudioElementSpeakerLayout kMono(0);
constexpr AudioElementSpeakerLayout kStereo(1);
constexpr AudioElementSpeakerLayout k5Point1(2);
constexpr AudioElementSpeakerLayout k5Point1Point2(3);
constexpr AudioElementSpeakerLayout k5Point1Point4(4);
constexpr AudioElementSpeakerLayout k7Point1(5);
constexpr AudioElementSpeakerLayout k7Point1Point2(6);
constexpr AudioElementSpeakerLayout k7Point1Point4(7);
constexpr AudioElementSpeakerLayout k3Point1Point2(8);
constexpr AudioElementSpeakerLayout kBinaural(9);
constexpr AudioElementSpeakerLayout kHOA1(10);
constexpr AudioElementSpeakerLayout kHOA2(11);
constexpr AudioElementSpeakerLayout kHOA3(12);
constexpr int lastStandardLayout = 12;

/******************************
Expanded Audio Element layouts

NOTE: Layout 9.1.6 is peculiar as it isn't a base loudspeaker layout, but
rather an expanded loudspeaker layout. It's also a possible playback layout
per IAMF 7.3.2.2.
NOTE: Layout 9.1.6 is to be rendered from BS2051 9+10+3 with speaker subset:
{FL/FR/FC/LFE1/BL/BR/FLc/FRc/SiL/SiR/TpFL/TpFR/TpBL/TpBR/TpSiL/TpSiR} the
full layout being:
{FL/FR/FC/LFE1/BL/BR/FLc/FRc/BC/LFE2/SiL/SiR/TpFL/TpFR/TpFC/TPC/TpBL/TpBR/TpSiL/TpSiR/TpBC/BtFC/BtFL/BtFR}
******************************/
constexpr int firstExpandedLayout = 13;
constexpr AudioElementSpeakerLayout kExplLFE(13);
constexpr AudioElementSpeakerLayout kExpl5Point1Point4Surround(14);
constexpr AudioElementSpeakerLayout kExpl7Point1Point4SideSurround(15);
constexpr AudioElementSpeakerLayout kExpl7Point1Point4RearSurround(16);
constexpr AudioElementSpeakerLayout kExpl7Point1Point4TopFront(17);
constexpr AudioElementSpeakerLayout kExpl7Point1Point4TopBack(18);
constexpr AudioElementSpeakerLayout kExpl7Point1Point4Top(19);
constexpr AudioElementSpeakerLayout kExpl7Point1Point4Front(20);
constexpr AudioElementSpeakerLayout kExpl9Point1Point6(21);
constexpr AudioElementSpeakerLayout kExpl9Point1Point6Front(22);
constexpr AudioElementSpeakerLayout kExpl9Point1Point6Side(23);
constexpr AudioElementSpeakerLayout kExpl9Point1Point6TopSide(24);
constexpr AudioElementSpeakerLayout kExpl9Point1Point6Top(25);
constexpr int lastExpandedLayout = 25;

/******************************
Extra layouts required for rendering
******************************/
constexpr int firstExtraLayout = 26;
constexpr AudioElementSpeakerLayout k22p2(26);
constexpr AudioElementSpeakerLayout kHOA4(27);
constexpr AudioElementSpeakerLayout kHOA5(28);
constexpr AudioElementSpeakerLayout kHOA6(29);
constexpr AudioElementSpeakerLayout kHOA7(30);
constexpr int lastExtraLayout = 30;

constexpr AudioElementSpeakerLayout kUnknown(-1);
}  // namespace Speakers