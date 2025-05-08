// liveness.cpp
#include "liveness.h"
#include <iostream>
#include <opencv2/imgproc.hpp>

static std::unique_ptr<Ort::Session> liveness_session;
std::mutex liveness_mutex;
static std::string liveness_input_name;
static float liveness_thresh;
static const int MODEL_INPUT_SIZE = 128;

Ort::Env& getOrtEnv() {
    static Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "Liveness");
    return env;
}

bool init_liveness_detector(Ort::SessionOptions& session_options, const std::string& model_path, const float liveness_threshold) {
    try {
        std::lock_guard<std::mutex> lock(liveness_mutex);
        liveness_session = std::make_unique<Ort::Session>(getOrtEnv(), model_path.c_str(), session_options);
        Ort::AllocatorWithDefaultOptions allocator;
        Ort::AllocatedStringPtr input_name = liveness_session->GetInputNameAllocated(0, allocator);
        liveness_input_name = input_name.get();
        liveness_thresh = liveness_threshold;
        
        std::cout << "[Liveness] Loaded model: " << model_path << std::endl;
        
        return true;
    } catch (const Ort::Exception& e) {
        std::cerr << "[Liveness] Failed to load ONNX model: " << e.what() << std::endl;
        return false;
    }
}

cv::Mat preprocess_liveness_crop(const cv::Mat& input_image, const FaceDetectionResult& face, int target_size = 128, float bbox_inc = 1.5f) {
    
    cv::Mat rgb;
    cv::cvtColor(input_image, rgb, cv::COLOR_BGR2RGB);
    
    int real_w = rgb.cols;
    int real_h = rgb.rows;

    int x = face.box.x;
    int y = face.box.y;
    int w = face.box.width;
    int h = face.box.height;

    if (w <= 0 || h <= 0) {
        std::cerr << "[Liveness] Invalid face bounding box." << std::endl;
        return {};
    }

    int l = std::max(w, h);
    float xc = x + w / 2.0f;
    float yc = y + h / 2.0f;

    int crop_x = static_cast<int>(xc - l * bbox_inc / 2.0f);
    int crop_y = static_cast<int>(yc - l * bbox_inc / 2.0f);
    int crop_w = static_cast<int>(l * bbox_inc);
    int crop_h = static_cast<int>(l * bbox_inc);

    if (crop_x >= real_w || crop_y >= real_h || crop_x + crop_w <= 0 || crop_y + crop_h <= 0) {
        std::cerr << "[Liveness] Crop is out of image bounds." << std::endl;
        return {};
    }

    int x1 = std::max(0, crop_x);
    int y1 = std::max(0, crop_y);
    int x2 = std::min(real_w, crop_x + crop_w);
    int y2 = std::min(real_h, crop_y + crop_h);

    if ((x2 - x1) <= 0 || (y2 - y1) <= 0) {
        std::cerr << "[Liveness] Invalid crop region." << std::endl;
        return {};
    }

    cv::Mat cropped = rgb(cv::Rect(x1, y1, x2 - x1, y2 - y1)).clone();

    // Padding if crop area exceeds image borders
    int top = y1 - crop_y;
    int bottom = crop_y + crop_h - y2;
    int left = x1 - crop_x;
    int right = crop_x + crop_w - x2;

    cv::copyMakeBorder(cropped, cropped, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    cv::Mat resized;
    cv::resize(cropped, resized, cv::Size(target_size, target_size));
    return resized;
}

LivenessResult run_liveness_check(const cv::Mat& input_image, const FaceDetectionResult& face) {
    LivenessResult lr;
    
    if (!liveness_session) {
        std::cerr << "[Liveness] ERROR: no valid session" << std::endl;
        return lr;
    }

    // Preprocess face crop with bounding box
    cv::Mat padded = preprocess_liveness_crop(input_image, face, MODEL_INPUT_SIZE);
    if (padded.empty()) {
        std::cerr << "[Liveness] Preprocessing failed, skipping liveness check." << std::endl;
        return lr;
    }
    
//    cv::imshow("input_image", input_image);
//    cv::imshow("padded", padded);
//    cv::waitKey();

    // Convert to CHW float32 [1,3,128,128]
    padded.convertTo(padded, CV_32F, 1.0 / 255);
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
    std::vector<const char*> input_names = {liveness_input_name.c_str()};
    Ort::AllocatorWithDefaultOptions allocator;
    Ort::AllocatedStringPtr output_name = liveness_session->GetOutputNameAllocated(0, allocator);
    std::vector<const char*> output_names = {output_name.get()};

    auto output_tensors = liveness_session->Run(Ort::RunOptions{nullptr}, input_names.data(), &input_tensor_ort, 1, output_names.data(), 1);
    float* output_data = output_tensors[0].GetTensorMutableData<float>();
    
    std::cout << "[Liveness] Raw output: " << output_data[0] << ", " << output_data[1] << std::endl;

    // Softmax
    float e0 = std::exp(output_data[0]);
    float e1 = std::exp(output_data[1]);
    float sum = e0 + e1;
    float live_score = e0 / sum;

    std::cout << "[Liveness] Score: " << live_score << std::endl;
    
    lr.score = live_score;
    lr.isLive = live_score > liveness_thresh;
    
    return lr;
}

