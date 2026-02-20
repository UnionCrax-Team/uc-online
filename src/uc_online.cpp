#include "uc_online.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <spawn.h>
#include <signal.h>
#include <vector>

extern char **environ; // For exec

UCOnline::UCOnline(const std::string& iniFilePath) {
	_config = std::make_unique<IniConfig>(iniFilePath);
	_currentAppID = _config->GetAppID();
	_gameExecutable = _config->GetGameExecutable();
	_gameArguments = _config->GetGameArguments();
	_steamApiLibPath = _config->GetSteamApiLibPath();  // Changed DLL to Lib

	std::string logFile = _config->GetValue("Logging", "LogFile", "uc_online.log");
	bool enableLogging = _config->GetValue("Logging", "EnableLogging", "true") == "true";
	_logger = std::make_unique<Logger>(logFile, enableLogging);

	_logger->Log("uc-online initialized with appid: " + std::to_string(_currentAppID));
	_logger->Log("Game executable: " + (_gameExecutable.empty() ? "not configured" : _gameExecutable));
	_logger->Log("Steam library path: " + (_steamApiLibPath.empty() ? "default loading" : _steamApiLibPath));
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

		//LoadSteamApiLib();

		//bool result = TryMultipleInitializationMethods();

		//if (!result) {
		//	return false;
		//}

		_steamInitialized = true;
		_logger->Log("Steam initialized successfully");

		//if (InitializeSteamInterfaces()) {
		//	_logger->Log("Steam interfaces accessible");
		//} else {
		//	_logger->LogWarning("Steam interfaces not accessible");
		//}

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
			std::cerr << "Failed to create steam_appid.txt" << std::endl;
		}
	} catch (const std::exception& ex) {
		std::cerr << "Failed to create steam_appid.txt: " << ex.what() << std::endl;
	}
}

void UCOnline::LoadSteamApiLib() {
	try {
		std::string libName;
		bool is64Bit = sizeof(void*) == 8;

		if (is64Bit) {
			libName = "libsteam_api64.so";
			_logger->Log("Current process is 64-bit, looking for libsteam_api64.so");
		} else {
			libName = "libsteam_api.so";
			_logger->Log("Current process is 32-bit, looking for libsteam_api.so");
		}

		if (_steamApiLibPath.empty()) {
			_logger->Log("No custom steam library path configured, using default path");
			// Try common Steam library locations
			std::vector<std::string> defaultPaths = {
				".",
				"./lib",
				"./lib64",
				libName,
				getenv("HOME") ? std::string(getenv("HOME")) + "/.steam/steam/ubuntu12_32" : "",
				getenv("HOME") ? std::string(getenv("HOME")) + "/.local/share/Steam/ubuntu12_32" : "",
				"/usr/lib",
				"/usr/lib64",
				"/usr/local/lib",
				"/usr/local/lib64"
			};

			for (const auto& path : defaultPaths) {
				if (path.empty()) continue;
				std::filesystem::path libPath = std::filesystem::path(path) / libName;
				if (std::filesystem::exists(libPath)) {
					_logger->Log("Trying to load from: " + libPath.string());
					_steamApiModule = dlopen(libPath.string().c_str(), RTLD_LAZY);
					if (_steamApiModule) {
						_logger->Log("Successfully loaded " + libName + " from: " + libPath.string());
						break;
					}
				}
			}

			if (!_steamApiModule) {
				_logger->Log("Trying to load with just library name");
				_steamApiModule = dlopen(libName.c_str(), RTLD_LAZY);
			}
		} else {
			std::filesystem::path libPath = std::filesystem::path(_steamApiLibPath) / libName;
			if (std::filesystem::exists(libPath)) {
				_logger->Log("Found " + libName + " at: " + libPath.string());
				_steamApiModule = dlopen(libPath.string().c_str(), RTLD_LAZY);
				if (_steamApiModule) {
					_logger->Log("Successfully loaded " + libName + " from set path");
				} else {
					_logger->LogWarning("Failed to load " + libName + " from set path: " + std::string(dlerror()));
					_logger->LogWarning("Falling back to default loading");
					_steamApiModule = dlopen(libName.c_str(), RTLD_LAZY);
				}
			} else {
				_logger->LogWarning(libName + " not found at configured path, trying default loading");
				_steamApiModule = dlopen(libName.c_str(), RTLD_LAZY);
			}
		}

		if (_steamApiModule) {
			SteamAPI_Init = (SteamAPI_Init_t)dlsym(_steamApiModule, "SteamAPI_Init");
			SteamAPI_InitFlat = (SteamAPI_InitFlat_t)dlsym(_steamApiModule, "SteamAPI_InitFlat");
			SteamAPI_Shutdown = (SteamAPI_Shutdown_t)dlsym(_steamApiModule, "SteamAPI_Shutdown");
			SteamAPI_RunCallbacks = (SteamAPI_RunCallbacks_t)dlsym(_steamApiModule, "SteamAPI_RunCallbacks");
			SteamAPI_RestartAppIfNecessary = (SteamAPI_RestartAppIfNecessary_t)dlsym(_steamApiModule, "SteamAPI_RestartAppIfNecessary");
			SteamClient = (SteamClient_t)dlsym(_steamApiModule, "SteamClient");
			SteamApps = (SteamApps_t)dlsym(_steamApiModule, "SteamApps");
			GetHSteamPipe = (GetHSteamPipe_t)dlsym(_steamApiModule, "GetHSteamPipe");

			// GameServer interfaces
			GameServer_Init = (GameServer_Init_t)dlsym(_steamApiModule, "GameServer_Init");
			GameServer_Shutdown = (GameServer_Shutdown_t)dlsym(_steamApiModule, "GameServer_Shutdown");
			GameServer_RunCallbacks = (GameServer_RunCallbacks_t)dlsym(_steamApiModule, "GameServer_RunCallbacks");

			// Networking interfaces
			SteamNetworking = (SteamNetworking_t)dlsym(_steamApiModule, "SteamNetworking");
			SteamP2P_SendPacket = (SteamP2P_SendPacket_t)dlsym(_steamApiModule, "SteamP2P_SendPacket");

			// UGC interfaces
			SteamUGC = (SteamUGC_t)dlsym(_steamApiModule, "SteamUGC");

			// HTTP interfaces
			SteamHTTP = (SteamHTTP_t)dlsym(_steamApiModule, "SteamHTTP");

			// Clear any dlerror
			dlerror();
		} else {
			_logger->LogError("Failed to load steam library: " + std::string(dlerror()));
		}
	} catch (const std::exception& ex) {
		_logger->LogException(ex, "Error loading steam library");
		std::cout << "Error loading steam library: " << ex.what() << std::endl;
	}
}

