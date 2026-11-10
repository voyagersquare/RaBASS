cmake_minimum_required(VERSION 3.22)
project(RaBASS VERSION 0.2.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(RABASS_DSP_ONLY "Build portable DSP tests without JUCE" OFF)
option(RABASS_BUILD_TESTS "Build DSP and processor validation" ON)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
if(RABASS_BUILD_TESTS OR RABASS_DSP_ONLY)
    enable_testing()
    add_executable(RaBASSDspTests Tests/DspTests.cpp)
    target_include_directories(RaBASSDspTests PRIVATE Source)
    add_test(NAME bass-dsp COMMAND RaBASSDspTests)
endif()
if(RABASS_DSP_ONLY)
    return()
endif()
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 51a8a6d7aeae7326956d747737ccf1575e61e209
    GIT_CONFIG core.longpaths=true)
FetchContent_MakeAvailable(JUCE)
juce_add_plugin(RaBASS
    COMPANY_NAME "Independent Audio"
    BUNDLE_ID "audio.independent.rabass"
    PLUGIN_MANUFACTURER_CODE InAu
    PLUGIN_CODE RaBs
    FORMATS VST3
    PRODUCT_NAME "RaBASS"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD FALSE)
juce_generate_juce_header(RaBASS)
target_sources(RaBASS PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(RaBASS PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0 JUCE_USE_MP3AUDIOFORMAT=0)
target_link_libraries(RaBASS PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
if(MSVC)
    target_compile_options(RaBASS PRIVATE /bigobj)
endif()
if(RABASS_BUILD_TESTS)
    add_executable(RaBASSProcessorTests Tests/ProcessorTests.cpp)
    target_include_directories(RaBASSProcessorTests PRIVATE Source "${juce_SOURCE_DIR}/modules"
        "${CMAKE_CURRENT_BINARY_DIR}/RaBASS_artefacts/JuceLibraryCode")
    target_compile_definitions(RaBASSProcessorTests PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1)
    target_link_libraries(RaBASSProcessorTests PRIVATE RaBASS)
    add_test(NAME bass-processor COMMAND RaBASSProcessorTests)
    set_tests_properties(bass-processor PROPERTIES TIMEOUT 45)
endif()
