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

#include "AudioFilePlayer.h"

#include <filesystem>
#include <memory>

#include "components/icons/svg/SvgIconLookup.h"
#include "components/src/EclipsaColours.h"
#include "data_structures/src/FilePlayback.h"
#include "player/src/transport/IAMFPlaybackDevice.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

class AudioFilePlayer::Spinner : public juce::Component, private juce::Timer {
 public:
  Spinner() : angle_(0.0f) { startTimerHz(60); }
  ~Spinner() override { stopTimer(); }

  void paint(juce::Graphics& g) override {
    auto bounds = getLocalBounds().toFloat();
    float radius =
        std::min(bounds.getWidth(), bounds.getHeight()) / 2.0f - 2.0f;
    juce::Point<float> center = bounds.getCentre();
    float thickness = radius * 0.18f;
    float arcLength = juce::MathConstants<float>::pi * 1.2f;
    float startAngle = angle_;
    float endAngle = startAngle + arcLength;

    g.setColour(EclipsaColours::inactiveGrey);
    g.drawEllipse(center.x - radius, center.y - radius, radius * 2, radius * 2,
                  thickness);

    juce::Path arcPath;
    arcPath.addArc(center.x - radius, center.y - radius, radius * 2, radius * 2,
                   startAngle, endAngle, true);
    g.setColour(EclipsaColours::selectCyan);
    g.strokePath(arcPath, juce::PathStrokeType(thickness));
  }

  void timerCallback() override {
    angle_ += 0.12f;
    if (angle_ > juce::MathConstants<float>::twoPi)
      angle_ -= juce::MathConstants<float>::twoPi;
    repaint();
  }

 private:
  float angle_;
};

AudioFilePlayer::AudioFilePlayer(FilePlaybackRepository& filePlaybackRepo,
                                 FileExportRepository& fileExportRepo)
    : playButton_("Play", SvgMap::kPlay),
      pauseButton_("Pause", SvgMap::kPause),
      stopButton_("Stop", SvgMap::kStop),
      timeLabel_("timeLabel", "00:00 / 00:00"),
      volumeIcon_(SvgMap::kVolume),
      spinner_(std::make_unique<Spinner>()),
      fpbr_(filePlaybackRepo),
      fer_(fileExportRepo) {
  playButton_.setColour(juce::TextButton::buttonColourId,
                        EclipsaColours::rolloverGrey);
  pauseButton_.setColour(juce::TextButton::buttonColourId,
                         EclipsaColours::rolloverGrey);
  stopButton_.setColour(juce::TextButton::buttonColourId,
                        EclipsaColours::rolloverGrey);
  playButton_.onClick = [this]() {
    auto fpb = fpbr_.get();
    fpb.setPlayState(FilePlayback::kPlay);
    fpbr_.update(fpb);
  };
  pauseButton_.onClick = [this]() {
    auto fpb = fpbr_.get();
    fpb.setPlayState(FilePlayback::kPause);
    fpbr_.update(fpb);
  };
  stopButton_.onClick = [this]() {
    auto fpb = fpbr_.get();
    fpb.setPlayState(FilePlayback::kStop);
    fpbr_.update(fpb);
  };

  timeLabel_.setColour(juce::Label::ColourIds::backgroundColourId,
                       juce::Colours::transparentBlack);
  timeLabel_.setColour(juce::Label::textColourId, EclipsaColours::headingGrey);
  timeLabel_.setFont(juce::Font("Roboto", 12.0f, juce::Font::plain));

  fileSelectLabel_.setColour(juce::Label::ColourIds::backgroundColourId,
                             juce::Colours::transparentBlack);
  fileSelectLabel_.setColour(juce::Label::textColourId, EclipsaColours::red);
  fileSelectLabel_.setFont(juce::Font("Roboto", 12.0f, juce::Font::plain));
  fileSelectLabel_.setJustificationType(juce::Justification::centred);

  playbackSlider_.setRange(0.0, 1.0);
  playbackSlider_.setValue(0.0);
  playbackSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
  playbackSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  playbackSlider_.onValueChange = [this]() {
    auto fpb = fpbr_.get();
    fpb.setSeekPosition(static_cast<float>(playbackSlider_.getValue()));
    fpbr_.update(fpb);
  };
  addAndMakeVisible(playbackSlider_);

  volumeSlider_.setRange(0, 1);
  volumeSlider_.setValue(0.5);
  volumeSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
  volumeSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  addAndMakeVisible(volumeSlider_);

  addAndMakeVisible(playButton_);
  addAndMakeVisible(pauseButton_);
  addAndMakeVisible(stopButton_);
  addAndMakeVisible(timeLabel_);
  addAndMakeVisible(volumeIcon_);
  addAndMakeVisible(fileSelectLabel_);
  addAndMakeVisible(*spinner_);

  fpbr_.registerListener(this);
  fer_.registerListener(this);
  if (fpbr_.get().getPlaybackFile().isNotEmpty()) {
    attemptCreatePlaybackEngine();
  }
  updateComponentVisibility();
  startTimerHz(30);
}

