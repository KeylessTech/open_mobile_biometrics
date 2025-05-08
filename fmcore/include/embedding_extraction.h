#pragma once
#include <opencv2/core.hpp>
#include <vector>
#include <string>
#include <onnxruntime_cxx_api.h>

bool init_embedding_extractor(Ort::SessionOptions& options, const std::string& model_path);
std::vector<float> extract_embedding(const cv::Mat& aligned_face);
