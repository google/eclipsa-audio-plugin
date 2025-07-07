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

#include "PresentationMonitorScreen.h"

#include "../RendererProcessor.h"
#include "data_structures/src/ActiveMixPresentation.h"
#include "data_structures/src/MixPresentation.h"
#include "data_structures/src/MixPresentationSoloMute.h"
#include "data_structures/src/RepositoryCollection.h"
#include "logger/logger.h"

CustomTabbedComponent::CustomTabbedComponent()
    : juce::TabbedComponent(juce::TabbedButtonBar::Orientation::TabsAtTop) {}

CustomTabbedComponent::~CustomTabbedComponent() { setLookAndFeel(nullptr); }

void CustomTabbedComponent::currentTabChanged(
    int newCurrentTabIndex, const juce::String& newCurrentTabName) {
  MixPresentationViewPort* tab = static_cast<MixPresentationViewPort*>(
      getTabContentComponent(newCurrentTabIndex));
  if (tab == nullptr) {
    return;
  }

  // Only update the active mix presentation if we're not in tab restoration
  // mode
  if (!isRestoringTabs_) {
    tab->updateActiveMixPresentation();
    LOG_ANALYTICS(RendererProcessor::instanceId_,
                  "Tab changed to: " + newCurrentTabName.toStdString());
  }
}

PresentationMonitorScreen::PresentationMonitorScreen(
    MainEditor& editor, RepositoryCollection repos,
    ChannelMonitorData& channelMonitorData, int totalChannelCount)
    : elementRoutingScreen_(editor, &repos.aeRepo_,
                            &repos.audioElementSpatialLayoutRepo_,
                            &repos.fioRepo_, &repos.mpRepo_, totalChannelCount),
      editPresentationScreen_(editor, &repos.aeRepo_, &repos.mpRepo_,
                              &repos.activeMPRepo_),
      presentationTabs_(std::make_unique<CustomTabbedComponent>()),
      repos_(repos),
      mixPresentationRepository_(&repos.mpRepo_),
      mixPresentationSoloMuteRepository_(&repos.mpSMRepo_),
      multiChannelRepository_(&repos.chGainRepo_),
      audioElementRepository_(&repos.aeRepo_),
      activeMixRepository_(&repos.activeMPRepo_),
      channelMonitorData_(channelMonitorData),
      editPresentationButton(
          IconStore::getInstance()
              .getEditIcon()),  // Initialize with specified sizes
      changeRoutingButton(IconStore::getInstance().getTrackIcon()) {
  // Apply LookAndFeel to buttons
  editPresentationButton.setBlueLookAndFeel();
  changeRoutingButton.setBlueLookAndFeel();
  LOG_ANALYTICS(RendererProcessor::instanceId_,
                " PresentationMonitorScreen created.");

  // Set up the reroute button
  changeRoutingButton.setButtonText("Reroute");
  changeRoutingButton.setButtonOnClick([this, &editor] {
    elementRoutingScreen_.updateAudioElementChannels();
    elementRoutingScreen_.repaint();
    editor.setScreen(elementRoutingScreen_);
  });

  // Set up the edit button
  editPresentationButton.setButtonText("Edit");
  editPresentationButton.setButtonOnClick(
      [this, &editor] { editor.setScreen(editPresentationScreen_); });
  mixPresentationRepository_->registerListener(this);
  activeMixRepository_->registerListener(this);

  // Update mix presentation information
  updateMixPresentations();
  updatePresentationTabs();

  addAndMakeVisible(presentationTabs_.get());

  presentationTabs_->getTabbedButtonBar().setColour(
      juce::TabbedButtonBar::ColourIds::tabTextColourId,
      EclipsaColours::tabTextGrey);
  presentationTabs_->getTabbedButtonBar().setColour(
      juce::TabbedButtonBar::ColourIds::frontTextColourId,
      EclipsaColours::selectCyan);
  presentationTabs_->getTabbedButtonBar().setColour(
      juce::TabbedButtonBar::ColourIds::tabOutlineColourId,
      EclipsaColours::backgroundOffBlack);
  presentationTabs_->getTabbedButtonBar().setColour(
      juce::TabbedButtonBar::ColourIds::frontOutlineColourId,
      EclipsaColours::backgroundOffBlack);
  presentationTabs_->setColour(
      juce::TabbedComponent::ColourIds::backgroundColourId,
      EclipsaColours::backgroundOffBlack);
  presentationTabs_->setColour(
      juce::TabbedComponent::ColourIds::outlineColourId,
      EclipsaColours::backgroundOffBlack);
}

PresentationMonitorScreen::~PresentationMonitorScreen() {
  setLookAndFeel(nullptr);
  mixPresentationRepository_->deregisterListener(this);
  activeMixRepository_->deregisterListener(this);
  presentationTabs_->clearTabs();
}

