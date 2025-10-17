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

#include "Speakers.h"

using namespace Speakers;

AudioElementSpeakerLayout::AudioElementSpeakerLayout(
    ChannelSet fromChannelSet) {
  if (fromChannelSet == ChannelSet::mono()) {
    index_ = kMono;
  } else if (fromChannelSet == ChannelSet::stereo()) {
    index_ = kStereo;
  } else if (fromChannelSet == ChannelSet::create5point1()) {
    index_ = k5Point1;
  } else if (fromChannelSet == ChannelSet::create5point1point2()) {
    index_ = k5Point1Point2;
  } else if (fromChannelSet == ChannelSet::create5point1point4()) {
    index_ = k5Point1Point4;
  } else if (fromChannelSet == ChannelSet::create7point1()) {
    index_ = k7Point1;
  } else if (fromChannelSet == ChannelSet::create7point1point2()) {
    index_ = k7Point1Point2;
  } else if (fromChannelSet == ChannelSet::create7point1point4()) {
    index_ = k7Point1Point4;
  } else if (fromChannelSet == ChannelSet::ambisonic(1)) {
    index_ = kHOA1;
  } else if (fromChannelSet == ChannelSet::ambisonic(2)) {
    index_ = kHOA2;
  } else if (fromChannelSet == ChannelSet::ambisonic(3)) {
    index_ = kHOA3;
  } else if (fromChannelSet == ChannelSet::ambisonic(4)) {
    index_ = kHOA4;
  } else if (fromChannelSet == ChannelSet::ambisonic(5)) {
    index_ = kHOA5;
  } else if (fromChannelSet == ChannelSet::ambisonic(6)) {
    index_ = kHOA6;
  } else if (fromChannelSet == ChannelSet::ambisonic(7)) {
    index_ = kHOA7;
  } else {
    index_ = kMono;
  }
}

Speakers::AudioElementSpeakerLayout::AudioElementSpeakerLayout(
    iamf_tools::api::OutputLayout layout)
    : index_(kMono) {
  switch (layout) {
    case iamf_tools::api::OutputLayout::kItu2051_SoundSystemA_0_2_0:
      index_ = kStereo;
      break;
    case iamf_tools::api::OutputLayout::kIAMF_SoundSystemExtension_2_3_0:
      index_ = k3Point1Point2;
      break;
    case iamf_tools::api::OutputLayout::kItu2051_SoundSystemB_0_5_0:
      index_ = k5Point1;
      break;
    case iamf_tools::api::OutputLayout::kItu2051_SoundSystemC_2_5_0:
      index_ = k5Point1Point2;
      break;
    case iamf_tools::api::OutputLayout::kItu2051_SoundSystemD_4_5_0:
      index_ = k5Point1Point4;
      break;
    case iamf_tools::api::OutputLayout::kItu2051_SoundSystemI_0_7_0:
      index_ = k7Point1;
      break;
    case iamf_tools::api::OutputLayout::kIAMF_SoundSystemExtension_2_7_0:
      index_ = k7Point1Point2;
      break;
    case iamf_tools::api::OutputLayout::kItu2051_SoundSystemJ_4_7_0:
      index_ = k7Point1Point4;
      break;
    case iamf_tools::api::OutputLayout::kItu2051_SoundSystemH_9_10_3:
      index_ = kExpl9Point1Point6;
      break;
    default:
      index_ = kMono;
      break;
  }
}

BaseLayout AudioElementSpeakerLayout::getIamfLayout() const {
  switch (index_) {
    case kMono:
      return BaseLayout::LOUDSPEAKER_LAYOUT_MONO;
    case kStereo:
      return BaseLayout::LOUDSPEAKER_LAYOUT_STEREO;
    case k3Point1Point2:
      return BaseLayout::LOUDSPEAKER_LAYOUT_3_1_2_CH;
    case k5Point1:
      return BaseLayout::LOUDSPEAKER_LAYOUT_5_1_CH;
    case k5Point1Point2:
      return BaseLayout::LOUDSPEAKER_LAYOUT_5_1_2_CH;
    case k5Point1Point4:
      return BaseLayout::LOUDSPEAKER_LAYOUT_5_1_4_CH;
    case k7Point1:
      return BaseLayout::LOUDSPEAKER_LAYOUT_7_1_CH;
    case k7Point1Point2:
      return BaseLayout::LOUDSPEAKER_LAYOUT_7_1_2_CH;
    case k7Point1Point4:
      return BaseLayout::LOUDSPEAKER_LAYOUT_7_1_4_CH;
    case kBinaural:
      return BaseLayout::LOUDSPEAKER_LAYOUT_BINAURAL;
    case kExplLFE:
    case kExpl5Point1Point4Surround:
    case kExpl7Point1Point4SideSurround:
    case kExpl7Point1Point4RearSurround:
    case kExpl7Point1Point4TopFront:
    case kExpl7Point1Point4TopBack:
    case kExpl7Point1Point4Top:
    case kExpl7Point1Point4Front:
    case kExpl9Point1Point6:
    case kExpl9Point1Point6Front:
    case kExpl9Point1Point6Side:
    case kExpl9Point1Point6TopSide:
    case kExpl9Point1Point6Top:
      return BaseLayout::LOUDSPEAKER_LAYOUT_EXPANDED;
  }
  return BaseLayout::LOUDSPEAKER_LAYOUT_INVALID;
}

