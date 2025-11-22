cmake_minimum_required(VERSION 3.16)
project(rk3588_service VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/inih
)

# 查找依赖
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)

# 查找MQTT库
pkg_check_modules(LIBMOSQUITTO libmosquitto)
if(LIBMOSQUITTO_FOUND)
    add_definitions(-DHAVE_MQTT)
endif()

# 源文件
set(COMMON_SOURCES
    src/common/logger.cpp
    src/common/config.cpp
)

set(DATA_LAYER_SOURCES
    src/data_layer/sensor_reader.cpp
    src/data_layer/video_capture.cpp
)

set(BUSINESS_LAYER_SOURCES
    src/business_layer/sensor_service.cpp
    src/business_layer/video_service.cpp
    src/business_layer/interaction_service.cpp
    src/business_layer/mqtt_service.cpp
)

set(PRESENTATION_LAYER_SOURCES
    src/presentation_layer/sensor_monitor.cpp
    src/presentation_layer/http_uploader.cpp
)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
    ${COMMON_SOURCES}
    ${DATA_LAYER_SOURCES}
    ${BUSINESS_LAYER_SOURCES}
    ${PRESENTATION_LAYER_SOURCES}
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    ${OpenCV_LIBS}
)

if(LIBMOSQUITTO_FOUND)
    target_link_libraries(${PROJECT_NAME} ${LIBMOSQUITTO_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${LIBMOSQUITTO_INCLUDE_DIRS})
endif()

# RK3588优化
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -O3 
        -mcpu=cortex-a76 
        -mtune=cortex-a76
        -march=armv8.2-a
    )
    
    # RK3588 NPU相关优化（如果有AI推理）
    if(EXISTS "/usr/lib/aarch64-linux-gnu/rknpu")
        target_compile_definitions(${PROJECT_NAME} PRIVATE USE_RKNPU)
        target_link_directories(${PROJECT_NAME} PRIVATE /usr/lib/aarch64-linux-gnu/rknpu)
    endif()
endif()

# 安装目标
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY config/ DESTINATION etc/rk3588_service)