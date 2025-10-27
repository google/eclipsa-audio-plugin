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

#include "RendererProcessor.h"

#include <processors/processors.h>

#include "RendererEditor.h"
#include "RendererVersionConverter.h"
#include "data_repository/implementation/ActiveMixPresentationRepository.h"
#include "data_structures/src/ActiveMixPresentation.h"
#include "data_structures/src/FileExport.h"
#include "data_structures/src/MixPresentation.h"
#include "data_structures/src/RoomSetup.h"
#include "logger/logger.h"
#include "processors/processor_base/ProcessorBase.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

//==============================================================================
RendererProcessor::RendererProcessor()
    // Logic Pro optimized builds: use host-wide layout
    : ProcessorBase(ProcessorBase::getHostWideLayout(),
                    kIsLogicProBuild ? ProcessorBase::getHostWideLayout()
                                     : juce::AudioChannelSet::stereo()),
      // Load persistent state. Initialize repositories from persistent state.
      persistentState_(kRendererStateKey),
      roomSetupRepository_(getTreeWithId(kRoomSetupKey)),
      audioElementRepository_(getTreeWithId(kAudioElementsKey)),
      mixPresentationRepository_(getTreeWithId(kMixPresentationsKey)),
      mixPresentationSoloMuteRepository_(
          getTreeWithId(kMixPresentationSoloMuteKey)),
      mixPresentationLoudnessRepository_(
          getTreeWithId(kMixPresentationLoudnessKey)),
      multichannelgainRepository_(getTreeWithId(kMultiChannelGainsKey)),
      audioElementSpatialLayoutRepository_(
          juce::ValueTree("AudioElementSpatialLayoutRepository")),
      syncServer_(&audioElementRepository_, 2134, this),
      fileExportRepository_(getTreeWithId(kFileExportKey)),
      msPlaybackRepository_(getTreeWithId(kMSPlaybackKey)),
      activeMixPresentationRepository_(getTreeWithId(kActiveMixKey)),
      filePlaybackRepository_(getTreeWithId(kFilePlaybackKey)),
      isRealtime_(true) {
  // Initialize Logger
  Logger::getInstance().init("EclipsaRenderer");

  // Log instantiation of RendererProcessor
  LOG_ANALYTICS(instanceId_, "RendererProcessor instantiated.");

  // Construct processor chain.
  audioProcessors_.push_back(
      std::make_unique<GainProcessor>(&multichannelgainRepository_));
  if (juce::PluginHostType().isPremiere()) {
    audioProcessors_.push_back(
        std::make_unique<PremiereProLoudnessExportProcessor>(
            fileExportRepository_, mixPresentationRepository_,
            mixPresentationLoudnessRepository_, audioElementRepository_));
    audioProcessors_.push_back(std::make_unique<PremiereProFileOutputProcessor>(
        fileExportRepository_, audioElementRepository_,
        mixPresentationRepository_, mixPresentationLoudnessRepository_));
  } else {
    audioProcessors_.push_back(std::make_unique<LoudnessExportProcessor>(
        fileExportRepository_, mixPresentationRepository_,
        mixPresentationLoudnessRepository_, audioElementRepository_));
    audioProcessors_.push_back(std::make_unique<FileOutputProcessor>(
        fileExportRepository_, audioElementRepository_,
        mixPresentationRepository_, mixPresentationLoudnessRepository_));
  }
  audioProcessors_.push_back(std::make_unique<ChannelMonitorProcessor>(
      channelMonitorData_, &mixPresentationRepository_,
      &mixPresentationSoloMuteRepository_));
  audioProcessors_.push_back(std::make_unique<RenderProcessor>(
      this, &roomSetupRepository_, &audioElementRepository_,
      &mixPresentationRepository_, &activeMixPresentationRepository_,
      monitorData_));
  audioProcessors_.push_back(std::make_unique<WavFileOutputProcessor>(
      fileExportRepository_, roomSetupRepository_));
  audioProcessors_.push_back(std::make_unique<MSProcessor>(getRepositories()));
  audioProcessors_.push_back(std::make_unique<MixMonitorProcessor>(
      roomSetupRepository_, monitorData_));
  audioProcessors_.push_back(std::make_unique<RemappingProcessor>(this, true));
  // Use host layout size for output channels configuration
  juce::AudioChannelSet outputChannels;
  for (int i = 0; i < getHostWideLayout().size(); i++) {
    outputChannels.addChannel((juce::AudioChannelSet::ChannelType)i);
  }

  // Set up listening for the switch to manual offline mode
  fileExportRepository_.registerListener(this);
  roomSetupRepository_.registerListener(this);
}

