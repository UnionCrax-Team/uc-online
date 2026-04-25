#include "ini_config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

IniConfig::IniConfig(const std::string& iniFilePath) : _iniFilePath(PathUtils::ResolveRelativeToExecutable(iniFilePath)) {
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

std::string IniConfig::GetSteamApiDllPath() {
    return GetValue("uc-online", "SteamApiDLLPath", "");
}

void IniConfig::SetSteamApiDllPath(const std::string& dllPath) {
    SetValue("uc-online", "SteamApiDLLPath", dllPath);
    SaveConfig();
}

std::string IniConfig::GetSteamAppIdFile() {
    return GetValue("uc-online", "SteamAppIdFile", "steam_appid.txt");
}

void IniConfig::SetSteamAppIdFile(const std::string& appIdFilePath) {
    SetValue("uc-online", "SteamAppIdFile", appIdFilePath);
    SaveConfig();
}

void IniConfig::CreateDefaultConfig() {
    std::string defaultConfig = R"(
[uc-online]
# Set the AppID to be used here, e.g., 730 for Counter-Strike 2)
# (Please note that you will want to set it to a game you can get for free that is multiplayer. Anything else, and it won't work.)
# Default AppID is set to 480 (Spacewar), however you can change it to any AppID you want.
AppID = 480

# Executable needs to be set directly.
# Unlike the dll, there is no 'default' for the exe.
# Using UE5 games as an example, the correct launcher path will look like this:
# game folder\game folder\Binaries\Win64\game folder-Win64-Shipping.exe
GameExecutable = 

# Set launch arguments where necessary - e.g., for Source Engine games like Half-Life: Source, set it to '-game hl1 -windowed' to launch it correctly.
GameArguments = 

# Set the path to the steam_appid.txt file to use. (If one does not exist, it will be generated with the appID set at the top.)
SteamAppIdFile = steam_appid.txt

# Path to steam_api.dll (leave empty to use default location - in the same folder next to the launcher.)
# Only set the path as the folder containing the dll relative to the launcher.
# Again, using UE5 games as an example:
# game folder\Engine\Binaries\ThirdParty\Steamworks\Steamv153\Win64
SteamApiDLLPath = 

[Logging]
# Turns on logging. Not much gets logged, so it's not exactly useful. I recommend keeping it set to false, however with it being rewritten, it seems to behave differently.
# It doesn't give you a chance to see what the issue was if you set something incorrectly, it just closes immediately or runs for a second and then closes. 
# If you need it, set it to true. Otherwise, there's nothing really worth logging.
EnableLogging = false
LogFile = uc_online.log
)";

    std::ofstream file(_iniFilePath);
    if (file.is_open()) {
        file << defaultConfig;
        LoadConfig();
    } else {
        std::cerr << "Error creating default config: " << _iniFilePath << std::endl;
    }
}