void PresentationMonitorScreen::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds();  // x=0, y=0, height=311, width=735
  // Split the bounds into the control buttons and the audio element
  // monitoring view
  // AudioElementMonitoringBounds is the left 80% of the width of the screen
  presentationTabBounds_ = bounds.removeFromLeft(bounds.getWidth() * 0.8);

  auto controlButtonsBounds =
      bounds;  // x = 551, y = 0, width = 184, height = 311

  // Add the two control buttons
  addAndMakeVisible(editPresentationButton);
  addAndMakeVisible(changeRoutingButton);

  int buttonHeight = 40;         // Height for both buttons
  int editButtonWidth = 85;      // Specific width for the Edit button
  int rerouteButtonWidth = 125;  // Specific width for the Reroute button
  int padding = 10;              // Padding from the edge of the component

  // Position the Edit button at the top right
  int editButtonX = bounds.getRight() - editButtonWidth -
                    padding;  // X position for the Edit button
  int editButtonY = padding;  // Y position for the Edit button

  // Position the Reroute button at the bottom right
  int rerouteButtonX = bounds.getRight() - rerouteButtonWidth -
                       padding;  // X position for the Reroute button
  int rerouteButtonY = bounds.getBottom() - buttonHeight -
                       padding;  // Y position for the Reroute button

  editPresentationButton.setBounds(editButtonX, editButtonY, editButtonWidth,
                                   buttonHeight);
  changeRoutingButton.setBounds(rerouteButtonX, rerouteButtonY,
                                rerouteButtonWidth, buttonHeight);

  // ensure the tab buttons have the correct bounds
  updateTabButtonBounds(presentationTabBounds_);
};

void PresentationMonitorScreen::updateTabButtonBounds(
    const juce::Rectangle<int>& audioElementMonitoringBounds) {
  // nothing to paint if no mixes
  if (numMixes_ == 0) {
    return;
  }

  presentationTabs_->setBounds(audioElementMonitoringBounds);

  float fraction = 1.f / numMixes_;
  auto tabbedbuttonbarbounds =
      presentationTabs_->getTabbedButtonBar().getBounds();

  tabbedbuttonbarbounds.setWidth(presentationTabs_->getBounds().getWidth());
  const int numTabs = presentationTabs_->getNumTabs();
  for (int i = 0; i < numTabs; i++) {
    juce::TabBarButton* tabButton =
        presentationTabs_->getTabbedButtonBar().getTabButton(i);
    tabButton->setBounds(tabbedbuttonbarbounds.removeFromLeft(
        audioElementMonitoringBounds.getWidth() * fraction));
  }
};

void PresentationMonitorScreen::updateMixPresentations() {
  // Update the mix presentations array and the number of mixes
  mixPresentationRepository_->getAll(mixPresentationArray_);

  numMixes_ = mixPresentationArray_.size();

  // address the case where there is just 1 mix presentation on start up
  // that is added before this component is added as a listener
  // manually add the mixPresentationID to MixPresentataionSoloMuteRepository
  juce::OwnedArray<MixPresentationSoloMute> mixPresSoloMuteArray;
  mixPresentationSoloMuteRepository_->getAll(mixPresSoloMuteArray);
  if (numMixes_ == 1 && mixPresSoloMuteArray.isEmpty()) {
    MixPresentationSoloMute mixPresentationSoloMute(
        mixPresentationArray_[0]->getId(), mixPresentationArray_[0]->getName());
    mixPresentationSoloMuteRepository_->add(mixPresentationSoloMute);
  }
  LOG_ANALYTICS(
      RendererProcessor::instanceId_,
      "Mix presentations updated. Total mixes: " + std::to_string(numMixes_));
}

void PresentationMonitorScreen::updatePresentationTabs() {
  // Set flag to prevent active mix updates during tab restoration
  presentationTabs_->setTabRestorationMode(true);

  // Use RAII to ensure flag is always cleared, even if exceptions occur
  struct RestorationGuard {
    CustomTabbedComponent* tabs;
    ~RestorationGuard() { tabs->setTabRestorationMode(false); }
  } guard{presentationTabs_.get()};

  presentationTabs_->clearTabs();

  // Add tabs for each mix
  for (auto mix : mixPresentationArray_) {
    presentationTabs_->addTab(
        mix->getName(), EclipsaColours::backgroundOffBlack,
        new MixPresentationViewPort(mix->getId(), repos_, channelMonitorData_),
        true);
  }

  // Get the ID of the active mix presentation.
  juce::Uuid activeMixID = activeMixRepository_->get().getActiveMixId();
  if (activeMixID != juce::Uuid::null()) {
    LOG_ANALYTICS(0, "Active mix presentation ID: " +
                         activeMixID.toString().toStdString());
    // Check there exists a tab with the active mix presentation ID.
    for (int i = 0; i < presentationTabs_->getNumTabs(); ++i) {
      MixPresentationViewPort* tab = static_cast<MixPresentationViewPort*>(
          presentationTabs_->getTabContentComponent(i));

      if (tab->getMixPresID() == activeMixID) {
        presentationTabs_->setCurrentTabIndex(i);
        return;
      }
    }
  }
  // In case the active mix presentation no longer exists, attempt to set the
  // active mix presentation to the last tab and select it.
  else {
    presentationTabs_->setCurrentTabIndex(presentationTabs_->getNumTabs() - 1);
    MixPresentationViewPort* tab = static_cast<MixPresentationViewPort*>(
        presentationTabs_->getCurrentContentComponent());
    juce::Uuid chosenActiveMixID = tab->getMixPresID();
    LOG_ANALYTICS(0, "No Active mix presentation ID found, so setting it to: " +
                         chosenActiveMixID.toString().toStdString());
  }
  LOG_ANALYTICS(RendererProcessor::instanceId_,
                "All presentation tabs updated.");
}

