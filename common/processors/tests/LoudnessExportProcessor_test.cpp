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

#include "processors/loudness_export/LoudnessExportProcessor.h"

#include <gtest/gtest.h>
#include <juce_video/juce_video.h>

#include <filesystem>
#include <vector>

#include "data_repository/implementation/AudioElementRepository.h"
#include "data_repository/implementation/FileExportRepository.h"
#include "data_repository/implementation/MixPresentationLoudnessRepository.h"
#include "data_repository/implementation/MixPresentationRepository.h"
#include "data_structures/src/AudioElement.h"
#include "data_structures/src/FileExport.h"
#include "data_structures/src/LoudnessExportData.h"
#include "data_structures/src/MixPresentation.h"
#include "data_structures/src/MixPresentationLoudness.h"
#include "processors/mix_monitoring/loudness_standards/MeasureEBU128.h"
#include "processors/render/RenderProcessor.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"
#include "substream_rdr/surround_panner/MonoToSpeakerPanner.h"

void configureMixPresentations(
    const std::vector<juce::Uuid>& mixIds,
    const std::vector<juce::String>& mixNames,
    const std::vector<float>& mixGains,
    const std::vector<std::vector<AudioElement>>& audioElements,
    MixPresentationRepository& mixPresRepo) {
  jassert(mixIds.size() == mixNames.size() &&
          mixIds.size() == mixGains.size() &&
          audioElements.size() == mixIds.size());
  for (int i = 0; i < mixIds.size(); i++) {
    MixPresentation mixPresentation(mixIds[i], mixNames[i], mixGains[i]);
    for (int j = 0; j < audioElements[i].size(); j++) {
      mixPresentation.addAudioElement(audioElements[i][j].getId(), 1.f,
                                      audioElements[i][j].getName());
    }
    mixPresRepo.updateOrAdd(mixPresentation);
  }
  return;
}

void configureMixPresentationLoudness(
    MixPresentationLoudness& mixLoudness,
    const Speakers::AudioElementSpeakerLayout& layout) {
  // assign obscure initial values to the mix loudness
  const float obscureInitialValue = -500.f;
  mixLoudness.replaceLargestLayout(layout);
  mixLoudness.setLayoutIntegratedLoudness(layout, obscureInitialValue);
  mixLoudness.setLayoutDigitalPeak(layout, obscureInitialValue);
  mixLoudness.setLayoutTruePeak(layout, obscureInitialValue);

  mixLoudness.setLayoutIntegratedLoudness(Speakers::kStereo,
                                          obscureInitialValue);
  mixLoudness.setLayoutDigitalPeak(Speakers::kStereo, obscureInitialValue);
  mixLoudness.setLayoutTruePeak(Speakers::kStereo, obscureInitialValue);
}

