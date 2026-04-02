cmake_minimum_required(VERSION 3.15)
project(RSPIPUtils)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GDAL_ROOT "" CACHE PATH "Optional hint path to a GDAL installation")
set(EIGEN_ROOT "" CACHE PATH "Optional hint path to the Eigen headers")
option(RSPIP_BUILD_CONSOLE "Build the console sample target" ON)

if(GDAL_ROOT)
    list(PREPEND CMAKE_PREFIX_PATH "${GDAL_ROOT}")
endif()

find_package(OpenCV REQUIRED)
find_package(OpenMP REQUIRED)
find_package(GDAL REQUIRED)

if(EIGEN_ROOT)
    find_path(EIGEN_INCLUDE_DIR Eigen/Core PATHS "${EIGEN_ROOT}" PATH_SUFFIXES "" include eigen3 NO_DEFAULT_PATH)
else()
    find_path(EIGEN_INCLUDE_DIR Eigen/Core PATH_SUFFIXES eigen3 include)
endif()

if(NOT EIGEN_INCLUDE_DIR)
    message(FATAL_ERROR "Eigen headers were not found. Set EIGEN_ROOT or provide Eigen/Core on your include path.")
endif()

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(RSPIP_Core STATIC ${LIB_SOURCES})

target_include_directories(RSPIP_Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
    ${GDAL_INCLUDE_DIRS}
    ${EIGEN_INCLUDE_DIR})

target_link_libraries(RSPIP_Core PUBLIC ${OpenCV_LIBS} ${GDAL_LIBRARIES})

if(OpenMP_CXX_FOUND)
    target_link_libraries(RSPIP_Core PUBLIC OpenMP::OpenMP_CXX)
endif()

if(MSVC)
    target_link_options(RSPIP_Core PRIVATE $<$<CONFIG:Release>:/LTCG>)
endif()

if(RSPIP_BUILD_CONSOLE)
    add_executable(RSPIP_Console
        app/main.cpp
        samples/ExampleScenarios.cpp)
    target_include_directories(RSPIP_Console PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(RSPIP_Console PRIVATE RSPIP_Core)
    if(MSVC)
        target_link_options(RSPIP_Console PRIVATE $<$<CONFIG:Release>:/LTCG>)
    endif()
endif()

message(STATUS "Built Static Library: RSPIP_Core")
if(RSPIP_BUILD_CONSOLE)
    message(STATUS "Built Console App: RSPIP_Console")
endif()
