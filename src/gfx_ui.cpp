#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_framerate.h>

#include "game.h"
#include "gfx.h"
#include "textutil.h"


void gfx_MessageBox(System &state, std::string text) {
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(state.renderer, &screenWidth, &screenHeight);
    const int lineHeight = state.smallFont->getLineHeight();
    const int textOffset = 8;
    const int targetHeight = (screenHeight - 200) / 2;
    const int height = targetHeight / lineHeight * lineHeight + 2 * textOffset;
    const unsigned maxLines = targetHeight / lineHeight;
    const int width = screenWidth - 200;
    const int left = 100;
    const int top = screenHeight - height - 100;
    const int textWidth = (width - textOffset * 2) / state.smallFont->getCharWidth();

    std::string finalLine;
    finalLine.insert(0, textWidth - 1, ' ');
    finalLine += "\x07";
    std::vector<std::string> lines;
    wordwrap(text, textWidth, lines);
    lines.push_back(finalLine);


    unsigned firstLine = 0;
    while (1) {
        repaint(state, false);
        gfx_DrawFrame(state, left, top, width, height);
        for (unsigned i = 0; i < maxLines; ++i) {
            unsigned lineNo = firstLine + i;
            if (lineNo >= lines.size()) break;
            state.smallFont->out(left + 8, top + 8 + i * lineHeight, lines[lineNo]);
        }
        SDL_RenderPresent(state.renderer);

        SDL_Event event;
        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT) {
            state.wantsToQuit = true;
            return;
        }
        if (event.type == SDL_MOUSEBUTTONUP) return;
        if (event.type == SDL_KEYDOWN) {
            switch(event.key.keysym.sym) {
                case SDLK_q:
                    if (event.key.keysym.mod & KMOD_SHIFT) {
                        state.wantsToQuit = true;
                        return;
                    }
                    break;
                case SDLK_UP:
                case SDLK_w:
                    if (firstLine) {
                        --firstLine;
                    }
                    break;
                case SDLK_DOWN:
                case SDLK_s:
                    if (lines.size() > maxLines && firstLine < lines.size() - maxLines) {
                        ++firstLine;
                    }
                    break;
                case SDLK_SPACE:
                case SDLK_RETURN:
                case SDLK_ESCAPE:
                case SDLK_KP_ENTER:
                    return;
            }
        }

        SDL_framerateDelay(static_cast<FPSmanager*>(state.fpsManager));
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
            if (text.size() < maxLength) {
                text += event.text.text;
            }
        }
        if (event.type == SDL_KEYDOWN) {
            switch(event.key.keysym.sym) {
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

        SDL_framerateDelay(static_cast<FPSmanager*>(system.fpsManager));
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
