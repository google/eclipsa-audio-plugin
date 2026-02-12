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

#====================================================================
# Set plugin code and name based on build type
#====================================================================
if (ECLIPSA_LOGIC_PRO_BUILD)
    set(RENDERER_PLUGIN_CODE "Ercl")
    set(RENDERER_PRODUCT_NAME "Eclipsa Audio Renderer for Logic Pro")
else ()
    set(RENDERER_PLUGIN_CODE "Ecrd")
    set(RENDERER_PRODUCT_NAME "Eclipsa Audio Renderer")
endif ()

juce_add_plugin(RendererPlugin
        PLUGIN_MANUFACTURER_CODE ${ECLIPSA_MANUFACTURER_CODE}
        PLUGIN_CODE ${RENDERER_PLUGIN_CODE}
        BUNDLE_ID "${ECLIPSA_RENDERER_BUNDLE_ID}"
        COMPANY_NAME "${ECLIPSA_COMPANY_NAME}"
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD ${JUCE_COPY_PLUGIN_AFTER_BUILD}
        FORMATS ${ECLIPSA_PLUGIN_FORMATS}
        PRODUCT_NAME ${RENDERER_PRODUCT_NAME}
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

eclipsa_plugin_defaults(RendererPlugin)

target_compile_definitions(RendererPlugin
        PUBLIC
        JucePlugin_IsSynth=1
        JucePlugin_Category="Synth")

if (ECLIPSA_LOGIC_PRO_BUILD)
    target_compile_definitions(RendererPlugin PRIVATE ECLIPSA_LOGIC_PRO_BUILD=1)
endif ()
#====================================================================
# Link libraries
#====================================================================
target_link_libraries(RendererPlugin
        PRIVATE
        eclipsa_common
        binary_data
        libzmq)

#====================================================================
#Testing
#====================================================================
if (CI_TEST OR INTERNAL_TEST)
    target_link_libraries(RendererPlugin
            PRIVATE
            juce::juce_cryptography)
    add_subdirectory(test)
endif ()

#====================================================================
# Copy resources for each plugin format
#====================================================================

set(_ARTEFACTS_BASE "${CMAKE_BINARY_DIR}/rendererplugin/RendererPlugin_artefacts")
set(_PLUGIN_NAME "Eclipsa Audio Renderer")

if (TARGET RendererPlugin_AAX)
    copy_resources(RendererPlugin_AAX "${_ARTEFACTS_BASE}/$<CONFIG>/AAX/${_PLUGIN_NAME}.aaxplugin")
endif ()

if (TARGET RendererPlugin_VST3)
    copy_resources(RendererPlugin_VST3 "${_ARTEFACTS_BASE}/$<CONFIG>/VST3/${_PLUGIN_NAME}.vst3")
endif ()

if (TARGET RendererPlugin_Standalone)
    copy_resources(RendererPlugin_Standalone "${_ARTEFACTS_BASE}/$<CONFIG>/Standalone")
endif ()

if (TARGET RendererPlugin_AU)
    if (ECLIPSA_LOGIC_PRO_BUILD)
        copy_resources(RendererPlugin_AU "${_ARTEFACTS_BASE}/$<CONFIG>/AU/${_PLUGIN_NAME} for Logic Pro.component")
    else ()
        copy_resources(RendererPlugin_AU "${_ARTEFACTS_BASE}/$<CONFIG>/AU/${_PLUGIN_NAME}.component")
    endif ()
endif ()
