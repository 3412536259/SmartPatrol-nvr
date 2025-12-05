#include "data_layer/video_data_object.h"

const std::vector<uint8_t>& getData() const { return data_; }
int getWidth() const { return width_; }
int getHeight() const { return height_; }
double getTimestamp() const { return timestamp_; }

    // 修改器
void setData(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    data_ = data;
}

void setWidth(int width) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    width_ = width;
}

void setHeight(int height) {
     std::lock_guard<std::mutex> lock(dataMutex_);
    height_ = height;
}

void setTimestamp(double timestamp) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    timestamp_ = timestamp;
}