RendererProcessor::~RendererProcessor() { audioProcessors_.clear(); }

bool RendererProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const {
  if (kIsLogicProBuild) {
    // Logic Pro optimized builds: use wide layout support
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in.isDisabled() || out.isDisabled()) return false;
    return Speakers::isNamedBed(in) || Speakers::isSymmetricDiscrete(in);
  } else {
    // Non-optimized builds: original working code for all DAWs
    // Ensure the input channel set is wide enough for us
    if (layouts.getMainInputChannelSet() != getHostWideLayout()) {
      return false;
    }

    auto hostType = juce::PluginHostType();

    if (hostType.isReaper()) {
      return layouts.getMainOutputChannelSet() == outputChannelSet_;
    }

    // Ensure the output channel set it one of the channel sets we support
    // rendering to
    if (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() ||
        layouts.getMainOutputChannelSet() ==
            juce::AudioChannelSet::create5point1() ||
        layouts.getMainOutputChannelSet() ==
            juce::AudioChannelSet::create5point1point2() ||
        layouts.getMainOutputChannelSet() ==
            juce::AudioChannelSet::create5point1point4() ||
        layouts.getMainOutputChannelSet() ==
            juce::AudioChannelSet::create7point1() ||
        layouts.getMainOutputChannelSet() ==
            juce::AudioChannelSet::create7point1point4()) {
      return true;
    } else {
      return false;
    }
  }
}

bool RendererProcessor::applyBusLayouts(const BusesLayout& layouts) {
  bool check = ProcessorBase::applyBusLayouts(layouts);
  if (check) {
    std::string applyBusLayoutMessage =
        "applyBusLayouts returning TRUE with output: " +
        layouts.getMainOutputChannelSet().getDescription().toStdString() + "\n";

    LOG_ANALYTICS(instanceId_, applyBusLayoutMessage);
  }

  return check;
}

//==============================================================================
const juce::String RendererProcessor::getName() const {
  return {"Eclipsa Audio Renderer"};
}

//==============================================================================
void RendererProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  LOG_ANALYTICS(instanceId_, "RendererProcessor prepareToPlay \n");
  setRateAndBufferSizeDetails(sampleRate, samplesPerBlock);
  for (const auto& proc : audioProcessors_) {
    proc->prepareToPlay(sampleRate, samplesPerBlock);
  }
  // Keep a wide internal processing buffer (28 ch) regardless of the active bus
  // to avoid auval crashes when Logic probes wider layouts.
  // Use host layout size instead of hardcoded 28 for consistency
  processingBuffer_.setSize(getHostWideLayout().size(), samplesPerBlock, false,
                            true, true);
  LOG_ANALYTICS(instanceId_, "activeMixPresentation Uuid: " +
                                 activeMixPresentationRepository_.get()
                                     .getActiveMixId()
                                     .toString()
                                     .toStdString() +
                                 "\n");
}

void RendererProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

void RendererProcessor::setNonRealtime(bool isNonRealtime) noexcept {
  juce::AudioProcessor::setNonRealtime(
      isNonRealtime);  // call the superclasses overriden function
  for (const auto& proc : audioProcessors_) {
    proc->setNonRealtime(isNonRealtime);
  }
}

void RendererProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

#if JUCE_DEBUG
  juce::SpinLock::ScopedLockType realtimeLock(realtimeLock_);
