#include "uc_online.hpp"
#include <thread>
#include <chrono>
#include <windows.h>
#include <string>

// Helper function to convert std::string to std::wstring
std::wstring StringToWideString(const std::string& str) {
    int length = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wideStr(length, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wideStr[0], length);
    return wideStr;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UCOnline uc_online;

    try {
        uc_online.SetCustomAppID(480);
        uc_online.GetLogger()->Log("Now starting uc-online initialization");
        uc_online.GetLogger()->Log("Appid set to: " + std::to_string(uc_online.GetCurrentAppID()));

        // Set SteamOverlayGameId env var from ogAppID config
        std::string ogAppID = uc_online.GetSteamOverlayGameId();
        if (!ogAppID.empty()) {
            SetEnvironmentVariableA("SteamOverlayGameId", ogAppID.c_str());
            uc_online.GetLogger()->Log("Set SteamOverlayGameId env var to: " + ogAppID);
        }

        if (!uc_online.InitializeUCOnline()) {
            uc_online.GetLogger()->LogError("uc-online initialization failed");
            MessageBoxW(NULL, L"Failed to initialize Steam", L"uc-online Error", MB_OK | MB_ICONERROR);
            return 1;
        }

        if (!uc_online.GetGameExecutable().empty()) {
            if (uc_online.LaunchGame()) {
                return 0;
            } else {
                uc_online.GetLogger()->LogError("Failed to launch game");
                MessageBoxW(NULL, L"Failed to launch game. Check your configuration.", L"uc-online Error", MB_OK | MB_ICONERROR);
            }
        } else {
            uc_online.GetLogger()->LogError("No game executable configured");
            MessageBoxW(NULL, L"No game executable configured in config.ini file. Please set a game executable path.", L"uc-online Error", MB_OK | MB_ICONERROR);
        }
    } catch (const std::exception& ex) {
        uc_online.GetLogger()->LogError("Exception: " + std::string(ex.what()));
        MessageBoxW(NULL, StringToWideString(ex.what()).c_str(), L"uc-online Error", MB_OK | MB_ICONERROR);
    }

    return 0;
}