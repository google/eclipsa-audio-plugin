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

#include "components/src/room_views/PerspectiveRoomViews.h"
// clang-format on

#include <gtest/gtest.h>

#include <cmath>

#include "data_structures/src/AudioElementParameterTree.h"
#include "processors/processor_base/ProcessorBase.h"

namespace {

using Elevation = AudioElementSpatialLayout::Elevation;

constexpr int kSize = 400;

// The parameter tree needs an AudioProcessor to attach to; nothing here
// processes audio.
class DummyPannerProcessor final : public ProcessorBase {
 public:
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
};

// Exposes where the source marker was last drawn, so input can target it.
class SourceProbe : public AudioElementPluginTopView {
 public:
  using AudioElementPluginTopView::AudioElementPluginTopView;
  juce::Point<float> sourceWindowPosition() const {
    return {transformedTracks_[0].pos.a[0], transformedTracks_[0].pos.a[1]};
  }
};

// A panner over a Flat surface with the source above it, so the marker and
// both halves of the height indicator are all on screen when shown.
struct PannerFixture {
  PannerFixture() : parameterTree(processor), view(monitorData) {
    monitorData.reinitializeLoudnesses(2);
    view.setBounds(0, 0, kSize, kSize);
    view.setElevationPattern(Elevation::kFlat);
    view.setFlatHeight(-25.f);
    view.setParameterTree(&parameterTree);
    view.setTracks({source()});
  }

  static AudioElementUpdateData source() {
    AudioElementUpdateData data;
    data.x = 20.f;
    data.y = 20.f;
    data.z = 25.f;
    return data;
  }

  juce::Image paint() {
    juce::Image frame(juce::Image::ARGB, kSize, kSize, true);
    juce::Graphics g(frame);
    view.paintEntireComponent(g, false);
    return frame;
  }

  // Reads the parameter itself rather than the view or the tree accessors.
  int readParameter(const juce::String& parameterName) const {
    const juce::RangedAudioParameter* kParameter =
        parameterTree.getParameter(parameterName);
    return kParameter == nullptr ? 0
                                 : (int)std::lround(kParameter->convertFrom0to1(
                                       kParameter->getValue()));
  }

  juce::MouseEvent mouseEvent(const juce::Point<float> position,
                              const juce::Point<float> downPosition,
                              const bool dragged) {
    const juce::Time kNow = juce::Time::getCurrentTime();
    return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),
                            position, {}, 0.f, 0.f, 0.f, 0.f, 0.f, &view, &view,
                            kNow, downPosition, kNow, 1, dragged);
  }

  // Presses on the source and drags it 60 pixels to the right. mouseDown grabs
  // focus on a view that is not on screen, which JUCE logs as an assertion.
  void dragSource() {
    const juce::Point<float> kStart = view.sourceWindowPosition();
    const juce::Point<float> kEnd = kStart.translated(60.f, 0.f);
    view.mouseDown(mouseEvent(kStart, kStart, false));
    view.mouseDrag(mouseEvent(kEnd, kStart, true));
    view.mouseUp(mouseEvent(kEnd, kStart, true));
  }

  const juce::ScopedJuceInitialiser_GUI kJuce;
  SpeakerMonitorData monitorData;
  DummyPannerProcessor processor;
  AudioElementParameterTree parameterTree;
  SourceProbe view;
};

int countDifferingPixels(const juce::Image& a, const juce::Image& b) {
  int differing = 0;
  for (int y = 0; y < a.getHeight(); ++y) {
    for (int x = 0; x < a.getWidth(); ++x) {
      if (a.getPixelAt(x, y) != b.getPixelAt(x, y)) {
        ++differing;
      }
    }
  }
  return differing;
}

// A view no screen has enabled the source on paints exactly what it paints
// with no source.
TEST(PannerSourceDisplay, SourceOffUntilEnabled) {
  PannerFixture fixture;
  const juce::Image kWithSource = fixture.paint();
  fixture.view.setTracks({});
  EXPECT_EQ(countDifferingPixels(kWithSource, fixture.paint()), 0);
}

// A shown source draws its marker and height indicator over the empty room.
TEST(PannerSourceDisplay, ShownSourceIsDrawn) {
  PannerFixture fixture;
  fixture.view.setDisplaySource(true);
  const juce::Image kWithSource = fixture.paint();
  fixture.view.setTracks({});
  EXPECT_GT(countDifferingPixels(kWithSource, fixture.paint()), 0);
}

// Hiding a shown source removes the marker and every height-indicator line.
TEST(PannerSourceDisplay, HiddenSourceLeavesNoTrace) {
  PannerFixture fixture;
  fixture.view.setDisplaySource(true);
  fixture.paint();
  fixture.view.setDisplaySource(false);
  const juce::Image kHidden = fixture.paint();
  fixture.view.setTracks({});
  EXPECT_EQ(countDifferingPixels(kHidden, fixture.paint()), 0);
}

// Dragging a shown source moves it.
TEST(PannerSourceDisplay, ShownSourceDrags) {
  PannerFixture fixture;
  fixture.view.setDisplaySource(true);
  fixture.paint();
  const int kX = fixture.readParameter(AutoParamMetaData::xPosition);
  fixture.dragSource();
  EXPECT_NE(fixture.readParameter(AutoParamMetaData::xPosition), kX);
}

// A press and drag where a hidden source sits writes no position.
TEST(PannerSourceDisplay, HiddenSourceIgnoresDrag) {
  PannerFixture fixture;
  fixture.view.setDisplaySource(true);
  fixture.paint();
  fixture.view.setDisplaySource(false);
  const int kX = fixture.readParameter(AutoParamMetaData::xPosition);
  const int kY = fixture.readParameter(AutoParamMetaData::yPosition);
  fixture.dragSource();
  EXPECT_EQ(fixture.readParameter(AutoParamMetaData::xPosition), kX);
  EXPECT_EQ(fixture.readParameter(AutoParamMetaData::yPosition), kY);
}

// Arrow keys nudge a shown source and pass through while it is hidden.
TEST(PannerSourceDisplay, HiddenSourceIgnoresNudge) {
  PannerFixture fixture;
  const juce::KeyPress kRight(juce::KeyPress::rightKey);
  const int kX = fixture.readParameter(AutoParamMetaData::xPosition);

  EXPECT_FALSE(fixture.view.keyPressed(kRight));
  EXPECT_EQ(fixture.readParameter(AutoParamMetaData::xPosition), kX);

  fixture.view.setDisplaySource(true);
  EXPECT_TRUE(fixture.view.keyPressed(kRight));
  EXPECT_NE(fixture.readParameter(AutoParamMetaData::xPosition), kX);
}

// The wheel raises a shown source and leaves a hidden one's height alone.
TEST(PannerSourceDisplay, HiddenSourceIgnoresWheel) {
  PannerFixture fixture;
  juce::MouseWheelDetails wheel{};
  wheel.deltaY = 1.f;
  const juce::Point<float> kCentre{kSize / 2.f, kSize / 2.f};
  const int kZ = fixture.readParameter(AutoParamMetaData::zPosition);

  fixture.view.mouseWheelMove(fixture.mouseEvent(kCentre, kCentre, false),
                              wheel);
  EXPECT_EQ(fixture.readParameter(AutoParamMetaData::zPosition), kZ);

  fixture.view.setDisplaySource(true);
  fixture.view.mouseWheelMove(fixture.mouseEvent(kCentre, kCentre, false),
                              wheel);
  EXPECT_NE(fixture.readParameter(AutoParamMetaData::zPosition), kZ);
}

}  // namespace
