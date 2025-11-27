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

#include "substream_rdr/hoa2bed_rdr/HOAToBedRdr.h"

#include <gtest/gtest.h>

#include "TestHelper.h"
#include "ear/ear.hpp"
#include "substream_rdr/rdr_factory/RendererFactory.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

// Macro to enable writing rendered output to a file for debugging purposes.
#undef RDR_TO_FILE

using Speakers::AudioElementSpeakerLayout;

const int kNumSamps = 1;

const std::vector<AudioElementSpeakerLayout> kInputLayouts = {
    Speakers::kHOA1,
    // IAMFSpkrLayout::kHOA2,
    //  IAMFSpkrLayout::kHOA3,
    // IAMFSpkrLayout::kHOA4, IAMFSpkrLayout::kHOA5,
    // IAMFSpkrLayout::kHOA6,
    // IAMFSpkrLayout::kHOA7
};

const std::vector<AudioElementSpeakerLayout> kPlaybackLayouts = {
    Speakers::kMono,          Speakers::kStereo,
    Speakers::k5Point1,       Speakers::k5Point1Point2,
    Speakers::k7Point1,       Speakers::k7Point1Point4,
    Speakers::k3Point1Point2, Speakers::k7Point1Point2,
};

const std::vector<AudioElementSpeakerLayout> kExpandedLayouts = {
    Speakers::kExplLFE,
    Speakers::kExpl5Point1Point4Surround,
    Speakers::kExpl7Point1Point4SideSurround,
    Speakers::kExpl7Point1Point4RearSurround,
    Speakers::kExpl7Point1Point4TopFront,
    Speakers::kExpl7Point1Point4TopBack,
    Speakers::kExpl7Point1Point4Top,
    Speakers::kExpl7Point1Point4Front,
    Speakers::kExpl9Point1Point6,
    Speakers::kExpl9Point1Point6Front,
    Speakers::kExpl9Point1Point6Side,
    Speakers::kExpl9Point1Point6TopSide,
    Speakers::kExpl9Point1Point6Top,
};

// ITU playback layouts are renderable (can construct rdr)
TEST(test_h2b_rdr, construct_rdr_itu_playback) {
  Speakers::FBuffer dummyBuff;
  for (const AudioElementSpeakerLayout l : kInputLayouts) {
    for (const AudioElementSpeakerLayout p : kPlaybackLayouts) {
      auto rdr = createRenderer(l, p);
      ASSERT_TRUE(rdr != nullptr)
          << "Unable to create rdr for layout " << (int)l << " to " << (int)p;
    }
  }
}

// Renderer construction for non-ITU playback layouts
TEST(test_h2b_rdr, construct_rdr_non_itu_playback) {
  AudioElementSpeakerLayout in = Speakers::kHOA1, playback;
  std::unique_ptr<Renderer> rdr;

  // Requires downmixing.
  playback = Speakers::k3Point1Point2;
  rdr = createRenderer(in, playback);
  EXPECT_TRUE(rdr != nullptr);

  // Requires downmixing.
  playback = Speakers::k7Point1Point2;
  rdr = createRenderer(in, playback);
  EXPECT_TRUE(rdr != nullptr);
}

// Loop through I/O layouts, performing rendering.
TEST(test_h2b_rdr, rdr) {
  Speakers::FBuffer srcBuff, outBuff;
  for (const auto srcLayout : kInputLayouts) {
    srcBuff = Speakers::FBuffer(srcLayout.getNumChannels(), kNumSamps);
    populateInput(srcBuff);
    for (const auto playbackLayout : kPlaybackLayouts) {
      outBuff = Speakers::FBuffer(playbackLayout.getNumChannels(), kNumSamps);
      auto rdr = createRenderer(srcLayout, playbackLayout);
      rdr->render(srcBuff, outBuff);

      auto debug = examineRdrOutput(outBuff);
      ASSERT_TRUE(debug.size() == outBuff.getNumChannels());
    }
  }
}

/**
 * @brief Validate that a HOA to Extended Layout renderer can be constructed for
 * each layout.
 *
 */
TEST(test_h2b_rdr_extl, construct_rdr_ext) {
  for (const auto extLayout : kExpandedLayouts) {
    auto rdr = createRenderer(Speakers::kHOA1, extLayout);
    ASSERT_TRUE(rdr != nullptr)
        << "Unable to create rdr for layout " << (int)extLayout;
  }
}

/**
 * @brief Attempt rendering from HOA to each Extended Layout. Validate that the
 * output buffer has data only on the expected channels.
 *
 */
TEST(test_h2b_rdr_extl, rdr_ext) {
  Speakers::FBuffer srcBuff =
      Speakers::FBuffer(Speakers::kHOA1.getNumChannels(), kNumSamps);
  Speakers::FBuffer outBuff;
  for (const auto extLayout : kExpandedLayouts) {
    populateInput(srcBuff);
    outBuff = Speakers::FBuffer(extLayout.getNumChannels(), kNumSamps);
    outBuff.clear();

    auto rdr = createRenderer(Speakers::kHOA1, extLayout);
    rdr->render(srcBuff, outBuff);

    // Expect each channel of the expanded layout output buffer to have data
    // (except LFE).
    if (extLayout != Speakers::kExplLFE) {
      for (int i = 0; i < outBuff.getNumChannels(); ++i) {
        // We don't expect the LFE channel of 9.1.6 to have data.
        if (extLayout == Speakers::kExpl9Point1Point6 && i == 3) {
          EXPECT_EQ(outBuff.getSample(i, 0), 0.f)
              << "Layout " << extLayout.toString() << " channel " << i;
        }
        // All other expanded layout channels should have data.
        else {
          EXPECT_NE(outBuff.getSample(i, 0), 0.f)
              << "Layout " << extLayout.toString() << " channel " << i;
        }
      }
    }
  }
}

