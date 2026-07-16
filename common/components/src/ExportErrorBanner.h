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

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

#include "WarningBannerBase.h"
#include "data_repository/implementation/FileExportRepository.h"
#include "data_structures/src/FileExport.h"

// Dismissible warning banner that surfaces the reason the most recent export
// attempt failed, reusing WarningBannerBase's chrome. Unlike DAWWarningBanner,
// this banner starts hidden and only shows on a fresh transition into an
// error state (see shouldShowOnTransition) so a dismissal is not immediately
// undone by the same still-set error value persisting in the repository.
class ExportErrorBanner : public WarningBannerBase, public juce::ValueTree::Listener {
 public:
  explicit ExportErrorBanner(FileExportRepository* repository)
      : repository_(repository) {
    if (repository_) {
      repository_->registerListener(this);
      previousError_ = repository_->get().getExportError();
    }
    // Seed visibility from whatever error state is already recorded (e.g.
    // the editor was recreated -- some hosts do this on window close/reopen
    // -- while a prior export's failure is still on record and no fresh
    // export has run since). Without this, a banner constructed after an
    // unresolved failure would stay hidden until the NEXT export attempt,
    // silently missing the one that already happened -- the exact bug this
    // feature exists to fix.
    setVisible(previousError_ != kNoError);
  }

  ~ExportErrorBanner() override {
    if (repository_) repository_->deregisterListener(this);
  }

  void valueTreePropertyChanged(juce::ValueTree& tree,
                                const juce::Identifier& property) override {
    if (!repository_ || tree.getType() != repository_->getTree().getType())
      return;
    if (property != FileExport::kExportError) return;

    const ExportError current = repository_->get().getExportError();
    const bool shouldShow = shouldShowOnTransition(previousError_, current);
    const bool shouldHide = shouldHideOnTransition(current);
    previousError_ = current;

    juce::Component::SafePointer<ExportErrorBanner> self(this);
    juce::MessageManager::callAsync([self, shouldShow, shouldHide] {
      if (!self) return;
      if (shouldShow) {
        self->setVisible(true);
      } else if (shouldHide) {
        self->setVisible(false);
      }
      if (self->onVisibilityChanged) self->onVisibilityChanged();
    });
  }

  // Notified whenever this banner's visibility changes asynchronously as a
  // result of the listener-driven show/hide path (the button-driven dismiss
  // path already gets a parent repaint for free from
  // WarningBannerBase::buttonClicked).
  std::function<void()> onVisibilityChanged;

  // Maps the current export error to distinct, user-facing wording. Extracted
  // as a pure static function so it can be unit-tested headlessly.
  static juce::String messageForError(ExportError error) {
    switch (error) {
      case kNoError:
        return "";
      case kInvalidExportPath:
        return "Export failed: the destination folder does not exist.";
      case kFileWriteFailed:
        return "Export failed: could not write the output file.";
      case kPermissionDenied:
        return
            "Export failed: permission denied. Check that you have write "
            "access to the destination folder.";
      case kMuxFailed:
        return
            "Export failed: audio was exported, but combining it with the "
            "video failed. Check the video source and output folder.";
      default:
        return messageForError(kFileWriteFailed);
    }
  }

  // The banner only re-appears on a fresh transition into an error state from
  // kNoError (which FileOutputProcessor guarantees happens at the start of
  // every export attempt), not merely because the same error value persists.
  static bool shouldShowOnTransition(ExportError previous, ExportError current) {
    return previous == kNoError && current != kNoError;
  }

  static bool shouldHideOnTransition(ExportError current) {
    return current == kNoError;
  }

 protected:
  juce::String getMessageText() const override {
    return messageForError(repository_ ? repository_->get().getExportError()
                                       : kNoError);
  }

  // Transient dismiss: no repository write, unlike DAWWarningBanner's
  // persisted dismiss. The base class already hides the component and
  // repaints the parent on dismiss.
  void onDismiss() override {}

 private:
  FileExportRepository* repository_;
  ExportError previousError_ = kNoError;
};
