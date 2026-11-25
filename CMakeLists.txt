cmake_minimum_required(VERSION 3.22)
project(SkellFlang VERSION 0.9.2 LANGUAGES C CXX)
include(FetchContent)
if(DEFINED JUCE_PATH AND EXISTS "${JUCE_PATH}/CMakeLists.txt")
  add_subdirectory("${JUCE_PATH}" JUCE)
else()
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.12 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)
endif()
add_subdirectory(SkellCore)
juce_add_plugin(SkellFlang
  COMPANY_NAME "Skell Audio"
  BUNDLE_ID "audio.skell.skellflang"
  PLUGIN_MANUFACTURER_CODE SkAu
  PLUGIN_CODE Fl26
  FORMATS VST3
  PRODUCT_NAME "Skell-Flang"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE)
juce_generate_juce_header(SkellFlang)
target_sources(SkellFlang PRIVATE Source/PluginProcessor.cpp Source/PluginProcessor.h Source/PluginEditor.cpp Source/PluginEditor.h Source/FlangDSP.h)
target_compile_features(SkellFlang PRIVATE cxx_std_17)
target_compile_definitions(SkellFlang PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0)
target_compile_definitions(SkellFlang PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(SkellFlang PRIVATE juce::juce_audio_utils juce::juce_audio_processors juce::juce_gui_extra juce::juce_dsp)

target_link_libraries(SkellFlang PRIVATE SkellCore::DSP SkellCore::UI)

# CI-only editor/render smoke target. Not a standalone audio plugin; the
# distribution artifact includes VST3 only.
option(SKELLFLANG_BUILD_UI_SMOKE "Build the JUCE editor smoke-test program" OFF)
if(SKELLFLANG_BUILD_UI_SMOKE)
  juce_add_console_app(SkellFlangUiSmoke PRODUCT_NAME "Skell-Flang UI Smoke")
  juce_generate_juce_header(SkellFlangUiSmoke)
  target_sources(SkellFlangUiSmoke PRIVATE tests/test_ui_host.cpp)
  target_compile_features(SkellFlangUiSmoke PRIVATE cxx_std_17)
  target_compile_definitions(SkellFlangUiSmoke PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  target_link_libraries(SkellFlangUiSmoke PRIVATE SkellFlang SkellCore::UIRenderer juce::juce_audio_utils juce::juce_gui_extra)
endif()


# Normal source layout. Legacy source-only checkout remains buildable until
# the owner supplies the licensed native files. Existing CMake cache paths
# continue to work; no separate native plugin or VST3 identity.
set(SKELLY_FLANGUS_SOURCE_DIR "" CACHE PATH "Native Flangus source override (optional)")
if(SKELLY_FLANGUS_SOURCE_DIR STREQUAL "" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/NativeFlangus/Flangus.cpp")
  set(SKELLY_FLANGUS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/NativeFlangus")
endif()
option(SKELLFLANG_REQUIRE_NATIVE "Fail configuration if native Flangus is unavailable" OFF)
if(SKELLFLANG_REQUIRE_NATIVE AND SKELLY_FLANGUS_SOURCE_DIR STREQUAL "")
  message(FATAL_ERROR "Native Flangus source missing from Source/NativeFlangus")
endif()
if(NOT SKELLY_FLANGUS_SOURCE_DIR STREQUAL "")
  foreach(f Flangus.cpp SkellFlangusAdapter.h core/FlangusFastDist.h)
    if(NOT EXISTS "${SKELLY_FLANGUS_SOURCE_DIR}/${f}")
      message(FATAL_ERROR "Missing native Flangus source: ${f}")
    endif()
  endforeach()
  target_sources(SkellFlang PRIVATE "${SKELLY_FLANGUS_SOURCE_DIR}/Flangus.cpp" Source/SkellRawFlangusPath.h)
  target_include_directories(SkellFlang PUBLIC "${SKELLY_FLANGUS_SOURCE_DIR}")
  target_compile_definitions(SkellFlang PUBLIC SKELLFLANG_RAW_ENGINE=1)
endif()

option(SKELLY_FLANGUS_SAMPLE_CLOCK_RAMP "Use sample-clock native gain slew" OFF)
if(SKELLY_FLANGUS_SAMPLE_CLOCK_RAMP)
  if(SKELLY_FLANGUS_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "Sample-clock ramp requires native v2 DSP source")
  endif()
  target_compile_definitions(SkellFlang PUBLIC SKELLFLANG_SAMPLE_CLOCK_RAMP=1)
endif()
