/* The meat and potatoes of our whole operation. Rewritten once again in C.
   This was to make it compilable in TCC (tiny C compiler) and prevent any necessary
   dependencies being required like VC Runtime and then GCC redists from compiling using
   MSVC and GCC (MSYS2 / MingW64) respectively.

   ~veeanti<3 */

#include "uc_online.h"
#include "ini_config.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

struct UCOnline {
    char dllDirectory[260];
    uint32_t currentAppID;
    bool steamInitialized;
    HMODULE steamApiModule;
    IniConfig* config;
    Logger* logger;
    
    SteamAPI_Init_t SteamAPI_Init;
    SteamAPI_InitFlat_t SteamAPI_InitFlat;
    SteamAPI_Shutdown_t SteamAPI_Shutdown;
    SteamAPI_RunCallbacks_t SteamAPI_RunCallbacks;
    SteamAPI_RestartAppIfNecessary_t SteamAPI_RestartAppIfNecessary;
    SteamClient_t SteamClient;
    SteamApps_t SteamApps;
    GetHSteamPipe_t GetHSteamPipe;
};

// Default AppID (Spacewar)
#define DEFAULT_APP_ID 480

// Possible DLL names to search for
static const char* dllNames[] = {
    "steam_api.dll",
    "steam_api64.dll", 
    "union-crax.dll",
    "union-crax64.dll",
    "steam_api_orig.dll",
    "steam_api64_orig.dll",
    "original_steam_api.dll",
    "original_steam_api64.dll"
};

// Search for DLL recursively in directory and subdirectories
static bool findSteamDllRecursive(const char* directory, char* foundPath, size_t pathSize) {
    WIN32_FIND_DATAA findData;
    char searchPath[520];
    char fullPath[520];
    
    snprintf(searchPath, sizeof(searchPath), "%s\\*", directory);
    
    HANDLE hFind = FindFirstFileA(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    do {
        if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0) {
            continue;
        }
        
        snprintf(fullPath, sizeof(fullPath), "%s\\%s", directory, findData.cFileName);
        
        // Check if it's a DLL
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Recursively search subdirectory
            if (findSteamDllRecursive(fullPath, foundPath, pathSize)) {
                FindClose(hFind);
                return true;
            }
        } else {
            // Check if it's one of our target DLLs
            for (int i = 0; i < sizeof(dllNames) / sizeof(dllNames[0]); i++) {
                if (_stricmp(findData.cFileName, dllNames[i]) == 0) {
                    strncpy(foundPath, fullPath, pathSize - 1);
                    foundPath[pathSize - 1] = '\0';
                    FindClose(hFind);
                    return true;
                }
            }
        }
    } while (FindNextFileA(hFind, &findData));
    
    FindClose(hFind);
    return false;
}

// Auto-detect the original Steam DLL
static bool autoDetectSteamDll(UCOnline* uconline, char* dllPath, size_t pathSize) {
    char searchDir[260];
    
    // First check the DLL directory
    if (uconline->dllDirectory[0]) {
        strncpy(searchDir, uconline->dllDirectory, sizeof(searchDir) - 1);
    } else {
        // Get current directory
        GetCurrentDirectoryA(sizeof(searchDir), searchDir);
    }
    
    char logMessage[512];
    
    // Check in the same directory first (non-recursive)
    for (int i = 0; i < sizeof(dllNames) / sizeof(dllNames[0]); i++) {
        snprintf(dllPath, pathSize, "%s\\%s", searchDir, dllNames[i]);
        if (GetFileAttributesA(dllPath) != INVALID_FILE_ATTRIBUTES) {
            snprintf(logMessage, sizeof(logMessage), "Found steam DLL: %s", dllPath);
            Logger_Log(uconline->logger, logMessage);
            return true;
        }
    }
    
    // Recursively search subdirectories
    snprintf(logMessage, sizeof(logMessage), "Searching for steam DLL in: %s", searchDir);
    Logger_Log(uconline->logger, logMessage);
    
    if (findSteamDllRecursive(searchDir, dllPath, pathSize)) {
        snprintf(logMessage, sizeof(logMessage), "Found steam DLL recursively: %s", dllPath);
        Logger_Log(uconline->logger, logMessage);
        return true;
    }
    
    // Also check parent directory
    char* lastBackslash = strrchr(searchDir, '\\');
    if (lastBackslash && lastBackslash > searchDir) {
        *lastBackslash = '\0';
        
        for (int i = 0; i < sizeof(dllNames) / sizeof(dllNames[0]); i++) {
            snprintf(dllPath, pathSize, "%s\\%s", searchDir, dllNames[i]);
            if (GetFileAttributesA(dllPath) != INVALID_FILE_ATTRIBUTES) {
                snprintf(logMessage, sizeof(logMessage), "Found steam DLL in parent: %s", dllPath);
                Logger_Log(uconline->logger, logMessage);
                return true;
            }
        }
    }
    
    return false;
}

