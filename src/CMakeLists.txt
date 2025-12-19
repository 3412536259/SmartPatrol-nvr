cmake_minimum_required(VERSION 3.10)
project(video_ai_system LANGUAGES CXX C)

# ========== 核心配置（全局） ==========
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ========== 关键：仅定义一次海康路径（删除重复定义） ==========
set(HIK_OPENSSL_ROOT "/home/ztl/workspace/SmartPatrol-nvr/lib/nvr/branks/hikvision")
set(HIK_SDK_LIB_DIR "${HIK_OPENSSL_ROOT}")
set(HIK_SDK_COM_LIB_DIR "${HIK_OPENSSL_ROOT}/HCNetSDKCom")

# RPATH配置：优先海康库，其次系统库（补充-rpath-link参数）
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH 
    "${HIK_OPENSSL_ROOT}"          # 1. 优先海康OpenSSL
    "${HIK_SDK_COM_LIB_DIR}"       # 2. 海康SDK组件库
    "/usr/lib/aarch64-linux-gnu"   # 3. 系统库兜底
    "$ORIGIN/../lib"
)

# 编译警告
add_compile_options(-Wall -Wextra -Werror=return-type -Wno-unused-parameter)

# ========== 变量定义 ==========
set(SRC_FILES
    # 业务层 - 视频
    business_layer/video/HKVDevice.cpp
    # business_layer/video/VideoService.cpp
    # # 业务层 - 设备管理
    # business_layer/device/device_manager.cpp
    # # 业务层 - 任务调度
    # business_layer/task/job_scheduler.cpp
    # business_layer/task/task.cpp
    # # 业务层 - MQTT（核心冲突源）
    # business_layer/mqtt/device_status_reporter.cpp
    # business_layer/mqtt/mqtt_command_dispatcher.cpp
    # business_layer/mqtt/mqtt_service.cpp
    # # 业务层 - AI
    # business_layer/ai/ai_model_service.cpp
    # business_layer/ai/ai_recognize.cpp
    # # 数据层
    data_layer/Camera.cpp
    # # 表现层
    # presentation_layer/web_service.cpp
    # presentation_layer/http_command_controller.cpp
    # 测试文本（按需注释）
    # text/http_image_text.cpp
    # text/status_task_text.cpp
    # text/ai_text.cpp
    # text/mqtt_video_text.cpp
    # text/video_text.cpp
    text/HCNetSDK_text.cpp
    # text/hktext.cpp
    # text/vide_file_text.cpp
    # 公共工具
    # ../include/common/base64/base64.cpp
    # ../include/common/FileUtils/FileUtils.cpp
    # ../include/common/yolo8/postprocess.cc
    # ../include/common/yolo8/yolov8.cc
    # ../include/common/Imageprocess/ImageProcessor.cpp
    # ../include/common/logger/logger.cpp
    ../include/common/config/config_parser.cpp
)

# 库定义：移除海康列表中的crypto/ssl，单独用海康版本链接
set(MQTT_LIBS paho-mqttpp3 paho-mqtt3as)
set(HIKVISION_LIBS
    hcnetsdk  PlayCtrl  SuperRender  AudioRender  hpr  analyzedata
    AudioIntercom  HCAlarm  HCCoreDevCfg  HCDisplay  HCGeneralCfgMgr
    HCIndustry  HCPlayBack  HCPreview  HCVoiceTalk  StreamTransClient
    SystemTransform
    z # 保留zlib，移除crypto/ssl
)
set(RKNN_UTIL_LIBS fileutils imagedrawing imageutils audioutils)
set(SYSTEM_CORE_LIBS dl pthread m)

# ========== 依赖查找 ==========
# 1. 基础工具
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale)

# 2. 强制指定海康OpenSSL（删除重复的路径定义）
# OpenSSL头文件（海康有则用海康，无则用系统1.1）
if(EXISTS "${HIK_OPENSSL_ROOT}/include/openssl/ssl.h")
    set(OPENSSL_INCLUDE_DIR "${HIK_OPENSSL_ROOT}/include")
else()
    set(OPENSSL_INCLUDE_DIR "/usr/include")
    # 校验系统OpenSSL版本（必须是1.1）
    execute_process(COMMAND openssl version OUTPUT_VARIABLE OPENSSL_VERSION)
    if(NOT OPENSSL_VERSION MATCHES "1.1")
        message(FATAL_ERROR "❌ 系统OpenSSL版本不是1.1.x，海康SDK不兼容！")
    endif()
endif()

# 强制链接海康的库文件（硬指定，跳过find）
set(OPENSSL_SSL_LIB "${HIK_OPENSSL_ROOT}/libssl.so.1.1")
set(OPENSSL_CRYPTO_LIB "${HIK_OPENSSL_ROOT}/libcrypto.so.1.1")

# 校验库文件存在性
foreach(LIB_FILE ${OPENSSL_SSL_LIB} ${OPENSSL_CRYPTO_LIB})
    if(NOT EXISTS "${LIB_FILE}")
        message(FATAL_ERROR "❌ 海康OpenSSL库缺失：${LIB_FILE}")
    endif()
endforeach()

# 3. OpenCV
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui videoio)
if(NOT OpenCV_FOUND)
    message(WARNING "System OpenCV not found, try 3rdparty...")
    set(OpenCV_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/3rdparty/opencv/include)
    set(OpenCV_LIBS ${CMAKE_SOURCE_DIR}/3rdparty/opencv/lib/libopencv_core.so)
