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

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "AudioFilePlayer.h"
#include "components/icons/svg/SvgIconComponent.h"
#include "components/icons/svg/SvgIconLookup.h"
#include "components/src/SelectionBox.h"
#include "data_repository/implementation/FileExportRepository.h"
#include "data_repository/implementation/FilePlaybackRepository.h"
#include "data_structures/src/FilePlayback.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class ExportValidationComponent : public juce::Component,
                                  juce::ValueTree::Listener {
 public:
  ExportValidationComponent(FilePlaybackRepository& filePlaybackRepo,
                            FileExportRepository& fileExportRepo)
      : title_("Export validation", "Export validation"),
        audioPlayer_(filePlaybackRepo, fileExportRepo),
        playbackDevice_("Playback Device"),
        layoutToDecode_("Mix Presentation Layout"),
        decodeToolTip_(SvgMap::kHelp,
                       "The decoder will decode the Mix Presentation which "
                       "best matches the requested layout."),
        fpbr_(filePlaybackRepo) {
    title_.setColour(juce::Label::textColourId, EclipsaColours::headingGrey);
    title_.setJustificationType(juce::Justification::left);
    title_.setFont(juce::Font("Roboto", 22.0f, juce::Font::plain));
    addAndMakeVisible(title_);

    // Populate playback devices
    populatePlaybackDevices();

    // Setup playback device onChange handler
    playbackDevice_.onChange([this]() {
      const int selectedIndex = playbackDevice_.getSelectedIndex();
      if (selectedIndex >= 0 && selectedIndex < deviceNames_.size()) {
        auto fpb = fpbr_.get();
        fpb.setPlaybackDevice(deviceNames_[selectedIndex]);
        fpbr_.update(fpb);
      }
    });

    // Populate layout options
    for (const auto& layout : kLayouts) {
      layoutToDecode_.addOption(layout.toString());
    }

    // Setup layout onChange handler
    layoutToDecode_.onChange([this]() {
      const int selectedIndex = layoutToDecode_.getSelectedIndex();
      if (selectedIndex >= 0 && selectedIndex < kLayouts.size()) {
        auto fpb = fpbr_.get();
        fpb.setReqdDecodeLayout(kLayouts[selectedIndex]);
        fpbr_.update(fpb);
      }
    });

    // Setup the Mix Presentation Layout tooltip
    decodeToolTip_.setInterceptsMouseClicks(true, true);
    // Add tooltip window to enable tooltip display
    tooltipWindow_.setMillisecondsBeforeTipAppears(500);
    addAndMakeVisible(tooltipWindow_);

    addAndMakeVisible(audioPlayer_);
    addAndMakeVisible(playbackDevice_);
    addAndMakeVisible(layoutToDecode_);
    addAndMakeVisible(decodeToolTip_);
    fpbr_.registerListener(this);
  }

  ~ExportValidationComponent() { fpbr_.deregisterListener(this); }

  void resized() override {
    auto bounds = getLocalBounds();

    const int kRowHeight = 65, kGap = 10;
    title_.setBounds(bounds.removeFromTop(kRowHeight));
    audioPlayer_.setBounds(bounds.removeFromTop(kRowHeight));
    bounds.removeFromTop(kGap);

    auto selectionBoxRow = bounds.removeFromTop(kRowHeight);

    juce::FlexBox flexBox;
    flexBox.flexDirection = juce::FlexBox::Direction::row;
    flexBox.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    flexBox.alignItems = juce::FlexBox::AlignItems::center;

    const float kHalfGap = kGap / 2.0f;
    const float kDropdownWidth = 178.0f;
    const float kTooltipWidth = 24.0f;
    flexBox.items.add(
        juce::FlexItem(playbackDevice_)
            .withMinWidth(kDropdownWidth)
            .withHeight(kRowHeight)
            .withMargin(juce::FlexItem::Margin(0, kHalfGap, 0, 0)));
    flexBox.items.add(
        juce::FlexItem(layoutToDecode_)
            .withMinWidth(kDropdownWidth)
            .withHeight(kRowHeight)
            .withMargin(juce::FlexItem::Margin(0, kHalfGap, 0, kHalfGap)));
    flexBox.items.add(juce::FlexItem(decodeToolTip_)
                          .withMinWidth(kTooltipWidth)
                          .withHeight(kRowHeight)
                          .withMargin(juce::FlexItem::Margin(
                              kTooltipWidth, kHalfGap, 0, kHalfGap)));
    flexBox.performLayout(selectionBoxRow);
  }

 private:
  class SVGToolTip : public juce::Component,
                     public juce::SettableTooltipClient {
   public:
    SVGToolTip(const SvgMap::Icon icon, const juce::String& tooltipText)
        : iconComponent_(icon) {
      setTooltip(tooltipText);
      addAndMakeVisible(iconComponent_);
    }

    void resized() override {
      auto bounds = getLocalBounds();
      // Scale icon to a percentage of available space
      int iconSize = juce::jmin(bounds.getWidth(), bounds.getHeight());
      auto iconBounds = juce::Rectangle<int>(iconSize, iconSize);
      iconBounds = iconBounds.withCentre(bounds.getCentre());
      iconComponent_.setBounds(iconBounds);
    }

   private:
    SvgIconComponent iconComponent_;
  };

  void populatePlaybackDevices() {
    // Initialize device manager temporarily to query devices
    juce::AudioDeviceManager deviceManager;
    deviceManager.initialiseWithDefaultDevices(0, 2);

    // Get available device types
    auto& deviceTypes = deviceManager.getAvailableDeviceTypes();

    for (auto* deviceType : deviceTypes) {
      deviceType->scanForDevices();
      auto deviceNames =
          deviceType->getDeviceNames(false);  // false = output devices

      for (const auto& deviceName : deviceNames) {
        playbackDevice_.addOption(deviceName);
        deviceNames_.push_back(deviceName);
      }
    }

    // Set default device if available
    if (!deviceNames_.empty()) {
      playbackDevice_.setSelectedIndex(0, juce::dontSendNotification);
      auto fpb = fpbr_.get();
      fpb.setPlaybackDevice(deviceNames_[0]);
      fpbr_.update(fpb);
    }
  }

  void valueTreePropertyChanged(juce::ValueTree& tree,
                                const juce::Identifier& property) {
    const FilePlayback::CurrentPlayerState kState = fpbr_.get().getPlayState();
    // UI is visible but not interactive during buffering
    if (kState == FilePlayback::kBuffering ||
        kState == FilePlayback::kDisabled) {
      audioPlayer_.setVisible(true);
      playbackDevice_.setVisible(true);
      layoutToDecode_.setVisible(true);
      decodeToolTip_.setVisible(true);
      playbackDevice_.setInterceptsMouseClicks(false, false);
      layoutToDecode_.setInterceptsMouseClicks(false, false);
      audioPlayer_.setInterceptsMouseClicks(false, false);
    }
    // When a file gets selected or play/pause/stop/seek - remain visible
    else if (property == FilePlayback::kPlaybackFile ||
             property == FilePlayback::kPlayState) {
      audioPlayer_.setVisible(true);
      playbackDevice_.setVisible(true);
      layoutToDecode_.setVisible(true);
      decodeToolTip_.setVisible(true);
      playbackDevice_.setInterceptsMouseClicks(true, true);
      layoutToDecode_.setInterceptsMouseClicks(true, true);
      audioPlayer_.setInterceptsMouseClicks(true, true);
    }
    // Disabled/other states are irrelevant to the components
    else {
      audioPlayer_.setVisible(true);
      playbackDevice_.setVisible(true);
      layoutToDecode_.setVisible(true);
      decodeToolTip_.setVisible(true);
    }
  }

  const std::array<Speakers::AudioElementSpeakerLayout, 9> kLayouts{
      Speakers::kStereo,
      Speakers::k3Point1Point2,
      Speakers::k5Point1,
      Speakers::k5Point1Point2,
      Speakers::k5Point1Point4,
      Speakers::k7Point1,
      Speakers::k7Point1Point2,
      Speakers::k7Point1Point4,
      Speakers::kExpl9Point1Point6,
  };

  FilePlaybackRepository& fpbr_;
  juce::Label title_;
  SelectionBox playbackDevice_;
  SelectionBox layoutToDecode_;
  SVGToolTip decodeToolTip_;
  AudioFilePlayer audioPlayer_;
  std::vector<juce::String> deviceNames_;
  juce::TooltipWindow tooltipWindow_{this};
};