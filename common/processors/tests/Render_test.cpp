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

#include <gtest/gtest.h>

#include "../processor_base/ProcessorBase.h"
#include "../render/RenderProcessor.h"
#include "data_structures/src/ActiveMixPresentation.h"
#include "data_structures/src/AudioElement.h"
#include "data_structures/src/MixPresentation.h"
#include "data_structures/src/RoomSetup.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

// Dummy processor to simulate a host processor.
class DummyHostProcessor final : public ProcessorBase {
 public:
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {
    // Provide a dummy implementation
  }
};

const int kSampleRate = 48000;

void ensureAmbisonicToStereoIsRenderedCorrectly(
    RoomSetupRepository roomSetupData, AudioElementRepository audioElementData,
    MixPresentationRepository mixPresData, ActiveMixRepository activeMixdata,
    SpeakerMonitorData& rtData) {
  DummyHostProcessor hostProc;
  RenderProcessor rProcessor(&hostProc, &roomSetupData, &audioElementData,
                             &mixPresData, &activeMixdata, rtData);

  RoomSetup setupInfo = roomSetupData.get();
  setupInfo.setSpeakerLayout(
      speakerLayoutConfigurationOptions[0]);  // Stereo speakers
  roomSetupData.update(setupInfo);

  // First, create an audio buffer and fill it with some test data
  // 9 Channels are needed for second order ambisonics
  int samplesPerBlock = 32;
  juce::AudioBuffer<float> testDataBuffer(9, samplesPerBlock);
  for (int i = 0; i < samplesPerBlock; i++) {
    testDataBuffer.setSample(0, i, i + 1);
    testDataBuffer.setSample(1, i, i + 1);
    testDataBuffer.setSample(2, i, i + 1);
    testDataBuffer.setSample(3, i, i + 1);
    testDataBuffer.setSample(4, i, i + 1);
    testDataBuffer.setSample(5, i, i + 1);
    testDataBuffer.setSample(6, i, i + 1);
    testDataBuffer.setSample(7, i, i + 1);
    testDataBuffer.setSample(8, i, i + 1);
  }
  juce::MidiBuffer midiBuffer;

  // Now process the buffer
  rProcessor.prepareToPlay(kSampleRate, samplesPerBlock);
  rProcessor.processBlock(testDataBuffer, midiBuffer);

  // Mono->Stereo should output data only on both channels
  for (int i = 0; i < samplesPerBlock; i++) {
    ASSERT_NE(testDataBuffer.getSample(0, i), 0);
    ASSERT_NE(testDataBuffer.getSample(1, i), 0);
    ASSERT_EQ(testDataBuffer.getSample(2, i), 0);
    ASSERT_EQ(testDataBuffer.getSample(3, i), 0);
    ASSERT_EQ(testDataBuffer.getSample(4, i), 0);
    ASSERT_EQ(testDataBuffer.getSample(5, i), 0);
    ASSERT_EQ(testDataBuffer.getSample(6, i), 0);
    ASSERT_EQ(testDataBuffer.getSample(7, i), 0);
    ASSERT_EQ(testDataBuffer.getSample(8, i), 0);
  }
}

void ensureTwoStereoElementsAreMixedCorrectly(
    RoomSetupRepository roomSetupData, AudioElementRepository audioElementData,
    MixPresentationRepository mixPresData, ActiveMixRepository activeMixdata,
    SpeakerMonitorData& rtData) {
  DummyHostProcessor hostProc;
  RenderProcessor rProcessor(&hostProc, &roomSetupData, &audioElementData,
                             &mixPresData, &activeMixdata, rtData);

  RoomSetup setupInfo = roomSetupData.get();
  setupInfo.setSpeakerLayout(
      speakerLayoutConfigurationOptions[0]);  // Stereo speakers
  roomSetupData.update(setupInfo);

  // First, create an audio buffer and fill it with some test data
  // Four channels are needed, two for each audio element
  int samplesPerBlock = 32;
  juce::AudioBuffer<float> testDataBuffer(4, samplesPerBlock);
  for (int i = 0; i < samplesPerBlock; i++) {
    testDataBuffer.setSample(0, i, i + 1);
    testDataBuffer.setSample(1, i, i + 1);
    testDataBuffer.setSample(2, i, i + 1);
    testDataBuffer.setSample(3, i, i + 1);
  }
  juce::MidiBuffer midiBuffer;

  // Now process the buffer
  rProcessor.prepareToPlay(kSampleRate, samplesPerBlock);
  rProcessor.processBlock(testDataBuffer, midiBuffer);

  // Stereo output should only be on the first 2 channels
  for (int i = 0; i < samplesPerBlock; i++) {
    ASSERT_NE(testDataBuffer.getSample(0, i), 0);
    ASSERT_NE(testDataBuffer.getSample(1, i), 0);
    ASSERT_EQ(testDataBuffer.getSample(2, i), 0);
    ASSERT_EQ(testDataBuffer.getSample(3, i), 0);
  }
}

