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

#include <memory>

#include "components/icons/svg/SvgIconLookup.h"
#include "components/src/EclipsaColours.h"
#include "data_structures/src/FilePlayback.h"

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
                                 FilePlaybackProcessorData& fpbData)
    : playButton_("Play", SvgMap::kPlay),
      pauseButton_("Pause", SvgMap::kPause),
      stopButton_("Stop", SvgMap::kStop),
      timeLabel_("timeLabel", "00:00 / 00:00"),
      volumeIcon_(SvgMap::kVolume),
      spinner_(std::make_unique<Spinner>()),
      fpbr_(filePlaybackRepo),
      fpbData_(fpbData) {
  playButton_.setColour(juce::TextButton::buttonColourId,
                        EclipsaColours::rolloverGrey);
  pauseButton_.setColour(juce::TextButton::buttonColourId,
                         EclipsaColours::rolloverGrey);
  stopButton_.setColour(juce::TextButton::buttonColourId,
                        EclipsaColours::rolloverGrey);
  playButton_.onClick = [this]() {
    FilePlayback fpb = fpbr_.get();
    fpb.setPlaybackCommand(FilePlayback::PlaybackCommand::kPlay);
    fpbr_.update(fpb);
  };
  pauseButton_.onClick = [this]() {
    FilePlayback fpb = fpbr_.get();
    fpb.setPlaybackCommand(FilePlayback::PlaybackCommand::kPause);
    fpbr_.update(fpb);
  };
  stopButton_.onClick = [this]() {
    FilePlayback fpb = fpbr_.get();
    fpb.setPlaybackCommand(FilePlayback::PlaybackCommand::kStop);
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
  playbackSlider_.setValue(0.0, juce::sendNotification);
  playbackSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
  playbackSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  playbackSlider_.onDragEnd = [this]() {
    FilePlayback fpb = fpbr_.get();
    fpb.setSeekPosition(playbackSlider_.getValue());
    fpbr_.update(fpb);
  };
  addAndMakeVisible(playbackSlider_);

  volumeSlider_.setRange(0, 2);
  volumeSlider_.setValue(1, juce::sendNotification);
  volumeSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
  volumeSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  volumeSlider_.onValueChange = [this]() {
    FilePlayback fpb = fpbr_.get();
    const float kVol = volumeSlider_.getValue();
    fpb.setVolume(kVol);
    fpbr_.update(fpb);
  };

  addAndMakeVisible(volumeSlider_);
  addAndMakeVisible(playButton_);
  addAndMakeVisible(pauseButton_);
  addAndMakeVisible(stopButton_);
  addAndMakeVisible(timeLabel_);
  addAndMakeVisible(volumeIcon_);
  addAndMakeVisible(fileSelectLabel_);
  addAndMakeVisible(*spinner_);

  updateComponentVisibility();
  startTimerHz(30);
}

AudioFilePlayer::~AudioFilePlayer() {
  FilePlayback fpb = fpbr_.get();
  fpb.setPlaybackCommand(FilePlayback::PlaybackCommand::kPause);
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

  // Only render the warning label if we get to a disabled state
  FilePlayback::ProcessorState state;
  fpbData_.processorState.read(state);
  if (state == FilePlayback::ProcessorState::kError) {
    flexBox.items.add(juce::FlexItem(fileSelectLabel_)
                          .withFlex(1)
                          .withHeight(kButtonSz)
                          .withMargin(juce::FlexItem::Margin(0, 5, 0, 5)));
    flexBox.performLayout(bounds);
    return;
  }

  // Render the spinner when buffering
  if (state == FilePlayback::ProcessorState::kBuffering) {
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
  FilePlayback::ProcessorState state;
  fpbData_.processorState.read(state);
  if (state != FilePlayback::ProcessorState::kError) {
    float currPos = 0.0f;
    fpbData_.currFilePosition.read(currPos);
    unsigned long long duration_s = 0.0f;
    fpbData_.fileDuration_s.read(duration_s);
    const float kCurrentTime = currPos * duration_s;

    const int kCurrMinutes = static_cast<int>(kCurrentTime) / 60;
    const int kCurrSeconds = static_cast<int>(kCurrentTime) % 60;
    const int kTotalMinutes = static_cast<int>(duration_s) / 60;
    const int kTotalSeconds = static_cast<int>(duration_s) % 60;

    juce::String timeStr;
    // Format long file durations differently
    if (duration_s >= 3600) {
      timeStr =
          juce::String::formatted("%d:%02d / %d:%02d", kCurrMinutes,
                                  kCurrSeconds, kTotalMinutes, kTotalSeconds);
    } else {
      timeStr =
          juce::String::formatted("%02d:%02d / %02d:%02d", kCurrMinutes,
                                  kCurrSeconds, kTotalMinutes, kTotalSeconds);
    }
    timeLabel_.setText(timeStr, juce::dontSendNotification);

    if (!playbackSlider_.isMouseButtonDown()) {
      playbackSlider_.setValue(currPos, juce::dontSendNotification);
    }
  }
}

void AudioFilePlayer::timerCallback() {
  update();
  updateComponentVisibility();
  resized();
}

void AudioFilePlayer::updateComponentVisibility() {
  FilePlayback::ProcessorState playState;
  fpbData_.processorState.read(playState);
  const bool kPlaying = (playState == FilePlayback::ProcessorState::kPlaying);
  const bool kBuffering =
      (playState == FilePlayback::ProcessorState::kBuffering);
  const bool kError = (playState == FilePlayback::ProcessorState::kError);
  fileSelectLabel_.setVisible(kError);
  playButton_.setVisible(!kPlaying && !kBuffering && !kError);
  pauseButton_.setVisible(kPlaying && !kError);
  stopButton_.setVisible(!kBuffering && !kError);
  timeLabel_.setVisible(!kError);
  playbackSlider_.setVisible(!kError);
  playbackSlider_.setEnabled(!kBuffering);
  volumeIcon_.setVisible(!kError);
  volumeSlider_.setVisible(!kError);
  if (spinner_) spinner_->setVisible(kBuffering);
}