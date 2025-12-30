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

#include "AudioElementPluginProcessor.h"

#include <memory>

#include "AudioElementPluginEditor.h"
#include "AudioElementVersionConverter.h"
#include "data_structures/src/AudioElementSpatialLayout.h"
#include "data_structures/src/ParameterMetaData.h"
#include "logger/logger.h"
#include "processors/audioelementplugin_publisher/AudioElementPluginDataPublisher.h"
#include "processors/mix_monitoring/TrackMonitorProcessor.h"
#include "processors/panner/Panner3DProcessor.h"
#include "processors/routing/RoutingProcessor.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

#ifdef _WIN32
// Windows doesn't need unistd.h - functionality is in io.h if needed
// #include <windows.h>
#else
#include <unistd.h>
#endif

int AudioElementPluginProcessor::instanceId_ = 0;

AudioElementPluginProcessor::AudioElementPluginProcessor()
    // For Logic Pro optimized builds: use host-wide layout
    : ProcessorBase(kIsLogicProBuild ? ProcessorBase::getHostWideLayout()
                                     : juce::AudioChannelSet::mono(),
                    ProcessorBase::getHostWideLayout()),
      persistentState_(kAudioElementSpatialPluginStateKey),
      audioElementSpatialLayoutRepository_(
          persistentState_.getOrCreateChildWithName(
              kAudioElementSpatialLayoutRepositoryStateKey, nullptr)),
      msRespository_(persistentState_.getOrCreateChildWithName(
          kMSPlaybackRepositoryStateKey, nullptr)),
      firstOutputChannel(-1),
      outputChannelCount(1),
      syncClient_(&audioElementSpatialLayoutRepository_, 2134),
      automationParametersTreeState(*this),
      trackName_("") {
#ifdef WIN32
  LoadWindowsDependencies();
#endif
  elevationListener_.setListeners(&automationParametersTreeState,
                                  &audioElementSpatialLayoutRepository_);

  audioProcessors_.push_back(std::make_unique<RemappingProcessor>(this, false));
  audioProcessors_.push_back(std::make_unique<Panner3DProcessor>(
      this, &audioElementSpatialLayoutRepository_,
      &automationParametersTreeState));
  audioProcessors_.push_back(std::make_unique<MSProcessor>(msRespository_));
  audioProcessors_.push_back(std::make_unique<TrackMonitorProcessor>(
      monitorData_, &audioElementSpatialLayoutRepository_));
  audioProcessors_.push_back(std::make_unique<AudioElementPluginDataPublisher>(
      &audioElementSpatialLayoutRepository_, &automationParametersTreeState));
  audioProcessors_.push_back(std::make_unique<SoundFieldProcessor>(
      &audioElementSpatialLayoutRepository_, &syncClient_, &ambisonicsData_));
  audioProcessors_.push_back(std::make_unique<RoutingProcessor>(
      &audioElementSpatialLayoutRepository_, &syncClient_,
      getBusesLayout().getMainOutputChannelSet().size()));

  Logger::getInstance().init("EclipsaAudioElementPlugin");

  ++instanceId_;

  LOG_ANALYTICS(instanceId_, "AudioElementPluginProcessor instantiated.");

  // Always open the max possible channels, since dynamically updating it
  // doesn't seem to work
  juce::AudioChannelSet outputChannels;
  for (int i = 0; i < 28; i++) {
    outputChannels.addChannel((juce::AudioChannelSet::ChannelType)i);
  }

  // Set a default name only if one doesn't already exist
  AudioElementSpatialLayout audioElementSpatialLayout =
      audioElementSpatialLayoutRepository_.get();
  if (audioElementSpatialLayout.getName().isEmpty()) {
    audioElementSpatialLayout.setName("Audio");
    audioElementSpatialLayoutRepository_.update(audioElementSpatialLayout);
    LOG_ANALYTICS(instanceId_, "Constructor: set default name 'Audio'");
  } else {
    LOG_ANALYTICS(instanceId_,
                  "Constructor: existing name found: '" +
                      audioElementSpatialLayout.getName().toStdString() + "'");
  }

  // Register this instance of the Audio Element plugin with the renderer plugin
  audioElementSpatialLayoutRepository_.registerListener(this);
  syncClient_.connect();
}

void AudioElementPluginProcessor::releaseResources() {}

void AudioElementPluginProcessor::updateTrackProperties(
    const TrackProperties& properties) {
  trackName_ = properties.name;
  AudioElementSpatialLayout toUpdate =
      audioElementSpatialLayoutRepository_.get();
  toUpdate.setName(trackName_);
  audioElementSpatialLayoutRepository_.update(toUpdate);
}

