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

#include "PassthroughRdr.h"

std::unique_ptr<Renderer> PassthroughRdr::createPassthroughRdr(
    const IAMFSpkrLayout layout) {
  return std::unique_ptr<Renderer>(new PassthroughRdr(layout));
}

PassthroughRdr::PassthroughRdr(const IAMFSpkrLayout layout)
    : kLayout_(layout), kNumCh_(layout.getNumChannels()) {}

void PassthroughRdr::render(const FBuffer& srcBuffer, FBuffer& outBuffer) {
  outBuffer.makeCopyOf(srcBuffer);
}