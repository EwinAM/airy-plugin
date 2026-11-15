cmake_minimum_required(VERSION 3.22)
project(AiryV22 VERSION 2.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Windows (MSVC): static runtime so no extra DLLs are needed
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Mac builds: one universal binary for Intel and Apple Silicon
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")

# Use a local JUCE checkout if JUCE_DIR is given, otherwise download JUCE 7.0.12
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 7.0.12
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(AiryV22
    COMPANY_NAME "Ewin"
    PRODUCT_NAME "Airy v2.2"
    BUNDLE_ID "com.ewin.airyv22"
    PLUGIN_MANUFACTURER_CODE Ewin
    PLUGIN_CODE Ar22
    FORMATS VST3 AU
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx EQ
    VST3_AUTO_MANIFEST FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(AiryV22)
juce_add_binary_data(AiryData SOURCES
    fonts/Manrope-Medium.ttf
    fonts/Manrope-SemiBold.ttf
    fonts/Manrope-Bold.ttf)

target_sources(AiryV22 PRIVATE src/PluginProcessor.cpp src/PluginEditor.cpp)

# Released under GPLv3, so the JUCE splash screen is not required
target_compile_definitions(AiryV22 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(AiryV22
    PRIVATE AiryData juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(MINGW)
    target_link_options(AiryV22_VST3 PRIVATE -static -static-libgcc -static-libstdc++ -s)
endif()
