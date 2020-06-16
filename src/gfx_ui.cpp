#include <iostream>
#include <string>
#include <vector>
#include <SDL2/SDL.h>

#include "command.h"
#include "game.h"
#include "gfx.h"
#include "textutil.h"

bool pointInBox(int x, int y, const SDL_Rect &box) {
    if (x < box.x || y < box.y) return false;
    if (x > box.x + box.w) return false;
    if (y > box.y + box.h) return false;
    return true;
}

bool gfx_confirm(System &state, const std::string &title, const std::string &message, bool defaultResult) {
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(state.renderer, &screenWidth, &screenHeight);

    unsigned titleLength = title.size();
    unsigned messageLength = message.size();
    unsigned maxLength = titleLength;
    if (maxLength < messageLength) maxLength = messageLength;
    unsigned realLength = state.smallFont->getCharWidth() * maxLength;
    int boxWidth = realLength + 32;
    int boxHeight = 32 + state.smallFont->getCharHeight() * 3.5;
    int boxX = (screenWidth - boxWidth) / 2;
    int boxY = (screenHeight - boxHeight) / 2;
    unsigned titleX = boxX + (boxWidth - titleLength * state.smallFont->getCharWidth()) / 2;
    unsigned messageX = boxX + (boxWidth - messageLength * state.smallFont->getCharWidth()) / 2;

    SDL_Rect yesBox {
        boxX + 16,
        boxY + boxHeight - 16 - state.smallFont->getCharHeight(),
        3 * state.smallFont->getCharWidth(),
        state.smallFont->getCharHeight()
    };
    SDL_Rect noBox {
        boxX + boxWidth - 16 - 2 * state.smallFont->getCharWidth(),
        boxY + boxHeight - 16 - state.smallFont->getCharHeight(),
        2 * state.smallFont->getCharWidth(),
        state.smallFont->getCharHeight()
    };

    while (1) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        repaint(state, false);
        gfx_DrawFrame(state, boxX, boxY, boxWidth, boxHeight);
        state.smallFont->out(titleX, boxY + 6, title);
        gfx_HLine(state, boxX, boxX + boxWidth, boxY + 12 + state.smallFont->getCharHeight(), Color{255, 255, 255});
        state.smallFont->out(messageX, boxY + 16 + state.smallFont->getCharHeight(), message);
        SDL_SetRenderDrawColor(state.renderer, 127, 127, 127, SDL_ALPHA_OPAQUE);
        if (pointInBox(mouseX, mouseY, yesBox)) SDL_RenderFillRect(state.renderer, &yesBox);
        else if (pointInBox(mouseX, mouseY, noBox)) SDL_RenderFillRect(state.renderer, &noBox);
        state.smallFont->out(yesBox.x, yesBox.y, "Yes");
        state.smallFont->out(noBox.x, noBox.y, "No");
        state.advanceFrame();
        SDL_RenderPresent(state.renderer);

        SDL_Event event;
        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT) {
            if (state.wantsToQuit) {
                // if we're already trying to quit, take this as a "yes"
                // the only time we call this function while quitting should be
                // to ask "Are you sure you want to quit?"
                return true;
            }
            state.wantsToQuit = true;
            return false;
        }
        if (event.type == SDL_MOUSEBUTTONUP) {
            if (pointInBox(mouseX, mouseY, yesBox)) return true;
            else if (pointInBox(mouseX, mouseY, noBox)) return false;
        }
        if (event.type == SDL_KEYDOWN) {
            const CommandDef &command = getCommand(state, event, confirmCommands);
            switch(command.command) {
                case Command::Quit:
                    if (event.key.keysym.mod & KMOD_SHIFT) {
                        state.wantsToQuit = true;
                        return false;
                    }
                    break;
                case Command::Move:
                    return true;
                case Command::Cancel:
                    return false;
                case Command::Interact:
                    return defaultResult;
                case Command::Close:
                    return !defaultResult;
                default:
                    // we don't care about any other commands
                    break;
            }
        }
    }
}