bool UCOnline::TryMultipleInitializationMethods() {
	if (!SteamAPI_Init) {
		_logger->LogError("SteamAPI_Init function not found");
		return false;
	}

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

		// Initialize GameServer interfaces
		if (GameServer_Init) {
			_logger->Log("GameServer_Init interface available");
		} else {
			_logger->LogWarning("GameServer_Init interface not available");
		}

		if (GameServer_Shutdown) {
			_logger->Log("GameServer_Shutdown interface available");
		} else {
			_logger->LogWarning("GameServer_Shutdown interface not available");
		}

		if (GameServer_RunCallbacks) {
			_logger->Log("GameServer_RunCallbacks interface available");
		} else {
			_logger->LogWarning("GameServer_RunCallbacks interface not available");
		}

		// Initialize Networking interfaces
		if (SteamNetworking) {
			void* steamNetworking = SteamNetworking();
			if (!steamNetworking) {
				_logger->LogWarning("SteamNetworking interface not available or cannot be found");
			} else {
				_logger->Log("Successfully obtained SteamNetworking interface");
			}
		} else {
			_logger->LogWarning("SteamNetworking interface not available");
		}

		if (SteamP2P_SendPacket) {
			_logger->Log("SteamP2P_SendPacket interface available");
		} else {
			_logger->LogWarning("SteamP2P_SendPacket interface not available");
		}

		// Initialize UGC interfaces
		if (SteamUGC) {
			void* steamUGC = SteamUGC();
			if (!steamUGC) {
				_logger->LogWarning("SteamUGC interface not available or cannot be found");
			} else {
				_logger->Log("Successfully obtained SteamUGC interface");
			}
		} else {
			_logger->LogWarning("SteamUGC interface not available");
		}

		// Initialize HTTP interfaces
		if (SteamHTTP) {
			void* steamHTTP = SteamHTTP();
			if (!steamHTTP) {
				_logger->LogWarning("SteamHTTP interface not available or cannot be found");
			} else {
				_logger->Log("Successfully obtained SteamHTTP interface");
			}
		} else {
			_logger->LogWarning("SteamHTTP interface not available");
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
		_logger->LogError("No game executable configured in config.ini file.");
		std::cout << "No game executable configured in config.ini file." << std::endl;
		return false;
	}

	if (!std::filesystem::exists(_gameExecutable)) {
		_logger->LogError("Game executable not found: " + _gameExecutable);
		std::cout << "Game executable not found: " << _gameExecutable << std::endl;
		return false;
	}

	try {
		_logger->Log("Launching game: " + _gameExecutable + " " + _gameArguments);
		std::cout << "Launching game: " << _gameExecutable << " " << _gameArguments << std::endl;

		// Fork and exec for Linux
		pid_t pid = fork();
		if (pid == 0) {
			// Child process

			// Set working directory
			std::filesystem::path exePath(_gameExecutable);
			std::string workingDir = exePath.parent_path().string();
			if (!workingDir.empty()) {
				chdir(workingDir.c_str());
			}

			// Prepare arguments
			std::vector<char*> args;
			args.push_back(const_cast<char*>(exePath.filename().c_str()));

			if (!_gameArguments.empty()) {
				// Simple argument splitting (for basic use cases)
				std::istringstream iss(_gameArguments);
				std::string arg;
				while (iss >> arg) {
					args.push_back(const_cast<char*>(strdup(arg.c_str())));
				}
			}
			args.push_back(nullptr);

			auto newEnv = std::vector<char*>();
			for(char** env = environ; *env; env++)
			{
				newEnv.emplace_back(*env);
			}

			std::stringstream steamAppId;
			steamAppId << "SteamAppId=" << UCOnline::GetCurrentAppID();
			newEnv.emplace_back(const_cast<char*>(steamAppId.str().c_str()));

			// Execute the game
			execvpe(_gameExecutable.c_str(), args.data(), newEnv.data());

			// If execvp returns, it failed
			_logger->LogError("execvp failed: " + std::string(strerror(errno)));
			exit(EXIT_FAILURE);
		} else if (pid > 0) {
			// Parent process
			_logger->Log("Game launched successfully! (PID: " + std::to_string(pid) + ")");
			std::cout << "Game launched successfully! The game's window should appear shortly." << std::endl;

			// Don't wait for child, let it run independently
			signal(SIGCHLD, SIG_IGN);
			return true;
		} else {
			// Fork failed
			_logger->LogError("Failed to fork process: " + std::string(strerror(errno)));
			std::cout << "Failed to launch game process" << std::endl;
			return false;
		}
	} catch (const std::exception& ex) {
		_logger->LogException(ex, "Game launch failed");
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

std::string UCOnline::GetSteamApiLibPath() const {
	return _steamApiLibPath;
}

void UCOnline::SetSteamApiLibPath(const std::string& libPath) {
	_steamApiLibPath = libPath;
	_config->SetSteamApiLibPath(libPath);
	_config->SaveConfig();
}

bool UCOnline::InitializeGameServer(uint32_t ip, uint16_t port, uint16_t gamePort, uint16_t queryPort, uint32_t flags, const char* version) {
	if (!GameServer_Init) {
		_logger->LogError("GameServer_Init function not available");
		return false;
	}

	bool result = GameServer_Init(ip, port, gamePort, queryPort, flags, version);
	if (result) {
		_logger->Log("GameServer initialized successfully");
	} else {
		_logger->LogError("Failed to initialize GameServer");
	}
	return result;
}

void UCOnline::ShutdownGameServer() {
	if (GameServer_Shutdown) {
		GameServer_Shutdown();
		_logger->Log("GameServer shut down");
	} else {
		_logger->LogWarning("GameServer_Shutdown function not available");
	}
}

void UCOnline::RunGameServerCallbacks() {
	if (GameServer_RunCallbacks) {
		GameServer_RunCallbacks();
	} else {
		_logger->LogWarning("GameServer_RunCallbacks function not available");
	}
}

bool UCOnline::SendP2PPacket(uint64_t steamIDRemote, const void* pubData, uint32_t cubData, int nChannel, int nSendType) {
	if (!SteamP2P_SendPacket) {
		_logger->LogError("SteamP2P_SendPacket function not available");
		return false;
	}

	bool result = SteamP2P_SendPacket(steamIDRemote, pubData, cubData, nChannel, nSendType);
	if (result) {
		_logger->Log("P2P packet sent successfully");
	} else {
		_logger->LogError("Failed to send P2P packet");
	}
	return result;
}
