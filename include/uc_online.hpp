#pragma once

#include "ini_config.hpp"
#include "logger.hpp"
#include <string>
#include <memory>
#include <windows.h>

class UCOnline {
public:
    UCOnline(const std::string& iniFilePath = "config.ini");
    ~UCOnline();

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

    std::string GetSteamApiDllPath() const;
    void SetSteamApiDllPath(const std::string& dllPath);
    std::string GetSteamOverlayGameId() const;
    void SetSteamOverlayGameId(const std::string& gameId);

private:
    bool _steamInitialized = false;
    uint32_t _currentAppID;
    std::unique_ptr<IniConfig> _config;
    std::unique_ptr<Logger> _logger;
    std::string _gameExecutable;
    std::string _gameArguments;
    std::string _steamApiDllPath;
    std::string _ogAppId;

    bool TryMultipleInitializationMethods();
    void LoadSteamApiDll();
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