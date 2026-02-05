# Copyright 2025 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

#====================================================================
# Imported Targets - macOS
#====================================================================
# GPAC
if (NOT TARGET vendored_gpac)
    add_library(vendored_gpac SHARED IMPORTED GLOBAL)
    set_target_properties(vendored_gpac PROPERTIES
            IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/libgpac.dylib"
    )
endif ()
# IAMF Tools
if (NOT TARGET vendored_iamf_tools)
    add_library(vendored_iamf_tools SHARED IMPORTED GLOBAL)
    set_target_properties(vendored_iamf_tools PROPERTIES
            IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/third_party/iamftools/lib/libiamf_tools.dylib"
    )
endif ()
# OBR
if (NOT TARGET vendored_obr)
    add_library(vendored_obr SHARED IMPORTED GLOBAL)
    set_target_properties(vendored_obr PROPERTIES
            IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/third_party/obr/lib/obr.dylib"
    )
endif ()