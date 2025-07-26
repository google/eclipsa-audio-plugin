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

#include "MessagingThread.h"

MessagingThread::MessagingThread(const juce::String& threadName)
    : juce::Thread(threadName) {
  startThread();  // Start the thread immediately
}

MessagingThread::~MessagingThread() {
  signalThreadShouldExit();
  notify();          // Notify the thread to wake up if it's waiting
  stopThread(1000);  // Stop the thread gracefully
}

void MessagingThread::run() {
  // Create a local publisher instance
  std::unique_ptr<AudioElementPublisher> localPublisher =
      std::make_unique<AudioElementPublisher>();
  while (!threadShouldExit()) {
    // Wait for data to be pushed to the queue
    wait(-1);

    std::vector<AudioElementUpdateData> localDataQueue;
    {
      juce::ScopedLock lock(queueLock_);
      localDataQueue.swap(dataQueue_);  // Swap to avoid holding the lock
    }
    // Process all data in the queue
    while (!localDataQueue.empty()) {
      AudioElementUpdateData data = localDataQueue.front();
      localDataQueue.erase(localDataQueue.begin());

      // Publish the data
      localPublisher->publishData(data);
    }
  }
}

void MessagingThread::pushAudioElementUpdateData(
    const AudioElementUpdateData& data) {
  dataQueue_.push_back(data);
  notify();
}