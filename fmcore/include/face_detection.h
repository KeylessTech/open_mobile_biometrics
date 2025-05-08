#pragma once
#include <opencv2/core.hpp>
#include <vector>
#include <onnxruntime_cxx_api.h>
#include "utils.h"

bool init_face_detector(Ort::SessionOptions& options, const std::string& model_path, bool short_range);
std::vector<FaceDetectionResult> detect_faces(const cv::Mat& image);
