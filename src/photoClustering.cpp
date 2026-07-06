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
const fs::path OUTPUT_BASE_DIR = "groups";
constexpr int HAMMING_THRESHOLD = 5;


struct ImageResult {
    uint64_t hash;
    std::string path;
};


// Function prototipes
bool parseLine(const std::string& line, ImageResult& outResult);
std::vector<ImageResult> loadDataset(const std::string& filePath);
void clusterWorker(const std::vector<ImageResult>& dataset, std::vector<bool>& visited, 
    std::vector<std::string>& currentGroup, uint64_t anchorHash, size_t startIdx, size_t endIdx, std::mutex& mtx);
void processSingleCluster(const std::vector<ImageResult>& dataset, std::vector<bool>& visited, 
    std::vector<std::string>& currentGroup, size_t anchorIdx, unsigned int numWorkers, std::mutex& mtx);
void moveClusterFiles(const std::vector<std::string>& currentGroup, size_t groupId);
void processClusters(const std::vector<ImageResult>& dataset, unsigned int numWorkers);


int main() {
    Logger::init();

    // Load data
    std::vector<ImageResult> dataset = loadDataset(INPUT_FILE);
    if (dataset.empty()) {
        return 0; 
    }

    // Determinate threads
    unsigned int numWorkers = std::thread::hardware_concurrency();
    if (numWorkers == 0) numWorkers = 2;
    std::cout << "Spawning " << numWorkers << " threads for parallel processing...\n";

    // Clustering algorithm
    processClusters(dataset, numWorkers);

    std::cout << "\nExecution sequence complete\n";
    return 0;
}


// Loads plan file in memory
std::vector<ImageResult> loadDataset(const std::string& filePath) {
    std::vector<ImageResult> dataset;
    std::cout << "Opening data file: " << filePath << "...\n";
    std::ifstream inputFile(filePath);
    
    if (!inputFile.is_open()) {
        Logger::error("Critical Error: Could not open " + filePath + ", file may be missing or locked");
        std::cerr << "Critical Error: Could not open " << filePath << "\n";
        return dataset;
    }

    std::string line;
    size_t lineCount = 0;
    while (std::getline(inputFile, line)) {
        lineCount++;
        ImageResult res;
        if (parseLine(line, res)) {
            dataset.push_back(res);
        } else {
            // Logg parse error and continue
            Logger::warning("Malformed data pattern at line " + std::to_string(lineCount) + ": " + line);
        }
    }
    inputFile.close();

    std::cout << "Successfully parsed " << dataset.size() << " records\n";
    if (dataset.empty()) {
        Logger::warning("The dataset file " + filePath + " was read successfully but contained no valid hashes");
    }
    
    return dataset;
}

// Clustering loop
void processClusters(const std::vector<ImageResult>& dataset, unsigned int numWorkers) {
    size_t numImages = dataset.size();
    std::vector<bool> visited(numImages, false);
    std::mutex mtx;
    size_t groupCounter = 1;

    for (size_t i = 0; i < numImages; ++i) {
        if (visited[i]) continue;

        std::vector<std::string> currentGroup;
        visited[i] = true;
        currentGroup.push_back(dataset[i].path);

        // Create thread for specific cluster
        processSingleCluster(dataset, visited, currentGroup, i, numWorkers, mtx);

        // Move file if group is valid
        if (currentGroup.size() > 1) {
            moveClusterFiles(currentGroup, groupCounter++);
        }
    }
    
    Logger::info("Clustering finished. Total groups created: " + std::to_string(groupCounter - 1));
}

// Handles the slave thread lifecycle for an individual cluster
void processSingleCluster(const std::vector<ImageResult>& dataset, std::vector<bool>& visited, 
    std::vector<std::string>& currentGroup, size_t anchorIdx, unsigned int numWorkers, std::mutex& mtx) {

    size_t numImages = dataset.size();
    uint64_t anchorHash = dataset[anchorIdx].hash;
    size_t totalRemaining = numImages - (anchorIdx + 1);

    if (totalRemaining == 0) return;

    std::vector<std::thread> workers;
    size_t chunkSize = (totalRemaining + numWorkers - 1) / numWorkers; 

    for (unsigned int t = 0; t < numWorkers; ++t) {
        size_t startIdx = (anchorIdx + 1) + (t * chunkSize);
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

// Move photo clusters
void moveClusterFiles(const std::vector<std::string>& currentGroup, size_t groupId) {
    fs::path groupFolder = OUTPUT_BASE_DIR / ("group_" + std::to_string(groupId));
    
    try {
        fs::create_directories(groupFolder);
        std::cout << "\n[Group " << groupId << "] Found " << currentGroup.size() << " visually close photos\n";
        
        for (const auto& origPathStr : currentGroup) {
            fs::path origPath(origPathStr);
            if (fs::exists(origPath)) {
                fs::path destPath = groupFolder / origPath.filename();
                fs::rename(origPath, destPath); 
                std::cout << "  -> Relocated: " << origPath.filename() << "\n";
            } else {
                Logger::warning("File missing from OS storage during migration: " + origPathStr);
                std::cerr << "  -> File skipped (Not found on system): " << origPathStr << "\n";
            }
        }
    } catch (const fs::filesystem_error& e) {
        Logger::error("OS Filesystem failure on group " + std::to_string(groupId) + ". Reason: " + e.what());
        std::cerr << "Filesystem Error occurred processing group: " << e.what() << "\n";
    }
}

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
        return false; 
    }
}

void clusterWorker(const std::vector<ImageResult>& dataset, std::vector<bool>& visited, 
    std::vector<std::string>& currentGroup, uint64_t anchorHash, size_t startIdx, size_t endIdx, std::mutex& mtx) {

    std::vector<std::string> localMatches;

    for (size_t j = startIdx; j < endIdx; ++j) {
        if (visited[j]) continue; 

        if (std::popcount(anchorHash ^ dataset[j].hash) <= HAMMING_THRESHOLD) {
            std::lock_guard<std::mutex> lock(mtx);
            if (!visited[j]) { 
                visited[j] = true;
                localMatches.push_back(dataset[j].path);
            }
        }
    }

    if (!localMatches.empty()) {
        std::lock_guard<std::mutex> lock(mtx);
        currentGroup.insert(currentGroup.end(), localMatches.begin(), localMatches.end());
    }
}