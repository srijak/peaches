cmake_minimum_required(VERSION 2.6)
set(PROJECT_NAME PEACHES)
set(PROJ_VERSION 0.0.1)
project(${PROJECT_NAME})
enable_testing()

#--- Libs
INCLUDE(CheckIncludeFiles)
INCLUDE(FindPkgConfig)
INCLUDE(CheckLibraryExists)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
#find_package(LibEV REQUIRED)

#--- Compiler options
if(CMAKE_COMPILER_IS_GNUCC)
  add_definitions(-Wall -Werror -pedantic -std=c99)
endif(CMAKE_COMPILER_IS_GNUCC)

#--- Subdirs
add_subdirectory(src)
add_subdirectory(tests)

