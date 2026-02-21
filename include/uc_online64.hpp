#pragma once

#include "ini_config.hpp"
#include "logger.hpp"
#include <string>
#include <memory>
#include <windows.h>

// DLL Export/Import macros
#ifdef UC_ONLINE_EXPORTS
#define UC_ONLINE_API __declspec(dllexport)
#else
#define UC_ONLINE_API __declspec(dllimport)
#endif

// C API for external use
extern "C" {
    UC_ONLINE_API void* UCOnline64_Create(const char* iniFilePath);
    UC_ONLINE_API void UCOnline64_Destroy(void* instance);
    UC_ONLINE_API bool UCOnline64_Initialize(void* instance);
    UC_ONLINE_API void UCOnline64_Shutdown(void* instance);
    UC_ONLINE_API void UCOnline64_SetAppID(void* instance, uint32_t appID);
    UC_ONLINE_API uint32_t UCOnline64_GetAppID(void* instance);
    UC_ONLINE_API bool UCOnline64_IsSteamInitialized(void* instance);
    UC_ONLINE_API bool UCOnline64_LaunchGame(void* instance);
    UC_ONLINE_API void UCOnline64_SetGameExecutable(void* instance, const char* exePath);
    UC_ONLINE_API void UCOnline64_SetGameArguments(void* instance, const char* arguments);
    UC_ONLINE_API const char* UCOnline64_GetGameExecutable(void* instance);
    UC_ONLINE_API const char* UCOnline64_GetGameArguments(void* instance);
    UC_ONLINE_API void UCOnline64_RunCallbacks(void* instance);
}

class UC_ONLINE_API UCOnline64 {
public:
    UCOnline64(const std::string& iniFilePath = "config.ini", const std::string& dllDirectory = "");
    ~UCOnline64();

    bool InitializeUCOnline();
    void ShutdownUCOnline();
    void RunSteamCallbacks();
    void SetCustomAppID(uint32_t appID);
    uint32_t GetCurrentAppID() const;
    bool IsSteamInitialized() const;

    void CreateAppIdFile();
    bool LaunchGame();
    void SetGameExecutable(const std::string& gameExePath);
    void SetGameArguments(const std::string& arguments);
    std::string GetGameExecutable() const;
    std::string GetGameArguments() const;
    void SaveConfig();
    void ReloadConfig();

    Logger* GetLogger();
    void SetLoggingEnabled(bool enabled);
    bool IsLoggingEnabled() const;
    void ClearLog();

private:
    bool _steamInitialized = false;
    uint32_t _currentAppID;
    std::unique_ptr<IniConfig> _config;
    std::unique_ptr<Logger> _logger;
    std::string _gameExecutable;
    std::string _gameArguments;
    std::string _dllDirectory;

    bool TryMultipleInitializationMethods();
    void LoadSteamApi64Dll();
    bool InitializeSteamInterfaces();

    // Steam API function pointers
    typedef bool (*SteamAPI_Init_t)(char* errorMessage);
    typedef bool (*SteamAPI_InitFlat_t)(char* errorMessage);
    typedef void (*SteamAPI_Shutdown_t)();
    typedef void (*SteamAPI_RunCallbacks_t)();
    typedef bool (*SteamAPI_RestartAppIfNecessary_t)(uint32_t appId);
    typedef void* (*SteamClient_t)();
    typedef void* (*SteamApps_t)();
    typedef void* (*GetHSteamPipe_t)();

    SteamAPI_Init_t SteamAPI_Init = nullptr;
    SteamAPI_InitFlat_t SteamAPI_InitFlat = nullptr;
    SteamAPI_Shutdown_t SteamAPI_Shutdown = nullptr;
    SteamAPI_RunCallbacks_t SteamAPI_RunCallbacks = nullptr;
    SteamAPI_RestartAppIfNecessary_t SteamAPI_RestartAppIfNecessary = nullptr;
    SteamClient_t SteamClient = nullptr;
    SteamApps_t SteamApps = nullptr;
    GetHSteamPipe_t GetHSteamPipe = nullptr;

    HMODULE _steamApiModule = nullptr;
};