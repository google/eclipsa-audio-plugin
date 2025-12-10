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

#include "EditPresentationScreen.h"

#include <cstddef>

#include "../RendererProcessor.h"
#include "MixPresentationTagScreen.h"
#include "components/src/EclipsaColours.h"
#include "data_repository/implementation/MixPresentationRepository.h"
#include "data_structures/src/MixPresentation.h"
#include "mix_tabs/PresentationEditorTab.h"

EditPresentationTabbedComponent::EditPresentationTabbedComponent(
    MixPresentationRepository* mixPresentationRepository,
    MixPresentationTagScreen* tagScreen)
    : juce::TabbedComponent(juce::TabbedButtonBar::TabsAtTop),
      mixPresentationRepository_(mixPresentationRepository),
      tagScreen_(tagScreen) {}

EditPresentationTabbedComponent::~EditPresentationTabbedComponent() {
  setLookAndFeel(nullptr);
}

void EditPresentationTabbedComponent::currentTabChanged(
    int newCurrentTabIndex, const juce::String& newCurrentTabName) {
  juce::TabbedComponent::currentTabChanged(newCurrentTabIndex,
                                           newCurrentTabName);
  PresentationEditorTab* tab = static_cast<PresentationEditorTab*>(
      getTabContentComponent(newCurrentTabIndex));
  if (tab == nullptr) {
    return;
  }
  tagScreen_->changeMixPresentation(tab->getMixPresentationUuid());
}

EditPresentationScreen::EditPresentationScreen(
    MainEditor& editor, AudioElementRepository* ae_repository,
    MixPresentationRepository* mixPresentationRepository,
    ActiveMixRepository* activeMixRepo)
    : tagScreen_(mixPresentationRepository,
                 mixPresentationRepository->getFirst()->getId()),
      initialTabIndex_(0),
      presentationTabs_(std::make_unique<EditPresentationTabbedComponent>(
          mixPresentationRepository, &tagScreen_)),
      headerBar_("Presentations", editor),
      mixPresentationRepository_(mixPresentationRepository),
      audioElementRepository_(ae_repository),
      activeMixRepository_(activeMixRepo),
      addMixPresentationButton_(IconStore::getInstance().getPlusIcon()) {
  setLookAndFeel(&lookAndFeel_);
  setWantsKeyboardFocus(true);
  mixPresentationRepository_->registerListener(this);

  // Configure adding mix presentations
  addMixPresentationButton_.setCyanLookAndFeel();
  addMixPresentationButton_.setButtonOnClick([this]() {
    // Add a new mix presentation
    MixPresentation presentation(
        juce::Uuid(), "My Mix Presentation" + juce::String(numMixes_ + 1), 1,
        LanguageData::MixLanguages::Undetermined, {});
    mixPresentationRepository_->add(presentation);

    // Log the addition of a new mix presentation
    LOG_ANALYTICS(
        RendererProcessor::instanceId_,
        "Adding new mix presentation: " + presentation.getName().toStdString());
  });

  // Update mix presentation information
  updateMixPresentations();

  updatePresentationTabs();

  addAndMakeVisible(presentationTabs_.get());

  presentationTabs_->setColour(
      juce::TabbedComponent::ColourIds::backgroundColourId,
      EclipsaColours::backgroundOffBlack);
  presentationTabs_->setColour(
      juce::TabbedComponent::ColourIds::outlineColourId,
      EclipsaColours::backgroundOffBlack);
  presentationTabs_->getTabbedButtonBar().setColour(
      juce::TabbedButtonBar::ColourIds::frontTextColourId,
      EclipsaColours::selectCyan);

  addAndMakeVisible(tagScreen_);
}

EditPresentationScreen::~EditPresentationScreen() {
  setLookAndFeel(nullptr);
  mixPresentationRepository_->deregisterListener(this);
  presentationTabs_->clearTabs();
}

void EditPresentationScreen::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds();

  // create a copy to refer to later
  auto presentationEditorBounds = bounds;

  // Draw the header bar
  auto headerBarBounds = bounds.removeFromTop(45);
  addAndMakeVisible(headerBar_);
  headerBar_.setBounds(headerBarBounds);

  // Create the presentationEditor Tabs a
  // Remove unused portion on the right
  float tabsWidth = 0.5f;
  float rightColumnWidth = 0.35f;
  const float topTrim = 0.06f;

  bounds.removeFromRight(presentationEditorBounds.getWidth() *
                         rightColumnWidth);
  bounds.removeFromTop(presentationEditorBounds.proportionOfHeight(
      topTrim));  // some additional vertical spacing
  auto presentationTabsBounds =
      bounds.removeFromLeft(presentationEditorBounds.getWidth() * tabsWidth);

  presentationTabs_->setBounds(presentationTabsBounds);

  // Add Mix Presentation button
  addAndMakeVisible(addMixPresentationButton_);
  addMixPresentationButton_.setButtonText("Add Mix Presentation");
  auto addMixButtonBounds =
      bounds.removeFromTop(presentationTabs_->getTabbedButtonBar().getHeight());
  addMixButtonBounds.removeFromLeft(presentationEditorBounds.getWidth() *
                                    0.04f);
  addMixPresentationButton_.setBounds(addMixButtonBounds);

  updateTabButtonBounds(presentationTabsBounds);

  // calculate bounds for mix tags screen
  auto mixTagsBounds = getLocalBounds();

  mixTagsBounds.setLeft(addMixButtonBounds.getBottomLeft().getX());

  mixTagsBounds.setTop(addMixButtonBounds.getBottomLeft().getY());

  tagScreen_.setBounds(mixTagsBounds);

  auto demarkingLineBounds = getLocalBounds();
  demarkingLineBounds.setLeft(presentationTabsBounds.getRight());
  demarkingLineBounds.setTop(addMixButtonBounds.getBottomLeft().getY());
  demarkingLineBounds.removeFromTop(mixTagsBounds.getHeight() * 0.0075f);

  g.setColour(EclipsaColours::headingGrey.withAlpha(0.4f));
  g.drawHorizontalLine(demarkingLineBounds.getTopLeft().getY(),
                       demarkingLineBounds.getBottomLeft().getX(),
                       demarkingLineBounds.getRight());
}

