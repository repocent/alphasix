cmake_minimum_required(VERSION 3.22)
project(AlphaSix VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
                              GIT_TAG 8.0.8 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# Shared DSP / UI sources, used by both the plugin and the offline test.
set(ALPHASIX_SOURCES
    Source/Params.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

# Only link what a plugin needs (no juce_audio_devices -> no ALSA required).
set(ALPHASIX_LIBS
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_events
    juce::juce_data_structures
    juce::juce_core)

set(ALPHASIX_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_XRANDR=0
    JUCE_USE_XINERAMA=0
    JUCE_USE_XCURSOR=0
    JUCE_USE_XSHM=0
    JUCE_USE_XRENDER=0)

juce_add_plugin(AlphaSix
    COMPANY_NAME "AlphaSix"
    PLUGIN_MANUFACTURER_CODE Alsx
    PLUGIN_CODE Als6
    FORMATS VST3
    PRODUCT_NAME "AlphaSix"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(AlphaSix PRIVATE ${ALPHASIX_SOURCES})
target_compile_definitions(AlphaSix PUBLIC ${ALPHASIX_DEFS})
target_link_libraries(AlphaSix
    PRIVATE ${ALPHASIX_LIBS}
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)

# Offline render test (no host needed).
juce_add_console_app(AlphaSixTest PRODUCT_NAME "AlphaSixTest")
target_sources(AlphaSixTest PRIVATE Tests/render_test.cpp ${ALPHASIX_SOURCES})
target_compile_definitions(AlphaSixTest PUBLIC ${ALPHASIX_DEFS})
target_link_libraries(AlphaSixTest
    PRIVATE ${ALPHASIX_LIBS}
    PUBLIC juce::juce_recommended_config_flags)
