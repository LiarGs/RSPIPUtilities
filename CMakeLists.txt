cmake_minimum_required(VERSION 3.15)
project(RSPIPUtils)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# 注意：请确保 GDAL 的路径在你的机器上是正确的
set(GDAL_ROOT "D:/C++_Library/GDAL/Release")
set(GDAL_INCLUDE_DIR "${GDAL_ROOT}/include")
set(GDAL_LIBRARY "${GDAL_ROOT}/lib/gdal.lib")


find_package(OpenCV REQUIRED)
find_package(OpenMP REQUIRED)
find_package(GDAL REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS} include)
include_directories(${GDAL_INCLUDE_DIRS})


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
target_link_libraries(${PROJECT_NAME} ${GDAL_LIBRARIES})

if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} OpenMP::OpenMP_CXX)
endif()
