#ifndef I_DEVICE_MANAGER_H
#define I_DEVICE_MANAGER_H
#include <string>
#include "video_data_object.h"

class IDeviceManager{
public:
    virtual ~IDeviceManager() = default;

    virtual VideoDerviceStatusInfo getStatus() = 0; //设备状态获取

    virtual PreviewFrame getRealImage(const std::string& camId , const std::string& nvrId) = 0; //获取对应摄像头实时图片
    virtual VideoFrames getAllRealImage() = 0; //获取所有摄像头实时图片
    virtual void getHistoryImage(const std::string& camId) = 0;
    virtual void getAllHistoryImage() = 0;
    
    virtual void operateCamera() = 0;
    virtual void operatePlc(const std::string &deviceId, const std::string &cmd) = 0;

    virtual void updateConfig() = 0; //更新配置

    virtual void queryRecordFiles(std::string camId_,std::string startTime_,std::string endTime_,VideoFiles videoFiles);


};



#endif