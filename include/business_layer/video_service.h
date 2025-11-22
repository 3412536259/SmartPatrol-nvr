#ifndef VIDEO_SERVICE_H
#define VIDEO_SERVICE_H

#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <string>

class IVideoService {
public:
    virtual ~IVideoService() = default;
    virtual bool initialize() = 0;
    virtual void startProcessing() = 0;
    virtual void stopProcessing() = 0;
    virtual bool isProcessing() const = 0;
    virtual void setAlarmCallback(std::function<void(const std::vector<std::string>& detections,
                                                   long timestamp)> callback) = 0;
    virtual void setFrameCallback(std::function<void(const std::vector<uint8_t>& frame_data,
                                                   int width, int height, long timestamp)> callback) = 0;
    virtual void enableDetection(bool enabled) = 0;
    virtual void startRecording() = 0;
    virtual void stopRecording() = 0;
    virtual bool isRecording() const = 0;
};

class VideoService : public IVideoService {
public:
    VideoService(int camera_id, int width, int height, int fps);
    ~VideoService();
    
    bool initialize() override;
    void startProcessing() override;
    void stopProcessing() override;
    bool isProcessing() const override;
    void setAlarmCallback(std::function<void(const std::vector<std::string>& detections,
                                           long timestamp)> callback) override;
    void setFrameCallback(std::function<void(const std::vector<uint8_t>& frame_data,
                                           int width, int height, long timestamp)> callback) override;
    void enableDetection(bool enabled) override;
    void startRecording() override;
    void stopRecording() override;
    bool isRecording() const override;
    
private:
    void processingLoop();
    void handleVideoFrame(const std::vector<uint8_t>& frame_data, int width, int height, long timestamp);
    
    int camera_id_;
    int width_;
    int height_;
    int fps_;
    std::thread processing_thread_;
    std::atomic<bool> processing_{false};
    std::atomic<bool> detection_enabled_{true};
    std::atomic<bool> recording_{false};
    std::function<void(const std::vector<std::string>&, long)> alarm_callback_;
    std::function<void(const std::vector<uint8_t>&, int, int, long)> frame_callback_;
};

#endif