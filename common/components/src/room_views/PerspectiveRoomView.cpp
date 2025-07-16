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

#include "PerspectiveRoomView.h"

#include "Coordinates.h"
#include "data_structures/src/RepositoryCollection.h"

PerspectiveRoomView::PerspectiveRoomView(
    const std::vector<FaceLookup::Face>& faces,
    const Coordinates::Mat4& transformMat,
    const std::unordered_set<SpeakerLookup::SpeakerTag>& hiddenSpeakers,
    const juce::Image& figure, const SpeakerMonitorData& monitorData,
    RepositoryCollection repos)
    : kTransformMat_(transformMat),
      kFaces_(faces),
      kHiddenSpeakers_(hiddenSpeakers),
      transformedFaces_(std::vector<DrawableFace>(faces.size())),
      monitorData_(monitorData),
      rendererPlugin_(
          true),  // Set to true if this is used in the renderer plugin.
      mixPresSMRepo_(&repos.mpSMRepo_),
      activeMixRepo_(&repos.activeMPRepo_),
      aeslRepo_(&repos.audioElementSpatialLayoutRepo_) {
  imageComponent_.setImage(figure);
  addAndMakeVisible(imageComponent_);
  setSpeakers(Speakers::kStereo);
}

PerspectiveRoomView::PerspectiveRoomView(
    const std::vector<FaceLookup::Face>& faces,
    const Coordinates::Mat4& transformMat,
    const std::unordered_set<SpeakerLookup::SpeakerTag>& hiddenSpeakers,
    const juce::Image& figure, const SpeakerMonitorData& monitorData)
    : kTransformMat_(transformMat),
      kFaces_(faces),
      kHiddenSpeakers_(hiddenSpeakers),
      transformedFaces_(std::vector<DrawableFace>(faces.size())),
      monitorData_(monitorData),
      rendererPlugin_(
          false),  // Set to false if this is not used in the renderer plugin.
      mixPresSMRepo_(nullptr),
      activeMixRepo_(nullptr),
      aeslRepo_(nullptr) {
  imageComponent_.setImage(figure);
  addAndMakeVisible(imageComponent_);
  setSpeakers(Speakers::kStereo);
}

void PerspectiveRoomView::PerspectiveRoomView::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds();

  if (recalculateStaticVertices_) {
    transformStaticVertices();
    recalculateStaticVertices_ = false;
  }

  transformDynamicVertices();

  for (const DrawableFace& face : transformedFaces_) {
    drawFace(face.faceVertPts, face.faceColour, g);
    drawGridlines(face, g);
  }

  if (displaySpeakers_) {
    updateSpeakerColours();
    for (const DrawableSpeaker& spkr : transformedSpeakers_) {
      drawSpeaker(spkr, g);
    }
  }

  // Draw central figure.
  auto imageBounds =
      bounds.withSizeKeepingCentre(imageComponent_.getImage().getWidth(),
                                   imageComponent_.getImage().getHeight());
  imageComponent_.setBounds(imageBounds);

  if (displayTracks_) {
    for (const DrawableTrack& track : transformedTracks_) {
      drawTrack(track, g);
    }
  }
}

void PerspectiveRoomView::transformStaticVertices() {
  // Pull window data from the bounds.
  Coordinates::WindowData wData = {
      .leftCornerX = 0.0f,
      .bottomCornerY = (float)getHeight(),
      .width = (float)getWidth(),
      .height = (float)getHeight(),
  };

  // Calculate window coordinates for room vertices.
  transformedFaces_.clear();
  for (const FaceLookup::Face& face : kFaces_) {
    DrawableFace newFace;
    for (int j = 0; j < face.cornerVertices.size(); ++j) {
      newFace.faceVertPts[j] =
          Coordinates::toWindow(kTransformMat_, wData, face.cornerVertices[j]);
    }
    for (int k = 0; k < face.gridlineVertices.size(); ++k) {
      newFace.gridlineVertPts[k].first = Coordinates::toWindow(
          kTransformMat_, wData, face.gridlineVertices[k].first);
      newFace.gridlineVertPts[k].second = Coordinates::toWindow(
          kTransformMat_, wData, face.gridlineVertices[k].second);
    }
    newFace.faceColour = face.faceColour;
    newFace.gridlineColour = face.gridColour;
    transformedFaces_.push_back(newFace);
  }

  // Calculate window coordinates for speaker vertices.
  transformedSpeakers_.clear();
  for (const SpeakerLookup::RoomViewSpeaker& spkr : speakers_) {
    DrawableSpeaker newSpkr;
    newSpkr.pos = Coordinates::toWindow(kTransformMat_, wData, spkr.pos);
    newSpkr.speakerLabel = spkr.name;
    newSpkr.tag = spkr.tag;
    transformedSpeakers_.push_back(newSpkr);
  }
}

