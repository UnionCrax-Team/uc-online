#include "uc_online.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "uc-online Launcher 32-bit" << std::endl;
    std::cout << "=========================" << std::endl << std::endl;

    UCOnline uc_online;

    try {
        std::cout << "Current configuration:" << std::endl;
        std::cout << "  Appid: " << uc_online.GetCurrentAppID() << std::endl;
        std::cout << "  Game Executable: " << (uc_online.GetGameExecutable().empty() ? "(not configured)" : uc_online.GetGameExecutable()) << std::endl;
        std::cout << "  Game Arguments: " << (uc_online.GetGameArguments().empty() ? "(none)" : uc_online.GetGameArguments()) << std::endl;
        std::cout << std::endl;

        std::cout << "Using appid from config: " << uc_online.GetCurrentAppID() << std::endl;

        uc_online.GetLogger()->Log("Now starting uc-online initialization");
        uc_online.GetLogger()->Log("Appid set to: " + std::to_string(uc_online.GetCurrentAppID()));

        if (!uc_online.InitializeUCOnline()) {
            if (uc_online.WasRestartRequested()) {
                std::cout << "Steam requested a restart. Exiting so Steam can relaunch the app." << std::endl;
                return 0;
            }

            uc_online.GetLogger()->LogError("uc-online initialization failed");
            std::cout << "Failed to initialize Steam with appid " << uc_online.GetCurrentAppID() << "." << std::endl;
            std::cout << "This usually means you don't own this app on your Steam account." << std::endl;
            std::cout << "Try setting AppId=480 in config.ini (Spacewar - free for everyone)." << std::endl;
            return 1;
        }

        std::cout << "Steam initialized successfully!" << std::endl;

        if (uc_online.GetGameExecutable().empty()) {
            std::cout << "No game executable configured in config.ini file. You'll need to do that to get anywhere here." << std::endl;
            std::cout << "You need to set a game's executable in the config.ini for this to work. There's no default exe." << std::endl;
            return 1;
        }

        std::cout << "Attempting to launch game using set game executable in config..." << std::endl;
        if (uc_online.LaunchGame()) {
            std::cout << "Game launched! Keeping Steam connection active." << std::endl;
            std::cout << "Press Enter to exit (game will keep running)." << std::endl;

            // Run Steam callbacks to maintain the "playing" status on Steam.
            // Break when Enter is pressed or Steam disconnects.
            while (uc_online.IsSteamInitialized()) {
                uc_online.RunSteamCallbacks();
                Sleep(100);
                if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
                    break;
                }
            }

            // Flush leftover Enter state so it doesn't leak to the terminal
            Sleep(200);
            std::cout << std::endl << "Steam connection closed. This window is now safe to close." << std::endl;
            return 0;
        }

        return 1;
    } catch (const std::exception& ex) {
        std::cout << "An error occurred: " << ex.what() << std::endl;
        return 1;
    }
}
