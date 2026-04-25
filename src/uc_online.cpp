#include "uc_online.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <windows.h>

UCOnline::UCOnline(const std::string& iniFilePath) {
    _config = std::make_unique<IniConfig>(iniFilePath);
    _currentAppID = _config->GetAppID();
    _gameExecutable = _config->GetGameExecutable();
    _gameArguments = _config->GetGameArguments();
    _steamApiDllPath = _config->GetSteamApiDllPath();
    _steamAppIdFile = _config->GetSteamAppIdFile();

    std::string logFile = _config->GetValue("Logging", "LogFile", "uc_online.log");
    bool enableLogging = _config->GetValue("Logging", "EnableLogging", "true") == "true";
    _logger = std::make_unique<Logger>(logFile, enableLogging);

    _logger->Log("uc-online initialized with appid: " + std::to_string(_currentAppID));
    _logger->Log("Game executable: " + (_gameExecutable.empty() ? "not configured" : _gameExecutable));
    _logger->Log("steam_api.dll path: " + (_steamApiDllPath.empty() ? "default loading" : _steamApiDllPath));
    _logger->Log("steam_appid.txt path: " + _steamAppIdFile);
}

UCOnline::~UCOnline() {
    _logger->Log("uc-online shutting down");
    ShutdownUCOnline();
}

