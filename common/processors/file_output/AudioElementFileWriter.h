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
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "FileWriter.h"
#include "data_structures/src/AudioElement.h"

class AudioElementFileWriter {
 public:
  AudioElementFileWriter(const juce::String& filename, double sampleRate,
                         int bitDepth, AudioCodec codec,
                         AudioElement& element)
      : element_(element)  // Use a local copy of the audio element to avoid
                           // updates elsewhere causing issues
  {
    fileWriter = new FileWriter(filename, sampleRate, element.getChannelCount(),
                                element.getFirstChannel(), bitDepth, codec);
  }

  ~AudioElementFileWriter() {
    close();
    if (fileWriter != nullptr) delete fileWriter;
  };

  // disable copy semantics (copy-ctor & copy-assignment) to prevent
  // accidental shallow copies and double-free bugs
  AudioElementFileWriter(const AudioElementFileWriter&) = delete;
  AudioElementFileWriter& operator=(const AudioElementFileWriter&) = delete;

  // enable default move semantics so the object can be relocated
  // (e.g. by std::vector growth) without copying internal buffers
  AudioElementFileWriter(AudioElementFileWriter&&) noexcept = default;
  AudioElementFileWriter& operator=(AudioElementFileWriter&&) noexcept =
      default;

  void write(juce::AudioBuffer<float>& buffer) { fileWriter->write(buffer); }

  void close() { fileWriter->close(); }

  AudioElement& getElement() { return element_; }

  std::string getFilePath() { return fileWriter->getFilePath(); }

  int getFramesWritten() { return fileWriter->getFramesWritten(); }

 private:
  AudioElement element_;  // Use a local copy to avoid updates elsewhere causing
                          // issues
  FileWriter* fileWriter;
};