AudioFilePlayer::~AudioFilePlayer() {
  // Signal that we're being destroyed. Join the background thread for safe
  // cleanup.
  isBeingDestroyed_ = true;
  if (playbackEngineLoaderThread_.joinable()) {
    playbackEngineLoaderThread_.join();
  }

  fpbr_.deregisterListener(this);
  fer_.deregisterListener(this);

  FilePlayback fpb = fpbr_.get();
  fpb.setPlayState(FilePlayback::kStop);
  fpbr_.update(fpb);
}

void AudioFilePlayer::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds();
  g.setColour(EclipsaColours::semiOnButtonGrey);
  g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
}

void AudioFilePlayer::resized() {
  auto bounds = getLocalBounds();
  bounds.reduce(5, 5);

  juce::FlexBox flexBox;
  flexBox.flexDirection = juce::FlexBox::Direction::row;
  flexBox.justifyContent = juce::FlexBox::JustifyContent::flexStart;
  flexBox.alignItems = juce::FlexBox::AlignItems::center;

  const int kButtonSz = 24;
  const int kGap = 5;

  auto fpb = fpbr_.get();
  bool isBuffering = (fpb.getPlayState() == FilePlayback::kBuffering);

  // Only render the warning label if we get to a disabled state
  if (fpb.getPlayState() == FilePlayback::kDisabled) {
    flexBox.items.add(juce::FlexItem(fileSelectLabel_)
                          .withFlex(1)
                          .withHeight(kButtonSz)
                          .withMargin(juce::FlexItem::Margin(0, 5, 0, 5)));
    flexBox.performLayout(bounds);
    return;
  }

  // Render the spinner when buffering
  if (isBuffering) {
    flexBox.items.add(
        juce::FlexItem(*spinner_)
            .withWidth(kButtonSz)
            .withHeight(kButtonSz)
            .withMargin(juce::FlexItem::Margin(0, kGap + kButtonSz / 2.0f, 0,
                                               kGap + kButtonSz / 2.0f)));
  }
  // Render the play/stop buttons otherwise
  else {
    if (playButton_.isVisible()) {
      flexBox.items.add(juce::FlexItem(playButton_)
                            .withWidth(kButtonSz)
                            .withHeight(kButtonSz)
                            .withMargin(juce::FlexItem::Margin(0, kGap, 0, 0)));
    } else {
      flexBox.items.add(juce::FlexItem(pauseButton_)
                            .withWidth(kButtonSz)
                            .withHeight(kButtonSz)
                            .withMargin(juce::FlexItem::Margin(0, kGap, 0, 0)));
    }
    flexBox.items.add(juce::FlexItem(stopButton_)
                          .withWidth(kButtonSz)
                          .withHeight(kButtonSz)
                          .withMargin(juce::FlexItem::Margin(0, kGap, 0, 0)));
  }
  // Render the time label, playback slider and volume controls
  flexBox.items.add(juce::FlexItem(timeLabel_)
                        .withFlex(1)
                        .withHeight(kButtonSz)
                        .withMargin(juce::FlexItem::Margin(0, 5, 0, 5)));
  flexBox.items.add(juce::FlexItem(playbackSlider_)
                        .withFlex(2)
                        .withHeight(kButtonSz)
                        .withMargin(juce::FlexItem::Margin(0, kGap, 0, 0)));
  flexBox.items.add(juce::FlexItem(volumeIcon_)
                        .withWidth(kButtonSz * .7)
                        .withHeight(kButtonSz)
                        .withMargin(juce::FlexItem::Margin(0, kGap, 0, 0)));
  flexBox.items.add(juce::FlexItem(volumeSlider_)
                        .withWidth(kButtonSz * 2 + kGap * 3)
                        .withHeight(kButtonSz));

  flexBox.performLayout(bounds);
}

