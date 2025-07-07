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

#include "PresentationEditorTab.h"

#include <optional>

#include "../../RendererProcessor.h"
#include "data_structures/src/AudioElement.h"
#include "data_structures/src/MixPresentation.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

PresentationEditorTab::PresentationEditorTab(
    const juce::Uuid mixPresentationId,
    MixPresentationRepository* mixPresentationRepository,
    AudioElementRepository* aeRepository,
    ActiveMixRepository* activeMixRepository)
    : mixPresentationRepository_(mixPresentationRepository),
      audioElementRepository_(aeRepository),
      mixPresentationId_(mixPresentationId),
      activeMixPresentationRepository_(activeMixRepository),
      presentationName_("Presentation Name"),
      mixGainChanged_([this]() { mixGainChangedCallback(); }),
      gainControl_(mixGainChanged_),
      presentationLanguage_("Language"),
      addAudioElement_("", IconStore::getInstance().getAddIcon()),
      currentMixGain_(initializeCurrentMixGain(mixPresentationId)),
      gainBounds_({-100, 12}),
      presentationGainKnob_(gainBounds_.first, gainBounds_.second, 0.0,
                            currentMixGain_.getIntValue(), "dB"),
      audioElementChanged_([this]() { audioElementChangedCallback(); }),
      languageChanged_([this]() { languageChangedCallback(); }),
      mixPresentationRemoved_([this]() { deleteMixPresentation(); }),
      viewPort_(&audioElementsAlreadyDrawn_),
      changeMixPresentationName_(
          [this]() { changeMixPresentationNameCallback(); }),

      deleteMixPresentationButton_(IconStore::getInstance().getDeleteIcon()) {
  deleteMixPresentationButton_.setGreyLookAndFeel();
  setLookAndFeel(&lookAndFeel_);
  MixPresentation mixPres = mixPresentationRepository_->get(mixPresentationId_)
                                .value_or(MixPresentation());
  setName(mixPres.getName());
  addAndMakeVisible(presentationLanguage_);
  audioElementRepository_->registerListener(this);
  mixPresentationRepository_->registerListener(this);

  setWantsKeyboardFocus(false);

  addAndMakeVisible(addAudioElement_);
  addAndMakeVisible(viewPort_);

  // Setup the presentation name and gain control text boxes
  addAndMakeVisible(gainControl_);

  gainControl_.setText(currentMixGain_);

  addAndMakeVisible(presentationGainKnob_);
  presentationGainKnob_.setValueUpdatedCallback([this](int newVal) {
    MixPresentation mixPres =
        mixPresentationRepository_->get(mixPresentationId_)
            .value_or(MixPresentation());
    if (newVal == mixPres.getGainIndB()) {
      return;
    }
    latestSliderValue_ = newVal;
    gainControl_.setText(juce::String(newVal));
    if (!isTimerRunning()) {
      previousSliderValue_ = newVal;
      startTimerHz(timerFrequency);
    }
  });
  setupTitleTextBox(presentationName_, changeMixPresentationName_);
  presentationName_.setText(mixPres.getName());

  configureLanguageDropDownBox();

  // get the audio elements for this tab
  addAudioElement_.setTextWhenNothingSelected(addAudioElementDefaultText_);
  getAudioElements();
  addAudioElement_.onChange(audioElementChanged_);
  addAudioElement_.setNameForComboBox(mixPres.getName());

  juce::String deleteText = "Delete \"" + mixPres.getName() + "\"";
  deleteMixPresentationButton_.setButtonText(deleteText);
  deleteMixPresentationButton_.setButtonOnClick(mixPresentationRemoved_);
  addAndMakeVisible(deleteMixPresentationButton_);
  updateDeleteMixPresButton();
}

void PresentationEditorTab::timerCallback() {
  if (latestSliderValue_ == previousSliderValue_) {
    sliderValueUnchangedTally_++;
  } else {  // slider was updated while the timer was active
    sliderValueUnchangedTally_ = 0;
    previousSliderValue_ = latestSliderValue_;  // update the
                                                // previousSliderValue_
  }

  if (sliderValueUnchangedTally_ >= 5) {
    stopTimer();
    sliderValueUnchangedTally_ = 0;
    // update the mix gain
    MixPresentation mixPres =
        mixPresentationRepository_->get(mixPresentationId_)
            .value_or(MixPresentation());
    mixPres.setGainFromdB(latestSliderValue_);
    mixPresentationRepository_->update(mixPres);
  }
}

