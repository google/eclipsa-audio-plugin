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

#include "../logger.h"

#include <gtest/gtest.h>
#include <juce_core/juce_core.h>

#include <fstream>
#include <string>
#include <thread>

// Helper function to read log file content
std::string readLogFile(const std::string& logFilePath) {
  std::ifstream file(logFilePath);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

// Test Logger Initialization
TEST(LoggerTest, InitializeLogger) {
  // Clean up existing log files before the test
  Logger::getInstance().init("testlog");  // Use JUCE standard directory
  std::vector<std::string> existingLogFiles =
      Logger::getInstance().getLogFilePaths();
  for (const auto& file : existingLogFiles) {
    std::remove(file.c_str());
  }

  // Initialize the logger
  Logger::getInstance().init("testlog");  // Use JUCE standard directory

  // Log a message to ensure the log file is created
  LOG_INFO(1, "Initialization Test Message");

  // Flush logs to file
  Logger::getInstance().flush();

  // Now retrieve the log files
  std::vector<std::string> logFiles = Logger::getInstance().getLogFilePaths();
  ASSERT_FALSE(logFiles.empty()) << "Log files were not created.";

  // Ensure the log file(s) are created and can be opened
  for (const auto& filePath : logFiles) {
    std::ifstream file(filePath);
    ASSERT_TRUE(file.is_open()) << "Failed to open log file at " << filePath;
  }

  // Attempt to re-initialize the logger with different parameters
  Logger::getInstance().init(
      "testlog2");  // Re-initialization should be prevented

  // Log a message to verify that the logger is still functioning
  LOG_INFO(1, "Initialization Test Message");

  // Flush logs to file
  Logger::getInstance().flush();

  // Read and combine the content of all log files
  std::string logContent;
  for (const auto& file : logFiles) {
    logContent += readLogFile(file);
  }

  // Check that the log contains "Initialization Test Message"
  EXPECT_NE(logContent.find("Initialization Test Message"), std::string::npos)
      << "Log message not found in log";

  // Since re-initialization should be prevented, the plugin name should still
  // be "testlog" You can check that the log files are still named with
  // "testlog"

  // Clean up
  for (const auto& file : logFiles) {
    std::remove(file.c_str());
  }
}

// Test Logging Different Severity Levels
TEST(LoggerTest, LogMessages) {
  // Clean up log files before the test
  Logger::getInstance().init("testlog", 1, boost::log::trivial::debug);
  std::vector<std::string> existingLogFiles =
      Logger::getInstance().getLogFilePaths();
  for (const auto& file : existingLogFiles) {
    std::remove(file.c_str());
  }

  LOG_DEBUG(1, "Debug Message");
  LOG_INFO(1, "Info Message");
  LOG_WARNING(2, "Warning Message");
  LOG_ERROR(2, "Error Message");

  // Ensure all messages are flushed to the file
  Logger::getInstance().flush();

  // Retrieve the log files generated during the test
  std::vector<std::string> logFiles = Logger::getInstance().getLogFilePaths();

  // Read and combine the content of all log files
  std::string logContent;
  for (const auto& file : logFiles) {
    logContent += readLogFile(file);
  }

  // Check for essential parts of the messages
  EXPECT_NE(logContent.find("[debug]"), std::string::npos)
      << "Debug level not found in log";
  EXPECT_NE(logContent.find("Debug Message"), std::string::npos)
      << "Debug message not found in log";

  EXPECT_NE(logContent.find("[info]"), std::string::npos)
      << "Info level not found in log";
  EXPECT_NE(logContent.find("Info Message"), std::string::npos)
      << "Info message not found in log";

  EXPECT_NE(logContent.find("[warning]"), std::string::npos)
      << "Warning level not found in log";
  EXPECT_NE(logContent.find("Warning Message"), std::string::npos)
      << "Warning message not found in log";

  EXPECT_NE(logContent.find("[error]"), std::string::npos)
      << "Error level not found in log";
  EXPECT_NE(logContent.find("Error Message"), std::string::npos)
      << "Error message not found in log";

  // Clean up
  for (const auto& file : logFiles) {
    std::remove(file.c_str());
  }
}

// Test Thread Safety by Logging from Multiple Threads
TEST(LoggerTest, LogFromMultipleThreads) {
  // Clean up log files before the test
  Logger::getInstance().init("testlog");
  std::vector<std::string> existingLogFiles =
      Logger::getInstance().getLogFilePaths();
  for (const auto& file : existingLogFiles) {
    std::remove(file.c_str());
  }

  auto logFunction = [](int instanceId) {
    for (int i = 0; i < 100; ++i) {
      LOG_INFO(instanceId, "Threaded Log Message");
    }
  };

  std::thread t1(logFunction, 1);
  std::thread t2(logFunction, 2);

  t1.join();
  t2.join();

  // Ensure all messages are flushed to the file
  Logger::getInstance().flush();

  // Retrieve the log files generated during the test
  std::vector<std::string> logFiles = Logger::getInstance().getLogFilePaths();

  // Read and combine the content of all log files
  std::string logContent;
  for (const auto& file : logFiles) {
    logContent += readLogFile(file);
  }

  // Count occurrences of "Threaded Log Message"
  size_t pos = 0;
  int count = 0;
  std::string target = "Threaded Log Message";
  while ((pos = logContent.find(target, pos)) != std::string::npos) {
    ++count;
    pos += target.length();
  }

  EXPECT_EQ(count, 200) << "Expected 200 log messages, but found " << count;

  // Clean up
  for (const auto& file : logFiles) {
    std::remove(file.c_str());
  }
}

// Test if init is called multiple times (ensure it doesn't reinitialize)
TEST(LoggerTest, LoggerInitMultipleCalls) {
  // Initialize logger - the path is now determined automatically by JUCE
  Logger::getInstance().init("testlog");

  // Get the actual log files created
  std::vector<std::string> logFiles = Logger::getInstance().getLogFilePaths();

  // Clean up existing log files before the test
  for (const auto& file : logFiles) {
    std::remove(file.c_str());
  }

  Logger::getInstance().init("testlog");
  Logger::getInstance().init("testlog2");  // This should be ignored

  // Log a message and check that it appears
  LOG_INFO(1, "Test message after multiple init calls");
  Logger::getInstance().flush();

  // Get the updated log files
  logFiles = Logger::getInstance().getLogFilePaths();

  // Read and combine the content of all log files
  std::string logContent;
  for (const auto& file : logFiles) {
    logContent += readLogFile(file);
  }

  // Validate that the content was logged correctly and wasn't reset
  EXPECT_NE(logContent.find("Test message after multiple init calls"),
            std::string::npos)
      << "Log message not found - logger may have been reinitialized";

  // Clean up
  for (const auto& file : logFiles) {
    std::remove(file.c_str());
  }
}