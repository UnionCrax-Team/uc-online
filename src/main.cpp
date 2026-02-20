#include "uc_online.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
	std::cout << "uc-online Launcher" << std::endl;
	std::cout << "==================" << std::endl << std::endl;

	UCOnline uc_online;

	try {
		std::cout << "Current configuration:" << std::endl;
		std::cout << "  Appid: " << uc_online.GetCurrentAppID() << std::endl;
		std::cout << "  Game Executable: " << (uc_online.GetGameExecutable().empty() ? "(not configured)" : uc_online.GetGameExecutable()) << std::endl;
		std::cout << "  Game Arguments: " << (uc_online.GetGameArguments().empty() ? "(none)" : uc_online.GetGameArguments()) << std::endl;
		std::cout << std::endl;

		std::cout << "Using appid: " << uc_online.GetCurrentAppID() << " (Spacewar)" << std::endl;

		uc_online.GetLogger()->Log("Now starting uc-online initialization");
		uc_online.GetLogger()->Log("Appid set to: " + std::to_string(uc_online.GetCurrentAppID()));

		if (!uc_online.InitializeUCOnline()) {
			uc_online.GetLogger()->LogError("uc-online initialization failed");
			std::cout << "Failed to initialize Steam" << std::endl;
			return 1;
		}

		std::cout << "Steam initialized successfully!" << std::endl;

		if (!uc_online.GetGameExecutable().empty()) {
			std::cout << "Attempting to launch game using set game executable in config..." << std::endl;
			if (uc_online.LaunchGame()) {
				std::cout << "Game launched! Press any key to close this window." << std::endl;
				std::cin.get();
				return 0;
			}
		} else {
			std::cout << "No game executable configured in config.ini file. You'll need to do that to get anywhere here." << std::endl;
			std::cout << "You need to set a game's executable in the config.ini for this to work. There's no default exe." << std::endl;
		}

		std::cout << "Game should now launch and work fine through Steam. You can close this window now or after the game's window opens, or even after you close the game." << std::endl;

		for (int i = 0; i < 5; i++) {
			if (uc_online.IsSteamInitialized()) {
				uc_online.RunSteamCallbacks();
			}
			std::this_thread::sleep_for(std::chrono::seconds(1));
			std::cout << "Trying to run the game... This usually means it won't run even after successfully initializing Steam using a spoofed appid. Double check your config, especially the gameexecutable line." << std::endl;
			std::cout << "(" << (i + 1) << "/5 seconds until this window automatically closes unless game launch succeeds.)" << std::endl;
		}

		std::cout << std::endl << "This window is now safe to close." << std::endl;
	} catch (const std::exception& ex) {
		std::cout << "An error occurred: " << ex.what() << std::endl;
	}

	std::cout << "Done!" << std::endl;
	return 0;
}
