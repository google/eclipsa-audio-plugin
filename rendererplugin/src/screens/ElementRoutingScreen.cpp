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

#include "ElementRoutingScreen.h"

#include <vector>

#include "../RendererProcessor.h"
#include "components/src/EclipsaColours.h"
#include "components/src/Icons.h"
#include "data_repository/implementation/AudioElementSpatialLayoutRepository.h"
#include "data_repository/implementation/FileExportRepository.h"
#include "data_structures/src/AudioElement.h"
#include "data_structures/src/AudioElementSpatialLayout.h"
#include "data_structures/src/FileExport.h"
#include "logger/logger.h"
#include "processors/processor_base/ProcessorBase.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

ElementRoutingScreen::ElementRoutingScreen(
    MainEditor& editor, AudioElementRepository* audioElementRepository,
    MultibaseAudioElementSpatialLayoutRepository*
        AudioElementSpatialLayoutRepository,
    FileExportRepository* fileExportRepository,
    MixPresentationRepository* mixPresentationRepository, int totalChanneCount)
    : audioElementRepository_(audioElementRepository),
      audioElementSpatialLayoutRepository_(AudioElementSpatialLayoutRepository),
      fileExportRepository_(fileExportRepository),
      mixPresentationRepository_(mixPresentationRepository),
      headerBar_("Routing", editor),
      profileSelectionBox_("Profile"),
      addAudioElementButton_("+ Add Audio Element", this),
      audioElementViewport_(&pannerAudioElementTableViewport_),
      kLayoutsRef_([]() {
        std::vector<Speakers::AudioElementSpeakerLayout> layouts(
            (Speakers::lastExpandedLayout - Speakers::firstStandardLayout) + 1);
        for (int i = Speakers::firstStandardLayout;
             i <= Speakers::lastExpandedLayout; ++i) {
          layouts[i - Speakers::firstStandardLayout] =
              Speakers::AudioElementSpeakerLayout(i);
        }
        return layouts;
      }()) {
  setLookAndFeel(&lookAndFeel_);

  // Set profile selection
  FileExport profileConfig = fileExportRepository_->get();
  profileSelectionBox_.addOption("Simple");

  // Add the base option to the profile config only if 18 channels are
  // available, as the base profile supports 18 channels
  if (totalChanneCount >= 18) {
    profileSelectionBox_.addOption("Base");
  }
  // Add the base option to the profile config only if 28 channels are
  // available, as the base enhanced profile supports 28 channels
  if (totalChanneCount >= 28) {
    profileSelectionBox_.addOption("Base Enhanced");
  }
  profileSelectionBox_.onChange([this]() {
    FileExport profileConfig = fileExportRepository_->get();
    int idx = profileSelectionBox_.getSelectedIndex();
    currentProfile_ = static_cast<FileProfile>(idx);
    profileConfig.setProfile(currentProfile_);
    fileExportRepository_->update(profileConfig);
    LOG_ANALYTICS(RendererProcessor::instanceId_,
                  "Profile changed to: " + std::to_string(idx));
    updateAudioElementChannels();
  });

  profileSelectionBox_.setSelectedIndex(
      (int)profileConfig.getProfile(),
      juce::NotificationType::dontSendNotification);

  // Initialize current profile
  currentProfile_ = profileConfig.getProfile();

  // Initialize channels in use
  channelsInUse_ = 0;

  // Add the tooltip window. This can only ever be done once
  tooltipWindow_.setMillisecondsBeforeTipAppears(50);
  tooltipWindow_.setColour(juce::TooltipWindow::backgroundColourId,
                           juce::Colours::black);
  addAndMakeVisible(tooltipWindow_);

  // Configure the tooltip image
  tooltipImage_.setImage(IconStore::getInstance().getTooltipIcon());
  tooltipImage_.setTooltip(
      "Profiles\n\n"
      "Simple profile supports up to 1 audio element with a "
      "maximum of 16 channels.\n\n"
      "Base profile supports up to 2 audio elements with a maximum "
      "of 18 channels.\n\n"
      "Base Enhanced profile supports up to 28 audio elements with a "
      "maximum of 28 channels.\n\n"
      "Available audio element layouts are filtered based on remaining "
      "channel capacity and profile limitations.");

  // Update the local rendering with the current audio elements and panners
  updateAudioElementChannels();

  // Add a listener for the addition of new panners
  audioElementSpatialLayoutRepository_->registerListener(this);

  // Tie the horizontal scrollbars together
}

