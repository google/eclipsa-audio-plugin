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

#include "IAMFPlaybackDevice.h"

#include <memory>

#include "data_structures/src/FilePlayback.h"
#include "player/src/transport/IAMFDecoderSource.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

IAMFPlaybackDevice::Result IAMFPlaybackDevice::create(
    const std::filesystem::path iamfPath, const juce::String pbDeviceName,
    std::atomic_bool& abortConstruction,
    FilePlaybackRepository& filePlaybackRepo,
    juce::AudioDeviceManager& deviceManager) {
  // Attempt to create the IAMFFileReader first. Being unable to create the
  // reader for any reason invalidates the playback device.
  // While attempting to index the file during construction, we acknowledge that
  // due to the potential size of IAMF files we may need to abort before
  // indexing can complete
  auto reader = IAMFFileReader::createIamfReader(
      iamfPath, IAMFFileReader::kDefaultReaderSettings, abortConstruction);
  if (!reader && abortConstruction) {
    return {nullptr, Error::kEarlyAbortRequested};
  }
  if (!reader) {
    LOG_ERROR(0, "IAMFPlaybackDevice: Failed to create IAMF reader");
    return {nullptr, Error::kInvalidIAMFFile};
  }

  auto device = std::unique_ptr<IAMFPlaybackDevice>(
      new IAMFPlaybackDevice(iamfPath, pbDeviceName, filePlaybackRepo,
                             deviceManager, std::move(reader)));

  // Complete initialization
  FilePlayback fpb = filePlaybackRepo.get();
  device->configureDecodeLayout(fpb.getReqdDecodeLayout());
  device->configurePlaybackDevice(fpb.getPlaybackDevice());
  return {std::move(device), Error::kNoError};
}

IAMFPlaybackDevice::IAMFPlaybackDevice(const std::filesystem::path iamfPath,
                                       const juce::String pbDeviceName,
                                       FilePlaybackRepository& filePlaybackRepo,
                                       juce::AudioDeviceManager& deviceManager,
                                       std::unique_ptr<IAMFFileReader> reader)
    : kPath_(iamfPath),
      fpbr_(filePlaybackRepo),
      deviceManager_(deviceManager),
      decoderSource_(std::move(reader)) {
  deviceManager_.initialiseWithDefaultDevices(0, 2);

  decoderSource_.setOnFinishedCallback([this] {
    juce::MessageManager::callAsync(
        [this]() { setRepoState(FilePlayback::kStop); });
  });

  fpbr_.registerListener(this);
}

IAMFPlaybackDevice::~IAMFPlaybackDevice() {
  decoderSource_.stop();
  deviceManager_.removeAudioCallback(&sourcePlayer_);
  sourcePlayer_.setSource(nullptr);
  fpbr_.deregisterListener(this);
}

void IAMFPlaybackDevice::play() { decoderSource_.play(); }

void IAMFPlaybackDevice::pause() { decoderSource_.pause(); }

void IAMFPlaybackDevice::stop() { decoderSource_.stop(); }

void IAMFPlaybackDevice::seekTo(const float position) {
  jassert(position >= 0 && position <= 1.0);
  deviceManager_.closeAudioDevice();
  const bool wasPlaying = decoderSource_.isPlaying();
  decoderSource_.pause();
  const size_t kFrameIdx =
      static_cast<size_t>(position * decoderSource_.getStreamData().numFrames);
  decoderSource_.seek(kFrameIdx);
  deviceManager_.restartLastAudioDevice();
  if (wasPlaying) decoderSource_.play();
}

void IAMFPlaybackDevice::configurePlaybackDevice(
    const juce::String deviceName) {
  const bool kFirstSetup = deviceManager_.getCurrentAudioDevice() == nullptr;

  if (!kFirstSetup) {
    decoderSource_.pause();
    deviceManager_.removeAudioCallback(&sourcePlayer_);
    sourcePlayer_.setSource(nullptr);
  }

  const IAMFFileReader::StreamData kData = decoderSource_.getStreamData();
  const juce::AudioDeviceManager::AudioDeviceSetup kAppliedSetup =
      setupAudioDevice(deviceName, kData, kFirstSetup);
  updateResampler(kData.sampleRate,
                  static_cast<unsigned>(kAppliedSetup.sampleRate),
                  kData.numChannels);
  setPlayerSource();
  deviceManager_.addAudioCallback(&sourcePlayer_);
}

void IAMFPlaybackDevice::configureDecodeLayout(
    const Speakers::AudioElementSpeakerLayout layout) {
  // Stop audio callbacks while we recreate decoder
  deviceManager_.removeAudioCallback(&sourcePlayer_);
  sourcePlayer_.setSource(nullptr);

  const Speakers::AudioElementSpeakerLayout kLayout =
      fpbr_.get().getReqdDecodeLayout();
  decoderSource_.setLayout(kLayout);
  setPlayerSource();
  deviceManager_.addAudioCallback(&sourcePlayer_);
}

void IAMFPlaybackDevice::setVolume(const float volume) {
  sourcePlayer_.setGain(volume);
}

