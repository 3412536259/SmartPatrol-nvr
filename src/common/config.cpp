#include "common/config.h"
#include "third_party/inih/ini.h"
#include <iostream>
#include <fstream>

static int ini_handler(void* user, const char* section, const char* name, const char* value) {
    auto* config = static_cast<std::unordered_map<std::string, std::string>*>(user);
    std::string key = std::string(section) + "." + name;
    (*config)[key] = value;
    return 1;
}

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

bool Config::load(const std::string& filename) {
    config_file_ = filename;
    return ini_parse(filename.c_str(), ini_handler, &config_map_) == 0;
}

void Config::set(const std::string& key, const std::string& value) {
    config_map_[key] = value;
}

std::string Config::getString(const std::string& key, const std::string& default_val) {
    auto it = config_map_.find(key);
    return (it != config_map_.end()) ? it->second : default_val;
}

int Config::getInt(const std::string& key, int default_val) {
    auto it = config_map_.find(key);
    return (it != config_map_.end()) ? std::stoi(it->second) : default_val;
}

float Config::getFloat(const std::string& key, float default_val) {
    auto it = config_map_.find(key);
    return (it != config_map_.end()) ? std::stof(it->second) : default_val;
}

bool Config::getBool(const std::string& key, bool default_val) {
    auto it = config_map_.find(key);
    if (it != config_map_.end()) {
        std::string val = it->second;
        return (val == "true" || val == "1" || val == "yes");
    }
    return default_val;
}