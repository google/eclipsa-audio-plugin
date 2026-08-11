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

// Dismissible warning banner that surfaces failures or warnings
// that occur during file export. Dismissing it clears the underlying
// ExportError in the repository (see onDismiss), the same way starting a
// new export does -- so, like DAWWarningBanner, a dismissal survives the
// editor being recreated, but without needing a second persisted flag: once
// dismissed there is no error left on record to re-seed visibility from.
// This banner also gates re-showing on a fresh transition into an error
// state (see shouldShowOnTransition) so the same still-set error value
// persisting in the repository doesn't undo a dismissal on its own.
class ExportErrorBanner : public WarningBannerBase,
                          public juce::ValueTree::Listener {
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
    // silently missing the one that already happened. A dismissed failure
    // does not re-trigger this: onDismiss clears the error itself, so
    // there is nothing left on record to seed visibility from.
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

    // This listener can fire off the message thread: FileOutputProcessor
    // updates the repository from setNonRealtime(), which JUCE hosts may
    // call from a realtime/render thread, and ValueTree::Listener callbacks
    // run synchronously on whichever thread triggered the change. Defer
    // every bit of Component-touching work -- including obtaining a safe
    // self-reference -- to the message thread. `weakSelf_` was already
    // constructed on the message thread (at banner-construction time), so
    // capturing a copy of it here is just an atomic refcount bump, unlike
    // constructing a fresh SafePointer<ExportErrorBanner>(this) on this
    // thread would be, which touches Component's WeakReference::Master and
    // is only safe on the message thread.
    juce::MessageManager::callAsync([weakSelf = weakSelf_, current] {
      if (!weakSelf) return;
      const bool shouldShow =
          shouldShowOnTransition(weakSelf->previousError_, current);
      const bool shouldHide = shouldHideOnTransition(current);
      weakSelf->previousError_ = current;
      if (shouldShow) {
        weakSelf->setVisible(true);
      } else if (shouldHide) {
        weakSelf->setVisible(false);
      }
      if (weakSelf->onVisibilityChanged) weakSelf->onVisibilityChanged();
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
        return "Export failed: permission denied. Check that you have write "
               "access to the destination folder.";
      case kMuxFailed:
        return "Export failed: audio was exported, but combining it with the "
               "video failed. Check the video source and output folder.";
      case kVideoLongerThanAudio:
        return "The video is longer than the exported audio. The file was "
               "exported, but the audio will end before the video during "
               "playback.";
      case kAudioLongerThanVideo:
        return "The exported audio is longer than the video. The file was "
               "exported, but the audio has been shortened to fit the video "
               "file.";
      default:
        return messageForError(kFileWriteFailed);
    }
  }

  // The banner only re-appears on a fresh transition into an error state from
  // kNoError (which FileOutputProcessor guarantees happens at the start of
  // every export attempt), not merely because the same error value persists.
  static bool shouldShowOnTransition(ExportError previous,
                                     ExportError current) {
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

  // The base class already hides the component and repaints the parent on
  // dismiss; this clears the underlying error so it does not reappear on
  // editor reopen (mirroring DAWWarningBanner::onDismiss's persisted
  // dismiss, without needing a second persisted flag -- once the error is
  // cleared here, there is nothing left on record to re-seed visibility
  // from on the next construction).
  void onDismiss() override {
    if (!repository_) return;
    FileExport config = repository_->get();
    config.setExportError(kNoError);
    repository_->update(config);
  }

 private:
  FileExportRepository* repository_;
  ExportError previousError_ = kNoError;
  // Constructed once on the message thread (this banner is always built as
  // part of editor construction) and only ever copied afterward -- see the
  // comment in valueTreePropertyChanged for why that distinction matters.
  const juce::Component::SafePointer<ExportErrorBanner> weakSelf_{this};
};
