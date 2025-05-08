#include "face_detection.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <iostream>
#include <mutex>
#include <numeric>

namespace {

Ort::Env& getOrtEnv() {
    static Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "FaceDetection");
    return env;
}

std::unique_ptr<Ort::Session> face_session;
std::mutex session_mutex;

int input_width = 128;
int input_height = 128;
float threshold = 0.6f;
float iou_threshold = 0.3f;

cv::Mat preprocess_image(const cv::Mat& image, float& scale_out) {
    cv::Mat rgb;
    cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);

    int target_width = input_width;
    int target_height = input_height;

    // Calculate scale keeping aspect ratio
    float scale = std::min(
        static_cast<float>(target_width) / rgb.cols,
        static_cast<float>(target_height) / rgb.rows
    );
    scale_out = scale;

    int new_w = static_cast<int>(rgb.cols * scale);
    int new_h = static_cast<int>(rgb.rows * scale);

    cv::Mat resized;
    cv::resize(rgb, resized, cv::Size(new_w, new_h), 0, 0, cv::INTER_CUBIC);

    // Place resized image on black background
    cv::Mat padded = cv::Mat::zeros(target_height, target_width, CV_8UC3);
    resized.copyTo(padded(cv::Rect(0, 0, resized.cols, resized.rows)));

    // Convert to float32
    cv::Mat chw;
    padded.convertTo(padded, CV_32F); // Keep values in [0,255]

    std::vector<cv::Mat> channels(3);
    cv::split(padded, channels);

    std::vector<float> data(3 * target_width * target_height);
    for (int i = 0; i < 3; ++i) {
        std::memcpy(
            &data[i * target_width * target_height],
            channels[i].data,
            target_width * target_height * sizeof(float)
        );
    }

    return cv::Mat(1, 3 * target_width * target_height, CV_32F, data.data()).clone();
}


float IoU(const cv::Rect& a, const cv::Rect& b) {
    int x1 = std::max(a.x, b.x);
    int y1 = std::max(a.y, b.y);
    int x2 = std::min(a.x + a.width, b.x + b.width);
    int y2 = std::min(a.y + a.height, b.y + b.height);
    int interArea = std::max(0, x2 - x1) * std::max(0, y2 - y1);
    int unionArea = a.area() + b.area() - interArea;
    return unionArea > 0 ? static_cast<float>(interArea) / unionArea : 0.0f;
}

std::vector<int> non_max_suppression(const std::vector<cv::Rect>& boxes, const std::vector<float>& scores, float threshold_iou) {
    std::vector<int> indices(boxes.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](int i, int j) {
        return scores[i] > scores[j];
    });

    std::vector<int> keep;
    std::vector<bool> suppressed(boxes.size(), false);

    for (size_t _i = 0; _i < indices.size(); ++_i) {
        int i = indices[_i];
        if (suppressed[i]) continue;
        keep.push_back(i);
        for (size_t _j = _i + 1; _j < indices.size(); ++_j) {
            int j = indices[_j];
            if (IoU(boxes[i], boxes[j]) > threshold_iou) {
                suppressed[j] = true;
            }
        }
    }
    return keep;
}

} // namespace

bool init_face_detector(Ort::SessionOptions& options, const std::string& model_path, bool short_range) {
    try {
        std::lock_guard<std::mutex> lock(session_mutex);
        input_width = short_range ? 128 : 256;
        input_height = short_range ? 128 : 256;

        face_session = std::make_unique<Ort::Session>(getOrtEnv(), model_path.c_str(), options);
        std::cout << "[FaceDetector] Loaded model: " << model_path << std::endl;

//        Ort::AllocatorWithDefaultOptions allocator;
//        size_t count = face_session->GetOutputCount();
//        for (size_t i = 0; i < count; ++i) {
//            auto name = face_session->GetOutputNameAllocated(i, allocator);
//            std::cout << "[FaceDetector] Output[" << i << "] name: " << name.get() << std::endl;
//        }
        return true;
    } catch (const Ort::Exception& e) {
        std::cerr << "[FaceDetector] Failed to load model: " << e.what() << std::endl;
        return false;
    }
}

