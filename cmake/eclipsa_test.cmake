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

# Function to add a test executable for CTest.
# test_name:    Test executable name.
# test_source:  Test source file name.
# test_libs:    Test link libraries as ';' separated list.
function(eclipsa_add_test test_name test_source test_libs)
    set(absolute_test_source "${CMAKE_CURRENT_SOURCE_DIR}/${test_source}")

    get_property(test_sources GLOBAL PROPERTY ECLIPSA_TEST_SOURCES)
    set_property(GLOBAL PROPERTY ECLIPSA_TEST_SOURCES "${test_sources};${absolute_test_source}")

    list(FIND test_libs "opus" opus_found)
    if (opus_found EQUAL -1)
        list(APPEND test_libs opus)
    endif ()

    list(FIND test_libs "ogg" ogg_found)
    if (ogg_found EQUAL -1)
        list(APPEND test_libs ogg)
    endif ()

    # Dependency logic for iamf/iamfdec_utils
    if ("${test_libs}" MATCHES "iamf" OR "${test_libs}" MATCHES "iamfdec_utils")
        if (NOT DEFINED LIBIAMF_INCLUDE_DIRS)
            message(WARNING "LIBIAMF_INCLUDE_DIRS not defined, but required by ${test_name}")
        endif ()
    endif ()

    if (TARGET iamf AND "${test_libs}" MATCHES "iamfdec_utils")
        list(FIND test_libs "iamf" iamf_already_linked)
        if (iamf_already_linked EQUAL -1)
            list(APPEND test_libs iamf)
        endif ()
    endif ()

    get_property(test_link_libs GLOBAL PROPERTY ECLIPSA_TEST_LINK_LIBS)
    set_property(GLOBAL PROPERTY ECLIPSA_TEST_LINK_LIBS "${test_link_libs};${test_libs}")
endfunction()