#include "video_data_object.h"

CameraStatus Camera::getStatus() {
    std::lock_guard<std::mutex> lock(statusMutex_);
    return status_;
}

bool Camera::getLastKeyFrame(FrameData& out) {
    if (!lastKeyFrame_.frame || lastKeyFrame_.width == 0 || lastKeyFrame_.height == 0) {
        return false;
    }

    AVFrame* clone = av_frame_clone(lastKeyFrame_.frame.get());
    if (!clone) {
        return false;
    }

    out.frame = std::shared_ptr<AVFrame>(clone, [](AVFrame* f){ av_frame_free(&f); });
    out.width = lastKeyFrame_.width;
    out.height = lastKeyFrame_.height;
    out.lastKeyFrameTime = lastKeyFrame_.lastKeyFrameTime;
    return true;
}

void Camera::onEncodedFrame(uint8_t* data, size_t len) {
    if (!data || len == 0 || !codecCtx_) {
        updateStatus(CameraStatus::OFFLINE);
        return;
    }
  
    if (!decoderInitialized_) {
        AVCodecID id = detectCodec(data, len);
        if (id == AV_CODEC_ID_NONE) {
            std::cerr << "[Camera] 未检测到有效编码类型" << std::endl;
            return;
        }

        const AVCodec* codec = avcodec_find_decoder(id);
        codecCtx_->codec_id = id;
        codecCtx_->codec_type = AVMEDIA_TYPE_VIDEO;

        if (avcodec_open2(codecCtx_, codec, nullptr) < 0) {
            return;
        }
        decoderInitialized_ = true;
    }
  
    // 分配并初始化AVPacket
    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        return;
    }

    pkt->data = data;
    pkt->size = static_cast<int>(len);

    // 发送数据包到解码器（加锁避免多线程冲突）
    int ret = 0;
    {
        std::lock_guard<std::mutex> lock(decoderMutex_);
        ret = avcodec_send_packet(codecCtx_, pkt);
    }

    if (ret < 0) {
        logFFmpegError("avcodec_send_packet", ret);
        av_packet_free(&pkt);
        updateStatus(CameraStatus::OFFLINE);
        return;
    }

    // 接收解码后的帧
    {
        std::lock_guard<std::mutex> lock(decoderMutex_);
        ret = avcodec_receive_frame(codecCtx_, frameYUV_);
    }

    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        av_packet_free(&pkt);
        return;
    } else if (ret < 0) {
        logFFmpegError("avcodec_receive_frame", ret);
        av_packet_free(&pkt);
        updateStatus(CameraStatus::OFFLINE);
        return;
    }
    
    // 只处理关键帧（保留YUV原始帧，删除RGB转换）
    if (frameYUV_->key_frame == 1) {
        // 验证YUV帧数据有效性
        if (!frameYUV_->data[0] || !frameYUV_->data[1] || !frameYUV_->data[2]) {
            std::cerr << "[Camera] 通道" << info_.channel << " YUV帧指针为空" << std::endl;
            av_packet_free(&pkt);
            return;
        }

        // 直接保存YUV关键帧（不再转换为RGB）
        AVFrame* keyFrameCopy = av_frame_clone(frameYUV_);
        if (!keyFrameCopy) {
            std::cerr << "Failed to clone frameYUV_ for lastKeyFrame_" << std::endl;
        } else {
            std::lock_guard<std::mutex> lock(frameMutex_);  // 改用frameMutex_（删除了frameRGBMutex_）
            lastKeyFrame_.frame = std::shared_ptr<AVFrame>(keyFrameCopy, [](AVFrame* f){
                av_frame_free(&f);
            });
            lastKeyFrame_.width = frameYUV_->width;
            lastKeyFrame_.height = frameYUV_->height;
            lastKeyFrame_.lastKeyFrameTime = av_gettime_relative();
        }

        av_packet_free(&pkt);
        // 重置YUV帧（准备接收下一帧）
        av_frame_unref(frameYUV_);
    }
}

// 打印FFmpeg错误信息
void Camera::logFFmpegError(const char* func, int err) {
    char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(err, err_buf, sizeof(err_buf));
    std::cerr << "FFmpeg error in " << func << ": " << err_buf << " (code: " << err << ")" << std::endl;
}

bool Camera::initDecoder() {
    // 注册所有解码器（兼容旧版FFmpeg）
    // avcodec_register_all();

    // 先不指定具体解码器，在首次解码时动态检测
    codecCtx_ = avcodec_alloc_context3(nullptr);
    if (!codecCtx_) {
        std::cerr << "[Camera] 通道" << info_.channel << " 分配解码器上下文失败" << std::endl;
        return false;
    }

    // 开启自动检测编码格式
    codecCtx_->flags2 |= AV_CODEC_FLAG2_CHUNKS;

    return true;
}

AVCodecID Camera::detectCodec(uint8_t* data, size_t len) {
    if (len < 5) return AV_CODEC_ID_NONE;

    // AnnexB 00 00 00 01
    if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01) {
        uint8_t nal = data[4];

        if ((nal & 0x1F) == 7) return AV_CODEC_ID_H264; // H264 SPS
        if ((nal >> 1) == 0x20) return AV_CODEC_ID_HEVC; // H265 VPS
    }
    return AV_CODEC_ID_NONE;
}

// 更新最新关键帧（带锁）
void Camera::updateKeyFrame(FrameData newKeyFrame) {
    std::lock_guard<std::mutex> lock(frameMutex_);
    lastKeyFrame_ = newKeyFrame;
}

// 更新状态（带锁）
void Camera::updateStatus(CameraStatus status) {
    std::lock_guard<std::mutex> lock(statusMutex_);
    status_ = status;
}