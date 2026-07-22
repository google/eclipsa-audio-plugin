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

#include "data_structures/src/FileExport.h"

class FileWriter {
 public:
  FileWriter(const juce::String& filename, double sampleRate, int numChannels,
             int firstChannel, int bitDepth, AudioCodec codec)
      : outputFile_(filename),
        framesWritten(0),
        numChannels_(numChannels),
        firstChannel_(firstChannel),
        bitDepth_(bitDepth) {
    outputFile_.deleteFile();
    juce::WavAudioFormat format;
    auto* stream = new juce::FileOutputStream(outputFile_);
    writer_ = format.createWriterFor(stream,
                                     sampleRate,    // Sample Rate
                                     numChannels_,  // Number of channels
                                     bitDepth_,     // Bits per sample
                                     {},
                                     0  // Quality option index
    );
    if (writer_ == nullptr) {
      // createWriterFor does NOT take ownership of the stream when it fails
      // (see juce_AudioFormat.h) -- delete it ourselves or it leaks.
      delete stream;
      outputStream_ = nullptr;
    } else if (!stream->getStatus().wasOk()) {
      // createWriterFor only checks that the stream pointer is non-null, not
      // whether the underlying file actually opened -- a stream whose open
      // failed (e.g. a missing parent directory, permission denied) still
      // produces a non-null writer here. Treat that the same as an open
      // failure: deleting writer_ also deletes the stream it owns.
      delete writer_;
      writer_ = nullptr;
      outputStream_ = nullptr;
    } else {
      // Non-owning: writer_ owns and will delete this stream. Only ever
      // dereferenced while writer_ is non-null, i.e. while it's still alive.
      outputStream_ = stream;
    }
  }

  // Virtual so tests can subclass and override write()/close() to simulate
  // a failure after a successful open (disk full, WAV 4GB cap) -- a
  // FileWriter constructed against a real, writable path always opens
  // successfully, so that scenario can't otherwise be reproduced in a test.
  virtual ~FileWriter() { close(); };

  // Returns whether the underlying file was successfully opened for writing.
  virtual bool isOpen() const { return writer_ != nullptr; }

  // Returns false (and writes nothing) if the writer failed to open, or if
  // the write itself fails (e.g. disk full, WAV 4GB size limit exceeded).
  virtual bool write(juce::AudioBuffer<float>& buffer) {
    if (writer_ == nullptr) {
      return false;
    }
    juce::AudioBuffer<float> toWrite;
    toWrite.setDataToReferTo(&buffer.getArrayOfWritePointers()[firstChannel_],
                             numChannels_, buffer.getNumSamples());
    const bool kWriteOk =
        writer_->writeFromAudioSampleBuffer(toWrite, 0, buffer.getNumSamples());
    if (kWriteOk) {
      framesWritten += buffer.getNumSamples();
    }
    return kWriteOk;
  }

  // Flushes and closes the writer. Returns true if there was nothing to
  // close (never opened, or already closed), or if flushing succeeded and
  // the underlying stream reports no error. A failure purely in the final
  // header rewrite (which JUCE performs in the writer's destructor with no
  // status return) cannot be detected here -- this is a best-effort check.
  virtual bool close() {
    if (writer_ == nullptr) {
      return true;
    }
    const bool kFlushOk = writer_->flush();
    const bool kStreamOk =
        outputStream_ == nullptr || outputStream_->getStatus().wasOk();
    delete writer_;
    writer_ = nullptr;
    outputStream_ = nullptr;
    return kFlushOk && kStreamOk;
  }

  std::string getFilePath() {
    return outputFile_.getFullPathName().toStdString();
  }

  int getFramesWritten() { return framesWritten; }

 private:
  juce::AudioFormatWriter* writer_;
  // Non-owning observer of the stream writer_ owns; used only to query
  // write-error status at close() time. See constructor/close() for the
  // lifetime argument.
  juce::FileOutputStream* outputStream_;
  juce::File outputFile_;
  int framesWritten;
  int numChannels_;
  int firstChannel_;
  int bitDepth_;
};