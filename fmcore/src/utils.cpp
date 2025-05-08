#include <opencv2/imgproc.hpp>
#include <iostream>
#include "utils.h"

#ifdef FMCORE_NATIVE_BUILD
    void draw_detections(const cv::Mat& image, const std::vector<FaceDetectionResult>& detections) {
        cv::Mat debug_img = image.clone();

        for (const auto& det : detections) {
            // Draw bounding box
            cv::rectangle(debug_img, det.box, cv::Scalar(0, 255, 0), 2);

            // Draw landmarks
            for (const auto& lm : det.landmarks) {
                cv::circle(debug_img, cv::Point(cvRound(lm.x), cvRound(lm.y)), 3, cv::Scalar(0, 0, 255), -1);
            }
            
            std::string score_text = cv::format("%.2f", det.score);
            cv::putText(debug_img, score_text, det.box.tl() + cv::Point(0, -5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 0), 1);

        }

        cv::imshow("Detections", debug_img);
        cv::waitKey(0);
    }

    void debug_aligned_faces(const cv::Mat& image, const FaceDetectionResult& face, const cv::Mat& alignedFace) {
        cv::Mat debugImage = image.clone();

        cv::rectangle(debugImage, face.box, cv::Scalar(0, 255, 0), 2);

        for (const auto& lm : face.landmarks) {
            cv::circle(debugImage, cv::Point(cvRound(lm.x), cvRound(lm.y)), 3, cv::Scalar(0, 0, 255), -1);
        }
        
        std::cout << "original image.rows: " << image.rows << " - image.cols: " << image.cols << std::endl;
        std::cout << "alignedFace.rows: " << alignedFace.rows << " - alignedFace.cols: " << alignedFace.cols << std::endl;

        cv::imshow("Aligned face", alignedFace);
        cv::imshow("Detected Faces", debugImage);
        cv::waitKey(0);
    }
#endif

void normalize_to_11(const cv::Mat& src, cv::Mat& normalized){
    CV_Assert(src.type() == CV_8UC3);
    src.convertTo(normalized, CV_32FC3, 1.0 / 255.0);
    normalized = (normalized - 0.5f) / 0.5f;
}

std::string joinPath(const std::string& base, const std::string& filename) {
    if (base.empty()) return filename;
    if (base.back() == '/' || base.back() == '\\') return base + filename;
    return base + "/" + filename;
}