PresentationEditorTab::~PresentationEditorTab() {
  // deregister listeners
  setLookAndFeel(nullptr);
  audioElementRepository_->deregisterListener(this);
  mixPresentationRepository_->deregisterListener(this);
}

void PresentationEditorTab::configureLanguageDropDownBox() {
  presentationLanguage_.onChange(languageChanged_);
  //  set up the Language selection box
  presentationLanguage_.setTextWhenNothingSelected("Select Language");
  for (int i = 1; i < static_cast<int>(LanguageData::MixLanguages::COUNT);
       i++) {
    LanguageData::MixLanguages language =
        static_cast<LanguageData::MixLanguages>(i);
    presentationLanguage_.addOption(
        MixPresentation::languageToString(language));
  }
  MixPresentation mixPres = mixPresentationRepository_->get(mixPresentationId_)
                                .value_or(MixPresentation());
  // set the combo box to the selected language from the repo
  if (mixPres.getMixPresentationLanguage() ==
      LanguageData::MixLanguages::Undetermined) {
    presentationLanguage_.setSelectedIndex(-1, juce::dontSendNotification);
    presentationLanguage_.setText("Select Language");
  } else {
    LanguageData::MixLanguages language(mixPres.getMixPresentationLanguage());
    int boxIndex = static_cast<int>(language);
    presentationLanguage_.setSelectedIndex(boxIndex - 1,
                                           juce::dontSendNotification);
  }
}

void PresentationEditorTab::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds();

  bounds.removeFromTop(10);
  bounds.removeFromBottom(10);

  g.setColour(juce::Colours::transparentWhite);
  // assign a copy to refer to the original height and width
  auto presentationEditingBounds = bounds;

  // Presentation Name, Language, Gain and Gain Dial will go here
  const float topComponentsHeight = 0.32f;
  auto topComponents = bounds.removeFromTop(
      presentationEditingBounds.getHeight() * topComponentsHeight);

  // Split the topComponents into two columns, left column will have the
  // name,language and combobox right column will have the gain and gain dial
  // The top half will be name language and the dial
  const float topRightColumnWidth = 0.14f;
  const float componentHeight = 0.15f;
  auto nameLanguageDialBounds = topComponents.removeFromTop(
      presentationEditingBounds.getHeight() * componentHeight);

  // assign the left 84% to the name and language
  auto nameandLanguageBounds = nameLanguageDialBounds.removeFromLeft(
      topComponents.getWidth() * (1.f - topRightColumnWidth));

  const float middlepspacing = 0.02f;  // spacing between the name and language
  auto nameBounds = nameandLanguageBounds.removeFromLeft(
      nameandLanguageBounds.getWidth() * 0.5f);
  nameBounds.removeFromRight(
      topComponents.getWidth() *
      middlepspacing);  // apply middle spacing to the right of name
  presentationName_.setBounds(nameBounds);

  nameandLanguageBounds.removeFromLeft(
      topComponents.getWidth() *
      middlepspacing);  // apply middle spacing to the left of language
  presentationLanguage_.setBounds(nameandLanguageBounds);

  // Draw the dial here
  const float offset = 0.01f;
  const float dialHeight = 0.08f;
  auto dialbounds = nameLanguageDialBounds.removeFromBottom(
      presentationEditingBounds.getHeight() * (dialHeight + offset));

  dialbounds.removeFromLeft(presentationEditingBounds.getWidth() * 0.05f);
  // remove some space from the right
  dialbounds.removeFromRight(presentationEditingBounds.getWidth() * 0.01f);
  // ensure the dialbounds have aspect ratio of one
  adjustDialAspectRatio(dialbounds);
  presentationGainKnob_.setBounds(dialbounds);

  auto leftColumn = topComponents.removeFromLeft(
      presentationEditingBounds.proportionOfWidth(1.f - topRightColumnWidth));

  leftColumn.removeFromTop(presentationEditingBounds.getHeight() * 0.01f);
  addAudioElementBounds_ = leftColumn;
  updateSelectionBoxVisuals();
  addAudioElement_.setBounds(addAudioElementBounds_);

  topComponents.removeFromBottom(presentationEditingBounds.getHeight() * 0.01f);
  topComponents.removeFromLeft(presentationEditingBounds.getWidth() * 0.04f);
  gainControl_.setBounds(topComponents);

  // save the bounds of the audio element containers
  containersBounds_ = bounds.removeFromTop(calculateViewPortHeight());
  viewPort_.setBounds(containersBounds_);
  viewPort_.repaint();

  // some padding
  bounds.removeFromTop(20);
  bounds.reduce(presentationEditingBounds.proportionOfWidth(0.33f), 0);

  deleteMixPresentationButton_.setBounds(bounds.removeFromTop(30));
}