void ensureMonoToStereoIsRenderedCorrectly(
    RoomSetupRepository roomSetupData, AudioElementRepository audioElementData,
    MixPresentationRepository mixPresData, ActiveMixRepository activeMixdata,
    SpeakerMonitorData& rtData) {
  DummyHostProcessor hostProc;
  RenderProcessor rProcessor(&hostProc, &roomSetupData, &audioElementData,
                             &mixPresData, &activeMixdata, rtData);

  RoomSetup setupInfo = roomSetupData.get();
  setupInfo.setSpeakerLayout(
      speakerLayoutConfigurationOptions[0]);  // Stereo speakers
  roomSetupData.update(setupInfo);

  // First, create an audio buffer and fill it with some test data
  // Two channels are still needed, since we'll output to the second channel
  int samplesPerBlock = 32;
  juce::AudioBuffer<float> testDataBuffer(2, samplesPerBlock);
  for (int i = 0; i < samplesPerBlock; i++) {
    testDataBuffer.setSample(0, i, i + 1);
    testDataBuffer.setSample(1, i, 0);
  }
  juce::MidiBuffer midiBuffer;

  // Now process the buffer
  rProcessor.prepareToPlay(kSampleRate, samplesPerBlock);
  rProcessor.processBlock(testDataBuffer, midiBuffer);

  // Mono->Stereo should output data only on both channels
  for (int i = 0; i < samplesPerBlock; i++) {
    ASSERT_NE(testDataBuffer.getSample(0, i), 0);
    ASSERT_NE(testDataBuffer.getSample(1, i), 0);
  }
}

void ensureStereoToFiveOneIsRenderedCorrectly(
    RoomSetupRepository roomSetupData, AudioElementRepository audioElementData,
    MixPresentationRepository mixPresData, ActiveMixRepository activeMixdata,
    SpeakerMonitorData& rtData) {
  DummyHostProcessor hostProc;
  RenderProcessor rProcessor(&hostProc, &roomSetupData, &audioElementData,
                             &mixPresData, &activeMixdata, rtData);

  RoomSetup setupInfo = roomSetupData.get();
  setupInfo.setSpeakerLayout(
      speakerLayoutConfigurationOptions[2]);  // 5.1 speakers
  roomSetupData.update(setupInfo);

  // First, create an audio buffer and fill it with some test data
  int samplesPerBlock = 32;
  juce::AudioBuffer<float> testDataBuffer(7, samplesPerBlock);
  for (int i = 0; i < samplesPerBlock; i++) {
    testDataBuffer.setSample(0, i, i + 1);
    testDataBuffer.setSample(1, i, i + 1);
    testDataBuffer.setSample(2, i, 0);
    testDataBuffer.setSample(3, i, 0);
    testDataBuffer.setSample(4, i, 0);
    testDataBuffer.setSample(5, i, 0);
    testDataBuffer.setSample(6, i, 0);
  }
  juce::MidiBuffer midiBuffer;

  // Now process the buffer
  rProcessor.prepareToPlay(kSampleRate, samplesPerBlock);
  rProcessor.processBlock(testDataBuffer, midiBuffer);

  for (int i = 0; i < samplesPerBlock; i++) {
    ASSERT_NE(testDataBuffer.getSample(0, i), 0);
    ASSERT_NE(testDataBuffer.getSample(1, i), 0);
  }
}

