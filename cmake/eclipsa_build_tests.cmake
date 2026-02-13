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

function(eclipsa_build_tests)
    get_property(test_sources GLOBAL PROPERTY ECLIPSA_TEST_SOURCES)
    get_property(test_link_libs GLOBAL PROPERTY ECLIPSA_TEST_LINK_LIBS)

    list(REMOVE_DUPLICATES test_link_libs)

    add_executable(eclipsa_tests ${test_sources})
    target_compile_definitions(eclipsa_tests
            PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            JUCE_SILENCE_XCODE_15_LINKER_WARNING
    )

    # Vendor library path (set in toolchain)
    if (DEFINED LIBIAMF_VENDOR_PATH)
        target_link_directories(eclipsa_tests PRIVATE "${LIBIAMF_VENDOR_PATH}")
    elseif (DEFINED LIBIAMF_VENDOR_RELEASE_PATH)
        target_link_directories(eclipsa_tests PRIVATE
                "$<IF:$<CONFIG:Debug>,${LIBIAMF_VENDOR_DEBUG_PATH},${LIBIAMF_VENDOR_RELEASE_PATH}>"
        )
    endif ()

    if (WIN32 AND DEFINED ECLIPSA_TEST_DLLS)
        add_custom_command(TARGET eclipsa_tests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:eclipsa_tests>"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${ECLIPSA_TEST_DLLS}
                "$<TARGET_FILE_DIR:eclipsa_tests>/"
                COMMAND_EXPAND_LISTS
                COMMENT "Copying test DLLs to $<TARGET_FILE_DIR:eclipsa_tests>"
        )
    endif ()

    if (DEFINED LIBIAMF_INCLUDE_DIRS)
        target_include_directories(eclipsa_tests PRIVATE ${LIBIAMF_INCLUDE_DIRS})
    endif ()

    target_link_libraries(eclipsa_tests
            PRIVATE
            ${test_link_libs}
            PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
            GTest::gtest_main
    )

    if (WIN32)
        add_custom_command(TARGET eclipsa_tests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:eclipsa_tests>
                $<TARGET_FILE_DIR:eclipsa_tests>
                COMMAND_EXPAND_LISTS
                COMMENT "Copying runtime DLLs for eclipsa_tests"
        )
    endif ()

    gtest_discover_tests(eclipsa_tests DISCOVERY_MODE PRE_TEST)
endfunction()