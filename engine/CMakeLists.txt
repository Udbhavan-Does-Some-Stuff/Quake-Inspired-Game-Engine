cmake_minimum_required(VERSION 3.15)
project(engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ---- core module --------------------------------------------------
add_library(core STATIC
    core/log.cpp
    core/memory.cpp
)
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# ---- core module test ----------------------------------------------
add_executable(core_test
    tests/core_test.cpp
)
target_link_libraries(core_test PRIVATE core)