void ensureStereoToStereoIsRenderedCorrectly(
    RoomSetupRepository roomSetupData, AudioElementRepository audioElementData,
    MixPresentationRepository mixPresData, ActiveMixRepository activeMixdata,
    SpeakerMonitorData& rtData) {
  DummyHostProcessor hostProc;
  RenderProcessor rProcessor(&hostProc, &roomSetupData, &audioElementData,
                             &mixPresData, &activeMixdata, rtData);

  RoomSetup setupInfo = roomSetupData.get();
  setupInfo.setSpeakerLayout(
      speakerLayoutConfigurationOptions[0]);  // Stereo speakers
  roomSetupData.update(setupInfo);

  // First, create an audio buffer and fill it with some test data
  int samplesPerBlock = 512;
  juce::AudioBuffer<float> testDataBuffer(2, samplesPerBlock);
  for (int i = 0; i < samplesPerBlock; i++) {
    testDataBuffer.setSample(0, i, i);
    testDataBuffer.setSample(1, i, i);
  }
  juce::MidiBuffer midiBuffer;

  // Now process the buffer
  rProcessor.prepareToPlay(kSampleRate, samplesPerBlock);
  rProcessor.processBlock(testDataBuffer, midiBuffer);

  // Stereo->Stereo should output the same data
  // Verify the buffer is unchanged
  for (int i = 0; i < 24; i++) {
    ASSERT_EQ(testDataBuffer.getSample(0, i), i);
    ASSERT_EQ(testDataBuffer.getSample(1, i), i);
  }
}

void ensureRoomUpdatesWhenRoomSetupChanges(
    RoomSetupRepository roomSetupData, AudioElementRepository audioElementData,
    MixPresentationRepository mixPresData, ActiveMixRepository activeMixdata,
    SpeakerMonitorData& rtData) {
  DummyHostProcessor hostProc;
  RenderProcessor rProcessor(&hostProc, &roomSetupData, &audioElementData,
                             &mixPresData, &activeMixdata, rtData);

  ASSERT_EQ(rProcessor.getSpeakersOut(), Speakers::kStereo.getNumChannels());
  ASSERT_EQ(rProcessor.getAudioElementRenderers().size(), 1);

  ASSERT_EQ(
      rProcessor.getAudioElementRenderers()[0]->outputData.getNumChannels(),
      Speakers::kStereo.getNumChannels());

  RoomSetup setupInfo = roomSetupData.get();
  setupInfo.setSpeakerLayout(speakerLayoutConfigurationOptions[2]);  // 5,1
  roomSetupData.update(setupInfo);

  ASSERT_EQ(rProcessor.getSpeakersOut(), Speakers::k5Point1.getNumChannels());
  ASSERT_EQ(rProcessor.getAudioElementRenderers().size(), 1);
}

void ensureProcessorInitializedCorrectly(
    RoomSetupRepository roomSetupData, AudioElementRepository audioElementData,
    MixPresentationRepository mixPresData, ActiveMixRepository activeMixdata,
    SpeakerMonitorData& rtData) {
  DummyHostProcessor hostProc;
  RenderProcessor rProcessor(&hostProc, &roomSetupData, &audioElementData,
                             &mixPresData, &activeMixdata, rtData);

  ASSERT_EQ(rProcessor.getSpeakersOut(), Speakers::kStereo.getNumChannels());
  ASSERT_EQ(rProcessor.getAudioElementRenderers().size(), 1);

  ASSERT_EQ(
      rProcessor.getAudioElementRenderers()[0]->outputData.getNumChannels(),
      Speakers::kStereo.getNumChannels());
}

