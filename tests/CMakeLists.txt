FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)

add_executable(KeyDetectorTests
  KeyTests.cpp
  KeyAnalyzerTests.cpp
  ${CMAKE_SOURCE_DIR}/source/Key.cpp
  ${CMAKE_SOURCE_DIR}/source/KeyProfiles.cpp
  ${CMAKE_SOURCE_DIR}/source/KeyAnalyzer.cpp
)

target_include_directories(KeyDetectorTests PRIVATE ${CMAKE_SOURCE_DIR}/source)

target_link_libraries(KeyDetectorTests PRIVATE
  Catch2::Catch2WithMain
  juce::juce_core
  juce::juce_dsp
  juce::juce_recommended_config_flags
)

target_compile_definitions(KeyDetectorTests PRIVATE
  JUCE_STANDALONE_APPLICATION=1
  JUCE_USE_CURL=0
  JUCE_WEB_BROWSER=0
)

include(Catch)
catch_discover_tests(KeyDetectorTests)
