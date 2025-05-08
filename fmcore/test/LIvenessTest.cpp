#include "FMCore.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>

namespace fs = std::filesystem;

struct LivenessResult {
    std::string path;
    bool isGenuine;
    float score;
};

void collect_images(const fs::path& root, std::vector<std::string>& images) {
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            if (ext == ".jpg" || ext == ".png" || ext == ".bmp") {
                images.push_back(entry.path().string());
            }
        }
    }
}

void saveResultsToCSV(
    const std::vector<std::tuple<std::string, bool, float>>& results,
    const std::string& csvPath
) {
    std::ofstream out(csvPath);
    if (!out.is_open()) {
        std::cerr << "Failed to open CSV file: " << csvPath << std::endl;
        return;
    }

    out << "full_path,is_genuine,score\n";
    for (const auto& [path, is_genuine, score] : results) {
        out << "\"" << path << "\"," << (is_genuine ? 1 : 0) << "," << score << "\n";
    }

    out.close();
    std::cout << "Results saved to " << csvPath << std::endl;
}

void compute_thresholds(const std::string& csvPath) {
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << csvPath << std::endl;
        return;
    }

    std::vector<LivenessResult> results;
    std::string line;
    std::getline(file, line); // skip header
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string path, is_genuine_str, score_str;
        std::getline(ss, path, ',');
        std::getline(ss, is_genuine_str, ',');
        std::getline(ss, score_str, ',');

        bool is_genuine = (is_genuine_str == "1" || is_genuine_str == "true");
        float score = std::stof(score_str);
        results.push_back({path, is_genuine, score});
    }

    std::vector<float> genuine_scores, spoof_scores;
    for (const auto& r : results) {
        (r.isGenuine ? genuine_scores : spoof_scores).push_back(r.score);
    }

    std::sort(results.begin(), results.end(),
              [](const LivenessResult& a, const LivenessResult& b) {
                  return a.score > b.score;
              });

    std::vector<double> far_targets = {0.1, 0.01, 0.001, 0.0001};

    std::cout << "Total genuine: " << genuine_scores.size() << ", Total spoof: " << spoof_scores.size() << "\n";

    for (double target_far : far_targets) {
        float best_threshold = 0.0f;
        double best_far = 1.0, best_frr = 1.0;

        // Step through thresholds from 0 to 1 in small increments
        for (float threshold = 0.0f; threshold <= 1.0f; threshold += 0.0005f) {
            int false_accepts = std::count_if(spoof_scores.begin(), spoof_scores.end(),
                                              [&](float s) { return s >= threshold; });
            int false_rejects = std::count_if(genuine_scores.begin(), genuine_scores.end(),
                                              [&](float s) { return s < threshold; });

            double far = double(false_accepts) / spoof_scores.size();
            double frr = double(false_rejects) / genuine_scores.size();

            if (far <= target_far) {
                best_threshold = threshold;
                best_far = far;
                best_frr = frr;
                break;  // lowest threshold that satisfies FAR ≤ target
            }
        }

        std::cout << "Threshold for FAR ≤ " << target_far
                  << ": " << best_threshold
                  << " -> FAR: " << best_far
                  << ", FRR: " << best_frr << "\n";
    }
}


int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: ./test_liveness <dataset_path>" << std::endl;
        return 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--thresh") {
        compute_thresholds("liveness_results.csv");
        return 0;
    }

    std::string dataset_path = argv[1];
    std::string config_path = "assets/config.json";
    std::string model_base_path = "../../models";

    // Load config
    std::ifstream configFile(config_path);
    if (!configFile.is_open()) {
        std::cerr << "Failed to open config file: " << config_path << std::endl;
        return 1;
    }
    std::stringstream buffer;
    buffer << configFile.rdbuf();
    std::string configJson = buffer.str();

    FMCore core;
    if (!core.init(configJson, model_base_path)) {
        std::cerr << "Failed to initialize FMCore" << std::endl;
        return 1;
    }

    std::vector<std::string> genuine_images;
    std::vector<std::string> spoof_images;
    collect_images(fs::path(dataset_path) / "GENUINE", genuine_images);
    collect_images(fs::path(dataset_path) / "SPOOF", spoof_images);

    int genuine_noface = 0;
    int spoof_noface = 0;
    int genuine_fp = 0; // false positive: classified as spoof
    int spoof_fn = 0;   // false negative: classified as live
    
    std::vector<std::tuple<std::string, bool, float>> results;

    std::cout << "Testing GENUINE samples..." << std::endl;
    for (const auto& img : genuine_images) {
        ProcessResult result = core.process(img, PipelineMode::OnlyLiveness);
        results.emplace_back(img, true, result.livenessScore);
        if (!result.livenessChecked) {
            ++genuine_noface;
            continue;
        }
        if (!result.isLive) {
            ++genuine_fp;
        }
    }

    std::cout << "Testing SPOOF samples..." << std::endl;
    for (const auto& img : spoof_images) {
        ProcessResult result = core.process(img, PipelineMode::OnlyLiveness);
        results.emplace_back(img, false, result.livenessScore);
        if (!result.livenessChecked) {
            ++spoof_noface;
            continue;
        }
        if (result.isLive) {
            ++spoof_fn;
        }
    }
    
    saveResultsToCSV(results, "liveness_results.csv");


    std::cout << "\n===== Evaluation Report =====" << std::endl;
    std::cout << "GENUINE images: " << genuine_images.size() << std::endl;
    std::cout << "SPOOF images: " << spoof_images.size() << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "False Positives (GENUINE misclassified): " << genuine_fp
              << " (" << 100.0 * genuine_fp / genuine_images.size() << "%)" << std::endl;
    std::cout << "False Negatives (SPOOF misclassified): " << spoof_fn
              << " (" << 100.0 * spoof_fn / spoof_images.size() << "%)" << std::endl;
    std::cout << "No faces detected: " << genuine_noface << " genuine - " << spoof_noface << " spoof - " << genuine_noface + spoof_noface << " total" << std::endl;

    return 0;
}

