#pragma once 
#include <string>



class HttpFileUploader {
public:
    bool uploadFile(const std::string& url,
                    const std::string& filePath,
                    const std::string& taskId);
};
