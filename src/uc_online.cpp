#include "uc_online.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

UCOnline::UCOnline(const std::string& iniFilePath) {
    _config = std::make_unique<IniConfig>(iniFilePath);
    _currentAppID = _config->GetAppID();
    _gameExecutable = _config->GetGameExecutable();
    _gameArguments = _config->GetGameArguments();
    _steamApiDllPath = _config->GetSteamApiDllPath();
    _ogAppID = _config->GetSteamOverlayGameId();

    std::string logFile = _config->GetValue("Logging", "LogFile", "uc_online.log");
    bool enableLogging = _config->GetValue("Logging", "EnableLogging", "true") == "true";
    _logger = std::make_unique<Logger>(logFile, enableLogging);

    _logger->Log("uc-online initialized with appid: " + std::to_string(_currentAppID));
    _logger->Log("Game executable: " + (_gameExecutable.empty() ? "not configured" : _gameExecutable));
    _logger->Log("steam_api.dll path: " + (_steamApiDllPath.empty() ? "default loading" : _steamApiDllPath));
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

void UCOnline::LoadSteamApiDll() {
    try {
#ifdef _WIN64
        std::string dllName = "steam_api64.dll";
#else
        std::string dllName = "steam_api.dll";
#endif

        _logger->Log(std::string("Current process is ") +
#ifdef _WIN64
            std::string("64-bit")
#else
            std::string("32-bit")
#endif
            + std::string(", looking for ") + dllName);

        if (_steamApiDllPath.empty()) {
            _logger->Log("No custom steam_api.dll path configured, using default path: same directory as this is running from.");
            // Load default
            _steamApiModule = LoadLibraryA(dllName.c_str());
        } else {
            bool loadedFromCustom = false;
            std::filesystem::path dllPath = std::filesystem::path(_steamApiDllPath) / dllName;
            if (std::filesystem::exists(dllPath)) {
                _logger->Log("Found " + dllName + " at: " + dllPath.string());
                _steamApiModule = LoadLibraryA(dllPath.string().c_str());
                if (_steamApiModule) {
                    _logger->Log("Successfully loaded " + dllName + " from set path");
                    loadedFromCustom = true;
                } else {
                    _logger->LogWarning("Failed to load " + dllName + " from set path, likely wasn't written correctly, so it's falling back to the default path - next to this / in the same directory.");
                    _steamApiModule = LoadLibraryA(dllName.c_str());
                }
            } else {
#ifdef _WIN64
                std::filesystem::path win64Path = std::filesystem::path(_steamApiDllPath) / "win64" / dllName;
                if (std::filesystem::exists(win64Path)) {
                    _logger->Log("Found " + dllName + " at: " + win64Path.string());
                    _steamApiModule = LoadLibraryA(win64Path.string().c_str());
                    if (_steamApiModule) {
                        _logger->Log("Successfully loaded " + dllName + " from win64 subdirectory");
                        loadedFromCustom = true;
                    } else {
                        _logger->LogWarning("Failed to load " + dllName + " from win64 subdirectory, falling back to default loading");
                        _steamApiModule = LoadLibraryA(dllName.c_str());
                    }
                } else {
                    _logger->LogWarning(dllName + " not found at configured path, using default loading");
                    _steamApiModule = LoadLibraryA(dllName.c_str());
                }
#else
                _logger->LogWarning(dllName + " not found at configured path, using default loading");
                _steamApiModule = LoadLibraryA(dllName.c_str());
#endif
            }
        }
        if (_steamApiModule) {
            SteamAPI_Init = (SteamAPI_Init_t)GetProcAddress(_steamApiModule, "SteamAPI_Init");
            SteamAPI_InitFlat = (SteamAPI_InitFlat_t)GetProcAddress(_steamApiModule, "SteamAPI_InitFlat");
            SteamAPI_Shutdown = (SteamAPI_Shutdown_t)GetProcAddress(_steamApiModule, "SteamAPI_Shutdown");
            SteamAPI_RunCallbacks = (SteamAPI_RunCallbacks_t)GetProcAddress(_steamApiModule, "SteamAPI_RunCallbacks");
            SteamAPI_RestartAppIfNecessary = (SteamAPI_RestartAppIfNecessary_t)GetProcAddress(_steamApiModule, "SteamAPI_RestartAppIfNecessary");
            SteamClient = (SteamClient_t)GetProcAddress(_steamApiModule, "SteamClient");
            SteamApps = (SteamApps_t)GetProcAddress(_steamApiModule, "SteamApps");
            GetHSteamPipe = (GetHSteamPipe_t)GetProcAddress(_steamApiModule, "GetHSteamPipe");
        } else {
            _logger->LogError("Failed to load steam_api.dll");
        }
    } catch (const std::exception& ex) {
        _logger->LogException(ex, "Error loading steam_api.dll");
        std::cout << "Error loading steam_api.dll: " << ex.what() << std::endl;
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

bool UCOnline::LaunchGame() {
    _logger->Log("Attempting to launch game: " + _gameExecutable);

    if (_gameExecutable.empty()) {
        _logger->LogError("No game executable configured in config.ini file. You'll need to do that to get anywhere here.");
        std::cout << "No game executable configured in config.ini file. (I suggest you set it lol)" << std::endl;
        return false;
    }

    if (!std::filesystem::exists(_gameExecutable)) {
        _logger->LogError("Game executable not found (Did you write it correctly? Path and all too, if applicable.): " + _gameExecutable);
        std::cout << "Game executable not found (Did you write it correctly? Path and all too, if applicable.): " << _gameExecutable << std::endl;
        return false;
    }

    try {
        _logger->Log("Launching game: " + _gameExecutable + " " + _gameArguments);
        std::cout << "Launching game: " << _gameExecutable << " " << _gameArguments << std::endl;

        std::filesystem::path exePath(_gameExecutable);
        std::string workingDir = exePath.parent_path().string();

        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;

        std::string commandLine = "\"" + _gameExecutable + "\" " + _gameArguments;

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
    _config->SetSteamOverlayGameId(_ogAppID);
    _config->SaveConfig();
}

void UCOnline::ReloadConfig() {
    _config->LoadConfig();
    _currentAppID = _config->GetAppID();
    _gameExecutable = _config->GetGameExecutable();
    _gameArguments = _config->GetGameArguments();
    _ogAppID = _config->GetSteamOverlayGameId();
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
    _config->SaveConfig();
}

std::string UCOnline::GetSteamOverlayGameId() const {
    return _ogAppID;
}

void UCOnline::SetSteamOverlayGameId(const std::string& gameId) {
    _ogAppID = gameId;
    _config->SetSteamOverlayGameId(gameId);
    _config->SaveConfig();
}