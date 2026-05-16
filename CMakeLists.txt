cmake_minimum_required(VERSION 3.24)

project(JucePlugins VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_XCODE_GENERATE_SCHEME ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# =========================================================
# Output names for Unity DLLs
# =========================================================
set(VBAP_UNITY_OUTPUT_NAME "audioplugin_VBAP"
    CACHE STRING "Output filename for the Unity VBAP plugin" FORCE)

set(BINAURAL_UNITY_OUTPUT_NAME "audioplugin_Binaural"
    CACHE STRING "Output filename for the Unity Binaural plugin" FORCE)

# =========================================================
# Copy destinations
# =========================================================
set(UNITY_PLUGIN_COPY_DIR "C:/Users/nikos/Desktop/Diplomatiki/VST-Test/Assets/Plugins"
    CACHE PATH "Where to copy built Unity plugins when COPY_PLUGIN_AFTER_BUILD is TRUE")

set(VST3_PLUGIN_COPY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/VST3"
    CACHE PATH "Where to copy built VST3 plugins when COPY_PLUGIN_AFTER_BUILD is TRUE")

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
# Helper function — creates one JUCE plugin target.
#   PLUGIN_NAME: name the plugin reports to hosts (DAW/Unity mixer).
#                Independent of PRODUCT_NAME (which controls the output filename).
#   EDITOR_MODE: integer passed to source code as BINAURAL_EDITOR_MODE
#                (0 = test editor, 1 = parameter editor; only used by Binaural).
#   BUILD_UNITY: TRUE to build VST3+Standalone+Unity, FALSE for Standalone only.
# =========================================================
function(add_juce_unity_plugin TARGET_NAME PRODUCT_NAME PLUGIN_NAME UNITY_OUTPUT_NAME PLUGIN_CODE EDITOR_MODE BUILD_UNITY)
    if (${BUILD_UNITY})
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

        COPY_PLUGIN_AFTER_BUILD TRUE
        UNITY_COPY_DIR "${UNITY_PLUGIN_COPY_DIR}"
        VST3_COPY_DIR "${VST3_PLUGIN_COPY_DIR}"

        PLUGIN_MANUFACTURER_CODE JuPl
        PLUGIN_CODE ${PLUGIN_CODE}

        FORMATS ${_formats}
        PRODUCT_NAME "${PRODUCT_NAME}"
        PLUGIN_NAME  "${PLUGIN_NAME}"
    )

    # Unity DLL filename
    if (TARGET ${TARGET_NAME}_Unity)
        set_target_properties(${TARGET_NAME}_Unity PROPERTIES
            OUTPUT_NAME "${UNITY_OUTPUT_NAME}")
    endif()

    # Extra explicit Unity copy
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
# VBAP plugin (Standalone + VST3 + Unity)
# =========================================================
add_juce_unity_plugin(
    VBAPPlugin
    "VBAP"                          # PRODUCT_NAME (filename)
    "VBAP"                          # PLUGIN_NAME (shown to hosts)
    "${VBAP_UNITY_OUTPUT_NAME}"
    Vb01
    0       # EDITOR_MODE (unused for VBAP)
    TRUE    # BUILD_UNITY
    ${VBAP_SOURCE_FILES}
)

# =========================================================
# Binaural plugin — test session editor.
# Standalone is named BinauralTest.exe, but inside Unity / DAWs the plugin
# still reports itself as "Binaural" so the existing mixer effect keeps working.
# =========================================================
add_juce_unity_plugin(
    BinauralPlugin
    "BinauralTest"                  # PRODUCT_NAME (standalone/vst3 filename)
    "Binaural"                      # PLUGIN_NAME (host-facing identifier)
    "${BINAURAL_UNITY_OUTPUT_NAME}"
    Bi01
    0       # EDITOR_MODE = test editor
    TRUE    # BUILD_UNITY
    ${BINAURAL_SOURCE_FILES}
)

# =========================================================
# Binaural plugin — parameter editor.
# Standalone only (Binaural.exe). Shares audio code with the test variant,
# selects the parameter editor via BINAURAL_EDITOR_MODE=1.
# =========================================================
add_juce_unity_plugin(
    BinauralParamsPlugin
    "Binaural"                      # PRODUCT_NAME
    "Binaural"                      # PLUGIN_NAME
    "audioplugin_Binaural_unused"   # Unity not built for this target
    Bi02
    1       # EDITOR_MODE = parameter editor
    FALSE   # BUILD_UNITY
    ${BINAURAL_SOURCE_FILES}
)

# =========================================================
# Nice folder grouping in IDEs
# =========================================================
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
    ${COMMON_SOURCE_FILES}
    ${VBAP_SOURCE_FILES}
    ${BINAURAL_SOURCE_FILES}
)
