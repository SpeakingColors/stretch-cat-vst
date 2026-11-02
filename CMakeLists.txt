cmake_minimum_required(VERSION 3.22)
project(stretchcat VERSION 0.2.0 LANGUAGES C CXX)

include(FetchContent)
FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.16-security GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(juce)
FetchContent_Declare(cat_ui GIT_REPOSITORY https://github.com/SpeakingColors/cat-ui GIT_TAG ba762e1755bbabd6847076e26c5ed7a9c5d6c76d)
FetchContent_MakeAvailable(cat_ui)

juce_add_binary_data(StretchCatAssets SOURCES
    assets/fonts/BarlowCondensed-Medium.ttf assets/fonts/BarlowCondensed-SemiBold.ttf assets/fonts/BarlowCondensed-Bold.ttf assets/fonts/JetBrainsMono-Regular.ttf)
set_target_properties(StretchCatAssets PROPERTIES POSITION_INDEPENDENT_CODE ON)

juce_add_plugin(StretchCat
    COMPANY_NAME "Studio 87"
    PLUGIN_MANUFACTURER_CODE Spco
    PLUGIN_CODE Stct
    FORMATS VST3 Standalone
    PRODUCT_NAME "Stretch Cat"
    IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE)
target_sources(StretchCat PRIVATE src/Plugins.cpp src/stretch/StretchCat.cpp src/stretch/StretchCatEditor.cpp)
target_include_directories(StretchCat PRIVATE src)
target_compile_definitions(StretchCat PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_USE_MP3AUDIOFORMAT=1)
target_link_libraries(StretchCat PRIVATE StretchCatAssets cat_ui juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# Offscreen window renderer and self tests (CI only, not part of releases).
option(SC_BUILD_SNAPSHOT "Build the editor snapshot tool" OFF)
if(SC_BUILD_SNAPSHOT)
    juce_add_console_app(StretchCatSnapshot PRODUCT_NAME "StretchCatSnapshot")
    target_sources(StretchCatSnapshot PRIVATE tools/snapshot.cpp src/stretch/StretchCat.cpp src/stretch/StretchCatEditor.cpp)
    target_include_directories(StretchCatSnapshot PRIVATE src)
    target_compile_definitions(StretchCatSnapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_USE_MP3AUDIOFORMAT=1
        JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1 JucePlugin_Name="StretchCat" JucePlugin_VersionString="0.2.0")
    target_link_libraries(StretchCatSnapshot PRIVATE StretchCatAssets cat_ui juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp juce::juce_recommended_config_flags)
endif()