juce::AudioBuffer<float> createSinWaveAudio(const int kSamplesPerFrame,
                                            const int kSampleRate) {
  // Generate a 440Hz tone
  juce::AudioBuffer<float> sineWaveAudio(1, kSamplesPerFrame);
  for (int i = 0; i < kSamplesPerFrame; ++i) {
    sineWaveAudio.setSample(
        0, i, 0.1f * std::sin(2 * M_PI * 440 * (float)i / kSampleRate));
  }
  return sineWaveAudio;
}
// this tests ensures that the loudness values are copied to the
// repository when the processor is toggled between non-realtime and realtime
TEST(test_loudness_proc, copyExportContainerDataToRepo) {
  juce::ValueTree testState("test_state");

  FileExportRepository fileExportRepository(
      testState.getOrCreateChildWithName("file", nullptr));
  MixPresentationLoudnessRepository mixPresentationLoudnessRepository(
      testState.getOrCreateChildWithName("mixLoudness", nullptr));

  MixPresentationRepository mixPresentationRepository(
      testState.getOrCreateChildWithName("mixPres", nullptr));

  AudioElementRepository audioElementRepository(
      testState.getOrCreateChildWithName("audioElement", nullptr));

  // Data used by all test fixtures:
  // Constants
  const int kSampleRate = 48e3;
  const int kSamplesPerFrame = 128;
  // Set the duration of the input video file.
  const float kAudioDuration_s = 0.2;
  const int kTotalSamples = kAudioDuration_s * kSampleRate;

  // Update the file export config
  // loudness_proc only cares about AudioFileFormat and setExportAudio(true)
  FileExport ex = fileExportRepository.get();
  ex.setExportAudio(true);
  ex.setAudioFileFormat(AudioFileFormat::IAMF);
  ex.setSampleRate(kSampleRate);
  fileExportRepository.update(ex);

  // Specify the audio element layouts.
  // Largest layout will be 5.1
  const Speakers::AudioElementSpeakerLayout kAudioElementLayout1 =
      Speakers::kStereo;
  const Speakers::AudioElementSpeakerLayout kAudioElementLayout2 =
      Speakers::k5Point1;

  const int numChannels = kAudioElementLayout1.getNumChannels() +
                          kAudioElementLayout2.getNumChannels();

  // Create a mix presentation with two audio elements.
  const std::vector<juce::Uuid> mixIds{juce::Uuid()};
  const std::vector<juce::String> mixNames{"Mix 1"};
  // create 3 audio elements
  const AudioElement audioElement1(juce::Uuid(), "AE 1", Speakers::kStereo, 0);
  const AudioElement audioElement2(
      juce::Uuid(), "AE 2", Speakers::k5Point1,
      audioElement1.getChannelCount() + audioElement1.getFirstChannel());

  audioElementRepository.updateOrAdd(audioElement1);
  audioElementRepository.updateOrAdd(audioElement2);

  // create a vector of audio elements, that will be assigned to the mix
  // presentation
  const std::vector<std::vector<AudioElement>> audioElements = {
      {audioElement1, audioElement2}};

  const std::vector<float> mixGains = {1.f};

  configureMixPresentations(mixIds, mixNames, mixGains, audioElements,
                            mixPresentationRepository);

  MixPresentationLoudness mixLoudness(mixIds[0]);
  // ensure the largest layout is 5.1
  configureMixPresentationLoudness(mixLoudness, kAudioElementLayout2);

  mixPresentationLoudnessRepository.updateOrAdd(
      mixLoudness);  // update the repository

  // Create instance of processor
  LoudnessExportProcessor loudness_proc(
      fileExportRepository, mixPresentationRepository,
      mixPresentationLoudnessRepository, audioElementRepository);

  // Generate a 440Hz tones.
  // the sine wave and will be assigned to a different
  // audio element
  juce::AudioBuffer<float> sineWaveAudio =
      createSinWaveAudio(kSamplesPerFrame, kSampleRate);

  // Start cacluclaing loudness values
  loudness_proc.prepareToPlay(kSampleRate, kSamplesPerFrame);
  // ensure there is 1 loudness implementation, for the non-stero layout
  loudness_proc.setNonRealtime(true);

  // Copy the the sine wave audio to each buffer channel and process the
  // frame.
  juce::AudioBuffer<float> audioBuffer(numChannels, kSamplesPerFrame);
  juce::MidiBuffer midiBuffer;
  for (int sampsProcd = 0; sampsProcd < kTotalSamples;
       sampsProcd += kSamplesPerFrame) {
    // Copy audio data to each channel of audioBuffer.
    for (int i = 0; i < numChannels; ++i) {
      audioBuffer.copyFrom(i, 0, sineWaveAudio, 0, 0, kSamplesPerFrame);
    }
    loudness_proc.processBlock(audioBuffer, midiBuffer);
  }

  // should copy the loudness values to the repository
  loudness_proc.setNonRealtime(false);

  // compare values from the RT Data Struct to the repository
  using EBU128Stats = MeasureEBU128::LoudnessStats;
  EBU128Stats stereoLoudnessStats;
  EBU128Stats layoutLoudnessStats;
  const MixPresentationLoudnessExportContainer* exportContainer =
      loudness_proc.getExportContainers()[0];
  exportContainer->loudnessExportData->stereoEBU128.read(stereoLoudnessStats);
  exportContainer->loudnessExportData->layoutEBU128.read(layoutLoudnessStats);
  MixPresentationLoudness results =
      mixPresentationLoudnessRepository.get(mixIds[0]).value();
  EXPECT_EQ(results.getLargestLayout(), Speakers::k5Point1);

  EXPECT_EQ(results.getLayoutIntegratedLoudness(Speakers::kStereo),
            stereoLoudnessStats.loudnessIntegrated);
  EXPECT_EQ(results.getLayoutIntegratedLoudness(Speakers::k5Point1),
            layoutLoudnessStats.loudnessIntegrated);

  EXPECT_EQ(results.getLayoutDigitalPeak(Speakers::kStereo),
            stereoLoudnessStats.loudnessDigitalPeak);
  EXPECT_EQ(results.getLayoutDigitalPeak(Speakers::k5Point1),
            layoutLoudnessStats.loudnessDigitalPeak);

  EXPECT_EQ(results.getLayoutTruePeak(Speakers::kStereo),
            stereoLoudnessStats.loudnessTruePeak);
  EXPECT_EQ(results.getLayoutTruePeak(Speakers::k5Point1),
            layoutLoudnessStats.loudnessTruePeak);
}

