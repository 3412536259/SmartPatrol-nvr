#include "task.h"
#include "mqtt_service.h"
#include "json.hpp"
#include "ImageProcessor.h"
#include "config_info.h"
#include "config_parser.h"
#include "mqtt_topics.h"
#include "device_info.h"
void GetCameraRealImageTask::run(TaskContext& ctx)
{   
    {
        nlohmann::json ack;
        ack["success"] = true;
        ack["nvrId"] = nvrId_;
        ack["cameraId"] = camId_;
        ctx.publisher->publish(RESULT_GET_REAL_IMAGE_TOPIC, ack.dump());
    }
    PreviewFrame image = ctx.devMgr->getRealImage(camId_,nvrId_);
    image_buffer_t out_image;
    std::vector<unsigned char> outJpeg;
    if(image.getIntegrity())
    {
        ImageProcessor::avframeToRGB(image.getFrame().frame.get(),640,640,&out_image);
        ImageProcessor::compressToJpeg(&out_image,outJpeg);
        std::string imageBase64 = ImageProcessor::jpegToBase64(outJpeg);
        nlohmann::json j;
        j["nvrId"] = nvrId_;
        j["cameraId"] =  camId_;
        j["image"] = imageBase64;
        ctx.publisher->publish(RESULT_GET_REAL_IMAGE_TOPIC, j.dump());
    }
    else{
        nlohmann::json j;
        j["code"] = "no image";
        ctx.publisher->publish(RESULT_GET_REAL_IMAGE_TOPIC, j.dump());
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));

}

void OperateValveTask::run(TaskContext& ctx)
{
    // OperatePLC res = ctx.devMgr->operatePlc(deviceId_, cmd_);
    // nlohmann::json j;
    // if(!res.integrity) j["code"] = "operate failed";
    // j["deviceId"] = deviceId_;
    // j["message"] = res.message;
    // j["status"] = res.status;
    // DeviceConfigRoot cfg = ConfigParser::getInstance().getConfig();
    // ctx.publisher->publish(RESULT_OPERATE_PLC_TOPIC, j.dump());
    // std::this_thread::sleep_for(std::chrono::seconds(1));
}

void GetPLCDeviceTask::run(TaskContext& ctx)
{
    // PLCDeviceState status = ctx.devMgr->getPLCDeviceStatus(deviceId_);
    // nlohmann::json j;
    // j["deviceId"] = status.data.id;
    // j["name"] = status.data.name;
    // j["status"] = status.data.status;
    // ctx.publisher->publish("device/control/result", j.dump());
    // std::this_thread::sleep_for(std::chrono::seconds(1));
}


void GetSensorDataTask::run(TaskContext& ctx)
{
    // RealSensorData rsd = ctx.devMgr->getSensorData(sensorId_);
    // const SensorData& data = rsd.data;
    // nlohmann::json j;
    // j["sensorId"] = sensorId_;
    // j["type"] = data.type;
    // j["status"] = to_string(data.status);

    // if(data.status == SensorStatus::NORMAL)
    // {
    //     if (data.type == "modbus") {
    //         j["temperature"] = data.temperature;
    //         j["humidity"] = data.humidity;
    //     }
    //     if (data.type == "gpio" || data.type == "custom") {
    //         j["value"] = data.value;
    //     }
    // }
    // else
    // {
    //     j["code"] = "no data";
    // }
    // ctx.publisher->publish(RESULT_GET_SENSOR_DATA_TOPIC, j.dump());
    // std::this_thread::sleep_for(std::chrono::seconds(1));
}