void PresentationEditorTab::valueTreeChildAdded(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) {
  if (childWhichHasBeenAdded.getType() == AudioElement::kTreeType) {
    // add the new audio element to the list
    // ensures the correct look and feel is assigned
    getAudioElements();

    // repaint
    repaint(addAudioElementBounds_);

  } else if (childWhichHasBeenAdded.getType() == MixPresentation::kTreeType) {
    // the EditPresentationScreen will handle the removal of the tab
    // we want to check if there is only one mix presentaiton remaining
    // if so, ensure the "Delete Mix Presentation" button is disabled
    updateDeleteMixPresButton();
  }
}

void PresentationEditorTab::valueTreeChildRemoved(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved,
    int indexFromWhichChildWasRemoved) {
  if (childWhichHasBeenRemoved.getType() == AudioElement::kTreeType) {
    AudioElement removed = AudioElement::fromTree(childWhichHasBeenRemoved);
    MixPresentation mixPres =
        mixPresentationRepository_->get(mixPresentationId_).value();
    std::vector<MixPresentationAudioElement> mixPresentationAE =
        mixPres.getAudioElements();
    // only update the mix presentation if the audio element belongs to it
    // otherwise do nothing
    for (auto& mixAE : mixPresentationAE) {
      if (mixAE.getId() == removed.getId()) {
        mixPres.removeAudioElement(removed.getId());
        mixPresentationRepository_->update(mixPres);
        removeFromAlreadyDrawnMap(removed.getId());
        break;
      }
    }
    getAudioElements();

    repaint();

  } else if (childWhichHasBeenRemoved.getType() == MixPresentation::kTreeType) {
    // the EditPresentationScreen will handle the removal of the tab
    // we want to check if there is only one mix presentaiton remaining
    // if so, ensure the "Delete Mix Presentation" button is disabled
    updateDeleteMixPresButton();
  }
}

void PresentationEditorTab::valueTreePropertyChanged(
    juce::ValueTree& treeWhosePropertyHasChanged,
    const juce::Identifier& property) {
  // check if AudioElement Renamed
  if (treeWhosePropertyHasChanged.getType() == AudioElement::kTreeType &&
      property == AudioElement::kName) {
    getAudioElements();
    const juce::Uuid audioElementId =
        juce::Uuid(treeWhosePropertyHasChanged[AudioElement::kId]);
    if (audioElementsAlreadyDrawn_.contains(audioElementId)) {
      audioElementsAlreadyDrawn_[audioElementId]->updateName(
          treeWhosePropertyHasChanged[AudioElement::kName]);
    }
    repaint();
  }
}

