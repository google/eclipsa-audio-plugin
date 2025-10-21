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

#include "FileExportScreen.h"

#include "../RendererProcessor.h"
#include "components/src/EclipsaColours.h"
#include "data_structures/src/FileExport.h"
#include "data_structures/src/MixPresentation.h"

FileExportScreen::FileExportScreen(MainEditor& editor,
                                   RepositoryCollection repos)
    : headerBar_("Export options", editor),
      startTimer_("Start"),
      endTimer_("End"),
      formatSelector_("Format"),
      codecSelector_("Codec"),
      bitDepthSelector_("Bit depth"),
      sampleRate_("Sample rate"),
      customCodecParameter_("Codec Param"),
      mixPresentations_("Mix presentations"),
      audioElements_("Audio elements"),
      exportAudioLabel_("ExportAudioLbl", "Export audio"),
      exportPath_("Save audio to ..."),
      exportAudioElementsLabel_("ExportAudioElementsLbl",
                                "Export audio elements as WAV"),
      muxVidoeLabel_("MuxVideoLbl", "Mux video"),
      exportVideoFolder_("Save video to ..."),
      videoSource_("Video source"),
      audioOutputSelect_(
          "Select a file to export audio to",
          juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
          "*.iamf;*.wav"),
      muxVideoSourceSelect_(
          "Select a video file to mux",
          juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
          "*.mp4;*.mov"),
      muxVideoOutputSelect_(
          "Select a file to output mux video to",
          juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
          "*.mp4;*.mov"),
      exportButton_("Start Export"),
      repository_(&repos.fioRepo_),
      aeRepository_(&repos.aeRepo_),
      mpRepository_(&repos.mpRepo_) {
  // Setup listeners to know when to redraw the screen
  aeRepository_->registerListener(this);
  mpRepository_->registerListener(this);
  repository_->registerListener(this);

  // Fetch the current configuration for setting up the screen
  FileExport config = repository_->get();

  // Set the label colours and fonts
  juce::Colour textColour = juce::Colour(221, 228, 227);
  exportAudioElementsLabel_.setColour(juce::Label::textColourId, textColour);
  exportAudioLabel_.setColour(juce::Label::textColourId, textColour);
  muxVidoeLabel_.setColour(juce::Label::textColourId, textColour);
  startTimerErrorLabel_.setColour(juce::Label::ColourIds::textColourId,
                                  EclipsaColours::red);
  endTimerErrorLabel_.setColour(juce::Label::ColourIds::textColourId,
                                EclipsaColours::red);
  customCodecParameterErrorLabel_.setColour(
      juce::Label::ColourIds::textColourId, EclipsaColours::red);
  exportButton_.setColour(juce::TextButton::textColourOffId,
                          EclipsaColours::green);

  juce::Font textFont = juce::Font("Roboto", 22.0f, juce::Font::plain);
  exportAudioElementsLabel_.setFont(
      juce::Font("Roboto", 16.0f, juce::Font::plain));
  exportAudioLabel_.setFont(textFont);
  muxVidoeLabel_.setFont(textFont);
  startTimerErrorLabel_.setFont(juce::Font("Roboto", 12.0f, juce::Font::plain));
  endTimerErrorLabel_.setFont(juce::Font("Roboto", 12.0f, juce::Font::plain));
  startTimerErrorLabel_.setJustificationType(juce::Justification::topLeft);
  endTimerErrorLabel_.setJustificationType(juce::Justification::topLeft);
  customCodecParameterErrorLabel_.setFont(
      juce::Font("Roboto", 12.0f, juce::Font::plain));
  customCodecParameterErrorLabel_.setJustificationType(
      juce::Justification::topLeft);

  // Set the error labels
  startTimerErrorLabel_.setText("",
                                juce::NotificationType::dontSendNotification);
  endTimerErrorLabel_.setText("", juce::NotificationType::dontSendNotification);

  // Set the checkbox colours
  exportAudioElementsToggle_.setColour(
      juce::ToggleButton::tickColourId,
      EclipsaColours::buttonRolloverTextColour);

  // Set the image button images
  juce::Image folderImage = IconStore::getInstance().getFolderIcon();
  browseButton_.setImages(false, true, true, folderImage, 1.0f,
                          juce::Colours::transparentBlack, folderImage, 0.5f,
                          juce::Colours::grey, folderImage, 0.8f,
                          juce::Colours::white);
  browseVideoButton_.setImages(false, true, true, folderImage, 1.0f,
                               juce::Colours::transparentBlack, folderImage,
                               0.5f, juce::Colours::grey, folderImage, 0.8f,
                               juce::Colours::white);
  browseVideoSourceButton_.setImages(false, true, true, folderImage, 1.0f,
                                     juce::Colours::transparentBlack,
                                     folderImage, 0.5f, juce::Colours::grey,
                                     folderImage, 0.8f, juce::Colours::white);

  // Add the format options
  formatSelector_.addOption("IAMF");
  formatSelector_.addOption("WAV");
  formatSelector_.setOption(
      config.getAudioFileFormat() == AudioFileFormat::IAMF ? "IAMF" : "WAV");
  formatSelector_.onChange([this] {
    FileExport config = repository_->get();
    if (formatSelector_.getSelectedIndex() == 0) {
      config.setAudioFileFormat(AudioFileFormat::IAMF);
    } else {
      config.setAudioFileFormat(AudioFileFormat::WAV);
    }
    repository_->update(config);
  });

  // Add the codec options
  codecSelector_.addOption("LPCM");
  codecSelector_.addOption("FLAC");
  codecSelector_.addOption("OPUS");
  codecSelector_.setSelectedIndex((int)config.getAudioCodec(),
                                  juce::NotificationType::dontSendNotification);
  codecSelector_.onChange([this] {
    FileExport config = repository_->get();
    int selectedIndex = codecSelector_.getSelectedIndex();

    // Only allow Opus (index 2) if sample rate is 48kHz
    if (selectedIndex == 2 && config.getSampleRate() != 48000) {
      // Revert to previous selection
      codecSelector_.setSelectedIndex(
          (int)config.getAudioCodec(),
          juce::NotificationType::dontSendNotification);
      return;
    }

    config.setAudioCodec((AudioCodec)codecSelector_.getSelectedIndex());
    configureCustomCodecParameter(config.getAudioCodec());
    repository_->update(config);
  });

  // Add the custom parameter options
  configureCustomCodecParameter(config.getAudioCodec());

  // Add the bit depth options
  bitDepthSelector_.addOption("16 bit");
  bitDepthSelector_.addOption("24 bit");
  bitDepthSelector_.setOption(config.getBitDepth() == 16 ? "16 bit" : "24 bit");
  bitDepthSelector_.onChange([this] {
    FileExport config = repository_->get();
    config.setBitDepth(bitDepthSelector_.getSelectedIndex() == 0 ? 16 : 24);
    repository_->update(config);
  });

  // Configure the export audio file selection button
  browseButton_.onClick = [this] {
    audioOutputSelect_.launchAsync(
        juce::FileBrowserComponent::saveMode |
            juce::FileBrowserComponent::canSelectFiles,
        [this](const auto& file) {
          exportPath_.setText(file.getResult().getFullPathName());
          FileExport config = repository_->get();
          config.setExportFile(file.getResult().getFullPathName());
          config.setExportFolder(
              file.getResult().getParentDirectory().getFullPathName());
          repository_->update(config);
        });
  };
  exportPath_.setText(config.getExportFile());

  // Configure the export video folder
  browseVideoButton_.onClick = [this] {
    muxVideoOutputSelect_.launchAsync(
        juce::FileBrowserComponent::saveMode |
            juce::FileBrowserComponent::canSelectFiles,
        [this](const auto& file) {
          exportVideoFolder_.setText(file.getResult().getFullPathName());
          FileExport config = repository_->get();
          config.setVideoExportFolder(file.getResult().getFullPathName());
          repository_->update(config);
        });
  };
  exportVideoFolder_.setText(config.getVideoExportFolder());

  // Configure the video source file
  browseVideoSourceButton_.onClick = [this] {
    muxVideoSourceSelect_.launchAsync(
        juce::FileBrowserComponent::openMode |
            juce::FileBrowserComponent::canSelectFiles,
        [this](const auto& file) {
          videoSource_.setText(file.getResult().getFullPathName());
          FileExport config = repository_->get();
          config.setVideoSource(file.getResult().getFullPathName());
          repository_->update(config);
        });
  };
  videoSource_.setText(config.getVideoSource());

  exportAudioElementsToggle_.setToggleState(
      config.getExportAudioElements(),
      juce::NotificationType::dontSendNotification);
  exportAudioElementsToggle_.onClick = [this] {
    FileExport config = repository_->get();
    bool toggleState = exportAudioElementsToggle_.getToggleState();
    config.setExportAudioElements(exportAudioElementsToggle_.getToggleState());
    repository_->update(config);
  };

  muxVideoToggle_.setToggleState(config.getExportVideo(),
                                 juce::NotificationType::dontSendNotification);
  muxVideoToggle_.onClick = [this] {
    FileExport config = repository_->get();
    config.setExportVideo(muxVideoToggle_.getToggleState());
    repository_->update(config);
  };

  enableFileExport_.setToggleState(
      config.getExportAudio(), juce::NotificationType::dontSendNotification);
  enableFileExport_.onClick = [this] {
    FileExport config = repository_->get();
    config.setExportAudio(enableFileExport_.getToggleState());
    repository_->update(config);
  };

  // Set the start and end time
  startTimer_.setText(timeToString(config.getStartTime()));
  startTimer_.onTextChanged([this] {
    int startTime = stringToTime(startTimer_.getText());
    if (startTime < 0) {
      startTimerErrorLabel_.setText(
          "Invalid time format", juce::NotificationType::dontSendNotification);
      return;
    }
    startTimerErrorLabel_.setText("",
                                  juce::NotificationType::dontSendNotification);
    FileExport config = repository_->get();
    config.setStartTime(stringToTime(startTimer_.getText()));
    repository_->update(config);
  });
  endTimer_.setText(timeToString(config.getEndTime()));
  endTimer_.onTextChanged([this] {
    int endTime = stringToTime(endTimer_.getText());
    if (endTime < 0) {
      endTimerErrorLabel_.setText("Invalid time format",
                                  juce::NotificationType::dontSendNotification);
      return;
    }
    endTimerErrorLabel_.setText("",
                                juce::NotificationType::dontSendNotification);
    FileExport config = repository_->get();
    config.setEndTime(stringToTime(endTimer_.getText()));
    repository_->update(config);
  });

  // Finally, set up the manual export button
  exportButton_.onClick = [this] {
    FileExport config = repository_->get();
    if (juce::PluginHostType().isPremiere() && !validFileExportConfig(config)) {
      return;
    }
    config.setManualExport(!config.getManualExport());
    repository_->update(config);
    if (config.getManualExport()) {
      startTimer_.setEnabled(false);
      endTimer_.setEnabled(false);
      formatSelector_.setEnabled(false);
      codecSelector_.setEnabled(false);
      bitDepthSelector_.setEnabled(false);
      enableFileExport_.setEnabled(false);
      browseButton_.setEnabled(false);
      exportPath_.setEnabled(false);
      exportAudioElementsToggle_.setEnabled(false);
      muxVideoToggle_.setEnabled(false);
      videoSource_.setEnabled(false);
      exportVideoFolder_.setEnabled(false);
      browseVideoButton_.setEnabled(false);
      browseVideoSourceButton_.setEnabled(false);

    } else {
      startTimer_.setEnabled(true);
      endTimer_.setEnabled(true);
      formatSelector_.setEnabled(true);
      codecSelector_.setEnabled(true);
      bitDepthSelector_.setEnabled(true);
      enableFileExport_.setEnabled(true);
      browseButton_.setEnabled(true);
      exportPath_.setEnabled(true);
      exportAudioElementsToggle_.setEnabled(true);
      muxVideoToggle_.setEnabled(true);
      videoSource_.setEnabled(true);
      exportVideoFolder_.setEnabled(true);
      browseVideoButton_.setEnabled(true);
      browseVideoSourceButton_.setEnabled(true);
    }
    repaint();
  };
  LOG_ANALYTICS(RendererProcessor::instanceId_, "FileExportScreen initiated.");

  // Redraw the non-configurable components
  refreshComponents();

  addAndMakeVisible(warningLabel_);
  warningLabel_.setColour(juce::Label::ColourIds::textColourId,
                          EclipsaColours::red);
  refreshFileExportComponents();
}

