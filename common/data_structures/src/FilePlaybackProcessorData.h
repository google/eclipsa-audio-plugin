#pragma once
#include "FilePlayback.h"
#include "RealtimeDataType.h"

struct FilePlaybackProcessorData {
  RealtimeDataType<FilePlayback::ProcessorState> processorState;
  RealtimeDataType<juce::uint64> fileDuration_s;
  RealtimeDataType<float> currFilePosition;
};