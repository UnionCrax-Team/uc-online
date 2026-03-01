/* Intercept calls made to the original steam_api.dll, run our code to spoof
   the appID using init functions, then forward everything else to the original DLL
   and let the game handle the rest. 

   ~veeanti<3 */

#include "uc_online.h"
#include "ini_config.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static UCOnline* g_ucOnlineInstance = NULL;
static bool g_autoInitialized = false;
bool g_deferInitialization = true;
bool g_steamApiInitCalledByUCOnline = false;

static HMODULE g_hOriginalSteamApi = NULL;
static HMODULE g_hSelf = NULL;

#define STEAM_EXPORT(name) FARPROC orig_##name = NULL;
#include "steam_api_exports.inc"
#undef STEAM_EXPORT
FARPROC orig_SteamAPI_Init = NULL;

static void getDllDirectory(HMODULE hModule, char* buffer, size_t bufferSize) {
    char path[MAX_PATH];
    if (GetModuleFileNameA(hModule, path, MAX_PATH) == 0) {
        buffer[0] = '\0';
        return;
    }
    
    char* lastSlash = strrchr(path, '\\');
    if (lastSlash) {
        *lastSlash = '\0';
    }
    
    strncpy(buffer, path, bufferSize - 1);
    buffer[bufferSize - 1] = '\0';
}

static bool initializeSteamApiProxy() {
    char dllDir[260];
    getDllDirectory(g_hSelf, dllDir, sizeof(dllDir));
    
    char configPath[260];
    if (dllDir[0]) {
        snprintf(configPath, sizeof(configPath), "%s\\config.ini", dllDir);
    } else {
        strcpy(configPath, "config.ini");
    }
    
    IniConfig* cfg = IniConfig_Create(configPath);
    const char* configuredPath = IniConfig_GetOriginalDllPath(cfg);
    
    char origDllPath[260];
    if (*configuredPath != '\0') {
        bool isAbsolute = (strlen(configuredPath) >= 2 && configuredPath[1] == ':') ||
                         (*configuredPath == '\\' || *configuredPath == '/');
                         
        if (!isAbsolute && dllDir[0]) {
            snprintf(origDllPath, sizeof(origDllPath), "%s\\%s", dllDir, configuredPath);
        } else {
            strncpy(origDllPath, configuredPath, sizeof(origDllPath) - 1);
            origDllPath[sizeof(origDllPath) - 1] = '\0';
        }
    } else {
        const char* defaultName;
        #ifdef _WIN64
            defaultName = "union-crax64.dll";
        #else
            defaultName = "union-crax.dll";
        #endif
        
        if (dllDir[0]) {
            snprintf(origDllPath, sizeof(origDllPath), "%s\\%s", dllDir, defaultName);
        } else {
            strcpy(origDllPath, defaultName);
        }
    }
    
    IniConfig_Destroy(cfg);
    
    g_hOriginalSteamApi = LoadLibraryA(origDllPath);
    if (!g_hOriginalSteamApi) {
        return false;
    }
    
    #define STEAM_EXPORT(name) orig_##name = GetProcAddress(g_hOriginalSteamApi, #name);
    #include "steam_api_exports.inc"
    #undef STEAM_EXPORT
    orig_SteamAPI_Init = GetProcAddress(g_hOriginalSteamApi, "SteamAPI_Init");
    
    return true;
}

void AutoInitializeUCOnline() {
    if (g_autoInitialized || g_ucOnlineInstance) {
        return;
    }
    
    char dllDir[260];
    getDllDirectory(g_hSelf, dllDir, sizeof(dllDir));
    
    char configPath[260];
    if (dllDir[0]) {
        snprintf(configPath, sizeof(configPath), "%s\\config.ini", dllDir);
    } else {
        strcpy(configPath, "config.ini");
    }
    
    g_ucOnlineInstance = UCOnline_Create(configPath, dllDir);
    if (g_ucOnlineInstance) {
        if (UCOnline_Initialize(g_ucOnlineInstance)) {
            g_autoInitialized = true;
            g_steamApiInitCalledByUCOnline = true;
        } else {
            UCOnline_Destroy(g_ucOnlineInstance);
            g_ucOnlineInstance = NULL;
        }
    }
}

static void autoShutdownUCOnline() {
    if (g_ucOnlineInstance) {
        UCOnline_Shutdown(g_ucOnlineInstance);
        UCOnline_Destroy(g_ucOnlineInstance);
        g_ucOnlineInstance = NULL;
        g_autoInitialized = false;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            g_hSelf = hModule;
            initializeSteamApiProxy();
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            autoShutdownUCOnline();
            if (g_hOriginalSteamApi) {
                FreeLibrary(g_hOriginalSteamApi);
                g_hOriginalSteamApi = NULL;
            }
            break;
    }
    return TRUE;
}