// Auto-detect AppID from game executable
static uint32_t autoDetectAppId(UCOnline* uconline) {
    char exePath[260];
    char appIdStr[32];
    FILE* file;
    
    // Try to read from steam_appid.txt in DLL directory
    if (uconline->dllDirectory[0]) {
        snprintf(exePath, sizeof(exePath), "%s\\steam_appid.txt", uconline->dllDirectory);
    } else {
        strncpy(exePath, "steam_appid.txt", sizeof(exePath) - 1);
    }
    
    file = fopen(exePath, "r");
    if (file) {
        if (fgets(appIdStr, sizeof(appIdStr), file)) {
            // Remove whitespace
            char* p = appIdStr;
            while (*p) {
                if (*p == '\n' || *p == '\r' || *p == ' ') {
                    *p = '\0';
                    break;
                }
                p++;
            }
            uint32_t appId = (uint32_t)strtoul(appIdStr, NULL, 10);
            if (appId > 0) {
                char logMessage[512];
                snprintf(logMessage, sizeof(logMessage), "Detected AppID from steam_appid.txt: %u", appId);
                Logger_Log(uconline->logger, logMessage);
                fclose(file);
                return appId;
            }
        }
        fclose(file);
    }
    
    // Default to Spacewar
    return DEFAULT_APP_ID;
}

static void createAppIdFile(UCOnline* uconline, uint32_t appId) {
    char appIdFilePath[260];
    
    if (uconline->dllDirectory[0]) {
        snprintf(appIdFilePath, sizeof(appIdFilePath), "%s\\steam_appid.txt", uconline->dllDirectory);
    } else {
        strncpy(appIdFilePath, "steam_appid.txt", sizeof(appIdFilePath) - 1);
    }
    
    FILE* file = fopen(appIdFilePath, "w");
    if (file) {
        fprintf(file, "%u", appId);
        fclose(file);
        
        char logMessage[512];
        snprintf(logMessage, sizeof(logMessage), "Wrote steam_appid.txt with appid: %u", appId);
        Logger_Log(uconline->logger, logMessage);
    } else {
        char logMessage[512];
        snprintf(logMessage, sizeof(logMessage), "Failed to create steam_appid.txt");
        Logger_LogWarning(uconline->logger, logMessage);
    }
}

static void loadSteamApiDll(UCOnline* uconline) {
    char dllPath[520];
    char logMessage[512];
    
    // First try to use configured path
    const char* configuredPath = IniConfig_GetOriginalDllPath(uconline->config);
    
    if (*configuredPath != '\0') {
        bool isAbsolute = (strlen(configuredPath) >= 2 && configuredPath[1] == ':') ||
                         (*configuredPath == '\\' || *configuredPath == '/');
                          
        if (!isAbsolute && uconline->dllDirectory[0]) {
            snprintf(dllPath, sizeof(dllPath), "%s\\%s", uconline->dllDirectory, configuredPath);
        } else {
            strncpy(dllPath, configuredPath, sizeof(dllPath) - 1);
            dllPath[sizeof(dllPath) - 1] = '\0';
        }
        
        snprintf(logMessage, sizeof(logMessage), "Loading steam DLL from configured path: %s", dllPath);
        Logger_Log(uconline->logger, logMessage);
        
        uconline->steamApiModule = LoadLibraryA(dllPath);
        
        if (uconline->steamApiModule) {
            goto dll_loaded;
        }
    }
    
    // Auto-detect the DLL
    snprintf(logMessage, sizeof(logMessage), "Auto-detecting steam DLL...");
    Logger_Log(uconline->logger, logMessage);
    
    if (autoDetectSteamDll(uconline, dllPath, sizeof(dllPath))) {
        uconline->steamApiModule = LoadLibraryA(dllPath);
        
        if (uconline->steamApiModule) {
            goto dll_loaded;
        }
    }
    
    // Last resort: try default names
    const char* defaultDllName = sizeof(void*) == 8 ? "union-crax64.dll" : "union-crax.dll";
    
    if (uconline->dllDirectory[0]) {
        snprintf(dllPath, sizeof(dllPath), "%s\\%s", uconline->dllDirectory, defaultDllName);
    } else {
        strncpy(dllPath, defaultDllName, sizeof(dllPath) - 1);
        dllPath[sizeof(dllPath) - 1] = '\0';
    }
    
    snprintf(logMessage, sizeof(logMessage), "Trying default DLL path: %s", dllPath);
    Logger_Log(uconline->logger, logMessage);
    
    uconline->steamApiModule = LoadLibraryA(dllPath);
    
    if (!uconline->steamApiModule && uconline->dllDirectory[0]) {
        // Try current directory
        const char* filename = strrchr(dllPath, '\\');
        if (filename) filename++;
        else filename = dllPath;
        
        uconline->steamApiModule = LoadLibraryA(filename);
    }
    
dll_loaded:
    if (uconline->steamApiModule) {
        Logger_Log(uconline->logger, "Successfully loaded original steam_api.dll");
        
        uconline->SteamAPI_Init = (SteamAPI_Init_t)GetProcAddress(uconline->steamApiModule, "SteamAPI_Init");
        uconline->SteamAPI_InitFlat = (SteamAPI_InitFlat_t)GetProcAddress(uconline->steamApiModule, "SteamAPI_InitFlat");
        uconline->SteamAPI_Shutdown = (SteamAPI_Shutdown_t)GetProcAddress(uconline->steamApiModule, "SteamAPI_Shutdown");
        uconline->SteamAPI_RunCallbacks = (SteamAPI_RunCallbacks_t)GetProcAddress(uconline->steamApiModule, "SteamAPI_RunCallbacks");
        uconline->SteamAPI_RestartAppIfNecessary = (SteamAPI_RestartAppIfNecessary_t)GetProcAddress(uconline->steamApiModule, "SteamAPI_RestartAppIfNecessary");
        uconline->SteamClient = (SteamClient_t)GetProcAddress(uconline->steamApiModule, "SteamClient");
        uconline->SteamApps = (SteamApps_t)GetProcAddress(uconline->steamApiModule, "SteamApps");
        uconline->GetHSteamPipe = (GetHSteamPipe_t)GetProcAddress(uconline->steamApiModule, "GetHSteamPipe");
    } else {
        snprintf(logMessage, sizeof(logMessage), "Failed to load original steam_api.dll. Place it in the game directory or subfolder.");
        Logger_LogError(uconline->logger, logMessage);
    }
}

