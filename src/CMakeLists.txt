cmake_minimum_required(VERSION 3.10)
project(src)


set(SRC_FILES
    main.cpp

  
)

add_executable(main ${SRC_FILES})
target_compile_definitions(main PUBLIC
    DISABLE_RGA  # 启用禁用 RGA 的宏，强制使用 CPU 转换
    # 其他已有的宏（如果有）...
)

# -------------------------- 新增：启用 pkg-config 用于查找系统 FFmpeg 库 --------------------------
find_package(PkgConfig REQUIRED)
# 查找系统安装的 FFmpeg 核心库（自动获取头文件和库路径）
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale)


# -------------------------- 1. 配置头文件路径（用 target_include_directories）--------------------------
target_include_directories(main PUBLIC
    # 项目自有头文件路径
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/inlcude
    ${CMAKE_SOURCE_DIR}/lib


    # 3rdparty 暴露的头文件路径（直接使用变量，无需硬编码）
    ${STB_INCLUDES}
    ${LIBJPEG_INCLUDES}
    ${LIBRKNNRT_INCLUDES}  # 自动指向 RKNN 头文件（如 rknn/3rdparty/rknpu2/include）
    ${LIBRGA_INCLUDES}
    ${LIBSNDFILE_INCLUDES}
    ${LIBFFTW_INCLUDES}
    ${OpenCV_INCLUDE_DIRS}
)

# -------------------------- 2. 配置库文件路径（用 target_link_directories）--------------------------
target_link_directories(main PUBLIC
    # 海康 SDK 库路径
    ${CMAKE_SOURCE_DIR}/lib/nvr/branks/hikvision
    ${CMAKE_SOURCE_DIR}/lib/nvr/branks/hikvision/HCNetSDKCom
    # RKNN 库路径（3rdparty 已通过 LIBRKNNRT 提供完整路径，可省略此步）
    # 新增：添加系统 FFmpeg 库路径
    ${FFMPEG_LIBRARY_DIRS}
)

# -------------------------- 3. 链接依赖库（用 target_link_libraries）--------------------------
target_link_libraries(main PUBLIC
    # 1. 海康 SDK 库
    hcnetsdk  
    PlayCtrl       
    SuperRender    
    AudioRender    
    hpr            
    analyzedata    
    AudioIntercom  
    HCAlarm        
    HCCoreDevCfg   
    HCDisplay     
    HCGeneralCfgMgr
    HCIndustry     
    HCPlayBack     
    HCPreview      
    HCVoiceTalk    
    StreamTransClient
    SystemTransform       
    crypto         
    ssl            
    z    
    # 2. rknn/utils 生成的库
    fileutils
    imagedrawing
    imageutils
    audioutils
    # 3. 3rdparty 库（直接使用变量，自动指向正确库文件）
    ${LIBJPEG}
    ${LIBRKNNRT}  # 自动指向 librknnrt.so（3rdparty 已配置）
    ${LIBRGA}
    ${LIBSNDFILE}
    ${LIBFFTW}
     # 4. FFmpeg 库：替换原有的 avutil/swscale 等，改用系统库
    ${FFMPEG_LIBRARIES}  # 自动链接系统的 libavformat、libavcodec、libavutil、libswscale
    # 5. 系统库
    ${OpenCV_LIBS}
    dl
    pthread
    m
)