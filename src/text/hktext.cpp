#include <iostream>
#include <string>
#include <map>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <stdexcept>

// 引入海康C++ SDK头文件（根据实际路径调整）
#include "hikvision/hikDevice.h"

// 引入JSON库（nlohmann/json，需提前安装或引入头文件）
#include "nlohmann/json.hpp"
using json = nlohmann::json;

// 设备登录句柄（模拟Java版HCLoginSDK，实际需从登录接口获取）
struct HCLoginSDK {
    LONG lUserID = -1; // 海康SDK登录句柄，-1表示未登录

    LONG getLUserID() const {
        return lUserID;
    }

    // 模拟登录（实际需调用NET_DVR_Login_V40）
    bool login(const std::string& ip, int port, const std::string& user, const std::string& pwd) {
        NET_DVR_USER_LOGIN_INFO loginInfo = {0};
        strncpy(loginInfo.sDeviceAddress, ip.c_str(), sizeof(loginInfo.sDeviceAddress) - 1);
        loginInfo.wPort = port;
        strncpy(loginInfo.sUserName, user.c_str(), sizeof(loginInfo.sUserName) - 1);
        strncpy(loginInfo.sPassword, pwd.c_str(), sizeof(loginInfo.sPassword) - 1);

        NET_DVR_DEVICEINFO_V40 deviceInfo = {0};
        lUserID = NET_DVR_Login_V40(&loginInfo, &deviceInfo);
        if (lUserID < 0) {
            std::cerr << "设备登录失败，错误码：" << NET_DVR_GetLastError() << std::endl;
            return false;
        }
        return true;
    }

    // 登出设备
    void logout() {
        if (lUserID >= 0) {
            NET_DVR_Logout(lUserID);
            lUserID = -1;
        }
    }
};

// 工具类（对齐Java版Utils）
class Utils {
public:
    /**
     * @brief 将yyyy-MM-dd HH:mm:ss格式字符串转为SDK时间结构体
     * @param timeStr 时间字符串（如"2025-01-01 08:00:00"）
     * @return NET_DVR_TIME SDK时间结构体
     */
    static NET_DVR_TIME getNvrTime(const std::string& timeStr) {
        NET_DVR_TIME nvrTime = {0};
        std::tm tm = {0};
        std::istringstream iss(timeStr);
        // 解析yyyy-MM-dd HH:mm:ss
        iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        if (iss.fail()) {
            throw std::invalid_argument("时间格式错误，需为yyyy-MM-dd HH:mm:ss");
        }

        nvrTime.dwYear = tm.tm_year + 1900;    // tm_year是从1900开始的差值
        nvrTime.dwMonth = tm.tm_mon + 1;       // tm_mon是0-11，SDK是1-12
        nvrTime.dwDay = tm.tm_mday;
        nvrTime.dwHour = tm.tm_hour;
        nvrTime.dwMinute = tm.tm_min;
        nvrTime.dwSecond = tm.tm_sec;

        return nvrTime;
    }

