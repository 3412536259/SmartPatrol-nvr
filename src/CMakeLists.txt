cmake_minimum_required(VERSION 3.10)
project(video_ai_system LANGUAGES CXX C)  # 明确语言，兼容C库（如海康SDK）

# ========== 核心配置（全局） ==========
# C++17强制开启，关闭扩展以避免兼容性问题
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# RPATH配置（统一管理，适配嵌入式系统）
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/../../lib:/usr/lib/aarch64-linux-gnu")

# 编译警告（全局启用，提前发现问题）
add_compile_options(-Wall -Wextra -Werror=return-type -Wno-unused-parameter)

# ========== 变量定义（模块化，便于维护） ==========
# 1. 源文件（按模块分组，清晰易查）
set(SRC_FILES
    # 业务层 - 视频
    business_layer/video/HKVDevice.cpp
    business_layer/video/VideoService.cpp
    # 业务层 - 设备管理
    business_layer/device/device_manager.cpp
    # 业务层 - 任务调度
    business_layer/task/job_scheduler.cpp
    business_layer/task/task.cpp
    # 业务层 - MQTT
    business_layer/mqtt/device_status_reporter.cpp
    business_layer/mqtt/mqtt_command_dispatcher.cpp
    business_layer/mqtt/mqtt_service.cpp
    # 业务层 - AI
    business_layer/ai/ai_model_service.cpp
    business_layer/ai/ai_recognize.cpp
    # 数据层
    data_layer/Camera.cpp
    # 表现层
    presentation_layer/web_service.cpp
    presentation_layer/http_command_controller.cpp
    # 测试文本（按需注释）
    text/mqtt_video_text.cpp
    # text/video_text.cpp
    # text/HCNetSDK_text.cpp

    # 公共工具
    ../include/common/base64/base64.cpp
    ../include/common/FileUtils/FileUtils.cpp
    ../include/common/yolo8/postprocess.cc
    ../include/common/yolo8/yolov8.cc
    ../include/common/Imageprocess/ImageProcessor.cpp
    ../include/common/logger/logger.cpp
    ../include/common/config/config_parser.cpp
)

# 2. 库定义（模块化，避免硬编码）
set(MQTT_LIBS paho-mqttpp3 paho-mqtt3as)
set(HIKVISION_LIBS
    hcnetsdk  PlayCtrl  SuperRender  AudioRender  hpr  analyzedata
    AudioIntercom  HCAlarm  HCCoreDevCfg  HCDisplay  HCGeneralCfgMgr
    HCIndustry  HCPlayBack  HCPreview  HCVoiceTalk  StreamTransClient
    SystemTransform
)
set(RKNN_UTIL_LIBS fileutils imagedrawing imageutils audioutils)
set(SYSTEM_CORE_LIBS z dl pthread m)

# ========== 依赖查找（容错+优先级） ==========
# 1. 基础工具
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)  

# 2. FFmpeg（pkg-config，适配嵌入式）
pkg_check_modules(FFMPEG REQUIRED
    libavformat libavcodec libavutil libswscale
)
if(NOT FFMPEG_FOUND)
    message(FATAL_ERROR "FFmpeg not found! Please install FFmpeg development packages.")
endif()

# 3. OpenCV（优先系统，其次3rdparty）
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui videoio)
if(NOT OpenCV_FOUND)
    message(WARNING "System OpenCV not found, try 3rdparty...")
    set(OpenCV_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/3rdparty/opencv/include)
    set(OpenCV_LIBS ${CMAKE_SOURCE_DIR}/3rdparty/opencv/lib/libopencv_core.so)
endif()

# ========== 生成可执行文件 ==========
add_executable(main ${SRC_FILES})

# 目标专属RPATH（适配嵌入式部署）
set_target_properties(main PROPERTIES
    BUILD_RPATH "$ORIGIN/../..:/usr/lib/aarch64-linux-gnu"
    INSTALL_RPATH_USE_LINK_PATH TRUE
    # 解决大工程链接慢问题
    LINK_FLAGS "-Wl,--as-needed"  # 只链接必要的库
)

