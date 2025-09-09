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
    # Construct the absolute path to the test source file
    set(absolute_test_source "${CMAKE_CURRENT_SOURCE_DIR}/${test_source}")

    # Append the absolute test source path to a global list of sources
    get_property(test_sources GLOBAL PROPERTY ECLIPSA_TEST_SOURCES)
    set_property(GLOBAL PROPERTY ECLIPSA_TEST_SOURCES "${test_sources};${absolute_test_source}")

    # Append the test libraries to a global list of libraries
    get_property(test_link_libs GLOBAL PROPERTY ECLIPSA_TEST_LINK_LIBS)
    set_property(GLOBAL PROPERTY ECLIPSA_TEST_LINK_LIBS "${test_link_libs};${test_libs}")

    # Add codec libraries for all test targets if on macOS
    if(APPLE)
        # Append only if not already present in test_libs
        list(FIND test_libs "opus" opus_found)
        if(opus_found EQUAL -1)
            list(APPEND test_libs opus)
        endif()
        list(FIND test_libs "ogg" ogg_found)
        if(ogg_found EQUAL -1)
            list(APPEND test_libs ogg)
        endif()
    endif()

    # Add IAMF include directories if 'iamf' or 'iamfdec_utils' is requested
    if("${test_libs}" MATCHES "iamf" OR "${test_libs}" MATCHES "iamfdec_utils")
        if(NOT DEFINED LIBIAMF_INCLUDE_DIRS)
             message(WARNING "LIBIAMF_INCLUDE_DIRS not defined, but required by ${test_name}")
        endif()
    endif()
    
    # If iamfdec_utils was requested, make sure iamf is also linked (dependency)
    if(TARGET iamf AND "${test_libs}" MATCHES "iamfdec_utils")
        # Check if iamf is already in the list to avoid duplicates
        list(FIND test_libs "iamf" iamf_already_linked)
        if(iamf_already_linked EQUAL -1)
            # Link iamf privately as it's a dependency of a private lib
            list(APPEND test_libs iamf)
            message(STATUS "Also linking core 'iamf' library as dependency for ${test_name}")
        endif()
    endif()
endfunction()