void AudioFilePlayer::update() {
  std::lock_guard<std::mutex> lock(pbeMutex_);
  if (playbackEngine_) {
    const IAMFFileReader::StreamData kData = playbackEngine_->getStreamData();
    const float kDuration_s =
        kData.numFrames * kData.frameSize / (float)kData.sampleRate;
    const float kPosition_s =
        kData.currentFrameIdx * kData.frameSize / (float)kData.sampleRate;

    const int durationMins = (int)(kDuration_s / 60);
    const int durationSecs = (int)(kDuration_s) % 60;
    const int currentMins = (int)(kPosition_s / 60);
    const int currentSecs = (int)(kPosition_s) % 60;
    timeLabel_.setText(
        juce::String::formatted("%02d:%02d / %02d:%02d", currentMins,
                                currentSecs, durationMins, durationSecs),
        juce::dontSendNotification);
    playbackSlider_.setValue(
        kDuration_s > 0.0f ? (kPosition_s / kDuration_s) : 0.0f,
        juce::dontSendNotification);
    playbackEngine_->setVolume(volumeSlider_.getValue());
  } else {
    timeLabel_.setText("00:00 / 00:00", juce::dontSendNotification);
    playbackSlider_.setValue(0, juce::dontSendNotification);
  }
}

void AudioFilePlayer::timerCallback() { update(); }

void AudioFilePlayer::valueTreePropertyChanged(
    juce::ValueTree& tree, const juce::Identifier& property) {
  if (property == FilePlayback::kPlayState) {
    triggerAsyncUpdate();
  } else if (property == FilePlayback::kPlaybackFile) {
    attemptCreatePlaybackEngine();
  } else if (property == FileExport::kExportCompleted) {
    // When this property is false a new export is starting, so we want to
    // destroy the player and wait until export is complete.
    // When this property is true we want to attempt to create the playback
    // engine again.
    if (fer_.get().getExportCompleted()) {
      auto safeThis = juce::Component::SafePointer<AudioFilePlayer>(this);
      juce::MessageManager::callAsync(
          [safeThis]() { safeThis->attemptCreatePlaybackEngine(); });
    } else {
      cancelCreatePlaybackEngine();
    }
  }
}

void AudioFilePlayer::handleAsyncUpdate() {
  updateComponentVisibility();
  resized();
}

void AudioFilePlayer::updateComponentVisibility() {
  auto fpb = fpbr_.get();
  auto playState = fpb.getPlayState();
  const bool kPlaying = (playState == FilePlayback::kPlay);
  const bool kBuffering = (playState == FilePlayback::kBuffering);
  const bool kDisabled = (playState == FilePlayback::kDisabled);
  fileSelectLabel_.setVisible(kDisabled);
  playButton_.setVisible(!kPlaying && !kBuffering && !kDisabled);
  pauseButton_.setVisible(kPlaying && !kDisabled);
  stopButton_.setVisible(!kBuffering && !kDisabled);
  timeLabel_.setVisible(!kDisabled);
  playbackSlider_.setVisible(!kDisabled);
  volumeIcon_.setVisible(!kDisabled);
  volumeSlider_.setVisible(!kDisabled);
  if (spinner_) spinner_->setVisible(kBuffering);
}