std::vector<FaceDetectionResult> detect_faces(const cv::Mat& image) {
    std::lock_guard<std::mutex> lock(session_mutex);
    if (!face_session) return {};

    float scale = 1.0f;
    cv::Mat input = preprocess_image(image, scale);

    std::vector<int64_t> input_dims = {1, 3, input_height, input_width};
    Ort::MemoryInfo mem_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        mem_info, input.ptr<float>(), input.total(), input_dims.data(), input_dims.size()
    );

    // Hold AllocatedStringPtrs so their memory stays valid
    Ort::AllocatorWithDefaultOptions allocator;

    std::vector<Ort::AllocatedStringPtr> input_name_holders;
    std::vector<const char*> input_names;
    input_name_holders.emplace_back(face_session->GetInputNameAllocated(0, allocator));
    input_names.push_back(input_name_holders[0].get());

    std::vector<Ort::AllocatedStringPtr> output_name_holders;
    std::vector<const char*> output_names;
    for (int i = 0; i < 3; ++i) {
        output_name_holders.emplace_back(face_session->GetOutputNameAllocated(i, allocator));
        output_names.push_back(output_name_holders[i].get());
    }

    // Run the model
    auto outputs = face_session->Run(
        Ort::RunOptions{nullptr},
        input_names.data(), &input_tensor, 1,
        output_names.data(), 3
    );

    
    auto shape_scores = outputs[0].GetTensorTypeAndShapeInfo().GetShape();
    auto shape_boxes = outputs[1].GetTensorTypeAndShapeInfo().GetShape();
    auto shape_landmarks = outputs[2].GetTensorTypeAndShapeInfo().GetShape();

//    std::cout << "[ONNX] scores shape: ";
//    for (auto d : shape_scores) std::cout << d << " ";
//    std::cout << "\n[ONNX] boxes shape: ";
//    for (auto d : shape_boxes) std::cout << d << " ";
//    std::cout << "\n[ONNX] landmarks shape: ";
//    for (auto d : shape_landmarks) std::cout << d << " ";
//    std::cout << std::endl;

    // 🔧 Extract output tensors
    float* scores    = outputs[0].GetTensorMutableData<float>();
    float* boxes     = outputs[1].GetTensorMutableData<float>();
    float* landmarks = outputs[2].GetTensorMutableData<float>();
    
//    std::cout << "[ONNX] First 10 scores: ";
//    for (int i = 0; i < 10; ++i) std::cout << scores[i] << " ";
//    std::cout << std::endl;

    std::vector<cv::Rect> raw_boxes;
    std::vector<std::vector<cv::Point2f>> raw_landmarks;
    std::vector<float> raw_scores;

    for (int i = 0; i < 896; ++i) {
        float score = scores[i];
        if (score < threshold) continue;
        
        float x1 = boxes[i * 4 + 0] / scale;
        float y1 = boxes[i * 4 + 1] / scale;
        float x2  = boxes[i * 4 + 2] / scale;
        float y2  = boxes[i * 4 + 3] / scale;

        
        raw_boxes.emplace_back(cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)));

        std::vector<cv::Point2f> lm;
        for (int j = 0; j < 6; ++j) {
            float lx = landmarks[i * 12 + j * 2 + 0] / scale;
            float ly = landmarks[i * 12 + j * 2 + 1] / scale;
            lm.emplace_back(cv::Point2f(lx, ly));
        }
        raw_landmarks.push_back(lm);
        raw_scores.push_back(score);
    }

    std::vector<int> keep = non_max_suppression(raw_boxes, raw_scores, iou_threshold);

    std::vector<FaceDetectionResult> results;
    for (int idx : keep) {
        results.push_back({raw_boxes[idx], raw_landmarks[idx], raw_scores[idx]});
    }

    // sort by score descending
    std::sort(results.begin(), results.end(), [](const FaceDetectionResult& a, const FaceDetectionResult& b) {
        return a.score > b.score;
    });
    
//    if (!results.empty()) {
//        const auto& top = results.front();
//        std::cout << "[DEBUG] Top score: " << top.score << "\n";
//        std::cout << "[DEBUG] Box: x=" << top.box.x << " y=" << top.box.y
//                  << " w=" << top.box.width << " h=" << top.box.height << "\n";
//        std::cout << "[DEBUG] Landmarks:\n";
//        for (const auto& pt : top.landmarks) {
//            std::cout << "  (" << pt.x << ", " << pt.y << ")\n";
//        }
//    }

    return results;
}
