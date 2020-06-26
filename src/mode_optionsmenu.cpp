#include <iomanip>
#include <sstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <physfs.h>

#include "game.h"
#include "gamestate.h"
#include "gfx_menu.h"
#include "config.h"

void adjustMusicVolume(GameState &system, int value);
void adjustAudioVolume(GameState &system, int value);
void adjustFontScale(GameState &system, int value);


const int menuMusic         = 0;
const int menuAudio         = 1;
const int menuTile          = 2;
const int menuFont          = 3;
const int menuFullscreen    = 4;
const int menuShowDiceRolls = 5;
const int menuAnimDelay     = 6;
const int menuSaveChanges   = 7;
const int menuDiscard       = 8;


void adjustMusicVolume(GameState &system, int value) {
    Mix_VolumeMusic(value);
}

void adjustAudioVolume(GameState &system, int value) {
    Mix_Volume(-1, value);
    system.playEffect(0);
}

void adjustFontScale(GameState &system, int value) {
    system.setFontScale(value);
}

void doOptionsMenu(GameState &state) {
    static Menu optionsMenu;
    if (optionsMenu.empty()) {
        optionsMenu.add(MenuOption(menuMusic,          "Music Volume",         50, 1, 0, MIX_MAX_VOLUME, adjustMusicVolume));
        optionsMenu.add(MenuOption(menuAudio,          "Effects Volume",       50, 1, 0, MIX_MAX_VOLUME, adjustAudioVolume));
        optionsMenu.add(MenuOption(menuTile,           "Tile Scale",           1, 1, 1, 10));
        optionsMenu.add(MenuOption(menuFont,           "Font Scale",           1, 1, 1, 10, adjustFontScale));
        optionsMenu.add(MenuOption(menuFullscreen,     "Fullscreen",           MenuType::Bool));
        optionsMenu.add(MenuOption(menuShowDiceRolls,  "Show Dice Rolls",      MenuType::Bool));
        optionsMenu.add(MenuOption(menuAnimDelay,      "Animation Delay (ms)", 25, 25, 50, 500));
        optionsMenu.add(MenuOption(menuNone,           "",                     MenuType::Disabled));
        optionsMenu.add(MenuOption(menuSaveChanges,    "Save Changes",         MenuType::Choice));
        optionsMenu.add(MenuOption(menuDiscard,        "Discard Changes",      MenuType::Choice));
    }

    const int initialMusicVolume = Mix_VolumeMusic(-1);
    const int initialAudioVolume = Mix_Volume(-1, -1);
    const int initialFontScale   = state.config->getInt("font_scale", 1);
    optionsMenu.getOptionByCode(menuMusic).value = initialMusicVolume;
    optionsMenu.getOptionByCode(menuAudio).value = initialAudioVolume;
    optionsMenu.getOptionByCode(menuTile).value = state.config->getInt("tile_scale", 1);
    optionsMenu.getOptionByCode(menuFont).value = initialFontScale;
    optionsMenu.getOptionByCode(menuFullscreen).value = state.config->getBool("fullscreen", false);
    optionsMenu.getOptionByCode(menuShowDiceRolls).value = state.config->getBool("showrolls", false);
    optionsMenu.getOptionByCode(menuAnimDelay).value = state.config->getInt("anim_delay", 100);
    optionsMenu.setSelectedByCode(menuMusic);

    state.playMusic(0);

    int wantsToSave = false;
    while (!state.wantsToQuit && !wantsToSave) {
        int choice = optionsMenu.run(state);
        switch(choice) {
            case menuQuit:
                state.wantsToQuit = true;
                break;
            case menuDiscard:
            case menuClose:
                state.setAudioVolume(initialAudioVolume);
                state.setMusicVolume(initialMusicVolume);
                state.setFontScale(initialFontScale);
                return;
            case menuSaveChanges:
                wantsToSave = true;
                break;;
        }
    }

    if (wantsToSave) {
        bool wantsFullscreen = optionsMenu.getOptionByCode(menuFullscreen).value;
        SDL_SetWindowFullscreen(state.window, wantsFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        state.config->set("music", optionsMenu.getOptionByCode(menuMusic).value);
        state.config->set("audio", optionsMenu.getOptionByCode(menuAudio).value);
        state.config->set("tile_scale", optionsMenu.getOptionByCode(menuTile).value);
        state.config->set("font_scale", optionsMenu.getOptionByCode(menuFont).value);
        state.config->set("fullscreen", optionsMenu.getOptionByCode(menuFullscreen).value ? 1 : 0);
        state.config->set("showrolls",  optionsMenu.getOptionByCode(menuShowDiceRolls).value ? 1 : 0);
        state.config->set("anim_delay", optionsMenu.getOptionByCode(menuAnimDelay).value);
    }
}
