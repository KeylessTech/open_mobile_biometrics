#pragma once
#include <string>
#include <vector>
#include <opencv2/core.hpp>

enum class PipelineMode {
    OnlyLiveness = 0,
    SkipLiveness = 1,
    WholePipeline = 2
};

struct ProcessResult {
    bool livenessChecked = false;
    bool isLive = false;
    bool faceDetected = false;
    bool embeddingExtracted = false;
    float livenessScore = -1.0;
    std::vector<float> embedding;
};

class FMCore {
public:
    bool init(const std::string& configJson, const std::string& modelBasePath);
    ProcessResult process(const std::string& imagePath, PipelineMode mode);
    bool match(const std::vector<float>& embedding1, const std::vector<float>& embedding2);
    void reset();
};




// Debug
void setDebugSavePath(const std::string& path);
void saveDebugImage(const cv::Mat& img, const std::string& filename);