bool AudioElementPluginProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const {
  if (kIsLogicProBuild) {
    // Logic Pro optimized builds: use wide layout support
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in.isDisabled() || out.isDisabled()) return false;
    return Speakers::isNamedBed(in) || Speakers::isSymmetricDiscrete(in);
  } else {
    // prevent REAPER from downsizing the output channel set when
    // the probing for smaller output channel sets (i.e STEREO)
    // right after the desired/most complex layout has been assigned to the
    // output bus
    if (!allowDownSizing_ && lastOutputChannelSet_.size() >
                                 layouts.getMainInputChannelSet().size()) {
      return false;
    }

    if (layouts.getMainOutputChannelSet() != getHostWideLayout()) {
      return false;
    }

    const std::vector<juce::AudioChannelSet> supportedInputChannelSets = {
        juce::AudioChannelSet::mono(),
        juce::AudioChannelSet::stereo(),
        juce::AudioChannelSet::create5point1(),
        juce::AudioChannelSet::create5point1point2(),
        juce::AudioChannelSet::create5point1point4(),
        juce::AudioChannelSet::create7point1(),
        juce::AudioChannelSet::create7point1point2(),
        juce::AudioChannelSet::create7point1point4(),
        juce::AudioChannelSet::create9point1point6(),
        juce::AudioChannelSet::ambisonic(1),
        juce::AudioChannelSet::ambisonic(2),
        juce::AudioChannelSet::ambisonic(3)};

    if (std::find(supportedInputChannelSets.begin(),
                  supportedInputChannelSets.end(),
                  layouts.getMainInputChannelSet()) !=
        supportedInputChannelSets.end()) {
      return true;
    }

    return false;
  }
}

bool AudioElementPluginProcessor::applyBusLayouts(const BusesLayout& layouts) {
  bool check = ProcessorBase::applyBusLayouts(layouts);
  if (check) {
    // prevent REAPER from downsizing the output channel set when
    // the probing for smaller output channel sets (i.e STEREO)
    // right after the desired/most complex layout has been assigned to the
    // output bus
    allowDownSizing_ = false;
    lastOutputChannelSet_ = layouts.getMainInputChannelSet();
    std::string applyBusLayoutMessage =
        "applyBusLayouts returning TRUE with \n input: " +
        layouts.getMainInputChannelSet().getDescription().toStdString() + "\n" +
        "output: " +
        layouts.getMainOutputChannelSet().getDescription().toStdString() + "\n";

    LOG_ANALYTICS(instanceId_, applyBusLayoutMessage);
  }
  return check;
}

void AudioElementPluginProcessor::valueTreePropertyChanged(
    juce::ValueTree& treeWhosePropertyHasChanged,
    const juce::Identifier& property) {
  // An update to the audio element spatial layout repository has occurred
  // Fetch the first channel and output channels and update accordingly
  // Note that updates apply sequentially, so an update that updates first and
  // total channels will get applied twice, once changing one value and then
  // the other
  juce::ignoreUnused(treeWhosePropertyHasChanged);
  juce::ignoreUnused(property);
  AudioElementSpatialLayout audioElementSpatialLayout =
      audioElementSpatialLayoutRepository_.get();
  setOutputChannels(
      audioElementSpatialLayout.getFirstChannel(),
      audioElementSpatialLayout.getChannelLayout().getNumChannels());
  syncClient_.sendAudioElementSpatialLayoutRepository();
}

void AudioElementPluginProcessor::prepareToPlay(double sampleRate,
                                                int samplesPerBlock) {
  // unrestrict the isBusesLayoutSupported function
  // once the REAPER has finished  probing for supported output channel sets
  allowDownSizing_ = true;
  LOG_ANALYTICS(instanceId_, "Audio Element Plugin Processor prepareToPlay \n");
  for (const auto& proc : audioProcessors_) {
    proc->prepareToPlay(sampleRate, samplesPerBlock);
  }
}

void AudioElementPluginProcessor::setOutputChannels(int firstChannel,
                                                    int totalChannels) {
  // Reconfigure the output channels for the panner
  firstOutputChannel = firstChannel;
  outputChannelCount = totalChannels;
}

void AudioElementPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midi) {
  // if the first channel is set, and unmute is true, apply automation
  if (firstOutputChannel >= 0 && automationParametersTreeState.getUnmute()) {
    // get gain in decibels
    float currentVolume = automationParametersTreeState.getVolume();
    // convert to linear gain
    float linearGain = juce::Decibels::decibelsToGain(currentVolume);
    // Apply the volume to each sample in the buffer (bounded by buffer size to
    // avoid OOB)
    int lastChan = std::min(firstOutputChannel + outputChannelCount,
                            buffer.getNumChannels());
    for (int channel = firstOutputChannel; channel < lastChan; ++channel) {
      buffer.applyGain(channel, 0, buffer.getNumSamples(), linearGain);
    }
  }
  for (const auto& proc : audioProcessors_) proc->processBlock(buffer, midi);
}

