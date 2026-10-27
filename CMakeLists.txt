cmake_minimum_required(VERSION 3.16)
project(pa2_selection CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to an optimised build. Single-config generators (Makefiles, Ninja)
# otherwise give an EMPTY build type, i.e. no optimisation at all.
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT is_multi_config AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(selection
  src/quickselect/quickselect.cpp
  src/mom/mom.cpp)
target_include_directories(selection PUBLIC src)

# Counted variant: every key comparison increments Stats::comparisons.
# Used by tests and comparison-count experiments.
add_library(selection_counted
  src/quickselect/quickselect.cpp
  src/mom/mom.cpp)
target_include_directories(selection_counted PUBLIC src)
target_compile_definitions(selection_counted PUBLIC SEL_COUNT_COMPARISONS)

add_executable(test_select tests/test_select.cpp)
target_link_libraries(test_select PRIVATE selection_counted)

# Timing runs use the uncounted build so the counter doesn't distort times.
add_executable(bench bench/bench.cpp)
target_link_libraries(bench PRIVATE selection)

add_executable(bench_counted bench/bench.cpp)
target_link_libraries(bench_counted PRIVATE selection_counted)
