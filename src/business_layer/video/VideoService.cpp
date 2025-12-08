#include "business_layer/video_service.h"
// private:

bool VideoService ::addCamera(const CameraInfo& info){
    std::lock_guard<std::mutex> lock(mutex_);
    if (cameras_.count(info.cameraId)) return false;
    cameras_[info.cameraId] = std::make_unique<Camera>(info);
    return true;
}
bool VideoService ::removeCamera(const CameraInfo& info){
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cameras_.find(info.cameraId);
    if(it == cameras_.end()) return false;
    if (!cameras_.count(info.cameraId)) return false;
    nvr_->stop(info.channel,it->second.get());
    cameras_.erase(info.cameraId);
    return true;
}
bool VideoService ::registerDevices(){
    //1.读取配置文件
    auto& config = ConfigParser::getInstance().getConfig(); //这里可能不是这样的
    //2.创建NVR设备
    nvr_ = NVRFactory::createNVR(config.nvr);
    if(!nvr_){
        //日志
        return false;
    }
    //3.初始化NVR设备
    if (!nvr_->initSDK()) {
        //日志
        return false;
    }
    //4.登录NVR设备
    if(!nvr_->login(config.nvr.ip, config.nvr.port, config.nvr.username, config.nvr.password)){
        //日志
        return false;
    }
    //5.创建摄像头 还有绑定摄像头 调用 nvr_->bindCamera(camera)
    std::cout << " 创建摄像头" << std::endl;
    for(auto& camera : config.cameras)
    {
        CameraInfo cameraInfo;
        cameraInfo.cameraId = camera.cameraId;
        cameraInfo.nvrId = camera.nvrId;
        cameraInfo.channel = camera.channelNo;
        cameraInfo.name = camera.name;
        addCamera(cameraInfo);
    }
    std::cout << " 创建摄像头完成" << std::endl;
    return true;
}


// public:
VideoService::VideoService(){
    start();
}

VideoService::~VideoService(){
    stop();
    if(nvr_){
        nvr_->logout();
        nvr_->deinitSDK();
    }
}
void VideoService::start(){
    if(running_){
        return;
    }
    running_ = true;
    if(!registerDevices()){
        // cout << "Failed to register devices" << endl;
        //日志
    }
    //这里 启动nvr 设备才能拿
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "开始进行摄像头的拉流" << std::endl;
    for(auto& camera : cameras_){
        nvr_->start(camera.second->getCameraInfo().channel,camera.second.get());
    }
}

void VideoService::stop(){
    if(!running_){
        return;
    }
    running_ = false;
    std::lock_guard<std::mutex> lock(mutex_);
    // 这里 让nvr 设备才能拿去取消
    for(auto& camera : cameras_){
        nvr_->stop(camera.second->getCameraInfo().channel,camera.second.get());
    }

}




bool VideoService::getDeviceStatus(VideoDerviceStatusInfo& out){
    std::lock_guard<std::mutex> lock(mutex_);
    out.clearCameraStatus();
    if (nvr_) {
        out.setNvrStatus(nvr_->getNVRStatus());
    } else {
        return false;
    }
    for (auto& kv : cameras_) {
        Camera& cam = *kv.second;
        CameraStatusInfo info;
        info.setCameraId(cam.getCameraInfo().cameraId);
        info.setNvrId(cam.getCameraInfo().nvrId);
        info.setStatus(cam.getStatus());
        out.addCameraStatus(info);
    }
    return true;
}

bool VideoService::viewCameraPreviewStream(const PreviewStream& in,PreviewFrame& out){ //单个摄像头的帧
    std::lock_guard<std::mutex> lock(mutex_);

    if (!cameras_.count(in.getCameraId())) return false;
    
    Camera& camera = *cameras_[in.getCameraId()];

    FrameData frame;
    if (!camera.getLastKeyFrame(frame)) return false;
    out.setCameraId(in.getCameraId());
    out.setNvrId(in.getNvrId());
    out.setFrame(frame);
    return true;
}


bool VideoService::getAllLastKeyFrames(VideoFrames& out)
{
    std::lock_guard<std::mutex> lock(mutex_);
    out.clear();
    for(auto& kv : cameras_){
        Camera& cam = *kv.second; 
        FrameData frame;
        if (cam.getLastKeyFrame(frame)) {
            out.addFrame(cam.getCameraInfo().cameraId, frame);
        }
    }
    return true;
}