/* Logger. That's all.
   It should instead show in the msgbox error window if there is one.

   ~veeanti<3 */

#include "logger.h"
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

struct Logger {
    char logFilePath[260];
    bool loggingEnabled;
    bool outputToDebug;  // Output to debug window
    CRITICAL_SECTION lock;
};

static char* getCurrentTimeString() {
    static char timeStr[20];
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_s(&tm_info, &now);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &tm_info);
    return timeStr;
}

Logger* Logger_Create(const char* logFilePath, bool enableLogging) {
    if (!logFilePath) return NULL;
    
    Logger* logger = (Logger*)malloc(sizeof(Logger));
    if (!logger) return NULL;
    
    strncpy(logger->logFilePath, logFilePath, sizeof(logger->logFilePath) - 1);
    logger->logFilePath[sizeof(logger->logFilePath) - 1] = '\0';
    logger->loggingEnabled = enableLogging;
    logger->outputToDebug = true;  // Always output to debug window
    InitializeCriticalSection(&logger->lock);
    
    if (enableLogging) {
        Logger_Log(logger, "Logger initialized");
    }
    
    return logger;
}

void Logger_Destroy(Logger* logger) {
    if (!logger) return;
    DeleteCriticalSection(&logger->lock);
    free(logger);
}

void Logger_SetLoggingEnabled(Logger* logger, bool enabled) {
    if (!logger) return;
    logger->loggingEnabled = enabled;
    Logger_Log(logger, enabled ? "Logging enabled" : "Logging disabled");
}

bool Logger_IsLoggingEnabled(Logger* logger) {
    return logger ? logger->loggingEnabled : false;
}

static void outputToDebugWindow(const char* message) {
    if (message) {
        OutputDebugStringA(message);
        OutputDebugStringA("\n");
    }
}

void Logger_Log(Logger* logger, const char* message) {
    if (!logger || !message) return;
    
    char logMessage[1024];
    snprintf(logMessage, sizeof(logMessage), "%s [INFO] %s", getCurrentTimeString(), message);
    
    // Always output to debug window
    outputToDebugWindow(logMessage);
    
    if (!logger->loggingEnabled) return;
    
    EnterCriticalSection(&logger->lock);
    
    strncat(logMessage, "\n", sizeof(logMessage) - strlen(logMessage) - 1);
    
    FILE* file = fopen(logger->logFilePath, "a");
    if (file) {
        fputs(logMessage, file);
        fclose(file);
    }
    
    LeaveCriticalSection(&logger->lock);
}

void Logger_LogWarning(Logger* logger, const char* message) {
    if (!logger || !message) return;
    
    char logMessage[1024];
    snprintf(logMessage, sizeof(logMessage), "%s [WARNING] %s", getCurrentTimeString(), message);
    
    // Always output to debug window
    outputToDebugWindow(logMessage);
    
    if (!logger->loggingEnabled) return;
    
    EnterCriticalSection(&logger->lock);
    
    strncat(logMessage, "\n", sizeof(logMessage) - strlen(logMessage) - 1);
    
    FILE* file = fopen(logger->logFilePath, "a");
    if (file) {
        fputs(logMessage, file);
        fclose(file);
    }
    
    LeaveCriticalSection(&logger->lock);
}

void Logger_LogError(Logger* logger, const char* message) {
    if (!logger || !message) return;
    
    char logMessage[1024];
    snprintf(logMessage, sizeof(logMessage), "%s [ERROR] %s", getCurrentTimeString(), message);
    
    // Always output to debug window
    outputToDebugWindow(logMessage);
    
    // Show error in message box
    MessageBoxA(NULL, message, "UC-Online Error", MB_OK | MB_ICONERROR);
    
    if (!logger->loggingEnabled) return;
    
    EnterCriticalSection(&logger->lock);
    
    strncat(logMessage, "\n", sizeof(logMessage) - strlen(logMessage) - 1);
    
    FILE* file = fopen(logger->logFilePath, "a");
    if (file) {
        fputs(logMessage, file);
        fclose(file);
    }
    
    LeaveCriticalSection(&logger->lock);
}

void Logger_ClearLog(Logger* logger) {
    if (!logger || !logger->loggingEnabled) return;
    
    EnterCriticalSection(&logger->lock);
    
    FILE* file = fopen(logger->logFilePath, "w");
    if (file) {
        char header[100];
        snprintf(header, sizeof(header), "uc-online Log - %s\n", getCurrentTimeString());
        fputs(header, file);
        fclose(file);
    }
    
    LeaveCriticalSection(&logger->lock);
}
