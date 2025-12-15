#include "http_service.h"
#include "json.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <atomic>

using nlohmann::json;

// ===================== WebService 实现 =====================
// 修复：构造函数参数匹配头文件声明
WebService::WebService(const std::string httpPath, ICommandDispatcher* dispatcher)
    : m_bind_ip(httpPath),dispatcher_(dispatcher), m_server_fd(-1), m_running(false) {
    m_port = 8080;
    if (m_bind_ip == "localhost") {
        m_bind_ip = "127.0.0.1";
    }
}

WebService::~WebService() {
    stop();
}

bool WebService::start() {
    if (m_running) {
        std::cout << "WebService: Already running" << std::endl;
        return true;
    }

    // 1. 创建套接字（添加错误检查）
    m_server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_fd < 0) {
        std::cerr << "WebService: Failed to create server socket, errno=" << errno << std::endl;
        return false;
    }

    // 2. 设置套接字选项（添加错误检查）
    int opt = 1;
    if (setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        std::cerr << "WebService: Failed to set socket options, errno=" << errno << std::endl;
        ::close(m_server_fd);
        m_server_fd = -1;
        return false;
    }

    // 3. 绑定地址
    sockaddr_in addr{}; // 用{}初始化，替代memset更现代
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(m_port));


    // 解析绑定IP：如果是空/无效，默认用INADDR_ANY
    if (m_bind_ip.empty() || m_bind_ip == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;  // 监听所有网卡
    } else {
        // 将字符串IP转为网络字节序的整数
        if (inet_pton(AF_INET, m_bind_ip.c_str(), &addr.sin_addr.s_addr) <= 0) {
            std::cerr << "WebService: Invalid bind IP address: " << m_bind_ip << std::endl;
            ::close(m_server_fd);
            m_server_fd = -1;
            return false;
        }
    }

    if (bind(m_server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "WebService: Failed to bind socket, errno=" << errno << std::endl;
        ::close(m_server_fd);
        m_server_fd = -1;
        return false;
    }

    // 4. 开始监听（backlog建议用SOMAXCONN，或配置化）
    if (listen(m_server_fd, SOMAXCONN) < 0) {
        std::cerr << "WebService: Failed to listen, errno=" << errno << std::endl;
        ::close(m_server_fd);
        m_server_fd = -1;
        return false;
    }

    // 5. 启动服务线程
    m_running = true;
    m_thread = std::thread(&WebService::run, this);
    std::cout << "WebService: Started successfully, listening on port " << m_port << std::endl;
    return true;
}

void WebService::stop() {
    if (!m_running) return;

    // 标记停止状态（原子操作，避免线程竞争）
    m_running = false;

    // 关闭监听套接字，唤醒accept阻塞
    if (m_server_fd >= 0) {
        ::shutdown(m_server_fd, SHUT_RDWR);
        ::close(m_server_fd);
        m_server_fd = -1;
    }

    // 等待服务线程退出
    if (m_thread.joinable()) {
        m_thread.join();
    }

    std::cout << "WebService: Stopped successfully" << std::endl;
}

void WebService::run() {
    while (m_running) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        // 接受客户端连接（非阻塞优化可选，这里保持简单）
        int client_fd = accept(m_server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd < 0) {
            // 仅在服务运行中时打印错误
            if (m_running) {
                std::cerr << "WebService: Accept failed, errno=" << errno << std::endl;
            }
            continue; // 而非break，避免单次accept失败导致服务退出
        }

        // 打印客户端信息（调试用）
        std::cout << "WebService: New client connected: " 
                  << inet_ntoa(client_addr.sin_addr) << ":" 
                  << ntohs(client_addr.sin_port) << std::endl;

        // 处理客户端连接（分离线程，注意资源泄漏风险，可考虑线程池）
        std::thread client_thread(&WebService::handleClient, this, client_fd);
        client_thread.detach();
    }
}

