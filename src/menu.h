#ifndef MENU_H
#define MENU_H

#include <string>
#include <vector>

class System;

const int menuQuit = -1;
const int menuEnd = -2;
const int menuMusicVolume = -3;
const int menuAudioVolume = -4;

enum class MenuType {
    Choice,
    Bool,
    Value,
    Disabled,
};

struct MenuRect {
    int x, y, w, h;
};

struct MenuOption {
    std::string text;
    int key;
    int code;
    MenuType type;
    int value;
    int min, max;

    MenuRect rect;
};

int runMenu(System &state, MenuOption *menu, int defaultOption = 0);
void gfx_RunInfo(System &state, const std::vector<std::string> &text, bool autoscroll);
void showVersion(System &state);

#endif
