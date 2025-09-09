/*
 * Copyright (c) 2025 Google LLC
 *
 * This source code is subject to the terms of the BSD 3-Clause Clear License,
 * which you can find in the LICENSE file, and the Open Binaural Renderer
 * Patent License 1.0, which you can find in the PATENTS file.
 */

#ifndef BINAURAL_FILTERS_WRAPPER_H_
#define BINAURAL_FILTERS_WRAPPER_H_

#include <memory>
#include <string>

namespace obr {

class BinauralFiltersWrapper {
 public:
  BinauralFiltersWrapper();
  ~BinauralFiltersWrapper();

  std::unique_ptr<std::string> GetFile(const std::string& filename) const;
};

}  // namespace obr

#endif  // BINAURAL_FILTERS_WRAPPER_H_
