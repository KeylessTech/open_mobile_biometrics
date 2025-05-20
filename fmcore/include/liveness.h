#pragma once

#include <opencv2/core.hpp>
#include <onnxruntime_cxx_api.h>
#include "utils.h"

struct LivenessResult {
    bool isLive = false;
    float score = -1.0;
};

bool init_liveness_detector(Ort::SessionOptions& session_options, const std::vector<std::string>& model_paths, const float liveness_thresh);
LivenessResult run_liveness_check(const cv::Mat& input_image, const FaceDetectionResult& face);
void release_liveness_detector();