void PresentationMonitorScreen::valueTreeChildAdded(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) {
  if (childWhichHasBeenAdded.getType() == MixPresentation::kTreeType) {
    // Update the number of mix presentations
    updateMixPresentations();
    // handle the case of adding a new mix presentation
    presentationTabs_->addTab(
        childWhichHasBeenAdded[MixPresentation::kPresentationName].toString(),
        EclipsaColours::backgroundOffBlack,
        new MixPresentationViewPort(
            juce::Uuid(childWhichHasBeenAdded[MixPresentation::kId]), repos_,
            channelMonitorData_),
        true);

    MixPresentationSoloMute mixPresentationSoloMute(
        juce::Uuid(childWhichHasBeenAdded[MixPresentationSoloMute::kId]),
        childWhichHasBeenAdded[MixPresentationSoloMute::kName]);

    mixPresentationSoloMuteRepository_->updateOrAdd(mixPresentationSoloMute);

    // repaint the tabs/presentation screen
    repaint(presentationTabBounds_);
  } else if (parentTree.getType() == MixPresentation::kTreeType) {
    childWhichHasBeenAdded.getType();
    // handle the case of renaming a mix presentation
    juce::Uuid mixPresId = juce::Uuid(parentTree[MixPresentation::kId]);
    for (int i = 0; i < presentationTabs_->getNumTabs(); i++) {
      MixPresentationViewPort* tab = static_cast<MixPresentationViewPort*>(
          presentationTabs_->getTabContentComponent(i));
      if (tab->getMixPresID() == mixPresId) {
        presentationTabs_->getTabbedButtonBar().setTabName(
            i, parentTree[MixPresentation::kPresentationName].toString());
        break;
      }
    }
    // update the name in the solo mute repo as well
    MixPresentationSoloMute mixPresSoloMute =
        mixPresentationSoloMuteRepository_->get(mixPresId).value_or(
            MixPresentationSoloMute());
    mixPresSoloMute.setName(
        parentTree[MixPresentation::kPresentationName].toString());
    mixPresentationSoloMuteRepository_->update(mixPresSoloMute);
  }
}

void PresentationMonitorScreen::valueTreeChildRemoved(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved,
    int index) {
  if (childWhichHasBeenRemoved.getType() == MixPresentation::kTreeType) {
    // update the mix presentations
    updateMixPresentations();

    const int numbTabs = presentationTabs_->getNumTabs();
    for (int i = 0; i < numbTabs; i++) {
      MixPresentationViewPort* viewPort = static_cast<MixPresentationViewPort*>(
          presentationTabs_->getTabContentComponent(i));
      if (viewPort->getMixPresID() ==
          juce::Uuid(childWhichHasBeenRemoved[MixPresentation::kId])) {
        presentationTabs_->removeTab(i);
        break;
      }
    }

    // update the tab button bounds
    updateTabButtonBounds(presentationTabBounds_);
    // if a mix presentation is removed, remove it from the mpSM repository
    // this includes removing the audio elements
    MixPresentationSoloMute mixPresentationSoloMute(
        juce::Uuid(childWhichHasBeenRemoved[MixPresentationSoloMute::kId]),
        childWhichHasBeenRemoved[MixPresentationSoloMute::kName]);

    mixPresentationSoloMuteRepository_->remove(mixPresentationSoloMute);
  }
  // repaint the tabs/presentation screen
  repaint(presentationTabBounds_);
}

void PresentationMonitorScreen::valueTreePropertyChanged(
    juce::ValueTree& treeWhosePropertyHasChanged,
    const juce::Identifier& property) {
  if (treeWhosePropertyHasChanged.getType() ==
          ActiveMixPresentation::kTreeType &&
      property == ActiveMixPresentation::kActiveMixID) {
    // Update the active mix presentation
    juce::Uuid activeMixId = juce::Uuid(
        treeWhosePropertyHasChanged[ActiveMixPresentation::kActiveMixID]);
    for (int i = 0; i < presentationTabs_->getNumTabs(); ++i) {
      MixPresentationViewPort* tab = static_cast<MixPresentationViewPort*>(
          presentationTabs_->getTabContentComponent(i));
      if (tab->getMixPresID() == activeMixId) {
        presentationTabs_->setCurrentTabIndex(i);
        return;
      }
    }
  }
}
