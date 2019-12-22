#include <iomanip>
#include <sstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <physfs.h>

#include "command.h"
#include "game.h"
#include "gfx.h"
#include "menu.h"
#include "gamestate.h"
#include "vm.h"
#include "config.h"

MenuOption mainMenu[] = {
    { "Start Standard Game",    0,      0,                  MenuType::Choice },
    { "Start Custom Game",      0,      1,                  MenuType::Disabled },
    { "Load Saved Game",        0,      4,                  MenuType::Disabled },
    { "",                       0,      0,                  MenuType::Disabled },
    { "Resume Game",            0,      3,                  MenuType::Disabled },
    { "",                       0,      0,                  MenuType::Disabled },
    { "General Options",        0,      5,                  MenuType::Choice },
    { "View Keybindings",       0,      6,                  MenuType::Choice },
    { "",                       0,      0,                  MenuType::Disabled },
    { "Credits",                0,      7,                  MenuType::Choice },
    { "Quit",                   SDLK_q, menuQuit,           MenuType::Choice },
    { "",                       0,      menuEnd,            MenuType::Choice }
};

MenuOption optionsMenu[] = {
    { "Music Volume",           0,      menuMusicVolume,    MenuType::Value,    50, 0, MIX_MAX_VOLUME },
    { "Effects Volume",         0,      menuAudioVolume,    MenuType::Value,    50, 0, MIX_MAX_VOLUME },
    { "Demo Bool Option",       0,      0,                  MenuType::Bool },
    { "",                       0,      0,                  MenuType::Disabled },
    { "Save Changes",           0,      1,                  MenuType::Choice },
    { "Discard Changes",        0,      0,                  MenuType::Choice },
    { "",                       0,      menuEnd,            MenuType::Choice }
};

void doControlsList(System &system);

void doGameMenu(System &state) {
    int optBool = 1;

    GameState *gameState = nullptr;
    int defOpt = 0;

    state.playMusic(0);

    while (!state.wantsToQuit) {
        int choice = runMenu(state, mainMenu, defOpt);
        switch(choice) {
            case menuQuit:
                state.wantsToQuit = true;
                break;
            case 0:
                mainMenu[4].type = MenuType::Choice;
                defOpt = 4;
                if (gameState) delete gameState;
                gameState = new GameState(state.coreRNG, *state.vm);
                gameState->system = &state;
                state.game = gameState;
                gameState->getVM().runFunction("start");
                gameloop(state);
                state.playMusic(0);
                break;
            case 3:
                gameloop(state);
                state.playMusic(0);
                defOpt = 4;
                break;
            case 5: {
                int initialMusicVolume = Mix_VolumeMusic(-1);
                int initialAudioVolume = Mix_Volume(-1, -1);
                optionsMenu[0].value = initialMusicVolume;
                optionsMenu[1].value = initialAudioVolume;
                optionsMenu[2].value = optBool;
                if (runMenu(state, optionsMenu)) {
                    state.setMusicVolume(optionsMenu[0].value);
                    state.setAudioVolume(optionsMenu[1].value);
                    state.config->set("music", optionsMenu[0].value);
                    state.config->set("audio", optionsMenu[1].value);
                    optBool = optionsMenu[2].value;
                } else {
                    state.setAudioVolume(initialAudioVolume);
                    state.setMusicVolume(initialMusicVolume);
                }
                break; }
            case 6:
                doControlsList(state);
                break;
            case 7:
                doCredits(state);
                defOpt = 9;
                break;
        }
    }

    if (gameState) delete gameState;
}

static std::string makeCommandEntry(const CommandDef &cmd) {
    std::stringstream left, right;
    left << cmd.command;
    if (cmd.direction != Dir::None) {
        left << ' ' << cmd.direction;
    }
    right << std::setw(15) << left.str();
    right << ": ";
    if (cmd.mod != 0) {
        if (cmd.mod & KMOD_LSHIFT)  right << "left shift + ";
        if (cmd.mod & KMOD_LCTRL)   right << "left ctrl + ";
        if (cmd.mod & KMOD_LALT)    right << "left alt + ";
        if (cmd.mod & KMOD_LGUI)    right << "left gui + ";
        if (cmd.mod & KMOD_RSHIFT)  right << "right shift + ";
        if (cmd.mod & KMOD_RCTRL)   right << "right ctrl + ";
        if (cmd.mod & KMOD_RALT)    right << "right alt + ";
        if (cmd.mod & KMOD_RGUI)    right << "right gui + ";
    }
    right << SDL_GetKeyName(cmd.key);
    return right.str();
}

void doControlsList(System &system) {
    std::vector<std::string> list;

    list.push_back("        * MAP COMMANDS *");
    for (int i = 0; gameCommands[i].command != Command::None; ++i ) {
        list.push_back(makeCommandEntry(gameCommands[i]));
    }

    list.push_back("");
    list.push_back("    * PARTY SCREEN COMMANDS *");
    for (int i = 0; partyCommands[i].command != Command::None; ++i ) {
        list.push_back(makeCommandEntry(partyCommands[i]));
    }

    list.push_back("");
    list.push_back("      * COMBAT COMMANDS *");
    for (int i = 0; combatCommands[i].command != Command::None; ++i ) {
        list.push_back(makeCommandEntry(combatCommands[i]));
    }
    gfx_RunInfo(system, list, false);
}