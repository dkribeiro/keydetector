cmake_minimum_required(VERSION 3.22)
project(KeyDetector VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.4
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(KeyDetector
  PRODUCT_NAME "KeyDetector"
  COMPANY_NAME "KeyDetector"
  BUNDLE_ID "com.keydetector.keydetector"
  PLUGIN_MANUFACTURER_CODE Kdet
  PLUGIN_CODE Kdt1
  FORMATS AU VST3
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(KeyDetector PRIVATE
  source/PluginProcessor.cpp
  source/Key.cpp
  source/KeyProfiles.cpp
  source/KeyAnalyzer.cpp
)

target_compile_definitions(KeyDetector PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(KeyDetector PRIVATE
  juce::juce_audio_utils
  juce::juce_dsp
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags
)

option(KEYDETECTOR_BUILD_TESTS "Build unit tests" ON)
if(KEYDETECTOR_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