#endif

  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // In case we have more outputs than inputs, this code clears any output
  // channels that didn't contain input data, (because these aren't
  // guaranteed to be empty - they may contain garbage).
  // This is here to avoid people getting screaming feedback
  // when they first compile a plugin, but obviously you don't need to keep
  // this code if your algorithm always overwrites all the output channels.
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  // Copy the input buffer into the processing buffer
  // We do this since we may want to modify audio element audio or render
  // to more channels than are available on output. ProTools makes channels
  // beyond the playback layout channel read-only in the buffer, so we
  // need to copy the data into a buffer we can modify.

  // Add bounds checking to prevent crashes during auval testing
  int channelsToCopy =
      juce::jmin(totalNumInputChannels, buffer.getNumChannels(),
                 processingBuffer_.getNumChannels());

  for (int ch = 0; ch < channelsToCopy; ++ch) {
    processingBuffer_.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
  }

  for (const auto& proc : audioProcessors_) {
    proc->processBlock(processingBuffer_, midiMessages);
  }

  // Copy the processing buffer back to the output buffer
  // Copy back only the number of channels that the DAW expects to render
  int channelsToOutput =
      juce::jmin(totalNumOutputChannels, buffer.getNumChannels(),
                 processingBuffer_.getNumChannels());

  for (int ch = 0; ch < channelsToOutput; ++ch) {
    buffer.copyFrom(ch, 0, processingBuffer_, ch, 0, buffer.getNumSamples());
  }
}

//==============================================================================
bool RendererProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* RendererProcessor::createEditor() {
  LOG_ANALYTICS(instanceId_, "RendererProcessor createEditor \n");
  return new RendererEditor(*this);
}

//==============================================================================
void RendererProcessor::getStateInformation(juce::MemoryBlock& destData) {
  LOG_ANALYTICS(instanceId_, "RendererProcessor getStateInformation \n");

// Always add the latest version attribute to the XML state
#if defined(ECLIPSA_VERSION)
  LOG_ANALYTICS(instanceId_, "Renderer Plugin setting config version to \n" +
                                 std::string(ECLIPSA_VERSION));
  persistentState_.setProperty("version", ECLIPSA_VERSION, nullptr);
#endif

  copyXmlToBinary(*persistentState_.createXml(), destData);
}

void RendererProcessor::setStateInformation(const void* data, int sizeInBytes) {
  LOG_ANALYTICS(instanceId_, "RendererProcessor setStateInformation \n");
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));

  if (xmlState.get() && xmlState->hasTagName(persistentState_.getType())) {
    persistentState_ = juce::ValueTree::fromXml(*xmlState);
  }

  // Check the version converstion to see if version upgrade is needed and apply
  // upgrades Do this before updating repositories since if we load the
  // repositories and then update their values, it will cause tree change events
  // on the processors, which normally updating the repositories would not do.
  RendererVersionConverter::convertToLatestVersion(xmlState);

  updateRepositories();

  initializeMixPresentations();

  configureOutputBus();

  if (juce::PluginHostType().isPremiere()) {
    FileExport initialConfig =
        fileExportRepository_.get();  // Get the initial file export config

    if (initialConfig.getManualExport()) {
      LOG_ANALYTICS(
          instanceId_,
          "setStateInformation: Calling setNonRealTime(true) for Premiere Pro");
      setNonRealtime(true);
    }
  }

  LOG_ANALYTICS(instanceId_, "activeMixPresentation Uuid: " +
                                 activeMixPresentationRepository_.get()
                                     .getActiveMixId()
                                     .toString()
                                     .toStdString() +
                                 "\n");

  reinitializeAfterStateRestore();
}

