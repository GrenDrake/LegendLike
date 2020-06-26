#include <iomanip>
#include <sstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <physfs.h>

#include "command.h"
#include "game.h"
#include "gamestate.h"
#include "gfx_menu.h"
#include "vm.h"
#include "config.h"
#include "actor.h"
#include "random.h"
#include "textutil.h"

std::vector<std::string> creditsText;
static void loadCredits();

static std::string getRandomName(Random &rng) {
    switch(rng.next32() % 21) {
        case  0: return "Eadweard";
        case  1: return "Gery";
        case  2: return "Cuthbaeld";
        case  3: return "Stephye";
        case  4: return "Guthre";
        case  5: return "Gauwalt";
        case  6: return "Witheab";
        case  7: return "Walda";
        case  8: return "Jamund";
        case  9: return "Lafa";
        case 10: return "Wise";
        case 11: return "Mera";
        case 12: return "Burne";
        case 13: return "Elith";
        case 14: return "Eryel";
        case 15: return "Frythiue";
        case 16: return "Sane";
        case 17: return "Aengyth";
        case 18: return "Eyter";
        case 19: return "Ridget";
        case 20: return "Alyn";
    }
    return "Fred";
}

const int menuStartStandard  = 1;
const int menuStartCustom    = 2;
const int menuLoadSaved      = 3;
const int menuResumeGame     = 4;
const int menuGeneralOptions = 5;
const int menuCredits        = 6;

void doOptionsMenu(GameState &state);

void doGameMenu(GameState &state) {
    static Menu mainMenu;
    if (mainMenu.empty()) {
        mainMenu.add(MenuOption(menuStartStandard,  "Start Standard Game",    MenuType::Choice));
        mainMenu.add(MenuOption(menuStartCustom,    "Start Custom Game",      MenuType::Disabled));
        mainMenu.add(MenuOption(menuLoadSaved,      "Load Saved Game",        MenuType::Disabled));
        mainMenu.add(MenuOption(menuNone,           "",                       MenuType::Disabled));
        mainMenu.add(MenuOption(menuResumeGame,     "Resume Game",            MenuType::Disabled));
        mainMenu.add(MenuOption(menuNone,           "",                       MenuType::Disabled));
        mainMenu.add(MenuOption(menuGeneralOptions, "General Options",        MenuType::Choice));
        mainMenu.add(MenuOption(menuNone,           "",                       MenuType::Disabled));
        mainMenu.add(MenuOption(menuCredits,        "Credits",                MenuType::Choice));
        mainMenu.add(MenuOption(menuQuit,           "Quit",                   MenuType::Choice));
    }
    state.playMusic(0);

    while (!state.wantsToQuit) {
        int choice = mainMenu.run(state);
        switch(choice) {
            case menuQuit:
                state.wantsToQuit = true;
                break;
            case menuStartStandard: {
                if (state.gameInProgress) {
                    if (!gfx_Confirm(state, "Start a new game?", "This will abandon your current game!", true)) {
                        break;
                    }
                }
                state.gameInProgress = true;
                mainMenu.getOptionByCode(menuResumeGame).type = MenuType::Choice;
                state.reset();
                Actor *player = state.getPlayer();
                player->name = getRandomName(state.coreRNG);
                player->hasProperName = true;
                gfx_EditText(state, "Name?", state.getPlayer()->name, 16);
                state.vm->runFunction("start");
                gameloop(state);
                mainMenu.setSelectedByCode(menuResumeGame);
                state.playMusic(0);
                break; }
            case menuResumeGame:
            case menuClose:
                if (state.gameInProgress) {
                    gameloop(state);
                    state.playMusic(0);
                }
                break;
            case menuGeneralOptions:
                doOptionsMenu(state);
                break;
            case menuCredits:
                if (creditsText.empty()) loadCredits();
                gfx_RunInfo(state, creditsText, true);
                break;
        }

        if (state.wantsToQuit && state.gameInProgress) {
            if (!gfx_Confirm(state, "Quit?", "This will abandon your current game!", true)) {
                state.wantsToQuit = false;;
            }
        }
    }
}

void loadCredits() {
    SDL_version version;
    const SDL_version *pVersion;
    PHYSFS_Version physfsVersion;
    std::stringstream line;

    line << GAME_NAME << ' ' << GAME_MAJOR << '.' << GAME_MINOR << '.' << GAME_PATCH;
    creditsText.push_back(line.str());
    line.str(""); line.clear();
    creditsText.push_back("");

    SDL_GetVersion(&version);
    line << "SDL " << ' ' << static_cast<int>(version.major) << '.' << static_cast<int>(version.minor) << '.' << static_cast<int>(version.patch);
    creditsText.push_back(line.str());
    line.str(""); line.clear();

    pVersion = IMG_Linked_Version();
    line << "SDL_image " << ' ' << static_cast<int>(pVersion->major) << '.' << static_cast<int>(pVersion->minor) << '.' << static_cast<int>(pVersion->patch);
    creditsText.push_back(line.str());
    line.str(""); line.clear();

    pVersion = Mix_Linked_Version();
    line << "SDL_mixer " << ' ' << static_cast<int>(pVersion->major) << '.' << static_cast<int>(pVersion->minor) << '.' << static_cast<int>(pVersion->patch);
    creditsText.push_back(line.str());
    line.str(""); line.clear();

    PHYSFS_getLinkedVersion(&physfsVersion);
    line << "PHYSFS " << ' ' << static_cast<int>(physfsVersion.major) << '.' << static_cast<int>(physfsVersion.minor) << '.' << static_cast<int>(physfsVersion.patch);
    creditsText.push_back(line.str());
    line.str(""); line.clear();

    creditsText.push_back("libfov 1.0.4");
    creditsText.push_back("");
    creditsText.push_back("");

    char *rawFile = slurpFile("credits.txt");
    std::vector<std::string> lines = explode(rawFile, "\n");
    delete[] rawFile;
    for (const std::string &line : lines) {
        if (line == "#")    creditsText.push_back("");
        else                creditsText.push_back(line);
    }
}
