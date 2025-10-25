cmake_minimum_required(VERSION 3.15)
project(RSPIPUtils)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS} include)
include_directories("D:/C++_Library/GDAL/Release/include")

file(GLOB SOURCES "src/*.cpp")
file(GLOB_RECURSE SOURCES_RECURSIVE "src/**/*.cpp")

if(NOT SOURCES AND NOT SOURCES_RECURSIVE)
    message(FATAL_ERROR "No source files found in src directory!")
endif()

message(STATUS "Found sources: ${SOURCES} ${SOURCES_RECURSIVE}")

# Enable Link Time Code Generation (LTCG) for MSVC builds
if (MSVC)
    add_link_options(/LTCG)
endif()

# add_library(${PROJECT_NAME} ${SOURCES} ${SOURCES_RECURSIVE})

add_executable(${PROJECT_NAME} ${SOURCES} ${SOURCES_RECURSIVE})

target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME} "D:/C++_Library/GDAL/Release/lib/gdal.lib")
