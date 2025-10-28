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

  juce::Colour textColour = juce::Colour(221, 228, 227);
  timeLabel_.setColour(juce::Label::ColourIds::backgroundColourId,
                       juce::Colours::transparentBlack);
  timeLabel_.setColour(juce::Label::textColourId, textColour);
  timeLabel_.setFont(juce::Font("Roboto", 12.0f, juce::Font::plain));

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
  addAndMakeVisible(*spinner_);

  if (fpbr_.get().getPlaybackFile().isNotEmpty()) {
    attemptCreatePlaybackEngine();
  }
  updateButtonVisibility();
  update();
  startTimerHz(30);
  fpbr_.registerListener(this);
  fer_.registerListener(this);
}

AudioFilePlayer::~AudioFilePlayer() {
  fpbr_.deregisterListener(this);
  fer_.deregisterListener(this);
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

  if (isBuffering) {
    flexBox.items.add(
        juce::FlexItem(*spinner_)
            .withWidth(kButtonSz)
            .withHeight(kButtonSz)
            .withMargin(juce::FlexItem::Margin(0, kGap + kButtonSz / 2.0f, 0,
                                               kGap + kButtonSz / 2.0f)));
  } else {
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
  }
}

void AudioFilePlayer::timerCallback() { update(); }

void AudioFilePlayer::valueTreePropertyChanged(
    juce::ValueTree& tree, const juce::Identifier& property) {
  if (property == FilePlayback::kPlayState) {
    triggerAsyncUpdate();
  } else if (property == FilePlayback::kPlaybackFile) {
    attemptCreatePlaybackEngine();
  } else if (property == FileExport::kExportCompleted &&
             fer_.get().getExportCompleted()) {
    attemptCreatePlaybackEngine();
  }
}

void AudioFilePlayer::handleAsyncUpdate() {
  updateButtonVisibility();
  resized();
}

void AudioFilePlayer::updateButtonVisibility() {
  auto fpb = fpbr_.get();
  auto playState = fpb.getPlayState();
  bool isPlaying = (playState == FilePlayback::kPlay);
  bool isBuffering = (playState == FilePlayback::kBuffering);

  playButton_.setVisible(!isPlaying && !isBuffering);
  pauseButton_.setVisible(isPlaying);
  stopButton_.setVisible(!isBuffering);
  if (spinner_) spinner_->setVisible(isBuffering);
}

void AudioFilePlayer::createPlaybackEngine(
    const std::filesystem::path iamfPath) {
  auto playbackState = fpbr_.get();
  playbackState.setPlayState(FilePlayback::kBuffering);
  fpbr_.update(playbackState);
  const juce::String kDevice = fpbr_.get().getPlaybackDevice();
  playbackEngine_ =
      std::make_unique<IAMFPlaybackDevice>(iamfPath, kDevice, fpbr_);
}

void AudioFilePlayer::attemptCreatePlaybackEngine() {
  auto playbackState = fer_.get();
  const std::filesystem::path kFileToLoad(
      playbackState.getExportFile().toStdString());
  if (kFileToLoad.empty() || kFileToLoad.extension() != ".iamf" ||
      !std::filesystem::exists(kFileToLoad)) {
    return;
  }

  createPlaybackEngine(kFileToLoad);
}