ExpandedLayout AudioElementSpeakerLayout::getIamfExpl() const {
  switch (index_) {
    case kExplLFE:
      return ExpandedLayout::EXPANDED_LOUDSPEAKER_LAYOUT_LFE;
    case kExpl5Point1Point4Surround:
      return ExpandedLayout::EXPANDED_LOUDSPEAKER_LAYOUT_STEREO_S;
    case kExpl7Point1Point4SideSurround:
      return ExpandedLayout::EXPANDED_LOUDSPEAKER_LAYOUT_STEREO_SS;
    case kExpl7Point1Point4RearSurround:
      return ExpandedLayout::EXPANDED_LOUDSPEAKER_LAYOUT_STEREO_RS;
    case kExpl7Point1Point4TopFront:
      return ExpandedLayout::EXPANDED_LOUDSPEAKER_LAYOUT_STEREO_TF;
    case kExpl7Point1Point4TopBack:
      return ExpandedLayout::EXPANDED_LOUDSPEAKER_LAYOUT_STEREO_TB;
    case kExpl7Point1Point4Top:
      return ExpandedLayout::EXPANDED_LOUDSPEAKER_LAYOUT_TOP_4_CH;
    case kExpl7Point1Point4Front:
      return ExpandedLayout::EXPANDED_LOUDSPEAKER_LAYOUT_3_0_CH;
    case kExpl9Point1Point6:
      return ExpandedLayout::EXPANDED_LOUDSPEAKER_LAYOUT_9_1_6_CH;
    case kExpl9Point1Point6Front:
      return ExpandedLayout::EXPANDED_LOUDSPEAKER_LAYOUT_STEREO_F;
    case kExpl9Point1Point6Side:
      return ExpandedLayout::EXPANDED_LOUDSPEAKER_LAYOUT_STEREO_SI;
    case kExpl9Point1Point6TopSide:
      return ExpandedLayout::EXPANDED_LOUDSPEAKER_LAYOUT_STEREO_TP_SI;
    case kExpl9Point1Point6Top:
      return ExpandedLayout::EXPANDED_LOUDSPEAKER_LAYOUT_TOP_6_CH;
    default:
      return ExpandedLayout::EXPANDED_LOUDSPEAKER_LAYOUT_INVALID;
  }
}

int AudioElementSpeakerLayout::getUncoupledChannelCount() const {
  if (isAmbisonics()) {
    return getNumChannels();
  }

  switch (index_) {
    case kMono:
      return 1;
    case kStereo:
      return 0;
    case k3Point1Point2:
      return 2;
    case k5Point1:
      return 2;
    case k5Point1Point2:
      return 2;
    case k5Point1Point4:
      return 2;
    case k7Point1:
      return 2;
    case k7Point1Point2:
      return 2;
    case k7Point1Point4:
      return 2;
    case kBinaural:
      return 0;
    case kExplLFE:
      return 1;
    case kExpl7Point1Point4Top:
      return 0;
    case kExpl9Point1Point6:
      return 2;
    case kExpl7Point1Point4Front:
      return 1;
    case kExpl9Point1Point6Top:
      return 0;
    case kExpl5Point1Point4Surround:
    case kExpl7Point1Point4SideSurround:
    case kExpl7Point1Point4RearSurround:
    case kExpl7Point1Point4TopFront:
    case kExpl7Point1Point4TopBack:
    case kExpl9Point1Point6Front:
    case kExpl9Point1Point6Side:
    case kExpl9Point1Point6TopSide:
      return 0;
  }
  return -1;
}

