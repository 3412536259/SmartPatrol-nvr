#include "business_layer/video_service.h"
// private:

bool VideoService ::addCamera(const CameraListConfig& info){
    std::lock_guard<std::mutex> lock(mutex_);
    int no = CameraChangeChannelNo(info.camera_id);//
    if (cameras_.count(no)) return false;
    cameras_[no] = std::make_unique<Camera>(info);
    cameras_[no].initDecoder();
}
bool VideoService ::removeCamera(const CameraListConfig& info){
    std::lock_guard<std::mutex> lock(mutex_);
    int no = CameraChangeChannelNo(info.camera_id);
    if (cameras_.count(no)) return false;
    nvr_.stop(no,cameras_[no]);
    camera_.erase(no);
    return true;
}
bool VideoService ::registerDevices(){
    //1.读取配置文件
    auto& config = ConfigParser::getInstance().getVideoConfig(); //这里可能不是这样的
    //2.创建NVR设备
    nvr = NVRFactory::createNVR(config.nvr);
    if(!nvr){
        //日志
        return false;
    }
    //3.初始化NVR设备
    if (!nvr->initSDK()) {
        //日志
        return false;
    }
    //4.登录NVR设备
    if(!nvr->login(config.nvr.ip, config.nvr.port, config.nvr.username, config.nvr.password)){
        //日志
        return false;
    }
    //5.创建摄像头 还有绑定摄像头 调用 nvr_->bindCamera(camera)
    std::lock_guard<std::mutex> lock(mutex_);
    for(auto& camera : config.cameras)
    {
        CameraListConfig cameraInfo;
        cameraInfo.camera_id = camera.camera_id;
        cameraInfo.channel_no = camera.channel_no;
        cameraInfo.name = camera.name;
        addCamera(cameraInfo);
    }
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
void VideoService ::start(){
    if(running_){
        return;
    }
    running_ = true;
    if(!registerDevices()){
        // cout << "Failed to register devices" << endl;
        //日志
    }
    std::lock_guard<std::mutex> lock(mutex_);
    //这里 启动nvr 设备才能拿
    std::lock_guard<std::mutex> lock(mutex_);
    for(auto& camera : cameras_){
        nvr_.start(camera.channel_no,camera);
    }
}

void VideoService ::stop(){
    if(!running_){
        return;
    }
    running_ = false;
    std::lock_guard<std::mutex> lock(mutex_);
    // 这里 让nvr 设备才能拿去取消
    for(auto& camera : cameras_){
        nvr_.stop(camera.channel_no,camera);
    }

}




bool VideoService::getCameraStatus(videoDerviceStatusInfo& out){
    std::lock_guard<std::mutex> lock(mutex_);


    

}

bool VideoService::viewCameraPreviewStream(const PreviewStream& previewStream,PreviewFrame& out){
    //1.这个是previewStream 从云端来参数，然后去获取这个摄像头的预览流帧
    

    //2.这个是把最新的帧 previewFrame 预览帧给云端，


}