// Validate that the MixPresentationExportContainer is creating
// the correct number of renderers, mixBuffers and loudness intstruments
// for each mix presentation
TEST(test_loudness_proc, test_export_container_struct) {
  juce::ValueTree testState("test_state");

  FileExportRepository fileExportRepository(
      testState.getOrCreateChildWithName("file", nullptr));
  MixPresentationLoudnessRepository mixPresentationLoudnessRepository(
      testState.getOrCreateChildWithName("mixLoudness", nullptr));

  MixPresentationRepository mixPresentationRepository(
      testState.getOrCreateChildWithName("mixPres", nullptr));

  AudioElementRepository audioElementRepository(
      testState.getOrCreateChildWithName("audioElement", nullptr));

  // Data used by all test fixtures:
  // Constants
  const int kSampleRate = 48e3;
  const int kSamplesPerFrame = 128;

  // Update the file export config
  // loudness_proc only cares about AudioFileFormat and setExportAudio(true)
  FileExport ex = fileExportRepository.get();
  ex.setProfile(BASE_ENHANCED);
  ex.setExportAudio(true);
  ex.setAudioFileFormat(AudioFileFormat::IAMF);
  ex.setSampleRate(kSampleRate);
  fileExportRepository.update(ex);

  // declare the audio element layouts
  const Speakers::AudioElementSpeakerLayout kAudioElementLayout1 =
      Speakers::kStereo;
  const Speakers::AudioElementSpeakerLayout kAudioElementLayout2 =
      Speakers::k5Point1;
  const Speakers::AudioElementSpeakerLayout kAudioElementLayout3 =
      Speakers::k7Point1;

  // create 3 mix presentations
  const std::vector<juce::Uuid> mixIds{juce::Uuid(), juce::Uuid(),
                                       juce::Uuid()};
  const std::vector<juce::String> mixNames{"Mix 1", "Mix 2", "Mix 3"};
  // create 3 audio elements
  const AudioElement audioElement1(juce::Uuid(), "AE 1", Speakers::kStereo, 0);
  const AudioElement audioElement2(
      juce::Uuid(), "AE 2", Speakers::k5Point1,
      audioElement1.getChannelCount() + audioElement1.getFirstChannel());
  const AudioElement audioElement3(
      juce::Uuid(), "AE 3", Speakers::k7Point1,
      audioElement2.getChannelCount() + audioElement2.getFirstChannel());

  audioElementRepository.updateOrAdd(audioElement1);
  audioElementRepository.updateOrAdd(audioElement2);
  audioElementRepository.updateOrAdd(audioElement3);

  // create a vector of audio elements, that will be assigned to mix
  // presentations 1, 2 and 3
  const std::vector<std::vector<AudioElement>> audioElements = {
      {audioElement1},
      {audioElement1, audioElement2},
      {audioElement2, audioElement3}};

  const std::vector<float> mixGains = {1.f, 2.f, 0.5f};

  configureMixPresentations(mixIds, mixNames, mixGains, audioElements,
                            mixPresentationRepository);

  std::vector<Speakers::AudioElementSpeakerLayout> layouts = {
      kAudioElementLayout1, kAudioElementLayout2, kAudioElementLayout3};
  // Create a mix presentation loudness for each mix presentation
  // the mix presentations have been configured such that the largest layout in
  // each one is stereo, 5.1 and 7.1 respectively
  for (int i = 0; i < mixIds.size(); i++) {
    MixPresentationLoudness mixLoudness(mixIds[i]);
    configureMixPresentationLoudness(mixLoudness, layouts[i]);
    mixPresentationLoudnessRepository.updateOrAdd(mixLoudness);
  }

  // Create instance of processor
  LoudnessExportProcessor loudness_proc(
      fileExportRepository, mixPresentationRepository,
      mixPresentationLoudnessRepository, audioElementRepository);

  // Start cacluclaing loudness values
  // ensure renderer's are initialized
  loudness_proc.prepareToPlay(kSampleRate, kSamplesPerFrame);
  loudness_proc.setNonRealtime(true);

  loudness_proc.setNonRealtime(false);

  auto exportcontainers =
      loudness_proc.getExportContainers();  // get the export containers

  // ensure there is a an export container for each mix presentation
  juce::OwnedArray<MixPresentation> mixPresentations;
  mixPresentationRepository.getAll(mixPresentations);
  EXPECT_EQ(exportcontainers.size(), mixPresentations.size());

  // confirm there is a stereo loudness implementation
  // for all mix presentations
  for (const auto& exportContainer : exportcontainers) {
    EXPECT_EQ(exportContainer->loudnessImpls.first->playbackLayout_,
              Speakers::kStereo.getChannelSet());
  }

  // confirm that the first layout loudness implementation is null, but the 2nd
  // and 3rd correspond to 5.1 and 7.1 respectively
  EXPECT_EQ(exportcontainers[0]->loudnessImpls.second, nullptr);
  EXPECT_EQ(exportcontainers[1]->loudnessImpls.second->playbackLayout_,
            Speakers::k5Point1.getChannelSet());
  EXPECT_EQ(exportcontainers[2]->loudnessImpls.second->playbackLayout_,
            Speakers::k7Point1.getChannelSet());

  // confirm that the first mix buffer for each mix presentation conforms to
  // stereo
  //  the second mix buffer should be mono, 5.1 and 7.1 respectively
  std::vector<int> numChannelsVec{Speakers::kMono.getNumChannels(),
                                  Speakers::k5Point1.getNumChannels(),
                                  Speakers::k7Point1.getNumChannels()};
  for (int i = 0; i < exportcontainers.size(); i++) {
    const auto exportContainer = exportcontainers[i];
    EXPECT_EQ(exportContainer->mixPresBuffers.first.getNumChannels(),
              Speakers::kStereo.getNumChannels());
    EXPECT_EQ(exportContainer->mixPresBuffers.second.getNumChannels(),
              numChannelsVec[i]);
  }

  // confirm that the correct number of renderers are made for each mix
  // presentation
  for (int i = 0; i < exportcontainers.size(); i++) {
    const MixPresentationLoudness mixPresLoudness =
        mixPresentationLoudnessRepository.get(mixPresentations[i]->getId())
            .value();
    auto mixPresAudioElements = mixPresentations[i]->getAudioElements();
    auto& mixPresRenderers = exportcontainers[i]->audioElementRenderers;
    // there should be a std::pair<> for each audio element
    EXPECT_EQ(mixPresRenderers.size(), mixPresAudioElements.size());
    for (int j = 0; j < mixPresRenderers.size(); j++) {
      // confirm that the input Layout of each AudioElementRenderer is the same
      //  as the channel config of the corresponding audio element
      AudioElement audioElement =
          audioElementRepository.get(mixPresAudioElements[j].getId()).value();
      EXPECT_EQ(mixPresRenderers[j].first->inputLayout.getChannelSet(),
                audioElement.getChannelConfig().getChannelSet());
      // confirm the outputLayout of the first renderer is always stereo
      EXPECT_EQ(mixPresRenderers[j].first->outputData.getNumChannels(),
                Speakers::kStereo.getNumChannels());
      // if the largest layout is stereo, the second renderer should be null
      if (mixPresLoudness.getLargestLayout() == Speakers::kStereo) {
        EXPECT_EQ(mixPresRenderers[j].second, nullptr);
      } else {
        // confirm the outputLayout of the second renderer is the largest
        // layout
        EXPECT_EQ(mixPresRenderers[j].second->outputData.getNumChannels(),
                  mixPresLoudness.getLargestLayout().getNumChannels());
      }
    }
  }
}

