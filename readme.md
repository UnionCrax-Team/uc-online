# __Attention:__ uc-online2 is now available, [you can find it here on the new repo](https://github.com/veeeanti/uc-online2) - it is still in its early stages however, expect issues and instability!

====================

## __When forking this repo, PLEASE make sure to toggle off the "only fork main branch" option so you can work with all versions / branches and push your modifications to them as needed and build them too using GitHub actions. if you make a new branch, add in the name of the branch in the workflow files for main and new branch to have it build correctly. if you want to contribute, contact me on discord or create a pull request if you can and I'll look at it and we'll go from there.__ 

====================

## Building:

Requirements:
 - Cmake (latest)
 - Visual Studio (latest, make sure C / C++ development tools is checked and installed on initial VS Installer window and tab)
 - MSVC v143 Build tools 
 - Windows 10 / 11 SDK
 - MSBuild(?)

Configuring and Building:
Preferably in a CMD window in the root of the repo folder on Windows,
 - run ``mkdir build-x86`` then ``cd build-x86`` to move into it.
 - run ``cmake .. -G "Visual Studio 17 2022" -A Win32`` to configure x86 variants for building.
 - if you want, you can go ahead and run ``cmake --build . --config Release`` to build the 32bit exes.
 - run ``cd ..`` to move back one folder and then run ``mkdir build-x64`` and ``cd build-x64`` to configure the x64 variants build.
 - to config it, run ``cmake .. -G "Visual Studio 17 2022" -A x64`` and then if you want to get it done with and built, run ``cmake --build . --config Release`` to build the 64bit exes.
 - For some odd reason, it builds two exes in the Release folders - "uc-online.exe" and "uc-online64.exe", make sure to grab the right one so there's no confusion. 64 will likely be bigger sizes than 32, from what I've seen so far.

In terminal of choice on Linux, Ubuntu / Debian based distros first,
 - run ``sudo apt-get install -y cmake build-essential gcc-multilib g++-multilib libgtk-3-dev`` and confirm the other packages if any others are needed or are depended on.
 - move the terminal into your repo folder you have cloned, ``cd ~/Documents/GitHub/uc-online`` if you used GitHub desktop to clone and pull with.
 - run ``mkdir build && cd build && cmake .. && make`` to run everything in one line. if it doesn't work, try it again but with each command separately.
 - both launcher variants can be found in "build/uc-online" (x86) and in "build/uc-online64" (x64), next you will need to make them executable.
 - if your terminal is still in the "build" folder, ``chmod +x ./build/uc-online && chmod +x ./build/uc-online64`` to set them as executables. otherwise you'll have issues getting it to work at all. xD

For Arch / Arch-based, Fedora, or any distro using RPM packages,
 - run ``sudo pacman -Syu cmake build-essential gcc-multilib g++-multilib libgtk-3-dev`` or use your package manager of your choice. it should be still the same packages specified. let me know if this is not the case for you when building.
 - follow Cmake instructions above and other instructions written for Ubuntu / Debian based distros. 

For TempleOS,
 - how, or more importantly - WHY do you have this installed and what are you planning?
 - are you secretly Terry Davis, the greatest programmer ever to be chosen by God?
 - I don't believe this would be compatible with the HolyC language and wouldn't even begin to compile... much less even be useful to anyone if it did somehow compile and run successfully. steam is not on this and probably will never be, sorry.

I do recommend just letting the workflows take care of building it for you, it uploads each variant artifacts separately to download and only takes two to three minutes to complete for Windows and will build in under a minute for Linux (even though it takes significantly less storage to build for Linux lol).

====================

## How to use:

### Heads up!!
 - When running the uc-online(64).exe with __no config.ini__ next to it, one will be generated. Unfortunately, the only thing it creates is the appid line. I tried fixing this multiple times so it even generates the commented lines to help understand how this needs to be set up to correctly work with examples written too. Please help with this, anyone that can figure it out. 

