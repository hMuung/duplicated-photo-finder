#include <iostream>
#include <string>
#include <cstdint>
#include <filesystem>
#include <vector>
#include <fstream>
#include <thread>

#include "dpf/PHash.h"
#include "dpf/Logger.h"
#include "dpf/SafeQueue.h"


namespace fs = std::filesystem;


struct ImageResult {
    std::string path;
    uint64_t hash;
};


bool isImage(const fs::path& extension);
void collectImagePaths(const std::string& folderPath, SafeQueue& imgQueue);
void processHashes(SafeQueue& imgQueue, std::vector<ImageResult>& results, std::mutex& resultsMutex);
bool saveResultsToDisk(const std::string& outputFilePath, const std::vector<ImageResult>& results);
    

int main() {
    
    Logger::init();

    std::string folderPath = "D:\\Documentos\\respaldo\\footos";
    std::string outputFilePath = "hashes_output.txt";

    SafeQueue imgQueue;
    std::vector<ImageResult> results;
    std::mutex resultsMutex;

    // Collection phase with background producer thread
    std::cout << "Starting image collection ...\n";
    std::thread producer(collectImagePaths, std::ref(folderPath), std::ref(imgQueue));

    // Determine available worker threads
    unsigned int numWorkers = std::thread::hardware_concurrency();
    if (numWorkers == 0) numWorkers = 2;
    std::cout << "Launching " << numWorkers << " parallel processing worker threads...\n";

    // Procesing phase across multiple consumer threads
    std::vector<std::thread> workers;
    for (unsigned int i = 0; i < numWorkers; ++i) {
        workers.emplace_back(processHashes, std::ref(imgQueue), std::ref(results), std::ref(resultsMutex));
    }

    // Wait for collection to end and workers to finish consuming
    producer.join();
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    std::cout << "Processing complete. Total processed assets: " << results.size() << "\n";
    std::cout << "Saving results to disk...\n";

    // Output phase
    if (saveResultsToDisk(outputFilePath, results)) {
        std::cout << "Results successfully written to '" << outputFilePath << "'\n";
    } else {
        return 1;
    }

    Logger::shutdown();

    return 0;
}


bool isImage(const fs::path& extension) {
    std::string ext = extension.string();
    for (char& c : ext) {
        c = std::tolower(c);
    }
    return (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp");
}

void collectImagePaths(const std::string& folderPath, SafeQueue& imgQueue) {
    
    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        std::cerr << "Error: Target path does not exist or is not a directory\n";
        Logger::error("Target path does not exist or is not a directory: " + folderPath);
        imgQueue.setFinished();
        return;
    }
    
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.is_regular_file() && isImage(entry.path().extension())) {
            imgQueue.push(entry.path());
        }
    }

    // Notify workers that no more images will be added
    imgQueue.setFinished();

}

void processHashes(SafeQueue& imgQueue, std::vector<ImageResult>& results, std::mutex& resultsMutex) {
    PHash phash{};
    fs::path imagePath;

    // Pop returns false only when the queue is empty AND collection is finished
    while (imgQueue.pop(imagePath)) {
        std::string pathStr = imagePath.string();
        uint64_t hash = phash.getPHash(pathStr.c_str());

        if (hash == 0) {
            Logger::error("Failed to process image or extract hash: " + pathStr);
        }

        //Total size unknown, final vector push protected
        std::lock_guard<std::mutex> lock(resultsMutex);
        results.push_back({ pathStr, hash });
    }
}

bool saveResultsToDisk(const std::string& outputFilePath, const std::vector<ImageResult>& results) {
    std::ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) {
        std::cerr << "Critical Error: Could not open output file for writing\n";
        Logger::error("Could not open output file: " + outputFilePath);
        return false;
    }

    for (const auto& res : results) {
        outputFile << res.hash << "|" << res.path << "\n";
    }

    outputFile.close();
    return true;
}



