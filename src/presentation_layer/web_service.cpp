#include "http_service.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <atomic>
#include <algorithm>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <sys/time.h>

using nlohmann::json;

WebService::WebService(const std::string& httpPath, ICommandDispatcher* dispatcher)
    : m_bind_ip("0.0.0.0"), dispatcher_(dispatcher), m_server_fd(-1), m_running(false), m_port(8080) {
    size_t colon_pos = httpPath.find(':');
    if (colon_pos != std::string::npos) {
        m_bind_ip = httpPath.substr(0, colon_pos);
        if (m_bind_ip.empty()) m_bind_ip = "0.0.0.0";
        try {
            m_port = std::stoul(httpPath.substr(colon_pos + 1));
        } catch (...) {
            m_port = 8080;
        }
    } else if (!httpPath.empty()) {
        m_bind_ip = httpPath;
    }

    if (m_bind_ip == "localhost") {
        m_bind_ip = "127.0.0.1";
    }

    std::cout << "WebService: Config - bind_ip=" << m_bind_ip << ", port=" << m_port << std::endl;
}

WebService::~WebService() {
    stop();
}

bool WebService::start() {
    if (m_running) {
        std::cout << "WebService: Already running" << std::endl;
        return true;
    }

    m_server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_fd < 0) {
        std::cerr << "WebService: Failed to create server socket, errno=" << errno << std::endl;
        return false;
    }

    int opt = 1;
    if (setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        std::cerr << "WebService: Failed to set socket options, errno=" << errno << std::endl;
        ::close(m_server_fd);
        m_server_fd = -1;
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(m_port));

    if (m_bind_ip.empty() || m_bind_ip == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
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

    if (listen(m_server_fd, SOMAXCONN) < 0) {
        std::cerr << "WebService: Failed to listen, errno=" << errno << std::endl;
        ::close(m_server_fd);
        m_server_fd = -1;
        return false;
    }

    m_running = true;
    m_thread = std::thread(&WebService::run, this);
    std::cout << "WebService: Started successfully, listening on " << m_bind_ip << ":" << m_port << std::endl;
    return true;
}

void WebService::stop() {
    if (!m_running) return;

    m_running = false;

    if (m_server_fd >= 0) {
        ::shutdown(m_server_fd, SHUT_RDWR);
        ::close(m_server_fd);
        m_server_fd = -1;
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }

    std::cout << "WebService: Stopped successfully" << std::endl;
}

void WebService::run() {
    while (m_running) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(m_server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd < 0) {
            if (m_running) {
                std::cerr << "WebService: Accept failed, errno=" << errno << std::endl;
            }
            continue;
        }

        char client_ip[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        uint16_t client_port = ntohs(client_addr.sin_port);
        std::cout << "\n===== New Client Connected =====" << std::endl;
        std::cout << "Client IP: " << client_ip << ", Port: " << client_port << std::endl;

        // 修复1：unique_ptr初始化错误 - 改为值拷贝+指针转换
        std::thread client_thread([this, client_fd, client_ip, client_port]() {
            // 用局部变量存储fd，避免const指针问题
            int local_fd = client_fd;
            // unique_ptr管理局部fd的生命周期
            std::unique_ptr<int, std::function<void(int*)>> guard(&local_fd, [](int* fd) {
                if (fd != nullptr && *fd >= 0) {
                    ::shutdown(*fd, SHUT_RDWR);
                    ::close(*fd);
                    *fd = -1;
                }
            });
            handleClient(local_fd, client_ip, client_port);
        });
        client_thread.detach();
    }
}

