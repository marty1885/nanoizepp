cmake_minimum_required(VERSION 3.16)
project(nanoizepp CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NANOIZEPP_BUILD_EXAMPLES "Build examples" ON)
option(NANOIZEPP_BUILD_TESTS "Build tests" OFF)

include_directories(.)
add_subdirectory(nanoizepp)

if (NANOIZEPP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (NANOIZEPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()