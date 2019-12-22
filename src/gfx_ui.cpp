#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_framerate.h>

#include "game.h"
#include "gfx.h"
#include "textutil.h"


void gfx_MessageBox(System &state, std::string text, const std::string &portrait, int portraitSide) {
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

        if (portraitSide != portraitNone) {
            SDL_Rect portraitDest = { 0, top - portraitHeight, portraitWidth, portraitHeight };
            if (portraitSide == portraitLeft) {
                portraitDest.x = left + portraitOffset;
            } else if (portraitSide == portraitCentre) {
                portraitDest.x = left + width / 2 - portraitWidth / 2;
            } else {
                portraitDest.x = left + width - portraitOffset - portraitWidth;
            }
            SDL_Rect portraitBox = {
                portraitDest.x - 2, portraitDest.y - 2,
                portraitDest.w + 4, portraitDest.h + 4
            };

            SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(state.renderer, &portraitBox);
            SDL_Texture *portraitTex = state.getImage(portrait);
            if (portraitTex) {
                SDL_RenderCopy(state.renderer, portraitTex, nullptr, &portraitDest);
            }
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