endif()

# ========== 生成可执行文件 ==========
add_executable(main ${SRC_FILES})

# ========== 编译宏定义 ==========
target_compile_definitions(main PUBLIC
    DISABLE_RGA  # 禁用RGA，强制CPU转换
)

# 消除警告
add_compile_options(
    -Wno-comment          # 多行注释警告
    -Wno-missing-field-initializers # 结构体初始化不完整
    -Wno-unused-variable  # 未使用变量
    -fpermissive          # 兼容默认参数重复定义（临时）
)

# ========== 头文件路径 ==========
target_include_directories(main PUBLIC
    # 业务层头文件
    ${CMAKE_SOURCE_DIR}/include/business_layer/ai
    ${CMAKE_SOURCE_DIR}/include/business_layer/camera
    ${CMAKE_SOURCE_DIR}/include/business_layer/device
    ${CMAKE_SOURCE_DIR}/include/business_layer/http
    ${CMAKE_SOURCE_DIR}/include/business_layer/mqtt
    ${CMAKE_SOURCE_DIR}/include/business_layer/task
    # 公共工具头文件
    ${CMAKE_SOURCE_DIR}/include/common/base64
    ${CMAKE_SOURCE_DIR}/include/common/config
    ${CMAKE_SOURCE_DIR}/include/common/logger
    ${CMAKE_SOURCE_DIR}/include/common/threads
    ${CMAKE_SOURCE_DIR}/include/common/timestamp
    ${CMAKE_SOURCE_DIR}/include/common/yolo8
    ${CMAKE_SOURCE_DIR}/include/common/Imageprocess
    # 数据层/表现层
    ${CMAKE_SOURCE_DIR}/include/data_layer
    ${CMAKE_SOURCE_DIR}/include/interfaces
    ${CMAKE_SOURCE_DIR}/include/nvr/branks
    ${CMAKE_SOURCE_DIR}/include/nvr/interfaces
    ${CMAKE_SOURCE_DIR}/include/presentation_layer
    # 库目录头文件
    ${CMAKE_SOURCE_DIR}/lib/nvr/interfaces
    ${CMAKE_SOURCE_DIR}/lib/json
    # 3rdparty
    ${STB_INCLUDES}
    ${LIBJPEG_INCLUDES}
    ${LIBRKNNRT_INCLUDES}
    ${LIBRGA_INCLUDES}
    ${LIBSNDFILE_INCLUDES}
    ${LIBFFTW_INCLUDES}
    ${CMAKE_SOURCE_DIR}/rknn/utils
    # OpenCV/OpenSSL（优先海康）
    ${OpenCV_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}

    ${CURL_INCLUDE_DIR}
)

# ========== 库搜索路径（调整优先级：海康库优先） ==========
target_link_directories(main PUBLIC
    # 1. 海康SDK库（最高优先级）
    ${HIK_SDK_LIB_DIR}
    ${HIK_SDK_COM_LIB_DIR}
    # 2. 系统库目录
    /usr/lib/aarch64-linux-gnu
    # 3. FFmpeg库目录
    ${FFMPEG_LIBRARY_DIRS}
    ${CMAKE_INSTALL_PREFIX}/lib
)

# ========== 目标属性（修复RPATH，消除警告） ==========
set_target_properties(main PROPERTIES
    # 补充-rpath-link和--disable-new-dtags，强制绑定海康库
    LINK_FLAGS "-Wl,--enable-new-dtags -Wl,--as-needed \
                -Wl,-rpath=${HIK_OPENSSL_ROOT}:${HIK_SDK_COM_LIB_DIR} \
                -Wl,-rpath-link=${HIK_OPENSSL_ROOT} \
                -Wl,--disable-new-dtags"
    SKIP_BUILD_RPATH FALSE
    BUILD_WITH_INSTALL_RPATH TRUE  # 编译时就应用INSTALL_RPATH
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

# ========== 链接库（核心调整：海康OpenSSL优先） ==========
target_link_libraries(main PUBLIC
    # 1. 海康OpenSSL（绝对路径，强制优先）
    ${OPENSSL_SSL_LIB}
    ${OPENSSL_CRYPTO_LIB}
    # 2. 海康SDK核心库（依赖海康OpenSSL）
    ${HIKVISION_LIBS}
    # 3. 系统核心库
    Threads::Threads
    ${SYSTEM_CORE_LIBS}
    # 4. RKNN/第三方算法库
    ${RKNN_UTIL_LIBS}
    ${LIBJPEG}
    ${LIBRKNNRT}
    ${LIBRGA}
    ${LIBSNDFILE}
    ${LIBFFTW}
    # 5. 多媒体/视觉库
    ${FFMPEG_LIBRARIES}
    ${OpenCV_LIBS}
    # 6. MQTT（此时已链接海康OpenSSL，而非系统）
    ${MQTT_LIBS}
    ${CURL_LIBRARIES}
)

# ========== 验证：打印OpenSSL路径（调试用） ==========
message(STATUS "✅ 使用的OpenSSL头文件路径：${OPENSSL_INCLUDE_DIR}")
message(STATUS "✅ 使用的SSL库：${OPENSSL_SSL_LIB}")
message(STATUS "✅ 使用的Crypto库：${OPENSSL_CRYPTO_LIB}")
message(STATUS "✅ 海康SDK库路径：${HIK_SDK_LIB_DIR}")