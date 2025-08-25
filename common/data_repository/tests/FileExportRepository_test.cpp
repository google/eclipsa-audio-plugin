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

#include "data_repository/implementation/FileExportRepository.h"

#include <gtest/gtest.h>

class TestFileExportRepository : public FileExportRepository {
 public:
  TestFileExportRepository() : FileExportRepository(juce::ValueTree{"test"}) {}
};

// Sanity check basic usage of the repository
TEST(test_file_export_repository, default_export) {
  TestFileExportRepository repositoryInstance;
  FileExport testExportData;
  FileExport defaultExportData = repositoryInstance.get();
  ASSERT_EQ(testExportData.getAudioCodec(), defaultExportData.getAudioCodec());
}

TEST(test_file_export_repository, mod_export) {
  TestFileExportRepository repositoryInstance;
  FileExport testExportData(0, 0, "/test/path/test.wav", "/test/path", IAMF,
                            LPCM, 16, 44100, true, true, true,
                            "/test/path/test.mp4", "/test/path", true,
                            FileProfile::SIMPLE, 8, 64000, 24, false);
  repositoryInstance.update(testExportData);
  FileExport updatedExportData = repositoryInstance.get();
  ASSERT_EQ(testExportData.getAudioCodec(), updatedExportData.getAudioCodec());
  ASSERT_EQ(testExportData.getExportFile(), updatedExportData.getExportFile());
  ASSERT_EQ(testExportData.getSampleRate(), updatedExportData.getSampleRate());
  ASSERT_EQ(testExportData.getBitDepth(), updatedExportData.getBitDepth());
  ASSERT_EQ(testExportData.getExportVideo(),
            updatedExportData.getExportVideo());
  ASSERT_EQ(testExportData.getFlacCompressionLevel(),
            updatedExportData.getFlacCompressionLevel());
  ASSERT_EQ(testExportData.getOpusTotalBitrate(),
            updatedExportData.getOpusTotalBitrate());
  ASSERT_EQ(testExportData.getLPCMSampleSize(),
            updatedExportData.getLPCMSampleSize());
}
