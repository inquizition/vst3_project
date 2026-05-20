cmake_minimum_required(VERSION 3.14.0)

add_subdirectory(vst3sdk)

set(KISSFFT_TEST OFF CACHE BOOL "" FORCE)
set(KISSFFT_TOOLS OFF CACHE BOOL "" FORCE)
set(KISSFFT_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(kissfft)
