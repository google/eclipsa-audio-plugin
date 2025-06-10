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

#include <boost/filesystem.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>
#include <mutex>
#include <string>

#define FILEINFO()                                                            \
  (std::string(boost::filesystem::path(__FILE__).filename().string()) + " " + \
   std::string(__FUNCTION__) + " " + std::to_string(__LINE__))

class Logger {
 public:
  static Logger& getInstance() {
    static Logger instance;
    return instance;
  }

  void init(const std::string& pluginName, size_t maxFileSizeMB = 5,
            boost::log::trivial::severity_level minSeverity =
                boost::log::trivial::info);
  void flush();
  std::vector<std::string> getLogFilePaths() const;

  template <typename T>
  void log(int instanceId, boost::log::trivial::severity_level level,
           const T& message) {
    BOOST_LOG_SEV(lg, level) << "[ Instance " << instanceId << "] " << message;
  }

  bool isInitialized() const { return initialized; }

 private:
  Logger() = default;
  ~Logger() = default;
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  std::mutex initMutex;
  bool initialized = false;
  std::string logFilePattern;
  boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level>
      lg;
};
#define LOG_DEBUG(instanceId, message)                              \
  Logger::getInstance().log(instanceId, boost::log::trivial::debug, \
                            "[ " + FILEINFO() + " ] " + (message))

#define LOG_INFO(instanceId, message)                              \
  Logger::getInstance().log(instanceId, boost::log::trivial::info, \
                            "[ " + FILEINFO() + " ] " + (message))

#define LOG_WARNING(instanceId, message)                              \
  Logger::getInstance().log(instanceId, boost::log::trivial::warning, \
                            "[ " + FILEINFO() + " ] " + (message))

#define LOG_ERROR(instanceId, message)                              \
  Logger::getInstance().log(instanceId, boost::log::trivial::error, \
                            "[ " + FILEINFO() + " ] " + (message))

#define LOG_ANALYTICS(instanceId, message)                         \
  Logger::getInstance().log(instanceId, boost::log::trivial::info, \
                            "[ " + FILEINFO() + " ] [Analytics] " + (message))
