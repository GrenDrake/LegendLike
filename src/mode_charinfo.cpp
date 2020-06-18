#include <iomanip>
#include <sstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "creature.h"
#include "command.h"
#include "gfx.h"
#include "board.h"
#include "game.h"
#include "point.h"
#include "command.h"
#include "textutil.h"

static int selection = 0;
static std::vector<SDL_Rect> tabButtons;
static SDL_Rect doneButton{ -1 };

void gfx_drawCharInfo(System &state, bool callPresent) {
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(state.renderer, &screenWidth, &screenHeight);

    const int charWidth  = state.smallFont->getCharWidth();
    const int lineHeight = state.smallFont->getLineHeight();
    const int barHeight = lineHeight / 2;
    const int barWidth = charWidth * 40;
    const int column2 = charWidth * 30;

    SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(state.renderer);
    //  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////
    //  PLAYER INFO
    int yPos = 8;
    int xPos = 8;
    Creature *player = state.getPlayer();
    const double healthPercent = static_cast<double>(player->curHealth) / static_cast<double>(player->typeInfo->maxHealth);
    const double energyPercent = static_cast<double>(player->curEnergy) / static_cast<double>(player->typeInfo->maxEnergy);

    state.smallFont->out(xPos, yPos, player->name);
    yPos += lineHeight;
    gfx_DrawBar(state, xPos, yPos, barWidth, barHeight, healthPercent, Color{127,255,127});
    yPos += barHeight;
    gfx_DrawBar(state, xPos, yPos, barWidth, barHeight, energyPercent, Color{127,127,255});
    yPos += barHeight * 2;

    state.smallFont->out(xPos, yPos, "HP: " + std::to_string(player->curHealth) + "/" + std::to_string(player->typeInfo->maxHealth));
    state.smallFont->out(column2, yPos, "    Energy: " + std::to_string(player->curEnergy) + "/" + std::to_string(player->typeInfo->maxEnergy));
    yPos += lineHeight * 2;

    state.smallFont->out(xPos, yPos, "CARRYING");
    state.smallFont->out(column2, yPos, "EQUIPPED");


    const int tabWidth = 200;
    const int tabHeight = lineHeight + 8;
    xPos = 0;
    yPos = screenHeight - tabHeight;
    if (doneButton.x < 0) {
        doneButton.x = 0;
        doneButton.y = screenHeight - tabHeight;
        doneButton.w = tabWidth;
        doneButton.h = tabHeight;
    };
    SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(state.renderer, &doneButton);
    SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawRect(state.renderer, &doneButton);
    const int offset = (tabWidth - 4 * charWidth) / 2;
    state.smallFont->out(xPos + 4 + offset, yPos + 4, "Done");


    //  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////
    //  FPS INFO
    if (state.showFPS) {
        std::stringstream ss;
        ss << "  FPS: " << state.getFPS();
        state.smallFont->out(0, 0, ss.str().c_str());
    }

    state.advanceFrame();
    if (callPresent) SDL_RenderPresent(state.renderer);
}

void doCharInfo(System &system) {
    selection = 0;

    while (1) {
        gfx_drawCharInfo(system, true);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (pointInBox(event.button.x, event.button.y, doneButton)) {
                    return;
                }
            } else {
                const CommandDef &cmd = getCommand(system, event, characterCommands);
                switch(cmd.command) {
                    case Command::Quit:
                        system.wantsToQuit = true;
                        return;
                    case Command::Close:
                        return;
                    default:
                        /* we don't need to handle the other commands */
                        break;
                }
            }
        }
    }
}
