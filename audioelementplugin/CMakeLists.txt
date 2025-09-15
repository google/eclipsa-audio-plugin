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

# Set plugin code and name based on build type
if(ECLIPSA_LOGIC_PRO_BUILD)
    set(AUDIO_ELEMENT_PLUGIN_CODE "Eacl")
    set(AUDIO_ELEMENT_PRODUCT_NAME "Eclipsa Audio Element Plugin for Logic Pro")
else()
    set(AUDIO_ELEMENT_PLUGIN_CODE "Ecae")
    set(AUDIO_ELEMENT_PRODUCT_NAME "Eclipsa Audio Element Plugin")
endif()

juce_add_plugin(AudioElementPlugin
    PLUGIN_MANUFACTURER_CODE ${ECLIPSA_MANUFACTURER_CODE}
    PLUGIN_CODE ${AUDIO_ELEMENT_PLUGIN_CODE}
    BUNDLE_ID "${ECLIPSA_PANNER_BUNDLE_ID}"
    COMPANY_NAME "${ECLIPSA_COMPANY_NAME}"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME ${AUDIO_ELEMENT_PRODUCT_NAME}
    AAX_CATEGORY "AAX_ePlugInCategory_SoundField")

target_sources(AudioElementPlugin 
    PRIVATE
        src/AudioElementPluginProcessor.cpp
        src/AudioElementPluginEditor.cpp
        src/AudioElementVersionConverter.cpp
        src/screens/RoomViewScreen.cpp
        src/screens/PositionSelectionScreen.cpp
        src/screens/TrackMonitorScreen.cpp)

target_compile_definitions(AudioElementPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_SILENCE_XCODE_15_LINKER_WARNING
        JUCE_ENABLE_LIVE_CONSTANT_EDITOR=1
        JucePlugin_Category="Spatial")

# Add DAW-specific build flags when enabled
if(ECLIPSA_LOGIC_PRO_BUILD)
    target_compile_definitions(AudioElementPlugin PRIVATE ECLIPSA_LOGIC_PRO_BUILD=1)
endif()

target_link_options(AudioElementPlugin
    PUBLIC
        "-Wl"
        "-ld_classic")


target_link_libraries(AudioElementPlugin
    PRIVATE
        substream_rdr
        processors
        components
        logger
        data_repository
        data_structures
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(CI_TEST OR INTERNAL_TEST)
    add_subdirectory(test)
endif()

# Copy resources for each plugin format
if(BUILD_AAX)
    message(STATUS "AAX build. Setting up AudioElementplugin path and copying resources.")
    set(AUDIO_PLUGIN_PATH "${CMAKE_BINARY_DIR}/audioelementplugin/AudioElementPlugin_artefacts/Release/AAX/Eclipsa Audio Element Plugin.aaxplugin")
    copy_resources(AudioElementPlugin ${AUDIO_PLUGIN_PATH})
endif()

if(BUILD_VST3)
    message(STATUS "VST3 build. Setting up AudioElementplugin path and copying resources.")
    set(AUDIO_PLUGIN_PATH "${CMAKE_BINARY_DIR}/audioelementplugin/AudioElementPlugin_artefacts/Release/VST3/Eclipsa Audio Element Plugin.vst3")
    copy_resources(AudioElementPlugin ${AUDIO_PLUGIN_PATH})
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Release build detected. Copying Audio Element Resources for AU Plugin.")
    set(AUDIO_PLUGIN_PATH "${CMAKE_BINARY_DIR}/audioelementplugin/AudioElementPlugin_artefacts/Release/AU/Eclipsa Audio Element Plugin.component")
    copy_resources(AudioElementPlugin ${AUDIO_PLUGIN_PATH})
endif()

if(NOT BUILD_AAX AND NOT BUILD_VST3 AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "No supported plugin formats detected. Skipping resource setup and resource copy for AudioElementPlugin.")
endif()