// This functions stores all audio elements in the repository to
// allAudioElementsArray_
// Also updates the dropDown ComboBox with the audio elements
void PresentationEditorTab::getAudioElements() {
  audioElementRepository_->getAll(allAudioElementsArray_);
  addAudioElement_.clear(juce::dontSendNotification);
  // Add to drop down
  for (auto audioElement : allAudioElementsArray_) {
    addAudioElement_.addOption(audioElement->getName());
  }
  ensureComboBoxNothingSelected();

  // are there already audio elements assigned to this mix presentation?
  MixPresentation mixPres = mixPresentationRepository_->get(mixPresentationId_)
                                .value_or(MixPresentation());
  std::vector<MixPresentationAudioElement> mixpresentationAudioElements =
      mixPres.getAudioElements();

  audioElementsAlreadyDrawn_.clear();  // Clear existing containers

  for (const auto& mixAE : mixpresentationAudioElements) {
    juce::Uuid audioElementId = mixAE.getId();
    AudioElement* audioElement = getAudioElement(audioElementId);
    if (audioElement) {
      audioElementsAlreadyDrawn_[audioElementId] =
          std::make_unique<MixAEContainer>(
              audioElement->getName(),  // Use AE name, not mixAE name
              audioElement->getChannelConfig().toString());
      auto container = audioElementsAlreadyDrawn_[audioElementId].get();
      container->setDeleteButtonListener(this);
      // Set checkbox state from repository
      container->getIsBinauralCheckbox()->setToggleState(
          mixAE.isBinaural(), juce::dontSendNotification);
      // Set change handler to update repository
      container->setBinauralChangeHandler(
          [this, audioElementId](bool newBinauralState) {
            MixPresentation mixPres =
                mixPresentationRepository_->get(mixPresentationId_)
                    .value_or(MixPresentation());
            mixPres.setBinaural(audioElementId, newBinauralState);
            mixPresentationRepository_->update(mixPres);
          });
    }
  }

  updateSelectionBoxVisuals();
}

AudioElement* PresentationEditorTab::getAudioElement(const juce::Uuid& Id) {
  for (auto audioElement : allAudioElementsArray_) {
    if (audioElement->getId() == Id) {
      return audioElement;
    }
  }

  // if audio element was not found, it was removed from the Audio Element
  // Repository The mix presentation should be updated to reflect this
  LOG_ANALYTICS(
      RendererProcessor::instanceId_,
      "MixPres AE not Found in AE Repository. Need to update mixPresRepo");

  // Attempt to remove the audio element from the mix presentation.
  MixPresentation mixPres = mixPresentationRepository_->get(mixPresentationId_)
                                .value_or(MixPresentation());
  mixPres.removeAudioElement(Id);
  mixPresentationRepository_->update(mixPres);
}

void PresentationEditorTab::audioElementChangedCallback() {
  // get the selected audio element
  int index = addAudioElement_.getSelectedIndex();

  ensureComboBoxNothingSelected();
  if (index == -1) {
    LOG_ANALYTICS(RendererProcessor::instanceId_,
                  "AudioElement selection cleared.");
    return;
  }
  if (allAudioElementsArray_[index] == nullptr) {
    LOG_ERROR(RendererProcessor::instanceId_, "AudioElement is null");
  }

  // if the audio element container is already drawn, return
  if (audioElementsAlreadyDrawn_.find(allAudioElementsArray_[index]->getId()) !=
      audioElementsAlreadyDrawn_.end()) {
    viewPort_.repaint();
    return;
  }
  MixPresentation mixPres = mixPresentationRepository_->get(mixPresentationId_)
                                .value_or(MixPresentation());
  mixPres.addAudioElement(allAudioElementsArray_[index]->getId(), 1,
                          allAudioElementsArray_[index]->getName());
  mixPresentationRepository_->update(mixPres);
  addToAlreadyDrawnMap(allAudioElementsArray_[index]->getId());

  repaint();
  LOG_ANALYTICS(RendererProcessor::instanceId_,
                "Added AudioElement to MixPresentation");
}

