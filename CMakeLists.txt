cmake_minimum_required(VERSION 3.15)
project(RSPIPUtils)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# === 1. 依赖查找 ===
# 注意：请确保这些路径在你的机器上是正确的
set(GDAL_ROOT "D:/C++_Library/GDAL/Release")
set(GDAL_INCLUDE_DIR "${GDAL_ROOT}/include")
set(GDAL_LIBRARY "${GDAL_ROOT}/lib/gdal.lib")

find_package(OpenCV REQUIRED)
find_package(OpenMP REQUIRED)
find_package(GDAL REQUIRED)

# === 2. 收集源文件 ===
# 递归查找 src 下所有的 cpp 文件
file(GLOB_RECURSE ALL_SOURCES "src/*.cpp")

# 定义 main.cpp 的路径 (用于排除)
# 使用 get_filename_component 确保路径格式一致，防止排除失败
get_filename_component(MAIN_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp" ABSOLUTE)

# 创建库文件的源文件列表 (排除 main.cpp)
set(LIB_SOURCES ${ALL_SOURCES})
list(REMOVE_ITEM LIB_SOURCES ${MAIN_SOURCE_PATH})

# === 3. 定义核心静态库 (RSPIP_Core) ===
# 这个库包含了所有的算法逻辑，供控制台程序和未来的 UI 程序调用
add_library(RSPIP_Core STATIC ${LIB_SOURCES})

# 设置头文件包含路径
target_include_directories(RSPIP_Core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
    ${GDAL_INCLUDE_DIRS}
    "D:/C++_Library/eigen-5.0.1" # Eigen 路径
)

# 链接依赖库到核心库
# 使用 PUBLIC 关键字，这样链接了 RSPIP_Core 的程序会自动继承这些依赖
target_link_libraries(RSPIP_Core PUBLIC ${OpenCV_LIBS})
target_link_libraries(RSPIP_Core PUBLIC ${GDAL_LIBRARIES})

if(OpenMP_CXX_FOUND)
    target_link_libraries(RSPIP_Core PUBLIC OpenMP::OpenMP_CXX)
endif()

# MSVC 优化选项
if (MSVC)
    target_compile_options(RSPIP_Core PRIVATE /LTCG)
endif()

# # === 4. 定义原来的控制台测试程序 (RSPIP_Console) ===
# # 它只包含 main.cpp，并链接核心库
add_executable(RSPIP_Console src/main.cpp)

# 链接核心库
target_link_libraries(RSPIP_Console PRIVATE RSPIP_Core)

message(STATUS "Built Static Library: RSPIP_Core")
message(STATUS "Built Console App: RSPIP_Console")