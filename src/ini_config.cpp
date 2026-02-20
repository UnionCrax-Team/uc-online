#include "ini_config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

IniConfig::IniConfig(const std::string& iniFilePath) : _iniFilePath(iniFilePath) {
	LoadConfig();
}

void IniConfig::LoadConfig() {
	_configData.clear();

	std::ifstream file(_iniFilePath);
	if (!file.is_open()) {
		CreateDefaultConfig();
		return;
	}

	std::string line;
	std::string currentSection;
	while (std::getline(file, line)) {
		// Trim whitespace
		line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) { return !std::isspace(ch); }));
		line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());

		if (line.empty() || line[0] == ';' || line[0] == '#') continue;

		if (line[0] == '[' && line.back() == ']') {
			currentSection = line.substr(1, line.size() - 2);
			_configData[currentSection];
		} else if (!currentSection.empty()) {
			size_t equalsPos = line.find('=');
			if (equalsPos != std::string::npos) {
				std::string key = line.substr(0, equalsPos);
				std::string value = line.substr(equalsPos + 1);
				// Trim key and value
				key.erase(key.begin(), std::find_if(key.begin(), key.end(), [](unsigned char ch) { return !std::isspace(ch); }));
				key.erase(std::find_if(key.rbegin(), key.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), key.end());
				value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch) { return !std::isspace(ch); }));
				value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), value.end());
				_configData[currentSection][key] = value;
			}
		}
	}
}

void IniConfig::SaveConfig() {
	std::ofstream file(_iniFilePath);
	if (!file.is_open()) {
		std::cerr << "Error saving config: " << _iniFilePath << std::endl;
		return;
	}

	for (const auto& section : _configData) {
		file << "[" << section.first << "]\n";
		for (const auto& kvp : section.second) {
			file << kvp.first << " = " << kvp.second << "\n";
		}
		file << "\n";
	}
}

std::string IniConfig::GetValue(const std::string& section, const std::string& key, const std::string& defaultValue) const {
	auto secIt = _configData.find(section);
	if (secIt != _configData.end()) {
		auto keyIt = secIt->second.find(key);
		if (keyIt != secIt->second.end()) {
			return keyIt->second;
		}
	}
	return defaultValue;
}

void IniConfig::SetValue(const std::string& section, const std::string& key, const std::string& value) {
	_configData[section][key] = value;
}

uint32_t IniConfig::GetAppID() {
	std::string appIdStr = GetValue("uc-online", "AppID", "0");
	try {
		return std::stoul(appIdStr);
	} catch (...) {
		return 0;
	}
}

void IniConfig::SetAppID(uint32_t appId) {
	SetValue("uc-online", "AppID", std::to_string(appId));
}

std::string IniConfig::GetGameExecutable() {
	return GetValue("uc-online", "GameExecutable", "");
}

void IniConfig::SetGameExecutable(const std::string& gameExePath) {
	SetValue("uc-online", "GameExecutable", gameExePath);
}

std::string IniConfig::GetGameArguments() {
	return GetValue("uc-online", "GameArguments", "");
}

void IniConfig::SetGameArguments(const std::string& arguments) {
	SetValue("uc-online", "GameArguments", arguments);
}

std::string IniConfig::GetSteamApiLibPath() {
	return GetValue("uc-online", "SteamApiLibPath", "");
}

void IniConfig::SetSteamApiLibPath(const std::string& libPath) {
	SetValue("uc-online", "SteamApiLibPath", libPath);
	SaveConfig();
}

void IniConfig::CreateDefaultConfig() {
	std::string defaultConfig = R"(
[uc-online]
# Set the appID to be used here, e.g., 730 for Counter-Strike 2
# (Please note that you will want to set it to a game you can get for free that is multiplayer. Anything else, and it won't work.)
# Default appID is set to 480 (Spacewar), however you can change it to any appID you want.
AppID = 480

# Executable needs to be set directly. Unlike the library, there is no 'default' for the exe.
# For Linux games, this would typically be a .sh script or a binary
# Example: ./game.sh or ./game.x86_64
GameExecutable =

# Set launch arguments where necessary
GameArguments =

# Set the path to the steam_appid.txt file to use. (If one does not exist, it will be generated with the appID set at the top.)
SteamAppIdFile = steam_appid.txt

# Path to steamclient.so (leave empty to use default location - typically in Steam runtime)
# For Linux, Steam libraries are usually in Steam runtime paths like:
# ~/.steam/steam/ubuntu12_32/steamclient.so
# ~/.local/share/Steam/ubuntu12_32/steamclient.so
# Proton paths: ~/.steam/steam/steamapps/common/Proton XX/dist/lib/
SteamApiLibPath =

[Logging]
# Turns on logging. Not much gets logged, so it's not exactly useful.
EnableLogging = false
LogFile = uc_online.log
)";

std::ofstream file(_iniFilePath);
if (file.is_open()) {
	file << defaultConfig;
	file.flush();
	LoadConfig();
} else {
	std::cerr << "Error creating default config: " << _iniFilePath << std::endl;
}
}
