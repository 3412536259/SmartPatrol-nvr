#include "common/logger.h"
#include "common/config.h"
#include "presentation_layer/sensor_monitor.h"
#include <iostream>
#include <csignal>
#include <atomic>

std::atomic<bool> running{true};

void signal_handler(int signal) {
    Logger::getInstance().info("Received signal, shutting down...");
    running = false;
}

int main(int argc, char* argv[]) {
    // 设置信号处理
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    // 初始化配置
    auto& config = Config::getInstance();
    if (!config.load()) {
        std::cerr << "Failed to load configuration file" << std::endl;
        return -1;
    }
    
    // 初始化日志
    auto& logger = Logger::getInstance();
    logger.info("RK3588 Service starting...");
    
    try {
        // 创建传感器监控
        auto sensor_monitor = std::make_unique<SensorMonitor>();
        
        // 初始化系统
        if (!sensor_monitor->initialize()) {
            logger.error("Failed to initialize sensor monitor");
            return -1;
        }
        
        // 启动系统
        sensor_monitor->start();
        logger.info("RK3588 Service started successfully");
        
        // 主循环
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // 停止系统
        sensor_monitor->stop();
        logger.info("RK3588 Service stopped");
        
    } catch (const std::exception& e) {
        logger.error(std::string("Exception in main: ") + e.what());
        return -1;
    }
    
    return 0;
}