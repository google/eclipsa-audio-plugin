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

juce_add_plugin(RendererPlugin
    PLUGIN_MANUFACTURER_CODE ${ECLIPSA_MANUFACTURER_CODE}
    PLUGIN_CODE "Ercl"
    BUNDLE_ID "${ECLIPSA_RENDERER_BUNDLE_ID}"
    COMPANY_NAME "${ECLIPSA_COMPANY_NAME}"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "Eclipsa Audio Renderer"
    AAX_CATEGORY "AAX_ePlugInCategory_SWGenerators")

target_sources(RendererPlugin 
    PRIVATE
        src/RendererProcessor.cpp
        src/RendererEditor.cpp
        src/RendererVersionConverter.cpp
        src/screens/ElementRoutingScreen.cpp
        src/screens/FileExportScreen.cpp
        src/screens/PresentationMonitorScreen.cpp
        src/screens/RoomMonitoringScreen.cpp
        src/screens/EditPresentationScreen.cpp        
        src/screens/mix_tabs/PresentationTab.cpp
        src/screens/mix_tabs/PresentationEditorTab.cpp
        src/screens/mix_tabs/MixPresentationViewPort.cpp
        src/screens/mix_tabs/PresentationEditorViewPort.cpp
        src/screens/MixPresentationTagScreen.cpp)

target_compile_definitions(RendererPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_SILENCE_XCODE_15_LINKER_WARNING
        JUCE_ENABLE_LIVE_CONSTANT_EDITOR=1
        JucePlugin_IsSynth=1
        JucePlugin_Category="Synth")

# Add DAW-specific build flags when enabled
if(ECLIPSA_LOGIC_PRO_BUILD)
    target_compile_definitions(RendererPlugin PRIVATE ECLIPSA_LOGIC_PRO_BUILD=1)
endif()

target_link_options(RendererPlugin
    PUBLIC
        "-Wl"
        "-ld_classic")

target_include_directories(RendererPlugin PUBLIC "${BOOST_LIBRARY_INCLUDES}/include")

target_link_libraries(RendererPlugin
    PRIVATE
        binary_data
        substream_rdr
        processors
        components
        logger
        data_repository
        data_structures
        Boost::lockfree
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        libzmq)

if(CI_TEST OR INTERNAL_TEST)
    target_link_libraries(RendererPlugin
        PRIVATE        
            juce::juce_cryptography)
    add_subdirectory(test)
endif()

# Copy resources for each plugin format
if(BUILD_AAX)
    message(STATUS "AAX build detected. Setting up renderer plugin path and copying resources.")
    set(RENDERER_PLUGIN_PATH "${CMAKE_BINARY_DIR}/rendererplugin/RendererPlugin_artefacts/Release/AAX/Eclipsa Audio Renderer.aaxplugin")
    copy_resources(RendererPlugin ${RENDERER_PLUGIN_PATH})
endif()

if(BUILD_VST3)
    message(STATUS "VST3 build detected. Setting up renderer plugin path and copying resources.")
    set(RENDERER_PLUGIN_PATH "${CMAKE_BINARY_DIR}/rendererplugin/RendererPlugin_artefacts/Release/VST3/Eclipsa Audio Renderer.vst3")
    copy_resources(RendererPlugin ${RENDERER_PLUGIN_PATH})
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Release build detected. Copying Renderer Resources for AU Plugin.")
    set(RENDERER_PLUGIN_PATH "${CMAKE_BINARY_DIR}/rendererplugin/RendererPlugin_artefacts/Release/AU/Eclipsa Audio Renderer.component")
    copy_resources(RendererPlugin ${RENDERER_PLUGIN_PATH})
endif()

if(NOT BUILD_AAX AND NOT BUILD_VST3 AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "No supported plugin formats detected. Skipping resource setup and resource copy for Renderer Plugin.")
endif()
