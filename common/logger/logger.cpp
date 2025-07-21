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

#include "logger.h"

#include <juce_core/juce_core.h>

#include <boost/filesystem.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <iostream>
#include <regex>

void Logger::init(const std::string& pluginName, size_t maxFileSizeMB,
                  boost::log::trivial::severity_level minSeverity) {
  std::lock_guard<std::mutex> lock(
      initMutex);           // Ensure thread-safe initialization
  if (initialized) return;  // Prevent re-initialization
  initialized = true;

  try {
    // Create the full path: ~/Library/Application
    // Support/Eclipsa/Logs/{pluginName}/
    juce::File logDir =
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("Eclipsa")
            .getChildFile("Logs")
            .getChildFile(pluginName);

    std::string logDirPath = logDir.getFullPathName().toStdString();

    // JUCE's createDirectory() creates all parent directories recursively
    if (!logDir.exists()) {
      if (!logDir.createDirectory()) {
        std::cerr << "Error: Failed to create log directory: " << logDirPath
                  << std::endl;
        throw std::runtime_error("Failed to create log directory: " +
                                 logDirPath);
      }
    }

    // Set up the file sink with rotation
    boost::filesystem::path logDirPathBoost(logDirPath);
    std::string filePattern =
        (logDirPathBoost / (pluginName + "_%N.log")).string();
    auto fileSink = boost::log::add_file_log(
        boost::log::keywords::open_mode = std::ios::out,
        boost::log::keywords::file_name = filePattern,
        boost::log::keywords::rotation_size = maxFileSizeMB * 1024 * 1024,
        boost::log::keywords::time_based_rotation =
            boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
        boost::log::keywords::format =
            "[%TimeStamp%] [%Severity%] [%ThreadID%]: %Message%",
        boost::log::keywords::auto_flush = true);

    // Add a custom file collector with a retention policy
    fileSink->locked_backend()->set_file_collector(
        boost::log::sinks::file::make_collector(
            boost::log::keywords::target =
                logDirPath,  // Use the same directory as the log file
            boost::log::keywords::max_size =
                50 * 1024 * 1024,  // Total size for all logs
            boost::log::keywords::min_free_space =
                100 * 1024 * 1024,  // Free space to trigger file removal
            boost::log::keywords::max_files =
                5  // Max number of rotated log files to keep
            ));

    // Trigger the collector to scan for existing files and enforce retention
    // policies
    fileSink->locked_backend()->scan_for_files();

    // Store the log file pattern
    logFilePattern = filePattern;

    // Add common attributes
    boost::log::add_common_attributes();

    // Set the severity filter using the provided parameter
    boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                        minSeverity);
  } catch (const std::exception& e) {
    std::cerr << "Error initializing logger: " << e.what() << std::endl;
  }
}

std::vector<std::string> Logger::getLogFilePaths() const {
  std::vector<std::string> logFiles;
  boost::filesystem::path filePattern(logFilePattern);
  boost::filesystem::path logDir = filePattern.parent_path();
  std::string filenamePattern = filePattern.filename().string();

  // Replace %N with a regex pattern to match rotation numbers
  std::string regexPattern = filenamePattern;
  std::regex placeholder("%N");
  regexPattern = std::regex_replace(regexPattern, placeholder, R"(\d+)");

  // Create a regex object
  std::regex fileRegex(regexPattern);

  // Iterate over files in the log directory
  for (auto& entry : boost::filesystem::directory_iterator(logDir)) {
    if (boost::filesystem::is_regular_file(entry.status())) {
      std::string filename = entry.path().filename().string();
      if (std::regex_match(filename, fileRegex)) {
        logFiles.push_back(entry.path().string());
      }
    }
  }

  return logFiles;
}

void Logger::flush() {
  // Implementation to flush the logs
  boost::log::core::get()->flush();
}