int AudioElementSpeakerLayout::getCoupledChannelCount() const {
  if (isAmbisonics()) {
    return 0;
  }

  switch (index_) {
    case kMono:
      return 0;
    case kStereo:
      return 1;
    case k3Point1Point2:
      return 2;
    case k5Point1:
      return 2;
    case k5Point1Point2:
      return 3;
    case k5Point1Point4:
      return 4;
    case k7Point1:
      return 3;
    case k7Point1Point2:
      return 4;
    case k7Point1Point4:
      return 5;
    case kBinaural:
      return 1;
    case kExplLFE:
      return 0;
    case kExpl7Point1Point4Top:
      return 2;
    case kExpl9Point1Point6:
      return 7;
    case kExpl9Point1Point6Top:
      return 3;
    case kExpl5Point1Point4Surround:
    case kExpl7Point1Point4SideSurround:
    case kExpl7Point1Point4RearSurround:
    case kExpl7Point1Point4TopFront:
    case kExpl7Point1Point4TopBack:
    case kExpl7Point1Point4Front:
    case kExpl9Point1Point6Front:
    case kExpl9Point1Point6Side:
    case kExpl9Point1Point6TopSide:
      return 1;
  }
  return -1;
}

int AudioElementSpeakerLayout::getNumChannels() const {
  switch (index_) {
    case kMono:
      return 1;
    case kStereo:
      return 2;
    case k3Point1Point2:
      return 6;
    case k5Point1:
      return 6;
    case k5Point1Point2:
      return 8;
    case k5Point1Point4:
      return 10;
    case k7Point1:
      return 8;
    case k7Point1Point2:
      return 10;
    case k7Point1Point4:
      return 12;
    case kBinaural:
      return 2;
    case kHOA1:
      return 4;
    case kHOA2:
      return 9;
    case kHOA3:
      return 16;
    case kHOA4:
      return 25;
    case kHOA5:
      return 36;
    case kHOA6:
      return 49;
    case kHOA7:
      return 64;
    case k22p2:
      return 24;
    case kExplLFE:
      return 1;
    case kExpl5Point1Point4Surround:
      return 2;
    case kExpl7Point1Point4SideSurround:
      return 2;
    case kExpl7Point1Point4RearSurround:
      return 2;
    case kExpl7Point1Point4TopFront:
      return 2;
    case kExpl7Point1Point4TopBack:
      return 2;
    case kExpl7Point1Point4Top:
      return 4;
    case kExpl7Point1Point4Front:
      return 3;
    case kExpl9Point1Point6:
      return 16;
    case kExpl9Point1Point6Front:
      return 2;
    case kExpl9Point1Point6Side:
      return 2;
    case kExpl9Point1Point6TopSide:
      return 2;
    case kExpl9Point1Point6Top:
      return 6;
  }
  return -1;
}

juce::AudioChannelSet AudioElementSpeakerLayout::getChannelSet() const {
  switch (index_) {
    case kMono:
      return juce::AudioChannelSet::mono();
    case kStereo:
      return juce::AudioChannelSet::stereo();
    case k3Point1Point2:
      // Just create more channels in this case
      return juce::AudioChannelSet::create5point1();
    case k5Point1:
      return juce::AudioChannelSet::create5point1();
    case k5Point1Point2:
      return juce::AudioChannelSet::create5point1point2();
    case k5Point1Point4:
      return juce::AudioChannelSet::create5point1point4();
    case k7Point1:
      return juce::AudioChannelSet::create7point1();
    case k7Point1Point2:
      return juce::AudioChannelSet::create7point1point2();
    case k7Point1Point4:
      return juce::AudioChannelSet::create7point1point4();
    case kBinaural:
      return juce::AudioChannelSet::stereo();
    case kHOA1:
      return juce::AudioChannelSet::ambisonic(1);
    case kHOA2:
      return juce::AudioChannelSet::ambisonic(2);
    case kHOA3:
      return juce::AudioChannelSet::ambisonic(3);
    case kHOA4:
      return juce::AudioChannelSet::ambisonic(4);
    case kHOA5:
      return juce::AudioChannelSet::ambisonic(5);
    case kHOA6:
      return juce::AudioChannelSet::ambisonic(6);
    case kHOA7:
      return juce::AudioChannelSet::ambisonic(7);
    case kExpl5Point1Point4Surround:
      return juce::AudioChannelSet::create5point1point4();
    case kExplLFE:
    case kExpl7Point1Point4SideSurround:
    case kExpl7Point1Point4RearSurround:
    case kExpl7Point1Point4TopFront:
    case kExpl7Point1Point4TopBack:
    case kExpl7Point1Point4Top:
    case kExpl7Point1Point4Front:
      return juce::AudioChannelSet::create7point1point4();
    case kExpl9Point1Point6:
    case kExpl9Point1Point6Front:
    case kExpl9Point1Point6Side:
    case kExpl9Point1Point6TopSide:
    case kExpl9Point1Point6Top:
      return juce::AudioChannelSet::create9point1point6();
  }
  return juce::AudioChannelSet::disabled();
}

