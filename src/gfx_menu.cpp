#include <sstream>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "command.h"
#include "config.h"
#include "game.h"
#include "gamestate.h"
#include "gfx_menu.h"

MenuOption badOption{ -1, "BAD_OPTION", MenuType::Disabled };

MenuOption::MenuOption(int code, const std::string &text, MenuType type)
: code(code), text(text), type(type)
{ }

MenuOption::MenuOption(int code, const std::string &text, int value, int step, int min, int max, void (*callback)(GameState&, int))
: code(code), text(text), type(MenuType::Value),
  value(value), step(step), min(min), max(max), callback(callback)
{ }

void Menu::add(const MenuOption &newOption) {
    options.push_back(newOption);
}

bool Menu::empty() const {
    return options.empty();
}

void Menu::setSelectedByCode(int code) {
    for (unsigned i = 0; i < options.size(); ++i) {
        if (options[i].code == code) {
            selected = i;
            return;
        }
    }
}

MenuOption& Menu::getOptionByCode(int code) {
    for (MenuOption &option : options) {
        if (option.code == code) return option;
    }
    return badOption;
}

MenuOption& Menu::getOptionByCoord(int x, int y) {
    for (MenuOption &option : options) {
        if (x <  option.rect.x || y < option.rect.y) continue;
        if (x >= option.rect.x + option.rect.w) continue;
        if (y >= option.rect.y + option.rect.h) continue;
        return option;
    }
    return badOption;
}

MenuOption& Menu::getSelected() {
    return options[selected];
}

void Menu::next() {
    do {
        ++selected;
        if (selected >= options.size()) selected = 0;
    } while (options[selected].type == MenuType::Disabled || options[selected].text.empty());
}

void Menu::previous() {
    do {
        if (selected == 0) selected = options.size() - 1;
        else --selected;
    } while (options[selected].type == MenuType::Disabled || options[selected].text.empty());
}


void showVersion(GameState &state) {
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(state.renderer, &screenWidth, &screenHeight);
    std::string text = versionString();
    state.smallFont->out(
            screenWidth - state.smallFont->getCharWidth() * (text.size() + 1),
            screenHeight - state.smallFont->getLineHeight(),
            text);
    std::stringstream gameVersionS;
    gameVersionS << state.gameName << " [" << std::hex << state.gameId;
    gameVersionS << "] " << std::dec << state.majorVersion << '.' << state.minorVersion;
    std::string gameVersion = gameVersionS.str();

    state.smallFont->out(
            screenWidth - state.smallFont->getCharWidth() * (gameVersion.size() + 1),
            screenHeight - state.smallFont->getLineHeight() * 2,
            gameVersion);
}

