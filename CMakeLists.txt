cmake_minimum_required (VERSION 3.22)
project (YehoshuaMaster VERSION 1.0.0 LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    set (CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ---------------------------------------------------------------- JUCE
# Utilise un dossier JUCE local si fourni (-DJUCE_PATH=...), sinon le télécharge automatiquement.
set (JUCE_PATH "" CACHE PATH "Chemin vers un dossier JUCE existant (optionnel)")
if (JUCE_PATH AND EXISTS "${JUCE_PATH}/CMakeLists.txt")
    add_subdirectory ("${JUCE_PATH}" JUCE)
else()
    include (FetchContent)
    FetchContent_Declare (JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable (JUCE)
endif()

# ---------------------------------------------------------------- Plugin
juce_add_plugin (YehoshuaMaster
    COMPANY_NAME              "Yehoshua Musiq"
    PRODUCT_NAME              "YEHOSHUA MASTER"
    BUNDLE_ID                 "com.yehoshuamusiq.master"
    PLUGIN_MANUFACTURER_CODE  Yhmq
    PLUGIN_CODE               YmMs
    FORMATS                   VST3 Standalone
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES           Fx Mastering Dynamics EQ
    COPY_PLUGIN_AFTER_BUILD   FALSE)

juce_generate_juce_header (YehoshuaMaster)

juce_add_binary_data (YMBinaryData SOURCES Resources/logo.png)
set_target_properties (YMBinaryData PROPERTIES POSITION_INDEPENDENT_CODE ON)

set (YM_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_sources (YehoshuaMaster PRIVATE ${YM_SOURCES})
target_include_directories (YehoshuaMaster PRIVATE Source)

set (YM_DEFINES
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

# Build Linux sans certaines extensions X11 (non nécessaires)
if (UNIX AND NOT APPLE)
    list (APPEND YM_DEFINES JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_USE_XRENDER=0 JUCE_ALSA=0 JUCE_JACK=0)
endif()

target_compile_definitions (YehoshuaMaster PUBLIC ${YM_DEFINES})
if (MSVC)
    target_compile_options (YehoshuaMaster PRIVATE /utf-8)
endif()

target_link_libraries (YehoshuaMaster
    PRIVATE
        YMBinaryData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------- Banc d'essai (optionnel)
option (YM_BUILD_TESTS "Construire le banc d'essai (rendu audio + capture GUI)" OFF)
if (YM_BUILD_TESTS)
    juce_add_console_app (YMTest PRODUCT_NAME "YMTest")
    juce_generate_juce_header (YMTest)
    target_sources (YMTest PRIVATE tests/render_test.cpp ${YM_SOURCES})
    target_include_directories (YMTest PRIVATE Source)
    target_compile_definitions (YMTest PRIVATE ${YM_DEFINES} JucePlugin_Name="YEHOSHUA MASTER")
    target_link_libraries (YMTest PRIVATE YMBinaryData juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats
                           PUBLIC juce::juce_recommended_config_flags)
endif()