juce::AudioProcessorEditor* AudioElementPluginProcessor::createEditor() {
  return new AudioElementPluginEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new AudioElementPluginProcessor();
}

void AudioElementPluginProcessor::getStateInformation(
    juce::MemoryBlock& destData) {
  LOG_ANALYTICS(instanceId_,
                "Audio Element Plugin Processor getStateInformation \n");
  // This function is called when the plugin is saved, so save the current
  juce::ValueTree automationTree = automationParametersTreeState.copyState();

  persistentState_.appendChild(automationTree, nullptr);

  // Always add the latest version attribute to the XML state
#if defined(ECLIPSA_VERSION)
  LOG_ANALYTICS(instanceId_,
                "Audio Element Plugin setting config version to \n" +
                    std::string(ECLIPSA_VERSION));
  persistentState_.setProperty("version", ECLIPSA_VERSION, nullptr);
#endif

  copyXmlToBinary(*persistentState_.createXml(), destData);
  persistentState_.removeChild(automationTree, nullptr);
}

void AudioElementPluginProcessor::setStateInformation(const void* data,
                                                      int sizeInBytes) {
  LOG_ANALYTICS(instanceId_,
                "Audio Element Plugin Processor setStateInformation \n");
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));

  if (xmlState.get() && xmlState->hasTagName(persistentState_.getType())) {
    persistentState_ = juce::ValueTree::fromXml(*xmlState);
  }

  // Check the version converstion to see if version upgrade is needed and apply
  // upgrades Do this before updating repositories since if we load the
  // repositories and then update their values, it will cause tree change events
  // on the processors, which normally updating the repositories would not do.
  AudioElementVersionConverter::convertToLatestVersion(xmlState);

  juce::ValueTree audioElementSpatialLayoutTree =
      persistentState_.getChildWithName(
          kAudioElementSpatialLayoutRepositoryStateKey);
  if (audioElementSpatialLayoutTree.isValid()) {
    // Create a temporary repository and load the values from it instead
    // to avoid changing the existing repositories ID, since if it
    // is already connected to the renderer plugin, this ID is used to
    // identify it
    AudioElementSpatialLayoutRepository tempRepository;
    tempRepository.setStateTree(audioElementSpatialLayoutTree);

    AudioElementSpatialLayout repositoryAudioElementSpatialLayout =
        audioElementSpatialLayoutRepository_.get();
    repositoryAudioElementSpatialLayout.copyValuesFrom(tempRepository.get());
    audioElementSpatialLayoutRepository_.update(
        repositoryAudioElementSpatialLayout);

    // Only sync trackName_ from loaded state if we haven't received a track
    // name from the host yet
    if (trackName_.isEmpty()) {
      trackName_ = repositoryAudioElementSpatialLayout.getName();
      LOG_ANALYTICS(instanceId_,
                    "setStateInformation: synchronized trackName_ to '" +
                        trackName_.toStdString() + "'");
    } else {
      // If we already have a track name from updateTrackProperties, update the
      // loaded state with it
      repositoryAudioElementSpatialLayout.setName(trackName_);
      audioElementSpatialLayoutRepository_.update(
          repositoryAudioElementSpatialLayout);
      LOG_ANALYTICS(instanceId_,
                    "setStateInformation: kept current trackName_ '" +
                        trackName_.toStdString() + "' and updated repository");
    }

    // Now set the current repository to the one in the persistent state
    // so that it will be written out properly. Essentially we are changing
    // the ID of the tree in the persistent state to match the ID of the tree
    // we were using when we saved
    persistentState_.removeChild(audioElementSpatialLayoutTree, nullptr);
    persistentState_.addChild(audioElementSpatialLayoutRepository_.getTree(), 0,
                              nullptr);
  }

  juce::ValueTree msPlayback =
      persistentState_.getChildWithName(kMSPlaybackRepositoryStateKey);
  if (msPlayback.isValid()) {
    msRespository_.setStateTree(msPlayback);
  }
  juce::ValueTree automationTree =
      persistentState_.getChildWithName(AutoParamMetaData::kTreeType);
  if (automationTree.isValid()) {
    automationParametersTreeState.replaceState(automationTree);
  }

  // Re-initialize components after state restoration
  reinitializeAfterStateRestore();
}

void AudioElementPluginProcessor::reinitializeAfterStateRestore() {
  // Apply output channel layout
  AudioElementSpatialLayout layout = audioElementSpatialLayoutRepository_.get();
  setOutputChannels(layout.getFirstChannel(),
                    layout.getChannelLayout().getNumChannels());

  // Broadcast layout to renderer
  syncClient_.sendAudioElementSpatialLayoutRepository();

  // Re-initialize all child processors that require post-state setup
  for (auto& proc : audioProcessors_) {
    proc->reinitializeAfterStateRestore();
  }
}