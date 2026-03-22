cmake_minimum_required(VERSION 3.24)

project(JucePlugins VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_XCODE_GENERATE_SCHEME ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# =========================================================
# Output names for Unity DLLs
# =========================================================
set(VBAP_UNITY_OUTPUT_NAME "audiopluginVBAP"
    CACHE STRING "Output filename for the Unity VBAP plugin")

set(BINAURAL_UNITY_OUTPUT_NAME "audiopluginBinaural"
    CACHE STRING "Output filename for the Unity Binaural plugin")

# Optional: where Unity copies the built plugin automatically
# Recommended:
#   <YourUnityProject>/Assets/Plugins/x86_64
set(UNITY_PLUGIN_COPY_DIR "${CMAKE_BINARY_DIR}/UnityPluginStaging"
    CACHE PATH "Where to copy built Unity plugins when COPY_PLUGIN_AFTER_BUILD is TRUE")

# =========================================================
# CPM + JUCE
# =========================================================
set(CPM_DOWNLOAD_VERSION 0.34.0)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if (NOT EXISTS ${CPM_DOWNLOAD_LOCATION})
    message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
        ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})

CPMAddPackage(
    NAME juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG origin/master
)

# =========================================================
# Common source files shared by both plugins
# =========================================================
set(COMMON_SOURCE_FILES
    Source/Common/VbapEngine2D.h
    Source/Common/HrirBank.h
    Source/Common/HrirBank.cpp
)

# =========================================================
# Helper function to create one JUCE plugin target
# =========================================================
function(add_juce_unity_plugin TARGET_NAME PRODUCT_NAME UNITY_OUTPUT_NAME PLUGIN_CODE PROCESSOR_H PROCESSOR_CPP)
    juce_add_plugin(${TARGET_NAME}
        COMPANY_NAME "YourCompany"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        JUCE_VST3_CAN_REPLACE_VST2 FALSE

        COPY_PLUGIN_AFTER_BUILD TRUE
        UNITY_COPY_DIR "${UNITY_PLUGIN_COPY_DIR}"

        PLUGIN_MANUFACTURER_CODE JuPl
        PLUGIN_CODE ${PLUGIN_CODE}

        FORMATS VST3 Standalone Unity
        PRODUCT_NAME "${PRODUCT_NAME}"
    )

    # Unity DLL filename
    if (TARGET ${TARGET_NAME}_Unity)
        set_target_properties(${TARGET_NAME}_Unity PROPERTIES
            OUTPUT_NAME "${UNITY_OUTPUT_NAME}")
    endif()

    if (TARGET ${TARGET_NAME}_Unity)
    add_custom_command(TARGET ${TARGET_NAME}_Unity POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${UNITY_PLUGIN_COPY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:${TARGET_NAME}_Unity>"
            "${UNITY_PLUGIN_COPY_DIR}/$<TARGET_FILE_NAME:${TARGET_NAME}_Unity>"
    )
endif()

    target_sources(${TARGET_NAME}
        PRIVATE
            ${PROCESSOR_H}
            ${PROCESSOR_CPP}
            ${COMMON_SOURCE_FILES}
    )

    target_compile_definitions(${TARGET_NAME}
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
    )

    target_link_libraries(${TARGET_NAME}
        PUBLIC
            juce::juce_audio_basics
            juce::juce_audio_devices
            juce::juce_audio_formats
            juce::juce_audio_processors
            juce::juce_audio_utils
            juce::juce_core
            juce::juce_data_structures
            juce::juce_dsp
            juce::juce_events
            juce::juce_graphics
            juce::juce_gui_basics
            juce::juce_gui_extra
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
    )
endfunction()

# =========================================================
# VBAP plugin
# =========================================================
add_juce_unity_plugin(
    VBAPPlugin
    "VBAP"
    "${VBAP_UNITY_OUTPUT_NAME}"
    Vb01
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/VBAPPlugin/PluginProcessor.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/VBAPPlugin/PluginProcessor.cpp"
)

# =========================================================
# Binaural plugin
# =========================================================
add_juce_unity_plugin(
    BinauralPlugin
    "Binaural"
    "${BINAURAL_UNITY_OUTPUT_NAME}"
    Bi01
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/BinauralPlugin/PluginProcessor.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/BinauralPlugin/PluginProcessor.cpp"
)

# =========================================================
# Nice folder grouping in IDEs
# =========================================================
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
    ${COMMON_SOURCE_FILES}
    Source/VBAPPlugin/PluginProcessor.h
    Source/VBAPPlugin/PluginProcessor.cpp
    Source/BinauralPlugin/PluginProcessor.h
    Source/BinauralPlugin/PluginProcessor.cpp
)