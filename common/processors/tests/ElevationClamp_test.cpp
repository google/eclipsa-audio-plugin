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

// ElevationListener against a real AudioProcessorValueTreeState.
//
// The geometry it applies is covered by ElevationGeometry_test; covered here is
// the plumbing it reads and writes through. The value tree is refreshed from a
// timer that never fires without a message loop, so these tests hold a lagging
// tree read permanently open -- the state a host reaches for as long as half a
// second once that timer has idled.

#include <gtest/gtest.h>

#include <cmath>

#include "data_repository/implementation/AudioElementSpatialLayoutRepository.h"
#include "data_structures/src/AudioElementParameterTree.h"
#include "data_structures/src/AudioElementSpatialLayout.h"
#include "data_structures/src/Elevation.h"
#include "processors/processor_base/ProcessorBase.h"

namespace {

// The parameter tree needs an AudioProcessor to attach to; nothing here
// processes audio.
class DummyPannerProcessor final : public ProcessorBase {
 public:
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
};

// The dome's radius in position-parameter units.
constexpr float kPositionExtent = 50.f;

// One panner instance: processor, parameters, spatial layout, and the listener
// wired together the way AudioElementPluginProcessor wires them.
struct PannerFixture {
  explicit PannerFixture(AudioElementSpatialLayout::Elevation elevation)
      : parameterTree(processor),
        layoutRepository(juce::ValueTree{"elevation_clamp_test"}) {
    AudioElementSpatialLayout layout = layoutRepository.get();
    layout.setElevation(elevation);
    layoutRepository.update(layout);
    listener.setListeners(&parameterTree, &layoutRepository);
  }

  // The write the drag makes: straight at the parameter, as
  // AudioElementPluginTopView::writePositionParameter does.
  void writeParameter(const juce::String& parameterName, const int value) {
    juce::RangedAudioParameter* parameter =
        parameterTree.getParameter(parameterName);
    ASSERT_NE(parameter, nullptr);
    parameter->setValueNotifyingHost(parameter->convertTo0to1((float)value));
  }

  // Reads the parameter itself rather than the accessors under test. An
  // assertion that went through those accessors would read the same stale
  // value the defect produces and pass against the very state it is meant to
  // reject.
  int readParameter(const juce::String& parameterName) const {
    const juce::RangedAudioParameter* kParameter =
        parameterTree.getParameter(parameterName);
    return kParameter == nullptr ? 0
                                 : (int)std::lround(kParameter->convertFrom0to1(
                                       kParameter->getValue()));
  }

  DummyPannerProcessor processor;
  AudioElementParameterTree parameterTree;
  AudioElementSpatialLayoutRepository layoutRepository;
  ElevationListener listener;
};

// A parameter write is visible to the accessors immediately, with no message
// loop to flush the value tree. Every clamp and read-back in the panner
// depends on this.
TEST(ElevationClamp, positionReadsSeeTheParameterWriteImmediately) {
  PannerFixture fixture(AudioElementSpatialLayout::Elevation::kFlat);

  fixture.writeParameter(AutoParamMetaData::xPosition, 37);
  fixture.writeParameter(AutoParamMetaData::yPosition, -21);

  EXPECT_EQ(fixture.parameterTree.getXPosition(),
            fixture.readParameter(AutoParamMetaData::xPosition));
  EXPECT_EQ(fixture.parameterTree.getYPosition(),
            fixture.readParameter(AutoParamMetaData::yPosition));
  EXPECT_EQ(fixture.parameterTree.getXPosition(), 37);
  EXPECT_EQ(fixture.parameterTree.getYPosition(), -21);
}

// The listener's own write must land on the parameter too, not only on the
// value tree it read from: a tree write is dropped when the property already
// holds that number.
TEST(ElevationClamp, listenerHeightWriteIsVisibleImmediately) {
  PannerFixture fixture(AudioElementSpatialLayout::Elevation::kArch);

  // The arch is the parabola through (-1,-1), (1,-1) and (0,1), so front/back
  // at the room middle puts the source at the ceiling.
  fixture.writeParameter(AutoParamMetaData::yPosition, 0);

  EXPECT_EQ(fixture.readParameter(AutoParamMetaData::zPosition), 50);
}

// A drag past the rim writes left/right and front/back as two separate
// parameter changes. Whichever order the listener sees them in, the position it
// leaves behind is on or inside the dome.
TEST(ElevationClamp, domeClampsATwoAxisDragOntoTheCircle) {
  PannerFixture fixture(AudioElementSpatialLayout::Elevation::kDome);

  fixture.writeParameter(AutoParamMetaData::xPosition, 50);
  fixture.writeParameter(AutoParamMetaData::yPosition, 45);

  const float kRadius =
      std::hypot((float)fixture.readParameter(AutoParamMetaData::xPosition),
                 (float)fixture.readParameter(AutoParamMetaData::yPosition));
  EXPECT_LE(kRadius, kPositionExtent + 1.f);
}

// The same escape, driven at the speed a flick delivers it: several targets
// written back to back, none of them separated by the timer tick that used to
// be what made the clamp see anything at all.
TEST(ElevationClamp, domeHoldsAcrossABurstOfDragEvents) {
  PannerFixture fixture(AudioElementSpatialLayout::Elevation::kDome);

  for (int step = 0; step <= 50; ++step) {
    fixture.writeParameter(AutoParamMetaData::xPosition, step);
    fixture.writeParameter(AutoParamMetaData::yPosition, step);

    const float kRadius =
        std::hypot((float)fixture.readParameter(AutoParamMetaData::xPosition),
                   (float)fixture.readParameter(AutoParamMetaData::yPosition));
    EXPECT_LE(kRadius, kPositionExtent + 1.f) << "at step " << step;
  }
}

// Flat leaves the floor plan unconstrained, so the same drag keeps the corner
// the pointer asked for. Guards against the clamp being applied everywhere.
TEST(ElevationClamp, flatLeavesTheCornerAlone) {
  PannerFixture fixture(AudioElementSpatialLayout::Elevation::kFlat);

  fixture.writeParameter(AutoParamMetaData::xPosition, 50);
  fixture.writeParameter(AutoParamMetaData::yPosition, 45);

  EXPECT_EQ(fixture.readParameter(AutoParamMetaData::xPosition), 50);
  EXPECT_EQ(fixture.readParameter(AutoParamMetaData::yPosition), 45);
}

}  // namespace
