cmake_minimum_required(VERSION 4.1.2)
project(AnalogSynth VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# SDK paths must be set BEFORE adding JUCE
set(JUCE_LV2_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/lv2")

# JUCE Framework
add_subdirectory(external/JUCE)

# CLAP support via clap-juce-extensions
add_subdirectory(external/clap-juce-extensions EXCLUDE_FROM_ALL)

# Plugin-Formate
juce_add_plugin(AnalogSynth
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE YCOM
    PLUGIN_CODE ASyn
    FORMATS VST3 CLAP LV2 Standalone
    PRODUCT_NAME "Analog Synth"
    LV2URI "urn:yourcompany:analogsynth"

    # Synth-spezifische Flags
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE

    # Plugin-Eigenschaften
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(AnalogSynth)

# Add CLAP support
clap_juce_extensions_plugin(TARGET AnalogSynth
    CLAP_ID "com.yourcompany.analogsynth"
    CLAP_FEATURES instrument synthesizer)

target_sources(AnalogSynth PRIVATE
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginEditor.cpp
    src/core/AdsrEnvelope.cpp
    src/core/Oscillator.cpp
)

target_compile_definitions(AnalogSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_compile_options(AnalogSynth PRIVATE
    -Wall -Wextra -Wpedantic
    # Disable -Werror for now due to JUCE warnings with newer GCC
    # $<$<CONFIG:Release>:-Werror>
)

target_link_libraries(AnalogSynth PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
)

# Tests
include(CTest)
enable_testing()

add_subdirectory(external/Catch2)
add_executable(AnalogSynthTests
    tests/AdsrTests.cpp
    # Source-Dateien für Tests
    src/core/AdsrEnvelope.cpp
)

target_include_directories(AnalogSynthTests PRIVATE src)

target_compile_definitions(AnalogSynthTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(AnalogSynthTests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_dsp
)

add_test(NAME AdsrTests COMMAND AnalogSynthTests)
