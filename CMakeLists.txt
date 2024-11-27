cmake_minimum_required(VERSION 3.15)

project(tt-alloc-opt)
include_directories(.)

add_executable(tt-alloc-opt
        main.cpp
        tt_metal/impl/allocator/algorithms/free_list_opt.cpp
)

target_link_libraries(tt-alloc-opt fmt gtest gtest_main)

target_precompile_headers(tt-alloc-opt PRIVATE
    <fmt/core.h>
    <iostream>
    <vector>
    <map>
    <cstdint>
    <gtest/gtest.h>
    )