void PresentationEditorTab::addToAlreadyDrawnMap(
    const juce::Uuid audioElementId) {
  // Get MixPresentation instance
  MixPresentation mixPres = mixPresentationRepository_->get(mixPresentationId_)
                                .value_or(MixPresentation());

  // Find the audio element inside the MixPresentation
  auto audioElements = mixPres.getAudioElements();
  auto it =
      std::find_if(audioElements.begin(), audioElements.end(),
                   [audioElementId](const MixPresentationAudioElement& ae) {
                     return ae.getId() == audioElementId;
                   });

  if (it == audioElements.end()) {
    LOG_ERROR(RendererProcessor::instanceId_,
              "AudioElement not found in MixPresentation.");
    return;
  }

  MixPresentationAudioElement mixAE = *it;
  std::optional<AudioElement> linkedAudioElementOpt =
      audioElementRepository_->get(audioElementId);
  juce::String channelConfigStr =
      linkedAudioElementOpt.has_value()
          ? linkedAudioElementOpt->getChannelConfig().toString()
          : "Unknown";

  // Create a UI container for this audio element
  audioElementsAlreadyDrawn_[audioElementId] =
      std::make_unique<MixAEContainer>(mixAE.getName(), channelConfigStr);
  audioElementsAlreadyDrawn_[audioElementId]->setDeleteButtonListener(this);

  // Set up the isBinaural checkbox
  auto isBinauralCheckbox =
      audioElementsAlreadyDrawn_[audioElementId]->getIsBinauralCheckbox();
  isBinauralCheckbox->setToggleState(mixAE.isBinaural(),
                                     juce::dontSendNotification);

  audioElementsAlreadyDrawn_[audioElementId]->setBinauralChangeHandler(
      [this, audioElementId](bool newBinauralState) {
        LOG_ANALYTICS(RendererProcessor::instanceId_,
                      "Binaural checkbox changed to: " +
                          std::to_string(newBinauralState));

        MixPresentation mixPres =
            mixPresentationRepository_->get(mixPresentationId_)
                .value_or(MixPresentation());
        mixPres.setBinaural(audioElementId, newBinauralState);
        mixPresentationRepository_->update(mixPres);

        // Verify update was successful
        MixPresentation verifyMixPres =
            mixPresentationRepository_->get(mixPresentationId_)
                .value_or(MixPresentation());
        LOG_ANALYTICS(RendererProcessor::instanceId_,
                      "After update, binaural state is: " +
                          std::to_string(verifyMixPres.isAudioElementBinaural(
                              audioElementId)));
      });
}

void PresentationEditorTab::languageChangedCallback() {
  int index = presentationLanguage_.getSelectedIndex();
  MixPresentation mixPres = mixPresentationRepository_->get(mixPresentationId_)
                                .value_or(MixPresentation());
  int currentLanguageIndex =
      static_cast<int>(mixPres.getMixPresentationLanguage()) - 1;
  // no need to do anything
  if (index == -1) {
    presentationLanguage_.setText("Select Language");
    return;
  } else if (index == currentLanguageIndex) {
    return;
  }

  index++;
  LanguageData::MixLanguages language =
      static_cast<LanguageData::MixLanguages>(index);
  mixPres.setLanguage(language);
  mixPresentationRepository_->update(mixPres);
  LOG_ANALYTICS(RendererProcessor::instanceId_,
                std::string("Language changed for MixPresentation to: ") +
                    MixPresentation::languageToString(language).toStdString());
}

void PresentationEditorTab::ensureComboBoxNothingSelected() {
  // Remove temporarily to ensure that just .setText() is sufficient
  // addAudioElement_.setSelectedIndex(-1, juce::dontSendNotification);
  addAudioElement_.setText(addAudioElementDefaultText_);
}

// For the remove audio element button
void PresentationEditorTab::buttonClicked(juce::Button* button) {
  for (auto& pair : audioElementsAlreadyDrawn_) {
    if (button == pair.second.get()->getDeleteButton()) {
      MixPresentation mixPres =
          mixPresentationRepository_->get(mixPresentationId_)
              .value_or(MixPresentation());
      mixPres.removeAudioElement(pair.first);
      mixPresentationRepository_->update(mixPres);
      removeFromAlreadyDrawnMap(pair.first);
      return;
    }
  }
}

void PresentationEditorTab::removeFromAlreadyDrawnMap(
    const juce::Uuid audioElementId) {
  audioElementsAlreadyDrawn_.erase(audioElementId);
  updateSelectionBoxVisuals();
  repaint();
}

void PresentationEditorTab::updateSelectionBoxVisuals() {
  if (audioElementsAlreadyDrawn_.size() == allAudioElementsArray_.size()) {
    addAudioElement_.setEnabled(false);
    addAudioElement_.dimSelectionBox();

  } else {
    addAudioElement_.restoreLookAndFeel();
    addAudioElement_.setEnabled(true);
  }
}

