#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <unordered_map>

class Config {
public:
    static Config& getInstance();
    
    bool load(const std::string& filename = "config/config.ini");
    void set(const std::string& key, const std::string& value);
    
    std::string getString(const std::string& key, const std::string& default_val = "");
    int getInt(const std::string& key, int default_val = 0);
    float getFloat(const std::string& key, float default_val = 0.0f);
    bool getBool(const std::string& key, bool default_val = false);
    
private:
    Config() = default;
    ~Config() = default;
    
    std::unordered_map<std::string, std::string> config_map_;
    std::string config_file_;
};

#endif