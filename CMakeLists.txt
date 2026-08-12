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

option(KEYDETECTOR_BUILD_TESTS "Build unit tests" ON)
if(KEYDETECTOR_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