void PresentationEditorTab::adjustDialAspectRatio(
    juce::Rectangle<int>& dialBounds) {
  if (dialBounds.getWidth() < dialBounds.getHeight()) {
    dialBounds.setHeight(dialBounds.getWidth());
  } else {
    dialBounds.setWidth(dialBounds.getHeight());
  }
}

// Remove the mix presentation from the repository.
void PresentationEditorTab::deleteMixPresentation() {
  // If the mix presentation to be removed is the currently active mix
  // presentation, attempt to set a new active mix presentation while removing.
  ActiveMixPresentation activeMix = activeMixPresentationRepository_->get();
  MixPresentation mixPres = mixPresentationRepository_->get(mixPresentationId_)
                                .value_or(MixPresentation());

  mixPresentationRepository_->remove(mixPres);

  if (mixPres.getId() == activeMix.getActiveMixId()) {
    // Attempt to set the active mix to the first valid mix presentation in the
    // repository.
    activeMix.updateActiveMixId(
        mixPresentationRepository_->getFirst()->getId());

    activeMixPresentationRepository_->update(activeMix);
  }
}

void PresentationEditorTab::changeMixPresentationNameCallback() {
  if (presentationName_.getText().isEmpty()) {
    presentationName_.setText(getName());
    return;
  }
  // no need to update the repository
  if (presentationName_.getText() == getName()) {
    return;
  }
  // update the name so residual callbacks kick out
  setName(presentationName_.getText());

  MixPresentation mixPres = mixPresentationRepository_->get(mixPresentationId_)
                                .value_or(MixPresentation());
  // change the name of the mix presentation
  mixPres.setName(presentationName_.getText());
  mixPresentationRepository_->update(mixPres);

  deleteMixPresentationButton_.setButtonText(
      "Delete \"" + presentationName_.getText() + "\"");
}

void PresentationEditorTab::setupTitleTextBox(
    TitledTextBox& titleTextBox, const std::function<void()>& callback) {
  titleTextBox.setOnReturnCallback(callback);
  titleTextBox.setOnFocusLostCallback(callback);
  // may add these callbacks in the future
  addAndMakeVisible(titleTextBox);
}

void PresentationEditorTab::mixGainChangedCallback() {
  if (gainControl_.getText().getIntValue() == currentMixGain_.getIntValue()) {
    LOG_ANALYTICS(RendererProcessor::instanceId_, "Mix gain unchanged.");
    return;
  }
  int value = gainControl_.getText().getIntValue();
  // clamp the value
  if (value > gainBounds_.second) {
    value = gainBounds_.second;
  } else if (value < gainBounds_.first) {
    value = gainBounds_.first;
  }
  currentMixGain_ = juce::String(value);
  presentationGainKnob_.setValue(value);
  // update the mix gain
  MixPresentation mixPres = mixPresentationRepository_->get(mixPresentationId_)
                                .value_or(MixPresentation());
  mixPres.setGainFromdB(value);
  mixPresentationRepository_->update(mixPres);
  LOG_ANALYTICS(RendererProcessor::instanceId_,
                "Mix gain updated to: " + std::to_string(value) + " dB.");
}

int PresentationEditorTab::calculateViewPortHeight() {
  return std::min(viewPort_.getRequiredHeight(), viewPort_.kMaxHeight);
}

int PresentationEditorTab::initializeCurrentMixGain(const juce::Uuid) {
  MixPresentation mixPres = mixPresentationRepository_->get(mixPresentationId_)
                                .value_or(MixPresentation());

  return mixPres.getGainIndB();
}

void PresentationEditorTab::updateDeleteMixPresButton() {
  juce::OwnedArray<MixPresentation> mixPresArray;
  mixPresentationRepository_->getAll(mixPresArray);
  if (mixPresArray.size() == 1) {
    deleteMixPresentationButton_.setEnabled(false);
    deleteMixPresentationButton_.dimButton();
  } else {
    deleteMixPresentationButton_.setEnabled(true);
    deleteMixPresentationButton_.resetButton();
  }
  deleteMixPresentationButton_.repaint();
}