void AudioFilePlayer::cancelCreatePlaybackEngine() {
  isBeingDestroyed_ = true;
  if (playbackEngineLoaderThread_.joinable()) {
    playbackEngineLoaderThread_.join();
  }
  // Wait for async callback to complete
  std::unique_lock<std::mutex> lock(pbeMutex_);
  pbeCv_.wait(lock, [this] { return !isLoadingPlaybackEngine_; });
  isBeingDestroyed_ = false;
}

void AudioFilePlayer::attemptCreatePlaybackEngine() {
  cancelCreatePlaybackEngine();

  // If the file doesn't exist or it's a new file, we set the player to a
  // stopped state
  auto fe = fer_.get();
  const std::filesystem::path kFileToLoad(fe.getExportFile().toStdString());
  if (kFileToLoad.empty() || kFileToLoad.extension() != ".iamf" ||
      !std::filesystem::exists(kFileToLoad)) {
    auto playbackState = fpbr_.get();
    playbackState.setPlayState(FilePlayback::kStop);
    fpbr_.update(playbackState);
    return;
  }

  auto fpb = fpbr_.get();
  fpb.setPlayState(FilePlayback::kBuffering);
  fpbr_.update(fpb);

  createPlaybackEngine(kFileToLoad);
}

void AudioFilePlayer::createPlaybackEngine(
    const std::filesystem::path iamfPath) {
  const juce::String kDevice = fpbr_.get().getPlaybackDevice();
  playbackEngineLoaderThread_ = std::thread([this, iamfPath, kDevice]() {
    isLoadingPlaybackEngine_ = true;

    IAMFPlaybackDevice::Result res = IAMFPlaybackDevice::create(
        iamfPath, kDevice, isBeingDestroyed_, fpbr_, deviceManager_);

    auto safeThis = juce::Component::SafePointer<AudioFilePlayer>(this);

    if (isBeingDestroyed_) {
      isLoadingPlaybackEngine_ = false;
      return;
    }
    juce::MessageManager::callAsync(
        [safeThis, device = res.device.release(), error = res.error]() {
          if (safeThis && !safeThis->isBeingDestroyed_) {
            safeThis->onPlaybackEngineCreated(IAMFPlaybackDevice::Result{
                std::unique_ptr<IAMFPlaybackDevice>(device), error});
          } else {
            delete device;
          }

          // Always reset the loading flag and notify waiters
          if (safeThis) {
            std::lock_guard<std::mutex> lock(safeThis->pbeMutex_);
            safeThis->isLoadingPlaybackEngine_ = false;
            safeThis->pbeCv_.notify_all();
          }
        });
  });
}

void AudioFilePlayer::onPlaybackEngineCreated(IAMFPlaybackDevice::Result res) {
  std::lock_guard<std::mutex> lock(pbeMutex_);
  if (!res.device && res.error == IAMFPlaybackDevice::Error::kInvalidIAMFFile) {
    // Failed to create playback engine - reset state to disabled
    playbackEngine_ = nullptr;
    auto fpb = fpbr_.get();
    fpb.setPlayState(FilePlayback::kDisabled);
    fpbr_.update(fpb);
  } else if (res.error == IAMFPlaybackDevice::kEarlyAbortRequested ||
             isBeingDestroyed_) {
    // Do nothing - destruction was requested
    playbackEngine_ = nullptr;
  } else {
    playbackEngine_ = std::move(res.device);
    // Update play state from buffering to ready
    auto fpb = fpbr_.get();
    fpb.setPlayState(FilePlayback::kStop);
    fpbr_.update(fpb);
  }
}