void RendererProcessor::updateRepositories() {
  juce::ValueTree audioElements =
      persistentState_.getChildWithName(kAudioElementsKey);
  if (audioElements.isValid()) {
    audioElementRepository_.setStateTree(audioElements);
  }

  juce::ValueTree roomSetup = persistentState_.getChildWithName(kRoomSetupKey);
  if (roomSetup.isValid()) {
    roomSetupRepository_.setStateTree(roomSetup);
  }

  juce::ValueTree mixPresentations =
      persistentState_.getChildWithName(kMixPresentationsKey);
  int mixPresCount = mixPresentations.getNumChildren();
  if (mixPresentations.isValid()) {
    mixPresentationRepository_.setStateTree(mixPresentations);
    LOG_ANALYTICS(instanceId_,
                  "setStateInformation: Mix Presentations was successfully "
                  "loaded from persistent state.");
    LOG_ANALYTICS(
        instanceId_,
        "The Number of Mix Presentations found in the persistent state was: " +
            std::to_string(mixPresCount));
  } else {
    LOG_ANALYTICS(instanceId_,
                  "setStateInformation: Mix Presentation tree invalid or no "
                  "Mix Presentations found. There are currently " +
                      std::to_string(mixPresCount) +
                      " Mix Presentations in the repository.");
  }

  juce::ValueTree mixPresentationLoudness =
      persistentState_.getChildWithName(kMixPresentationLoudnessKey);
  if (mixPresentationLoudness.isValid()) {
    mixPresentationLoudnessRepository_.setStateTree(mixPresentationLoudness);
  }

  juce::ValueTree mixPresentationSoloMute =
      persistentState_.getChildWithName(kMixPresentationSoloMuteKey);
  if (mixPresentationSoloMute.isValid()) {
    mixPresentationSoloMuteRepository_.setStateTree(mixPresentationSoloMute);
  }

  juce::ValueTree activeMixPresentation =
      persistentState_.getChildWithName(kActiveMixKey);
  if (activeMixPresentation.isValid()) {
    activeMixPresentationRepository_.setStateTree(activeMixPresentation);
  }

  juce::ValueTree fileExport =
      persistentState_.getChildWithName(kFileExportKey);
  if (fileExport.isValid()) {
    fileExportRepository_.setStateTree(fileExport);
  }

  juce::ValueTree channelGains =
      persistentState_.getChildWithName(kMultiChannelGainsKey);
  if (channelGains.isValid()) {
    multichannelgainRepository_.setStateTree(channelGains);
    LOG_ANALYTICS(
        instanceId_,
        "setStateInformation: MultiChannelGainRepository was "
        "successfully loaded from persistent state. \n" +
            multichannelgainRepository_.getTree().toXmlString().toStdString());
  }

  juce::ValueTree muteSoloPlayback =
      persistentState_.getChildWithName(kMSPlaybackKey);
  if (muteSoloPlayback.isValid()) {
    msPlaybackRepository_.setStateTree(muteSoloPlayback);
  }

  juce::ValueTree mixPresMuteSolo =
      persistentState_.getChildWithName(kMixPresentationSoloMuteKey);
  if (mixPresMuteSolo.isValid()) {
    mixPresentationSoloMuteRepository_.setStateTree(mixPresMuteSolo);
  }

  juce::ValueTree filePlayback =
      persistentState_.getChildWithName(kFilePlaybackKey);
  if (filePlayback.isValid()) {
    filePlaybackRepository_.setStateTree(filePlayback);
  }
}

juce::ValueTree RendererProcessor::getTreeWithId(const juce::Identifier& id) {
  return persistentState_.getOrCreateChildWithName(id, nullptr);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new RendererProcessor();
}

void RendererProcessor::checkManualOfflineStartStop() {
// This is utilized by debug builds to perform the manual bounce operation
#if JUCE_DEBUG
  juce::SpinLock::ScopedLockType realtimeLock(realtimeLock_);
  FileExport configParams = fileExportRepository_.get();

  if (isRealtime_ != configParams.getManualExport()) {
    isRealtime_ = configParams.getManualExport();
    setNonRealtime(isRealtime_);
  }
#endif
}

void RendererProcessor::valueTreeRedirected(
    juce::ValueTree& treeWhichHasBeenChanged) {
  checkManualOfflineStartStop();
}