void EditPresentationScreen::updateTabButtonBounds(
    const juce::Rectangle<int>& presentationEditingBounds) {
  // nothing to paint if no mixes
  if (numMixes_ == 0) {
    return;
  }

  if (numMixes_ >= 5) {
    addMixPresentationButton_.dimButton();
    addMixPresentationButton_.setEnabled(false);
  } else {
    addMixPresentationButton_.resetButton();
    addMixPresentationButton_.setEnabled(true);
  }

  presentationTabs_->setBounds(presentationEditingBounds);

  float fraction = 1.f / numMixes_;
  auto tabbedbuttonbarbounds =
      presentationTabs_->getTabbedButtonBar().getBounds();

  tabbedbuttonbarbounds.setWidth(presentationTabs_->getBounds().getWidth());
  const int numTabs = presentationTabs_->getNumTabs();
  for (int i = 0; i < numTabs; i++) {
    juce::TabBarButton* tabButton =
        presentationTabs_->getTabbedButtonBar().getTabButton(i);
    tabButton->setBounds(tabbedbuttonbarbounds.removeFromLeft(
        presentationEditingBounds.getWidth() * fraction));
  }
}

void EditPresentationScreen::updateMixPresentations() {
  // Update the mix presentations array and the number of mixes
  mixPresentationRepository_->getAll(mixPresentationArray_);

  numMixes_ = mixPresentationArray_.size();
}

void EditPresentationScreen::updatePresentationTabs() {
  // Log the update event
  LOG_ANALYTICS(
      RendererProcessor::instanceId_,
      "Mix presentations updated. Total mixes: " + std::to_string(numMixes_));

  // Clear the tabs
  if (presentationTabs_->getNumTabs() > 0) {
    // store what tab the editor was initially on
    initialTabIndex_ = presentationTabs_->getCurrentTabIndex();
    presentationTabs_->clearTabs();
  }
  // Add tabs for each mix
  for (int i = 0; i < mixPresentationArray_.size(); i++) {
    auto mix = mixPresentationArray_[i];
    presentationTabs_->addTab(
        mix->getName(), EclipsaColours::backgroundOffBlack,
        new PresentationEditorTab(mix->getId(), mixPresentationRepository_,
                                  audioElementRepository_,
                                  activeMixRepository_),
        true);
  }
  if (initialTabIndex_ >= presentationTabs_->getNumTabs()) {
    // if the last tab was deleted, goto the new last tab
    initialTabIndex_ = presentationTabs_->getNumTabs() - 1;
  }
  presentationTabs_->setCurrentTabIndex(initialTabIndex_);
}

void EditPresentationScreen::valueTreeChildAdded(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) {
  // handle the case of adding a new mix presentation
  if (childWhichHasBeenAdded.getType() == MixPresentation::kTreeType) {
    presentationTabs_->addTab(
        childWhichHasBeenAdded[MixPresentation::kPresentationName].toString(),
        EclipsaColours::backgroundOffBlack,
        new PresentationEditorTab(
            juce::Uuid(childWhichHasBeenAdded[MixPresentation::kId]),
            mixPresentationRepository_, audioElementRepository_,
            activeMixRepository_),
        true);
    // Update the number of mix presentations
    updateMixPresentations();
    repaint();

  } else if (parentTree.getType() == MixPresentation::kTreeType) {
    juce::Uuid mixPresId = juce::Uuid(parentTree[MixPresentation::kId]);
    for (int i = 0; i < presentationTabs_->getNumTabs(); i++) {
      PresentationEditorTab* tab = static_cast<PresentationEditorTab*>(
          presentationTabs_->getTabContentComponent(i));
      if (tab->getMixPresentationUuid() == mixPresId) {
        presentationTabs_->getTabbedButtonBar().setTabName(
            i, parentTree[MixPresentation::kPresentationName].toString());
        break;
      }
    }
  }
}

void EditPresentationScreen::valueTreeChildRemoved(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved,
    int indexFromWhichChildWasRemoved) {
  if (childWhichHasBeenRemoved.getType() == MixPresentation::kTreeType) {
    presentationTabs_->removeTab(indexFromWhichChildWasRemoved);
    // Update the number of mix presentations
    updateMixPresentations();
    repaint();
    presentationTabs_->setCurrentTabIndex(0);
  }
}
