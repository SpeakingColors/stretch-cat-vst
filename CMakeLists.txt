cmake_minimum_required(VERSION 3.22)
project(stretchcat VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)
FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.16-security GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(juce)

juce_add_plugin(StretchCat
    COMPANY_NAME "Studio 87"
    PLUGIN_MANUFACTURER_CODE Spco
    PLUGIN_CODE Stct
    FORMATS VST3 Standalone
    PRODUCT_NAME "Stretch Cat"
    IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE)
target_sources(StretchCat PRIVATE src/Plugins.cpp src/stretch/StretchCat.cpp)
target_include_directories(StretchCat PRIVATE src)
target_compile_definitions(StretchCat PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_USE_MP3AUDIOFORMAT=1)
target_link_libraries(StretchCat PRIVATE juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
