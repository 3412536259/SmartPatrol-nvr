# src/business_layer/mqtt/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

# 定义MQTT模块的动态库项目（仅编译MQTT相关代码）
project(mqtt_module LANGUAGES CXX)

# ========== 基础配置 ==========
# C++17标准（和主程序保持一致）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# RPATH配置（确保动态库能找到自身依赖的MQTT/OpenSSL库）
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN/:/usr/lib/aarch64-linux-gnu")

# ========== 源文件（仅MQTT相关） ==========
set(MQTT_SRC_FILES
    mqtt_service.cpp
    mqtt_command_dispatcher.cpp
    # 若有其他MQTT相关cpp，仅在此添加
)

# ========== 编译动态库 ==========
add_library(mqtt_module SHARED ${MQTT_SRC_FILES})

# ========== 头文件路径 ==========
# 仅添加MQTT模块依赖的头文件，不引入海康SDK相关路径
target_include_directories(mqtt_module PUBLIC
    # 工程根目录的include（找MQTT头文件）
    ${CMAKE_SOURCE_DIR}/include
    # MQTT模块自身头文件目录
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include/business_layer/mqtt
    # MQTT依赖的其他公共头文件（按需添加）
    ${CMAKE_SOURCE_DIR}/include/business_layer/task  # 若依赖job_scheduler
    ${CMAKE_SOURCE_DIR}/include/common/logger       # 若依赖日志
    # 系统OpenSSL头文件（MQTT依赖）
    ${OPENSSL_INCLUDE_DIR}
    # MQTT库的头文件（系统默认路径，若自定义需手动指定）
    # /usr/include/paho-mqttpp3
)

# ========== 依赖查找 ==========
# 1. 查找MQTT库（paho-mqttpp3/paho-mqtt3as）
find_library(PAHO_MQTTPP3_LIB paho-mqttpp3 REQUIRED)
find_library(PAHO_MQTT3AS_LIB paho-mqtt3as REQUIRED)

# 2. 查找OpenSSL（MQTT依赖的加密库）
find_package(OpenSSL REQUIRED)

# 3. 线程库（MQTT异步通信依赖）
find_package(Threads REQUIRED)

# ========== 链接依赖（仅MQTT需要的库，无海康SDK） ==========
target_link_libraries(mqtt_module PUBLIC
    ${PAHO_MQTTPP3_LIB}          # MQTT C++异步客户端库
    ${PAHO_MQTT3AS_LIB}          # MQTT C底层库（带SSL/异步）
    OpenSSL::Crypto               # 系统OpenSSL加密库
    OpenSSL::SSL                  # 系统OpenSSL SSL库
    Threads::Threads              # 线程库
    dl                            # 动态链接库（可选）
    pthread                       # 线程依赖
    m                             # 数学库（可选）
)

# ========== 编译选项（可选，优化+警告） ==========
target_compile_options(mqtt_module PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter  # 屏蔽无关警告
    -O2                    # 编译优化
    -fPIC                  # 生成位置无关代码（动态库必需）
)

# ========== 安装规则（可选，便于部署） ==========
# 安装MQTT动态库到主程序的lib目录
install(TARGETS mqtt_module
    LIBRARY DESTINATION ${CMAKE_SOURCE_DIR}/build/lib  # 主程序build/lib目录
    ARCHIVE DESTINATION ${CMAKE_SOURCE_DIR}/build/lib
)