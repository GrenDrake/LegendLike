#include <string>
#include <vector>
#include <SDL2/SDL.h>

#include "command.h"
#include "game.h"
#include "gfx.h"
#include "textutil.h"


bool gfx_Confirm(System &state, const std::string &line1, const std::string &line2, bool defaultResult) {
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(state.renderer, &screenWidth, &screenHeight);

    int buttonWidth = 5 * state.smallFont->getCharWidth() + 8;
    int buttonHeight = state.smallFont->getCharHeight() + 8;
    unsigned titleLength = line1.size();
    unsigned messageLength = line2.size();
    unsigned maxLength = titleLength;
    if (maxLength < messageLength) maxLength = messageLength;
    unsigned realLength = state.smallFont->getCharWidth() * maxLength;
    int boxWidth = realLength + 32;
    int boxHeight = 48 + state.smallFont->getCharHeight() * 3 + buttonHeight;
    int boxX = (screenWidth - boxWidth) / 2;
    int boxY = (screenHeight - boxHeight) / 2;
    unsigned titleX = boxX + (boxWidth - titleLength * state.smallFont->getCharWidth()) / 2;
    unsigned messageX = boxX + (boxWidth - messageLength * state.smallFont->getCharWidth()) / 2;

    SDL_Rect yesButton {
        boxX + 16,
        boxY + boxHeight - 16 - buttonHeight,
        buttonWidth,
        buttonHeight
    };
    SDL_Rect noButton = yesButton;
    noButton.x = boxX + boxWidth - buttonWidth - 16;

    while (1) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        repaint(state, false);
        int topOffset = 8 + gfx_DrawFrame(state, boxX, boxY, boxWidth, boxHeight, "Confirm");
        state.smallFont->out(titleX, topOffset, line1);
        state.smallFont->out(messageX, topOffset + state.smallFont->getCharHeight(), line2);
        SDL_SetRenderDrawColor(state.renderer, 127, 127, 127, SDL_ALPHA_OPAQUE);
        bool yesHover = pointInBox(mouseX, mouseY, yesButton);
        bool noHover = pointInBox(mouseX, mouseY, noButton);
        gfx_DrawButton(state, yesButton, yesHover, "Yes");
        gfx_DrawButton(state, noButton,  noHover,  "No");
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
            if (pointInBox(mouseX, mouseY, yesButton)) return true;
            else if (pointInBox(mouseX, mouseY, noButton)) return false;
        }
        if (event.type == SDL_KEYDOWN) {
            const CommandDef &command = getCommand(state, event, confirmCommands);
            switch(command.command) {
                case Command::Quit:
                    if (state.wantsToQuit) {
                        // if we're already trying to quit, take this as a "yes"
                        // the only time we call this function while quitting should be
                        // to ask "Are you sure you want to quit?"
                        if (event.key.keysym.mod & KMOD_SHIFT) {
                            state.wantsToQuit = true;
                            return true;
                        }
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

void gfx_Alert(System &state, const std::string &line1, const std::string &line2) {
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(state.renderer, &screenWidth, &screenHeight);

    int buttonWidth = 6 * state.smallFont->getCharWidth() + 8;
    int buttonHeight = state.smallFont->getCharHeight() + 8;
    unsigned titleLength = line1.size();
    unsigned messageLength = line2.size();
    unsigned maxLength = titleLength;
    if (maxLength < messageLength) maxLength = messageLength;
    unsigned realLength = state.smallFont->getCharWidth() * maxLength;
    int boxWidth = realLength + 32;
    int boxHeight = 48 + state.smallFont->getCharHeight() * 3 + buttonHeight;
    int boxX = (screenWidth - boxWidth) / 2;
    int boxY = (screenHeight - boxHeight) / 2;
    unsigned titleX = boxX + (boxWidth - titleLength * state.smallFont->getCharWidth()) / 2;
    unsigned messageX = boxX + (boxWidth - messageLength * state.smallFont->getCharWidth()) / 2;

    SDL_Rect okButton {
        boxX + (boxWidth - buttonWidth) / 2,
        boxY + boxHeight - 16 - buttonHeight,
        buttonWidth,
        buttonHeight
    };

    while (1) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        repaint(state, false);
        int topOffset = 8 + gfx_DrawFrame(state, boxX, boxY, boxWidth, boxHeight, "Alert");
        state.smallFont->out(titleX, topOffset, line1);
        state.smallFont->out(messageX, topOffset + state.smallFont->getCharHeight(), line2);
        SDL_SetRenderDrawColor(state.renderer, 127, 127, 127, SDL_ALPHA_OPAQUE);
        bool okHover = pointInBox(mouseX, mouseY, okButton);
        gfx_DrawButton(state, okButton, okHover, "Okay");
        state.advanceFrame();
        SDL_RenderPresent(state.renderer);

        SDL_Event event;
        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT) {
            state.wantsToQuit = true;
            return;
        }
        if (event.type == SDL_MOUSEBUTTONUP) {
            if (pointInBox(mouseX, mouseY, okButton)) return;
        }
        if (event.type == SDL_KEYDOWN) {
            const CommandDef &command = getCommand(state, event, confirmCommands);
            switch(command.command) {
                case Command::Quit:
                    if (event.key.keysym.mod & KMOD_SHIFT) {
                        state.wantsToQuit = true;
                        return;
                    }
                    break;
                case Command::Move:
                case Command::Cancel:
                case Command::Interact:
                case Command::Close:
                    return;
                default:
                    // we don't care about any other commands
                    break;
            }
        }
    }
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
        gfx_DrawFrame(system, left, top, width, height, "");
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


bool pointInBox(int x, int y, const SDL_Rect &box) {
    if (x < box.x || y < box.y) return false;
    if (x > box.x + box.w) return false;
    if (y > box.y + box.h) return false;
    return true;
}

int gfx_DrawFrame(System &system, int x, int y, int w, int h, const std::string &title) {
    SDL_Rect background = { x, y, w, h };
    SDL_SetRenderDrawColor(system.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(system.renderer, &background);
    SDL_SetRenderDrawColor(system.renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawRect(system.renderer, &background);
    int lineY = y;
    if (!title.empty()) {
        SDL_Rect titleBar{ x, y, w, system.smallFont->getCharHeight() + 8 };
        int offsetX = (w - title.size() * system.smallFont->getCharWidth()) / 2;
        SDL_SetRenderDrawColor(system.renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(system.renderer, &titleBar);
        system.smallFont->out(x + offsetX + 4, y + 4, title, Color{0, 0, 0});
        lineY += titleBar.h;
    }
    return lineY + 1;
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

void gfx_DrawButton(System &system, const SDL_Rect &box, bool selected, const std::string &text) {
    const int charWidth = system.smallFont->getCharWidth();
    if (selected)   SDL_SetRenderDrawColor(system.renderer, 63, 63, 63, SDL_ALPHA_OPAQUE);
    else            SDL_SetRenderDrawColor(system.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(system.renderer, &box);
    SDL_SetRenderDrawColor(system.renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawRect(system.renderer, &box);
    const int offset = (box.w - text.size() * charWidth) / 2;
    system.smallFont->out(box.x + offset, box.y + 4, text);
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
