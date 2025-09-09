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
#include <juce_audio_formats/juce_audio_formats.h>

#include "substream_rdr/substream_rdr_utils/Speakers.h"
#include "substream_rdr/surround_panner/AmbisonicPanner.h"
#include "substream_rdr/surround_panner/BinauralPanner.h"
#include "substream_rdr/surround_panner/MonoToSpeakerPanner.h"

// Macro to enable writing rendered output to a file for debugging purposes.
#undef RDR_TO_FILE

using namespace Speakers;

struct TestLayout {
  Speakers::AudioElementSpeakerLayout layout;
  int rightSpeaker;
  int leftSpeaker;
  int lfe;

  TestLayout(Speakers::AudioElementSpeakerLayout layout, int leftSpeaker,
             int rightSpeaker, int lfe)
      : layout(layout),
        rightSpeaker(rightSpeaker),
        leftSpeaker(leftSpeaker),
        lfe(lfe) {}
};

const std::vector<AudioElementSpeakerLayout> kAmbiOutputLayouts = {
    Speakers::kHOA1,
    Speakers::kHOA2,
    Speakers::kHOA3,
    //  Speakers::kHOA4,Speakers::kHOA5, Speakers::kHOA6, Speakers::kHOA7
};

const std::vector<TestLayout> kBedTestLayouts = {
    TestLayout(Speakers::kStereo, 0, 1, -1),
    TestLayout(Speakers::k5Point1, 0, 1, 3),
    TestLayout(Speakers::k5Point1Point2, 0, 1, 3),
    TestLayout(Speakers::k7Point1, 0, 1, 3),
    TestLayout(Speakers::k7Point1Point4, 0, 1, 3),
    TestLayout(Speakers::k3Point1Point2, 0, 1, 3),
    TestLayout(Speakers::k7Point1Point2, 0, 1, 3),
    TestLayout(Speakers::kExpl5Point1Point4Surround, 0, 1, -1),
    TestLayout(Speakers::kExpl7Point1Point4SideSurround, 0, 1, -1),
    TestLayout(Speakers::kExpl7Point1Point4Top, 0, 1, -1),
    TestLayout(Speakers::kExpl9Point1Point6Top, 4, 5, -1)};

const int kNumSamples = 1024;

// Test panning to ambisonics
TEST(test_surround_panner, pan_to_ambi) {
  // Iterate over ambisonic input layouts.
  for (const auto& outputLayout : kAmbiOutputLayouts) {
    // Construct juce I/O buffers.
    juce::AudioBuffer<float> inputBuffer(1, kNumSamples);
    juce::AudioBuffer<float> outputBuffer(outputLayout.getNumChannels(),
                                          kNumSamples);
    outputBuffer.clear();  // Ensure channels are zeroed

    // Fill the input buffer
    inputBuffer.clear();
    for (int i = 0; i < kNumSamples; ++i) {
      inputBuffer.setSample(0, i, 256.f);
    }

    // Construct a panner for the given layout.
    AmbisonicPanner ambisonicPanner(outputLayout, kNumSamples, 48000);

    // Set source position.
    ambisonicPanner.setPosition(1.f, 2.f, 3.f);

    // Process audio.
    ambisonicPanner.process(inputBuffer, outputBuffer);

    // Validate something sensible was output
    for (int i = 0; i < outputBuffer.getNumChannels(); ++i) {
      for (int j = 0; j < outputBuffer.getNumSamples(); ++j) {
        EXPECT_NE(outputBuffer.getSample(i, j), 0.f);
      }
    }
  }
}

