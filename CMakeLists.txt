cmake_minimum_required(VERSION 3.10)
project(HikSdkDemo)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(TARGET_SOC "rk3568" CACHE STRING "Target SOC for RKNN")  # 匹配 RK356X
set(CMAKE_SYSTEM_NAME "Linux" CACHE STRING "Target system")

message(STATUS "⚠️ 已强制禁用 RGA，使用 CPU 处理图像")
add_definitions(-DDISABLE_RGA)

find_package(OpenCV REQUIRED COMPONENTS 
    core 
    imgproc 
    imgcodecs 
    videoio  # 包含 VideoCapture
    highgui  # 包含 imshow
)
if(OpenCV_FOUND)
    message(STATUS "找到系统 OpenCV: ${OpenCV_VERSION}")
    message(STATUS "OpenCV 头文件: ${OpenCV_INCLUDE_DIRS}")
    message(STATUS "OpenCV 库: ${OpenCV_LIBS}")
else()
    message(FATAL_ERROR "❌ 未找到系统 OpenCV")
endif()

# 先添加 3rdparty（生成 LIBRKNNRT_INCLUDES 等变量）
add_subdirectory(lib/rknn/3rdparty)
# 再添加其他子模块
add_subdirectory(lib/rknn/utils)
add_subdirectory(src)  # src 会自动继承顶层的变量