#include "embedding_extraction.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <numeric>
#include <mutex>
#include <iostream>

namespace {

Ort::Env& getOrtEnv() {
    static Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "Embedding");
    return env;
}

std::unique_ptr<Ort::Session> embedding_session;
std::mutex embedding_mutex;

int embedding_input_size = 112;
float input_mean = 127.5f;
float input_std = 127.5f;

}  // namespace

bool init_embedding_extractor(Ort::SessionOptions& options, const std::string& model_path) {
    try {
        std::lock_guard<std::mutex> lock(embedding_mutex);
        embedding_session = std::make_unique<Ort::Session>(getOrtEnv(), model_path.c_str(), options);
        std::cout << "[Embedding] Loaded model: " << model_path << std::endl;

//        Ort::AllocatorWithDefaultOptions allocator;
//        size_t count = embedding_session->GetOutputCount();
//        for (size_t i = 0; i < count; ++i) {
//            auto name = embedding_session->GetOutputNameAllocated(i, allocator);
//            std::cout << "[Embedding] Output[" << i << "] name: " << name.get() << std::endl;
//        }
        return true;
    } catch (const Ort::Exception& e) {
        std::cerr << "[Embedding] Failed to load model: " << e.what() << std::endl;
        return false;
    }
}

std::vector<float> extract_embedding(const cv::Mat& aligned_face) {
    std::lock_guard<std::mutex> lock(embedding_mutex);

    cv::Mat input;
    cv::resize(aligned_face, input, cv::Size(embedding_input_size, embedding_input_size));
    input.convertTo(input, CV_32FC3);
    cv::cvtColor(input, input, cv::COLOR_BGR2RGB);
    input = (input - input_mean) / input_std;

    std::vector<float> input_tensor_values(3 * embedding_input_size * embedding_input_size);
    std::vector<cv::Mat> channels(3);
    for (int i = 0; i < 3; ++i)
        channels[i] = cv::Mat(embedding_input_size, embedding_input_size, CV_32F, input_tensor_values.data() + i * embedding_input_size * embedding_input_size);
    cv::split(input, channels);  // HWC -> CHW

    std::vector<int64_t> input_shape = {1, 3, embedding_input_size, embedding_input_size};
    Ort::MemoryInfo mem_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        mem_info, input_tensor_values.data(), input_tensor_values.size(),
        input_shape.data(), input_shape.size()
    );

    Ort::AllocatorWithDefaultOptions allocator;
    auto input_name_holder = embedding_session->GetInputNameAllocated(0, allocator);
    auto output_name_holder = embedding_session->GetOutputNameAllocated(0, allocator);

    const char* input_name = input_name_holder.get();
    const char* output_name = output_name_holder.get();

    auto output_tensors = embedding_session->Run(Ort::RunOptions{nullptr},
                                                 &input_name, &input_tensor, 1,
                                                 &output_name, 1);
    float* output_data = output_tensors[0].GetTensorMutableData<float>();
    size_t dim = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape()[1];

    std::vector<float> embedding(output_data, output_data + dim);

    // L2 normalize
    float norm = std::sqrt(std::inner_product(embedding.begin(), embedding.end(), embedding.begin(), 0.0f));
    for (auto& val : embedding) val /= norm;

    return embedding;
}
