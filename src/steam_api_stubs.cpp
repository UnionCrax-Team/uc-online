// steam_api_stubs.cpp - Proxy stub functions for steam_api.dll / steam_api64.dll exports
// Each function forwards to the original (renamed) steam_api DLL loaded at runtime.
// The .def file maps the real export names to these proxy_ prefixed functions.

#include <windows.h>

// ============================================================================
// Original function pointer externs (defined in dllmain.cpp / dllmain64.cpp)
// ============================================================================

#define STEAM_EXPORT(name) extern FARPROC orig_##name;
#include "steam_api_exports.inc"
#undef STEAM_EXPORT

// SteamAPI_Init is excluded from the .inc (handled specially below)
extern FARPROC orig_SteamAPI_Init;

// ============================================================================
// Deferred init trigger (declared in dllmain.cpp / dllmain64.cpp)
// ============================================================================

extern void AutoInitializeUCOnline();
extern bool g_deferInitialization;

static inline void TryDeferredInit() {
    if (g_deferInitialization) {
        g_deferInitialization = false;
        AutoInitializeUCOnline();
    }
}

// ============================================================================
// Generic forwarder - handles up to 6 pointer-sized args
// ============================================================================

typedef DWORD_PTR (*GenericFunc6)(DWORD_PTR, DWORD_PTR, DWORD_PTR, DWORD_PTR, DWORD_PTR, DWORD_PTR);

// ============================================================================
// SteamAPI_Init - intercept to trigger UCOnline initialization
// ============================================================================

extern "C" DWORD_PTR proxy_SteamAPI_Init(DWORD_PTR a1, DWORD_PTR a2, DWORD_PTR a3, DWORD_PTR a4, DWORD_PTR a5, DWORD_PTR a6) {
    TryDeferredInit();
    if (!orig_SteamAPI_Init) return 0;
    return ((GenericFunc6)orig_SteamAPI_Init)(a1, a2, a3, a4, a5, a6);
}

// ============================================================================
// g_pSteamClientGameServer - data export (pointer variable), not a function
// We export a null pointer; the real value comes from the original DLL.
// ============================================================================
extern "C" void* proxy_g_pSteamClientGameServer = nullptr;

// ============================================================================
// All other proxy functions - generated via X-macro over steam_api_exports.inc
// (SteamAPI_Init is excluded from the .inc and defined above)
// ============================================================================

#define STEAM_EXPORT(name) \
    extern "C" DWORD_PTR proxy_##name(DWORD_PTR a1, DWORD_PTR a2, DWORD_PTR a3, DWORD_PTR a4, DWORD_PTR a5, DWORD_PTR a6) { \
        if (!orig_##name) return 0; \
        return ((GenericFunc6)orig_##name)(a1, a2, a3, a4, a5, a6); \
    }

#include "steam_api_exports.inc"
#undef STEAM_EXPORT
