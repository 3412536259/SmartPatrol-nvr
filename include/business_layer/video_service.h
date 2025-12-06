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
   virtual  ~VideoService()  = 0;

    virtual start() = 0;

    virtual stop() = 0;

    virtual getCameraStatus(videoDerviceStatusInfo& videoDerviceStatusInfo) = 0;//获取视频的设备状态（nvr 加上全部摄像头） //上面主动上传上去
    
    virtual viewCameraPreviewStream(const PreviewStream& previewStream,PreviewFrame& previewFrame) = 0; // 获取单个通道的最新预览帧

    virtualVideoFrames getAllLastKeyFrames() = 0;
};

class VideoService : public IVideoService {
    ~VideoService()override;
    VideoService();

    void start() override;

    void stop() override;

    bool getCameraStatus(videoDerviceStatusInfo& out)override;//获取视频的设备状态（nvr 加上全部摄像头） //上面主动上传上去
    
    bool viewCameraPreviewStream(const PreviewStream& previewStream,PreviewFrame& previewFrame) override; // 获取单个通道的最新预览帧

    VideoFrames getAllLastKeyFrames() override;
private:
    bool addCamera(const CameraListConfig& info);   //可能就是有业务需求的要移动走后的摄像头，重新添加到里面
    bool removeCamera(const CameraListConfig& info); //可能就是有业务需求的要移动走
    bool registerDevices();// 把摄像头都注册到这个map表里面，而且初始化SDK，还把NVR进行登陆
    void startAllStreams();//启动所有的通道预览流
private:
    std::unique_ptr<IDevice> nvr_;
    std::map<int, std::unique_ptr<Camera>> cameras_;  //摄像头的在线表（通道信息 + 状态 + 缓存帧）”）  通道号来进行
    std::mutex mutex_; 
    // std::atomic_bool running_ = false;
};

#endif