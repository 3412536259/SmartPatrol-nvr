#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

#include <string>
#include <memory>
#include <functional>
#include <vector>

class IMQTTService {
public:
    virtual ~IMQTTService() = default;
    virtual bool initialize() = 0;
    virtual bool connect() = 0;
    virtual bool disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual void publishSensorData(const std::string& sensor_type, const std::string& sensor_id, 
                                  bool triggered, long timestamp) = 0;
    virtual void publishImageUrl(const std::string& camera_id, const std::string& image_url, 
                                long timestamp) = 0;
    virtual void publishDetectionResults(const std::string& camera_id, 
                                        const std::vector<std::string>& detections,
                                        long timestamp) = 0;
};

class MQTTService : public IMQTTService {
public:
    MQTTService(const std::string& broker, int port);
    ~MQTTService();
    
    bool initialize() override;
    bool connect() override;
    bool disconnect() override;
    bool isConnected() const override;
    void publishSensorData(const std::string& sensor_type, const std::string& sensor_id, 
                          bool triggered, long timestamp) override;
    void publishImageUrl(const std::string& camera_id, const std::string& image_url, 
                        long timestamp) override;
    void publishDetectionResults(const std::string& camera_id, 
                                const std::vector<std::string>& detections,
                                long timestamp) override;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

#endif