FileExportScreen::~FileExportScreen() {
  setLookAndFeel(nullptr);
  aeRepository_->deregisterListener(this);
  mpRepository_->deregisterListener(this);
  repository_->deregisterListener(this);
}

void FileExportScreen::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds();
  /*
   * ==============================
   * Draw in the title bar row
   *===============================
   */
  auto titleBarBounds = bounds.removeFromTop(45);
  addAndMakeVisible(headerBar_);
  headerBar_.setBounds(titleBarBounds);

  // Add some padding
  bounds.removeFromTop(25);

  /* ================================
   *  Draw in the left side selectors
   * ================================
   */
  int rowHeight = 65;
  int rowPadding = 25;
  int columnPadding = 25;
  int componentWidth = 175;
  auto leftSideBounds = bounds.removeFromLeft(450);
  leftSideBounds.removeFromLeft(rowHeight);

  // Draw in the start and end row
  auto row = leftSideBounds.removeFromTop(rowHeight);
  addAndMakeVisible(startTimer_);
  startTimer_.setBounds(row.removeFromLeft(componentWidth));
  row.removeFromLeft(rowPadding);
  addAndMakeVisible(endTimer_);
  endTimer_.setBounds(row.removeFromLeft(componentWidth));

  // Add the error labels
  leftSideBounds.removeFromTop(2);
  row = leftSideBounds.removeFromTop(columnPadding - 2);
  addAndMakeVisible(startTimerErrorLabel_);
  startTimerErrorLabel_.setBounds(row.removeFromLeft(componentWidth));
  row.removeFromLeft(rowPadding);
  addAndMakeVisible(endTimerErrorLabel_);
  endTimerErrorLabel_.setBounds(row.removeFromLeft(componentWidth));

  // Draw in the format and codec row
  leftSideBounds.removeFromTop(columnPadding);
  row = leftSideBounds.removeFromTop(rowHeight);
  addAndMakeVisible(formatSelector_);
  formatSelector_.setBounds(row.removeFromLeft(componentWidth));
  row.removeFromLeft(rowPadding);
  addAndMakeVisible(codecSelector_);
  codecSelector_.setBounds(row.removeFromLeft(componentWidth));

  // Draw in the bit depth and sample rate row
  leftSideBounds.removeFromTop(columnPadding);
  row = leftSideBounds.removeFromTop(rowHeight);
  addAndMakeVisible(bitDepthSelector_);
  bitDepthSelector_.setBounds(row.removeFromLeft(componentWidth));
  row.removeFromLeft(rowPadding);
  addAndMakeVisible(sampleRate_);
  sampleRate_.setBounds(row.removeFromLeft(componentWidth));

  // Draw in the custom codec parameter
  leftSideBounds.removeFromTop(columnPadding);
  row = leftSideBounds.removeFromTop(rowHeight);
  addAndMakeVisible(customCodecParameter_);
  customCodecParameter_.setBounds(row.removeFromLeft(componentWidth));

  // Draw in the custom codec parameter error label
  leftSideBounds.removeFromTop(2);
  row = leftSideBounds.removeFromTop(columnPadding - 2);
  addAndMakeVisible(customCodecParameterErrorLabel_);
  customCodecParameterErrorLabel_.setBounds(row.removeFromLeft(componentWidth));

  // Draw in the mix presentation and Audio Elements row
  leftSideBounds.removeFromTop(columnPadding);
  row = leftSideBounds.removeFromTop(rowHeight);
  addAndMakeVisible(mixPresentations_);
  mixPresentations_.setBounds(row.removeFromLeft(componentWidth));
  row.removeFromLeft(rowPadding);
  addAndMakeVisible(audioElements_);
  audioElements_.setBounds(row.removeFromLeft(componentWidth));

  /* ==============================================
   *  Draw in the right side file selection options
   * ==============================================
   */
  rowHeight = 65;
  rowPadding = 25;
  columnPadding = 25;
  componentWidth = 400;

  // Add some padding
  bounds.removeFromLeft(150);

  // Add the export audio components
  row = bounds.removeFromTop(rowHeight);
  addAndMakeVisible(exportAudioLabel_);
  exportAudioLabel_.setBounds(row.removeFromLeft(150));
  addAndMakeVisible(enableFileExport_);
  enableFileExport_.setBounds(row.removeFromLeft(85).reduced(15));

  row = bounds.removeFromTop(rowHeight);
  addAndMakeVisible(exportPath_);
  exportPath_.setBounds(row.removeFromLeft(componentWidth));
  addAndMakeVisible(browseButton_);
  browseButton_.setBounds(
      row.removeFromLeft(75).withTrimmedTop(10).reduced(20));

  row = bounds.removeFromTop(rowHeight);
  addAndMakeVisible(exportAudioElementsToggle_);
  exportAudioElementsToggle_.setBounds(row.removeFromLeft(50));
  addAndMakeVisible(exportAudioElementsLabel_);
  exportAudioElementsLabel_.setBounds(row.removeFromLeft(componentWidth));

  // Only draw video export options if the audio export is enabled.
  if (enableFileExport_.getToggleState()) {
    // Add the mux video components
    bounds.removeFromTop(columnPadding);
    row = bounds.removeFromTop(rowHeight);
    addAndMakeVisible(muxVidoeLabel_);
    muxVidoeLabel_.setBounds(row.removeFromLeft(130));
    addAndMakeVisible(muxVideoToggle_);
    muxVideoToggle_.setBounds(row.removeFromLeft(85).reduced(15));

    row = bounds.removeFromTop(rowHeight);
    addAndMakeVisible(videoSource_);
    videoSource_.setBounds(row.removeFromLeft(componentWidth));
    addAndMakeVisible(browseVideoSourceButton_);
    browseVideoSourceButton_.setBounds(
        row.removeFromLeft(75).withTrimmedTop(10).reduced(20));

    row = bounds.removeFromTop(rowHeight);
    addAndMakeVisible(exportVideoFolder_);
    exportVideoFolder_.setBounds(row.removeFromLeft(componentWidth));
    addAndMakeVisible(browseVideoButton_);
    browseVideoButton_.setBounds(
        row.removeFromLeft(75).withTrimmedTop(10).reduced(20));
  }
  // Hide video export/mux options.
  else {
    muxVideoToggle_.setToggleState(false, true);
    muxVidoeLabel_.setVisible(false);
    muxVideoToggle_.setVisible(false);
    videoSource_.setVisible(false);
    browseVideoSourceButton_.setVisible(false);
    exportVideoFolder_.setVisible(false);
    browseVideoButton_.setVisible(false);
  }

  // Draw in the manual export button
  if (juce::PluginHostType().isPremiere()) {
    bounds.removeFromTop(columnPadding);
    row = bounds.removeFromTop(rowHeight * 0.75f);
    const auto rowReference = row;
    addAndMakeVisible(exportButton_);
    exportButton_.setBounds(row.removeFromLeft(125));
    auto labelBounds = rowReference;
    labelBounds.removeFromLeft(exportButton_.getWidth());
    warningLabel_.setBounds(labelBounds);
  } else {
#if JUCE_DEBUG
    bounds.removeFromTop(columnPadding);
    row = bounds.removeFromTop(rowHeight);
    addAndMakeVisible(exportButton_);
    exportButton_.setBounds(row.removeFromLeft(200));
#endif
  }
};

