#include "uc_online64.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

UCOnline64::UCOnline64(const std::string& iniFilePath, const std::string& dllDirectory) {
    _dllDirectory = dllDirectory;
    _config = std::make_unique<IniConfig>(iniFilePath);
    _currentAppID = _config->GetAppID();
    if (_currentAppID == 0) _currentAppID = 480;

    std::string logFile = _config->GetValue("Logging", "LogFile", "uc_online.log");
    bool enableLogging = _config->GetEnableLogging();
    _logger = std::make_unique<Logger>(logFile, enableLogging);

    _logger->Log("uc-online64 initialized with appid: " + std::to_string(_currentAppID));
    _logger->Log("DLL directory: " + (_dllDirectory.empty() ? "current directory" : _dllDirectory));
}

UCOnline64::~UCOnline64() {
    _logger->Log("uc-online64 shutting down");
    ShutdownUCOnline();
}

bool UCOnline64::InitializeUCOnline() {
    try {
        _currentAppID = _config->GetAppID();
        if (_currentAppID == 0) _currentAppID = 480;

        _logger->Log("Initializing Steam with appid: " + std::to_string(_currentAppID));
        CreateAppIdFile();

        LoadSteamApi64Dll();

        bool result = TryMultipleInitializationMethods();

        if (!result) {
            return false;
        }

        _steamInitialized = true;
        _logger->Log("Steam initialized successfully");

        if (InitializeSteamInterfaces()) {
            _logger->Log("Steam interfaces accessible");
        } else {
            _logger->LogWarning("Steam interfaces not accessible");
        }

        return true;
    } catch (const std::exception& ex) {
        std::cout << "Exception during Steam initialization: " << ex.what() << std::endl;
        return false;
    }
}

void UCOnline64::ShutdownUCOnline() {
    if (_steamInitialized) {
        _logger->Log("Shutting down...");
        if (SteamAPI_Shutdown) SteamAPI_Shutdown();
        _steamInitialized = false;
        _logger->Log("Shutdown complete");
    }
}

void UCOnline64::RunSteamCallbacks() {
    if (_steamInitialized && SteamAPI_RunCallbacks) {
        SteamAPI_RunCallbacks();
    }
}

void UCOnline64::SetCustomAppID(uint32_t appID) {
    _currentAppID = appID;
    _config->SetAppID(appID);
    _config->SaveConfig();
    _logger->Log("Appid changed to: " + std::to_string(appID));

    if (_steamInitialized) {
        _logger->Log("Reinitializing Steam with new appid");
        ShutdownUCOnline();
        InitializeUCOnline();
    }
}

uint32_t UCOnline64::GetCurrentAppID() const {
    return _currentAppID;
}

bool UCOnline64::IsSteamInitialized() const {
    return _steamInitialized;
}

void UCOnline64::CreateAppIdFile() {
    try {
        // Use the configured SteamAppIdFile path; resolve relative to DLL directory
        std::string appIdFileName = _config->GetSteamAppIdFile();
        if (appIdFileName.empty()) appIdFileName = "steam_appid.txt";

        std::string appIdFilePath;
        if (!_dllDirectory.empty() &&
            appIdFileName.find(':') == std::string::npos &&   // not absolute
            appIdFileName.find('\\') == std::string::npos &&
            appIdFileName.find('/') == std::string::npos) {
            appIdFilePath = _dllDirectory + "\\" + appIdFileName;
        } else {
            appIdFilePath = appIdFileName;
        }

        std::ofstream file(appIdFilePath);
        if (file.is_open()) {
            file << _currentAppID;
            _logger->Log("Wrote " + appIdFilePath + " with appid: " + std::to_string(_currentAppID));
        } else {
            _logger->LogWarning("Failed to create " + appIdFilePath);
        }
    } catch (const std::exception& ex) {
        _logger->LogException(ex, "Failed to create steam_appid.txt");
    }
}

bool UCOnline64::LaunchGame() {
    _logger->Log("LaunchGame() called - functionality not yet implemented");
    // TODO: Implement game launching functionality
    return false;
}