    /**
     * @brief 将SDK时间结构体转为yyyy-MM-dd HH:mm:ss字符串
     * @param sdkTime SDK时间结构体
     * @return 格式化时间字符串
     */
    static std::string sdkTimeToStr(const NET_DVR_TIME& sdkTime) {
        std::tm tm = {0};
        tm.tm_year = sdkTime.dwYear - 1900;
        tm.tm_mon = sdkTime.dwMonth - 1;
        tm.tm_mday = sdkTime.dwDay;
        tm.tm_hour = sdkTime.dwHour;
        tm.tm_min = sdkTime.dwMinute;
        tm.tm_sec = sdkTime.dwSecond;

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
};

// 摄像头参数结构体（对齐Java版CameraPojo）
struct CameraPojo {
    std::string channel;    // 通道号（字符串转int）
    std::string starttime;  // 开始时间 yyyy-MM-dd HH:mm:ss
    std::string endtime;    // 结束时间 yyyy-MM-dd HH:mm:ss
};

/**
 * @brief 获取指定时间内的视频列表（对齐Java版historyList接口）
 * @param pojo 摄像头参数（通道号、开始/结束时间）
 * @param login 已登录的设备句柄对象
 * @return json 录像文件列表JSON
 */
json historyList(const CameraPojo& pojo, const HCLoginSDK& login) {
    json resultJson; // 最终返回的JSON结果

    // 1. 校验登录状态
    if (login.getLUserID() < 0) {
        resultJson["message"] = "设备未登录，无法查询录像";
        return resultJson;
    }

    try {
        // 2. 转换时间字符串为SDK时间结构体
        NET_DVR_TIME lpStartTime = Utils::getNvrTime(pojo.starttime);
        NET_DVR_TIME lpStopTime = Utils::getNvrTime(pojo.endtime);

        // 3. 转换通道号
        int channel = std::stoi(pojo.channel);
        if (channel <= 0) {
            resultJson["message"] = "通道号必须为正整数";
            return resultJson;
        }

        // 4. 创建查找句柄（调用SDK查找文件接口）
        LONG lFindHandle = NET_DVR_FindFile(
            login.getLUserID(),        // 登录句柄
            channel,                   // 通道号
            0,                         // 文件类型：0=所有类型
            &lpStartTime,              // 开始时间
            &lpStopTime                // 结束时间
        );

        // 句柄创建失败处理
        if (lFindHandle < 0) {
            int errorCode = NET_DVR_GetLastError();
            std::cerr << "hcsdk 按时间查找录像文件失败,错误码:" << errorCode << std::endl;
            resultJson["message"] = "按时间查找录像文件失败 错误码:" + std::to_string(errorCode);
            NET_DVR_FindClose(lFindHandle); // 释放无效句柄
            return resultJson;
        }

        // 5. 循环遍历所有录像文件
        NET_DVR_FINDDATA_V40 lpFindData = {0}; // 单个文件信息结构体
        int videoIndex = 1;                    // 文件列表序号
        bool isFinding = true;

        while (isFinding) {
            // 逐个获取文件信息
            LONG lFindNextFile_V40 = NET_DVR_FindNextFile_V40(lFindHandle, &lpFindData);

            switch (lFindNextFile_V40) {
                case 1002: // 正在查找，请等待
                    std::cout << "hcsdk 正在查找文件，请等待..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 等待100ms重试
                    break;

                case 1000: { // 获取文件信息成功
                    json videoJson;
                    // 解析文件开始/结束时间
                    videoJson["starttime"] = Utils::sdkTimeToStr(lpFindData.struStartTime);
                    videoJson["endtime"] = Utils::sdkTimeToStr(lpFindData.struStopTime);
                    // 可选：补充文件名、文件大小等字段
                    videoJson["filename"] = std::string(lpFindData.sFileName);
                    videoJson["filesize"] = lpFindData.dwFileSize;

                    // 按序号存入结果（对齐Java版LinkedHashMap）
                    resultJson[std::to_string(videoIndex)] = videoJson;
                    videoIndex++;
                    break;
                }

                case 1003: // 没有更多文件，查找结束
                    std::cout << "hcsdk 没有更多的文件，查找结束" << std::endl;
                    isFinding = false;
                    break;

                default: // 其他错误状态
                    std::cerr << "hcsdk 查找文件异常，状态码:" << lFindNextFile_V40 << std::endl;
                    isFinding = false;
                    break;
            }
        }

        // 6. 释放查找句柄（必须执行，避免资源泄漏）
        if (lFindHandle >= 0) {
            NET_DVR_FindClose(lFindHandle);
            std::cout << "hcsdk 查找句柄已释放" << std::endl;
        }

    } catch (const std::invalid_argument& e) { // 时间格式错误
        resultJson["message"] = "时间格式错误：" + std::string(e.what());
        std::cerr << "时间格式错误：" << e.what() << std::endl;
    } catch (const std::exception& e) { // 其他异常
        resultJson["message"] = "查询失败：" + std::string(e.what());
        std::cerr << "查询失败：" << e.what() << std::endl;
    }

    return resultJson;
}

// 测试主函数
int main() {
    // 1. 初始化海康SDK
    if (!NET_DVR_Init()) {
        std::cerr << "海康SDK初始化失败，错误码：" << NET_DVR_GetLastError() << std::endl;
        return -1;
    }
    // 设置SDK日志（可选）
    NET_DVR_SetLogToFile(3, "./sdk_log/", true);

    // 2. 登录设备
    HCLoginSDK login;
    if (!login.login("10.9.255.21", 8000, "admin", "Wlkjaqxy411")) { // 替换为实际设备信息
        NET_DVR_Cleanup();
        return -1;
    }

    // 3. 构造查询参数
    CameraPojo pojo;
    pojo.channel = "33";                      // 通道号
    pojo.starttime = "2025-12-15 00:00:00";  // 开始时间
    pojo.endtime = "2025-12-15 23:59:59";    // 结束时间

    // 4. 查询历史录像列表
    json hisList = historyList(pojo, login);

    // 5. 打印结果（JSON格式化输出）
    std::cout << "查询结果：" << std::endl;
    std::cout << hisList.dump(4) << std::endl;

    // 6. 登出设备+清理SDK
    login.logout();
    NET_DVR_Cleanup();

    return 0;
}