cmake_minimum_required(VERSION 3.10)
project(ExpanderDecompKTH)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -Wall -Wpedantic -Wunused")
set(CMAKE_CXX_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O1 -g -mtune=native -march=native")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -lm")

add_subdirectory(external/tlx)

include_directories("lib")
include_directories("external/metis")

add_executable(EDC main/edc.cpp)
target_sources(EDC PRIVATE
		lib/datastructures/unit_flow.cpp
		lib/cut_matching.cpp
		lib/trimming.cpp
		lib/expander_decomp.cpp
		lib/sparse_cut_heuristics.cpp
		)

target_link_libraries(EDC tlx)
target_link_libraries(EDC ${CMAKE_SOURCE_DIR}/external/metis/libmetis.a)
