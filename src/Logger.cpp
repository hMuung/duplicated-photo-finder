#include "dpf/Logger.h"

#include <fstream>
#include <iostream>


void Logger::logError(std::string_view message) {
    
    // lock_guard locks the mutex when entering this scope
    // and automatically releases it when leaving the scope,
    // even if an exception occurs
    std::lock_guard<std::mutex> lock(logMutex);

    if (std::ofstream file{"errors.txt", std::ios::app}) {
        file << "[ERROR]: " << message << "\n";
    } else {
        std::cerr << "Critical error: could not open errors.txt.\n";
    }
}