juce::String FileExportScreen::timeToString(int timeInSeconds) {
  int seconds = timeInSeconds;
  int minutes = seconds / 60;
  seconds = seconds % 60;
  int hours = minutes / 60;
  minutes = minutes % 60;

  // Pad the hours with a 0 if necessary
  juce::String hourString =
      (hours < 10) ? "0" + juce::String(hours) : juce::String(hours);
  juce::String minuteString =
      (minutes < 10) ? "0" + juce::String(minutes) : juce::String(minutes);
  juce::String secondString =
      (seconds < 10) ? "0" + juce::String(seconds) : juce::String(seconds);

  return hourString + ":" + minuteString + ":" + secondString;
}

int FileExportScreen::stringToTime(juce::String val) {
  auto parts = juce::StringArray::fromTokens(val, ":", "");
  if (parts.size() != 3) {
    return -1;
  }
  if (!parts[0].containsOnly("0123456789") ||
      !parts[1].containsOnly("0123456789") ||
      !parts[2].containsOnly("0123456789")) {
    return -1;
  }

  int hours = parts[0].getIntValue();
  int minutes = parts[1].getIntValue();
  int seconds = parts[2].getIntValue();
  return (hours * 3600 + minutes * 60 + seconds);
}

void FileExportScreen::refreshComponents() {
  // Set the audio element count
  audioElements_.setText(juce::String(aeRepository_->getItemCount()));

  // Set the mix presentation count
  mixPresentations_.setText(juce::String(mpRepository_->getItemCount()));

  repaint();
}

