#include "uc_online.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

UCOnline::UCOnline(const std::string& iniFilePath, const std::string& dllDirectory) {
    _dllDirectory = dllDirectory;
    _config = std::make_unique<IniConfig>(iniFilePath);
    _currentAppID = _config->GetAppID();
    _gameExecutable = _config->GetGameExecutable();
    _gameArguments = _config->GetGameArguments();

    std::string logFile = _config->GetValue("Logging", "LogFile", "uc_online.log");
    bool enableLogging = _config->GetValue("Logging", "EnableLogging", "true") == "true";
    _logger = std::make_unique<Logger>(logFile, enableLogging);

    _logger->Log("uc-online initialized with appid: " + std::to_string(_currentAppID));
    _logger->Log("Game executable: " + (_gameExecutable.empty() ? "not configured" : _gameExecutable));
    _logger->Log("DLL directory: " + (_dllDirectory.empty() ? "current directory" : _dllDirectory));
}

UCOnline::~UCOnline() {
    _logger->Log("uc-online shutting down");
    ShutdownUCOnline();
}

bool UCOnline::InitializeUCOnline() {
    try {
        if (_currentAppID == 0) {
            _logger->LogWarning("No appid set in the config.ini. This likely will not work.");
            _logger->LogWarning("Please set appid in config.ini, if there is not one - there will be one after running this.");
            _logger->LogWarning("Continuing without set appid.");
        } else {
            _logger->Log("Initializing Steam with appid: " + std::to_string(_currentAppID));
            CreateAppIdFile();
        }

        LoadSteamApiDll();

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

void UCOnline::ShutdownUCOnline() {
    if (_steamInitialized) {
        _logger->Log("Shutting down...");
        if (SteamAPI_Shutdown) SteamAPI_Shutdown();
        _steamInitialized = false;
        _logger->Log("Shutdown complete!");
    }
}

void UCOnline::RunSteamCallbacks() {
    if (_steamInitialized && SteamAPI_RunCallbacks) {
        SteamAPI_RunCallbacks();
    }
}

void UCOnline::SetCustomAppID(uint32_t appID) {
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

uint32_t UCOnline::GetCurrentAppID() const {
    return _currentAppID;
}

bool UCOnline::IsSteamInitialized() const {
    return _steamInitialized;
}

void UCOnline::CreateAppIdFile() {
    if (_currentAppID == 0) {
        _logger->Log("Skipping steam_appid.txt creation - no appid configured.");
        _logger->Log("If there is one already, it will be ignored.");
        return;
    }

    try {
        std::ofstream file("steam_appid.txt");
        if (file.is_open()) {
            file << _currentAppID;
            _logger->Log("Created steam_appid.txt with set appid: " + std::to_string(_currentAppID));
        } else {
            std::cerr << "Failed to create steam_appid.txt, make sure you're running this in a directory like your Documents or Downloads folder." << std::endl;
            std::cerr << "(e.g., \"C:\\Users\\user\\Downloads\\game folder\")" << std::endl;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Failed to create steam_appid.txt: " << ex.what() << std::endl;
    }
}

bool UCOnline::LaunchGame() {
    _logger->Log("LaunchGame() called - functionality not yet implemented");
    // TODO: Implement game launching functionality
    return false;
}

void UCOnline::LoadSteamApiDll() {
    try {
        // The real steam_api.dll must be renamed to steam_api_orig.dll
        // and placed in the same directory as this DLL (the game's directory).
        std::string origDllName = "steam_api_orig.dll";

        std::string dllPath;
        if (!_dllDirectory.empty()) {
            dllPath = _dllDirectory + "\\" + origDllName;
            _logger->Log("Loading " + origDllName + " from game directory: " + dllPath);
        } else {
            dllPath = origDllName;
            _logger->Log("Loading " + origDllName + " from current directory (DLL directory unknown)");
        }

        _steamApiModule = LoadLibraryA(dllPath.c_str());

        if (!_steamApiModule && !_dllDirectory.empty()) {
            // Fallback: try current directory
            _logger->LogWarning("Failed to load " + origDllName + " from game directory, trying current directory");
            _steamApiModule = LoadLibraryA(origDllName.c_str());
        }

        if (_steamApiModule) {
            _logger->Log("Successfully loaded " + origDllName);
            SteamAPI_Init = (SteamAPI_Init_t)GetProcAddress(_steamApiModule, "SteamAPI_Init");
            SteamAPI_InitFlat = (SteamAPI_InitFlat_t)GetProcAddress(_steamApiModule, "SteamAPI_InitFlat");
            SteamAPI_Shutdown = (SteamAPI_Shutdown_t)GetProcAddress(_steamApiModule, "SteamAPI_Shutdown");
            SteamAPI_RunCallbacks = (SteamAPI_RunCallbacks_t)GetProcAddress(_steamApiModule, "SteamAPI_RunCallbacks");
            SteamAPI_RestartAppIfNecessary = (SteamAPI_RestartAppIfNecessary_t)GetProcAddress(_steamApiModule, "SteamAPI_RestartAppIfNecessary");
            SteamClient = (SteamClient_t)GetProcAddress(_steamApiModule, "SteamClient");
            SteamApps = (SteamApps_t)GetProcAddress(_steamApiModule, "SteamApps");
            GetHSteamPipe = (GetHSteamPipe_t)GetProcAddress(_steamApiModule, "GetHSteamPipe");
        } else {
            _logger->LogError("Failed to load " + origDllName + ". Make sure the original steam_api.dll has been renamed to steam_api_orig.dll in the game directory.");
        }
    } catch (const std::exception& ex) {
        _logger->LogException(ex, "Error loading steam_api_orig.dll");
        std::cout << "Error loading steam_api_orig.dll: " << ex.what() << std::endl;
    }
}

bool UCOnline::TryMultipleInitializationMethods() {
    if (!SteamAPI_Init) return false;

    char errorMsg[1024] = {0};
    bool result = SteamAPI_Init(errorMsg);

    if (result) {
        _logger->Log("SteamAPI_Init succeeded");
        return true;
    } else {
        _logger->Log("SteamAPI_Init failed: " + std::string(errorMsg));

        if (SteamAPI_InitFlat) {
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

bool UCOnline::InitializeSteamInterfaces() {
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

void UCOnline::SetGameExecutable(const std::string& gameExePath) {
    _gameExecutable = gameExePath;
    _config->SetGameExecutable(gameExePath);
    _config->SaveConfig();
}

void UCOnline::SetGameArguments(const std::string& arguments) {
    _gameArguments = arguments;
    _config->SetGameArguments(arguments);
    _config->SaveConfig();
}

std::string UCOnline::GetGameExecutable() const {
    return _gameExecutable;
}

std::string UCOnline::GetGameArguments() const {
    return _gameArguments;
}

void UCOnline::SaveConfig() {
    _config->SetAppID(_currentAppID);
    _config->SetGameExecutable(_gameExecutable);
    _config->SetGameArguments(_gameArguments);
    _config->SaveConfig();
}

void UCOnline::ReloadConfig() {
    _config->LoadConfig();
    _currentAppID = _config->GetAppID();
    _gameExecutable = _config->GetGameExecutable();
    _gameArguments = _config->GetGameArguments();
}

Logger* UCOnline::GetLogger() {
    return _logger.get();
}

void UCOnline::SetLoggingEnabled(bool enabled) {
    _logger->SetLoggingEnabled(enabled);
    _logger->Log("Logging " + std::string(enabled ? "enabled" : "disabled"));
}

bool UCOnline::IsLoggingEnabled() const {
    return _logger->IsLoggingEnabledA();
}

void UCOnline::ClearLog() {
    _logger->ClearLog();
}