ElementRoutingScreen::~ElementRoutingScreen() {
  setLookAndFeel(nullptr);
  audioElementSpatialLayoutRepository_->deregisterListener(this);
}

void ElementRoutingScreen::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds();
  /*
   * ==============================
   * Draw in the title bar row
   *===============================
   */
  auto titleBarBounds = bounds.removeFromTop(45);
  addAndMakeVisible(headerBar_);
  headerBar_.setBounds(titleBarBounds);

  /*
   * ======================================
   * Draw the channel and profile dropdown in the top left corner
   *=======================================
   */
  auto selectionRowBounds = bounds.removeFromTop(120);

  // Draw the profile selection drop down
  auto profileSelectionBounds = selectionRowBounds.removeFromLeft(200);
  addAndMakeVisible(profileSelectionBox_);
  auto selectionBoxBounds = profileSelectionBounds.removeFromTop(65);
  profileSelectionBox_.setBounds(selectionBoxBounds.removeFromLeft(150));

  addAndMakeVisible(tooltipImage_);
  tooltipImage_.setBounds(
      selectionBoxBounds.removeFromLeft(40).removeFromBottom(40).reduced(10));

  // Draw the remaining channels
  remainingChannelsLabel_.setColour(juce::Label::textColourId,
                                    juce::Colours::white);

  remainingChannelsLabel_.setBounds(
      profileSelectionBounds.removeFromBottom(22));
  addAndMakeVisible(remainingChannelsLabel_);

  selectionRowBounds.removeFromLeft(50);  // Add padding

  /*
   * ======================================
   * Draw the audio element selection row (each column should be 300 px wide)
   * Add each audio element to the audio element viewport so that they can be
   * scrolled horizontally.
   *=======================================
   */

  // First, compute the width of the container containing all audio elements
  // The viewport displays the container, which will be longer than the
  // viewport
  int audioElementContainerWidth = 300 * audioElementColumns_.size() + 250;
  audioElementContainer_.setSize(audioElementContainerWidth, 100);
  audioElementContainer_.clear();

  // Add each audio elements information to the container
  for (const auto& elementColumn : audioElementColumns_) {
    audioElementContainer_.addComponent(elementColumn.get(), 250);
    audioElementContainer_.addComponent(nullptr, 50);  // Add 50px padding
  }

  // Add the Add Audio Element button
  audioElementContainer_.addComponent(&addAudioElementButton_, 200, true);

  // Finally, add the viewport to view the configured audio element container
  audioElementViewport_.setBounds(selectionRowBounds);
  audioElementViewport_.setViewedComponent(&audioElementContainer_, false);
  audioElementViewport_.setScrollBarsShown(false, true);

  // Make the viewport visible on the screen
  addAndMakeVisible(audioElementViewport_);

  /*
   * ======================================
   * Create the panner audio element table containing all panners and there ae
   * associations.
   * This table is stored in the pannerAudioElementTableContainer and viewed
   *by the pannerAudioElementTableViewport which enables horizontal scrolling.
   *=======================================
   */

  // First, configure the container size for the panner audio element table
  pannerAudioElementTableContainer_.setSize(audioElementContainerWidth,
                                            (32 * pannerLabels_.size()) + 32);

  // Add all the panner rows to the container
  pannerAudioElementTableContainer_.clear();
  for (auto& pannerRow : pannerRows_) {
    pannerAudioElementTableContainer_.addComponent(pannerRow.get(), 32);
  }

  // Configure the viewport to display the container
  pannerAudioElementTableViewport_.setViewedComponent(
      &pannerAudioElementTableContainer_, false);

  // Disable the scrollbars since this is a linked view. We will use the
  // scrollbars in the audio element viewport instead
  pannerAudioElementTableViewport_.setScrollBarsShown(false, false);

  /*
   * ======================================
   * Create the component storing all the track names
   * Then, add both this component and the panner audio element tables
   *viewport to the track view container. This container will be viewed by the
   *track vertical viewport which enables vertical scrolling.
   *=======================================
   */
  bounds.removeFromTop(20);  // Add some padding

  // Add the tracks title
  addAndMakeVisible(tracksLabel_);
  tracksLabel_.setText("Tracks", juce::dontSendNotification);
  tracksLabel_.setFont(juce::Font("Roboto", 22.0f, juce::Font::plain));
  auto topBound = bounds.removeFromTop(30);
  tracksLabel_.setBounds(topBound.removeFromLeft(200));

  // Add all track names to the track label container after configuring it's
  // height
  trackLabelContainer_.clear();
  trackViewContainer_.setSize(250, 32 * pannerLabels_.size() + 32);
  for (auto& pannerLabel : pannerLabels_) {
    trackLabelContainer_.addComponent(pannerLabel.get(), 32);
  }

  // Create a vertical container which will hold the track names and the track
  // table viewport and add them both
  trackViewContainer_.clear();
  trackViewContainer_.setSize(bounds.getWidth(),
                              (32 * pannerLabels_.size()) + 32);
  trackViewContainer_.addComponent(&trackLabelContainer_, 250);
  trackViewContainer_.addComponent(&pannerAudioElementTableViewport_, 0);

  // Finally, configure the vertical viewport to display the track view
  // container and enable vertical scrolling
  trackVerticalViewport_.setBounds(bounds);
  trackVerticalViewport_.setViewedComponent(&trackViewContainer_, false);
  trackVerticalViewport_.setScrollBarsShown(true, false);
  auto verticalBounds = trackViewContainer_.getBounds();

  addAndMakeVisible(trackVerticalViewport_);
};