#ifdef RDR_TO_FILE
static void dumpBuffer(const juce::AudioBuffer<float>& buff) {
  for (int i = 0; i < buff.getNumChannels(); ++i) {
    for (int j = 0; j < buff.getNumSamples(); ++j) {
      std::cout << buff.getSample(i, j) << " ";
    }
    std::cout << "\n";
  }
  std::cout << "\n";
}

static std::unique_ptr<juce::AudioFormatWriter> prepareWriter(
    const int sampleRate,
    const Speakers::AudioElementSpeakerLayout outputLayout) {
  juce::File outputFile = juce::File::getCurrentWorkingDirectory().getChildFile(
      "hoa_to_" + outputLayout.toString() + ".wav");
  juce::WavAudioFormat wavFormat;
  std::unique_ptr<juce::FileOutputStream> outputStream(
      outputFile.createOutputStream());
  std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
      outputStream.get(), sampleRate, outputLayout.getNumChannels(),
      16,   // Bits per sample
      {},   // Metadata
      0));  // Default compression
  outputStream.release();
  return writer;
}

TEST(test_h2b_rdr, rdr_all_from_file) {
  const int kNumSamples = 256;
  long fileSamplesProcessed;
  Speakers::AudioElementSpeakerLayout inputLayout = Speakers::kHOA3;

  // Open the input file and create a file input stream
  std::filesystem::path kHOAAudioSource =
      (std::filesystem::current_path() / "samples/Transport_TOA_5s.wav");

  // As the test is run in the /build directory, the path needs to be adjusted
  // to point to the true location of the audio source file.
  std::filesystem::path correctedAudioSrcPath;
  for (const auto& part : kHOAAudioSource) {
    if (part == "build") {
      continue;  // Skip the 'build' segment
    }
    correctedAudioSrcPath /= part;
  }

  juce::File inputFile(correctedAudioSrcPath.string());
  if (!inputFile.existsAsFile()) {
    std::cerr << "Input file does not exist: " << inputFile.getFullPathName()
              << std::endl;
    return;
  }
  // Initialize the AudioFormatManager and create an AudioFormatReader
  juce::AudioFormatManager formatManager;
  formatManager.registerBasicFormats();
  std::unique_ptr<juce::AudioFormatReader> reader(
      formatManager.createReaderFor(inputFile));
  if (reader == nullptr) {
    std::cerr << "Failed to create reader for input file: "
              << inputFile.getFullPathName() << std::endl;
    return;
  }
  // Read samples to an internal buffer.
  juce::AudioSampleBuffer inputBuffer(reader->numChannels,
                                      reader->lengthInSamples);
  reader->read(&inputBuffer, 0, inputBuffer.getNumSamples(), 0, true, true);
  // Validate input file.
  std::cout << "Channels in input file: " << reader->numChannels << "\n";
  std::cout << "Input file format: "
            << reader->getChannelLayout().getDescription() << "\n";

  // Iterate over renderable layouts and render to output files.
  for (const auto outputLayout : kPlaybackLayouts) {
    // Prepare the output file.
    auto writer = prepareWriter(48000, outputLayout);

    // Read frames of samples from the file and render.
    auto rdr = createRenderer(inputLayout, outputLayout);
    Speakers::FBuffer srcBuff(inputLayout.getNumChannels(), kNumSamples);
    Speakers::FBuffer outBuff(outputLayout.getNumChannels(), kNumSamples);

    fileSamplesProcessed = 0;
    while (fileSamplesProcessed < inputBuffer.getNumSamples()) {
      srcBuff.clear();
      outBuff.clear();

      // Read a frame of samples from the input file
      for (int i = 0; i < inputBuffer.getNumChannels(); ++i) {
        for (int j = 0; j < kNumSamples &&
                        fileSamplesProcessed + j < inputBuffer.getNumSamples();
             ++j) {
          srcBuff.setSample(i, j,
                            inputBuffer.getSample(i, fileSamplesProcessed + j));
        }
      }
      fileSamplesProcessed += kNumSamples;

      // Perform rendering
      rdr->render(srcBuff, outBuff);

      // Write rendered data to an output file.
      writer->writeFromAudioSampleBuffer(outBuff, 0, outBuff.getNumSamples());
    }
    writer->flush();
  }

  // Render expanded layouts to file.
  for (const auto outputLayout : kExpandedLayouts) {
    // Prepare the output file.
    auto writer = prepareWriter(48000, outputLayout);

    // Read frames of samples from the file and render.
    auto rdr = createRenderer(inputLayout, outputLayout);
    Speakers::FBuffer srcBuff(inputLayout.getNumChannels(), kNumSamples);
    Speakers::FBuffer outBuff(outputLayout.getNumChannels(), kNumSamples);

    fileSamplesProcessed = 0;
    while (fileSamplesProcessed < inputBuffer.getNumSamples()) {
      srcBuff.clear();
      outBuff.clear();

      // Read a frame of samples from the input file
      for (int i = 0; i < inputBuffer.getNumChannels(); ++i) {
        for (int j = 0; j < kNumSamples &&
                        fileSamplesProcessed + j < inputBuffer.getNumSamples();
             ++j) {
          srcBuff.setSample(i, j,
                            inputBuffer.getSample(i, fileSamplesProcessed + j));
        }
      }
      fileSamplesProcessed += kNumSamples;

      // Perform rendering
      rdr->render(srcBuff, outBuff);

      // Write rendered data to an output file.
      writer->writeFromAudioSampleBuffer(outBuff, 0, kNumSamples);
    }
    writer->flush();
  }
}
#endif  // RDR_TO_FILE