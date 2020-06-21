#ifndef MENU_H
#define MENU_H

#include <string>
#include <vector>

class System;

const int menuNone = -1;
const int menuQuit = 10000;
const int menuClose = 10001;

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
    MenuOption(int code, const std::string &text, MenuType type);
    MenuOption(int code, const std::string &text, int value, int step, int min, int max, void (*callback)(System&, int) = nullptr);

    int code;
    std::string text;
    MenuType type;
    int value;
    int step, min, max;
    void (*callback)(System&, int);

    MenuRect rect;
};

class Menu {
public:
    int run(System &state);
    void add(const MenuOption &newOption);
    bool empty() const;
    void setSelectedByCode(int code);
    MenuOption& getOptionByCode(int code);
    MenuOption& getOptionByCoord(int x, int y);
    MenuOption& getSelected();
    void next();
    void previous();

private:
    std::vector<MenuOption> options;
    unsigned selected;
};

void gfx_RunInfo(System &state, const std::vector<std::string> &text, bool autoscroll);
void showVersion(System &state);

#endif