void ElementRoutingScreen::updateAudioElementChannels() {
  // Fetch all the audio elements
  juce::OwnedArray<AudioElement> audioElementArray;
  audioElementRepository_->getAll(audioElementArray);

  juce::OwnedArray<MixPresentation> mixPresentations;
  mixPresentationRepository_->getAll(mixPresentations);

  // Keep track of which audio elements are used in a mix presentation
  std::unordered_set<juce::Uuid> mixPresentationAudioElements;

  for (auto* mixPres : mixPresentations) {
    for (const auto& mixAE : mixPres->getAudioElements()) {
      mixPresentationAudioElements.insert(mixAE.getId());
    }
  }
  // Now, update all their channel information
  // Simulatneosly, update the rendered columns
  audioElementColumns_.clear();
  bool success = true;
  int currentChannelNumber = 0;
  int totalChannels = 0;
  for (auto* audioElement : audioElementArray) {
    audioElement->setFirstChannel(currentChannelNumber);
    audioElementRepository_->update(*audioElement);
    currentChannelNumber += audioElement->getChannelCount();
    audioElementColumns_.push_back(
        std::make_unique<AudioElementColumn>(*audioElement, this));
    totalChannels += audioElement->getChannelCount();
  }

  // Validate the profile selection and revert if necessary
  FileProfile minimumProfile = FileProfileHelper::minimumProfile(
      totalChannels, audioElementArray.size());
  if (minimumProfile > currentProfile_) {
    LOG_ANALYTICS(RendererProcessor::instanceId_,
                  "Profile downgraded to: " +
                      std::to_string(static_cast<int>(minimumProfile)) +
                      " due to channel limits.");
    currentProfile_ = minimumProfile;
    profileSelectionBox_.setSelectedIndex(
        static_cast<int>(currentProfile_),
        juce::NotificationType::sendNotification);
  }

  // Compute how many channels remain and determine if more audio elements
  // can still be added
  channelsInUse_ = totalChannels;

  int remainingChannels =
      FileProfileHelper::profileChannels(currentProfile_) - totalChannels;
  int remainingAudioElements =
      FileProfileHelper::profileAudioElements(currentProfile_) -
      audioElementArray.size();

  remainingChannelsLabel_.setText(
      juce::String(remainingChannels) + " remaining channels",
      juce::NotificationType::dontSendNotification);

  // Update the add audio element button with filtered layouts
  // This will also handle enabling/disabling the button appropriately
  updateAddAudioElementButton(getAudioElementNames(currentProfile_));

  // Update the panner rows
  pannerRows_.clear();
  pannerLabels_.clear();
  juce::OwnedArray<AudioElementSpatialLayout> audioElementSpatialLayouts;
  audioElementSpatialLayoutRepository_->getAll(audioElementSpatialLayouts);
  int idx = 0;
  for (auto* audioElementSpatialLayout : audioElementSpatialLayouts) {
    // Find the index of the audio element in the audio element array
    int audioElementIndex = -1;
    for (int i = 0; i < audioElementArray.size(); i++) {
      if (audioElementArray[i]->getId() ==
          audioElementSpatialLayout->getAudioElementId()) {
        audioElementIndex = i;
        break;
      }
    }

    // Alternate between black and grey
    juce::Colour bgColour = EclipsaColours::backgroundOffBlack;
    if (idx % 2 == 0) {
      bgColour = EclipsaColours::tableAlternateGrey;
    }
    idx++;

    pannerRows_.push_back(std::make_unique<PannerRow>(
        bgColour, audioElementIndex, audioElementArray.size()));
    pannerLabels_.push_back(std::make_unique<PannerLabel>(
        audioElementSpatialLayout->getName(), bgColour));

    // Disable the delete button for audio elements with assigned plugins
    if (audioElementIndex > -1) {
      audioElementColumns_[audioElementIndex]->disableDelete();
    }
  }

  for (int i = 0; i < audioElementArray.size(); i++) {
    if (mixPresentationAudioElements.find(audioElementArray[i]->getId()) !=
        mixPresentationAudioElements.end()) {
      audioElementColumns_[i]->disableDelete();
    }
  }
}

