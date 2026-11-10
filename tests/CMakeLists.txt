cmake_minimum_required(VERSION 3.18)
project(SkellFlangPortableDSPTests LANGUAGES CXX)
enable_testing()
add_executable(flang-dsp-tests test_dsp.cpp)
target_compile_features(flang-dsp-tests PRIVATE cxx_std_17)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(flang-dsp-tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME flang-dsp-tests COMMAND flang-dsp-tests)

# Run the measured benchmark only in the ordinary Release Linux job.
# Sanitizer builds compile it but skip timing analysis.
add_executable(flang-dsp-bench bench_dsp.cpp)
target_compile_features(flang-dsp-bench PRIVATE cxx_std_17)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(flang-dsp-bench PRIVATE -Wall -Wextra -Wpedantic)
endif()