static bool tryMultipleInitializationMethods(UCOnline* uconline) {
    createAppIdFile(uconline, uconline->currentAppID);
    
    if (!uconline->SteamAPI_Init) return false;
    
    char errorMsg[1024] = {0};
    bool result = uconline->SteamAPI_Init(errorMsg);
    
    if (result) {
        Logger_Log(uconline->logger, "SteamAPI_Init succeeded");
        return true;
    } else {
        char logMessage[512];
        snprintf(logMessage, sizeof(logMessage), "SteamAPI_Init failed: %s", errorMsg);
        Logger_Log(uconline->logger, logMessage);
        
        if (uconline->SteamAPI_InitFlat) {
            createAppIdFile(uconline, uconline->currentAppID);
            
            char errorMsgFlat[1024] = {0};
            bool resultFlat = uconline->SteamAPI_InitFlat(errorMsgFlat);
            
            if (resultFlat) {
                Logger_Log(uconline->logger, "SteamAPI_InitFlat succeeded");
                return true;
            } else {
                char logMessageFlat[512];
                snprintf(logMessageFlat, sizeof(logMessageFlat), "SteamAPI_InitFlat failed: %s", errorMsgFlat);
                Logger_Log(uconline->logger, logMessageFlat);
                
                if (uconline->SteamAPI_RestartAppIfNecessary && uconline->SteamAPI_RestartAppIfNecessary(uconline->currentAppID)) {
                    Logger_Log(uconline->logger, "Steam requested app restart");
                    return false;
                }
                
                return false;
            }
        }
    }
    
    return false;
}

static bool initializeSteamInterfaces(UCOnline* uconline) {
    if (uconline->SteamClient) {
        void* steamClient = uconline->SteamClient();
        if (!steamClient) {
            Logger_LogError(uconline->logger, "Failed to get SteamClient");
            return false;
        }
    }
    
    if (uconline->GetHSteamPipe) {
        void* hSteamPipe = uconline->GetHSteamPipe();
        if (!hSteamPipe) {
            Logger_LogError(uconline->logger, "Failed to get HSteamPipe");
            return false;
        }
    }
    
    if (uconline->SteamApps) {
        void* steamApps = uconline->SteamApps();
        if (!steamApps) {
            Logger_LogWarning(uconline->logger, "SteamApps interface not available or cannot be found");
        } else {
            Logger_Log(uconline->logger, "Successfully obtained SteamApps interface");
        }
    }
    
    return true;
}

