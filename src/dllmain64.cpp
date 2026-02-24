#include "uc_online64.hpp"
#include "ini_config.hpp"
#include <windows.h>
#include <string>

// Global UCOnline64 instance for auto-initialization when used as steam_api64.dll proxy
static UCOnline64* g_ucOnlineInstance = nullptr;
static bool g_autoInitialized = false;
bool g_deferInitialization = true;
// Set to true once UCOnline64 has internally called SteamAPI_Init on the original DLL.
// proxy_SteamAPI_Init checks this to avoid a double-call.
bool g_steamApiInitCalledByUCOnline = false;

// Handle to the original (renamed) steam_api DLL
static HMODULE g_hOriginalSteamApi = nullptr;

// ============================================================================
// steam_api64.dll proxy - original function pointers
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
// Load the original (renamed) steam_api64.dll and resolve ALL function pointers
// The real steam_api64.dll must be renamed to union-crax64.dll in the game dir.
// ============================================================================

static HMODULE g_hSelf = nullptr;

bool InitializeSteamApiProxy() {
    std::string dllDir = GetDllDirectory(g_hSelf);

    // Check config.ini for a custom original DLL path
    std::string configPath = dllDir.empty() ? "config.ini" : (dllDir + "\\config.ini");
    IniConfig cfg(configPath);
    std::string configuredPath = cfg.GetOriginalDllPath();

    std::string origDllPath;
    if (!configuredPath.empty()) {
        bool isAbsolute = (configuredPath.size() >= 2 && configuredPath[1] == ':') ||
                          (!configuredPath.empty() && (configuredPath[0] == '\\' || configuredPath[0] == '/'));
        origDllPath = (!isAbsolute && !dllDir.empty()) ? (dllDir + "\\" + configuredPath) : configuredPath;
    } else {
        // Default: union-crax64.dll next to this DLL
        origDllPath = dllDir.empty() ? "union-crax64.dll" : (dllDir + "\\union-crax64.dll");
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

    g_ucOnlineInstance = new UCOnline64(configPath, dllDir);
    if (g_ucOnlineInstance) {
        if (g_ucOnlineInstance->InitializeUCOnline()) {
            g_autoInitialized = true;
            // UCOnline64 called SteamAPI_Init internally; tell the proxy stub not to call it again.
            g_steamApiInitCalledByUCOnline = true;
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
// C API Implementation (UCOnline64 functions)
// ============================================================================

extern "C" {
    __declspec(dllexport) void* UCOnline64_Create(const char* iniFilePath) {
        try {
            std::string path = iniFilePath ? iniFilePath : "config.ini";
            return new UCOnline64(path);
        } catch (...) {
            return nullptr;
        }
    }

    __declspec(dllexport) void UCOnline64_Destroy(void* instance) {
        if (instance) {
            UCOnline64* uc = static_cast<UCOnline64*>(instance);
            delete uc;
        }
    }

    __declspec(dllexport) bool UCOnline64_Initialize(void* instance) {
        if (!instance) return false;
        return static_cast<UCOnline64*>(instance)->InitializeUCOnline();
    }

    __declspec(dllexport) void UCOnline64_Shutdown(void* instance) {
        if (!instance) return;
        static_cast<UCOnline64*>(instance)->ShutdownUCOnline();
    }

    __declspec(dllexport) void UCOnline64_SetAppID(void* instance, uint32_t appID) {
        if (!instance) return;
        static_cast<UCOnline64*>(instance)->SetCustomAppID(appID);
    }

    __declspec(dllexport) uint32_t UCOnline64_GetAppID(void* instance) {
        if (!instance) return 0;
        return static_cast<UCOnline64*>(instance)->GetCurrentAppID();
    }

    __declspec(dllexport) bool UCOnline64_IsSteamInitialized(void* instance) {
        if (!instance) return false;
        return static_cast<UCOnline64*>(instance)->IsSteamInitialized();
    }

    __declspec(dllexport) bool UCOnline64_LaunchGame(void* instance) {
        if (!instance) return false;
        return static_cast<UCOnline64*>(instance)->LaunchGame();
    }

    __declspec(dllexport) void UCOnline64_RunCallbacks(void* instance) {
        if (!instance) return;
        static_cast<UCOnline64*>(instance)->RunSteamCallbacks();
    }
}