void PerspectiveRoomView::transformDynamicVertices() {
  // Pull window data from the bounds.
  Coordinates::WindowData wData = {
      .leftCornerX = 0.0f,
      .bottomCornerY = (float)getHeight(),
      .width = (float)getWidth(),
      .height = (float)getHeight(),
  };

  // Calculate window coordinates for valid audio element tracks.
  transformedTracks_.clear();

  MixPresentationSoloMute mixPresSoloMute;
  if (rendererPlugin_) {
    juce::Uuid activeMix = activeMixRepo_->get().getActiveMixId();
    mixPresSoloMute = mixPresSMRepo_->get(activeMix).value();
  }
  for (const AudioElementUpdateData& data : tracks_) {
    DrawableTrack newTrack;
    Coordinates::Point4D pt = {data.x / 50, data.z / 50, -data.y / 50, 1.f};
    newTrack.pos = Coordinates::toWindow(kTransformMat_, wData, pt);
    if (rendererPlugin_) {
      newTrack.trackLoudness = assignTrackLoudness(data, mixPresSoloMute);
    } else {
      newTrack.trackLoudness = data.loudness;
    }
    newTrack.trackLabel = data.name;
    newTrack.sizeScale = getTrackScaling(pt);
    transformedTracks_.push_back(newTrack);
  }
}

void PerspectiveRoomView::drawFace(
    const std::array<Coordinates::Point2D, 4>& faceVerts, const juce::Colour& c,
    juce::Graphics& g) {
  // Draw the outline of the face.
  g.setColour(EclipsaColours::backgroundOffBlack);
  drawLine(faceVerts[0], faceVerts[1], g);
  drawLine(faceVerts[1], faceVerts[2], g);
  drawLine(faceVerts[2], faceVerts[3], g);
  drawLine(faceVerts[3], faceVerts[0], g);

  // Fill the face.
  juce::Path facePath;
  facePath.startNewSubPath(faceVerts[0].a[0], faceVerts[0].a[1]);
  facePath.lineTo(faceVerts[1].a[0], faceVerts[1].a[1]);
  facePath.lineTo(faceVerts[2].a[0], faceVerts[2].a[1]);
  facePath.lineTo(faceVerts[3].a[0], faceVerts[3].a[1]);
  facePath.closeSubPath();
  g.setColour(c);
  g.fillPath(facePath);
}

void PerspectiveRoomView::drawGridlines(const DrawableFace& face,
                                        juce::Graphics& g) {
  // NOTE: Only need to iterate over half the total gridline vertices to hash
  // the entire face.
  g.setColour(face.gridlineColour);
  for (const auto& gridline : face.gridlineVertPts) {
    drawLine(gridline.first, gridline.second, g);
  }
}

juce::Colour PerspectiveRoomView::getLoudnessColour(
    const float loudness) const {
  if (isinf(loudness) || isnan(loudness)) {
    return EclipsaColours::speakerSilentFill;
  }

  static const std::array<std::pair<float, juce::Colour>, 4> kColourLevels = {
      {{-60.f, EclipsaColours::speakerSilentFill},
       {-20.f, EclipsaColours::green},
       {-6.f, EclipsaColours::yellow},
       {-2.f, EclipsaColours::orange}}};

  for (const auto& level : kColourLevels) {
    if (loudness <= level.first) {
      return level.second;
    }
  }
  return EclipsaColours::red;
}

