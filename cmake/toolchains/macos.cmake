# Copyright 2025 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Include guard
if (DEFINED _MACOS_TOOLCHAIN_INCLUDED)
    return()
endif ()
set(_MACOS_TOOLCHAIN_INCLUDED TRUE)

# RPATH logic moved from root
set(CMAKE_BUILD_RPATH "@loader_path/../Resources;${CMAKE_SOURCE_DIR}" CACHE STRING "")

# Initialize formats
set(_MAC_DEFAULT_FORMATS "AU")

if (BUILD_AAX)
    set(AAX_SDK_VER "2-7-0" CACHE STRING "AAX SDK Version")
    list(APPEND _MAC_DEFAULT_FORMATS "AAX")
    # Typical Mac location for vendored SDKs
    set(AAX_SDK_SEARCH_HINT "/opt/aax-sdk-${AAX_SDK_VER}" CACHE INTERNAL "")
endif ()

set(PLUGIN_FORMATS "${_MAC_DEFAULT_FORMATS}" CACHE STRING "Target plugin formats")

# Vendored Lib Path Logic
set(VENDOR_LIB_PATH "${CMAKE_SOURCE_DIR}/third_party/libiamf/lib/macos" CACHE INTERNAL "")

# Xcode 15+ classic linker (avoids duplicate symbol warnings)
add_link_options("-Wl,-ld_classic")