# ========== 编译宏定义（按需开关） ==========
target_compile_definitions(main PUBLIC
    DISABLE_RGA  # 禁用RGA，强制CPU转换
)

# ========== 头文件路径（去重+绝对路径） ==========
target_include_directories(main PUBLIC
    ${CMAKE_SOURCE_DIR}/include/business_layer/ai
    ${CMAKE_SOURCE_DIR}/include/business_layer/camera
    ${CMAKE_SOURCE_DIR}/include/business_layer/device
    ${CMAKE_SOURCE_DIR}/include/business_layer/http
    ${CMAKE_SOURCE_DIR}/include/business_layer/mqtt
    ${CMAKE_SOURCE_DIR}/include/business_layer/task
    ${CMAKE_SOURCE_DIR}/include/common/base64
    ${CMAKE_SOURCE_DIR}/include/common/config
    ${CMAKE_SOURCE_DIR}/include/common/logger
    ${CMAKE_SOURCE_DIR}/include/common/threads
    ${CMAKE_SOURCE_DIR}/include/common/timestamp
    ${CMAKE_SOURCE_DIR}/include/common/yolo8
    ${CMAKE_SOURCE_DIR}/include/common/Imageprocess
    ${CMAKE_SOURCE_DIR}/include/data_layer
    ${CMAKE_SOURCE_DIR}/include/interfaces
    ${CMAKE_SOURCE_DIR}/include/nvr/branks
    ${CMAKE_SOURCE_DIR}/include/nvr/interfaces
    ${CMAKE_SOURCE_DIR}/include/presentation_layer
    
    ${CMAKE_SOURCE_DIR}/lib/nvr/interfaces
    ${CMAKE_SOURCE_DIR}/lib/json
    
    # 3rdparty 暴露的头文件路径（直接使用变量，无需硬编码）
    ${STB_INCLUDES}
    ${LIBJPEG_INCLUDES}
    ${LIBRKNNRT_INCLUDES}  # 自动指向 RKNN 头文件（如 rknn/3rdparty/rknpu2/include）
    ${LIBRGA_INCLUDES}
    ${LIBSNDFILE_INCLUDES}
    ${LIBFFTW_INCLUDES}
    ${CMAKE_SOURCE_DIR}/rknn/utils
    ${OpenCV_INCLUDE_DIRS}
)

# ========== 库搜索路径（按需添加） ==========
target_link_directories(main PUBLIC
    # 海康SDK库路径
    ${CMAKE_SOURCE_DIR}/lib/nvr/branks/hikvision
    ${CMAKE_SOURCE_DIR}/lib/nvr/branks/hikvision/HCNetSDKCom
    # 系统库路径
    ${FFMPEG_LIBRARY_DIRS}
    ${CMAKE_INSTALL_PREFIX}/lib
)

# ========== 链接库（按依赖优先级排序） ==========
target_link_libraries(main PUBLIC
    # 1. 海康SDK库（底层硬件相关）
    ${HIKVISION_LIBS}
    # 2. RKNN工具库（AI推理）
    ${RKNN_UTIL_LIBS}
    # 3. 第三方库（算法/编解码）
    ${LIBJPEG}
    ${LIBRKNNRT}
    ${LIBRGA}
    ${LIBSNDFILE}
    ${LIBFFTW}
    # 4. 系统核心库（网络/加密/多媒体）
    ${FFMPEG_LIBRARIES}   # FFmpeg
    ${OpenCV_LIBS}        # OpenCV
    OpenSSL::Crypto       # 系统OpenSSL（解决冲突）
    OpenSSL::SSL
    # 5. 基础系统库
    Threads::Threads
    ${SYSTEM_CORE_LIBS}

    ${MQTT_LIBS}          # MQTT（依赖OpenSSL）
)

