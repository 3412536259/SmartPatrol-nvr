#ifndef SENSOR_SERVICE_H
#define SENSOR_SERVICE_H

#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <string>

class ISensorService {
public:
    virtual ~ISensorService() = default;
    virtual bool initialize() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
    virtual void addInfraredSensor(const std::string& sensor_id, int gpio_pin) = 0;
    virtual void addWaterImmersionSensor(const std::string& sensor_id, int gpio_pin) = 0;
    virtual void addSmokeSensor(const std::string& sensor_id, int gpio_pin) = 0;
    virtual void setAlarmCallback(std::function<void(const std::string& alarm_type, 
                                                   const std::string& reason)> callback) = 0;
    virtual void setDataCallback(std::function<void(const std::string& sensor_type,
                                                  const std::string& sensor_id,
                                                  bool state, long timestamp)> callback) = 0;
};

class SensorService : public ISensorService {
public:
    SensorService();
    ~SensorService();
    
    bool initialize() override;
    void start() override;
    void stop() override;
    bool isRunning() const override;
    void addInfraredSensor(const std::string& sensor_id, int gpio_pin) override;
    void addWaterImmersionSensor(const std::string& sensor_id, int gpio_pin) override;
    void addSmokeSensor(const std::string& sensor_id, int gpio_pin) override;
    void setAlarmCallback(std::function<void(const std::string& alarm_type, 
                                           const std::string& reason)> callback) override;
    void setDataCallback(std::function<void(const std::string& sensor_type,
                                          const std::string& sensor_id,
                                          bool state, long timestamp)> callback) override;
    
private:
    void monitoringLoop();
    void handleGPIOSensor(const std::string& sensor_type, const std::string& sensor_id, 
                         int gpio_pin, bool triggered);
    
    std::thread monitoring_thread_;
    std::atomic<bool> running_{false};
    std::function<void(const std::string&, const std::string&)> alarm_callback_;
    std::function<void(const std::string&, const std::string&, bool, long)> data_callback_;
};

#endif