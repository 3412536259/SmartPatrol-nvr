#include "nvr/branks/hikvision/HikDevice.h"
//public
bool getLastKeyFrame(FrameData& out) {
    std::lock_guard<std::mutex> lock(frameMutex_);
    out = lastKeyFrame_;
    return true;
}

CameraStatus getStatus() {
    std::lock_guard<std::mutex> lock(statusMutex_);
    return status_;
}  
//prviate
void updateKeyFrame(const FrameData& frame) {
    std::lock_guard<std::mutex> lock(frameMutex_);
    lastKeyFrame_ = frame;
}

void updateStatus(CameraStatus status) {
    std::lock_guard<std::mutex> lock(statusMutex_);
    status_ = status;
}
