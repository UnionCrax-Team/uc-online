#pragma once

#include "ini_config.hpp"
#include "logger.hpp"
#include <string>
#include <memory>
#include <dlfcn.h>
#include <cstdint>

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

	std::string GetSteamApiLibPath() const;
	void SetSteamApiLibPath(const std::string& libPath);

	// Public methods to access Steamworks interfaces
	bool InitializeGameServer(uint32_t ip, uint16_t port, uint16_t gamePort, uint16_t queryPort, uint32_t flags, const char* version);
	void ShutdownGameServer();
	void RunGameServerCallbacks();
	bool SendP2PPacket(uint64_t steamIDRemote, const void* pubData, uint32_t cubData, int nChannel, int nSendType);

private:
	bool _steamInitialized = false;
	uint32_t _currentAppID;
	std::unique_ptr<IniConfig> _config;
	std::unique_ptr<Logger> _logger;
	std::string _gameExecutable;
	std::string _gameArguments;
	std::string _steamApiLibPath;

	bool TryMultipleInitializationMethods();
	void LoadSteamApiLib();
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

	// GameServer interfaces
typedef bool (*GameServer_Init_t)(uint32_t ip, uint16_t port, uint16_t gamePort, uint16_t queryPort, uint32_t flags, const char* version);
typedef void (*GameServer_Shutdown_t)();
typedef void (*GameServer_RunCallbacks_t)();

	// Networking interfaces
typedef void* (*SteamNetworking_t)();
typedef bool (*SteamP2P_SendPacket_t)(uint64_t steamIDRemote, const void* pubData, uint32_t cubData, int nChannel, int nSendType);

	// UGC interfaces
typedef void* (*SteamUGC_t)();

	// HTTP interfaces
typedef void* (*SteamHTTP_t)();

	SteamAPI_Init_t SteamAPI_Init = nullptr;
	SteamAPI_InitFlat_t SteamAPI_InitFlat = nullptr;
	SteamAPI_Shutdown_t SteamAPI_Shutdown = nullptr;
	SteamAPI_RunCallbacks_t SteamAPI_RunCallbacks = nullptr;
	SteamAPI_RestartAppIfNecessary_t SteamAPI_RestartAppIfNecessary = nullptr;
	SteamClient_t SteamClient = nullptr;
	SteamApps_t SteamApps = nullptr;
	GetHSteamPipe_t GetHSteamPipe = nullptr;

	// GameServer function pointers
	GameServer_Init_t GameServer_Init = nullptr;
	GameServer_Shutdown_t GameServer_Shutdown = nullptr;
	GameServer_RunCallbacks_t GameServer_RunCallbacks = nullptr;

	// Networking function pointers
	SteamNetworking_t SteamNetworking = nullptr;
	SteamP2P_SendPacket_t SteamP2P_SendPacket = nullptr;

	// UGC function pointers
	SteamUGC_t SteamUGC = nullptr;

	// HTTP function pointers
	SteamHTTP_t SteamHTTP = nullptr;

	void* _steamApiModule = nullptr;
};
