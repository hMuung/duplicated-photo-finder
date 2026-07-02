#pragma once

#include <string_view>
#include <mutex>
#include <fstream>
#include <filesystem>

/*
Thread-safe logging utility
- Single shared log file for the whole application
- Opens the file once at first log and keeps it alive
- Safe for concurrent access across multiple threads
*/

class Logger {
    private:
        // Synchronizes write access
        static std::mutex logMutex;

        // Persistent file stream
        static std::ofstream logFile;

        // Current file path
        static std::filesystem::path filePath;

    public:

        // Initializes logger and opens the file
        static void init(const std::filesystem::path& path = "errors.txt");

        // Flushes and closes the file
        static void shutdown();

        // Writes an error message
        static void error(std::string_view message);

        // Writes a warning message
        static void warning(std::string_view message);

        // Writes informational message
        static void info(std::string_view message);
};