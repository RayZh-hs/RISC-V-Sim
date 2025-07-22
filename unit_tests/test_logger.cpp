#include "third_party/logger.hpp"
#include <vector>

using namespace norb;

// --- Custom Type Outputting Utility ---
// To log a custom type, simply overload the << operator for it.
// Our logger's templated LogStream will automatically use it.
struct Point {
    int x, y;
};

std::ostream& operator<<(std::ostream& os, const Point& p) {
    os << "Point(x=" << p.x << ", y=" << p.y << ")";
    return os;
}

int main() {
    // ---- How to compile ----
    // g++ -std=c++17 -o logger_test main.cpp

    std::cout << "--- Using global logger instance ---" << std::endl;

    // Use the singleton for easy access
    Logger& log = Logger::get();

    // === Feature 1: Log Levels via log.as() ===
    log.as(LogLevel::INFO) << "Logger initialized. Default log level is INFO.";
    log.as(LogLevel::DEBUG) << "This is a debug message. It will NOT appear by default.";
    log.as(LogLevel::WARN) << "This is a warning message.";
    log.as(LogLevel::ERROR) << "This is an error message. It will go to stderr.";

    std::cout << "\n--- Setting log level to DEBUG ---" << std::endl;
    log.setLevel(LogLevel::DEBUG);
    log.as(LogLevel::DEBUG) << "This debug message WILL now appear.";

    // === Feature 2: Custom Type Outputting ===
    std::cout << "\n--- Logging custom types ---" << std::endl;
    Point p = {10, 20};
    log.as(LogLevel::INFO) << "Logging a custom struct: " << p;

    std::vector<int> v = {1, 2, 3};
    log.as(LogLevel::INFO) << "Logging a vector's size: " << v.size()
                           << " and its capacity: " << v.capacity();

    // === Feature 3: Selective File Output ===
    std::cout << "\n--- Enabling file logging to 'app.log' ---" << std::endl;
    if (log.setFileOutput("app.log")) {
        log.as(LogLevel::INFO) << "This message will go to the console AND app.log.";
        log.as(LogLevel::WARN) << "So will this one. Check the file!";
    }

    // === Feature 4: Selective Color Output ===
    std::cout << "\n--- Disabling color output ---" << std::endl;
    log.enableColor(false);
    log.as(LogLevel::INFO) << "This log message is now plain (no color).";
    log.as(LogLevel::ERROR) << "This error message is also plain (on stderr).";

    // Re-enable for the final message
    log.enableColor(true);

    std::cout << "\n--- Demonstrating FATAL log level ---" << std::endl;
    log.as(LogLevel::FATAL) << "This is a fatal error. The program will now exit.";

    // This line will never be reached
    log.as(LogLevel::INFO) << "This will not be printed.";

    return 0; // Unreachable
}