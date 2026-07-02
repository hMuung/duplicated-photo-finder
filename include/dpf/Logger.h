#pragma once

#include <string_view>

/*
Thread-safe subsystem for application event and diagnostic logging.
 
Manages the persistence of runtime failures and system diagnostics to a designated log file
Acts as a centralized utility to record critical error reports across the application
*/

class Logger {
public:
    // Writes error message with a line break
    static void logError(std::string_view mensaje);
};