TEST(test_render_processor, addbus) {
  juce::ValueTree temporaryState("Test");
  RoomSetupRepository roomSetupData(
      temporaryState.getOrCreateChildWithName(RoomSetup::kTreeType, nullptr));
  AudioElementRepository audioElementData(
      temporaryState.getOrCreateChildWithName(AudioElement::kTreeType,
                                              nullptr));
  MixPresentationRepository mixPresData(temporaryState.getOrCreateChildWithName(
      MixPresentation::kTreeType, nullptr));
  ActiveMixRepository activeMixPresData(temporaryState.getOrCreateChildWithName(
      ActiveMixPresentation::kTreeType, nullptr));
  SpeakerMonitorData spkrMonitorData;

  AudioElement stereoElement(juce::Uuid(), "TestElement", Speakers::kStereo, 0);
  audioElementData.add(stereoElement);
  int count = audioElementData.getItemCount();

  RoomSetup setupInfo = roomSetupData.get();
  setupInfo.setSpeakerLayout(RoomLayout(Speakers::kStereo, "stereo"));
  roomSetupData.update(setupInfo);

  // Create a mix presentation and add the existing audio element to it.
  // Update the active mix presentation repo (we want to test rendering the
  // first MP).
  juce::Uuid pres1ID{}, pres2ID{}, pres3ID{}, pres4ID{};
  MixPresentation presentation1(pres1ID, "1", 1.f,
                                LanguageData::MixLanguages::English, {}),
      presentation2(pres2ID, "2", 1.f, LanguageData::MixLanguages::English, {}),
      presentation3(pres3ID, "3", 1.f, LanguageData::MixLanguages::English, {}),
      presentation4(pres4ID, "4", 1.f, LanguageData::MixLanguages::English, {});

  // These tests rely on one stereo audio element
  presentation1.addAudioElement(stereoElement.getId(), 1.f,
                                stereoElement.getName());
  mixPresData.updateOrAdd(presentation1);
  activeMixPresData.update(pres1ID);
  ensureRoomUpdatesWhenRoomSetupChanges(roomSetupData, audioElementData,
                                        mixPresData, activeMixPresData,
                                        spkrMonitorData);
  ensureStereoToStereoIsRenderedCorrectly(roomSetupData, audioElementData,
                                          mixPresData, activeMixPresData,
                                          spkrMonitorData);
  ensureStereoToFiveOneIsRenderedCorrectly(roomSetupData, audioElementData,
                                           mixPresData, activeMixPresData,
                                           spkrMonitorData);

  // These tests rely on a mono audio element
  AudioElement monoElement(juce::Uuid(), "TestElement", Speakers::kMono, 0);
  audioElementData.add(monoElement);
  presentation2.addAudioElement(monoElement.getId(), 1.f,
                                monoElement.getName());
  mixPresData.updateOrAdd(presentation2);
  activeMixPresData.update(pres2ID);
  ensureMonoToStereoIsRenderedCorrectly(roomSetupData, audioElementData,
                                        mixPresData, activeMixPresData,
                                        spkrMonitorData);

  // These tests rely on two stereo audio elements
  AudioElement stereoElement2(juce::Uuid(), "TestElement2", Speakers::kStereo,
                              0);
  audioElementData.add(stereoElement2);
  presentation3.addAudioElement(stereoElement.getId(), 1.f,
                                stereoElement.getName());
  presentation3.addAudioElement(stereoElement2.getId(), 1.f,
                                stereoElement2.getName());
  mixPresData.updateOrAdd(presentation3);
  activeMixPresData.update(pres3ID);
  ensureTwoStereoElementsAreMixedCorrectly(roomSetupData, audioElementData,
                                           mixPresData, activeMixPresData,
                                           spkrMonitorData);

  // These tests rely on an ambisonic audio element
  AudioElement ambisonicElement(juce::Uuid(), "TestAmbisonic", Speakers::kHOA2,
                                0);
  audioElementData.add(ambisonicElement);
  presentation4.addAudioElement(ambisonicElement.getId(), 1.f,
                                ambisonicElement.getName());
  mixPresData.updateOrAdd(presentation4);
  activeMixPresData.update(pres4ID);
  ensureAmbisonicToStereoIsRenderedCorrectly(roomSetupData, audioElementData,
                                             mixPresData, activeMixPresData,
                                             spkrMonitorData);
}

class test_render_proc : public ::testing::Test {
 protected:
  test_render_proc()
      : temporaryState("Test"),
        // Initialize repositories as children of the temporary state tree.
        roomSetupData(temporaryState.getOrCreateChildWithName(
            RoomSetup::kTreeType, nullptr)),
        audioElementData(temporaryState.getOrCreateChildWithName(
            AudioElement::kTreeType, nullptr)),
        mixPresData(temporaryState.getOrCreateChildWithName(
            MixPresentation::kTreeType, nullptr)),
        activeMixPresData(temporaryState.getOrCreateChildWithName(
            ActiveMixPresentation::kTreeType, nullptr)),
        host(),
        proc(&host, &roomSetupData, &audioElementData, &mixPresData,
             &activeMixPresData, rtData) {}

  // Fill a buffer with a sinewave for a given amplitude, frequency, and phase
  // and the wave generated should be continuous.
  void fillBuffer(const int a, const int f, const float ph,
                  juce::AudioBuffer<float>& buffer) {}

  // Run the render processor.

  // Write the renderer audio from the render processor to a test wav file.

  juce::AudioBuffer<float> unityBuffer() {
    juce::AudioBuffer<float> buffer(kDefaultBusLayout, kSamplesPerBlock);
    for (int i = 0; i < buffer.getNumChannels(); ++i) {
      for (int j = 0; j < buffer.getNumSamples(); ++j) {
        buffer.setSample(i, j, 1.f);
      }
    }
    return buffer;
  }

