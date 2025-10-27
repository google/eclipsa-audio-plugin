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

#include "processors/mix_monitoring/loudness_standards/MeasureEBU128.h"

#include <gtest/gtest.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include <filesystem>
#include <memory>

#include "processors/file_output/FileWriter.h"
#include "processors/tests/FileOutputTestUtils.h"
#include "substream_rdr/surround_panner/MonoToSpeakerPanner.h"

// Enable to generate panned files to get the loudness information with FFMPEG
// Not running this every test to avoid having to regenerate the files everytime
// This test works by manually generating the output files locally and setting
// the values for the tests by enabling OUTPUT_FILES and then translating the
// FFMPEF output as follows:
//	"input_i" : "-8.74",  --> Integrated Loudness
//	"input_tp" : "-1.25", --> True Peak
//	"input_lra" : "2.30", --> Loudness Range
#define OUTPUT_FILES false

struct LoudnessTestInfo {
  float loudnessMomentary, loudnessShortTerm, loudnessIntegrated;
  float loudnessRange, loudnessTruePeak, loudnessDigitalPeak;
  Speakers::AudioElementSpeakerLayout speakerLayout;
};

MeasureEBU128::LoudnessStats measureLoudness(
    const juce::File& inputFile,
    Speakers::AudioElementSpeakerLayout speakerLayout) {
  // Initialize JUCE audio format manager and formats
  juce::AudioFormatManager formatManager;
  formatManager.registerBasicFormats();

  // Create an input file reader
  std::unique_ptr<juce::AudioFormatReader> reader(
      formatManager.createReaderFor(inputFile));

  if (reader == nullptr) {
    DBG("Failed to create reader for the file: "
        << inputFile.getFullPathName());
    return MeasureEBU128::LoudnessStats();
  }

  // Get the number of channels and length of the audio
  const int numChannels = reader->numChannels;
  const int numSamples = static_cast<int>(reader->lengthInSamples);

  // Decide on the size of the buffers to pass through the system
  const int chunkSize = reader->sampleRate;

  // Prepare a buffer to hold the audio data
  juce::AudioSampleBuffer inputBuffer(numChannels, numSamples);

  // Read the audio data from the input WAV file into the buffer
  reader->read(&inputBuffer, 0, numSamples, 0, true, true);

  // Create a loudness measure object
  MeasureEBU128 loudness(reader->sampleRate);
  MeasureEBU128::LoudnessStats stats;

  // Create a buffer to write the panned audio to
  juce::AudioSampleBuffer outputBuffer(speakerLayout.getNumChannels(),
                                       chunkSize);

  FileWriter* writer = NULL;
  if (OUTPUT_FILES) {
    writer = new FileWriter(
        juce::File("./common/processors/tests/test_resources/loudness_drum_" +
                   speakerLayout.toString() + ".wav")
            .getFullPathName(),
        reader->sampleRate, outputBuffer.getNumChannels(), 0, 16,
        AudioCodec::LPCM);
  }

  // Create an audio processor block for panning
  MonoToSpeakerPanner panner(speakerLayout, chunkSize, reader->sampleRate);

  for (int i = 0; i < (numSamples - chunkSize); i += chunkSize) {
    juce::AudioBuffer<float> chunk = juce::AudioBuffer<float>(
        inputBuffer.getArrayOfWritePointers(), numChannels, i, chunkSize);

    // Pan the audio and compute it's loudness
    panner.process(chunk, outputBuffer);
    stats =
        loudness.measureLoudness(speakerLayout.getChannelSet(), outputBuffer);

    if (writer != NULL) {
      writer->write(outputBuffer);
    }
  }

  if (writer != NULL) {
    std::cout << "Writing measurement for " << speakerLayout.toString()
              << std::endl;
    std::cout << "Momentary: " << stats.loudnessMomentary << std::endl;
    std::cout << "Short Term: " << stats.loudnessShortTerm << std::endl;
    std::cout << "Integrated: " << stats.loudnessIntegrated << std::endl;
    std::cout << "Range: " << stats.loudnessRange << std::endl;
    std::cout << "True Peak: " << stats.loudnessTruePeak << std::endl;
    std::cout << "Digital Peak: " << stats.loudnessDigitalPeak << std::endl;
    std::cout << "========================" << std::endl;

    writer->close();
    delete writer;

    // Call FFMPEG to get the loudness information
    juce::String command =
        "ffmpeg -i " +
        juce::File("./common/processors/tests/test_resources/loudness_drum_" +
                   speakerLayout.toString() + ".wav")
            .getFullPathName() +
        " -af loudnorm=print_format=json -f null -";
    juce::String result =
        juce::File("./common/processors/tests/test_resources/loudness_drum_" +
                   speakerLayout.toString() + ".txt")
            .getFullPathName();
    command += " 2> " + result;
    system(command.toRawUTF8());
  }

  return stats;
}

