cmake_minimum_required(VERSION 3.24)

project(JucePlugins VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_XCODE_GENERATE_SCHEME ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# =========================================================
# Output names for Unity DLLs
# =========================================================
set(VBAP_UNITY_OUTPUT_NAME "audioplugin_VBAP")
set(BINAURAL_UNITY_OUTPUT_NAME "audioplugin_Binaural")

# =========================================================
# Copy destinations
# Everything is copied inside the build folder.
# =========================================================
option(COPY_PLUGIN_AFTER_BUILD "Copy built plugins after build" ON)

set(UNITY_PLUGIN_COPY_DIR "${CMAKE_BINARY_DIR}/UnityPlugins")
set(VST3_PLUGIN_COPY_DIR "${CMAKE_BINARY_DIR}/VST3")

# =========================================================
# CPM + JUCE
# =========================================================
set(CPM_DOWNLOAD_VERSION 0.42.3)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if (NOT EXISTS "${CPM_DOWNLOAD_LOCATION}")
    message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD
        "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
        "${CPM_DOWNLOAD_LOCATION}"
    )
endif()

include("${CPM_DOWNLOAD_LOCATION}")

CPMAddPackage(
    NAME juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.14
    GIT_SHALLOW TRUE
)

# =========================================================
# Shared files
# =========================================================
set(COMMON_SOURCE_FILES
    Source/Common/VbapEngine2D.h
    Source/Common/TestSession.h
    Source/Common/AudioPlayer.h
    Source/Common/PluginEditorControls.h
    Source/Common/PluginEditorControls.cpp
)

# =========================================================
# VBAP plugin files
# =========================================================
set(VBAP_SOURCE_FILES
    Source/VBAPPlugin/PluginProcessor.h
    Source/VBAPPlugin/PluginProcessor.cpp
    Source/VBAPPlugin/VbapParameterEditor.h
    Source/VBAPPlugin/VbapParameterEditor.cpp
)

# =========================================================
# Binaural plugin files
# =========================================================
set(BINAURAL_SOURCE_FILES
    Source/BinauralPlugin/PluginProcessor.h
    Source/BinauralPlugin/PluginProcessor.cpp
    Source/BinauralPlugin/HrirBank.h
    Source/BinauralPlugin/HrirBank.cpp
    Source/BinauralPlugin/BinauralTestSessionEditor.h
    Source/BinauralPlugin/BinauralTestSessionEditor.cpp
    Source/BinauralPlugin/BinauralParameterEditor.h
    Source/BinauralPlugin/BinauralParameterEditor.cpp
)

# =========================================================
# Helper function
# =========================================================
function(add_juce_unity_plugin TARGET_NAME PRODUCT_NAME PLUGIN_NAME UNITY_OUTPUT_NAME PLUGIN_CODE EDITOR_MODE BUILD_UNITY)
    if (BUILD_UNITY)
        set(_formats VST3 Standalone Unity)
    else()
        set(_formats Standalone)
    endif()

    juce_add_plugin(${TARGET_NAME}
        COMPANY_NAME "YourCompany"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        JUCE_VST3_CAN_REPLACE_VST2 FALSE

        COPY_PLUGIN_AFTER_BUILD ${COPY_PLUGIN_AFTER_BUILD}
        UNITY_COPY_DIR "${UNITY_PLUGIN_COPY_DIR}"
        VST3_COPY_DIR "${VST3_PLUGIN_COPY_DIR}"

        PLUGIN_MANUFACTURER_CODE JuPl
        PLUGIN_CODE ${PLUGIN_CODE}

        FORMATS ${_formats}
        PRODUCT_NAME "${PRODUCT_NAME}"
        PLUGIN_NAME "${PLUGIN_NAME}"
    )

    # Unity DLL filename
    if (TARGET ${TARGET_NAME}_Unity)
        set_target_properties(${TARGET_NAME}_Unity PROPERTIES
            OUTPUT_NAME "${UNITY_OUTPUT_NAME}"
        )
    endif()

    # Copy Unity DLL into build/UnityPlugins
    if (COPY_PLUGIN_AFTER_BUILD AND TARGET ${TARGET_NAME}_Unity)
        add_custom_command(TARGET ${TARGET_NAME}_Unity POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${UNITY_PLUGIN_COPY_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${TARGET_NAME}_Unity>"
                "${UNITY_PLUGIN_COPY_DIR}/$<TARGET_FILE_NAME:${TARGET_NAME}_Unity>"
        )
    endif()

    # Copy Assets next to every Standalone executable.
    # This lets Binaural.exe / BinauralTest.exe find HRIR files without hardcoded paths.
    if (TARGET ${TARGET_NAME}_Standalone)
        add_custom_command(TARGET ${TARGET_NAME}_Standalone POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
                "$<TARGET_FILE_DIR:${TARGET_NAME}_Standalone>/Assets"
        )
    endif()

    # Also copy Assets into build/UnityPlugins.
    # Useful if you manually take the Unity DLLs from build/UnityPlugins.
    if (COPY_PLUGIN_AFTER_BUILD AND TARGET ${TARGET_NAME}_Unity)
        add_custom_command(TARGET ${TARGET_NAME}_Unity POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
                "${UNITY_PLUGIN_COPY_DIR}/Assets"
        )
    endif()

    target_sources(${TARGET_NAME}
        PRIVATE
            ${COMMON_SOURCE_FILES}
            ${ARGN}
    )

    target_compile_definitions(${TARGET_NAME}
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            BINAURAL_EDITOR_MODE=${EDITOR_MODE}
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
# Standalone + VST3 + Unity
# =========================================================
add_juce_unity_plugin(
    VBAPPlugin
    "VBAP"
    "VBAP"
    "${VBAP_UNITY_OUTPUT_NAME}"
    Vb01
    0
    TRUE
    ${VBAP_SOURCE_FILES}
)

# =========================================================
# Binaural plugin — test session editor
# Standalone + VST3 + Unity
# =========================================================
add_juce_unity_plugin(
    BinauralPlugin
    "BinauralTest"
    "Binaural"
    "${BINAURAL_UNITY_OUTPUT_NAME}"
    Bi01
    0
    TRUE
    ${BINAURAL_SOURCE_FILES}
)

# =========================================================
# Binaural plugin — parameter editor
# Standalone only
# =========================================================
add_juce_unity_plugin(
    BinauralParamsPlugin
    "Binaural"
    "Binaural"
    "audioplugin_Binaural_unused"
    Bi02
    1
    FALSE
    ${BINAURAL_SOURCE_FILES}
)

# =========================================================
# Nice folder grouping in IDEs
# =========================================================
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${COMMON_SOURCE_FILES}
    ${VBAP_SOURCE_FILES}
    ${BINAURAL_SOURCE_FILES}
)