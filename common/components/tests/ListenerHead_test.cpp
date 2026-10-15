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

// Pull in the umbrella header first: several components/src headers include
// components.h themselves and only resolve correctly once the umbrella has
// fully loaded once. See the same note in Coordinates_test.cpp.
// clang-format off
#include <components/components.h>

#include "components/src/room_views/ListenerHead.h"
#include "components/src/room_views/PerspectiveRoomViews.h"
// clang-format on

#include <gtest/gtest.h>

#include <cmath>
#include <cstring>
#include <new>

namespace {

using Elevation = AudioElementSpatialLayout::Elevation;

// Surfaces above the room's centre draw over the head.
TEST(PannerHeadOrder, HeadFirstUnderRaisedPatterns) {
  for (const Elevation kElevation : {Elevation::kTent, Elevation::kArch,
                                     Elevation::kDome, Elevation::kCurve}) {
    EXPECT_TRUE(ListenerHead::drawnBeforeElevation(kElevation, 0.f));
  }
}

// With no surface, the head goes where a surface would draw over it.
TEST(PannerHeadOrder, HeadFirstWithNoPattern) {
  EXPECT_TRUE(ListenerHead::drawnBeforeElevation(Elevation::kNone, 0.f));
}

// A flat surface below the head is drawn first so the head sits over it.
TEST(PannerHeadOrder, SurfaceFirstForFlatBelowTheHead) {
  EXPECT_FALSE(ListenerHead::drawnBeforeElevation(Elevation::kFlat, -1.f));
  EXPECT_FALSE(ListenerHead::drawnBeforeElevation(Elevation::kFlat, -0.02f));
}

// A flat surface at or above the head draws over it.
TEST(PannerHeadOrder, HeadFirstForFlatAtOrAboveTheHead) {
  EXPECT_TRUE(ListenerHead::drawnBeforeElevation(Elevation::kFlat, 0.f));
  EXPECT_TRUE(ListenerHead::drawnBeforeElevation(Elevation::kFlat, 0.02f));
  EXPECT_TRUE(ListenerHead::drawnBeforeElevation(Elevation::kFlat, 1.f));
}

// Exposes the speaker draw decision, which reads the display flag.
class SpeakerDrawProbe : public AudioElementPluginTopView {
 public:
  using AudioElementPluginTopView::AudioElementPluginTopView;
  bool drawsCentreSpeaker() const {
    DrawableSpeaker speaker{};
    speaker.tag = SpeakerLookup::kC;
    return speakerIsDrawn(speaker);
  }
};

// A view built over dirty memory draws no speakers until a screen enables
// them.
TEST(PannerDisplayFlags, SpeakersOffUntilEnabled) {
  const juce::ScopedJuceInitialiser_GUI kJuce;
  SpeakerMonitorData monitorData;
  alignas(SpeakerDrawProbe) unsigned char storage[sizeof(SpeakerDrawProbe)];
  std::memset(storage, 0xFF, sizeof(storage));
  SpeakerDrawProbe* view = new (storage) SpeakerDrawProbe(monitorData);
  EXPECT_FALSE(view->drawsCentreSpeaker());
  view->setDisplaySpeakers(true);
  EXPECT_TRUE(view->drawsCentreSpeaker());
  view->~SpeakerDrawProbe();
}

// Over a Flat surface below it, the head keeps its own alpha rather than
// taking the surface's.
TEST(PannerHeadRender, HeadOverFlatSurfaceKeepsFullOpacity) {
  const juce::ScopedJuceInitialiser_GUI kJuce;
  constexpr int kSize = 400;
  SpeakerMonitorData monitorData;
  monitorData.reinitializeLoudnesses(2);
  AudioElementPluginTopView view(monitorData);
  view.setBounds(0, 0, kSize, kSize);
  view.setElevationPattern(Elevation::kFlat);
  view.setFlatHeight(-25.f);
  // Keeps the source marker clear of the head.
  AudioElementUpdateData source;
  source.x = 40.f;
  source.y = -40.f;
  source.z = -25.f;
  view.setTracks({source});

  juce::Image frame(juce::Image::ARGB, kSize, kSize, true);
  {
    juce::Graphics g(frame);
    view.paintEntireComponent(g, false);
  }

  const Coordinates::WindowData kWindow = {.leftCornerX = 0.f,
                                           .bottomCornerY = (float)kSize,
                                           .width = (float)kSize,
                                           .height = (float)kSize};
  const Coordinates::Point2D kCentre = Coordinates::toWindow(
      Coordinates::getTopViewTransform(), kWindow, {0.f, 0.f, 0.f, 1.f});
  const juce::Image kHead = IconStore::getInstance().getTopIcon();
  const int kX = juce::roundToInt(kCentre.a[0]);
  const int kY = juce::roundToInt(kCentre.a[1]);
  const juce::Colour kIcon =
      kHead.getPixelAt(kHead.getWidth() / 2, kHead.getHeight() / 2);
  const juce::Colour kSurface = frame.getPixelAt(kX + kHead.getWidth(), kY);
  const juce::Colour kExpected = kSurface.overlaidWith(kIcon);
  const juce::Colour kDrawn = frame.getPixelAt(kX, kY);
  EXPECT_NEAR(kDrawn.getRed(), kExpected.getRed(), 6);
  EXPECT_NEAR(kDrawn.getGreen(), kExpected.getGreen(), 6);
  EXPECT_NEAR(kDrawn.getBlue(), kExpected.getBlue(), 6);
}

}  // namespace
