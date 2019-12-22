#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

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
    std::string text = versionString();
    state.smallFont->out(
            screenWidth - state.smallFont->getCharWidth() * (text.size() + 1),
            screenHeight - state.smallFont->getLineHeight(),
            text);
}

int runMenu(System &state, MenuOption *menu, int defaultOption) {
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

        showVersion(state);
        SDL_RenderPresent(state.renderer);

        SDL_Event event;
        SDL_WaitEvent(&event);
        switch(event.type) {
            case SDL_QUIT:
                state.wantsToQuit = true;
                return menuQuit;
            case SDL_MOUSEBUTTONUP: {
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
                break; }
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_KP_ENTER:
                    case SDLK_RETURN:
                    case SDLK_RETURN2:
                    case SDLK_SPACE:
                        if (menu[option].type == MenuType::Choice) {
                            return menu[option].code;
                        } else if (menu[option].type == MenuType::Bool) {
                            menu[option].value = !menu[option].value;
                        }
                        break;
                    case SDLK_a:
                    case SDLK_RIGHT:
                        if (menu[option].type == MenuType::Bool) {
                            menu[option].value = !menu[option].value;
                        } else if (menu[option].type == MenuType::Value) {
                            ++menu[option].value;
                            if (menu[option].value > menu[option].max) {
                                menu[option].value = menu[option].min;
                            }
                            if (menu[option].code == menuMusicVolume) {
                                Mix_VolumeMusic(menu[option].value);
                            } else if (menu[option].code == menuAudioVolume) {
                                Mix_Volume(-1, menu[option].value);
                                state.playEffect(0);
                            }
                        }
                        break;
                    case SDLK_d:
                    case SDLK_LEFT:
                        if (menu[option].type == MenuType::Bool) {
                            menu[option].value = !menu[option].value;
                        } else if (menu[option].type == MenuType::Value) {
                            --menu[option].value;
                            if (menu[option].value < menu[option].min) {
                                menu[option].value = menu[option].max;
                            }
                            if (menu[option].code == menuMusicVolume) {
                                Mix_VolumeMusic(menu[option].value);
                            } else if (menu[option].code == menuAudioVolume) {
                                Mix_Volume(-1, menu[option].value);
                                state.playEffect(0);
                            }
                        }
                        break;
                    case SDLK_w:
                    case SDLK_UP:
                        do {
                            --option;
                            if (option < 0) option = menuSize - 1;
                        } while (menu[option].type == MenuType::Disabled || menu[option].text.empty());
                        state.playEffect(0);
                        break;
                    case SDLK_s:
                    case SDLK_DOWN:
                        do {
                            ++option;
                            if (option >= menuSize) option = 0;
                        } while (menu[option].type == MenuType::Disabled || menu[option].text.empty());
                        state.playEffect(0);
                        break;
                    default:
                        for (int i = 0; menu[i].code != menuEnd; ++i) {
                            if (menu[i].key == event.key.keysym.sym) {
                                return menu[i].code;
                            }
                        }
                }
        }
    }
}

void gfx_RunInfo(System &state, const std::vector<std::string> &text, bool autoscroll) {
    const int lineHeight = state.smallFont->getLineHeight();
    const int numLines = text.size();
    int firstLineY = 20 + lineHeight;
    int maxLines = (screenHeight - firstLineY) / lineHeight - 2;

    bool pauseScroll = !autoscroll;
    int firstLine = 0, subScroll = 0;
    SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
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
        state.smallFont->out(0, screenHeight - state.smallFont->getLineHeight(), " RETURN/ESCAPE - Return to menu   SPACE - pause scroll   UP/DOWN - scroll text");
        SDL_RenderPresent(state.renderer);

        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                state.wantsToQuit = true;
                return;
            }
            if (event.type == SDL_MOUSEBUTTONUP) return;
            if (event.type == SDL_KEYDOWN) {
                switch(event.key.keysym.sym) {
                    case SDLK_q:
                        state.wantsToQuit = true;
                        return;
                    case SDLK_UP:
                        if (firstLine > 0) --firstLine;
                        break;
                    case SDLK_DOWN:
                        if (firstLine < numLines - maxLines) {
                            ++firstLine;
                        }
                        break;
                    case SDLK_SPACE:
                        pauseScroll = !pauseScroll;
                        break;
                    case SDLK_ESCAPE:
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                        return;
                }
            }
        }
    }
}
