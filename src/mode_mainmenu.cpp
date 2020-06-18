#include <iomanip>
#include <sstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <physfs.h>

#include "command.h"
#include "game.h"
#include "gfx.h"
#include "menu.h"
#include "vm.h"
#include "config.h"
#include "creature.h"
#include "random.h"
#include "textutil.h"

std::vector<std::string> creditsText;
static void loadCredits();
void adjustMusicVolume(System &system, int value);
void adjustAudioVolume(System &system, int value);
void adjustFontScale(System &system, int value);

MenuOption mainMenu[] = {
    { "Start Standard Game",    0,                  MenuType::Choice },
    { "Start Custom Game",      1,                  MenuType::Disabled },
    { "Load Saved Game",        4,                  MenuType::Disabled },
    { "",                       0,                  MenuType::Disabled },
    { "Resume Game",            3,                  MenuType::Disabled },
    { "",                       0,                  MenuType::Disabled },
    { "General Options",        5,                  MenuType::Choice },
    { "",                       0,                  MenuType::Disabled },
    { "Credits",                7,                  MenuType::Choice },
    { "Quit",                   menuQuit,           MenuType::Choice },
    { "",                       menuEnd,            MenuType::Choice }
};

const int menuMusicIndex = 0;
const int menuAudioIndex = 1;
const int menuTileIndex = 2;
const int menuFontIndex = 3;
const int menuFullscreenIndex = 4;
const int menuShowDiceRolls = 5;
MenuOption optionsMenu[] = {
    { "Music Volume",           0,                  MenuType::Value,    50, 0, MIX_MAX_VOLUME, adjustMusicVolume },
    { "Effects Volume",         0,                  MenuType::Value,    50, 0, MIX_MAX_VOLUME, adjustAudioVolume },
    { "Tile Scale",             0,                  MenuType::Value,    1,  1, 10 },
    { "Font Scale",             0,                  MenuType::Value,    1,  1, 10, adjustFontScale },
    { "Fullscreen",             0,                  MenuType::Bool,     0,  0, 0  },
    { "Show Dice Rolls",        0,                  MenuType::Bool,     0,  0, 0  },
    { "",                       0,                  MenuType::Disabled },
    { "Save Changes",           1,                  MenuType::Choice },
    { "Discard Changes",        menuDiscard,        MenuType::Choice },
    { "",                       menuEnd,            MenuType::Choice }
};

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

void adjustMusicVolume(System &system, int value) {
    Mix_VolumeMusic(value);
}

void adjustAudioVolume(System &system, int value) {
    Mix_Volume(-1, value);
    system.playEffect(0);
}

void adjustFontScale(System &system, int value) {
    system.setFontScale(value);
}

void doGameMenu(System &state) {
    int defOpt = 0;
    bool gameInProgress = false;

    state.playMusic(0);

    while (!state.wantsToQuit) {
        int choice = runMenu(state, mainMenu, defOpt);
        switch(choice) {
            case menuQuit:
                state.wantsToQuit = true;
                break;
            case 0:
                gameInProgress = true;
                mainMenu[4].type = MenuType::Choice;
                defOpt = 4;
                state.reset();
                state.getPlayer()->name = getRandomName(state.coreRNG);
                gfx_EditText(state, "Name?", state.getPlayer()->name, 16);
                state.vm->runFunction("start");
                gameloop(state);
                state.playMusic(0);
                break;
            case 3:
            case menuClose:
                if (gameInProgress) {
                    gameloop(state);
                    state.playMusic(0);
                    defOpt = 4;
                }
                break;
            case 5: {
                int initialMusicVolume = Mix_VolumeMusic(-1);
                int initialAudioVolume = Mix_Volume(-1, -1);
                int initialFontScale = state.config->getInt("font_scale", 1);
                optionsMenu[menuMusicIndex].value = initialMusicVolume;
                optionsMenu[menuAudioIndex].value = initialAudioVolume;
                optionsMenu[menuTileIndex].value = state.config->getInt("tile_scale", 1);
                optionsMenu[menuFontIndex].value = initialFontScale;
                optionsMenu[menuFullscreenIndex].value = state.config->getBool("fullscreen", false);
                optionsMenu[menuShowDiceRolls].value = state.config->getBool("showrolls", false);
                int result = runMenu(state, optionsMenu);
                if (result >= 0) {
                    state.setMusicVolume(optionsMenu[menuMusicIndex].value);
                    state.setAudioVolume(optionsMenu[menuAudioIndex].value);
                    state.setFontScale(optionsMenu[menuFontIndex].value);
                    SDL_SetWindowFullscreen(state.window,
                            optionsMenu[menuFullscreenIndex].value ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                    state.config->set("music", optionsMenu[menuMusicIndex].value);
                    state.config->set("audio", optionsMenu[menuAudioIndex].value);
                    state.config->set("tile_scale", std::to_string(optionsMenu[menuTileIndex].value));
                    state.config->set("font_scale", std::to_string(optionsMenu[menuFontIndex].value));
                    state.config->set("fullscreen", std::to_string(optionsMenu[menuFullscreenIndex].value ? 1 : 0));
                    state.config->set("showrolls",  std::to_string(optionsMenu[menuShowDiceRolls].value ? 1 : 0));
                } else {
                    state.setAudioVolume(initialAudioVolume);
                    state.setMusicVolume(initialMusicVolume);
                    state.setFontScale(initialFontScale);
                }
                if (result == menuQuit) state.wantsToQuit = true;
                break; }
            case 7:
                if (creditsText.empty()) loadCredits();
                gfx_RunInfo(state, creditsText, true);
                defOpt = 8;
                break;
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
