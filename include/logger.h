#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct Logger Logger;

Logger* Logger_Create(const char* logFilePath, bool enableLogging);
void Logger_Destroy(Logger* logger);
void Logger_SetLoggingEnabled(Logger* logger, bool enabled);
bool Logger_IsLoggingEnabled(Logger* logger);
void Logger_Log(Logger* logger, const char* message);
void Logger_LogWarning(Logger* logger, const char* message);
void Logger_LogError(Logger* logger, const char* message);
void Logger_ClearLog(Logger* logger);