void RendererProcessor::valueTreePropertyChanged(
    juce::ValueTree& treeWhosePropertyHasChanged,
    const juce::Identifier& property) {
  if (property == FileExport::kManualExport) {
    checkManualOfflineStartStop();
  } else if (treeWhosePropertyHasChanged.getType() == RoomSetup::kTreeType &&
             property == RoomSetup::kSpeakerLayout) {
    configureOutputBus();
    std::string mainBusInfo = "Main Bus Output Channels: " +
                              std::to_string(getMainBusNumOutputChannels()) +
                              "\n";
    LOG_ANALYTICS(instanceId_, mainBusInfo);
  }
}
void RendererProcessor::valueTreeChildAdded(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) {
  checkManualOfflineStartStop();
}
void RendererProcessor::valueTreeChildRemoved(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved,
    int indexFromWhichChildWasRemoved) {
  checkManualOfflineStartStop();
}

void RendererProcessor::initializeMixPresentations() {
  juce::ValueTree mixPresTree =
      persistentState_.getChildWithName(kMixPresentationsKey);
  int mixPresCount = mixPresTree.getNumChildren();
  LOG_ANALYTICS(instanceId_,
                "Initializing MixPresentations. The Number of Mix "
                "Presentations found in the persistent state was: " +
                    std::to_string(mixPresCount));

  juce::OwnedArray<MixPresentation> mixPresentations;
  mixPresentationRepository_.getAll(mixPresentations);

  if (mixPresentations.size() == 0) {
    MixPresentation mixPres(juce::Uuid(), "My Mix Presentation", 1);
    mixPresentationRepository_.add(mixPres);
    activeMixPresentationRepository_.update(mixPres.getId());
    LOG_ANALYTICS(instanceId_,
                  "setStateInformation: MixPresentationRepo was empty. Created "
                  "a new mix presentation w/ Uuid " +
                      mixPres.getId().toString().toStdString() +
                      " and set it as active.");
    return;  // Early return since we just set a valid active mix
  }

  // Get the current active mix from the repository
  ActiveMixPresentation activeMix = activeMixPresentationRepository_.get();
  juce::Uuid activeMixId = activeMix.getActiveMixId();

  // Set first mix as active if current active mix is invalid (null or not
  // found)
  if (activeMixId == juce::Uuid::null() ||
      !mixPresentationRepository_.get(activeMixId).has_value()) {
    activeMixPresentationRepository_.update(mixPresentations[0]->getId());
    LOG_ANALYTICS(
        instanceId_,
        "initializeMixPresentations: Set first mix presentation as active.");
  }
}

void RendererProcessor::configureOutputBus() {
  // Reaper/VST3 does not support changing the output channel set from Stereo
  // to other layouts dynamically, so we need to reconfigure the output bus
  // when the room setup changes.
  auto hostType = juce::PluginHostType();
  if (!hostType.isReaper()) {
    LOG_ANALYTICS(instanceId_,
                  "PluginHostType is NOT Reaper. Not Configuring output bus.");
    return;
  }

  // set the default output bus
  RoomSetup roomSetup = roomSetupRepository_.get();
  std::string newChannelSetMsg;
  if (roomSetup.getSpeakerLayout().getRoomSpeakerLayout()) {
    outputChannelSet_ =
        roomSetup.getSpeakerLayout().getRoomSpeakerLayout().getChannelSet();
    newChannelSetMsg =
        "roomSetup.getSpeakerLayout() is valid. Setting outputChannelSet_ "
        "to " +
        outputChannelSet_.getDescription().toStdString() + "\n";
  } else {
    outputChannelSet_ = juce::AudioChannelSet::stereo();
    newChannelSetMsg =
        "roomSetup.getSpeakerLayout() is NOT valid. Setting "
        "outputChannelSet_ "
        "to stereo \n";
  }

  LOG_ANALYTICS(instanceId_, newChannelSetMsg);

  // Update the bus output layout
  juce::AudioProcessor::BusesLayout busesLayout = getBusesLayout();

  busesLayout.outputBuses.remove(0);
  busesLayout.outputBuses.add(outputChannelSet_);

  setBusesLayout(busesLayout);
}

void RendererProcessor::reinitializeAfterStateRestore() {
  // Broadcast initial element list/layout to plugins after state load
  syncServer_.updateClients();

  // Notify and reinitialize all child processors as needed
  for (auto& proc : audioProcessors_) {
    proc->reinitializeAfterStateRestore();
  }
}