bool AudioElementSpeakerLayout::isAmbisonics() const {
  switch (index_) {
    case kHOA1:
    case kHOA2:
    case kHOA3:
    case kHOA4:
    case kHOA5:
    case kHOA6:
    case kHOA7:
      return true;
  }
  return false;
}

juce::String AudioElementSpeakerLayout::toString() const {
  switch (index_) {
    case kMono:
      return "Mono";
    case kStereo:
      return "Stereo";
    case k3Point1Point2:
      return "3.1.2";
    case k5Point1:
      return "5.1";
    case k5Point1Point2:
      return "5.1.2";
    case k5Point1Point4:
      return "5.1.4";
    case k7Point1:
      return "7.1";
    case k7Point1Point2:
      return "7.1.2";
    case k7Point1Point4:
      return "7.1.4";
    case kBinaural:
      return "Binaural";
    case kHOA1:
      return "1st Order Ambisonics";
    case kHOA2:
      return "2nd Order Ambisonics";
    case kHOA3:
      return "3rd Order Ambisonics";
    case kHOA4:
      return "4th Order Ambisonics";
    case kHOA5:
      return "5th Order Ambisonics";
    case kHOA6:
      return "6th Order Ambisonics";
    case kHOA7:
      return "7th Order Ambisonics";
    case kExplLFE:
      return "LFE";
    case kExpl5Point1Point4Surround:
      return "5.1.4 Surround (Ls/Rs)";
    case kExpl7Point1Point4SideSurround:
      return "7.1.4 Side Surround (Lss/Rss)";
    case kExpl7Point1Point4RearSurround:
      return "7.1.4 Rear Surround (Lrs/Rrs)";
    case kExpl7Point1Point4TopFront:
      return "7.1.4 Top Front (Ltf/Rtf)";
    case kExpl7Point1Point4TopBack:
      return "7.1.4 Top Back (Ltb/Rtb)";
    case kExpl7Point1Point4Top:
      return "7.1.4 Top (Ltf/Rtf/Ltb/Rtb)";
    case kExpl7Point1Point4Front:
      return "7.1.4 Front (L/C/R)";
    case kExpl9Point1Point6:
      return "9.1.6";
    case kExpl9Point1Point6Front:
      return "9.1.6 Front (FL/FR)";
    case kExpl9Point1Point6Side:
      return "9.1.6 Side (SiL/SiR)";
    case kExpl9Point1Point6TopSide:
      return "9.1.6 Top Side (TpSiL/TpSiR)";
    case kExpl9Point1Point6Top:
      return "9.1.6 Top (TpFL/TpFR/TpSiL/TpSiR/TpBL/TpBR)";
  }
  return "Unknown";
}