TEST(test_ebu128_measurements, loudness_test) {
  const std::vector<LoudnessTestInfo> kLoudnessTestInfo = {
      {0, 0, -8.43, 0.60, -1.22, 0, Speakers::kStereo},
      {0, 0, -8.43, 0.60, 1.79, 0, Speakers::k5Point1},
      {0, 0, -8.43, 0.60, 1.79, 0, Speakers::k5Point1Point2},
      {0, 0, -8.43, 0.60, 1.79, 0, Speakers::k5Point1Point4},
      {0, 0, -8.43, 0.60, 1.79, 0, Speakers::k7Point1},
      {0, 0, -10.07, 0.50, 0.15, 0, Speakers::k3Point1Point2},
      {0, 0, -8.43, 0.60, 1.79, 0, Speakers::kExpl9Point1Point6}};

  const std::filesystem::path kWavSrcPath =
      std::filesystem::current_path().parent_path() /
      "common/processors/tests/test_resources" / "loudness_test_drums.wav";
  juce::File inputWavFile(kWavSrcPath.string());

  for (auto test : kLoudnessTestInfo) {
    MeasureEBU128::LoudnessStats loudness =
        measureLoudness(inputWavFile, test.speakerLayout);
    // Check the measured loudness against pre-recorded values
    std::cout << loudness.loudnessIntegrated << " " << loudness.loudnessTruePeak
              << " " << loudness.loudnessRange << std::endl;
    ASSERT_NEAR(loudness.loudnessIntegrated, test.loudnessIntegrated, 0.1);
    ASSERT_NEAR(loudness.loudnessTruePeak, test.loudnessTruePeak, 0.1)
        << "Failed for layout: " << test.speakerLayout.toString();

    // Note: It's unclear to me why the error here is still so large in
    // comparison to FFMPEG. The calculation looks
    // correct, perhaps FFMPEG calculates the gating differently because it has
    // access to the entire file?
    ASSERT_NEAR(loudness.loudnessRange, test.loudnessRange, 1);
  }
}

TEST(test_ebu128_measurements, true_peak_vary_sr) {
  for (const auto sr : {16e3, 24e3, 44.1e3, 48e3, 96e3}) {
    const std::string kFname = "tp_" + std::to_string((int)sr) + ".wav";
    const std::filesystem::path kRefWavPath =
        std::filesystem::current_path() / kFname;
    auto sineWave = generateSineWave(440, sr, sr);
    sineWave.applyGain(0.5f);
    std::unique_ptr<WavFileWriter> writer =
        std::make_unique<WavFileWriter>(kRefWavPath, 1, sr);
    writer->write(sineWave, sineWave.getNumSamples());
    writer.reset();

    // Measure true peak level. We expect these to be the same regardless of
    // sample rate. Comparing against offline FFmpeg computed EBU values.
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(juce::File(kRefWavPath.string())));
    ASSERT_NE(reader, nullptr)
        << "Failed to create reader for the file: " << kRefWavPath.string();

    juce::AudioBuffer<float> buffer(sineWave.getNumChannels(),
                                    sineWave.getNumSamples());
    reader->read(&buffer, 0, buffer.getNumSamples(), 0, true, true);

    MeasureEBU128 loudness(sr);
    auto stats =
        loudness.measureLoudness(juce::AudioChannelSet::mono(), buffer);
    std::cout << "Sample Rate: " << sr
              << " True Peak: " << stats.loudnessTruePeak << std::endl;
    ASSERT_NEAR(stats.loudnessTruePeak, -26, 0.1f);
  }
}