// Called when the add audio button has an item selected
void ElementRoutingScreen::comboBoxChanged(
    juce::ComboBox* comboBoxThatHasChanged) {
  // First, fetch the selected item
  Speakers::AudioElementSpeakerLayout layout =
      getAudioElementLayout(comboBoxThatHasChanged->getText());

  // Verify their are enough available channels / audio elements for the
  // request
  if (channelsInUse_ + layout.getNumChannels() >
          FileProfileHelper::profileChannels(currentProfile_) ||
      audioElementRepository_->getItemCount() >=
          FileProfileHelper::profileAudioElements(currentProfile_)) {
    LOG_ANALYTICS(
        RendererProcessor::instanceId_,
        "Failed to add audio element: Insufficient channels or limit reached.");
    return;
  }

  // Add the relevant audio element
  AudioElement newElement;
  newElement.setName(formatAudioElementName(layout));
  newElement.setChannelConfig(layout);
  newElement.setDescription(layout.toString());
  newElement.setFirstChannel(0);
  audioElementRepository_->add(newElement);
  LOG_ANALYTICS(RendererProcessor::instanceId_,
                "Added audio element: " + layout.toString().toStdString());
  // Update the audio element channels to correctly set all the first channel
  // information with the new audio element
  updateAudioElementChannels();

  repaint();
}

void ElementRoutingScreen::removeAudioElement(juce::Uuid& element) {
  auto ae = audioElementRepository_->get(element);
  if (!ae.has_value()) {
    LOG_ANALYTICS(RendererProcessor::instanceId_,
                  "Failed to remove audio element: Element not found.");
    return;
  }

  LOG_ANALYTICS(RendererProcessor::instanceId_,
                "Removing audio element: " + ae->getName().toStdString());

  audioElementRepository_->remove(element);

  // Update the audio element channels
  updateAudioElementChannels();

  repaint();
}

void ElementRoutingScreen::updateAudioElementName(juce::Uuid& element,
                                                  juce::String name) {
  auto ae = audioElementRepository_->get(element);
  if (!ae.has_value()) {
    LOG_ANALYTICS(RendererProcessor::instanceId_,
                  "Failed to update name: Element not found.");
    return;
  }
  juce::OwnedArray<AudioElement> audioElements;
  audioElementRepository_->getAll(audioElements);
  for (auto audioElement : audioElements) {
    // change the name
    if (audioElement->getName() == name) {
      LOG_ANALYTICS(RendererProcessor::instanceId_,
                    "Failed to update name: Name already exists.");
      return;
    }
  }

  LOG_ANALYTICS(RendererProcessor::instanceId_,
                "Updated audio element name from " +
                    ae->getName().toStdString() + " to " + name.toStdString());

  ae->setName(name);
  audioElementRepository_->update(*ae);
}