std::vector<juce::String> AudioElementSpeakerLayout::getSpeakerLabels() const {
  switch (index_) {
    case kMono:
      return {"M"};
    case kStereo:
      return {"L", "R"};
    case k3Point1Point2:
      return {"L", "R", "C", "LFE", "Ltf", "Rtf"};
    case k5Point1:
      return {"L", "R", "C", "LFE", "Ls", "Rs"};
    case k5Point1Point2:
      return {"L", "R", "C", "LFE", "Ls", "Rs", "Ltf", "Rtf"};
    case k5Point1Point4:
      return {"L", "R", "C", "LFE", "Ls", "Rs", "Ltf", "Rtf", "Ltr", "Rtr"};
    case k7Point1:
      return {"L", "R", "C", "LFE", "Lss", "Rss", "Lrs", "Rrs"};
    case k7Point1Point2:
      return {"L", "R", "C", "LFE", "Lss", "Rss", "Lrs", "Rrs", "Ltf", "Rtf"};
    case k7Point1Point4:
      return {"L",   "R",   "C",   "LFE", "Lss", "Rss",
              "Lrs", "Rrs", "Ltf", "Rtf", "Ltb", "Rtb"};
    case kBinaural:
      return {"L", "R"};
    case kExplLFE:
      return {"LFE"};
    case kExpl5Point1Point4Surround:
      return {"Ls", "Rs"};
    case kExpl7Point1Point4SideSurround:
      return {"Lss", "Rss"};
    case kExpl7Point1Point4RearSurround:
      return {"Lrs", "Rrs"};
    case kExpl7Point1Point4TopFront:
      return {"Ltf", "Rtf"};
    case kExpl7Point1Point4TopBack:
      return {"Ltb", "Rtb"};
    case kExpl7Point1Point4Top:
      return {"Ltf", "Rtf", "Ltb", "Rtb"};
    case kExpl7Point1Point4Front:
      return {"L", "R", "C"};
    case kExpl9Point1Point6:
      return {
          "FL",  "FR",  "FC",   "LFE",  "BL",   "BR",   "FLc",   "FRc",
          "SiL", "SiR", "TpFL", "TpFR", "TpBL", "TpBR", "TpSiL", "TpSiR",
      };
    case kExpl9Point1Point6Front:
      return {"FL", "FR"};
    case kExpl9Point1Point6Side:
      return {"SiL", "SiR"};
    case kExpl9Point1Point6TopSide:
      return {"TpSiL", "TpSiR"};
    case kExpl9Point1Point6Top:
      return {"TpFL", "TpFR", "TpSiL", "TpSiR", "TpBL", "TpBR"};
  }
  return {};
}

