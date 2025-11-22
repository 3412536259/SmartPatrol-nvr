#ifndef HTTP_SERVICE_H
#define HTTP_SERVICE_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include "business_layer/sensor_service.h"
#include "business_layer/video_service.h"
#include "business_layer/interaction_service.h"

// 前向声明
class SensorService;
class VideoService;
class InteractionService;

class HTTPService {
public:
    struct HttpRequest {
        std::string method;
        std::string path;
        std::string body;
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> query_params;  // 新增：查询参数
    };

    struct HttpResponse {
        int status_code;
        std::string body;
        std::string content_type;
        
        HttpResponse(int code = 200, const std::string& content = "", 
                    const std::string& type = "application/json") 
            : status_code(code), body(content), content_type(type) {}
    };

    // 上传数据结构
    struct ImageData {
        std::vector<uint8_t> data;
        std::string format;  // "jpg", "png"等
        std::string timestamp;
    };

    struct SensorData {
        std::string type;
        std::string value;
        std::string timestamp;
    };

public:
    HTTPService(std::shared_ptr<SensorService> sensor_service,
                std::shared_ptr<VideoService> video_service,
                std::shared_ptr<InteractionService> interaction_service);
    ~HTTPService();
    
    // 服务器管理
    bool initialize(int port = 8080);
    bool start();
    bool stop();
    bool isRunning() const;
    
    // ========== 主动上传功能 - 业务层主动调用 ==========
    bool uploadImage(const ImageData& image_data);
    bool uploadSensorData(const SensorData& sensor_data);
    bool uploadEvent(const std::string& event_type, const std::string& event_data);
    
    // ========== 批量上传功能 ==========
    bool uploadImages(const std::vector<ImageData>& images);
    bool uploadSensorDataBatch(const std::vector<SensorData>& sensor_data_list);

private:
    // 依赖的业务层服务
    std::shared_ptr<SensorService> sensor_service_;
    std::shared_ptr<VideoService> video_service_;
    std::shared_ptr<InteractionService> interaction_service_;
    
    // ========== API请求处理器 - HTTP请求触发 ==========
    // 门锁控制
    HttpResponse handleDoorLock(const HttpRequest& request);
    HttpResponse handleDoorLockStatus(const HttpRequest& request);
    
    // 报警控制
    HttpResponse handleAlarmControl(const HttpRequest& request);
    HttpResponse handleAlarmStatus(const HttpRequest& request);
    
    // 摄像头控制
    HttpResponse handleCameraControl(const HttpRequest& request);
    HttpResponse handleCameraStatus(const HttpRequest& request);
    
    // 传感器相关
    HttpResponse handleSensorStatus(const HttpRequest& request);
    HttpResponse handleSensorHistory(const HttpRequest& request);
    
    // 视频流相关
    HttpResponse handleVideoStream(const HttpRequest& request);
    HttpResponse handleVideoRecord(const HttpRequest& request);
    
    // 图片相关
    HttpResponse handleImageUpload(const HttpRequest& request);  // HTTP上传
    HttpResponse handleImageQuery(const HttpRequest& request);
    
    // 系统状态
    HttpResponse handleSystemStatus(const HttpRequest& request);
    HttpResponse handleSystemConfig(const HttpRequest& request);
    
    // 工具方法
    std::string generateTimestamp() const;
    bool validateImageData(const ImageData& data) const;
    bool validateSensorData(const SensorData& data) const;
    
    // 内部实现
    class Impl;
    std::unique_ptr<Impl> impl_;
};

#endif