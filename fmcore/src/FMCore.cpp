#include "FMCore.h"
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "json.hpp"
#include "liveness.h"
#include "face_detection.h"
#include "face_alignment.h"
#include "embedding_extraction.h"

#ifdef FMCORE_NATIVE_BUILD
    const bool DEBUG = false;
    #include <opencv2/highgui.hpp>
#endif

using json = nlohmann::json;

static float matchingThresh;


// TODO include this stuff in a debug build only //////////////////////
#include <ctime>

static std::string g_debugSavePath;

void setDebugSavePath(const std::string& path) {
  g_debugSavePath = path;
}

void saveDebugImage(const cv::Mat& img, const std::string& filename) {
  if (g_debugSavePath.empty()) return;

  std::time_t now = std::time(nullptr);
  std::tm tm;
#if defined(_WIN32)
  localtime_s(&tm, &now);
#else
  localtime_r(&now, &tm);
#endif

  char buf[20];
  std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm);

  std::string outPath = g_debugSavePath + "/" + buf + "_" + filename;

  cv::imwrite(outPath, img);
}

// ////////////////////////////////

bool FMCore::init(const std::string& configJson, const std::string& modelBasePath) {
    std::cout << "[FMCore] Initialized with config: " << configJson << std::endl;
    
    // Parse configuration
    json config;
    try {
        config = json::parse(configJson);
    } catch (const std::exception& e) {
        std::cerr << "[FMCore] Failed to parse JSON config: " << e.what() << std::endl;
        return false;
    }

    // Extract model names
    if (!config.contains("liveness_model")) {
        std::cerr << "[FMCore] Missing liveness model in config." << std::endl;
        return false;
    }
    if (!config.contains("face_detector_model")) {
        std::cerr << "[FMCore] Missing face detector model in config." << std::endl;
        return false;
    }
    if (!config.contains("embedding_extractor_model")) {
        std::cerr << "[FMCore] Missing embedding extractor model in config." << std::endl;
        return false;
    }

    const std::string livenessModel = config["liveness_model"];
    const std::string faceModel = config["face_detector_model"];
    const std::string embModel = config["embedding_extractor_model"];
    const float livenessThresh = config["liveness_threshold"];
    matchingThresh = config["matching_threshold"];

    std::string livenessModelPath = joinPath(modelBasePath, livenessModel);
    std::string faceModelPath = joinPath(modelBasePath, faceModel);
    std::string embModelPath = joinPath(modelBasePath, embModel);
    
    std::cout << "[FMCore] Liveness model path: " << livenessModelPath << std::endl;
    std::cout << "[FMCore] Face detector model path: " << faceModelPath << std::endl;
    std::cout << "[FMCore] Embedding extractor model path: " << embModelPath << std::endl;
    
    Ort::SessionOptions ort_session_options;
    ort_session_options.SetIntraOpNumThreads(1);
    ort_session_options.SetGraphOptimizationLevel(ORT_ENABLE_BASIC);
    
    bool res_ld = init_liveness_detector(ort_session_options, livenessModelPath, livenessThresh);
    if (!res_ld) {
        std::cerr << "[FMCore] Failed to init liveness detector" << std::endl;
    }
    // Mediapipe onnx face detection model are from https://github.com/Tensor46/mpface
//    bool success = init_face_detector("../../models/mediapipe_short.onnx", /* short_range= */ true);
    bool res_fd = init_face_detector(ort_session_options, faceModelPath, /* short_range= */ false);
    if (!res_fd) {
        std::cout << "[FMCore] Failed to init face detector" << std::endl;
    }
    
    bool res_emb_ex = init_embedding_extractor(ort_session_options, embModelPath);
    if (!res_emb_ex) {
        std::cout << "[FMCore] Failed to init embedding extractor" << std::endl;
    }

    return res_ld && res_fd && res_emb_ex;
}


ProcessResult FMCore::process(const std::string& imagePath, PipelineMode mode) {
    std::cout << "[FMCore] Processing image: " << imagePath << std::endl;
    ProcessResult result;

    // Load image
    cv::Mat image = cv::imread(imagePath, cv::IMREAD_COLOR);
    
//    saveDebugImage(image, "input.png");
    
    if (image.empty()) {
        std::cerr << "[FMCore] Failed to load image at: " << imagePath << std::endl;
        return result;
    }

    std::cout << "[FMCore] Image size: " << image.cols << "x" << image.rows << std::endl;

    
    // Step 1: Face detection
    std::vector<FaceDetectionResult> faces = detect_faces(image);
    if (faces.empty()) {
        std::cout << "[FMCore] No faces detected." << std::endl;
        return result;
    }

    result.faceDetected = true;
    std::cout << "[FMCore] Detected " << faces.size() << " face(s)." << std::endl;
    
    // Step 2: Liveness
    if (mode == PipelineMode::OnlyLiveness || mode == PipelineMode::WholePipeline) {
        result.livenessChecked = true;
        LivenessResult resLiveness = run_liveness_check(image, faces[0]);
        result.isLive = resLiveness.isLive;
        result.livenessScore = resLiveness.score;
        if (!result.isLive) {
            std::cout << "[FMCore] Liveness check failed. [SPOOF]" << std::endl;
            return result;
        }
        std::cout << "[FMCore] Liveness check passed. [LIVE]" << std::endl;
    }

    if (mode == PipelineMode::OnlyLiveness) {
        return result;
    }

    // Step 3: Align and extract embedding
    cv::Mat alignedFace = align_face(image, faces[0]);

#ifdef FMCORE_NATIVE_BUILD
    if(DEBUG) {
        debug_aligned_faces(image, faces[0], alignedFace);
    }
#endif
    
//    saveDebugImage(alignedFace, "alignedFace.png");

    result.embedding = extract_embedding(alignedFace);
    result.embeddingExtracted = !result.embedding.empty();

    return result;
}


bool FMCore::match(const std::vector<float>& embedding1, const std::vector<float>& embedding2) {
    std::cout << "[FMCore] Matching embeddings..." << std::endl;

    if (embedding1.size() != embedding2.size()) return 0.0f;

    float dot = 0.0f, norm1 = 0.0f, norm2 = 0.0f;
    for (size_t i = 0; i < embedding1.size(); ++i) {
        dot += embedding1[i] * embedding2[i];
        norm1 += embedding1[i] * embedding1[i];
        norm2 += embedding2[i] * embedding2[i];
    }
    
    float score = dot / (std::sqrt(norm1) * std::sqrt(norm2) + 1e-6f);
    
    std::cout << "[FMCore] Matching score: " << score << std::endl;

    return score >= matchingThresh;
}

void FMCore::reset() {
    std::cout << "[FMCore] Resetting internal state..." << std::endl;
    // TODO: reset any cached state if needed
}

