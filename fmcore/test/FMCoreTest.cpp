#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "FMCore.h"

void process_and_store(
    FMCore& core,
    const std::string& imagePath,
    PipelineMode mode,
    ProcessResult& outResult
) {
    std::cout << "\n--- Processing: " << imagePath << " ---\n";
    outResult = core.process(imagePath, mode);
    
    if (mode == PipelineMode::SkipLiveness) {
        std::cout << "[Result] Liveness skipped" << std::endl;
    }
    if (outResult.livenessChecked) {
        std::cout << "[Result] Liveness check: " << (outResult.isLive ? "LIVE" : "SPOOF") << std::endl;
    }

    if (mode != PipelineMode::OnlyLiveness) {
        if (!outResult.faceDetected) {
            std::cout << "[Result] No face detected.\n";
        } else if (!outResult.embeddingExtracted) {
            std::cout << "[Result] Embedding extraction failed.\n";
        } else {
            std::cout << "[Result] Embedding extracted.\n";
        }
    }

}

void match_embeddings(const std::string& label, const std::vector<float>& emb1, const std::vector<float>& emb2, FMCore& core) {
    if (emb1.empty() || emb2.empty()) {
        std::cout << "[Match] Skipping " << label << ": one or both embeddings are empty.\n";
        return;
    }
    bool isHim = core.match(emb1, emb2);
    if (isHim) {
        std::cout << "[Match] ("<< label << "): SAME SUBJECT" << std::endl;
    } else {
        std::cout << "[Match] ("<< label << "): DIFFERENT SUBJECT" << std::endl;
    }
    
}

int main() {
    FMCore core;

    // Load config JSON from file
    const std::string configPath = "assets/config.json";
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        std::cerr << "Failed to open config file: " << configPath << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << configFile.rdbuf();
    std::string configJson = buffer.str();

    const std::string modelBasePath = "../../models";

    std::cout << "Initializing core...\n";
    if (!core.init(configJson, modelBasePath)) {
        std::cerr << "Initialization failed.\n";
        return 1;
    }

    // Process images
    ProcessResult r0, r1, r2, r3;

    std::cout << std::endl << std::endl;
    std::cout << "[Matching] Spoof image - whole pipeline..." << std::endl;
    process_and_store(core, "assets/spoof0.png", PipelineMode::WholePipeline, r0);
    std::cout << "[Matching] Spoof image - only liveness..." << std::endl;
    process_and_store(core, "assets/spoof0.png", PipelineMode::OnlyLiveness, r0);
    std::cout << "[Matching] Spoof image - skip liveness..." << std::endl;
    process_and_store(core, "assets/spoof0.png", PipelineMode::SkipLiveness, r0);
    std::cout << "[Matching] Keanu1 image - whole pipeline..." << std::endl;
    process_and_store(core, "assets/keanu.png", PipelineMode::WholePipeline, r1);
    std::cout << "[Matching] Keanu2 image - whole pipeline..." << std::endl;
    process_and_store(core, "assets/keanu2.png", PipelineMode::WholePipeline, r2);
    std::cout << "[Matching] Cruise image - whole pipeline..." << std::endl;
    process_and_store(core, "assets/cruise.png", PipelineMode::WholePipeline, r3);

    // Matching
    match_embeddings("Keanu1 vs Keanu2", r1.embedding, r2.embedding, core);
    match_embeddings("Keanu1 vs Cruise", r1.embedding, r3.embedding, core);
    match_embeddings("Keanu2 vs Cruise", r2.embedding, r3.embedding, core);

    core.reset();
    return 0;
}
