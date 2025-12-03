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

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include <boost/unordered_map.hpp>
#include <iostream>
#include <thread>

#include "ParameterMetaData.h"
#include "boost/unordered/unordered_map_fwd.hpp"
#include "zmq.hpp"

struct AudioElementUpdateData {
  float x;
  float y;
  float z;
  float loudness;
  std::array<char, 16> uuid;
  char name[64];

  AudioElementUpdateData() {
    x = 0;
    y = 0;
    z = 0;
    loudness = 0;
    memcpy(uuid.data(), juce::Uuid().getRawData(), 16);
    strcpy(name, "");
  }
};

class AudioElementPublisher {
  zmq::socket_t socket_;
  zmq::context_t context_;

 public:
  AudioElementPublisher() {
    // Create a zmq context
    context_ = zmq::context_t(1);
    // Create a zmq socket
    socket_ = zmq::socket_t(context_, ZMQ_PUB);
    // Bind to the socket
    socket_.connect("tcp://localhost:5555");
  }

  ~AudioElementPublisher() { socket_.close(); }

  void publishData(AudioElementUpdateData data) {
    // Create a zmq message
    zmq::message_t message(sizeof(AudioElementUpdateData));
    // Copy the data into the message
    memcpy(message.data(), &data, sizeof(AudioElementUpdateData));
    // Send the message
    socket_.send(message);
  }
};

struct AudioElementPluginUuidHash {
  std::size_t operator()(const std::array<char, 16>& uuid) const {
    // Hash function suggested by ChatGPT
    std::size_t hash_value = 0;
    for (const char& c : uuid) {
      hash_value ^= std::hash<char>()(c) + 0x9e3779b9 + (hash_value << 6) +
                    (hash_value >> 2);
    }
    return hash_value;
  }
};

struct AudioElementPluginUuidEqual {
  bool operator()(const std::array<char, 16>& lhs,
                  const std::array<char, 16>& rhs) const {
    return lhs == rhs;
  }
};

class AudioElementSubscriber {
  boost::unordered_map<std::array<char, 16>, AudioElementUpdateData,
                       AudioElementPluginUuidHash, AudioElementPluginUuidEqual>
      dataMap;
  juce::ReadWriteLock dataMapLock;
  zmq::socket_t socket;
  zmq::context_t context;
  std::thread listenerThread;

  // Variables for retrying the connections and handling premature deletion
  bool closing;
  std::mutex connectionMutex;
  std::condition_variable connectionCondition;

 public:
  AudioElementSubscriber() : closing(false) {
    // Start a zmq topic to listen for messages from subscriber
    // Create a zmq context
    context = zmq::context_t(1);
    // Create a zmq socket
    context.set(zmq::ctxopt::blocky, true);
    socket = zmq::socket_t(context, ZMQ_SUB);

    // Start a thread to using std::thread to listen for incoming messages and
    // update the data map
    listenerThread = std::thread([&]() {
      std::unique_lock<std::mutex> lock(connectionMutex);
      while (true) {
        if (closing) {
          socket.close();
          context.close();
          return;
        }
        // Connect to the socket
        try {
          socket.bind("tcp://localhost:5555");
          break;
        } catch (zmq::error_t e) {
          // Unable to open the socket
          // Delay for 10 seconds or until awoken, and then try again
          // We want a relatively short timeout, since we don't expect there to
          // be 2 instances and if you delete one you have to wait for the
          // timeout before things reconnect. We want some way to wake up so
          // that if a waiting renderer instance is deleted it doesn't delay
          // processing until it wakes up, which feels like lag
          connectionCondition.wait_for(lock, std::chrono::seconds(10));
        }
      }
      lock.unlock();

      // Subscribe to the topic
      socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);

      while (true) {
        // Create a zmq message
        zmq::message_t message;
        // Receive the message
        try {
          socket.recv(&message);
        } catch (zmq::error_t e) {
          // This catches ETERM here (sent by context.shutdown())
          // Note that ZMQ recommends you call socket.close() from the same
          // thread recv is in and we need to call context.close() after
          // socket.close()
          // This probably catches other errors too but I think it's better
          // to terminate since it's unclear how we might recover.
          socket.close();
          context.close();
          break;
        }
        // Copy the message into a data struct
        AudioElementUpdateData d;
        memcpy(&d, message.data(), sizeof(AudioElementUpdateData));
        // Write the data to the data map
        juce::ScopedWriteLock writeLock(dataMapLock);
        dataMap[d.uuid] = d;
      }
    });
  }

  ~AudioElementSubscriber() {
    connectionMutex.lock();
    closing = true;
    context.shutdown();  // Sends ETERM to socket.recv
    connectionCondition.notify_all();
    connectionMutex.unlock();
    listenerThread.join();
  };

  // AudioElementUpdateData getData(
  void getData(std::function<void(AudioElementUpdateData)> callback) const {
    // Read the data map
    juce::ScopedReadLock readLock(dataMapLock);
    // Iterate through each element, calling the callback
    for (auto it = dataMap.begin(); it != dataMap.end(); ++it) {
      callback(it->second);
    }
  }

  // This will need to be called occasionally, (maybe before prepareToPlay) to
  // avoid the map growing wildly
  void clearData() {
    // Write lock the data map
    juce::ScopedWriteLock writeLock(dataMapLock);
    // Clear the data map
    dataMap.clear();
  }
};