  // Test constants.
  const int kSampleRate = 48e3;
  const int kSamplesPerBlock = 128;
  const int kDuration = 2;
  const Speakers::AudioElementSpeakerLayout kDefaultBusLayout = Speakers::kHOA5;
  // Supported playback layouts.
  const std::vector<Speakers::AudioElementSpeakerLayout> playbackLayouts = {
      Speakers::kStereo,        Speakers::k3Point1Point2, Speakers::k5Point1,
      Speakers::k5Point1Point2, Speakers::k5Point1Point4, Speakers::k7Point1,
      Speakers::k7Point1Point2, Speakers::k7Point1Point4, Speakers::kBinaural};
  // Repository state tree.
  juce::ValueTree temporaryState;
  // Repositories.
  RoomSetupRepository roomSetupData;
  AudioElementRepository audioElementData;
  MixPresentationRepository mixPresData;
  ActiveMixRepository activeMixPresData;
  // Local data placeholders.
  RoomSetup room;
  juce::OwnedArray<AudioElement> audioElements;
  juce::OwnedArray<MixPresentation> mixes;
  ActiveMixPresentation activeMix;
  // Realtime data.
  SpeakerMonitorData rtData;
  // Host processor.
  DummyHostProcessor host;
  juce::MidiBuffer emptyMidi;
  // Processor under test.
  RenderProcessor proc;
};

// No audio elements, no mix presentations, no active mix presentation.
TEST_F(test_render_proc, no_ae_no_mp_no_amp) {
  juce::AudioBuffer<float> buffer = unityBuffer();
  for (const auto& layout : playbackLayouts) {
    room.setSpeakerLayout(RoomLayout(layout, layout.toString().toStdString()));
    roomSetupData.update(room);

    proc.prepareToPlay(kSampleRate, kSamplesPerBlock);

    proc.processBlock(buffer, emptyMidi);

    // As there are no Audio Elements, Mix Presentations, no Active Mix
    // Presentation, expect for a buffer filled with garbage to have an empty
    // buffer post rendering.
    for (int i = 0; i < buffer.getNumChannels(); ++i) {
      for (int j = 0; j < buffer.getNumSamples(); ++j) {
        EXPECT_EQ(buffer.getSample(i, j), 0.f);
      }
    }
  }
}

// No audio elements, no mix presentations, active mix presentation.
TEST_F(test_render_proc, no_ae_no_mp_amp) {
  juce::AudioBuffer<float> buffer = unityBuffer();
  for (const auto& layout : playbackLayouts) {
    room.setSpeakerLayout(RoomLayout(layout, layout.toString().toStdString()));
    roomSetupData.update(room);

    proc.prepareToPlay(kSampleRate, kSamplesPerBlock);

    activeMix.updateActiveMixId(juce::Uuid());
    activeMixPresData.update(activeMix);

    proc.prepareToPlay(kSampleRate, kSamplesPerBlock);

    proc.processBlock(buffer, emptyMidi);

    // As there are no Audio Elements, Mix Presentations, no Active Mix
    // Presentation, expect for a buffer filled with garbage to have an empty
    // buffer post rendering.
    for (int i = 0; i < buffer.getNumChannels(); ++i) {
      for (int j = 0; j < buffer.getNumSamples(); ++j) {
        EXPECT_EQ(buffer.getSample(i, j), 0.f);
      }
    }
  }
}

// 1 Audio Element, no Mix Presentation, no active mix presentation selected.
TEST_F(test_render_proc, one_ae_no_mp_no_amp) {
  juce::AudioBuffer<float> buffer = unityBuffer();
  for (const auto& layout : playbackLayouts) {
    room.setSpeakerLayout(RoomLayout(layout, layout.toString().toStdString()));
    roomSetupData.update(room);

    AudioElement ae(juce::Uuid(), "Test", layout, 0);
    audioElementData.add(ae);

    proc.prepareToPlay(kSampleRate, kSamplesPerBlock);

    proc.processBlock(buffer, emptyMidi);

    // Although there is 1 Audio Element, there are no Mix Presentations, no
    // Active Mix Presentation, so we expect an empty buffer post rendering.
    for (int i = 0; i < buffer.getNumChannels(); ++i) {
      for (int j = 0; j < buffer.getNumSamples(); ++j) {
        EXPECT_EQ(buffer.getSample(i, j), 0.f);
      }
    }
  }
}

