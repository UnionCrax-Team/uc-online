#pragma once

#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

class Logger {
public:
	Logger(const std::string& logFilePath, bool enableLogging);
	void SetLoggingEnabled(bool enabled);
	bool IsLoggingEnabledA() const;
	void Log(const std::string& message);
	void LogWarning(const std::string& message);
	void LogError(const std::string& message);
	void LogException(const std::exception& ex, const std::string& context = "");
	void ClearLog();

private:
	std::string _logFilePath;
	bool _loggingEnabled;
	std::mutex _lock;
	std::string GetCurrentTimeString() const;
};
