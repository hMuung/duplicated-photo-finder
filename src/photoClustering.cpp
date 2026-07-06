#include <iostream>
#include <string>
#include <cstdint>
#include <filesystem>
#include <vector>
#include <fstream>
#include <thread>
#include <mutex>
#include <bit>
#include <algorithm>

#include "dpf/Logger.h"

namespace fs = std::filesystem;


const std::string INPUT_FILE = "hashes_output.txt";
const fs::path OUTPUT_BASE_DIR = "grouped_photos";
constexpr int HAMMING_THRESHOLD = 5; // Max bit differences to consider "similar"


struct ImageResult {
    uint64_t hash;
    std::string path;
};

bool parseLine(const std::string& line, ImageResult& outResult);
void clusterWorker(const std::vector<ImageResult>& dataset, 
    std::vector<bool>& visited, std::vector<std::string>& currentGroup, 
    uint64_t anchorHash, size_t startIdx,  size_t endIdx, std::mutex& mtx);


int main() {

    Logger::init();

    std::vector<ImageResult> dataset;
    
    // Read existing file from disk
    std::cout << "Opening metadata file: " << INPUT_FILE << "...\n";
    std::ifstream inputFile(INPUT_FILE);
    if (!inputFile.is_open()) {
        std::cerr << "Critical Error: Could not open " << INPUT_FILE << ". Make sure it exists in the working directory.\n";
        return 1;
    }

    std::string line;
    while (std::getline(inputFile, line)) {
        ImageResult res;
        if (parseLine(line, res)) {
            dataset.push_back(res);
        }
    }
    inputFile.close();

    std::cout << "Successfully parsed " << dataset.size() << " records from log.\n";
    if (dataset.empty()) return 0;

    // Initialize tracking allocations
    size_t numImages = dataset.size();
    std::vector<bool> visited(numImages, false);
    std::mutex mtx;

    unsigned int numWorkers = std::thread::hardware_concurrency();
    if (numWorkers == 0) numWorkers = 2;
    std::cout << "Spawning " << numWorkers << " threads for parallel Hamming Distance processing...\n";

    size_t groupCounter = 1;

    // Multi-threaded Clustering Loop
    for (size_t i = 0; i < numImages; ++i) {
        if (visited[i]) continue;

        // Establish a baseline anchor for a new cluster group
        std::vector<std::string> currentGroup;
        visited[i] = true;
        currentGroup.push_back(dataset[i].path);

        uint64_t anchorHash = dataset[i].hash;
        size_t totalRemaining = numImages - (i + 1);
        
        if (totalRemaining > 0) {
            std::vector<std::thread> workers;
            size_t chunkSize = (totalRemaining + numWorkers - 1) / numWorkers; 

            for (unsigned int t = 0; t < numWorkers; ++t) {
                size_t startIdx = (i + 1) + (t * chunkSize);
                size_t endIdx = std::min(startIdx + chunkSize, numImages);

                if (startIdx < endIdx) {
                    workers.emplace_back(clusterWorker, std::ref(dataset), std::ref(visited), 
                                         std::ref(currentGroup), anchorHash, startIdx, endIdx, std::ref(mtx));
                }
            }

            for (auto& worker : workers) {
                if (worker.joinable()) worker.join();
            }
        }

        // File Output/Movement phase 
        // Only isolate files if a cluster actually contains multiple matching assets
        if (currentGroup.size() > 1) {
            fs::path groupFolder = OUTPUT_BASE_DIR / ("group_" + std::to_string(groupCounter++));
            
            try {
                fs::create_directories(groupFolder);
                std::cout << "\n[Group " << groupCounter - 1 << "] Found " << currentGroup.size() << " visually close photos.\n";
                
                for (const auto& origPathStr : currentGroup) {
                    fs::path origPath(origPathStr);
                    if (fs::exists(origPath)) {
                        fs::path destPath = groupFolder / origPath.filename();
                        fs::rename(origPath, destPath); // Moves the file quickly
                        std::cout << "  -> Relocated: " << origPath.filename() << "\n";
                    } else {
                        std::cerr << "  -> File skipped (Not found on system): " << origPathStr << "\n";
                    }
                }
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Filesystem Error occurred processing group: " << e.what() << "\n";
            }
        }
    }

    std::cout << "\nExecution sequence complete. " << (groupCounter - 1) << " total cluster groups identified.\n";
    return 0;
}


// Parses a line like "123456789|/path/to/img.jpg"
bool parseLine(const std::string& line, ImageResult& outResult) {

    size_t delimiterPos = line.find('|');
    if (delimiterPos == std::string::npos) return false;

    std::string hashStr = line.substr(0, delimiterPos);
    std::string pathStr = line.substr(delimiterPos + 1);

    if (hashStr.empty() || pathStr.empty()) return false;

    try {
        outResult.hash = std::stoull(hashStr, nullptr, 10);
        outResult.path = pathStr;
        return true;
    } catch (...) {
        Logger::error("Unable to get hash: " + outResult.path);
        return false; // Skip malformed lines quietly
    }
}


// Thread worker function to scan a chunk of the dataset
void clusterWorker(const std::vector<ImageResult>& dataset, 
    std::vector<bool>& visited, std::vector<std::string>& currentGroup, 
    uint64_t anchorHash, size_t startIdx,  size_t endIdx, std::mutex& mtx) {

    std::vector<std::string> localMatches;

    for (size_t j = startIdx; j < endIdx; ++j) {
        // Read-only optimization check before acquiring the lock
        if (visited[j]) continue; 

        // std::popcount calculates the exact Hamming distance using CPU hardware acceleration
        if (std::popcount(anchorHash ^ dataset[j].hash) <= HAMMING_THRESHOLD) {
            
            std::lock_guard<std::mutex> lock(mtx);
            if (!visited[j]) { // Double-check locking pattern
                visited[j] = true;
                localMatches.push_back(dataset[j].path);
            }
        }
    }

    // Safely append local batch updates to the master group
    if (!localMatches.empty()) {
        std::lock_guard<std::mutex> lock(mtx);
        currentGroup.insert(currentGroup.end(), localMatches.begin(), localMatches.end());
    }
}