void UCOnline64::LoadSteamApi64Dll() {
    try {
        // Determine the original DLL name/path:
        //   1. Use OriginalDllPath from config if non-empty.
        //   2. Otherwise fall back to "union-crax64.dll" in the DLL directory.
        std::string configuredPath = _config->GetOriginalDllPath();

        std::string dllPath;
        if (!configuredPath.empty()) {
            // If the configured path is relative, resolve it against the DLL directory
            bool isAbsolute = (configuredPath.size() >= 2 && configuredPath[1] == ':') ||
                              (!configuredPath.empty() && (configuredPath[0] == '\\' || configuredPath[0] == '/'));
            if (!isAbsolute && !_dllDirectory.empty()) {
                dllPath = _dllDirectory + "\\" + configuredPath;
            } else {
                dllPath = configuredPath;
            }
            _logger->Log("Loading original steam_api64.dll from configured path: " + dllPath);
        } else {
            // Default: union-crax64.dll next to this DLL
            std::string origDllName = "union-crax64.dll";
            if (!_dllDirectory.empty()) {
                dllPath = _dllDirectory + "\\" + origDllName;
            } else {
                dllPath = origDllName;
            }
            _logger->Log("Loading original steam_api64.dll (default): " + dllPath);
        }

        _steamApiModule = LoadLibraryA(dllPath.c_str());

        if (!_steamApiModule && !_dllDirectory.empty()) {
            // Fallback: try current directory with just the filename
            std::string fallback = dllPath.substr(dllPath.find_last_of("\\/") + 1);
            _logger->LogWarning("Failed to load from " + dllPath + ", trying current directory: " + fallback);
            _steamApiModule = LoadLibraryA(fallback.c_str());
        }

        if (_steamApiModule) {
            _logger->Log("Successfully loaded original steam_api64.dll");
            SteamAPI_Init = (SteamAPI_Init_t)GetProcAddress(_steamApiModule, "SteamAPI_Init");
            SteamAPI_InitFlat = (SteamAPI_InitFlat_t)GetProcAddress(_steamApiModule, "SteamAPI_InitFlat");
            SteamAPI_Shutdown = (SteamAPI_Shutdown_t)GetProcAddress(_steamApiModule, "SteamAPI_Shutdown");
            SteamAPI_RunCallbacks = (SteamAPI_RunCallbacks_t)GetProcAddress(_steamApiModule, "SteamAPI_RunCallbacks");
            SteamAPI_RestartAppIfNecessary = (SteamAPI_RestartAppIfNecessary_t)GetProcAddress(_steamApiModule, "SteamAPI_RestartAppIfNecessary");
            SteamClient = (SteamClient_t)GetProcAddress(_steamApiModule, "SteamClient");
            SteamApps = (SteamApps_t)GetProcAddress(_steamApiModule, "SteamApps");
            GetHSteamPipe = (GetHSteamPipe_t)GetProcAddress(_steamApiModule, "GetHSteamPipe");
        } else {
            _logger->LogError("Failed to load original steam_api64.dll from: " + dllPath +
                              ". Set OriginalDllPath in config.ini or rename the original to union-crax64.dll.");
        }
    } catch (const std::exception& ex) {
        _logger->LogException(ex, "Error loading original steam_api64.dll");
    }
}

bool UCOnline64::TryMultipleInitializationMethods() {
    // Write steam_appid.txt with the configured AppID immediately before calling
    // SteamAPI_Init so that Steam picks up the correct AppID from the file.
    CreateAppIdFile();

    if (!SteamAPI_Init) return false;

    char errorMsg[1024] = {0};
    bool result = SteamAPI_Init(errorMsg);

    if (result) {
        _logger->Log("SteamAPI_Init succeeded");
        return true;
    } else {
        _logger->Log("SteamAPI_Init failed: " + std::string(errorMsg));

        if (SteamAPI_InitFlat) {
            // Re-write the file in case Steam cleared it
            CreateAppIdFile();
            char errorMsgFlat[1024] = {0};
            bool resultFlat = SteamAPI_InitFlat(errorMsgFlat);

            if (resultFlat) {
                _logger->Log("SteamAPI_InitFlat succeeded");
                return true;
            } else {
                _logger->Log("SteamAPI_InitFlat failed: " + std::string(errorMsgFlat));

                if (SteamAPI_RestartAppIfNecessary && SteamAPI_RestartAppIfNecessary(_currentAppID)) {
                    _logger->Log("Steam requested app restart");
                    return false;
                }

                return false;
            }
        }
    }

    return false;
}

bool UCOnline64::InitializeSteamInterfaces() {
    try {
        if (SteamClient) {
            void* steamClient = SteamClient();
            if (!steamClient) {
                _logger->LogError("Failed to get SteamClient");
                return false;
            }
        }

        if (GetHSteamPipe) {
            void* hSteamPipe = GetHSteamPipe();
            if (!hSteamPipe) {
                _logger->LogError("Failed to get HSteamPipe");
                return false;
            }
        }

        if (SteamApps) {
            void* steamApps = SteamApps();
            if (!steamApps) {
                _logger->LogWarning("SteamApps interface not available or cannot be found");
            } else {
                _logger->Log("Successfully obtained SteamApps interface");
            }
        }

        return true;
    } catch (const std::exception& ex) {
        _logger->LogException(ex, "Error initializing Steam interfaces");
        return false;
    }
}

void UCOnline64::SaveConfig() {
    _config->SetAppID(_currentAppID);
    _config->SaveConfig();
}

void UCOnline64::ReloadConfig() {
    _config->LoadConfig();
    _currentAppID = _config->GetAppID();
    if (_currentAppID == 0) _currentAppID = 480;
}

Logger* UCOnline64::GetLogger() {
    return _logger.get();
}

void UCOnline64::SetLoggingEnabled(bool enabled) {
    _logger->SetLoggingEnabled(enabled);
    _logger->Log("Logging " + std::string(enabled ? "enabled" : "disabled"));
}

bool UCOnline64::IsLoggingEnabled() const {
    return _logger->IsLoggingEnabledA();
}

void UCOnline64::ClearLog() {
    _logger->ClearLog();
}
