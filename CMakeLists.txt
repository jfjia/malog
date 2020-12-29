cmake_minimum_required(VERSION 2.8)

project(malog)

option(WITH_EXAMPLES "build examples" OFF)

enable_language(CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RELEASE)
endif()

message("  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message("  CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message("  CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")

set(CMAKE_CXX_FLAGS "-std=c++11 -fPIC -fno-strict-aliasing -Wall ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -ggdb -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -ggdb -DNDEBUG")

message("  CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}: ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")

aux_source_directory(${PROJECT_SOURCE_DIR}/src malog_SRC)

if(UNIX)
    SET(DEPENDENT_LIBRARIES pthread)
else()
    SET(DEPENDENT_LIBRARIES ws2_32)
endif()

add_library(malog STATIC ${malog_SRC})
target_include_directories(malog PUBLIC ${PROJECT_SOURCE_DIR}/src)

if(WITH_EXAMPLES)
    add_subdirectory(examples)
endif()
