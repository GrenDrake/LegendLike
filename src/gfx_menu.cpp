#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "command.h"
#include "config.h"
#include "game.h"
#include "gfx.h"
#include "menu.h"


static int checkPosInMenu(MenuOption *menu, int x, int y) {
    for (int i = 0; menu[i].code != menuEnd; ++i) {
        if (x < menu[i].rect.x || y < menu[i].rect.y) continue;
        if (x >= menu[i].rect.x + menu[i].rect.w) continue;
        if (y >= menu[i].rect.y + menu[i].rect.h) continue;
        return i;
    }
    return -1;
}

void showVersion(System &state) {
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(state.renderer, &screenWidth, &screenHeight);
    std::string text = versionString();
    state.smallFont->out(
            screenWidth - state.smallFont->getCharWidth() * (text.size() + 1),
            screenHeight - state.smallFont->getLineHeight(),
            text);
}

int runMenu(System &state, MenuOption *menu, int defaultOption) {
    bool showInfo = false;
    const std::string writeDir = state.config->getString("writeDir", "unknown");
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(state.renderer, &screenWidth, &screenHeight);

    int option = defaultOption;

    SDL_Texture *logoArt = state.getImage("logo.png");

    int logoWidth, logoHeight;
    SDL_QueryTexture(logoArt, nullptr, nullptr, &logoWidth, &logoHeight);

    int menuSize = 0;
    for (int i = 0; menu[i].code != menuEnd; ++i) {
        ++menuSize;
        menu[i].rect.w = menu[i].text.length() * state.smallFont->getCharWidth();
        menu[i].rect.h = state.smallFont->getCharHeight();
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
        for (int i = 0; menu[i].code != menuEnd; ++i) {
            if (menu[i].text.empty()) {
                extraSpace += -(state.smallFont->getLineHeight() / 2);
            } else {
                if (option == i) {
                    colour = &current;
                    state.smallFont->out(100, optionsTop + i * state.smallFont->getLineHeight() + extraSpace, "-> ");
                } else {
                    if (menu[i].type == MenuType::Disabled) colour = &disabled;
                    else                                    colour = &other;
                }

                menu[i].rect.x = 100+threeCharWidth;
                menu[i].rect.y = optionsTop + i * state.smallFont->getLineHeight() + extraSpace;

                if (menu[i].type == MenuType::Bool) {
                    std::string msg = menu[i].text + "  ";
                    if (menu[i].value)  msg += "\x06";
                    else                msg += "\x05";
                    menu[i].rect.w = msg.length() * state.smallFont->getCharWidth();
                    state.smallFont->out(menu[i].rect.x, menu[i].rect.y, *colour + msg);
                } else if (menu[i].type == MenuType::Value) {
                    std::string msg = menu[i].text + " ";
                    msg += "\x03 " + std::to_string(menu[i].value) + " \x04";
                    menu[i].rect.w = msg.length() * state.smallFont->getCharWidth();
                    state.smallFont->out(menu[i].rect.x, menu[i].rect.y, *colour + msg);
                } else {
                    state.smallFont->out(menu[i].rect.x, menu[i].rect.y, *colour + menu[i].text);
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
                int choice = checkPosInMenu(menu, event.button.x, event.button.y);
                if (choice >= 0) {
                    if (menu[choice].type == MenuType::Choice) {
                        return menu[choice].code;
                    } else if (menu[choice].type == MenuType::Bool) {
                        menu[choice].value = !menu[choice].value;
                    }
                    if (menu[choice].type != MenuType::Disabled && !menu[choice].text.empty()) {
                        option = choice;
                    }
                }
                continue;
            }
            const CommandDef &cmd = getCommand(state, event, menuCommands);
            switch(cmd.command) {
                case Command::Quit:
                    state.wantsToQuit = true;
                    return menuQuit;
                case Command::Interact:
                    if (menu[option].type == MenuType::Choice) {
                        return menu[option].code;
                    } else if (menu[option].type == MenuType::Bool) {
                        menu[option].value = !menu[option].value;
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
                            if (menu[option].type == MenuType::Bool) {
                                menu[option].value = !menu[option].value;
                            } else if (menu[option].type == MenuType::Value) {
                                ++menu[option].value;
                                if (menu[option].value > menu[option].max) {
                                    menu[option].value = menu[option].min;
                                }
                                if (menu[option].callback) {
                                    menu[option].callback(state, menu[option].value);
                                }
                            }
                            break;
                        case Dir::West:
                            if (menu[option].type == MenuType::Bool) {
                                menu[option].value = !menu[option].value;
                            } else if (menu[option].type == MenuType::Value) {
                                --menu[option].value;
                                if (menu[option].value < menu[option].min) {
                                    menu[option].value = menu[option].max;
                                }
                                if (menu[option].callback) {
                                    menu[option].callback(state, menu[option].value);
                                }
                            }
                            break;
                        case Dir::North:
                            do {
                                --option;
                                if (option < 0) option = menuSize - 1;
                            } while (menu[option].type == MenuType::Disabled || menu[option].text.empty());
                            state.playEffect(0);
                            break;
                        case Dir::South:
                            do {
                                ++option;
                                if (option >= menuSize) option = 0;
                            } while (menu[option].type == MenuType::Disabled || menu[option].text.empty());
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

void gfx_RunInfo(System &state, const std::vector<std::string> &text, bool autoscroll) {
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
