cmake_minimum_required(VERSION 3.15)

project(tt-alloc-opt)
include_directories(.)

set(CMAKE_CXX_STANDARD 17)

add_library(tt-alloc-opt
        tt_metal/impl/allocator/algorithms/free_list_opt.cpp
        tt_metal/impl/allocator/algorithms/free_list.cpp
)
target_precompile_headers(tt-alloc-opt PUBLIC
    <fmt/core.h>
    <iostream>
    <vector>
    <map>
    <cstdint>
)
target_link_libraries(tt-alloc-opt fmt)

add_executable(tt-alloc-opt-bench benchmark.cpp)
target_precompile_headers(tt-alloc-opt-bench PUBLIC <benchmark/benchmark.h>)
find_package(benchmark REQUIRED)
find_package(fmt REQUIRED)
target_link_libraries(tt-alloc-opt-bench tt-alloc-opt benchmark::benchmark fmt::fmt)

find_package(Catch2 3 REQUIRED)
add_executable(tt-alloc-opt-test test.cpp)
target_link_libraries(tt-alloc-opt-test tt-alloc-opt Catch2::Catch2WithMain)
target_precompile_headers(tt-alloc-opt-test PUBLIC <catch2/catch_test_macros.hpp>)

add_executable(tt-alloc-fragmentation fragmentation.cpp)
target_link_libraries(tt-alloc-fragmentation tt-alloc-opt)
target_precompile_headers(tt-alloc-fragmentation PUBLIC <fmt/core.h>)