void WebService::handleClient(int client_fd) {
    constexpr size_t buf_size = 8192;
    char buffer[buf_size];
    std::string request;
    request.reserve(buf_size);

    // 1. 读取HTTP请求头+体（循环读取，避免单次读取不完整）
    ssize_t recv_len = 0;
    while (m_running) {
        recv_len = recv(client_fd, buffer, buf_size - 1, 0);
        if (recv_len <= 0) break;

        buffer[recv_len] = '\0';
        request.append(buffer, static_cast<size_t>(recv_len));

        // 检测请求结束（HTTP请求以\r\n\r\n为分隔符）
        if (request.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }

    // 空请求直接关闭
    if (request.empty()) {
        ::close(client_fd);
        return;
    }

    // 2. 解析HTTP方法、路径
    std::string method, path, protocol;
    std::istringstream req_stream(request);
    req_stream >> method >> path >> protocol;

    // 仅处理POST请求（根据业务需求扩展）
    if (method != "POST") {
        std::string error_resp = R"(HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\nConnection: close\r\n\r\n)";
        send(client_fd, error_resp.c_str(), error_resp.size(), 0);
        ::close(client_fd);
        return;
    }

    // 3. 解析Content-Length
    size_t content_length = 0;
    size_t cl_pos = request.find("Content-Length:");
    if (cl_pos != std::string::npos) {
        cl_pos += strlen("Content-Length:");
        size_t cl_end = request.find('\r', cl_pos);
        if (cl_end != std::string::npos) {
            try {
                content_length = std::stoul(request.substr(cl_pos, cl_end - cl_pos));
            } catch (const std::exception& e) {
                std::cerr << "WebService: Invalid Content-Length, " << e.what() << std::endl;
                content_length = 0;
            }
        }
    }

    // 4. 提取请求体
    std::string request_body;
    size_t body_sep_pos = request.find("\r\n\r\n");
    if (body_sep_pos != std::string::npos) {
        request_body = request.substr(body_sep_pos + 4);
        // 补充读取剩余的请求体（如果Content-Length大于已读取的长度）
        while (request_body.size() < content_length && m_running) {
            recv_len = recv(client_fd, buffer, buf_size - 1, 0);
            if (recv_len <= 0) break;
            request_body.append(buffer, static_cast<size_t>(recv_len));
        }
    }

    // 5. 业务逻辑处理
    json response_json;
    try {
        if (dispatcher_) {
            // 调用业务调度器处理请求（path作为topic，request_body作为payload）
            dispatcher_->onMessage(path, request_body);
            response_json["success"] = true;
            response_json["message"] = "Request processed successfully";
        } else {
            throw std::runtime_error("No command dispatcher available");
        }
    } catch (const std::exception& e) {
        std::cerr << "WebService: Failed to process request, " << e.what() << std::endl;
        response_json["success"] = false;
        response_json["error"] = e.what();
    }

    // 6. 构造HTTP响应
    std::string response_str = response_json.dump();
    std::ostringstream resp_stream;
    resp_stream << "HTTP/1.1 200 OK\r\n";
    resp_stream << "Content-Type: application/json; charset=utf-8\r\n";
    resp_stream << "Content-Length: " << response_str.size() << "\r\n";
    resp_stream << "Connection: close\r\n\r\n";
    resp_stream << response_str;

    std::string http_resp = resp_stream.str();
    // 发送响应（循环发送，确保全部发送完成）
    ssize_t sent_len = 0;
    while (sent_len < static_cast<ssize_t>(http_resp.size())) {
        ssize_t ret = send(client_fd, http_resp.c_str() + sent_len, http_resp.size() - sent_len, 0);
        if (ret < 0) {
            std::cerr << "WebService: Failed to send response, errno=" << errno << std::endl;
            break;
        }
        sent_len += ret;
    }

    // 7. 关闭客户端套接字（释放资源）
    ::shutdown(client_fd, SHUT_RDWR);
    ::close(client_fd);
}

