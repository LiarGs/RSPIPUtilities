cmake_minimum_required(VERSION 3.15)
project(RSPIPUtils)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)
find_package(OpenMP REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS} include)
# 注意：请确保 GDAL 的路径在你的机器上是正确的
include_directories("D:/C++_Library/GDAL/Release/include")


file(GLOB_RECURSE SOURCES "src/*.cpp")

if(NOT SOURCES)
    message(FATAL_ERROR "No source files specified!")
endif()

message(STATUS "Using sources: ${SOURCES}")

# Enable Link Time Code Generation (LTCG) for MSVC builds
if (MSVC)
    add_link_options(/LTCG)
endif()

# add_library(${PROJECT_NAME} ${SOURCES} ${SOURCES_RECURSIVE})

add_executable(${PROJECT_NAME} ${SOURCES} ${SOURCES_RECURSIVE})

target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
# 注意：请确保 GDAL 的库路径在你的机器上是正确的
target_link_libraries(${PROJECT_NAME} "D:/C++_Library/GDAL/Release/lib/gdal.lib")

if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} OpenMP::OpenMP_CXX)
endif()
