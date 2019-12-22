#include <SDL2/SDL.h>
#include <SDL2/SDL2_framerate.h>

#include "beast.h"
#include "command.h"
#include "game.h"
#include "gamestate.h"
#include "gfx.h"


void gfx_partyScreen(System &system, int &curBeast) {
    const int lineHeight = system.smallFont->getLineHeight();

    SDL_SetRenderDrawColor(system.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(system.renderer);
    int beastX = 0, beastY = 0;
    for (int i = 0; i < BeastParty::MAX_SIZE; ++i) {
        if (i == curBeast) {
            SDL_Rect box = { beastX, beastY, screenWidth / 2, screenHeight / 2 };
            SDL_SetRenderDrawColor(system.renderer, 127, 127, 127, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(system.renderer, &box);
        }
        Beast *beast = system.party.at(i);
        if (beast) {
            int yPos = beastY;
            int col2 = beastX + screenWidth / 4;
            int artY = beastY + screenHeight / 4;
            int artSize = screenHeight / 4;
            int artOffset = ((screenWidth / 4) - artSize) / 2;
            system.smallFont->out(beastX, yPos, beast->getName());
            system.smallFont->out(col2, yPos, "Lv: " + std::to_string(beast->level));
            yPos += lineHeight;
            system.smallFont->out(beastX, yPos, "HP: " + std::to_string(beast->curHealth)+"/"+std::to_string(beast->getStat(Stat::Health)));
            yPos += lineHeight;
            system.smallFont->out(beastX, yPos, "EN: " + std::to_string(beast->curHealth)+"/"+std::to_string(beast->getStat(Stat::Health)));
            yPos += lineHeight;
            system.smallFont->out(beastX, yPos, "XP: " + std::to_string(beast->xp)+"/"+std::to_string(beast->getStat(Stat::XP)));
            yPos += lineHeight;
            SDL_Rect artBox = { col2 + artOffset, artY, artSize, artSize };
            SDL_RenderCopy(system.renderer, beast->art, nullptr, &artBox);
            int typeX = beastX;
            for (int j = 0; j < 3; ++j) {
                int type = beast->typeInfo->type[j];
                if (type >= 0) {
                    const std::string &name = TypeInfo::getName(type);
                    system.smallFont->out(typeX, yPos, name);
                    typeX += (name.size() + 2) * system.smallFont->getCharWidth();
                }
            }
        }

        if (beastX > 0) {
            beastX = 0;
            beastY += screenHeight / 2;
        } else {
            beastX += screenWidth / 2;
        }
    }

    SDL_RenderPresent(system.renderer);
}

void doPartyScreen(System &system) {
    for (int i = 0; i < BeastParty::MAX_SIZE; ++i) {
        Beast *beast = system.party.at(i);
        if (beast) {
            beast->art = system.getImage("beasts/" + std::to_string(beast->typeIdent) + ".png");
        }
    }

    int curBeast = 0;

    while (1) {
        gfx_partyScreen(system, curBeast);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            const CommandDef &cmd = getCommand(event, partyCommands);
            switch(cmd.command) {
                case Command::Quit:
                    system.wantsToQuit = true;
                    return;
                case Command::Cancel:
                    return;
                case Command::Move:
                    if (cmd.direction == Dir::East || cmd.direction == Dir::West) {
                        if (curBeast == 0) curBeast = 1;
                        else if (curBeast == 1) curBeast = 0;
                        else if (curBeast == 2) curBeast = 3;
                        else if (curBeast == 3) curBeast = 2;
                    } else if (cmd.direction == Dir::South || cmd.direction == Dir::North) {
                        if (curBeast == 0) curBeast = 2;
                        else if (curBeast == 2) curBeast = 0;
                        else if (curBeast == 1) curBeast = 3;
                        else if (curBeast == 3) curBeast = 1;
                    }
                    break;
                case Command::Rename: {
                    Beast *b = system.party.at(curBeast);
                    if (!b) break;
                    std::string name = b->getName();
                    if (gfx_EditText(system, "Rename:", name, 16)) {
                        b->name = name;
                    }
                    break; }
                default:
                    /* we don't need to handle any of the other commands */
                    break;
            }
        }
        SDL_framerateDelay(static_cast<FPSmanager*>(system.fpsManager));
    }

}