Configs,
- for release downloads without steam_api(64).dll found next to uc-online(64).exe (distinguished by the "nosteamworks" in the zip name):
 - Extract files from either x32 or x64 folders into main game folder, depending on the architecture of the game itself (look for the steam_api.dll - if it has 64 in the name, it's 64bit, if not then it's 32)
 - Open config.ini, you MUST set it up like this or similarly according to your game.
   - ``AppId = 480`` - iirc the way I wrote it forces you to use only this appid and if you try anything else it gets angy lol. If you try that, make sure the steam_appid.txt reflects it and matches your changes here.
   - ``GameExecutable = .\gmod.exe`` - make sure you have the .\ at the start of it, it may not work without it. 
   - ``GameArguments = -steam -game garrysmod`` - nothing too much here.
   - ``SteamApiDLLPath = .\bin`` - DO NOT INCLUDE THE FILE NAME AT THE END, ONLY SET THE FOLDER PATH!!  It is written to search for the dll on its own and if not set then automatically looks next to the uc-online(64).exe as a fallback.
   - ``SteamAppIdFile = steam_appid.txt`` - this one doesn't require the .\ because it doesn't execute it like the exe, I believe.
   - ``EnableLogging = false`` - set it to true if necessary, in case something goes wrong and you need to figure it out. Not a whole lot gets logged iirc, but it'll at least point you in a right enough direction.
   - ``LogFile = uc-online.log`` - sets log file to be created and have possibly crucial info logged to help get running. 

for release downloads with its own steam_api(64).dll found next to exe (zip name will not have "nosteamworks" in it):
 - Extract files into main game dir so long as it doesn't interfere with any preexisting dll, if this is the case - make a folder to contain the games folder and then put the extracted uc-online files next to the game folder and set the GameExecutable line to include the folder in the path to its exe. 
 - Only difference with these versions are that there's no need to set the dll path, it will use its own dll from the Steamworks SDK and this makes it more universal than using the game's dll and not having the accounted for init functions for steam. As for everything else, just follow the above instructions for the config.ini and you'll be set.

After setting everything up, all that's left is to run the uc-online(64).exe to hopefully start the game and have no issues. 
On Windows, you will always get a scary red window warning you about it, this is only because it isn't signed. I don't want to and can't afford to pay that kinda money for it to be signed, it (as far as I remember) doesn't flag virustotal in the slightest, so if you downloaded this from someone else's repo and you're worried - just pass it through the site to be safe. If it's flagged, make sure to let me know on Discord so I can investigate for malware or anything malicious and do whatever I can about it. 

====================

