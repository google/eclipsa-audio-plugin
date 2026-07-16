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

// Pull in the umbrella header first: several components/src headers (e.g.
// Icons.h) include components.h themselves, and only resolve correctly once
// the umbrella has fully loaded once. Production code gets this for free
// because RendererEditor.h includes <components/components.h> before any
// individual components/src header; a standalone test including only
// ExportErrorBanner.h would otherwise hit that circular-include ordering.
#include <components/components.h>

#include "components/src/ExportErrorBanner.h"

#include <gtest/gtest.h>

TEST(test_export_error_banner, messageForErrorNonEmptyExceptNoError) {
  EXPECT_TRUE(ExportErrorBanner::messageForError(kNoError).isEmpty());
  EXPECT_TRUE(
      ExportErrorBanner::messageForError(kInvalidExportPath).isNotEmpty());
  EXPECT_TRUE(
      ExportErrorBanner::messageForError(kFileWriteFailed).isNotEmpty());
  EXPECT_TRUE(
      ExportErrorBanner::messageForError(kPermissionDenied).isNotEmpty());
  EXPECT_TRUE(ExportErrorBanner::messageForError(kMuxFailed).isNotEmpty());
}

TEST(test_export_error_banner, permissionDeniedHasDistinctWording) {
  EXPECT_NE(ExportErrorBanner::messageForError(kPermissionDenied),
           ExportErrorBanner::messageForError(kFileWriteFailed));
}

TEST(test_export_error_banner, shouldShowOnTransition) {
  EXPECT_TRUE(
      ExportErrorBanner::shouldShowOnTransition(kNoError, kFileWriteFailed));
  EXPECT_FALSE(ExportErrorBanner::shouldShowOnTransition(kFileWriteFailed,
                                                         kFileWriteFailed));
  EXPECT_FALSE(
      ExportErrorBanner::shouldShowOnTransition(kNoError, kNoError));
  EXPECT_FALSE(
      ExportErrorBanner::shouldShowOnTransition(kMuxFailed, kNoError));
}

TEST(test_export_error_banner, shouldHideOnTransition) {
  EXPECT_TRUE(ExportErrorBanner::shouldHideOnTransition(kNoError));
  EXPECT_FALSE(ExportErrorBanner::shouldHideOnTransition(kFileWriteFailed));
  EXPECT_FALSE(ExportErrorBanner::shouldHideOnTransition(kPermissionDenied));
}