// 1 Audio Element, no Mix Presentation, active mix presentation selected.
TEST_F(test_render_proc, one_ae_no_mp_amp) {
  juce::AudioBuffer<float> buffer = unityBuffer();
  for (const auto& layout : playbackLayouts) {
    room.setSpeakerLayout(RoomLayout(layout, layout.toString().toStdString()));
    roomSetupData.update(room);

    AudioElement ae(juce::Uuid(), "Test", layout, 0);
    audioElementData.add(ae);

    proc.prepareToPlay(kSampleRate, kSamplesPerBlock);

    activeMix.updateActiveMixId(juce::Uuid());
    activeMixPresData.update(activeMix);

    proc.prepareToPlay(kSampleRate, kSamplesPerBlock);

    proc.processBlock(buffer, emptyMidi);

    // Although there is 1 Audio Element and an active Mix Presentation, there
    // is no Mix Presentation matching the Active Mix ID and we expect an empty
    // buffer post rendering.
    for (int i = 0; i < buffer.getNumChannels(); ++i) {
      for (int j = 0; j < buffer.getNumSamples(); ++j) {
        EXPECT_EQ(buffer.getSample(i, j), 0.f);
      }
    }
  }
}

// 1 Audio Element, 1 Mix Presentation, no active mix presentation selected.
TEST_F(test_render_proc, one_ae_one_mp_no_amp) {
  juce::AudioBuffer<float> buffer = unityBuffer();
  for (const auto& layout : playbackLayouts) {
    room.setSpeakerLayout(RoomLayout(layout, layout.toString().toStdString()));
    roomSetupData.update(room);

    AudioElement ae(juce::Uuid(), "Test", layout, 0);
    audioElementData.add(ae);

    MixPresentation mp(juce::Uuid(), "Test", 1.f,
                       LanguageData::MixLanguages::English, {});
    mp.addAudioElement(ae.getId(), 1.f, ae.getName());
    mixPresData.updateOrAdd(mp);

    proc.prepareToPlay(kSampleRate, kSamplesPerBlock);

    proc.processBlock(buffer, emptyMidi);

    // Although there is 1 Audio Element and 1 Mix Presentation, there is no
    // Active Mix Presentation selected and we expect an empty buffer post
    // rendering.
    for (int i = 0; i < buffer.getNumChannels(); ++i) {
      for (int j = 0; j < buffer.getNumSamples(); ++j) {
        EXPECT_EQ(buffer.getSample(i, j), 0.f);
      }
    }
  }
}

// 1 Audio Element, 1 Mix Presentation, active mix presentation selected.
TEST_F(test_render_proc, one_ae_one_mp_amp) {
  Speakers::AudioElementSpeakerLayout layout = Speakers::kStereo;
  room.setSpeakerLayout(RoomLayout(layout, layout.toString().toStdString()));
  roomSetupData.update(room);

  AudioElement ae(juce::Uuid(), "5.1.2 AE", Speakers::k5Point1Point2, 0);
  audioElementData.add(ae);

  juce::Uuid mpId;
  MixPresentation mp(mpId, "Test", 1.f, LanguageData::MixLanguages::English,
                     {});
  mp.addAudioElement(ae.getId(), 1.f, ae.getName());
  mixPresData.updateOrAdd(mp);

  activeMix.updateActiveMixId(mpId);
  activeMixPresData.update(activeMix);

  proc.prepareToPlay(kSampleRate, kSamplesPerBlock);

  juce::AudioBuffer<float> buffer = unityBuffer();
  proc.processBlock(buffer, emptyMidi);

  // With 1 Audio Element, 1 Mix Presentation, and an Active Mix Presentation
  // we expect the output buffer to have data on the channels that comprise
  // the playback layout.
  for (int chIdx = 0; chIdx < layout.getNumChannels(); ++chIdx) {
    for (int j = 0; j < buffer.getNumSamples(); ++j) {
      ASSERT_NE(buffer.getSample(chIdx, j), 0.f)
          << "Failed rendering " << ae.getName() << " to " << layout.toString();
    }
  }
  for (int chIdx = layout.getNumChannels(); chIdx < buffer.getNumChannels();
       ++chIdx) {
    for (int j = 0; j < buffer.getNumSamples(); ++j) {
      ASSERT_EQ(buffer.getSample(chIdx, j), 0.f)
          << "Failed rendering " << ae.getName() << " to " << layout.toString();
    }
  }
}

