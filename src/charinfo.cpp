#include <iomanip>
#include <sstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL2_framerate.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>

#include "creature.h"
#include "command.h"
#include "gfx.h"
#include "board.h"
#include "game.h"
#include "point.h"
#include "command.h"
#include "textutil.h"

static int mode = charStats;
static std::vector<SDL_Rect> tabButtons;

void gfx_drawCharInfo(System &state, bool callPresent) {
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetRendererOutputSize(state.renderer, &screenWidth, &screenHeight);

    const int charWidth  = state.smallFont->getCharWidth();
    const int lineHeight = state.smallFont->getLineHeight();
    const int barHeight = lineHeight / 2;
    const int barWidth = charWidth * 40;
    const int column2 = charWidth * 20;

    SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(state.renderer);
    //  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////
    //  PLAYER INFO
    int yPos = 8;
    int xPos = 8;
    Creature *player = state.getPlayer();
    const double healthPercent = static_cast<double>(player->curHealth) / static_cast<double>(player->getStat(Stat::Health));
    const double energyPercent = static_cast<double>(player->curEnergy) / static_cast<double>(player->getStat(Stat::Energy));

    state.smallFont->out(xPos, yPos, player->name);
    yPos += lineHeight;
    gfx_DrawBar(state, xPos, yPos, barWidth, barHeight, healthPercent, Color{127,255,127});
    yPos += barHeight;
    gfx_DrawBar(state, xPos, yPos, barWidth, barHeight, energyPercent, Color{127,127,255});
    yPos += barHeight * 2;

    state.smallFont->out(xPos, yPos, "HP: " + std::to_string(player->curHealth) + "/" + std::to_string(player->getStat(Stat::Health)));
    state.smallFont->out(column2, yPos, "      EN: " + std::to_string(player->curEnergy) + "/" + std::to_string(player->getStat(Stat::Energy)));
    yPos += lineHeight * 2;
    const int paneTop = yPos;

    if (mode == charStats) {
        state.smallFont->out(xPos, yPos, "STATS");
        yPos += lineHeight;
        for (int i = 2; i < statCount; ++i) {
            Stat stat = static_cast<Stat>(i);
            std::stringstream line;
            line << getAbbrev(stat) << ": " << player->getStat(stat);
            state.smallFont->out(xPos, yPos, line.str());
            yPos += lineHeight;
        }

        yPos = paneTop;
        state.smallFont->out(column2, yPos, "RESISTANCES");
        yPos += lineHeight;
        for (int i = 0; i < damageTypeCount; ++i) {
            DamageType damageType = static_cast<DamageType>(i);
            std::stringstream line;
            line << std::fixed << std::setprecision(3);
            line << std::setw(8) << damageType << ": " << player->getResist(damageType);
            state.smallFont->out(column2, yPos, line.str());
            yPos += lineHeight;
        }

    } else if (mode == charAbilities) {
        state.smallFont->out(xPos, yPos, "CURRENT");
        yPos += lineHeight;

        yPos = paneTop;
        state.smallFont->out(column2, yPos, "KNOWN");
        yPos += lineHeight;
        for (unsigned i = 0; i < 4 && i < player->moves.size(); ++i) {
            const MoveType &move = MoveType::get(player->moves[i]);
            state.smallFont->out(column2, yPos, move.name);
            yPos += lineHeight;
        }
    } else if (mode == charInventory) {
        state.smallFont->out(xPos, yPos, "CARRYING");
        state.smallFont->out(column2, yPos, "EQUIPPED");
    }

    const int tabWidth = 200;
    const int tabHeight = lineHeight + 8;
    xPos = 0;
    yPos = screenHeight - tabHeight;
    tabButtons.clear();
    for (int i = 0; i <= charModeCount; ++i) {
        SDL_Rect box = { xPos, yPos, tabWidth, tabHeight };
        tabButtons.push_back(box);
        if (i == mode)  SDL_SetRenderDrawColor(state.renderer, 63, 63, 63, SDL_ALPHA_OPAQUE);
        else            SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(state.renderer, &box);
        SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawRect(state.renderer, &box);
        std::string text;
        switch(i) {
            case charStats:     text = "Stats"; break;
            case charAbilities: text = "Abilities"; break;
            case charInventory: text = "Inventory"; break;
            case charModeCount: text = "Done"; break;
        }
        const int offset = (tabWidth - text.size() * charWidth) / 2;
        state.smallFont->out(xPos + 4 + offset, yPos + 4, text);
        xPos += tabWidth;
    }


    //  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////  ////
    //  FPS INFO
    if (state.showFPS) {
        std::stringstream ss;
        ss << "  FPS: " << state.fps;
        state.smallFont->out(0, 0, ss.str().c_str());
    }

    if (callPresent) SDL_RenderPresent(state.renderer);
}

void doCharInfo(System &system, int initialMode) {
    if (initialMode < 0 || initialMode >= charModeCount) mode = charStats;
    else mode = initialMode;

    while (1) {
        gfx_drawCharInfo(system, true);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mx = event.button.x;
                int my = event.button.y;
                for (unsigned i = 0; i < tabButtons.size(); ++i) {
                    const SDL_Rect &box = tabButtons[i];
                    if (mx < box.x || my < box.y) continue;
                    if (mx >= box.x + box.w || my >= box.y + box.h) continue;
                    if (i == charModeCount) return;
                    mode = i;
                    break;
                }
            } else {
                const CommandDef &cmd = getCommand(event, partyCommands);
                if (cmd.command == Command::Quit) {
                    system.wantsToQuit = true;
                    return;
                }
                if (cmd.command == Command::Cancel) {
                    return;
                }
                if (cmd.command == Command::NextMode) {
                    ++mode;
                    if (mode >= charModeCount) {
                        mode = 0;
                    }
                }
                if (cmd.command == Command::PrevMode) {
                    --mode;
                    if (mode < 0) {
                        mode = charModeCount - 1;
                    }
                }
            }
        }

        SDL_framerateDelay(static_cast<FPSmanager*>(system.fpsManager));
    }

}
