#include "dpf/Logger.h"

#include <fstream>
#include <iostream>

void Logger::logError(std::string_view mensaje) {
    // Open file in add mode
    if (std::ofstream archivo{"erros.txt", std::ios::app}) {
        archivo << "[ERROR]: " << mensaje << "\n";
    } else {
        std::cerr << "Critical error: could not open error.txt.\n";
    }
}
