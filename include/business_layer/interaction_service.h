#ifndef INTERACTION_SERVICE_H
#define INTERACTION_SERVICE_H

#include <memory>
#include <atomic>

// 前向声明
class VideoService;

class IInteractionService {
public:
    virtual ~IInteractionService() = default;
    virtual bool initialize() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void processInfraredEvent(bool triggered, const std::string& sensor_id) = 0;
    virtual void processWaterImmersionEvent(bool immersed, const std::string& sensor_id) = 0;
    virtual void processSmokeEvent(bool detected, const std::string& sensor_id) = 0;
    virtual void setVideoService(std::shared_ptr<VideoService> video_service) = 0;
    virtual bool isRecording() const = 0;
};

class SensorVideoInteractionService : public IInteractionService {
public:
    SensorVideoInteractionService();
    ~SensorVideoInteractionService();
    
    bool initialize() override;
    void start() override;
    void stop() override;
    void processInfraredEvent(bool triggered, const std::string& sensor_id) override;
    void processWaterImmersionEvent(bool immersed, const std::string& sensor_id) override;
    void processSmokeEvent(bool detected, const std::string& sensor_id) override;
    void setVideoService(std::shared_ptr<VideoService> video_service) override;
    bool isRecording() const override;
    
private:
    void startVideoRecording();
    void stopVideoRecording();
    
    std::shared_ptr<VideoService> video_service_;
    std::atomic<bool> video_recording_{false};
    std::atomic<bool> enabled_{false};
};

#endif