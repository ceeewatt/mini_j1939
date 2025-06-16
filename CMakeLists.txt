cmake_minimum_required(VERSION 3.28)
project(MiniJ1939)

include(CTest)

add_subdirectory(src)

if (BUILD_TESTING)
    message(STATUS "[mini_j1939] Building unit tests")
    add_subdirectory(test)
endif()
