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
# Prebuilt Libraries
#====================================================================

# GPAC
add_library(gpac_impl SHARED IMPORTED GLOBAL)
set_target_properties(gpac_impl PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/libgpac.dylib"
)
add_library(vendored_gpac ALIAS gpac_impl)

# IAMF Tools
add_library(iamftools_impl SHARED IMPORTED GLOBAL)
set_target_properties(iamftools_impl PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/third_party/iamftools/lib/libiamf_tools.dylib"
)
add_library(vendored_iamf_tools ALIAS iamftools_impl)

# OBR
add_library(obr_impl SHARED IMPORTED GLOBAL)
set_target_properties(obr_impl PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/third_party/obr/lib/obr.dylib"
)
add_library(vendored_obr ALIAS obr_impl)

# Libear
add_library(libear_impl STATIC IMPORTED GLOBAL)
set_target_properties(libear_impl PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/third_party/libear/lib/libear.a"
)

#====================================================================
# Vendored Libraries
#====================================================================
set(ECLIPSA_VENDORED_LIBS
        vendored_gpac
        vendored_iamf_tools
        vendored_obr
        libzmq
        CACHE STRING ""
)