struct WavFileParameters {
  const int numChannels;
  const long long totalSamples;
  const double sampleRate;
  std::unique_ptr<juce::AudioFormatReader> reader;
  WavFileParameters(int numChannels, long long totalSamples, double sample_rate,
                    std::unique_ptr<juce::AudioFormatReader> reader)
      : reader(std::move(reader)),
        numChannels(numChannels),
        totalSamples(totalSamples),
        sampleRate(sample_rate) {}
};

WavFileParameters getWavFileParameters(const juce::File& wavFile) {
  jassert(wavFile.exists());
  jassert(wavFile.hasFileExtension("wav"));
  // Open the WAV file
  juce::AudioFormatManager formatManager;
  formatManager.registerBasicFormats();

  // Create an input file reader
  std::unique_ptr<juce::AudioFormatReader> reader(
      formatManager.createReaderFor(wavFile));

  jassert(reader != nullptr);

  // Get the number of channels and length of the audio
  const int numChannels = reader->numChannels;
  const long long numSamples = static_cast<int>(reader->lengthInSamples);
  const double sample_rate = reader->sampleRate;
  return {numChannels, numSamples, sample_rate, std::move(reader)};
}

// this functions uses the MEASURE_EBU128 class to get the loudness
// values for a mono track panned to stereo
// because we are panning to stereo, and measuring the loudness of a stereo
// layout, there is no need for a renderer
MeasureEBU128::LoudnessStats getPannedStereoVerificationData(
    const juce::AudioBuffer<float>& inputBuffer, const float mixGain,
    const int& totalSamples, const int& sampleRate,
    const int& samplesPerFrame) {
  // ensure the input buffer is mono
  jassert(inputBuffer.getNumChannels() == 1);

  MeasureEBU128 instrument(sampleRate, Speakers::kStereo.getChannelSet());
  MonoToSpeakerPanner monoToLayoutPanner(Speakers::kStereo, samplesPerFrame,
                                         sampleRate);

  juce::AudioBuffer<float> layoutOutputBuffer(
      Speakers::kStereo.getNumChannels(), samplesPerFrame);

  for (int i = 0; i < totalSamples - samplesPerFrame; i += samplesPerFrame) {
    // copy data from input buffer
    // because chunk is reinitalized each time,
    // we can use .addFrom() to apply the gain
    juce::AudioBuffer<float> chunk(Speakers::kMono.getNumChannels(),
                                   samplesPerFrame);
    // chunk.addFrom(0, 0, inputBuffer, 0, i, samplesPerFrame, mixGain);

    chunk.copyFrom(0, 0, inputBuffer, 0, i, samplesPerFrame);
    chunk.applyGain(mixGain);

    monoToLayoutPanner.process(chunk, layoutOutputBuffer);

    instrument.measureLoudness(Speakers::kStereo.getChannelSet(),
                               layoutOutputBuffer);
  }

  return instrument.loudnessStats_;
}

