
// ___      _______  _______  _______  _______  ______
// |   |    |       ||       ||       ||       ||    _ |
// |   |    |   _   ||    ___||    ___||    ___||   | ||
// |   |    |  | |  ||   | __ |   | __ |   |___ |   |_||_
// |   |___ |  |_|  ||   ||  ||   ||  ||    ___||    __  |
// |       ||       ||   |_| ||   |_| ||   |___ |   |  | |
// |_______||_______||_______||_______||_______||___|  |_|
//
// A simple logging utility with color output and file dump.

#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <chrono>
#include <iomanip>

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class Logger;

namespace norb {
    // ANSI color codes
    namespace Color {
        inline const char* RESET = "\033[0m";
        inline const char* RED = "\033[31m";
        inline const char* GREEN = "\033[32m";
        inline const char* YELLOW = "\033[33m";
        inline const char* BLUE = "\033[34m";
        inline const char* BOLDRED = "\033[1m\033[31m";
    }

    // Convert LogLevel to a string
    inline const char* levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO ";
            case LogLevel::WARN:  return "WARN ";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default: return "?????";
        }
    }

    // Get the color for a specific log level
    inline const char* levelToColor(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return Color::BLUE;
            case LogLevel::INFO:  return Color::GREEN;
            case LogLevel::WARN:  return Color::YELLOW;
            case LogLevel::ERROR: return Color::RED;
            case LogLevel::FATAL: return Color::BOLDRED;
            default: return Color::RESET;
        }
    }
} // end anonymous namespace


/**
 * @class LogStream
 * @brief A temporary, RAII-based stream object for building a log message.
 *
 * This object is returned by Logger::as(). It buffers all streamed data
 * and writes the complete message to the Logger upon its destruction
 * (when it goes out of scope at the end of the statement).
 */
class LogStream {
public:
    // Non-copyable and non-movable to ensure a single, clean write
    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;
    LogStream(LogStream&& other) = delete;
    LogStream& operator=(LogStream&& other) = delete;

    // The destructor is where the log message is actually written.
    ~LogStream();

    // Overload the << operator to accept any type that std::ostream can handle.
    template<typename T>
    LogStream& operator<<(const T& msg) {
        m_buffer << msg;
        return *this;
    }

private:
    // Only Logger can create a LogStream
    friend class Logger;

    // Private constructor, called by Logger::as()
    LogStream(Logger& logger, LogLevel level)
        : m_logger(logger), m_level(level) {}

    Logger& m_logger;
    LogLevel m_level;
    std::stringstream m_buffer;
};


/**
 * @class Logger
 * @brief The main logger class providing a stream-based interface.
 */
class Logger {
public:
    // Singleton access: Logger::get().as(LogLevel::INFO) << "Hello";
    static Logger& get() {
        static Logger instance;
        return instance;
    }

    static int getLineNumber() {
        return get().m_lineNumber;
    }

    void setLevel(LogLevel level) {
        m_logLevel = level;
    }

    void enableColor(bool enabled) {
        m_colorEnabled = enabled;
    }

    bool setFileOutput(const std::string& filename) {
        if (m_fileStream && m_fileStream->is_open()) {
            m_fileStream->close();
        }
        m_fileStream = std::make_unique<std::ofstream>(filename, std::ios::app);
        if (!m_fileStream->is_open()) {
            std::cerr << "Error: Could not open log file: " << filename << std::endl;
            m_fileStream.reset();
            return false;
        }
        return true;
    }

    [[nodiscard]] LogStream as(LogLevel level) {
        return {*this, level};
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

private:
    friend class LogStream; // Allows LogStream to call our private write() method.

    // Private constructor for singleton pattern
    Logger() : m_logLevel(LogLevel::INFO), m_colorEnabled(true), m_lineNumber(1) {}

    // Destructor to ensure the file stream is closed
    ~Logger() {
        if (m_fileStream && m_fileStream->is_open()) {
            m_fileStream->close();
        }
    }

    // The core write function, called by LogStream's destructor
    void write(LogLevel level, const std::string& message) {
        if (level < m_logLevel) {
            return;
        }

        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);

        std::stringstream prefix_stream;
        prefix_stream << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
                      << " [" << std::setw(4) << std::setfill('0') << m_lineNumber << "] "
                      << "[" << norb::levelToString(level) << "] ";

        // Console output (with or without color)
        std::ostream& out = (level >= LogLevel::ERROR) ? std::cerr : std::cout;
        if (m_colorEnabled) {
            out << norb::levelToColor(level) << prefix_stream.str()
                << message << norb::Color::RESET << std::endl;
        } else {
            out << prefix_stream.str() << message << std::endl;
        }

        // File output (always without color)
        if (m_fileStream && m_fileStream->is_open()) {
            *m_fileStream << prefix_stream.str() << message << std::endl;
        }

        // Increment line number for next log entry
        ++m_lineNumber;

        if (level == LogLevel::FATAL) {
            std::exit(EXIT_FAILURE);
        }
    }

    LogLevel m_logLevel;
    bool m_colorEnabled;
    std::unique_ptr<std::ofstream> m_fileStream;
    int m_lineNumber;
};

inline LogStream::~LogStream() {
    m_logger.write(m_level, m_buffer.str());
}