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

MenuOption mainMenu[] = {
    { "Start Standard Game",    0,      0,                  MenuType::Choice },
    { "Start Custom Game",      0,      1,                  MenuType::Disabled },
    { "Load Saved Game",        0,      4,                  MenuType::Disabled },
    { "",                       0,      0,                  MenuType::Disabled },
    { "Resume Game",            0,      3,                  MenuType::Disabled },
    { "",                       0,      0,                  MenuType::Disabled },
    { "General Options",        0,      5,                  MenuType::Choice },
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

void doGameMenu(System &state) {
    int optBool = 1;
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
            case 7:
                doCredits(state);
                defOpt = 8;
                break;
        }
    }
}
