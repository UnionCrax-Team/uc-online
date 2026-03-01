/* Get steam_api shit and force it into submission.
   
   ~veeanti<3 */

#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

// Declare original function pointers from the real steam_api.dll
#define STEAM(name) extern FARPROC orig_##name;
#include "steam_api_exports.inc"
#undef STEAM
extern FARPROC orig_SteamAPI_Init;

// UCOnline functions
extern void AutoInitializeUCOnline();
extern bool g_deferInitialization;
extern bool g_steamApiInitCalledByUCOnline;

// Forward declarations for UCOnline API
typedef struct UCOnline UCOnline;
UCOnline* UCOnline_Create(const char* iniFilePath, const char* dllDirectory);
void UCOnline_Destroy(UCOnline* uconline);
bool UCOnline_Initialize(UCOnline* uconline);
void UCOnline_Shutdown(UCOnline* uconline);
void UCOnline_RunCallbacks(UCOnline* uconline);
void UCOnline_SetCustomAppID(UCOnline* uconline, uint32_t appID);
uint32_t UCOnline_GetCurrentAppID(UCOnline* uconline);
bool UCOnline_IsSteamInitialized(UCOnline* uconline);
bool UCOnline_LaunchGame(UCOnline* uconline);
void UCOnline_ClearLog(UCOnline* uconline);
void UCOnline_SetLoggingEnabled(UCOnline* uconline, bool enabled);
bool UCOnline_IsLoggingEnabled(UCOnline* uconline);

// =============================================================================
// TryDeferredInit - deferred initialization export
// =============================================================================
__declspec(dllexport) void TryDeferredInit() {
    if (g_deferInitialization) {
        g_deferInitialization = false;
        AutoInitializeUCOnline();
    }
}

// =============================================================================
// UCOnline-specific exports
// =============================================================================
__declspec(dllexport) UCOnline* UCOnline_Create(const char* iniFilePath, const char* dllDirectory) {
    return UCOnline_Create(iniFilePath, dllDirectory);
}

__declspec(dllexport) void UCOnline_Destroy(UCOnline* uconline) {
    UCOnline_Destroy(uconline);
}

__declspec(dllexport) bool UCOnline_Initialize(UCOnline* uconline) {
    return UCOnline_Initialize(uconline);
}

__declspec(dllexport) void UCOnline_Shutdown(UCOnline* uconline) {
    UCOnline_Shutdown(uconline);
}

__declspec(dllexport) void UCOnline_RunCallbacks(UCOnline* uconline) {
    UCOnline_RunCallbacks(uconline);
}

__declspec(dllexport) void UCOnline_SetCustomAppID(UCOnline* uconline, uint32_t appID) {
    UCOnline_SetCustomAppID(uconline, appID);
}

__declspec(dllexport) uint32_t UCOnline_GetCurrentAppID(UCOnline* uconline) {
    return UCOnline_GetCurrentAppID(uconline);
}

__declspec(dllexport) bool UCOnline_IsSteamInitialized(UCOnline* uconline) {
    return UCOnline_IsSteamInitialized(uconline);
}

__declspec(dllexport) bool UCOnline_LaunchGame(UCOnline* uconline) {
    return UCOnline_LaunchGame(uconline);
}

__declspec(dllexport) void UCOnline_ClearLog(UCOnline* uconline) {
    UCOnline_ClearLog(uconline);
}

__declspec(dllexport) void UCOnline_SetLoggingEnabled(UCOnline* uconline, bool enabled) {
    UCOnline_SetLoggingEnabled(uconline, enabled);
}

__declspec(dllexport) bool UCOnline_IsLoggingEnabled(UCOnline* uconline) {
    return UCOnline_IsLoggingEnabled(uconline);
}

// =============================================================================
// g_pSteamClientGameServer export
// =============================================================================
__declspec(dllexport) void* g_pSteamClientGameServer = NULL;

// =============================================================================
// Typedef for generic 6-param function
// =============================================================================
typedef DWORD_PTR (*GenericFunc6)(DWORD_PTR, DWORD_PTR, DWORD_PTR, DWORD_PTR, DWORD_PTR, DWORD_PTR);

// =============================================================================
// SteamAPI_Init - special handling for deferred init
// =============================================================================
__declspec(dllexport) DWORD_PTR SteamAPI_Init(DWORD_PTR a1, DWORD_PTR a2, DWORD_PTR a3, DWORD_PTR a4, DWORD_PTR a5, DWORD_PTR a6) {
    TryDeferredInit();
    
    if (g_steamApiInitCalledByUCOnline) return 1;
    if (!orig_SteamAPI_Init) return 0;
    
    return ((GenericFunc6)orig_SteamAPI_Init)(a1, a2, a3, a4, a5, a6);
}

// =============================================================================
// Steam API proxy functions - forward to original DLL
// =============================================================================
#define STEAM(name) \
    __declspec(dllexport) DWORD_PTR name(DWORD_PTR a1, DWORD_PTR a2, DWORD_PTR a3, DWORD_PTR a4, DWORD_PTR a5, DWORD_PTR a6) { \
        if (!orig_##name) return 0; \
        return ((GenericFunc6)orig_##name)(a1, a2, a3, a4, a5, a6); \
    }

#include "steam_api_exports.inc"
#undef STEAM