int Menu::run(GameState &state) {
    bool showInfo = false;
    const std::string writeDir = state.config->getString("writeDir", "unknown");
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(state.renderer, &screenWidth, &screenHeight);

    SDL_Texture *logoArt = state.getImage("logo.png");

    int logoWidth, logoHeight;
    SDL_QueryTexture(logoArt, nullptr, nullptr, &logoWidth, &logoHeight);

    for (MenuOption &row : options) {
        row.rect.w = row.text.length() * state.smallFont->getCharWidth();
        row.rect.h = state.smallFont->getCharHeight();
    }

    std::string current  = "\x1B\x02\x7F\xFF\x7F";
    std::string disabled = "\x1B\x02\x7F\x7F\x7F";
    std::string other    = "\x1B\x02\xFF\xFF\xFF";
    std::string *colour = &other;

    const int threeCharWidth = state.smallFont->getCharWidth() * 3;
    SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    while (1) {
        int extraSpace = 0;
        SDL_RenderClear(state.renderer);
        SDL_Rect logoDest = { (screenWidth - logoWidth) / 2, 20, logoWidth, logoHeight };
        SDL_RenderCopy(state.renderer, logoArt, nullptr, &logoDest);

        const int optionsTop = logoHeight + 60 + 20;
        for (unsigned i = 0; i < options.size(); ++i) {
            MenuOption &row = options[i];
            if (row.text.empty()) {
                extraSpace += -(state.smallFont->getLineHeight() / 2);
            } else {
                if (selected == i) {
                    colour = &current;
                    state.smallFont->out(100, optionsTop + i * state.smallFont->getLineHeight() + extraSpace, "-> ");
                } else {
                    if (row.type == MenuType::Disabled) colour = &disabled;
                    else                                    colour = &other;
                }

                row.rect.x = 100+threeCharWidth;
                row.rect.y = optionsTop + i * state.smallFont->getLineHeight() + extraSpace;

                if (row.type == MenuType::Bool) {
                    std::string msg = row.text + "  ";
                    if (row.value)  msg += "\x06";
                    else                msg += "\x05";
                    row.rect.w = msg.length() * state.smallFont->getCharWidth();
                    state.smallFont->out(row.rect.x, row.rect.y, *colour + msg);
                } else if (row.type == MenuType::Value) {
                    std::string msg = row.text + " ";
                    msg += "\x03 " + std::to_string(row.value) + " \x04";
                    row.rect.w = msg.length() * state.smallFont->getCharWidth();
                    state.smallFont->out(row.rect.x, row.rect.y, *colour + msg);
                } else {
                    state.smallFont->out(row.rect.x, row.rect.y, *colour + row.text);
                }
            }
        }

        if (showInfo) {
            state.smallFont->out(0, screenHeight - state.smallFont->getCharHeight(), writeDir);
        }

        showVersion(state);
        state.advanceFrame();
        SDL_RenderPresent(state.renderer);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_MOUSEBUTTONUP) {
                MenuOption &choice = getOptionByCoord(event.button.x, event.button.y);
                if (choice.code >= 0) {
                    if (choice.type == MenuType::Choice) {
                        return choice.code;
                    } else if (choice.type == MenuType::Bool) {
                        choice.value = !choice.value;
                    }
                }
                continue;
            }
            const CommandDef &cmd = getCommand(state, event, menuCommands);
            MenuOption &current = options[selected];
            switch(cmd.command) {
                case Command::Quit:
                    state.wantsToQuit = true;
                    return menuQuit;
                case Command::Interact:
                    if (current.type == MenuType::Choice) {
                        return current.code;
                    } else if (current.type == MenuType::Bool) {
                        current.value = !current.value;
                    }
                    break;
                case Command::Debug_ShowInfo:
                    showInfo = !showInfo;
                    SDL_SetClipboardText(writeDir.c_str());
                    break;
                case Command::Close:
                    return menuClose;
                case Command::Move:
                    switch(cmd.direction) {
                        case Dir::East:
                            if (current.type == MenuType::Bool) {
                                current.value = !current.value;
                            } else if (current.type == MenuType::Value) {
                                current.value += current.step;
                                if (current.value > current.max) {
                                    current.value = current.min;
                                }
                                if (current.callback) {
                                    current.callback(state, current.value);
                                }
                            }
                            break;
                        case Dir::West:
                            if (current.type == MenuType::Bool) {
                                current.value = !current.value;
                            } else if (current.type == MenuType::Value) {
                                current.value -= current.step;
                                if (current.value < current.min) {
                                    current.value = current.max;
                                }
                                if (current.callback) {
                                    current.callback(state, current.value);
                                }
                            }
                            break;
                        case Dir::North:
                            previous();
                            state.playEffect(0);
                            break;
                        case Dir::South:
                            next();
                            state.playEffect(0);
                            break;
                        default:
                            /* we don't need to handle the other directions */
                            break;
                    }
                default:
                    /* we don't need to handle the other commands */
                    break;
            }
        }
    }
}

void gfx_RunInfo(GameState &state, const std::vector<std::string> &text, bool autoscroll) {
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(state.renderer, &screenWidth, &screenHeight);
    const int lineHeight = state.smallFont->getLineHeight();
    const int numLines = text.size();
    int firstLineY = 20 + lineHeight;
    int maxLines = (screenHeight - firstLineY) / lineHeight - 2;
    bool pauseScroll = !autoscroll;
    int firstLine = -maxLines + 1, subScroll = 0;
    SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    const std::string closeLine = "ESCAPE to close";
    while (!state.wantsToQuit) {
        if (!pauseScroll) {
            ++subScroll;
            while (subScroll >= lineHeight) {
                if (firstLine < numLines)   ++firstLine;
                else                        firstLine = -maxLines;
                subScroll -= lineHeight;
            }
        }

        SDL_RenderClear(state.renderer);
        for (int line = 0; line < maxLines; ++line) {
            const int realLine = line + firstLine;
            if (realLine < 0) continue;
            if (realLine >= numLines) break;
            state.smallFont->out(100, firstLineY + line * state.smallFont->getLineHeight() - subScroll, text[realLine]);
        }

        const int closeLeft = screenWidth - closeLine.size() * state.tinyFont->getCharWidth();
        const int closeTop  = screenHeight - state.tinyFont->getLineHeight();
        state.tinyFont->out(closeLeft, closeTop, closeLine);
        state.advanceFrame();
        SDL_RenderPresent(state.renderer);

        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            const CommandDef &cmd = getCommand(state, event, infoCommands);
            switch(cmd.command) {
                case Command::Quit:
                    state.wantsToQuit = true;
                    return;
                case Command::Move:
                    if (cmd.direction == Dir::North) {
                        if (firstLine > 0) --firstLine;
                    } else if (cmd.direction == Dir::South) {
                        if (firstLine < numLines - maxLines) {
                            ++firstLine;
                        }
                    }
                    break;
                case Command::Interact:
                    pauseScroll = !pauseScroll;
                    break;
                case Command::Close:
                    return;
                default:
                    /* we don't need to handle the other commands */
                    break;
            }
        }
    }
}
