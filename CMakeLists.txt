cmake_minimum_required(VERSION 3.22)
project(DrumForge VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.4
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(DrumForge
  COMPANY_NAME "DrumForge"
  PLUGIN_MANUFACTURER_CODE Dfrg
  PLUGIN_CODE Dfg1
  FORMATS VST3 Standalone
  PRODUCT_NAME "Drum Forge"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  VST3_CATEGORIES Instrument Drum
  COPY_PLUGIN_AFTER_BUILD FALSE)

  target_sources(DrumForge PRIVATE
     PluginProcessor.cpp
     PluginEditor.cpp)
     
target_compile_definitions(DrumForge PUBLIC
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(DrumForge
  PRIVATE juce::juce_audio_utils
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