std::vector<iamf_tools_cli_proto::ChannelLabel>
AudioElementSpeakerLayout::getIamfChannelLabels() const {
  using namespace iamf_tools_cli_proto;
  if (isAmbisonics()) {
    std::vector<ChannelLabel> labels(getNumChannels());
    for (int i = CHANNEL_LABEL_A_0; i < CHANNEL_LABEL_A_0 + getNumChannels();
         ++i) {
      labels[i - CHANNEL_LABEL_A_0] = static_cast<ChannelLabel>(i);
    }
    return labels;
  }
  switch (index_) {
    case kMono:
      return {CHANNEL_LABEL_MONO};
    case kStereo:
      return {CHANNEL_LABEL_L_2, CHANNEL_LABEL_R_2};
    case k5Point1:
      return {CHANNEL_LABEL_L_5, CHANNEL_LABEL_R_5,  CHANNEL_LABEL_CENTRE,
              CHANNEL_LABEL_LFE, CHANNEL_LABEL_LS_5, CHANNEL_LABEL_RS_5};
    case k5Point1Point2:
      return {CHANNEL_LABEL_L_5,   CHANNEL_LABEL_R_5,  CHANNEL_LABEL_CENTRE,
              CHANNEL_LABEL_LFE,   CHANNEL_LABEL_LS_5, CHANNEL_LABEL_RS_5,
              CHANNEL_LABEL_LTF_2, CHANNEL_LABEL_RTF_2};
    case k5Point1Point4:
      return {CHANNEL_LABEL_L_5,   CHANNEL_LABEL_R_5,   CHANNEL_LABEL_CENTRE,
              CHANNEL_LABEL_LFE,   CHANNEL_LABEL_LS_5,  CHANNEL_LABEL_RS_5,
              CHANNEL_LABEL_LTF_4, CHANNEL_LABEL_RTF_4, CHANNEL_LABEL_LTB_4,
              CHANNEL_LABEL_RTB_4};
    case k7Point1:
      return {CHANNEL_LABEL_L_7,   CHANNEL_LABEL_R_7,   CHANNEL_LABEL_CENTRE,
              CHANNEL_LABEL_LFE,   CHANNEL_LABEL_LSS_7, CHANNEL_LABEL_RSS_7,
              CHANNEL_LABEL_LRS_7, CHANNEL_LABEL_RRS_7};
    case k7Point1Point2:
      return {CHANNEL_LABEL_L_7,   CHANNEL_LABEL_R_7,   CHANNEL_LABEL_CENTRE,
              CHANNEL_LABEL_LFE,   CHANNEL_LABEL_LSS_7, CHANNEL_LABEL_RSS_7,
              CHANNEL_LABEL_LRS_7, CHANNEL_LABEL_RRS_7, CHANNEL_LABEL_LTF_2,
              CHANNEL_LABEL_RTF_2};
    case k7Point1Point4:
      return {CHANNEL_LABEL_L_7,   CHANNEL_LABEL_R_7,   CHANNEL_LABEL_CENTRE,
              CHANNEL_LABEL_LFE,   CHANNEL_LABEL_LSS_7, CHANNEL_LABEL_RSS_7,
              CHANNEL_LABEL_LRS_7, CHANNEL_LABEL_RRS_7, CHANNEL_LABEL_LTF_4,
              CHANNEL_LABEL_RTF_4, CHANNEL_LABEL_LTB_4, CHANNEL_LABEL_RTB_4};
    case k3Point1Point2:
      return {CHANNEL_LABEL_L_3, CHANNEL_LABEL_R_3,   CHANNEL_LABEL_CENTRE,
              CHANNEL_LABEL_LFE, CHANNEL_LABEL_LTF_3, CHANNEL_LABEL_RTF_3};
    case kBinaural:
      return {CHANNEL_LABEL_L_2, CHANNEL_LABEL_R_2};
    case kExplLFE:
      return {CHANNEL_LABEL_LFE};
    case kExpl5Point1Point4Surround:
      return {CHANNEL_LABEL_LS_5, CHANNEL_LABEL_RS_5};
    case kExpl7Point1Point4SideSurround:
      return {CHANNEL_LABEL_LSS_7, CHANNEL_LABEL_RSS_7};
    case kExpl7Point1Point4RearSurround:
      return {CHANNEL_LABEL_LRS_7, CHANNEL_LABEL_RRS_7};
    case kExpl7Point1Point4TopFront:
      return {CHANNEL_LABEL_LTF_4, CHANNEL_LABEL_RTF_4};
    case kExpl7Point1Point4TopBack:
      return {CHANNEL_LABEL_LTB_4, CHANNEL_LABEL_RTB_4};
    case kExpl7Point1Point4Top:
      return {CHANNEL_LABEL_LTF_4, CHANNEL_LABEL_RTF_4, CHANNEL_LABEL_LTB_4,
              CHANNEL_LABEL_RTB_4};
    case kExpl7Point1Point4Front:
      return {CHANNEL_LABEL_L_7, CHANNEL_LABEL_R_7, CHANNEL_LABEL_CENTRE};
    case kExpl9Point1Point6:
      return {CHANNEL_LABEL_FL,     CHANNEL_LABEL_FR,    CHANNEL_LABEL_FC,
              CHANNEL_LABEL_LFE,    CHANNEL_LABEL_BL,    CHANNEL_LABEL_BR,
              CHANNEL_LABEL_FLC,    CHANNEL_LABEL_FRC,   CHANNEL_LABEL_SI_L,
              CHANNEL_LABEL_SI_R,   CHANNEL_LABEL_TP_FL, CHANNEL_LABEL_TP_FR,
              CHANNEL_LABEL_TP_BL,  CHANNEL_LABEL_TP_BR, CHANNEL_LABEL_TP_SI_L,
              CHANNEL_LABEL_TP_SI_R};
    case kExpl9Point1Point6Front:
      return {CHANNEL_LABEL_FL, CHANNEL_LABEL_FR};
    case kExpl9Point1Point6Side:
      return {CHANNEL_LABEL_SI_L, CHANNEL_LABEL_SI_R};
    case kExpl9Point1Point6TopSide:
      return {CHANNEL_LABEL_TP_SI_L, CHANNEL_LABEL_TP_SI_R};
    case kExpl9Point1Point6Top:
      return {CHANNEL_LABEL_TP_FL,   CHANNEL_LABEL_TP_FR, CHANNEL_LABEL_TP_SI_L,
              CHANNEL_LABEL_TP_SI_R, CHANNEL_LABEL_TP_BL, CHANNEL_LABEL_TP_BR};
    default:
      return {};
  }
}

std::string AudioElementSpeakerLayout::getItuString() const {
  switch (index_) {
    case kStereo:
      return "0+2+0";
    case k5Point1:
      return "0+5+0";
    case k5Point1Point2:
      return "2+5+0";
    case k5Point1Point4:
      return "4+5+0";
    case k7Point1:
      return "0+7+0";
    case k7Point1Point4:
      return "4+7+0";
    case k22p2:
      return "9+10+3";
    default:
      return "Unknown";
  }
}