void FileExportScreen::configureCustomCodecParameter(AudioCodec format) {
  customCodecParameter_.setVisible(true);

  // Avoid calling the callback while we change the parameter value
  customCodecParameter_.onTextChanged([] {});

  // Set the text for the custom parameter
  FileExport config = repository_->get();
  if (format == AudioCodec::OPUS) {
    customCodecParameter_.setTitle("Per Channel Bitrate (kbps)");
    customCodecParameter_.setText(
        juce::String(config.getOpusTotalBitrate() / 1000));
  } else if (format == AudioCodec::FLAC) {
    customCodecParameter_.setTitle("Compression level");
    customCodecParameter_.setText(
        juce::String(config.getFlacCompressionLevel()));
  } else if (format == AudioCodec::LPCM) {
    customCodecParameter_.setTitle("Sample size");
    customCodecParameter_.setText(juce::String(config.getLPCMSampleSize()));
  } else {
    customCodecParameter_.setVisible(false);
  }

  // Set up the callback for the custom parameter
  customCodecParameter_.onTextChanged([this, format] {
    // Verify the value is valid
    juce::String text = customCodecParameter_.getText();
    if (!text.containsOnly("0123456789") || text.isEmpty()) {
      customCodecParameterErrorLabel_.setText(
          "Invalid value", juce::NotificationType::dontSendNotification);
      return;
    } else {
      customCodecParameterErrorLabel_.setText(
          "", juce::NotificationType::dontSendNotification);
    }
    int value = text.getIntValue();
    FileExport config = repository_->get();
    if (format == AudioCodec::OPUS) {
      if (value < 6 || value > 256) {
        customCodecParameterErrorLabel_.setText(
            "Value must be between 6-256",
            juce::NotificationType::dontSendNotification);
        return;
      }
      config.setOpusTotalBitrate(value * 1000);
    } else if (format == AudioCodec::FLAC) {
      if (value > 16) {
        customCodecParameterErrorLabel_.setText(
            "Value must be 16 or less",
            juce::NotificationType::dontSendNotification);
        return;
      }
      config.setFlacCompressionLevel(value);
    } else if (format == AudioCodec::LPCM) {
      if (!(value == 16 || value == 24 || value == 32)) {
        customCodecParameterErrorLabel_.setText(
            "LPCM sample size must be 16, 24, or 32",
            juce::NotificationType::dontSendNotification);
        return;
      }
      config.setLPCMSampleSize(value);
    }
    repository_->update(config);
  });
}