juce::StringArray ElementRoutingScreen::getAudioElementNames(
    const FileProfile& profile) {
  juce::StringArray audioElementNames;
  std::vector<Speakers::AudioElementSpeakerLayout> layouts;
  if (profile == FileProfile::SIMPLE || profile == FileProfile::BASE) {
    layouts = {
        Speakers::kMono,     Speakers::kStereo,        Speakers::k3Point1Point2,
        Speakers::k5Point1,  Speakers::k5Point1Point2, Speakers::k5Point1Point4,
        Speakers::k7Point1,  Speakers::k7Point1Point2, Speakers::k7Point1Point4,
        Speakers::kBinaural, Speakers::kHOA1,          Speakers::kHOA2,
        Speakers::kHOA3};
  } else {
    layouts = {Speakers::kMono,
               Speakers::kStereo,
               Speakers::k3Point1Point2,
               Speakers::k5Point1,
               Speakers::k5Point1Point2,
               Speakers::k5Point1Point4,
               Speakers::kExpl5Point1Point4Surround,
               Speakers::k7Point1,
               Speakers::k7Point1Point2,
               Speakers::k7Point1Point4,
               Speakers::kExpl7Point1Point4Front,
               Speakers::kExpl7Point1Point4SideSurround,
               Speakers::kExpl7Point1Point4RearSurround,
               Speakers::kExpl7Point1Point4TopFront,
               Speakers::kExpl7Point1Point4TopBack,
               Speakers::kExpl7Point1Point4Top,
               Speakers::kExpl9Point1Point6,
               Speakers::kExpl9Point1Point6Front,
               Speakers::kExpl9Point1Point6Side,
               Speakers::kExpl9Point1Point6TopSide,
               Speakers::kExpl9Point1Point6Top,
               Speakers::kBinaural,
               Speakers::kHOA1,
               Speakers::kHOA2,
               Speakers::kHOA3};
  }

  // Check remaining audio element count limit
  juce::OwnedArray<AudioElement> audioElementArray;
  audioElementRepository_->getAll(audioElementArray);
  int remainingAudioElements =
      FileProfileHelper::profileAudioElements(currentProfile_) -
      audioElementArray.size();

  // Add all layouts that fit within the audio element count limit
  // Channel constraints will be handled later via disabled options
  for (auto layout : layouts) {
    // Only add if there would be space for this new audio element
    if (remainingAudioElements > 0) {
      audioElementNames.add(layout.toString());
    }
  }

  return audioElementNames;
}

void ElementRoutingScreen::updateAddAudioElementButton(
    const juce::StringArray& audioElementNames) {
  addAudioElementButton_.clear();
  addAudioElementButton_.addItemList(audioElementNames, 1);

  // Get the host wide layout channel count constraint
  int hostWideLayoutChannels = ProcessorBase::getHostWideLayout().size();

  // Get available channels for new audio elements
  int remainingChannels =
      FileProfileHelper::profileChannels(currentProfile_) - channelsInUse_;

  // Apply enabled/disabled state to each option based on constraints
  for (int i = 0; i < audioElementNames.size(); i++) {
    juce::String layoutName = audioElementNames[i];
    auto layout = getAudioElementLayout(layoutName);
    int layoutChannels = layout.getNumChannels();

    // Disable layouts that exceed host wide layout or available channel
    // constraints
    bool enable = true;
    if (hostWideLayoutChannels > 0 && layoutChannels > hostWideLayoutChannels) {
      enable = false;
    } else if (layoutChannels > remainingChannels) {
      enable = false;
    }

    // Use SelectionButton setItemEnabled to disable incompatible options
    // Item IDs start from 1 (as used in addItemList)
    addAudioElementButton_.setItemEnabled(i + 1, enable);
  }

  // Enable/disable the entire button based on whether there are any enabled
  // options
  bool hasEnabledOptions = false;
  for (int i = 0; i < audioElementNames.size(); i++) {
    if (addAudioElementButton_.isItemEnabled(i + 1)) {
      hasEnabledOptions = true;
      break;
    }
  }

  if (!hasEnabledOptions || audioElementNames.isEmpty()) {
    addAudioElementButton_.disable();
    LOG_ANALYTICS(RendererProcessor::instanceId_,
                  "Add Audio Element button disabled: No compatible layouts "
                  "available for host layout (" +
                      juce::String(hostWideLayoutChannels).toStdString() +
                      " channels).");
  } else {
    addAudioElementButton_.enable();
  }
}

Speakers::AudioElementSpeakerLayout ElementRoutingScreen::getAudioElementLayout(
    const juce::String& name) {
  auto index =
      std::find_if(kLayoutsRef_.begin(), kLayoutsRef_.end(), [&](int i) {
        return Speakers::AudioElementSpeakerLayout(i).toString() == name;
      });
  if (index != kLayoutsRef_.end()) {
    return Speakers::AudioElementSpeakerLayout(*index);
  }
  LOG_ERROR(RendererProcessor::instanceId_,
            "ElementRoutingScreen::getAudioElementLayout() : Could not find "
            "layout: " +
                name.toStdString());
  return Speakers::kMono;
}

juce::String ElementRoutingScreen::formatAudioElementName(
    const Speakers::AudioElementSpeakerLayout& layout) {
  juce::OwnedArray<AudioElement> audioElements;
  audioElementRepository_->getAll(audioElements);

  juce::String name = layout.toString();
  int i = 2;

  for (int j = 0; j < audioElements.size(); ++j) {
    auto audioElement = audioElements[j];
    // change the name
    if (audioElement->getName() == name) {
      name = layout.toString() + " " + juce::String(i);
      i++;
      j = -1;  // Reset the index to check for duplicates again
    }
  }

  return name;
}
