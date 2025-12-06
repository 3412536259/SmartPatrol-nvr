#include "data_layer/video_data_object.h"

CameraStatus Camera::getStatus() {
    std::lock_guard<std::mutex> lock(statusMutex_);
    return status_;
}

bool Camera::getLastKeyFrame(FrameData& out) {
    std::lock_guard<std::mutex> lock(frameMutex_);
    if (!lastKeyFrame_.decoded) return false;
    out = lastKeyFrame_;
    return true;
}
void Camera::onEncodedFrame(uint8_t* data, size_t len) {
    std::lock_guard<std::mutex> lock(frameMutex_);
    if (!codecCtx_ || !frameRGB_) return;
    AVPacket packet;
    av_init_packet(&packet);
        packet.data = data;
        packet.size = static_cast<int>(len);

    // flush 上一个 frame，保证 I 帧覆盖
    avcodec_flush_buffers(codecCtx_);

    int ret = avcodec_send_packet(codecCtx_, &packet);
    if (ret < 0) {
        logFFmpegError("avcodec_send_packet", ret);
        return;
    }

    ret = avcodec_receive_frame(codecCtx_, frameRGB_);
    if (ret < 0) {
        if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            logFFmpegError("avcodec_receive_frame", ret);
        }
        return;
    }

    // 转换为 RGB
    if (!swsCtx_) {
        swsCtx_ = sws_getContext(
            frameRGB_->width, frameRGB_->height, (AVPixelFormat)frameRGB_->format,
            frameRGB_->width, frameRGB_->height, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );
    }
    AVFrame* rgbFrame = av_frame_alloc();
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, frameRGB_->width,
                                            frameRGB_->height, 1);
    std::vector<uint8_t> buffer(numBytes);
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer.data(),
                            AV_PIX_FMT_RGB24, frameRGB_->width, frameRGB_->height, 1);
    sws_scale(swsCtx_, frameRGB_->data, frameRGB_->linesize, 0, frameRGB_->height,
                rgbFrame->data, rgbFrame->linesize);

    // 保存最新帧
    FrameData fd;
    fd.decoded = std::shared_ptr<AVFrame>(rgbFrame, [](AVFrame* f) { av_frame_free(&f); });
    fd.width = frameRGB_->width;
    fd.height = frameRGB_->height;
    fd.timestampMs = timestampMs; //TODO这个 

    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        lastKeyFrame_ = fd;
    }
}

bool Camera::initDecoder() {
    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "Failed to find decoder" << std::endl;
        return false;
    }
    codecCtx_ = avcodec_alloc_context3(codec);
    if (!codecCtx_) return false;

    if (avcodec_open2(codecCtx_, codec, nullptr) < 0) {
        avcodec_free_context(&codecCtx_);
        return false;
    }

    frameRGB_ = av_frame_alloc();
    return frameRGB_ != nullptr;
}



void Camera::updateStatus(CameraStatus s) {
    std::lock_guard<std::mutex> lock(statusMutex_);
    status_ = s;
}

void Camera::logFFmpegError(const char* func, int err) {
    char errbuf[1024];
    av_strerror(err, errbuf, sizeof(errbuf));
    std::cerr << func << " failed: " << errbuf << std::endl;
}