void GetDeviceStatusTask::run(TaskContext& ctx)
{   
    {
        nlohmann::json ack;
        ack["success"] = true;  // 由于是获取所有设备的，所以不需要返回设备信息
        ctx.publisher->publish(RESULT_GET_ALL_DEVICE_STATUS_TOPIC, ack.dump());
    }
    // 2. 获取设备状态
    VideoDerviceStatusInfo status = ctx.devMgr->getStatus();
    // 3. 构造JSON响应（规范结构）
    nlohmann::json j;
    j["code"] = 0;          // 0=成功，非0=失败
    j["msg"] = "success";   // 状态描述
    auto& device = j["data"]["device"]; // 规范的data包裹层
    // 4. 处理NVR状态（优先从第一个摄像头取nvrId，若无摄像头则设为空）
    std::string nvrId;
    const auto& cameraList = status.getCameraStatusList();
    if (!cameraList.empty()) {
        nvrId = cameraList.front().getNvrId(); // 取第一个摄像头的NVR ID
    }
    // 5. 填充NVR信息
    device["nvr"]["nvrId"] = nvrId;
    device["nvr"]["nvrStatus"] = status.getNvrStatus() ? "online" : "offline"; // 转为易读字符串
    // 6. 填充摄像头列表
    nlohmann::json cameraArray = nlohmann::json::array();
    for (const auto& camera : cameraList) {
        nlohmann::json camJson;
        camJson["cameraId"] = camera.getCameraId();
        camJson["nvrId"] = camera.getNvrId();
        // 将CameraStatus枚举转为易读字符串（适配前端）
        switch (camera.getStatus()) {
            case CameraStatus::ONLINE:
                camJson["onlineStatus"] = "online";
                break;
            case CameraStatus::RUNNING:
                camJson["onlineStatus"] = "running";
                break;
            case CameraStatus::OFFLINE:
                camJson["onlineStatus"] = "offline";
                break;
            default:
                camJson["onlineStatus"] = "unknown";
                break;
        }

        cameraArray.push_back(camJson);
    }
    device["cameras"] = cameraArray;
    // 7. 处理无摄像头场景
    if (cameraArray.empty()) {
        device["cameras"] = nlohmann::json::array(); // 确保始终是数组类型
        std::cerr << "[GetDeviceStatusTask] No camera status found" << std::endl;
    }
    // for(const auto& devStatus : status.plcStatus_.plcList){
    //     for(const auto& dev : devStatus.deviceStatuses){
    //         device["plc_device"].push_back({
    //             {"deviceId", dev.id},
    //             {"name", dev.name},
    //             {"status", dev.status}
    //         });
    //     }
    // }

    // for (const auto& sensor : status.sensorStatus_.sensors) {
    //     nlohmann::json s;
    //     s["id"] = sensor.id;
    //     s["type"] = sensor.type;
    //     s["status"] = to_string(sensor.status); 

    //     if (sensor.status == SensorStatus::NORMAL) {
    //         if (sensor.type == "modbus") {
    //             s["temperature"] = sensor.temperature;
    //             s["humidity"] = sensor.humidity;
    //         } else if (sensor.type == "gpio" || sensor.type == "custom") {
    //             s["value"] = sensor.value;
    //         }
    //     } else {
    //         s["code"] = "no data";
    //     }
    //     device["sensor"].push_back(s);
    // }

    ctx.publisher->publish(RESULT_GET_ALL_DEVICE_STATUS_TOPIC, j.dump());
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void CarControlTask::run(TaskContext& ctx)
{
    // if (!validatePayload(payload_)) {
    //     nlohmann::json errorResult;
    //     errorResult["success"] = false;
    //     errorResult["error"] = "Invalid payload parameters";
    //     publishResult(ctx.publisher, errorResult);
    //     return;
    // }
    
    // std::string carId = payload_.value("carcontrol_id", std::string("carcontrol001"));
    // int motor1 = payload_.value("motor1", 0);
    // int motor2 = payload_.value("motor2", 0);
    
    // motor1 = std::max(-1500, std::min(1500, motor1));
    // motor2 = std::max(-1500, std::min(1500, motor2));
    
    // std::cout << "Executing car control: car_id=" << carId 
    //           << ", motor1=" << motor1 << ", motor2=" << motor2 << std::endl;
    
    // CarControlResult result = ctx.devMgr->operateCarControl(carId, motor1, motor2);
    
    // nlohmann::json jsonResponse;
    // jsonResponse["success"] = result.success;
    // jsonResponse["carcontrol_id"] = carId;
    // jsonResponse["motor1"] = result.motor1;
    // jsonResponse["motor2"] = result.motor2;
    // jsonResponse["status_byte"] = result.statusByte;
    // jsonResponse["message"] = result.message;
    
    // publishResult(ctx.publisher, jsonResponse);
    
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

bool CarControlTask::validatePayload(const nlohmann::json& payload)
{
    // if (!payload.contains("motor1") && !payload.contains("motor2")) {
    //     return false;
    // }
    
    // if (payload.contains("motor1") && !payload["motor1"].is_number_integer()) {
    //     return false;
    // }
    
    // if (payload.contains("motor2") && !payload["motor2"].is_number_integer()) {
    //     return false;
    // }
    
    // if (payload.contains("car_id") && !payload["car_id"].is_string()) {
    //     return false;
    // }
    
    return true;
}

void CarControlTask::publishResult(ITaskResultPublisher* publisher, const nlohmann::json& result)
{
    if (publisher) {
        publisher->publish("device/carcontrol/result", result.dump());
    }
}