// Validate that the correct loudness values are being stored in the
// metadata for mix presentation of varying gain
TEST(test_loudness_proc, verify_metadata) {
  juce::ValueTree testState("test_state");

  FileExportRepository fileExportRepository(
      testState.getOrCreateChildWithName("file", nullptr));
  MixPresentationLoudnessRepository mixPresentationLoudnessRepository(
      testState.getOrCreateChildWithName("mixLoudness", nullptr));

  MixPresentationRepository mixPresentationRepository(
      testState.getOrCreateChildWithName("mixPres", nullptr));

  AudioElementRepository audioElementRepository(
      testState.getOrCreateChildWithName("audioElement", nullptr));

  std::filesystem::path wavFilePath;
  // check if 'rendererplugin/test' is in the current path
  // if not, add it
  if (std::filesystem::current_path().string().find(
          "common/processors/tests") != std::string::npos) {
    wavFilePath = std::filesystem::current_path() /
                  "test_resources/loudness_test_drums.wav";
  } else {
    wavFilePath =
        std::filesystem::current_path() /
        "common/processors/tests/test_resources/loudness_test_drums.wav";
  }

  // remove the 'build' segment from the path
  std::filesystem::path correctedwavFilePath;
  for (const auto& part : wavFilePath) {
    if (part == "build") {
      continue;  // Skip the 'build' segment
    }
    correctedwavFilePath /= part;
  }
  wavFilePath = correctedwavFilePath;

  juce::File wavFile(wavFilePath.string());
  WavFileParameters wavFileParams(getWavFileParameters(wavFile));

  // Data used by all test fixtures:
  // Constants
  const int kSampleRate = wavFileParams.sampleRate;
  const int kSamplesPerFrame = wavFileParams.sampleRate;
  // Set the duration of the input video file.
  const int kTotalSamples = wavFileParams.totalSamples;
  const int inputNumChannels = wavFileParams.numChannels;

  // the input audio buffer
  // should be the same as the number of channels in the wav file
  // and the number of samples in the wav file
  juce::AudioBuffer<float> inputBuffer(inputNumChannels, kTotalSamples);

  wavFileParams.reader->read(&inputBuffer, 0, kTotalSamples, 0, true, true);

  // Update the file export config
  // loudness_proc only cares about AudioFileFormat and setExportAudio(true)
  FileExport ex = fileExportRepository.get();
  ex.setProfile(BASE_ENHANCED);
  ex.setExportAudio(true);
  ex.setAudioFileFormat(AudioFileFormat::IAMF);
  ex.setSampleRate(kSampleRate);
  fileExportRepository.update(ex);

  // declare the audio element layouts
  const Speakers::AudioElementSpeakerLayout kAudioElementLayout1 =
      Speakers::k7Point1;
  const Speakers::AudioElementSpeakerLayout kAudioElementLayout2 =
      Speakers::k5Point1;
  const Speakers::AudioElementSpeakerLayout kAudioElementLayout3 =
      Speakers::kStereo;

  // create 3 mix presentations
  const std::vector<juce::Uuid> mixIds{juce::Uuid(), juce::Uuid(),
                                       juce::Uuid()};
  const std::vector<juce::String> mixNames{"Mix 1", "Mix 2", "Mix 3"};
  // create 3 audio elements
  const AudioElement audioElement1(juce::Uuid(), "AE 1", kAudioElementLayout1,
                                   0);
  const AudioElement audioElement2(
      juce::Uuid(), "AE 2", kAudioElementLayout2,
      audioElement1.getChannelCount() + audioElement1.getFirstChannel());
  const AudioElement audioElement3(
      juce::Uuid(), "AE 3", kAudioElementLayout3,
      audioElement2.getChannelCount() + audioElement2.getFirstChannel());

  const int audioElementNumChannels = audioElement1.getChannelCount() +
                                      audioElement2.getChannelCount() +
                                      audioElement3.getChannelCount();

  audioElementRepository.updateOrAdd(audioElement1);
  audioElementRepository.updateOrAdd(audioElement2);
  audioElementRepository.updateOrAdd(audioElement3);

  // create a vector of audio elements, that will be assigned to mix
  // presentations 1, 2 and 3
  const std::vector<std::vector<AudioElement>> audioElements = {
      {audioElement3},
      {audioElement2, audioElement3},
      {audioElement1, audioElement2, audioElement3}};

  const std::vector<float> mixGains = {1.f, 2.f, .5f};

  configureMixPresentations(mixIds, mixNames, mixGains, audioElements,
                            mixPresentationRepository);

  // this vector stores the largest layout for each
  // mix presentation
  std::vector<Speakers::AudioElementSpeakerLayout> largestLayouts = {
      kAudioElementLayout3, kAudioElementLayout2, kAudioElementLayout1};
  jassert(largestLayouts.size() == mixIds.size());
  // Create a mix presentation loudness for each mix presentation
  // the mix presentations have been configured such that the largest layout in
  // each one is stereo, 5.1 and 7.1 respectively
  for (int i = 0; i < mixIds.size(); i++) {
    MixPresentationLoudness mixLoudness(mixIds[i]);
    configureMixPresentationLoudness(mixLoudness, largestLayouts[i]);
    mixPresentationLoudnessRepository.updateOrAdd(mixLoudness);
  }

  // Create instance of processor
  LoudnessExportProcessor loudness_proc(
      fileExportRepository, mixPresentationRepository,
      mixPresentationLoudnessRepository, audioElementRepository);
  juce::MidiBuffer midi;

  // specify which audio element will be panned
  const AudioElement audioElementToPan = audioElement3;

  MonoToSpeakerPanner monoToStereoPanner(audioElementToPan.getChannelConfig(),
                                         kSamplesPerFrame, kSampleRate);

  juce::AudioBuffer<float> mixBuffer(audioElementNumChannels, kSamplesPerFrame);
  juce::AudioBuffer<float> stereoBuffer(
      audioElementToPan.getChannelConfig().getNumChannels(), kSamplesPerFrame);

  loudness_proc.prepareToPlay(kSampleRate, kSamplesPerFrame);
  loudness_proc.setNonRealtime(true);

  for (int i = 0; i < kTotalSamples - kSamplesPerFrame; i += kSamplesPerFrame) {
    // copy data from input buffer
    juce::AudioBuffer<float> chunk(Speakers::kMono.getNumChannels(),
                                   kSamplesPerFrame);
    chunk.copyFrom(0, 0, inputBuffer, 0, i, kSamplesPerFrame);

    monoToStereoPanner.process(chunk, stereoBuffer);
    // copy the stereo buffer in to the mix buffer
    // the stereo audio element corresponds to the last two channels of the mix
    // buffer
    mixBuffer.clear();
    for (int i = audioElementToPan.getFirstChannel();
         i < audioElementNumChannels; i++) {
      mixBuffer.copyFrom(i, 0, stereoBuffer,
                         i - audioElementToPan.getFirstChannel(), 0,
                         kSamplesPerFrame);
    }

    loudness_proc.processBlock(mixBuffer, midi);
  }

  loudness_proc.setNonRealtime(false);

  using EBU128Stats = MeasureEBU128::LoudnessStats;
  auto exportContainers = loudness_proc.getExportContainers();
  for (auto exportContainer : exportContainers) {
    auto mixPresLoudness = mixPresentationLoudnessRepository
                               .get(exportContainer->mixPresentationId)
                               .value();
    EBU128Stats stereoLoudnessStats;
    exportContainer->loudnessExportData->stereoEBU128.read(stereoLoudnessStats);

    EBU128Stats stats = getPannedStereoVerificationData(
        inputBuffer,
        mixPresentationRepository.get(exportContainer->mixPresentationId)
            .value()
            .getDefaultMixGain(),
        kTotalSamples, kSampleRate, kSamplesPerFrame);
    EXPECT_EQ(stats.loudnessIntegrated, stereoLoudnessStats.loudnessIntegrated);
    EXPECT_EQ(stats.loudnessTruePeak, stereoLoudnessStats.loudnessTruePeak);
    EXPECT_EQ(stats.loudnessDigitalPeak,
              stereoLoudnessStats.loudnessDigitalPeak);

    if (mixPresLoudness.getLargestLayout() == Speakers::kStereo) {
      continue;
    }
    // the layout loudness stats have shown to be similar to the stereo
    // ensure they agree w/ the stereo stats
    EBU128Stats layoutLoudnessStats;
    exportContainer->loudnessExportData->layoutEBU128.read(layoutLoudnessStats);

    EXPECT_NEAR(stats.loudnessIntegrated,
                layoutLoudnessStats.loudnessIntegrated, 0.1f);
    EXPECT_NEAR(stats.loudnessTruePeak, layoutLoudnessStats.loudnessTruePeak,
                0.1f);
    EXPECT_NEAR(stats.loudnessDigitalPeak,
                layoutLoudnessStats.loudnessDigitalPeak, 0.1f);
  }
}