bool AudioElementSpeakerLayout::isExpandedLayout() const {
  return (index_ >= firstExpandedLayout) && (index_ <= lastExpandedLayout);
}

Speakers::AudioElementSpeakerLayout
Speakers::AudioElementSpeakerLayout::getExplBaseLayout() const {
  if (!isExpandedLayout()) {
    return AudioElementSpeakerLayout(index_);
  }
  // IAMF 7.3.2.1.
  else if (index_ == kExpl5Point1Point4Surround) {
    return k5Point1Point4;
  } else if (index_ <= kExpl7Point1Point4Front) {
    return k7Point1Point4;
  } else {
    return kExpl9Point1Point6;
  }
}

// NOTE: This list of channels denotes where the channel indexes are based on
// the results of getExplBaseLayout()
// This means that the 9.1.6 channels are for 9.1.6, not 9+10+3, even
// though 9.1.6 is a subset of 9+10+3. This is because 9.1.6 is directly
// supported as a rendering target by the IAMF libraries
const std::optional<std::vector<int>>
AudioElementSpeakerLayout::getExplValidChannels() const {
  switch (index_) {
    case kExplLFE:
      return {{3}};
    case kExpl5Point1Point4Surround:
      return {{4, 5}};
    case kExpl7Point1Point4SideSurround:
      return {{4, 5}};
    case kExpl7Point1Point4RearSurround:
      return {{6, 7}};
    case kExpl7Point1Point4TopFront:
      return {{8, 9}};
    case kExpl7Point1Point4TopBack:
      return {{10, 11}};
    case kExpl7Point1Point4Top:
      return {{8, 9, 10, 11}};
    case kExpl7Point1Point4Front:
      return {{0, 1, 2}};
    case kExpl9Point1Point6Front:
      return {{0, 1}};
    case kExpl9Point1Point6Side:
      return {{8, 9}};
    case kExpl9Point1Point6TopSide:
      return {{14, 15}};
    case kExpl9Point1Point6Top:
      return {{10, 11, 12, 13, 14, 15}};
    case kExpl9Point1Point6:
      return {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
    default:
      return {};
  }
}

const std::vector<ChGainMap> AudioElementSpeakerLayout::getChGainMap() const {
  switch (index_) {
    case kMono:
      return {
          {0, 0, .70710678f},
          {0, 1, .70710678f},
      };
    case k7Point1Point2:
      return {
          {0, 0, 1.f},
          {1, 1, 1.f},
          {2, 2, 1.f},
          {3, 3, 1.f},
          {4, 4, 1.f},
          {5, 5, 1.f},
          {6, 6, 1.f},
          {7, 7, 1.f},
          {8, 8, 1.f},
          {9, 9, 1.f},
          {8, 10, .7071067811865476f},
          {9, 11, .7071067811865476f},
      };
    case k3Point1Point2:
      return {
          {0, 0, 1.f},
          {1, 1, 1.f},
          {2, 2, 1.f},
          {3, 3, 1.f},
          {1, 4, .7071067811865476f},
          {2, 5, .7071067811865476f},
          {4, 6, 1.f},
          {5, 7, 1.f},
      };
    case kExplLFE:
      return {
          {0, 3, 1.f},
      };
    case kExpl5Point1Point4Surround:
      return {
          {0, 4, 1.f},
          {1, 5, 1.f},
      };
    case kExpl7Point1Point4SideSurround:
      return {
          {0, 4, 1.f},
          {1, 5, 1.f},
      };
    case kExpl7Point1Point4RearSurround:
      return {
          {0, 6, 1.f},
          {1, 7, 1.f},
      };
    case kExpl7Point1Point4TopFront:
      return {
          {0, 8, 1.f},
          {1, 9, 1.f},
      };
    case kExpl7Point1Point4TopBack:
      return {
          {0, 10, 1.f},
          {1, 11, 1.f},
      };
    case kExpl7Point1Point4Top:
      return {
          {0, 8, 1.f},
          {1, 9, 1.f},
          {2, 10, 1.f},
          {3, 11, 1.f},
      };
    case kExpl7Point1Point4Front:
      return {
          {0, 0, 1.f},
          {1, 1, 1.f},
          {2, 2, 1.f},
      };
    case kExpl9Point1Point6:
      return {
          {0, 0, 1.f},   {1, 1, 1.f},   {2, 2, 1.f},   {3, 3, 1.f},
          {4, 4, 1.f},   {5, 5, 1.f},   {6, 6, 1.f},   {7, 7, 1.f},
          {8, 10, 1.f},  {9, 11, 1.f},  {10, 12, 1.f}, {11, 13, 1.f},
          {12, 18, 1.f}, {13, 19, 1.f}, {14, 16, 1.f}, {15, 17, 1.f},
      };
    case kExpl9Point1Point6Front:
      return {
          {0, 0, 1.f},
          {1, 1, 1.f},
      };
    case kExpl9Point1Point6Side:
      return {
          {0, 10, 1.f},
          {1, 11, 1.f},
      };
    case kExpl9Point1Point6TopSide:
      return {
          {0, 18, 1.f},
          {1, 19, 1.f},
      };
    case kExpl9Point1Point6Top:
      return {
          {0, 12, 1.f}, {1, 13, 1.f}, {2, 18, 1.f},
          {3, 19, 1.f}, {4, 16, 1.f}, {5, 17, 1.f},
      };
    default:
      return {};
  }
}

// used to preserve the order of the channels in the ITU layout
// use in place of AudioChannelSet::getChannelTypes() if the order is important
const juce::Array<ChannelType>
AudioElementSpeakerLayout::getITUChannelOrdering() const {
  switch (index_) {
    case kStereo:
      return {ChannelType::left, ChannelType::right};
    case k3Point1Point2:
      return {ChannelType::left,         ChannelType::right,
              ChannelType::centre,       ChannelType::LFE,
              ChannelType::topFrontLeft, ChannelType::topFrontRight};
    case k5Point1:
      return {ChannelType::left,         ChannelType::right,
              ChannelType::centre,       ChannelType::LFE,
              ChannelType::leftSurround, ChannelType::rightSurround};
    case k5Point1Point2:
      return {ChannelType::left,         ChannelType::right,
              ChannelType::centre,       ChannelType::LFE,
              ChannelType::leftSurround, ChannelType::rightSurround,
              ChannelType::topSideLeft,  ChannelType::topSideRight};
    case k5Point1Point4:
      return {ChannelType::left,         ChannelType::right,
              ChannelType::centre,       ChannelType::LFE,
              ChannelType::leftSurround, ChannelType::rightSurround,
              ChannelType::topFrontLeft, ChannelType::topFrontRight,
              ChannelType::topRearLeft,  ChannelType::topRearRight};
    case k7Point1:
      return {ChannelType::left,
              ChannelType::right,
              ChannelType::centre,
              ChannelType::LFE,
              ChannelType::leftSurroundSide,
              ChannelType::rightSurroundSide,
              ChannelType::leftSurroundRear,
              ChannelType::rightSurroundRear};
    case k7Point1Point4:
      return {ChannelType::left,
              ChannelType::right,
              ChannelType::centre,
              ChannelType::LFE,
              ChannelType::leftSurroundSide,
              ChannelType::rightSurroundSide,
              ChannelType::leftSurroundRear,
              ChannelType::rightSurroundRear,
              ChannelType::topFrontLeft,
              ChannelType::topFrontRight,
              ChannelType::topRearLeft,
              ChannelType::topRearRight};
    default:
      return {};
      break;
  }
}

// The inverse of the constructor that takes an OutputLayout
iamf_tools::api::OutputLayout AudioElementSpeakerLayout::getIamfOutputLayout()
    const {
  using OutputLayout = iamf_tools::api::OutputLayout;
  switch (index_) {
    case kStereo:
      return OutputLayout::kItu2051_SoundSystemA_0_2_0;
    case k3Point1Point2:
      return OutputLayout::kIAMF_SoundSystemExtension_2_3_0;
    case k5Point1:
      return OutputLayout::kItu2051_SoundSystemB_0_5_0;
    case k5Point1Point2:
      return OutputLayout::kItu2051_SoundSystemC_2_5_0;
    case k5Point1Point4:
      return OutputLayout::kItu2051_SoundSystemD_4_5_0;
    case k7Point1:
      return OutputLayout::kItu2051_SoundSystemI_0_7_0;
    case k7Point1Point2:
      return OutputLayout::kIAMF_SoundSystemExtension_2_7_0;
    case k7Point1Point4:
      return OutputLayout::kItu2051_SoundSystemJ_4_7_0;
    case kExpl9Point1Point6:
      return OutputLayout::kItu2051_SoundSystemH_9_10_3;
    default:
      return OutputLayout::kItu2051_SoundSystemA_0_2_0;
  }
}