void WebService::handleClient(int client_fd, const char* client_ip, uint16_t client_port) {
    constexpr size_t buf_size = 8192;
    char buffer[buf_size];
    std::string request;
    request.reserve(buf_size * 2);

    // 设置5秒接收超时
    struct timeval timeout {5, 0};
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // 完整读取请求
    ssize_t recv_len = 0;
    while (m_running) {
        recv_len = recv(client_fd, buffer, buf_size - 1, 0);
        if (recv_len <= 0) {
            if (recv_len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::cerr << "WebService: Recv timeout from " << client_ip << ":" << client_port << std::endl;
                } else {
                    std::cerr << "WebService: Recv failed from " << client_ip << ":" << client_port 
                              << ", errno=" << errno << std::endl;
                }
            }
            break;
        }

        buffer[recv_len] = '\0';
        request.append(buffer, static_cast<size_t>(recv_len));

        // 检测请求结束
        if (request.find("\r\n\r\n") != std::string::npos) {
            if (request.find("Content-Length:") == std::string::npos) {
                break;
            }
            size_t content_length = parseContentLength(request);
            size_t body_sep_pos = request.find("\r\n\r\n") + 4;
            if (request.size() - body_sep_pos >= content_length) {
                break;
            }
        }
    }

    if (request.empty()) {
        std::cerr << "WebService: Empty request from " << client_ip << ":" << client_port << std::endl;
        // 修复2：sendErrorResponse参数匹配（使用默认empty json）
        sendErrorResponse(client_fd, 400, "Empty request");
        return;
    }

    std::cout << "===== Received Request from " << client_ip << ":" << client_port << " =====" << std::endl;
    std::cout << "Raw Request:\n" << request << "\n==============================" << std::endl;

    // 解析HTTP方法/路径/协议
    std::string method, path, protocol;
    std::istringstream req_stream(request);
    req_stream >> method >> path >> protocol;

    std::cout << "Parsed - Method: " << method << ", Path: " << path << ", Protocol: " << protocol << std::endl;

    // 仅处理POST
    if (method != "POST") {
        std::cerr << "WebService: Unsupported method " << method << " from " << client_ip << ":" << client_port << std::endl;
        sendErrorResponse(client_fd, 405, "Method Not Allowed");
        return;
    }

    // 解析Content-Length和请求体
    size_t content_length = parseContentLength(request);
    // 修复3：parseRequestBody新增client_fd参数，避免非法指针转换
    std::string request_body = parseRequestBody(request, content_length, client_fd);

    std::cout << "Content-Length: " << content_length << ", Request Body: " << request_body << std::endl;

    // 校验JSON合法性
    if (!request_body.empty()) {
        try {
            // 修复4：接收json::parse返回值，消除未使用警告
            json req_json = json::parse(request_body);
        } catch (const json::parse_error& e) {
            std::cerr << "WebService: Invalid JSON from " << client_ip << ":" << client_port 
                      << ", error: " << e.what() << std::endl;
            sendErrorResponse(client_fd, 400, "Invalid JSON format");
            return;
        }
    }

    // 标准化路径
    std::string topic = path;
    if (!topic.empty() && topic[0] == '/') {
        topic = topic.substr(1);
    }
    if (topic.empty()) {
        std::cerr << "WebService: Empty topic from " << client_ip << ":" << client_port << std::endl;
        sendErrorResponse(client_fd, 400, "Empty request path");
        return;
    }
    std::cout << "Normalized Topic: " << topic << std::endl;

    // 业务处理
    json response_json;
    try {
        if (dispatcher_) {
            std::cout << "WebService: Dispatching request to topic: " << topic << std::endl;
            dispatcher_->onMessage(topic, request_body);
            response_json["success"] = true;
            response_json["message"] = "Request processed successfully";
            response_json["topic"] = topic;
        } else {
            throw std::runtime_error("No command dispatcher available");
        }
    } catch (const std::exception& e) {
        std::cerr << "WebService: Failed to process request from " << client_ip << ":" << client_port 
                  << ", error: " << e.what() << std::endl;
        response_json["success"] = false;
        response_json["error"] = e.what();
        response_json["topic"] = topic;
        sendErrorResponse(client_fd, 500, e.what(), response_json);
        return;
    }

    // 发送成功响应
    sendSuccessResponse(client_fd, response_json);
    std::cout << "WebService: Client " << client_ip << ":" << client_port << " request processed" << std::endl;
}