IAMFFileReader::StreamData IAMFPlaybackDevice::getStreamData() const {
  return decoderSource_.getStreamData();
}

void IAMFPlaybackDevice::valueTreePropertyChanged(
    juce::ValueTree& tree, const juce::Identifier& property) {
  const FilePlayback fpb(fpbr_.get());
  const PlaybackState kPrevState = {decoderSource_.isPlaying(),
                                    fpb.getPlayState()};
  if (property == FilePlayback::kPlayState) {
    switch (fpb.getPlayState()) {
      case FilePlayback::kPlay:
        play();
        break;
      case FilePlayback::kPause:
        pause();
        break;
      case FilePlayback::kStop:
        stop();
        break;
      default:
        break;
    }
  } else if (property == FilePlayback::kVolume) {
    setVolume(fpb.getVolume());
  } else if (property == FilePlayback::kPlaybackDevice) {
    setRepoState(FilePlayback::kBuffering);
    configurePlaybackDevice(fpb.getPlaybackDevice());
    setRepoState(kPrevState.state);
    if (kPrevState.wasPlaying) decoderSource_.play();
  } else if (property == FilePlayback::kReqdDecodeLayout) {
    setRepoState(FilePlayback::kBuffering);
    configureDecodeLayout(fpb.getReqdDecodeLayout());
    setRepoState(FilePlayback::kStop);
  } else if (property == FilePlayback::kSeekPosition) {
    setRepoState(FilePlayback::kBuffering);
    seekTo(fpb.getSeekPosition());
    if (kPrevState.state == FilePlayback::kStop) {
      setRepoState(FilePlayback::kPause);
    } else {
      setRepoState(kPrevState.state);
    }
    if (kPrevState.wasPlaying) decoderSource_.play();
  }
}

void IAMFPlaybackDevice::setRepoState(
    const FilePlayback::CurrentPlayerState state) {
  auto fpb = fpbr_.get();
  fpb.setPlayState(state);
  fpbr_.update(fpb);
}

void IAMFPlaybackDevice::setPlayerSource() {
  juce::AudioSource* source;
  if (resampler_) {
    source = resampler_.get();
  } else {
    source = &decoderSource_;
  }
  sourcePlayer_.setSource(source);
}

IAMFPlaybackDevice::PlaybackState IAMFPlaybackDevice::capturePbState() const {
  const FilePlayback::CurrentPlayerState kState = fpbr_.get().getPlayState();
  return {decoderSource_.isPlaying(), kState};
}

juce::AudioDeviceManager::AudioDeviceSetup IAMFPlaybackDevice::setupAudioDevice(
    const juce::String& deviceName,
    const IAMFFileReader::StreamData& streamData, bool isInitialSetup) {
  const auto sampleRate = streamData.sampleRate;
  const auto frameSize = streamData.frameSize;
  const auto numChannels = streamData.numChannels;

  if (isInitialSetup && deviceName.isEmpty()) {
    deviceManager_.initialiseWithDefaultDevices(
        0, numChannels);  // Use default output.
  }

  auto setup = deviceManager_.getAudioDeviceSetup();
  if (!deviceName.isEmpty()) {
    setup.outputDeviceName = deviceName;
  }
  setup.sampleRate = sampleRate;
  setup.bufferSize = frameSize;
  setup.useDefaultOutputChannels = true;

  const auto err = deviceManager_.setAudioDeviceSetup(setup, true);
  if (!err.isEmpty()) {
    LOG_WARNING(
        0, "IAMFPlaybackEngine: Failed to set device: " + err.toStdString());
  }

  setup =
      deviceManager_.getAudioDeviceSetup();  // Refresh actual applied setup.
  if (!deviceName.isEmpty() && setup.outputDeviceName != deviceName) {
    LOG_WARNING(0, "IAMFPlaybackEngine: Device name mismatch after setup");
  }
  if (setup.sampleRate != sampleRate || setup.bufferSize != frameSize ||
      setup.outputChannels.countNumberOfSetBits() != numChannels) {
    LOG_WARNING(0,
                "IAMFPlaybackEngine: Device configuration differs from "
                "requested parameters.");
  }
  return setup;
}

void IAMFPlaybackDevice::updateResampler(unsigned sourceSampleRate,
                                         unsigned deviceSampleRate,
                                         unsigned numChannels) {
  if (deviceSampleRate != sourceSampleRate) {
    const double requiredRatio =
        static_cast<double>(sourceSampleRate) / deviceSampleRate;
    if (!resampler_ || resampler_->getResamplingRatio() != requiredRatio) {
      resampler_ = std::make_unique<juce::ResamplingAudioSource>(
          &decoderSource_, false, numChannels);
      resampler_->setResamplingRatio(requiredRatio);
      LOG_INFO(0, "IAMFPlaybackEngine: Configured resampler (" +
                      std::to_string(sourceSampleRate) + " Hz -> " +
                      std::to_string(deviceSampleRate) + " Hz)");
    }
  } else if (resampler_) {
    resampler_.reset();
    LOG_INFO(0, "IAMFPlaybackEngine: Removed resampler (not needed)");
  }
}
