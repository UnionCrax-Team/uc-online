#include "logger.hpp"
#include <fstream>
#include <iostream>

Logger::Logger(const std::string& logFilePath, bool enableLogging)
    : _logFilePath(logFilePath), _loggingEnabled(enableLogging) {
    if (_loggingEnabled) {
        Log("Logger initialized");
    }
}

void Logger::SetLoggingEnabled(bool enabled) {
    if (_loggingEnabled && !enabled) {
        Log("Logging disabled");
    }
    _loggingEnabled = enabled;
    if (enabled) {
        Log("Logging enabled");
    }
}

bool Logger::IsLoggingEnabledA() const {
    return _loggingEnabled;
}

void Logger::Log(const std::string& message) {
    if (!_loggingEnabled) return;

    std::string logMessage = GetCurrentTimeString() + " [INFO] " + message + "\n";

    std::lock_guard<std::mutex> lock(_lock);
    std::ofstream file(_logFilePath, std::ios::app);
    if (file.is_open()) {
        file << logMessage;
    } else {
        std::cerr << "Logging error: Could not open log file" << std::endl;
    }
}

void Logger::LogWarning(const std::string& message) {
    if (!_loggingEnabled) return;

    std::string logMessage = GetCurrentTimeString() + " [WARNING] " + message + "\n";

    std::lock_guard<std::mutex> lock(_lock);
    std::ofstream file(_logFilePath, std::ios::app);
    if (file.is_open()) {
        file << logMessage;
    } else {
        std::cerr << "Logging error: Could not open log file" << std::endl;
    }
}

void Logger::LogError(const std::string& message) {
    if (!_loggingEnabled) return;

    std::string logMessage = GetCurrentTimeString() + " [ERROR] " + message + "\n";

    std::lock_guard<std::mutex> lock(_lock);
    std::ofstream file(_logFilePath, std::ios::app);
    if (file.is_open()) {
        file << logMessage;
    } else {
        std::cerr << "Logging error: Could not open log file" << std::endl;
    }
}

void Logger::LogException(const std::exception& ex, const std::string& context) {
    if (!_loggingEnabled) return;

    std::string logMessage = GetCurrentTimeString() + " [EXCEPTION] " + context + ": " + ex.what() + "\n";

    std::lock_guard<std::mutex> lock(_lock);
    std::ofstream file(_logFilePath, std::ios::app);
    if (file.is_open()) {
        file << logMessage;
    } else {
        std::cerr << "Logging error: Could not open log file" << std::endl;
    }
}

void Logger::ClearLog() {
    if (!_loggingEnabled) return;

    std::lock_guard<std::mutex> lock(_lock);
    std::ofstream file(_logFilePath, std::ios::trunc);
    if (file.is_open()) {
        file << "uc-online Log - " << GetCurrentTimeString() << "\n";
    } else {
        std::cerr << "Error clearing log: Could not open log file" << std::endl;
    }
}

std::string Logger::GetCurrentTimeString() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time_t);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}