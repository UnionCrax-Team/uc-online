#include "uc_online.hpp"
#include <windows.h>
#include <string>

// Global UCOnline instance for auto-initialization when used as steam_api.dll proxy
static UCOnline* g_ucOnlineInstance = nullptr;
static bool g_autoInitialized = false;
bool g_deferInitialization = true;

// Handle to the original (renamed) steam_api DLL
static HMODULE g_hOriginalSteamApi = nullptr;

// ============================================================================
// steam_api.dll proxy - original function pointers
// Declared via X-macro over steam_api_exports.inc
// ============================================================================

#define STEAM_EXPORT(name) FARPROC orig_##name = nullptr;
#include "steam_api_exports.inc"
#undef STEAM_EXPORT
// SteamAPI_Init is excluded from the .inc (handled specially in steam_api_stubs.cpp)
FARPROC orig_SteamAPI_Init = nullptr;

// ============================================================================
// Get the directory containing this DLL
// ============================================================================

static std::string GetDllDirectory(HMODULE hModule) {
    char path[MAX_PATH];
    if (GetModuleFileNameA(hModule, path, MAX_PATH) == 0) {
        return "";
    }
    std::string fullPath(path);
    size_t lastSlash = fullPath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        return fullPath.substr(0, lastSlash);
    }
    return "";
}

// ============================================================================
// Load the original (renamed) steam_api.dll and resolve ALL function pointers
// The real steam_api.dll must be renamed to union-crax.dll in the game dir.
// ============================================================================

static HMODULE g_hSelf = nullptr;

bool InitializeSteamApiProxy() {
    std::string dllDir = GetDllDirectory(g_hSelf);
    std::string origDllPath;
    if (!dllDir.empty()) {
        origDllPath = dllDir + "\\union-crax.dll";
    } else {
        origDllPath = "union-crax.dll";
    }

    g_hOriginalSteamApi = LoadLibraryA(origDllPath.c_str());
    if (!g_hOriginalSteamApi) {
        return false;
    }

#define STEAM_EXPORT(name) orig_##name = GetProcAddress(g_hOriginalSteamApi, #name);
#include "steam_api_exports.inc"
#undef STEAM_EXPORT
    // SteamAPI_Init is excluded from the .inc (handled specially in steam_api_stubs.cpp)
    orig_SteamAPI_Init = GetProcAddress(g_hOriginalSteamApi, "SteamAPI_Init");

    return true;
}

// ============================================================================
// Auto-initialization
// ============================================================================

void AutoInitializeUCOnline() {
    if (g_autoInitialized || g_ucOnlineInstance) {
        return;
    }

    // Resolve config.ini relative to the DLL's own directory
    std::string dllDir = GetDllDirectory(g_hSelf);
    std::string configPath = dllDir.empty() ? "config.ini" : (dllDir + "\\config.ini");

    g_ucOnlineInstance = new UCOnline(configPath, dllDir);
    if (g_ucOnlineInstance) {
        if (g_ucOnlineInstance->InitializeUCOnline()) {
            g_autoInitialized = true;
        } else {
            delete g_ucOnlineInstance;
            g_ucOnlineInstance = nullptr;
        }
    }
}

void AutoShutdownUCOnline() {
    if (g_ucOnlineInstance) {
        g_ucOnlineInstance->ShutdownUCOnline();
        delete g_ucOnlineInstance;
        g_ucOnlineInstance = nullptr;
        g_autoInitialized = false;
    }
}

// ============================================================================
// DLL Entry Point
// ============================================================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        g_hSelf = hModule;
        InitializeSteamApiProxy();
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        AutoShutdownUCOnline();
        if (g_hOriginalSteamApi) {
            FreeLibrary(g_hOriginalSteamApi);
            g_hOriginalSteamApi = nullptr;
        }
        break;
    }
    return TRUE;
}

// ============================================================================
// C API Implementation (UCOnline functions)
// ============================================================================

extern "C" {
    __declspec(dllexport) void* UCOnline_Create(const char* iniFilePath) {
        try {
            std::string path = iniFilePath ? iniFilePath : "config.ini";
            return new UCOnline(path);
        } catch (...) {
            return nullptr;
        }
    }

    __declspec(dllexport) void UCOnline_Destroy(void* instance) {
        if (instance) {
            UCOnline* uc = static_cast<UCOnline*>(instance);
            delete uc;
        }
    }

    __declspec(dllexport) bool UCOnline_Initialize(void* instance) {
        if (!instance) return false;
        return static_cast<UCOnline*>(instance)->InitializeUCOnline();
    }

    __declspec(dllexport) void UCOnline_Shutdown(void* instance) {
        if (!instance) return;
        static_cast<UCOnline*>(instance)->ShutdownUCOnline();
    }

    __declspec(dllexport) void UCOnline_SetAppID(void* instance, uint32_t appID) {
        if (!instance) return;
        static_cast<UCOnline*>(instance)->SetCustomAppID(appID);
    }

    __declspec(dllexport) uint32_t UCOnline_GetAppID(void* instance) {
        if (!instance) return 0;
        return static_cast<UCOnline*>(instance)->GetCurrentAppID();
    }

    __declspec(dllexport) bool UCOnline_IsSteamInitialized(void* instance) {
        if (!instance) return false;
        return static_cast<UCOnline*>(instance)->IsSteamInitialized();
    }

    __declspec(dllexport) bool UCOnline_LaunchGame(void* instance) {
        if (!instance) return false;
        return static_cast<UCOnline*>(instance)->LaunchGame();
    }

    __declspec(dllexport) void UCOnline_RunCallbacks(void* instance) {
        if (!instance) return;
        static_cast<UCOnline*>(instance)->RunSteamCallbacks();
    }
}