size_t WebService::parseContentLength(const std::string& request) {
    size_t cl_pos = request.find("Content-Length:");
    if (cl_pos == std::string::npos) {
        return 0;
    }

    cl_pos += strlen("Content-Length:");
    while (cl_pos < request.size() && (request[cl_pos] == ' ' || request[cl_pos] == '\t')) {
        cl_pos++;
    }

    size_t cl_end = request.find('\r', cl_pos);
    if (cl_end == std::string::npos) {
        return 0;
    }

    try {
        return std::stoul(request.substr(cl_pos, cl_end - cl_pos));
    } catch (...) {
        return 0;
    }
}

// 修复5：新增client_fd参数，移除非法的指针转换
std::string WebService::parseRequestBody(const std::string& request, size_t content_length, int client_fd) {
    std::string request_body;
    size_t body_sep_pos = request.find("\r\n\r\n");
    if (body_sep_pos == std::string::npos) {
        return request_body;
    }

    body_sep_pos += 4;
    request_body = request.substr(body_sep_pos);

    // 补全请求体
    if (request_body.size() < content_length) {
        constexpr size_t buf_size = 8192;
        char buffer[buf_size];
        while (request_body.size() < content_length && m_running) {
            ssize_t recv_len = recv(client_fd, buffer, std::min(buf_size - 1, content_length - request_body.size()), 0);
            if (recv_len <= 0) {
                std::cerr << "WebService: Incomplete request body, expected " << content_length 
                          << " bytes, got " << request_body.size() << " bytes" << std::endl;
                break;
            }
            request_body.append(buffer, static_cast<size_t>(recv_len));
        }
    }

    // 截断超出部分
    if (request_body.size() > content_length) {
        request_body = request_body.substr(0, content_length);
    }

    return request_body;
}

// 修复6：添加默认参数，兼容3/4参数调用
void WebService::sendErrorResponse(int client_fd, int status_code, const std::string& message, const json& extra) {
    json response_json;
    response_json["success"] = false;
    response_json["error"] = message;
    if (!extra.empty()) {
        response_json.merge_patch(extra);
    }

    std::string response_str = response_json.dump(2);
    std::ostringstream resp_stream;
    resp_stream << "HTTP/1.1 " << status_code << " " << getStatusText(status_code) << "\r\n";
    resp_stream << "Content-Type: application/json; charset=utf-8\r\n";
    resp_stream << "Content-Length: " << response_str.size() << "\r\n";
    resp_stream << "Connection: close\r\n\r\n";
    resp_stream << response_str;

    std::string http_resp = resp_stream.str();
    send(client_fd, http_resp.c_str(), http_resp.size(), 0);
}

void WebService::sendSuccessResponse(int client_fd, const json& response_json) {
    std::string response_str = response_json.dump(2);
    std::ostringstream resp_stream;
    resp_stream << "HTTP/1.1 200 OK\r\n";
    resp_stream << "Content-Type: application/json; charset=utf-8\r\n";
    resp_stream << "Content-Length: " << response_str.size() << "\r\n";
    resp_stream << "Connection: close\r\n\r\n";
    resp_stream << response_str;

    std::string http_resp = resp_stream.str();
    ssize_t sent_len = 0;
    while (sent_len < static_cast<ssize_t>(http_resp.size())) {
        ssize_t ret = send(client_fd, http_resp.c_str() + sent_len, http_resp.size() - sent_len, 0);
        if (ret < 0) {
            std::cerr << "WebService: Failed to send response, errno=" << errno << std::endl;
            break;
        }
        sent_len += ret;
    }
}

std::string WebService::getStatusText(int status_code) {
    switch (status_code) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        default: return "Unknown Status";
    }
}