void FileExportScreen::valueTreeRedirected(
    juce::ValueTree& treeWhichHasBeenChanged) {
  if (treeWhichHasBeenChanged.getType() == repository_->getTree().getType()) {
    refreshFileExportComponents();
  } else {
    refreshComponents();
  }
}

void FileExportScreen::valueTreePropertyChanged(
    juce::ValueTree& treeWhosePropertyHasChanged,
    const juce::Identifier& property) {
  if (treeWhosePropertyHasChanged.getType() ==
      repository_->getTree().getType()) {
    refreshFileExportComponents();
  } else {
    refreshComponents();
  }
}

void FileExportScreen::valueTreeChildAdded(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) {
  if (childWhichHasBeenAdded.getType() == repository_->getTree().getType()) {
    refreshFileExportComponents();
  } else {
    refreshComponents();
  }
}

void FileExportScreen::valueTreeChildRemoved(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved,
    int indexFromWhichChildWasRemoved) {
  if (childWhichHasBeenRemoved.getType() == repository_->getTree().getType()) {
    refreshFileExportComponents();
  } else {
    refreshComponents();
  }
}

void FileExportScreen::refreshFileExportComponents() {
  // Set the sample rate information if possible
  FileExport config = repository_->get();
  if (config.getSampleRate() > 0) {
    sampleRate_.setText(juce::String(config.getSampleRate()) + " Hz");
  }

  // Only allow Opus codec if sample rate is 48kHz
  bool opusAllowed = (config.getSampleRate() == 48000);
  const juce::ComboBox* comboBox = codecSelector_.getComboBox();
  if (comboBox != nullptr) {
    // OPUS is item ID 3 (LPCM=1, FLAC=2, OPUS=3)
    const_cast<juce::ComboBox*>(comboBox)->setItemEnabled(3, opusAllowed);
  }

  // If Opus is currently selected but not allowed, switch to LPCM
  if (!opusAllowed && config.getAudioCodec() == AudioCodec::OPUS) {
    config.setAudioCodec(AudioCodec::LPCM);
    codecSelector_.setSelectedIndex(
        (int)AudioCodec::LPCM, juce::NotificationType::dontSendNotification);
    configureCustomCodecParameter(AudioCodec::LPCM);
    repository_->update(config);
  }

  if (config.getManualExport()) {
    exportButton_.setButtonText("Stop Export");
    exportButton_.setColour(juce::TextButton::ColourIds::textColourOffId,
                            EclipsaColours::red);
  } else {
    exportButton_.setButtonText("Start Export");
    exportButton_.setColour(juce::TextButton::textColourOffId,
                            EclipsaColours::green);
  }

  repaint();
}

bool FileExportScreen::validFileExportConfig(const FileExport& config) {
  // Check if the export file is valid
  if (config.getExportFile().isEmpty()) {
    warningLabel_.setVisible(true);
    warningLabel_.setText("Must Specify a .IAMF File to Export",
                          juce::NotificationType::dontSendNotification);
    return false;
  }

  juce::OwnedArray<MixPresentation> mixPresentations;
  mpRepository_->getAll(mixPresentations);

  for (const auto mixPres : mixPresentations) {
    if (mixPres->getAudioElements().size() == 0) {
      warningLabel_.setVisible(true);
      juce::Uuid mixId = mixPres->getId();
      juce::String mixName = mpRepository_->get(mixId).value().getName();
      warningLabel_.setText("No audio elements in mix presentation: " + mixName,
                            juce::NotificationType::dontSendNotification);
      return false;
    }
  }

  warningLabel_.setVisible(false);

  return true;
}