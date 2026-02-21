#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>

class IniConfig {
public:
    IniConfig(const std::string& iniFilePath = "config.ini");
    void LoadConfig();
    void SaveConfig();
    std::string GetValue(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;
    void SetValue(const std::string& section, const std::string& key, const std::string& value);

    // Specific getters/setters
    uint32_t GetAppID();
    void SetAppID(uint32_t appId);
    std::string GetGameExecutable();
    void SetGameExecutable(const std::string& gameExePath);
    std::string GetGameArguments();
    void SetGameArguments(const std::string& arguments);
    std::string GetSteamAppIdFile();
    void SetSteamAppIdFile(const std::string& filePath);
    bool GetEnableLogging();
    void SetEnableLogging(bool enable);
    std::string GetLogFile();
    void SetLogFile(const std::string& logFile);

private:
    std::string _iniFilePath;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> _configData;
    void CreateDefaultConfig();
};