// Rendering multiple audio elements in an active mix presentation.
TEST_F(test_render_proc, multiple_ae_one_mp_amp) {
  Speakers::AudioElementSpeakerLayout layout = Speakers::k5Point1Point2;
  room.setSpeakerLayout(RoomLayout(layout, layout.toString().toStdString()));
  roomSetupData.update(room);

  AudioElement ae1(juce::Uuid(), "5.1.2 AE 1", Speakers::k5Point1Point2, 0);
  audioElementData.add(ae1);
  AudioElement ae2(juce::Uuid(), "Stereo-F AE 2",
                   Speakers::kExpl9Point1Point6Front, 8);
  audioElementData.add(ae2);

  juce::Uuid mpId;
  MixPresentation mp(mpId, "Test", 1.f, LanguageData::MixLanguages::English,
                     {});
  mp.addAudioElement(ae1.getId(), 1.f, ae1.getName());
  mp.addAudioElement(ae2.getId(), 1.f, ae2.getName());
  mixPresData.updateOrAdd(mp);

  activeMix.updateActiveMixId(mpId);
  activeMixPresData.update(activeMix);

  proc.prepareToPlay(kSampleRate, kSamplesPerBlock);

  juce::AudioBuffer<float> buffer = unityBuffer();
  proc.processBlock(buffer, emptyMidi);

  // With 2 Audio Elements, 1 Mix Presentation, and an Active Mix Presentation
  // we expect the output buffer to have data on the channels that comprise
  // the playback layout.
  for (int chIdx = 0; chIdx < layout.getNumChannels(); ++chIdx) {
    for (int j = 0; j < buffer.getNumSamples(); ++j) {
      ASSERT_NE(buffer.getSample(chIdx, j), 0.f)
          << "Failed rendering " << ae1.getName() << " and " << ae2.getName()
          << " to " << layout.toString();
    }
  }
  for (int chIdx = layout.getNumChannels(); chIdx < buffer.getNumChannels();
       ++chIdx) {
    for (int j = 0; j < buffer.getNumSamples(); ++j) {
      ASSERT_EQ(buffer.getSample(chIdx, j), 0.f)
          << "Failed rendering " << ae1.getName() << " and " << ae2.getName()
          << " to " << layout.toString();
    }
  }
}

// Multiple mix presentations test rendering the correct one.
TEST_F(test_render_proc, multiple_mp_amp) {
  Speakers::AudioElementSpeakerLayout layout = Speakers::k5Point1Point2;
  room.setSpeakerLayout(RoomLayout(layout, layout.toString().toStdString()));
  roomSetupData.update(room);

  AudioElement ae1(juce::Uuid(), "5.1.2 AE 1", Speakers::k5Point1Point2, 0);
  audioElementData.add(ae1);
  AudioElement ae2(juce::Uuid(), "5.1.2 AE 2", Speakers::k5Point1Point2, 0);
  audioElementData.add(ae2);

  juce::Uuid mpId1, mpId2;
  MixPresentation mp1(mpId1, "Test 1", 1.f, LanguageData::MixLanguages::English,
                      {});
  mp1.addAudioElement(ae1.getId(), 1.f, ae1.getName());
  mp1.addAudioElement(ae2.getId(), 1.f, ae2.getName());
  mixPresData.updateOrAdd(mp1);
  MixPresentation mp2(mpId2, "Test 2", 0.f, LanguageData::MixLanguages::English,
                      {});
  mp2.addAudioElement(ae1.getId(), 1.f, ae1.getName());
  mp2.addAudioElement(ae2.getId(), 1.f, ae2.getName());
  mixPresData.updateOrAdd(mp2);

  activeMix.updateActiveMixId(mpId2);
  activeMixPresData.update(activeMix);

  proc.prepareToPlay(kSampleRate, kSamplesPerBlock);

  juce::AudioBuffer<float> buffer = unityBuffer();
  proc.processBlock(buffer, emptyMidi);

  // With 2 Audio Elements, 2 Mix Presentations, and an Active Mix Presentation
  // with a mix gain of 0, we expect all playback channels to be zero'd.
  for (int chIdx = 0; chIdx < layout.getNumChannels(); ++chIdx) {
    for (int j = 0; j < buffer.getNumSamples(); ++j) {
      ASSERT_EQ(buffer.getSample(chIdx, j), 0.f);
    }
  }
  for (int chIdx = layout.getNumChannels(); chIdx < buffer.getNumChannels();
       ++chIdx) {
    for (int j = 0; j < buffer.getNumSamples(); ++j) {
      ASSERT_EQ(buffer.getSample(chIdx, j), 0.f);
    }
  }
}

