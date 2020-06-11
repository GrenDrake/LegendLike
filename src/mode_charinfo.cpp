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

static int mode = charStats;
static int selection = 0;
static std::vector<SDL_Rect> tabButtons;


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
    const double healthPercent = static_cast<double>(player->curHealth) / static_cast<double>(player->getStat(Stat::Health));
    const double energyPercent = static_cast<double>(player->curEnergy) / static_cast<double>(player->getStat(Stat::Energy));

    state.smallFont->out(xPos, yPos, player->name);
    yPos += lineHeight;
    gfx_DrawBar(state, xPos, yPos, barWidth, barHeight, healthPercent, Color{127,255,127});
    yPos += barHeight;
    gfx_DrawBar(state, xPos, yPos, barWidth, barHeight, energyPercent, Color{127,127,255});
    yPos += barHeight * 2;

    state.smallFont->out(xPos, yPos, "HP: " + std::to_string(player->curHealth) + "/" + std::to_string(player->getStat(Stat::Health)));
    state.smallFont->out(column2, yPos, "    Energy: " + std::to_string(player->curEnergy) + "/" + std::to_string(player->getStat(Stat::Energy)));
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
            line << std::setw(10) << damageType << ": " << player->getResist(damageType);
            state.smallFont->out(column2, yPos, line.str());
            yPos += lineHeight;
        }

    } else if (mode == charAbilities) {
        state.smallFont->out(xPos, yPos, "KNOWN MOVES");
        yPos += lineHeight;
        const int costPos = column2 - 4 * charWidth;
        for (unsigned i = 0; i < player->moves.size(); ++i) {
            const MoveType &move = MoveType::get(player->moves[i]);
            if (static_cast<int>(i) == selection) {
                SDL_Rect highlight = {
                    xPos, yPos,
                    column2 - xPos - charWidth, lineHeight
                };
                SDL_SetRenderDrawColor(state.renderer, 127, 127, 127, SDL_ALPHA_OPAQUE);
                SDL_RenderFillRect(state.renderer, &highlight);
            }
            char buffer[4] = { 0, ':', ' ', 0 };
            buffer[0] = i + 'A';
            state.smallFont->out(xPos, yPos, buffer + move.name);
            state.smallFont->out(costPos, yPos, std::to_string(move.cost));
            yPos += lineHeight;
        }

        if (selection >= 0 && selection < static_cast<int>(player->moves.size())) {
            yPos = paneTop;
            const MoveType &move = MoveType::get(player->moves[selection]);
            state.smallFont->out(column2, yPos, "* " + upperFirst(move.name) + " *");
            yPos += lineHeight * 2;
            state.smallFont->out(column2, yPos, "  Accuracy: " + std::to_string(move.accuracy));
            yPos += lineHeight;
            state.smallFont->out(column2, yPos, "     Speed: " + std::to_string(move.speed));
            yPos += lineHeight;
            state.smallFont->out(column2, yPos, "      Cost: " + std::to_string(move.cost));
            yPos += lineHeight;
            state.smallFont->out(column2, yPos, "      Type: " + damageTypeName(static_cast<DamageType>(move.type)));
            yPos += lineHeight;
            state.smallFont->out(column2, yPos, "Min. Range: " + std::to_string(move.minRange));
            yPos += lineHeight;
            state.smallFont->out(column2, yPos, "Max. Range: " + std::to_string(move.maxRange));
            yPos += lineHeight;
            state.smallFont->out(column2, yPos, "    Damage: " + std::to_string(move.damage));
            yPos += lineHeight;
            state.smallFont->out(column2, yPos, "      Size: " + std::to_string(move.damageSize));
            yPos += lineHeight;
            state.smallFont->out(column2, yPos, "     Shape: " + damageShapeName(move.shape));
            yPos += lineHeight;
            state.smallFont->out(column2, yPos, "      Form: " + damageFormName(move.form));
        }
    } else if (mode == charInventory) {
        state.smallFont->out(xPos, yPos, "CARRYING");
        state.smallFont->out(column2, yPos, "EQUIPPED");
    }

    const int column3 = column2 * 2;
    yPos = paneTop;
    state.smallFont->out(column3, yPos, "QUICK SLOTS");
    yPos += lineHeight * 2;
    for (int i = 0; i < quickSlotCount; ++i) {
        std::string content = "nothing";
        switch(state.quickSlots[i].type) {
            case quickSlotAbility:
                content = MoveType::get(state.quickSlots[i].action).name;
                break;
            case quickSlotItem:
                content = "item";
                break;
        }
        state.smallFont->out(column3, yPos, std::to_string(i+1) + ": " + content);
        yPos += lineHeight;
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
        ss << "  FPS: " << state.getFPS();
        state.smallFont->out(0, 0, ss.str().c_str());
    }

    state.advanceFrame();
    if (callPresent) SDL_RenderPresent(state.renderer);
}

void doCharInfo(System &system, int initialMode) {
    if (initialMode < 0 || initialMode >= charModeCount) mode = charStats;
    else mode = initialMode;
    selection = 0;

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
            } else if (event.type == SDL_KEYDOWN && keyToIndex(event.key.keysym) >= 0) {
                int choice = keyToIndex(event.key.keysym);
                if (choice < static_cast<int>(system.getPlayer()->moves.size())) {
                    selection = choice;
                }
            } else {
                const CommandDef &cmd = getCommand(system, event, characterCommands);
                switch(cmd.command) {
                    case Command::Quit:
                        system.wantsToQuit = true;
                        return;
                    case Command::Close:
                        return;
                    case Command::NextMode:
                        selection = 0;
                        ++mode;
                        if (mode >= charModeCount) {
                            mode = 0;
                        }
                        break;
                    case Command::PrevMode:
                        selection = 0;
                        --mode;
                        if (mode < 0) {
                            mode = charModeCount - 1;
                        }
                    break;

                    case Command::QuickKey_1:
                    case Command::QuickKey_2:
                    case Command::QuickKey_3:
                    case Command::QuickKey_4: {
                        int slot = static_cast<int>(cmd.command) - static_cast<int>(Command::QuickKey_1);
                        if (mode == charAbilities && selection >= 0 && selection < static_cast<int>(system.getPlayer()->moves.size())) {
                            system.quickSlots[slot].type = quickSlotAbility;
                            system.quickSlots[slot].action = system.getPlayer()->moves[selection];
                        }
                        break; }
                    case Command::Move:
                        if (mode == charAbilities) {
                            if (cmd.direction == Dir::North) {
                                if (selection > 0) {
                                    --selection;
                                }
                            } else if (cmd.direction == Dir::South) {
                                if (selection < static_cast<int>(system.getPlayer()->moves.size()) - 1) {
                                    ++selection;
                                }
                            }
                        }
                        break;
                    default:
                        /* we don't need to handle the other commands */
                        break;
                }
            }
        }
    }
}
