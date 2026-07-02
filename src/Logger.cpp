#include "dpf/Logger.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

// Static member definitions
std::mutex Logger::logMutex{};
std::ofstream Logger::logFile{};
std::filesystem::path Logger::filePath{};

namespace { // start anonymous namespace

std::string getTimestamp() {
    using namespace std::chrono;

    const auto now = system_clock::now();
    const std::time_t timeNow = system_clock::to_time_t(now);

    std::tm localTime{};
    #ifdef _WIN32
        localtime_s(&localTime, &timeNow);
    #else
        localtime_r(&timeNow, &localTime);
    #endif

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");

    return stream.str();
}

void writeLog(std::ofstream& file, std::mutex& mutex, const std::filesystem::path& filePath, 
    std::string_view level, std::string_view message) {

    std::lock_guard<std::mutex> lock(mutex);
    
    if (!file.is_open()) {
        // Open file only on first log
        file.open(filePath, std::ios::app);
    }


    file << "[" << getTimestamp() << "] " << "[" << level << "] " << message << '\n';
    file.flush();
}

} // end anonymous namespace


void Logger::init(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(logMutex);
    filePath = path;
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(logMutex);

    if (logFile.is_open()) {
        logFile.flush();
        logFile.close();
    }
}

void Logger::error(std::string_view message) {
    writeLog(logFile, logMutex, filePath, "ERROR", message);
}

void Logger::warning(std::string_view message) {
    writeLog(logFile, logMutex, filePath, "WARNING", message);
}

void Logger::info(std::string_view message) {
    writeLog(logFile, logMutex, filePath, "INFO", message);
}