UCOnline* UCOnline_Create(const char* iniFilePath, const char* dllDirectory) {
    UCOnline* uconline = (UCOnline*)malloc(sizeof(UCOnline));
    if (!uconline) return NULL;
    
    memset(uconline, 0, sizeof(UCOnline));
    
    if (dllDirectory) {
        strncpy(uconline->dllDirectory, dllDirectory, sizeof(uconline->dllDirectory) - 1);
        uconline->dllDirectory[sizeof(uconline->dllDirectory) - 1] = '\0';
    }
    
    // Try to load config.ini if it exists, otherwise use defaults
    const char* configPath = iniFilePath ? iniFilePath : "config.ini";
    
    // IniConfig_Create will create defaults if file doesn't exist
    uconline->config = IniConfig_Create(configPath);
    if (!uconline->config) {
        uconline->config = IniConfig_Create(NULL);  // Fallback to memory-only config
    }
    
    // Auto-detect AppID (no config required)
    uconline->currentAppID = autoDetectAppId(uconline);
    
    // Create logger (always enabled by default for debugging)
    uconline->logger = Logger_Create("uc_online.log", true);
    
    char logMessage[512];
    snprintf(logMessage, sizeof(logMessage), "uc-online initialized with appid: %u", uconline->currentAppID);
    Logger_Log(uconline->logger, logMessage);
    
    if (uconline->dllDirectory[0]) {
        char dirMessage[512];
        snprintf(dirMessage, sizeof(dirMessage), "DLL directory: %s", uconline->dllDirectory);
        Logger_Log(uconline->logger, dirMessage);
    } else {
        Logger_Log(uconline->logger, "DLL directory: current directory");
    }
    
    return uconline;
}

void UCOnline_Destroy(UCOnline* uconline) {
    if (!uconline) return;
    
    Logger_Log(uconline->logger, "uc-online shutting down");
    UCOnline_Shutdown(uconline);
    
    Logger_Destroy(uconline->logger);
    IniConfig_Destroy(uconline->config);
    
    if (uconline->steamApiModule) {
        FreeLibrary(uconline->steamApiModule);
    }
    
    free(uconline);
}

bool UCOnline_Initialize(UCOnline* uconline) {
    if (!uconline) return false;
    
    // Re-detect AppID each time (in case it changed)
    uconline->currentAppID = autoDetectAppId(uconline);
    
    char logMessage[512];
    snprintf(logMessage, sizeof(logMessage), "Initializing Steam with appid: %u", uconline->currentAppID);
    Logger_Log(uconline->logger, logMessage);
    
    // Load the DLL
    loadSteamApiDll(uconline);
    
    bool result = tryMultipleInitializationMethods(uconline);
    
    if (!result) {
        return false;
    }
    
    uconline->steamInitialized = true;
    Logger_Log(uconline->logger, "Steam initialized successfully");
    
    if (initializeSteamInterfaces(uconline)) {
        Logger_Log(uconline->logger, "Steam interfaces accessible");
    } else {
        Logger_LogWarning(uconline->logger, "Steam interfaces not accessible");
    }
    
    return true;
}

void UCOnline_Shutdown(UCOnline* uconline) {
    if (!uconline || !uconline->steamInitialized) return;
    
    Logger_Log(uconline->logger, "Shutting down...");
    
    if (uconline->SteamAPI_Shutdown) {
        uconline->SteamAPI_Shutdown();
    }
    
    uconline->steamInitialized = false;
    Logger_Log(uconline->logger, "Shutdown complete");
}

void UCOnline_RunCallbacks(UCOnline* uconline) {
    if (!uconline || !uconline->steamInitialized || !uconline->SteamAPI_RunCallbacks) {
        return;
    }
    
    uconline->SteamAPI_RunCallbacks();
}

void UCOnline_SetCustomAppID(UCOnline* uconline, uint32_t appID) {
    if (!uconline) return;
    
    uconline->currentAppID = appID;
    
    char logMessage[512];
    snprintf(logMessage, sizeof(logMessage), "Appid changed to: %u", appID);
    Logger_Log(uconline->logger, logMessage);
    
    if (uconline->steamInitialized) {
        Logger_Log(uconline->logger, "Reinitializing Steam with new appid");
        UCOnline_Shutdown(uconline);
        UCOnline_Initialize(uconline);
    }
}

uint32_t UCOnline_GetCurrentAppID(UCOnline* uconline) {
    return uconline ? uconline->currentAppID : 0;
}

bool UCOnline_IsSteamInitialized(UCOnline* uconline) {
    return uconline ? uconline->steamInitialized : false;
}

bool UCOnline_LaunchGame(UCOnline* uconline) {
    // Steam API doesn't provide a programmatic way to launch games.
    // Games should be launched through the Steam client.
    return false;
}

void UCOnline_ClearLog(UCOnline* uconline) {
    if (!uconline) return;
    Logger_ClearLog(uconline->logger);
}

void UCOnline_SetLoggingEnabled(UCOnline* uconline, bool enabled) {
    if (!uconline) return;
    Logger_SetLoggingEnabled(uconline->logger, enabled);
    Logger_Log(uconline->logger, enabled ? "Logging enabled" : "Logging disabled");
}

bool UCOnline_IsLoggingEnabled(UCOnline* uconline) {
    return uconline ? Logger_IsLoggingEnabled(uconline->logger) : false;
}