// One mix presentation with 28 mono audio elements.
TEST_F(test_render_proc, many_ae_one_mp_amp) {
  Speakers::AudioElementSpeakerLayout layout = Speakers::kStereo;
  room.setSpeakerLayout(RoomLayout(layout, layout.toString().toStdString()));
  roomSetupData.update(room);

  juce::Uuid mpId;
  MixPresentation mp(mpId, "Test", 1.f, LanguageData::MixLanguages::English,
                     {});
  for (int i = 0; i < 28; ++i) {
    AudioElement ae(juce::Uuid(), "Mono AE " + std::to_string(i),
                    Speakers::kMono, 0);
    audioElementData.add(ae);
    mp.addAudioElement(ae.getId(), 1.f, ae.getName());
  }
  mixPresData.updateOrAdd(mp);

  activeMix.updateActiveMixId(mpId);
  activeMixPresData.update(activeMix);

  proc.prepareToPlay(kSampleRate, kSamplesPerBlock);

  juce::AudioBuffer<float> buffer = unityBuffer();
  proc.processBlock(buffer, emptyMidi);

  // With 28 Audio Elements, 1 Mix Presentation, and an Active Mix Presentation
  // we expect the output buffer to have data on the channels that comprise
  // the playback layout.
  for (int chIdx = 0; chIdx < layout.getNumChannels(); ++chIdx) {
    for (int j = 0; j < buffer.getNumSamples(); ++j) {
      ASSERT_NE(buffer.getSample(chIdx, j), 0.f);
    }
  }
}

// Verify that the correct binaural renderer is initialized
TEST_F(test_render_proc, binaural_renderers) {
  Speakers::AudioElementSpeakerLayout layout = Speakers::kBinaural;
  room.setSpeakerLayout(RoomLayout(layout, layout.toString().toStdString()));
  roomSetupData.update(room);

  const juce::Uuid mpId;
  MixPresentation mp(mpId, "All Binaural AEs", 1.f,
                     LanguageData::MixLanguages::English, {});

  const int kNumAudioElements = 3;

  // ensure all audio elements are binaural
  for (int i = 0; i < kNumAudioElements; ++i) {
    int firstChannel = i * 2;
    AudioElement ae(juce::Uuid(), "Stereo " + std::to_string(i),
                    Speakers::kStereo, firstChannel);
    audioElementData.add(ae);
    // ensure binaural
    mp.addAudioElement(ae.getId(), 1.f, ae.getName(), true);
  }
  mixPresData.updateOrAdd(mp);

  activeMix.updateActiveMixId(mpId);
  activeMixPresData.update(activeMix);

  proc.prepareToPlay(kSampleRate, kSamplesPerBlock);

  std::vector<AudioElementRenderer*> renderers =
      proc.getAudioElementRenderers();
  std::vector<MixPresentationAudioElement> mpAE =
      mixPresData.get(mpId)->getAudioElements();

  for (int i = 0; i < kNumAudioElements; ++i) {
    ASSERT_EQ(renderers[i]->kIsBinaural, mpAE[i].isBinaural());
  }

  const juce::Uuid mpId2;
  MixPresentation mp2(mpId2, "Non-Binaural AEs", 1.f,
                      LanguageData::MixLanguages::English, {});
  // now set all MixPresentationAudioElements to not binaural
  for (int i = 0; i < kNumAudioElements; ++i) {
    const juce::Uuid aeID = mpAE[i].getId();
    mp2.addAudioElement(aeID, 1.f, mpAE[i].getName(), false);
  }
  mixPresData.updateOrAdd(mp2);

  activeMix.updateActiveMixId(mpId2);
  activeMixPresData.update(activeMix);

  proc.prepareToPlay(kSampleRate, kSamplesPerBlock);

  renderers = proc.getAudioElementRenderers();
  std::vector<MixPresentationAudioElement> mp2AE =
      mixPresData.get(mpId2)->getAudioElements();

  for (int i = 0; i < kNumAudioElements; ++i) {
    ASSERT_EQ(renderers[i]->kIsBinaural, mp2AE[i].isBinaural());
  }
}