void PerspectiveRoomView::updateSpeakerColours() {
  // Update speaker colours based on loudness data.
  std::vector<float> loudnessData;
  monitorData_.playbackLoudness.read(loudnessData);
  for (int i = 0;
       i < std::min(transformedSpeakers_.size(), loudnessData.size()); ++i) {
    transformedSpeakers_[i].speakerColour = getLoudnessColour(loudnessData[i]);
  }
}

void PerspectiveRoomView::drawSpeaker(const DrawableSpeaker& spkr,
                                      juce::Graphics& g) {
  // Do not draw the speaker if it is hidden.
  if (kHiddenSpeakers_.contains(spkr.tag)) {
    return;
  }

  // Draw speaker outline.
  g.setColour(EclipsaColours::speakerOutline);
  g.drawRoundedRectangle(spkr.pos.a[0] - 5, spkr.pos.a[1] - 7.5, 10.f, 15.f,
                         1.f, 2.5f);
  // Fill speaker with loudness colour.
  g.setColour(spkr.speakerColour);
  g.fillRect(spkr.pos.a[0] - 5, spkr.pos.a[1] - 7.5, 10.f, 15.f);

  if (displayLabels_) {
    // Draw speaker label.
    g.setColour(EclipsaColours::tabTextGrey);
    g.drawText(spkr.speakerLabel,
               spkr.pos.a[0] - 11 + (5 - spkr.speakerLabel.length()) * 1.65,
               spkr.pos.a[1] + 9.7f, 100.f, 15.f,
               juce::Justification::verticallyCentred);
  }
}

void PerspectiveRoomView::drawLine(const Coordinates::Point2D& start,
                                   const Coordinates::Point2D& end,
                                   juce::Graphics& g, const float thickness) {
  g.drawLine(start.a[0], start.a[1], end.a[0], end.a[1], thickness);
}

void PerspectiveRoomView::drawTrack(const DrawableTrack& track,
                                    juce::Graphics& g) {
  const juce::Colour kTrackColour = getLoudnessColour(track.trackLoudness);
  g.setColour(kTrackColour);
  float width = 14.f * track.sizeScale;
  g.fillEllipse(track.pos.a[0] - width / 2, track.pos.a[1] - width / 2, width,
                width);

  // Determine the size of the outer track volume indicator based on the
  // loudness. Only draw if the track is not silent.
  if (kTrackColour != EclipsaColours::speakerSilentFill) {
    float sf2 = 6 * (1 - std::abs(track.trackLoudness) / 60.f);
    width *= sf2;
    g.setColour(kTrackColour.withAlpha(0.5f));
    g.fillEllipse(track.pos.a[0] - width / 2, track.pos.a[1] - width / 2, width,
                  width);
  }

  if (displayLabels_) {
    // Draw the track label.
    g.setColour(EclipsaColours::tabTextGrey);
    g.drawText(track.trackLabel, track.pos.a[0] - 50.f, track.pos.a[1] + 10.f,
               100.f, 15.f, juce::Justification::centred);
  }
}

const float PerspectiveRoomView::assignTrackLoudness(
    const AudioElementUpdateData& data,
    const MixPresentationSoloMute& mixPresSoloMute) {
  const juce::Uuid audioElementUuid = getTrackAudioElementUuid(data.uuid);

  // check if the audio element is even in the mix presentation
  AudioElementSoloMute audioElementSoloMute =
      mixPresSoloMute.getAudioElement(audioElementUuid);
  if (audioElementUuid != audioElementSoloMute.getId()) {
    return -300.f;  // if not in the active mix presentation, return -300 dB
  }

  if ((mixPresSoloMute.getAnySoloed() && !audioElementSoloMute.isSoloed()) ||
      audioElementSoloMute.isMuted()) {
    // if the audio element is not soloed or muted, return the loudness
    return -300.f;
  } else {
    // if the audio element is soloed or not muted, return the loudness
    return data.loudness;
  }
}
const juce::Uuid PerspectiveRoomView::getTrackAudioElementUuid(
    const std::array<char, 16>& uuid) {
  juce::uint8 rawUUID[sizeof(AudioElementUpdateData::uuid)];
  std::memcpy(rawUUID, uuid.data(), sizeof(AudioElementUpdateData::uuid));

  AudioElementSpatialLayout aesl = aeslRepo_->get(juce::Uuid(rawUUID)).value();

  return aesl.getAudioElementId();
}