## Branch / Version identification 

 - main - Windows, uses its own steam_api(64).dll from SDK to run the game exe, has a console window that lingers after closing game window.
 - main-noconsole - Unfortunately this slipped under my radar and is technically the same version, just without the extra path fixer code Uses its own steamworks implementation from the SDK.
 - main-noconsole-nosteamworks - Same thing, no console window, but uses the game's original steam_api(64).dll entirely to load the game as Spacewar. This is unreliable. Explained above.
 - main-nosteamworks - Same thing as the main branch, except for lacking Steamworks implementation and using original game steam_api(64).dll file to load.
 - linux - Linux variant of the main branch, uses its own libsteam_api.so. I believe it runs using a console window as well, I can't test right this moment.
 - linux-noconsole - Same thing but no console window.
 - linux-nosteamworks-steamclient - Credit goes to [Ahmedvflame](https://github.com/ahmedvflamez) for this, as he made it load from the default steamclient.so for x86 and x64 games from the main Steam installation dir found in "Ubuntu12_32" and the folder for x64 (i cannot recall it, i'm assuming "Ubuntu12_64"?). All I did to finish it was to make it just one runnable launcher for both architectures and being able to detect between the two. I decided on that since Linux Steam uses the same names for both variants, it's super easy to get them confused lol. But that made it easier to write out how to get it to tell between the two.
 - dll - I'm attempting to try and have it as just a dll to inject into the game instead of running a separate launcher to make it easier for running through Proton on Linux for [Deadboys Corner](https://GitHub.com/deadboy666) as a test currently. Still needs immense work to be done in order to get this functional at the very least.
 - dllwinmm - Another version of the dll branch that instead uses winmm.dll to inject and run the uc-online code all from using the game's original launcher. This version runs exceedingly well, though needs some polishing here and there desperately. Just download the built uc-online(64).dll or download the dllwinmm.zip from releases and drag and drop into a game folder with the launcher that has an import for winmm.dll, set up config snd run game. Everything should run fine on paper, Gmod worked fine. Same with Rivals of Aether.
 
====================

## Thanks and other notes:

Thank you all to everyone who's tried this out for themselves and gave their feedback on it and praised it and voiced their opinions on it as well on the cs.rin thread and tried helping out with ideas to make it better and better. Thank you to all of the friends I have and have made, mostly since trying to publicize this more by posting about it, out there that give me motivation to work on this and other similar projects of mine as well, and to just keep going on through life. I know this will be one day beyond my expectations and hopes and be way better than I initially thought when starting it. 

- Thank you to [Ahmedvflame](https://GitHub.com/ahmedvflamez) for helping me get this even working in the first place on Linux and testing it out for me whenever I would think I'd finally gotten it working, and eventually modifying the base Linux branch code very heavily to get it working completely using the steamclient.so found in the steam installation folder and finding the best method for using this while avoiding the libsteam_api.so entirely. 
- Thank you to [Lou](https://GitHub.com/Union-Crax) for letting me make this for us and use with our listings. And thank you for fixing the path issues when you were testing it with UnionCrax.Direct and figuring it out in the first place. Working with you and everyone else on the UC Team has been amazing, we've come so far with the site since I joined early sometime last year, thank you for giving me the opportunity to work with y'all and letting me make these sorts of projects for us to use and put out there to help our name become known even more online.
- Thank you to [Ace! _SL/S](https://GitHub.com/AceSLS) for trying to help me test it out and also helping me finally realize the reason why testing with Ahmed on GMod would not work when he connected to me, my firewall was causing issues. Also thank you for giving me motivation to make this work completely across the board on Linux as best as I can and to find the way to do it too. I'm glad this caught your interest and tickled your fancy and had you reach out. I really enjoy your presence and chatting with you, and I look forward to working on the projects we have in mind together and just becoming better friends. ^^ 
- Also, thank you to [Deadboys Corner](https://GitHub.com/deadboy666) for trying to help me out with the original C# code and trying to compile for Linux at that point in the projects life, and helping me understand more about .NET built projects and that even when compiled for Linux, they still use .dll files and sometimes even .exes even though they'll run as Linux binaries and work completely fine. And also for trying to give me a new idea for this by making it so it is used as a single .dll to rename to an import called on by the game to be loaded in proton to then be injected into the process and work as intended and expected without using the separate launcher at all. It requires much, much more work and does not work as it is in its current state. 
- Lastly, thank you EMPTERROR, for agreeing and pushing for me to make this open source. If you hadn't, this wouldn't be here and I would have likely never decided to improve it to be better and more appealing to whoever finds it and decides to try it. The switch to C++ from C# was the best decision I made and it's because of you telling me I should do this, you a real one G. 

To explain the name...
The name uc-online comes from having poor naming abilities (I can't come up with good names for anything) and also the group / site I work with, "UnionCrax". Its name being literally "unioncrax-online" lmao. This is because before I made it open source, it was going to likely be our own "online fix alternative", especially for games that they haven't worked with yet or at all.

If you have any issues with running, building from source, or working with it in any way, open an issue in the tab up top. Wanna talk in the discussions? Maybe share your ideas for this project, I'm open to new ideas and I'll get around to trying to make it a reality to the best of my abilities and making sure to credit whoever wrote about it properly.

One idea I had is to try making this MacOS compatible, but I have no clue how to, I don't even have a Mac of any kind nor do I have anything compatible with being Hackintosh'ed, so I'll leave that for someone else more capable than myself to try to work on for another time. 

~veeλnti<3

- https://union-crax.xyz/ - come check us out and maybe even put in a request!
- https://vee-anti.xyz/ - my personal site (made by Lou) I use to learn typescript and nextjs for random and silly additions that I want to try adding in lol. learn a bit more about me too!

====================

## Star History

<a href="https://www.star-history.com/?repos=veeeanti%2Fuc-online&type=date&legend=bottom-right">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/image?repos=veeeanti/uc-online&type=date&theme=dark&legend=bottom-right" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/image?repos=veeeanti/uc-online&type=date&legend=bottom-right" />
   <img alt="Star History Chart" src="https://api.star-history.com/image?repos=veeeanti/uc-online&type=date&legend=bottom-right" />
 </picture>
</a>
