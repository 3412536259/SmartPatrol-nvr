#ifndef AI_DETECTOR_SERVICE_H
#define AI_DETECTOR_SERVICE_H

#include <memory>
#include <vector>
#include <string>

struct DetectionResult {
    std::string label;
    float confidence;
    int x, y, width, height;
};

class IAIDetectorService {
public:
    virtual ~IAIDetectorService() = default;
    virtual bool initialize() = 0;
    virtual bool loadModel(const std::string& model_path) = 0;
    virtual std::vector<DetectionResult> detect(const std::vector<uint8_t>& image_data,
                                               int width, int height) = 0;
    virtual void setConfidenceThreshold(float threshold) = 0;
    virtual float getConfidenceThreshold() const = 0;
    virtual bool isModelLoaded() const = 0;
    virtual void setTargetClasses(const std::vector<std::string>& classes) = 0;
};

class AIDetectorService : public IAIDetectorService {
public:
    AIDetectorService();
    ~AIDetectorService();
    
    bool initialize() override;
    bool loadModel(const std::string& model_path) override;
    std::vector<DetectionResult> detect(const std::vector<uint8_t>& image_data,
                                       int width, int height) override;
    void setConfidenceThreshold(float threshold) override;
    float getConfidenceThreshold() const override;
    bool isModelLoaded() const override;
    void setTargetClasses(const std::vector<std::string>& classes) override;
    
private:
    float confidence_threshold_ = 0.6f;
    std::vector<std::string> target_classes_;
    bool model_loaded_ = false;
};

#endif