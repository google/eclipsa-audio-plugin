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

#pragma once

#include <string_view>

class SvgMap {
 public:
  enum Icon {
    kVolume,
    kPlay,
    kPause,
    kStop,
  };

  static constexpr std::string_view get(Icon icon) noexcept {
    switch (icon) {
      case kVolume:
        return R"(
<svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<path d="M3 8.99998V15H7L12 20V3.99998L7 8.99998H3ZM10 8.82998V15.17L7.83 13H5V11H7.83L10 8.82998ZM16.5 12C16.5 10.23 15.48 8.70998 14 7.96998V16.02C15.48 15.29 16.5 13.77 16.5 12ZM14 3.22998V5.28998C16.89 6.14998 19 8.82998 19 12C19 15.17 16.89 17.85 14 18.71V20.77C18.01 19.86 21 16.28 21 12C21 7.71998 18.01 4.13998 14 3.22998Z" fill="rgb(190, 201, 200)\"/>
</svg>
)";
      case kPlay:
        return R"(
<svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<path d="M8.5 8.64L13.77 12L8.5 15.36V8.64ZM6.5 5V19L17.5 12L6.5 5Z" fill="black"/>
</svg>
)";
      case kPause:
        return R"(
<svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<path d="M6 19H10V5H6V19ZM14 5V19H18V5H14Z" fill="black"/>
</svg>
)";
      case kStop:
        return R"(
<svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<path d="M16 8V16H8V8H16ZM18 6H6V18H18V6Z" fill="black"/>
</svg>
)";
    }
  }
};