#ifndef VIDEO_PROCESSOR_H
#define VIDEO_PROCESSOR_H

#include <memory>
#include <vector>
#include <string>

class IVideoProcessor {
public:
    virtual ~IVideoProcessor() = default;
    virtual bool initialize() = 0;
    virtual std::vector<uint8_t> encodeH264(const std::vector<uint8_t>& raw_frame, 
                                           int width, int height) = 0;
    virtual std::vector<uint8_t> encodeH265(const std::vector<uint8_t>& raw_frame,
                                           int width, int height) = 0;
    virtual std::vector<uint8_t> encodeJPEG(const std::vector<uint8_t>& raw_frame,
                                           int width, int height, int quality = 80) = 0;
    virtual std::vector<uint8_t> decodeH264(const std::vector<uint8_t>& encoded_data) = 0;
    virtual std::vector<uint8_t> decodeH265(const std::vector<uint8_t>& encoded_data) = 0;
    virtual std::vector<uint8_t> resizeFrame(const std::vector<uint8_t>& frame_data,
                                            int src_width, int src_height,
                                            int dst_width, int dst_height) = 0;
};

class VideoProcessor : public IVideoProcessor {
public:
    VideoProcessor();
    ~VideoProcessor();
    
    bool initialize() override;
    std::vector<uint8_t> encodeH264(const std::vector<uint8_t>& raw_frame, 
                                   int width, int height) override;
    std::vector<uint8_t> encodeH265(const std::vector<uint8_t>& raw_frame,
                                   int width, int height) override;
    std::vector<uint8_t> encodeJPEG(const std::vector<uint8_t>& raw_frame,
                                   int width, int height, int quality = 80) override;
    std::vector<uint8_t> decodeH264(const std::vector<uint8_t>& encoded_data) override;
    std::vector<uint8_t> decodeH265(const std::vector<uint8_t>& encoded_data) override;
    std::vector<uint8_t> resizeFrame(const std::vector<uint8_t>& frame_data,
                                    int src_width, int src_height,
                                    int dst_width, int dst_height) override;
    
private:
    // RK3588硬件编解码实现
};

#endif