// Test panning bed source audio.
TEST(test_surround_panner, pan_to_bed) {
  // Iterate over ambisonic input layouts.
  for (const auto& outputLayout : kBedTestLayouts) {
    // Construct juce I/O buffers.
    juce::AudioBuffer<float> inputBuffer(1, kNumSamples);
    juce::AudioBuffer<float> outputBuffer(outputLayout.layout.getNumChannels(),
                                          kNumSamples);

    // Fill the input buffer
    inputBuffer.clear();
    outputBuffer.clear();
    for (int i = 0; i < kNumSamples; ++i) {
      inputBuffer.setSample(0, i, 0.3f);
    }

    // Construct a panner for the given layout.
    MonoToSpeakerPanner monoToSpeakerPanner(outputLayout.layout, kNumSamples,
                                            48000);

    // Set source position all the way to the right
    monoToSpeakerPanner.setPosition(0.5, 0.5, 0.5);

    // Process audio.
    monoToSpeakerPanner.process(inputBuffer, outputBuffer);

    // Validate something sensible was output
    // Note that the first 255 samples are always 0
    for (int i = 0; i < outputBuffer.getNumChannels(); ++i) {
      if (i != outputLayout.lfe) {
        for (int j = 255; j < outputBuffer.getNumSamples(); ++j) {
          EXPECT_NE(outputBuffer.getSample(i, j), 0.f);
        }
      }
    }

    MonoToSpeakerPanner monoToSpeakerPanner2(outputLayout.layout, kNumSamples,
                                             48000);

    // Move the source position all the way to the right
    monoToSpeakerPanner2.setPosition(45.0f, 0.0f, 0.0f);

    // Process audio.
    monoToSpeakerPanner2.process(inputBuffer, outputBuffer);

    // Validate audio is output to the right speaker
    // Validate no audio is output to the left speaker
    for (int j = 255; j < outputBuffer.getNumSamples(); ++j) {
      EXPECT_NE(outputBuffer.getSample(outputLayout.rightSpeaker, j), 0.f);
      EXPECT_EQ(outputBuffer.getSample(outputLayout.leftSpeaker, j), 0.f);
    }
  }
}

// Test panning to binaural
TEST(test_surround_panner, pan_to_binaural) {
  // Construct juce I/O buffers.
  juce::AudioBuffer<float> inputBuffer(1, kNumSamples);
  juce::AudioBuffer<float> outputBuffer(Speakers::kBinaural.getNumChannels(),
                                        kNumSamples);
  outputBuffer.clear();  // Ensure channels are zeroed

  // Fill the input buffer
  inputBuffer.clear();
  for (int i = 0; i < kNumSamples; ++i) {
    inputBuffer.setSample(0, i, 256.f);
  }

  // Construct a panner for the given layout.
  BinauralPanner binPanner(kNumSamples, 48000);

  // Set source position.
  binPanner.setPosition(1.f, 2.f, 3.f);

  // Process audio.
  binPanner.process(inputBuffer, outputBuffer);

  // Validate something sensible was output
  for (int i = 0; i < outputBuffer.getNumChannels(); ++i) {
    for (int j = 0; j < outputBuffer.getNumSamples(); ++j) {
      EXPECT_NE(outputBuffer.getSample(i, j), 0.f);
    }
  }
}

#ifdef RDR_TO_FILE
static std::unique_ptr<juce::AudioFormatWriter> prepareWriter(
    const int sampleRate, const int numChannels) {
  juce::File outputFile =
      juce::File::getCurrentWorkingDirectory().getChildFile("panned.wav");
  juce::WavAudioFormat wavFormat;
  std::unique_ptr<juce::FileOutputStream> outputStream(
      outputFile.createOutputStream());
  std::unique_ptr<juce::AudioFormatWriter> writer(
      wavFormat.createWriterFor(outputStream.get(), sampleRate, numChannels,
                                16,   // Bits per sample
                                {},   // Metadata
                                0));  // Default compression
  outputStream.release();
  return writer;
}

// Test panning a sinewave and writing to a .wav file
TEST(test_surround_panner, pan_tone) {
  const int kSampleRate = 48e3;
  const int kNumSamples = 256;
  const AudioElementSpeakerLayout inputLayout = kMono;
  const AudioElementSpeakerLayout outputLayout = kHOA3;

  // Configure the source to be towards the front left.
  float x, y, z;
  x = -10.f, y = 10.f, z = 1.f;

  // Create a buffer of a 440Hz sinewave.
  juce::AudioBuffer<float> audioSource(inputLayout.getNumChannels(),
                                       kNumSamples);
  audioSource.clear();
  for (int i = 0; i < kNumSamples; ++i) {
    float val1 = .5f * std::sin(440 * i * juce::MathConstants<float>::twoPi /
                                kSampleRate);
    audioSource.setSample(0, i, val1);
  }

  // Construct a panner for the given layout.
  SurroundPanner encoder(inputLayout, outputLayout, kNumSamples);
  // Set source position.
  encoder.setPosition(x, y, z);

  // Prepare output stuff.
  juce::AudioBuffer<float> outBuff(outputLayout.getNumChannels(), kNumSamples);
  auto writer = prepareWriter(kSampleRate, outputLayout.getNumChannels());

  for (int i = 0; i < kSampleRate * 4; i += kNumSamples) {
    encoder.process(audioSource, outBuff);
    writer->writeFromAudioSampleBuffer(outBuff, 0, kNumSamples);
  }
  writer->flush();
}
#endif  // RDR_TO_FILE