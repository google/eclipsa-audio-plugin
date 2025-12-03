/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "data_repository/implementation/AudioElementRepository.h"
#include "data_repository/implementation/AudioElementSpatialLayoutRepository.h"
#include "data_structures/src/AudioElement.h"

class AudioElementPluginListener {
 public:
  virtual void audioElementsUpdated() = 0;
};

class AudioElementPluginSyncClient : public juce::InterprocessConnection {
 protected:
  AudioElementRepository
      rendererAudioElements_;  // Make protected so it can be set in unit tests

 private:
  AudioElementSpatialLayoutRepository* toRegister_;
  std::vector<AudioElementPluginListener*> listeners_;
  juce::CriticalSection rendererAudioElementsLock_;
  int port_;
  bool initialized_;
  std::atomic_bool connected_{false};
  std::atomic_bool terminationRequested_{false};
  std::thread connectionThread_;

 public:
  AudioElementPluginSyncClient(
      AudioElementSpatialLayoutRepository* AudioElementSpatialLayoutRepository,
      int port)
      : toRegister_(AudioElementSpatialLayoutRepository),
        port_(port),
        initialized_(false) {}

  ~AudioElementPluginSyncClient() override {}

  void disconnectClient() {
    juce::ScopedLock lock(rendererAudioElementsLock_);
    listeners_.clear();
    terminationRequested_ = true;
    connectionThread_.join();
    disconnect(30000);
  }

  void registerListener(AudioElementPluginListener* listener) {
    juce::ScopedLock lock(rendererAudioElementsLock_);
    listeners_.push_back(listener);
  }

  void removeListener(AudioElementPluginListener* listener) {
    juce::ScopedLock lock(rendererAudioElementsLock_);
    listeners_.erase(
        std::remove(listeners_.begin(), listeners_.end(), listener),
        listeners_.end());
  }

  void getAudioElements(juce::OwnedArray<AudioElement>& elements) {
    juce::ScopedLock lock(rendererAudioElementsLock_);
    juce::OwnedArray<AudioElement> elementsInternal;
    rendererAudioElements_.getAll(elementsInternal);

    // Make copies of the audio elements, since they could be modified
    // by a re-write of the tree in another thread
    for (auto element : elementsInternal) {
      elements.add(new AudioElement(*element));
    }
  }

  std::optional<AudioElement> getElement(juce::Uuid id) {
    juce::ScopedLock lock(rendererAudioElementsLock_);
    if (rendererAudioElements_.getValueTree().isValid()) {
      return rendererAudioElements_.get(id);
    } else {
      return std::nullopt;
    }
  }

  // EJ 10/31

  void tryConnect() {  // Start a thread here to try and perform the connection
    connectionThread_ = std::thread([this] {
      // Try and connect, checking to see if deletion
      // has been requested and we need to stop
      while (!connected_ && !terminationRequested_) {
        rendererAudioElementsLock_.enter();
        bool connected = connectToSocket("localhost", port_, 5000);
        if (connected) {
          rendererAudioElementsLock_.exit();
          connected_ = true;
          sendAudioElementSpatialLayoutRepository();
          return;
        }
        rendererAudioElementsLock_.exit();
        std::this_thread::sleep_for(std::chrono::seconds(1));
      };
    });
  }

  void connect() { tryConnect(); }

  void connectionMade() override {}

  void connectionLost() override {
    connected_ = true;  // Avoid possible deadlock if connection lost got called
                        // while we are trying to connect. I don't think this
                        // will happen in practice, but better safe
    connectionThread_.join();

    connected_ = false;
    if (!terminationRequested_) tryConnect();
  }

  void messageReceived(const juce::MemoryBlock& message) override {
    juce::MemoryInputStream stream(message, false);
    juce::ValueTree repository = juce::ValueTree::readFromStream(stream);
    juce::ScopedLock lock(rendererAudioElementsLock_);
    rendererAudioElements_.setStateTree(repository);

    for (auto listener : listeners_) {
      listener->audioElementsUpdated();
    }
    initialized_ = true;
  }

  void sendAudioElementSpatialLayoutRepository() {
    if (connected_) {
      juce::MemoryBlock block;
      juce::MemoryOutputStream stream(block, false);
      toRegister_->writeToStream(stream);
      sendMessage(block);
    }
  }
};