void gfx_DrawFrame(System &system, int x, int y, int w, int h) {
    SDL_Rect background = { x, y, w, h };
    SDL_SetRenderDrawColor(system.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(system.renderer, &background);

    SDL_Texture *uiEW = system.getImage("ui/ew.png");
    SDL_Texture *uiNS = system.getImage("ui/ns.png");
    SDL_Texture *uiNE = system.getImage("ui/ne.png");
    SDL_Texture *uiNW = system.getImage("ui/nw.png");
    SDL_Texture *uiSE = system.getImage("ui/se.png");
    SDL_Texture *uiSW = system.getImage("ui/sw.png");

    const int uiWidth = 8;
    const int uiHeight = 8;
    for (int i = 0; i < w; i += uiWidth) {
        SDL_Rect box = { i + x, y - uiHeight, uiWidth, uiHeight };
        SDL_RenderCopy(system.renderer, uiEW, nullptr, &box);
        box.y = y + h;
        SDL_RenderCopy(system.renderer, uiEW, nullptr, &box);
    }
    for (int i = 0; i < h; i += uiHeight) {
        SDL_Rect box = { x - uiWidth, y + i, uiWidth, uiHeight };
        SDL_RenderCopy(system.renderer, uiNS, nullptr, &box);
        box.x = x + w;
        SDL_RenderCopy(system.renderer, uiNS, nullptr, &box);
    }
    SDL_Rect box = { x - uiWidth, y - uiHeight, uiWidth, uiHeight };
    SDL_RenderCopy(system.renderer, uiSE, nullptr, &box);
    box.y = y + h;
    SDL_RenderCopy(system.renderer, uiNE, nullptr, &box);
    box.x = x + w;
    SDL_RenderCopy(system.renderer, uiNW, nullptr, &box);
    box.y = y - uiHeight;
    SDL_RenderCopy(system.renderer, uiSW, nullptr, &box);
}

void gfx_DrawBar(System &system, int x, int y, int length, int height, double percent, const Color &baseColor) {
    SDL_Rect dest = { x, y, length, height };
    SDL_SetRenderDrawColor(system.renderer, baseColor.r/2, baseColor.g/2, baseColor.b/2, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(system.renderer, &dest);
    dest.x += 2;
    dest.y += 2;
    dest.w -= 4;
    dest.h -= 4;
    SDL_SetRenderDrawColor(system.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(system.renderer, &dest);
    dest.w *= percent;
    SDL_SetRenderDrawColor(system.renderer, baseColor.r, baseColor.g, baseColor.b, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(system.renderer, &dest);
}

bool gfx_EditText(System &system, const std::string &prompt, std::string &text, int maxLength) {
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(system.renderer, &screenWidth, &screenHeight);
    const int lineHeight = system.smallFont->getLineHeight();
    const int textOffset = 8;
    const int height = lineHeight + 2 * textOffset;
    const int width = (prompt.size() + maxLength + 1) * system.smallFont->getCharWidth() + 2 * textOffset;
    const int left = (screenWidth - width) / 2;
    const int top = (screenHeight - height) / 2;
    const int inputLeft = left + (prompt.size() + 1) * system.smallFont->getCharWidth() + textOffset;

    SDL_StartTextInput();
    while (1) {
        SDL_SetRenderDrawColor(system.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(system.renderer);
        gfx_DrawFrame(system, left, top, width, height);
        system.smallFont->out(left + textOffset, top + textOffset, prompt);
        system.smallFont->out(inputLeft, top + textOffset, text);

        SDL_RenderPresent(system.renderer);

        SDL_Event event;
        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT) {
            system.wantsToQuit = true;
            SDL_StopTextInput();
            return false;
        }
        if (event.type == SDL_TEXTINPUT) {
            if (static_cast<int>(text.size()) < maxLength) {
                text += event.text.text;
            }
        }
        if (event.type == SDL_KEYDOWN) {
            switch(event.key.keysym.sym) {
                case SDLK_c:
                    if ((event.key.keysym.mod & SDLK_LCTRL) || (event.key.keysym.mod & SDLK_LCTRL)) {
                        SDL_SetClipboardText(text.c_str());
                    }
                    break;
                case SDLK_v:
                    if ((event.key.keysym.mod & SDLK_LCTRL) || (event.key.keysym.mod & SDLK_LCTRL)) {
                        text = SDL_GetClipboardText();
                    }
                    break;
                case SDLK_BACKSPACE:
                    if (!text.empty()) text.erase(text.size() - 1);
                    break;
                case SDLK_ESCAPE:
                    SDL_StopTextInput();
                    return false;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    SDL_StopTextInput();
                    return true;
            }
        }
    }
}

void gfx_DrawTooltip(System &system, int x, int y, const std::string &text) {
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(system.renderer, &screenWidth, &screenHeight);
    std::vector<std::string> lines = explode(text, "\n");
    unsigned longestLine = 0;
    for (const std::string &s : lines) {
        if (s.size() > longestLine) longestLine = s.size();
    }
    const int offset = 4;
    const int height = lines.size() * system.tinyFont->getLineHeight() + offset * 2;
    const int width = longestLine * system.tinyFont->getCharWidth() + offset * 2;
    if (x + width >= screenWidth) x -= width;
    if (y + height >= screenHeight) y -= height;

    SDL_Rect outer = { x - 2, y - 2, width + 4, height + 4 };
    SDL_Rect inner = { x, y, width, height };
    SDL_SetRenderDrawColor(system.renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(system.renderer, &outer);
    SDL_SetRenderDrawColor(system.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(system.renderer, &inner);
    system.tinyFont->out(x + offset, y + offset, text);
}

void gfx_DrawButton(System &system, int x, int y, int w, int h, bool selected, const std::string &text) {
    const int charWidth = system.smallFont->getCharWidth();
    SDL_Rect box = { x, y, w, h };
        if (selected)   SDL_SetRenderDrawColor(system.renderer, 63, 63, 63, SDL_ALPHA_OPAQUE);
        else            SDL_SetRenderDrawColor(system.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(system.renderer, &box);
    SDL_SetRenderDrawColor(system.renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawRect(system.renderer, &box);
    const int offset = (w - text.size() * charWidth) / 2;
    system.smallFont->out(x + 4 + offset, y + 4, text);
}

int keyToIndex(const SDL_Keysym &key) {
    if (key.mod & (KMOD_SHIFT | KMOD_ALT | KMOD_CTRL | KMOD_GUI)) return -1;
    switch(key.sym) {
        case SDLK_a:    return 0;
        case SDLK_b:    return 1;
        case SDLK_c:    return 2;
        case SDLK_d:    return 3;
        case SDLK_e:    return 4;
        case SDLK_f:    return 5;
        case SDLK_g:    return 6;
        case SDLK_h:    return 7;
        case SDLK_i:    return 8;
        case SDLK_j:    return 9;
        case SDLK_k:    return 10;
        case SDLK_l:    return 11;
        case SDLK_m:    return 12;
        case SDLK_n:    return 13;
        case SDLK_o:    return 14;
        case SDLK_p:    return 15;
        case SDLK_q:    return 16;
        case SDLK_r:    return 17;
        case SDLK_s:    return 18;
        case SDLK_t:    return 19;
        case SDLK_u:    return 20;
        case SDLK_v:    return 21;
        case SDLK_w:    return 22;
        case SDLK_x:    return 23;
        case SDLK_y:    return 24;
        case SDLK_z:    return 25;
        default:        return -1;
    }
}

void gfx_Clear(System &system) {
    SDL_SetRenderDrawColor(system.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(system.renderer);
}

void gfx_DrawRect(System &system, int x, int y, int x2, int y2, const Color &color) {
    SDL_Rect box = {x, y, x2 - x, y2 - y };
    SDL_SetRenderDrawColor(system.renderer, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(system.renderer, &box);
}

void gfx_FillRect(System &system, int x, int y, int x2, int y2, const Color &color) {
    SDL_Rect box = {x, y, x2 - x, y2 - y };
    SDL_SetRenderDrawColor(system.renderer, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(system.renderer, &box);
}

void gfx_HLine(System &system, int x, int x2, int y, const Color &color) {
    SDL_Rect box = {x, y, x2 - x, 1 };
    SDL_SetRenderDrawColor(system.renderer, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(system.renderer, &box);
}

void gfx_VLine(System &system, int x, int y, int y2, const Color &color) {
    SDL_Rect box = {x, y, 1, y2 - y };
    SDL_SetRenderDrawColor(system.renderer, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(system.renderer, &box);
}
