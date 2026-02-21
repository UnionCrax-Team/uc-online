#include "ini_config.hpp"
#include "path_utils.hpp"
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

    // Verify that all required sections and keys exist, add defaults if missing
    // Check uc-online section
    if (_configData.find("uc-online") == _configData.end()) {
        _configData["uc-online"] = {};
    }
    
    std::unordered_map<std::string, std::string>& ucOnlineSection = _configData["uc-online"];
    if (ucOnlineSection.find("AppID") == ucOnlineSection.end()) ucOnlineSection["AppID"] = "480";
    if (ucOnlineSection.find("GameExecutable") == ucOnlineSection.end()) ucOnlineSection["GameExecutable"] = "";
    if (ucOnlineSection.find("GameArguments") == ucOnlineSection.end()) ucOnlineSection["GameArguments"] = "";
    if (ucOnlineSection.find("SteamAppIdFile") == ucOnlineSection.end()) ucOnlineSection["SteamAppIdFile"] = "steam_appid.txt";
    
    // Check Logging section
    if (_configData.find("Logging") == _configData.end()) {
        _configData["Logging"] = {};
    }
    
    std::unordered_map<std::string, std::string>& loggingSection = _configData["Logging"];
    if (loggingSection.find("EnableLogging") == loggingSection.end()) loggingSection["EnableLogging"] = "false";
    if (loggingSection.find("LogFile") == loggingSection.end()) loggingSection["LogFile"] = "uc_online.log";
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
    std::string appIdStr = GetValue("uc-online", "AppID", "480");
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

std::string IniConfig::GetSteamAppIdFile() {
    return GetValue("uc-online", "SteamAppIdFile", "steam_appid.txt");
}

void IniConfig::SetSteamAppIdFile(const std::string& filePath) {
    SetValue("uc-online", "SteamAppIdFile", filePath);
}

bool IniConfig::GetEnableLogging() {
    std::string logStr = GetValue("Logging", "EnableLogging", "false");
    std::string lowerStr = logStr;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return (lowerStr == "true" || lowerStr == "1");
}

void IniConfig::SetEnableLogging(bool enable) {
    SetValue("Logging", "EnableLogging", enable ? "true" : "false");
}

std::string IniConfig::GetLogFile() {
    return GetValue("Logging", "LogFile", "uc_online.log");
}

void IniConfig::SetLogFile(const std::string& logFile) {
    SetValue("Logging", "LogFile", logFile);
}

void IniConfig::CreateDefaultConfig() {
    // First, initialize the config structure directly
    _configData["uc-online"] = {
        {"AppID", "480"},
        {"GameExecutable", ""},
        {"GameArguments", ""},
        {"SteamAppIdFile", "steam_appid.txt"}
    };
    
    _configData["Logging"] = {
        {"EnableLogging", "false"},
        {"LogFile", "uc_online.log"}
    };
    
    // Now save the default config to file
    std::ofstream file(_iniFilePath);
    if (file.is_open()) {
        file << "# uc-online configuration file\n";
        file << "# Generated automatically if not exists\n\n";
        
        file << "[uc-online]\n";
        file << "# Set the appID to be used here, e.g., 730 for Counter-Strike 2\n";
        file << "# (Please note that you will want to set it to a game you can get for free that is multiplayer. Anything else, and it won't work.)\n";
        file << "# Default appID is set to 480 (Spacewar), however you can change it to any appID you want.\n";
        file << "AppID = 480\n\n";
        
        file << "# Executable needs to be set directly. Unlike the dll, there is no 'default' for the exe.\n";
        file << "# Using UE5 games as an example, the correct launcher path will look like this:\n";
        file << "# game folder\\game folder\\Binaries\\Win64\\game folder-Win64-Shipping.exe\n";
        file << "GameExecutable = \n\n";
        
        file << "# Set launch arguments where necessary - e.g., for Source Engine games like Half-Life: Source, set it to '-game hl1 -windowed' to launch it correctly.\n";
        file << "GameArguments = \n\n";
        
        file << "# Set the path to the steam_appid.txt file to use. (If one does not exist, it will be generated with the appID set at the top.)\n";
        file << "SteamAppIdFile = steam_appid.txt\n\n";
        
        file << "[Logging]\n";
        file << "# Turns on logging. Not much gets logged, so it's not exactly useful. I recommend keeping it set to false, however with it being rewritten, it seems to behave differently.\n";
        file << "# It doesn't give you a chance to see what the issue was if you set something incorrectly, it just closes immediately or runs for a second and then closes. \n";
        file << "# If you need it, set it to true. Otherwise, there's nothing really worth logging.\n";
        file << "EnableLogging = false\n";
        file << "LogFile = uc_online.log\n";
        
        file.flush();
        if (!file.good()) {
            std::cerr << "Error writing default config: " << _iniFilePath << std::endl;
        }
    } else {
        std::cerr << "Error creating default config file: " << _iniFilePath << std::endl;
    }
}