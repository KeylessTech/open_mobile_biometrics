#pragma once
#include <opencv2/core.hpp>
#include "utils.h"

cv::Mat align_face(const cv::Mat& image, const FaceDetectionResult& faceDetected);
