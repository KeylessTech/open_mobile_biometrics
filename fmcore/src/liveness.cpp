// liveness.cpp
#include "liveness.h"
#include <iostream>
#include <opencv2/imgproc.hpp>

static std::vector<std::unique_ptr<Ort::Session>> liveness_sessions;
std::mutex liveness_mutex;
static std::vector<std::string> liveness_input_names;
static float liveness_thresh;
static const int MODEL_INPUT_SIZE = 80;
static float scales[] = {4.0, 2.7};

Ort::Env& getOrtEnv() {
    static Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "Liveness");
    return env;
}

bool init_liveness_detector(Ort::SessionOptions& session_options, const std::vector<std::string>& model_paths, const float liveness_threshold) {
    try {
        std::lock_guard<std::mutex> lock(liveness_mutex);
        liveness_sessions.clear();
        for (const auto& model_path : model_paths) {
            auto session = std::make_unique<Ort::Session>(getOrtEnv(), model_path.c_str(), session_options);
            liveness_sessions.push_back(std::move(session));
        }

        Ort::AllocatorWithDefaultOptions allocator;
        Ort::AllocatedStringPtr input_name0 = liveness_sessions[0]->GetInputNameAllocated(0, allocator);
        Ort::AllocatedStringPtr input_name1 = liveness_sessions[1]->GetInputNameAllocated(0, allocator);
        liveness_input_names.push_back(input_name0.get());
        liveness_input_names.push_back(input_name1.get());
        liveness_thresh = liveness_threshold;

        std::cout << "[Liveness] Loaded " << liveness_sessions.size() << " model(s)." << std::endl;
        return true;
    } catch (const Ort::Exception& e) {
        std::cerr << "[Liveness] Failed to load ONNX models: " << e.what() << std::endl;
        return false;
    }
}

cv::Mat preprocess_liveness_crop(const cv::Mat& input_image, const FaceDetectionResult& face, int target_size = 80, float scale = 4.0f) {
    cv::Mat rgb;
    cv::cvtColor(input_image, rgb, cv::COLOR_BGR2RGB);

    int src_w = rgb.cols;
    int src_h = rgb.rows;

    int x = face.box.x;
    int y = face.box.y;
    int box_w = face.box.width;
    int box_h = face.box.height;

    // Ensure valid bbox
    if (box_w <= 0 || box_h <= 0) {
        std::cerr << "[Liveness] Invalid face bounding box." << std::endl;
        return {};
    }

    // Calculate center and scaled box size
    scale = std::min((float)(src_h - 1) / box_h, std::min((float)(src_w - 1) / box_w, scale));
    float new_width = box_w * scale;
    float new_height = box_h * scale;
    float center_x = x + box_w / 2.0f;
    float center_y = y + box_h / 2.0f;

    float left_top_x = center_x - new_width / 2.0f;
    float left_top_y = center_y - new_height / 2.0f;
    float right_bottom_x = center_x + new_width / 2.0f;
    float right_bottom_y = center_y + new_height / 2.0f;

    // Adjust to stay within image bounds
    if (left_top_x < 0) {
        right_bottom_x -= left_top_x;
        left_top_x = 0;
    }
    if (left_top_y < 0) {
        right_bottom_y -= left_top_y;
        left_top_y = 0;
    }
    if (right_bottom_x > src_w - 1) {
        left_top_x -= (right_bottom_x - (src_w - 1));
        right_bottom_x = src_w - 1;
    }
    if (right_bottom_y > src_h - 1) {
        left_top_y -= (right_bottom_y - (src_h - 1));
        right_bottom_y = src_h - 1;
    }

    // Final crop and resize
    int x1 = std::max(0, (int)left_top_x);
    int y1 = std::max(0, (int)left_top_y);
    int x2 = std::min(src_w, (int)right_bottom_x);
    int y2 = std::min(src_h, (int)right_bottom_y);

    if ((x2 - x1) <= 0 || (y2 - y1) <= 0) {
        std::cerr << "[Liveness] Invalid crop region after adjustment." << std::endl;
        return {};
    }

    cv::Mat cropped = rgb(cv::Rect(x1, y1, x2 - x1, y2 - y1)).clone();
    cv::Mat resized;
    cv::resize(cropped, resized, cv::Size(target_size, target_size));
    
    return resized;
}



LivenessResult run_liveness_check(const cv::Mat& input_image, const FaceDetectionResult& face) {
    LivenessResult lr;
    
    if (liveness_sessions.empty()) {
        std::cerr << "[Liveness] ERROR: no valid sessions" << std::endl;
        return lr;
    }
    
    std::vector<float> probs_sum(3, 0.0f);
    
    int session_count = 0;
    for (const auto& session : liveness_sessions) {
        
        // Preprocess face crop with bounding box
        cv::Mat padded = preprocess_liveness_crop(input_image, face, MODEL_INPUT_SIZE, scales[session_count]);
        if (padded.empty()) {
            std::cerr << "[Liveness] Preprocessing failed, skipping liveness check." << std::endl;
            return lr;
        }
        
//            cv::imshow("input_image", input_image);
//            cv::imshow("padded" + std::to_string(session_count), padded);
//            cv::waitKey();

        // Convert to CHW float32 [1,3,80,80]
        padded.convertTo(padded, CV_32F);
        std::vector<cv::Mat> channels(3);
        cv::split(padded, channels);
        std::vector<float> input_tensor;
        for (int i = 0; i < 3; ++i) {
            input_tensor.insert(input_tensor.end(),
                (float*)channels[i].datastart,
                (float*)channels[i].dataend
            );
        }
        
        // Create tensor
        Ort::MemoryInfo mem_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<int64_t> input_shape = {1, 3, MODEL_INPUT_SIZE, MODEL_INPUT_SIZE};
        Ort::Value input_tensor_ort = Ort::Value::CreateTensor<float>(
                                                                      mem_info, input_tensor.data(), input_tensor.size(), input_shape.data(), input_shape.size()
                                                                      );
        
        // Run inference
        std::vector<const char*> input_names = {liveness_input_names.at(session_count).c_str()};
        Ort::AllocatorWithDefaultOptions allocator;
        Ort::AllocatedStringPtr output_name = session->GetOutputNameAllocated(0, allocator);
        std::vector<const char*> output_names = {output_name.get()};
        
        auto output_tensors = session->Run(Ort::RunOptions{nullptr}, input_names.data(), &input_tensor_ort, 1, output_names.data(), 1);
        float* output_data = output_tensors[0].GetTensorMutableData<float>();
        
        // Apply softmax per model
        std::vector<float> exp_logits(3);
        float sum_exp = 0.0f;
        for (int i = 0; i < 3; ++i) {
            exp_logits[i] = std::exp(output_data[i]);
            sum_exp += exp_logits[i];
        }
        for (int i = 0; i < 3; ++i) {
            probs_sum[i] += exp_logits[i] / sum_exp;
        }
        session_count++;
    }
    
    
    float real_score = probs_sum[1] / 2.0f;
    lr.score = real_score;
    lr.isLive = real_score > liveness_thresh;
    
    std::cout << "[Liveness] Score: " << real_score << std::endl;
    
    return lr;
}
