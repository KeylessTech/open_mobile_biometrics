#pragma once
#include <opencv2/core.hpp>
#ifdef FMCORE_NATIVE_BUILD
    #include <opencv2/highgui.hpp>
#endif
#include <vector>

struct FaceDetectionResult {
    cv::Rect box;
    std::vector<cv::Point2f> landmarks;
    float score;
};

void draw_detections(const cv::Mat& image, const std::vector<FaceDetectionResult>& detections);
void debug_aligned_faces(const cv::Mat& image, const FaceDetectionResult& face, const cv::Mat& alignedFace);
void normalize_to_11(const cv::Mat& src, cv::Mat& normalized);
std::string joinPath(const std::string& base, const std::string& filename);
