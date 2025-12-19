#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H
#include "video_service.h"
#include "video_data_object.h"
// #include "isensor_manager.h"
#include "idevice_manager.h"

class DeviceManager : public IDeviceManager{
public:
    DeviceManager();
    ~DeviceManager();
    VideoDerviceStatusInfo  getStatus() override;

    VideoFrames getAllRealImage() override;
    PreviewFrame getRealImage(const std::string& camId , const std::string& nvrId) override;
    void getAllHistoryImage() override;

    void getHistoryImage(const std::string& camId) override;
    void operateCamera() override;
    void operatePlc(const std::string &deviceId, const std::string &cmd) override;
    void updateConfig() override;
    void queryRecordFiles(std::string camId_,std::string startTime_,std::string endTime_,VideoFiles videoFiles) override;
private:
    std::shared_ptr<IVideoService> videoService_;
    // std::shared_ptr<IPLCManager> plcManager_;
    // std::shared_ptr<ISensorManager> sensorManager_;

};

#endif