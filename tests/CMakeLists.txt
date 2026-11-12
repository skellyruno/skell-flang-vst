cmake_minimum_required(VERSION 3.18)
project(SkellFlangPortableDSPTests LANGUAGES CXX)
enable_testing()
add_executable(flang-dsp-tests test_dsp.cpp)
target_compile_features(flang-dsp-tests PRIVATE cxx_std_17)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(flang-dsp-tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME flang-dsp-tests COMMAND flang-dsp-tests)

# Run the measured benchmark only in the ordinary Release Linux job.
# Sanitizer builds compile it but skip timing analysis.
add_executable(flang-dsp-bench bench_dsp.cpp)
target_compile_features(flang-dsp-bench PRIVATE cxx_std_17)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(flang-dsp-bench PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Additional portable release gates: local-world wire protocol and
# allocation-free stereo sample processing through all 8 internal engines.
add_executable(flang-world-protocol test_world_protocol.cpp)
target_compile_features(flang-world-protocol PRIVATE cxx_std_17)
add_test(NAME flang-world-protocol COMMAND flang-world-protocol)
add_executable(flang-realtime-allocation test_realtime_alloc.cpp)
target_compile_features(flang-realtime-allocation PRIVATE cxx_std_17)
add_test(NAME flang-realtime-allocation COMMAND flang-realtime-allocation)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  foreach(t flang-world-protocol flang-realtime-allocation)
    target_compile_options(${t} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endforeach()
endif()