bool UCOnline::InitializeUCOnline() {
    try {
        _restartRequested = false;
        _steamInitialized = false;

        // Set environment variables for child process inheritance
        std::string appIdStr = std::to_string(_currentAppID);
        SetEnvironmentVariableA("SteamAppId", appIdStr.c_str());
        SetEnvironmentVariableA("SteamGameId", appIdStr.c_str());
        _logger->Log("Set SteamAppId/SteamGameId environment variables for child process inheritance.");

        if (_currentAppID == 0) {
            _logger->LogWarning("No appid set in the config.ini. This likely will not work.");
            _logger->LogWarning("Please set appid in config.ini, if there is not one - there will be one after running this.");
            _logger->LogWarning("Continuing without set appid.");
        } else {
            CreateAppIdFile();
        }

        // Initialize Steam in the launcher process so Steam registers "playing" status
        _logger->Log("Initializing Steam with appid: " + std::to_string(_currentAppID));

        if (SteamAPI_RestartAppIfNecessary(_currentAppID)) {
            _logger->Log("Steam requested app restart");
            _restartRequested = true;
            return false;
        }

        SteamErrMsg errorMsg;
        if (SteamAPI_InitEx(&errorMsg) != k_ESteamAPIInitResult_OK) {
            _logger->LogError("SteamAPI_InitEx failed: " + std::string(errorMsg));
            std::cout << "SteamAPI_InitEx failed: " << errorMsg << std::endl;
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
        SteamAPI_Shutdown();
        _steamInitialized = false;
        _logger->Log("Shutdown complete!");
    }
}

void UCOnline::RunSteamCallbacks() {
    if (_steamInitialized) {
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

bool UCOnline::WasRestartRequested() const {
    return _restartRequested;
}

void UCOnline::CreateAppIdFile() {
    if (_currentAppID == 0) {
        _logger->Log("Skipping steam_appid.txt creation - no appid configured.");
        _logger->Log("If there is one already, it will be ignored.");
        return;
    }

    try {
        auto writeAppIdFile = [this](const std::string& appIdFilePath, bool createParentDirectories) {
            std::filesystem::path path(appIdFilePath);
            if (createParentDirectories && !path.parent_path().empty()) {
                std::filesystem::create_directories(path.parent_path());
            }

            std::ofstream file(appIdFilePath);
            if (file.is_open()) {
                file << _currentAppID;
                _logger->Log("Created steam_appid.txt at: " + appIdFilePath + " with appid: " + std::to_string(_currentAppID));
            } else {
                std::cerr << "Failed to create steam_appid.txt at: " << appIdFilePath << std::endl;
            }
        };

        std::string configuredAppIdFilePath = PathUtils::ResolveRelativeToExecutable(_steamAppIdFile.empty() ? "steam_appid.txt" : _steamAppIdFile);
        writeAppIdFile(configuredAppIdFilePath, true);

        if (!_gameExecutable.empty()) {
            std::filesystem::path gameExecutablePath(PathUtils::ResolveRelativeToExecutable(_gameExecutable));
            if (std::filesystem::exists(gameExecutablePath)) {
                std::string gameAppIdFilePath = (gameExecutablePath.parent_path() / "steam_appid.txt").string();
                if (gameAppIdFilePath != configuredAppIdFilePath) {
                    writeAppIdFile(gameAppIdFilePath, false);
                }
            } else {
                _logger->LogWarning("Skipping game-directory steam_appid.txt creation because configured game executable was not found: " + gameExecutablePath.string());
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Failed to create steam_appid.txt: " << ex.what() << std::endl;
    }
}



bool UCOnline::InitializeSteamInterfaces() {
    try {
        if (!SteamUser()) {
            _logger->LogError("SteamUser interface not available");
            return false;
        }

        if (!SteamApps()) {
            _logger->LogWarning("SteamApps interface not available");
        } else {
            _logger->Log("Successfully obtained SteamApps interface");
        }

        // Initialize all Steam interfaces
        if (!InitializeSteamGameServer()) {
            _logger->LogWarning("Failed to initialize Steam GameServer interface");
        }

        if (!InitializeSteamUGC()) {
            _logger->LogWarning("Failed to initialize Steam UGC interface");
        }

        if (!InitializeSteamHTTP()) {
            _logger->LogWarning("Failed to initialize Steam HTTP interface");
        }

        if (!InitializeSteamNetworking()) {
            _logger->LogWarning("Failed to initialize Steam Networking interface");
        }

        if (!InitializeSteamClient()) {
            _logger->LogWarning("Failed to initialize Steam Client interface");
        }

        return true;
    } catch (const std::exception& ex) {
        _logger->LogException(ex, "Error initializing Steam interfaces");
        return false;
    }
}

bool UCOnline::InitializeSteamGameServer() {
    try {
        if (!SteamGameServer()) {
            _logger->LogError("SteamGameServer interface not available");
            return false;
        }
        _logger->Log("Successfully obtained SteamGameServer interface");
        return true;
    } catch (const std::exception& ex) {
        _logger->LogException(ex, "Error initializing Steam GameServer interface");
        return false;
    }
}

bool UCOnline::InitializeSteamUGC() {
    try {
        if (!SteamUGC()) {
            _logger->LogError("SteamUGC interface not available");
            return false;
        }
        _logger->Log("Successfully obtained SteamUGC interface");
        return true;
    } catch (const std::exception& ex) {
        _logger->LogException(ex, "Error initializing Steam UGC interface");
        return false;
    }
}

bool UCOnline::InitializeSteamHTTP() {
    try {
        if (!SteamHTTP()) {
            _logger->LogError("SteamHTTP interface not available");
            return false;
        }
        _logger->Log("Successfully obtained SteamHTTP interface");
        return true;
    } catch (const std::exception& ex) {
        _logger->LogException(ex, "Error initializing Steam HTTP interface");
        return false;
    }
}

bool UCOnline::InitializeSteamNetworking() {
    try {
        if (!SteamNetworking()) {
            _logger->LogError("SteamNetworking interface not available");
            return false;
        }
        _logger->Log("Successfully obtained SteamNetworking interface");
        return true;
    } catch (const std::exception& ex) {
        _logger->LogException(ex, "Error initializing Steam Networking interface");
        return false;
    }
}

bool UCOnline::InitializeSteamClient() {
    try {
        if (!SteamClient()) {
            _logger->LogError("SteamClient interface not available");
            return false;
        }
        _logger->Log("Successfully obtained SteamClient interface");
        return true;
    } catch (const std::exception& ex) {
        _logger->LogException(ex, "Error initializing Steam Client interface");
        return false;
    }
}

bool UCOnline::LaunchGame() {
    _logger->Log("Attempting to launch game: " + _gameExecutable);

    if (_gameExecutable.empty()) {
        _logger->LogError("No game executable configured in config.ini file. You'll need to do that to get anywhere here.");
        std::cout << "No game executable configured in config.ini file. (I suggest you set it lol)" << std::endl;
        return false;
    }

    std::string gameExecutablePath = PathUtils::ResolveRelativeToExecutable(_gameExecutable);

    if (!std::filesystem::exists(gameExecutablePath)) {
        _logger->LogError("Game executable not found (Did you write it correctly? Path and all too, if applicable.): " + gameExecutablePath);
        std::cout << "Game executable not found (Did you write it correctly? Path and all too, if applicable.): " << gameExecutablePath << std::endl;
        return false;
    }

    try {
        _logger->Log("Launching game: " + gameExecutablePath + " " + _gameArguments);
        std::cout << "Launching game: " << gameExecutablePath << " " << _gameArguments << std::endl;

        std::filesystem::path exePath(gameExecutablePath);
        std::string workingDir = exePath.parent_path().string();

        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;

        std::string commandLine = "\"" + gameExecutablePath + "\" " + _gameArguments;

        if (CreateProcessA(NULL, const_cast<char*>(commandLine.c_str()), NULL, NULL, FALSE, 0, NULL, workingDir.c_str(), &si, &pi)) {
            _logger->Log("Game launched successfully! (PID: " + std::to_string(pi.dwProcessId) + ")");
            std::cout << "Game launched successfully! The game's window should appear shortly. This window can be closed and / or may close on its own." << std::endl;
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return true;
        } else {
            _logger->LogError("Failed to launch game process");
            std::cout << "Failed to launch game process" << std::endl;
            return false;
        }
    } catch (const std::exception& ex) {
        _logger->LogException(ex, "Game launch failed");
        _logger->Log("This can happen for many reasons, if you're certain it should work then just try whatever you can. Throw whatever you got at the wall and see what sticks.");
        std::cout << "Error launching game: " << ex.what() << std::endl;
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
    _config->SetSteamApiDllPath(_steamApiDllPath);
    _config->SetSteamAppIdFile(_steamAppIdFile);
    _config->SaveConfig();
}

void UCOnline::ReloadConfig() {
    _config->LoadConfig();
    _currentAppID = _config->GetAppID();
    _gameExecutable = _config->GetGameExecutable();
    _gameArguments = _config->GetGameArguments();
    _steamApiDllPath = _config->GetSteamApiDllPath();
    _steamAppIdFile = _config->GetSteamAppIdFile();
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

std::string UCOnline::GetSteamApiDllPath() const {
    return _steamApiDllPath;
}

void UCOnline::SetSteamApiDllPath(const std::string& dllPath) {
    _steamApiDllPath = dllPath;
    _config->SetSteamApiDllPath(dllPath);
    _logger->Log("Steam API DLL path set to: " + (dllPath.empty() ? "default" : dllPath));
}

std::string UCOnline::GetSteamAppIdFile() const {
    return _steamAppIdFile;
}

void UCOnline::SetSteamAppIdFile(const std::string& appIdFilePath) {
    _steamAppIdFile = appIdFilePath;
    _config->SetSteamAppIdFile(appIdFilePath);
    _logger->Log("steam_appid.txt path set to: " + (appIdFilePath.empty() ? "steam_appid.txt" : appIdFilePath));
}
