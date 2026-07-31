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
// clang-format off
#include <components/components.h>

#include "components/src/ExportErrorBanner.h"
// clang-format on

#include <gtest/gtest.h>

// Every ExportError value should map to banner text, except kNoError -- that
// one means "nothing to show" and must map to an empty string.
TEST(test_export_error_banner, messageForErrorNonEmptyExceptNoError) {
  EXPECT_TRUE(ExportErrorBanner::messageForError(kNoError).isEmpty());
  EXPECT_TRUE(
      ExportErrorBanner::messageForError(kInvalidExportPath).isNotEmpty());
  EXPECT_TRUE(
      ExportErrorBanner::messageForError(kFileWriteFailed).isNotEmpty());
  EXPECT_TRUE(
      ExportErrorBanner::messageForError(kPermissionDenied).isNotEmpty());
  EXPECT_TRUE(ExportErrorBanner::messageForError(kMuxFailed).isNotEmpty());
  EXPECT_TRUE(
      ExportErrorBanner::messageForError(kVideoLongerThanAudio).isNotEmpty());
  EXPECT_TRUE(
      ExportErrorBanner::messageForError(kAudioLongerThanVideo).isNotEmpty());
}

// Each duration-mismatch message must name its own direction, not read like a
// generic failure -- it's a warning, the export itself still succeeded -- and
// the two directions must not share wording.
TEST(test_export_error_banner, durationMismatchMessagesNameTheDirection) {
  EXPECT_TRUE(ExportErrorBanner::messageForError(kVideoLongerThanAudio)
                  .containsIgnoreCase("longer"));
  EXPECT_TRUE(ExportErrorBanner::messageForError(kAudioLongerThanVideo)
                  .containsIgnoreCase("longer"));
  EXPECT_NE(ExportErrorBanner::messageForError(kVideoLongerThanAudio),
            ExportErrorBanner::messageForError(kAudioLongerThanVideo));
}

// Permission-denied must not share generic write-failure wording -- this is
// the specific acceptance criterion that the banner distinguish causes
// rather than showing one generic message.
TEST(test_export_error_banner, permissionDeniedHasDistinctWording) {
  EXPECT_NE(ExportErrorBanner::messageForError(kPermissionDenied),
            ExportErrorBanner::messageForError(kFileWriteFailed));
}

// The banner should only (re-)appear on a fresh transition INTO an error
// state from kNoError: a brand-new failure shows it, the same error
// persisting without an intervening kNoError does not re-show it (so a user
// dismissal isn't immediately undone), and a transition INTO kNoError is a
// hide, not a show.
TEST(test_export_error_banner, shouldShowOnTransition) {
  EXPECT_TRUE(
      ExportErrorBanner::shouldShowOnTransition(kNoError, kFileWriteFailed));
  EXPECT_TRUE(ExportErrorBanner::shouldShowOnTransition(kNoError,
                                                        kVideoLongerThanAudio));
  EXPECT_TRUE(ExportErrorBanner::shouldShowOnTransition(kNoError,
                                                        kAudioLongerThanVideo));
  EXPECT_FALSE(ExportErrorBanner::shouldShowOnTransition(kFileWriteFailed,
                                                         kFileWriteFailed));
  EXPECT_FALSE(ExportErrorBanner::shouldShowOnTransition(kNoError, kNoError));
  EXPECT_FALSE(ExportErrorBanner::shouldShowOnTransition(kMuxFailed, kNoError));
}

// The banner should hide exactly when the current state is kNoError,
// regardless of what error (if any) preceded it.
TEST(test_export_error_banner, shouldHideOnTransition) {
  EXPECT_TRUE(ExportErrorBanner::shouldHideOnTransition(kNoError));
  EXPECT_FALSE(ExportErrorBanner::shouldHideOnTransition(kFileWriteFailed));
  EXPECT_FALSE(ExportErrorBanner::shouldHideOnTransition(kPermissionDenied));
}
