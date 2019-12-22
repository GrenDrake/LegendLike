#include <SDL2/SDL.h>
#include <SDL2/SDL2_framerate.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <algorithm>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>

#include "game.h"
#include "beast.h"
#include "command.h"
#include "gamestate.h"
#include "gfx.h"
#include "random.h"
#include "logger.h"

static int getRandomMove(Beast *beast, Random &rng);
static bool beastOrder(Beast *left, Beast *right);
static void processCombatRound(System &system);

static const char *playerCommands[] = {
    "Fight",
    "Run",
    nullptr
};

static CombatResult combatStatus(System &system) {
    CombatResult result = CombatResult::None;
    if (!system.party.alive()) result = CombatResult::Defeat;
    if (!system.combatInfo->foes.alive()) result = CombatResult::Victory;
    return result;
}

static void advanceBeast(System &system) {
    int value = system.combatInfo->curBeast;
    const Beast *who = nullptr;
    do {
        ++value;
        who = system.party.at(value);
    } while ((!who || who->isKOed()) && value < GameState::PARTY_SIZE);
    system.combatInfo->curBeast = value;
    system.combatInfo->curSelection = 0;
}

static void rewindBeast(System &system) {
    int value = system.combatInfo->curBeast;
    const Beast *who = nullptr;
    do {
        --value;
        who = system.party.at(value);
    } while ((!who || who->isKOed()) && value >= 0);
    if (value < 0) return;

    system.combatInfo->curBeast = value;
    system.combatInfo->curSelection = 0;
}

void paintCombat(System &system, bool doPaint) {
    const int lineHeight = system.smallFont->getLineHeight();

    int xPos = 0, yPos = 0;

    // //// //// //// //// //// //// //// //// //// //// //// //// //// ////
    // LOAD ART ASSETS
    SDL_Texture *bg = system.getImage("combatbg/" + std::to_string(system.combatInfo->background) + ".png");
    SDL_Texture *texGround = system.getImage("ground.png");
    SDL_Texture *ew = system.getImage("ui/ew.png");
    SDL_Texture *ns = system.getImage("ui/ns.png");
    SDL_Texture *esw = system.getImage("ui/esw.png");
    int ewWidth = 0, ewHeight = 0;
    SDL_QueryTexture(ew, nullptr, nullptr, &ewWidth, &ewHeight);

    // //// //// //// //// //// //// //// //// //// //// //// //// //// ////
    // CLEAR FRAME
    SDL_SetRenderDrawColor(system.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(system.renderer);

    // //// //// //// //// //// //// //// //// //// //// //// //// //// ////
    // DRAW COMBAT BACKGROUND
    const int environmentHeight = 192 / 4;
    int environmentWidth = 0;
    SDL_QueryTexture(bg, nullptr, nullptr, &environmentWidth, nullptr);
    environmentWidth /= 4;
    const int environmentAreaX = 0;
    SDL_Rect bgDest = { environmentAreaX, 0, environmentWidth, environmentHeight };
    for (int i = 0; i < screenWidth + 1; ++i) {
        SDL_RenderCopy(system.renderer, bg, nullptr, &bgDest);
        bgDest.x += environmentWidth;
    }

    // //// //// //// //// //// //// //// //// //// //// //// //// //// ////
    // DRAW COMBAT LOG
    const int combatLogWidth = 30;
    const int uiHeight = 8;
    const int uiTop = screenHeight - lineHeight * uiHeight;
    int msgNo = system.combatInfo->log.size() - 1;
    const unsigned combatLogLines = (screenHeight - uiTop) / system.tinyFont->getLineHeight();
    yPos = screenHeight - system.tinyFont->getLineHeight();
    xPos = 0;
    for (unsigned i = 0; i < combatLogLines && msgNo >= 0; ++i) {
        system.tinyFont->out(xPos, yPos, system.combatInfo->log[msgNo]);
        yPos -= system.tinyFont->getLineHeight();
        --msgNo;
    }
    const int combatLogRight = system.tinyFont->getCharWidth() * combatLogWidth;


    // //// //// //// //// //// //// //// //// //// //// //// //// //// ////
    // DRAW COMMAND INFO
    const int commandsWidth = 15;
    const int commandsLeft = combatLogRight + ewWidth;
    if (combatStatus(system) == CombatResult::None) {
        const int xPos = commandsLeft;
        int yPos = uiTop;
        SDL_Rect highlightBox = {
            xPos,
            yPos + (system.combatInfo->curSelection + 2) * lineHeight,
            system.smallFont->getCharWidth() * commandsWidth,
            lineHeight
        };
        SDL_SetRenderDrawColor(system.renderer, 127, 127, 127, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(system.renderer, &highlightBox);

        if (system.combatInfo->playerAction < 0) {
            system.smallFont->out(xPos, yPos, "Round " + std::to_string(system.combatInfo->roundNumber));
            yPos += lineHeight * 2;

            for (int i = 0; playerCommands[i] != nullptr; ++i) {
                system.smallFont->out(xPos, yPos, playerCommands[i]);
                yPos += lineHeight;
            }
        } else {
            Beast *current = system.party.at(system.combatInfo->curBeast);
            if (current) {
                system.smallFont->out(xPos, yPos, current->getName());
                yPos += lineHeight * 2;

                for (int i = 0; i < 4; ++i) {
                    int moveId = current->move[i];
                    if (moveId > 0) {
                        const MoveType &info = MoveType::get(moveId);
                        system.smallFont->out(xPos, yPos, info.name);
                    }
                    yPos += lineHeight;
                }
            }
        }
    }
    const int commandsRight = commandsLeft + system.smallFont->getCharWidth() * commandsWidth;

    // //// //// //// //// //// //// //// //// //// //// //// //// //// ////
    // DRAW PLAYER BEAST INFO
    const int maxNameLength = 20;
    const int hpTextLength = 11;
    const int infoLeft = commandsRight + ewWidth;
    const int infoHP = infoLeft + system.smallFont->getCharWidth() * maxNameLength;
    const int infoEN = infoHP + system.smallFont->getCharWidth() * hpTextLength;
    const int infoAction = infoEN + system.smallFont->getCharWidth() * hpTextLength;
    system.smallFont->out(infoLeft, uiTop, "Name");
    system.smallFont->out(infoHP, uiTop, "Health");
    system.smallFont->out(infoEN, uiTop, "Energy");
    system.smallFont->out(infoAction, uiTop, "Next Action");
    yPos = uiTop + lineHeight * 1.2;
    for (int i = 0; i < GameState::PARTY_SIZE; ++i) {
        Beast *beast = system.party.at(i);
        if (beast) {
            system.smallFont->out(infoLeft, yPos, beast->getName().substr(0,maxNameLength));
            system.smallFont->out(infoHP, yPos, std::to_string(beast->curHealth) + "/" + std::to_string(beast->getStat(Stat::Health)));
            system.smallFont->out(infoEN, yPos, std::to_string(beast->curEnergy) + "/" + std::to_string(beast->getStat(Stat::Energy)));
            if (beast->nextAction >= 0) {
                const MoveType &info = MoveType::get(beast->nextAction);
                system.smallFont->out(infoAction, yPos, info.name);
            }
        }
        yPos += lineHeight;
    }

    // //// //// //// //// //// //// //// //// //// //// //// //// //// ////
    // DRAW BEAST ART
    const int artAreaSize = uiTop - ewHeight - environmentHeight;
    const int defaultArtAreaSize = (1080 - lineHeight * uiHeight) - ewHeight - environmentHeight;
    const double scaleFactor = static_cast<double>(artAreaSize) / defaultArtAreaSize;
    const int artSize = 256 * scaleFactor;
    const int groundPatchHeight = 64 * scaleFactor;
    const int artGap = 32 * scaleFactor;
    const int barHeight = system.smallFont->getCharHeight() / 2;
    const int artBlockHeight = artSize * 2 + artGap;
    const int yArtOffset = (artAreaSize - artBlockHeight) / 2;
    const int foeArtX = artGap;
    const int allyArtX = screenWidth - artAreaSize - artGap;

    SDL_Rect foeArtBoxes[4] = {
        { foeArtX + artSize + artGap,   yArtOffset + environmentHeight,                     artSize, artSize },
        { foeArtX + artSize + artGap,   yArtOffset + environmentHeight + artSize + artGap,  artSize, artSize },
        { foeArtX,                      yArtOffset + environmentHeight,                     artSize, artSize },
        { foeArtX,                      yArtOffset + environmentHeight + artSize + artGap,  artSize, artSize },
    };
    SDL_Rect allyArtBoxes[4] = {
        { allyArtX,                     yArtOffset + environmentHeight,                     artSize, artSize },
        { allyArtX,                     yArtOffset + environmentHeight + artSize + artGap,  artSize, artSize },
        { allyArtX + artSize + artGap,  yArtOffset + environmentHeight,                     artSize, artSize },
        { allyArtX + artSize + artGap,  yArtOffset + environmentHeight + artSize + artGap,  artSize, artSize },
    };

    for (int i = 0; i < GameState::PARTY_SIZE; ++i) {
        Beast *foe = system.combatInfo->foes.at(i);
        Beast *ally = system.party.at(i);

        if (foe) {
            double healthPercent = static_cast<double>(foe->curHealth) / foe->getStat(Stat::Health);
            const int barX = foeArtBoxes[i].x;

            SDL_SetRenderDrawColor(system.renderer, 127, 127, 127, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(system.renderer, &foeArtBoxes[i]);

            SDL_Rect groundBox = {
                foeArtBoxes[i].x, foeArtBoxes[i].y + artSize - groundPatchHeight / 2,
                foeArtBoxes[i].w, groundPatchHeight
            };
            SDL_RenderCopy(system.renderer, texGround, nullptr, &groundBox);
            SDL_RenderCopyEx(system.renderer, foe->art, nullptr, &foeArtBoxes[i], 0, nullptr, SDL_FLIP_HORIZONTAL);

            int yPos = foeArtBoxes[i].y;
            const int nameOffset = (artSize - foe->getName().size() * system.smallFont->getCharWidth()) / 2;
            system.smallFont->out(barX + nameOffset, yPos, foe->getName().substr(0, maxNameLength));
            yPos += lineHeight;
            gfx_DrawBar(system, barX, yPos, artSize, barHeight, healthPercent, Color{63, 192, 63});
        }

        if (ally) {
            double healthPercent = static_cast<double>(ally->curHealth) / ally->getStat(Stat::Health);
            double energyPercent = static_cast<double>(ally->curEnergy) / ally->getStat(Stat::Energy);
            const int barX = allyArtBoxes[i].x;

            SDL_SetRenderDrawColor(system.renderer, 127, 127, 127, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(system.renderer, &allyArtBoxes[i]);

            SDL_Rect groundBox = {
                allyArtBoxes[i].x, allyArtBoxes[i].y + artSize - groundPatchHeight / 2,
                allyArtBoxes[i].w, groundPatchHeight
            };
            SDL_RenderCopy(system.renderer, texGround, nullptr, &groundBox);
            SDL_RenderCopy(system.renderer, ally->art, nullptr, &allyArtBoxes[i]);

            int yPos = allyArtBoxes[i].y;
            const int nameOffset = (artSize - ally->getName().size() * system.smallFont->getCharWidth()) / 2;
            system.smallFont->out(barX + nameOffset, yPos, ally->getName().substr(0, maxNameLength));
            yPos += lineHeight;
            gfx_DrawBar(system, barX, yPos, artSize, barHeight, healthPercent, Color{63, 192, 63});
            yPos += barHeight;
            gfx_DrawBar(system, barX, yPos, artSize, barHeight, energyPercent, Color{63, 63, 182});
        }
    }

    // //// //// //// //// //// //// //// //// //// //// //// //// //// ////
    // DRAW UI FRAME
    for (int i = 0; i < screenWidth; i += ewWidth) {
        SDL_Rect box = { i, uiTop - ewHeight, ewWidth, ewHeight };
        SDL_RenderCopy(system.renderer, ew, nullptr, &box);
    }
    for (int i = uiTop; i < screenHeight; i += ewHeight) {
        SDL_Rect box = { combatLogRight, i, ewWidth, ewHeight };
        SDL_RenderCopy(system.renderer, ns, nullptr, &box);
        box.x = commandsRight;
        SDL_RenderCopy(system.renderer, ns, nullptr, &box);
    }
    SDL_Rect uiCorner = { combatLogRight, uiTop - ewHeight, ewWidth, ewHeight };
    SDL_RenderCopy(system.renderer, esw, nullptr, &uiCorner);
    uiCorner.x = commandsRight;
    SDL_RenderCopy(system.renderer, esw, nullptr, &uiCorner);

    // //// //// //// //// //// //// //// //// //// //// //// //// //// ////
    // FINISH OFF AND PRESENT FRAME
    if (system.showFPS) {
        std::stringstream ss;
        ss << "  FPS: " << system.fps;
        system.smallFont->out(0, 0, ss.str().c_str());
    }
    if (doPaint) SDL_RenderPresent(system.renderer);
}

void gfx_combatFrameDelay(System &system) {
    paintCombat(system, true);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        const CommandDef &cmd = getCommand(event, gameCommands);
        if (cmd.command == Command::Quit) {
            system.wantsToQuit = true;
            return;
        }
    }
    SDL_framerateDelay(static_cast<FPSmanager*>(system.fpsManager));
}

void gfx_combatWaitKey(System &system) {
    paintCombat(system, true);
    SDL_Event event;
    while (1) {
        while (SDL_PollEvent(&event)) {
            const CommandDef &cmd = getCommand(event, gameCommands);
            if (cmd.command == Command::Quit) {
                system.wantsToQuit = true;
                return;
            }
            if (event.type == SDL_KEYDOWN) return;
            if (event.type == SDL_MOUSEBUTTONUP) return;
        }
    }
    SDL_framerateDelay(static_cast<FPSmanager*>(system.fpsManager));
}

std::string beastName(Beast *beast) {
    if (!beast) return "\x1B\x02\xFF\x00\x00(null)\x1B\x01";
    std::string colour;
    if (beast->inParty) colour = "\x1B\x02\x7F\xFF\x7F";
    else                colour = "\x1B\x02\xFF\x7F\x7F";
    return colour + beast->getName() + "\x1B\x01";
}

CombatResult doCombat(System &system, CombatInfo &info) {
    system.combatInfo = &info;

    info.roundNumber = 1;
    info.log.push_back("\x1B\x02\xFF\xFF\x01\x43ombat begins!");

    system.combatInfo->curBeast = 0;
    system.combatInfo->curSelection = 0;
    system.combatInfo->playerAction = -1;
    system.combatInfo->retreatFlag = false;
    for (int i = 0; i < GameState::PARTY_SIZE; ++i) {
        Beast *p = system.party.at(i);
        if (p) {
            p->nextAction = -1;
            p->art = system.getImage("beasts/" + std::to_string(p->typeIdent) + ".png");
        }
        Beast *f = info.foes.at(i);
        if (f) {
            f->art = system.getImage("beasts/" + std::to_string(f->typeIdent) + ".png");
        }
    }

    while (1) {
        paintCombat(system, true);

        Beast *current = system.party.at(system.combatInfo->curBeast);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            const CommandDef &cmd = getCommand(event, combatCommands);
            switch(cmd.command) {
                case Command::Quit:
                    system.wantsToQuit = true;
                    return CombatResult::QuitGame;
                case Command::Move:
                    if (cmd.direction == Dir::North && system.combatInfo->curSelection > 0) {
                        --system.combatInfo->curSelection;
                    } else if (cmd.direction == Dir::South) {
                        if (system.combatInfo->playerAction < 0) {
                            if (playerCommands[system.combatInfo->curSelection + 1] != nullptr) {
                                ++system.combatInfo->curSelection;
                            }
                        } else if (system.combatInfo->curSelection < Beast::MAX_MOVES - 1) {
                            ++system.combatInfo->curSelection;
                        }
                    }
                    break;
                case Command::Interact: {
                    if (system.combatInfo->playerAction < 0) {
                        system.combatInfo->playerAction = system.combatInfo->curSelection;
                        system.combatInfo->curSelection = 0;
                        if (system.combatInfo->playerAction == 1) system.combatInfo->curBeast = 99;
                    } else {
                        if (!current) break;
                        int move = current->move[system.combatInfo->curSelection];
                        if (move > 0) {
                            current->nextAction = move;
                            advanceBeast(system);
                        }
                    }
                    break; }
                case Command::Cancel:
                    if (system.combatInfo->curBeast > 0) {
                        rewindBeast(system);
                        current = system.party.at(system.combatInfo->curBeast);
                        current->nextAction = -1;
                    }
                    break;
                case Command::Debug_ShowFPS:
                    system.showFPS = !system.showFPS;
                    break;
                case Command::Debug_Heal:
                    for (int i = 0; i < GameState::PARTY_SIZE; ++i) {
                        Beast *b = system.party.at(i);
                        if (b) {
                            b->reset();
                        }
                    }
                    break;
                default:
                    /* we don't need to handle the other command types */
                    break;
            }
        }
        SDL_framerateDelay(static_cast<FPSmanager*>(system.fpsManager));

        if (system.combatInfo->curBeast >= GameState::PARTY_SIZE) {
            processCombatRound(system);
        }

        if (system.combatInfo->retreatFlag) {
            system.combatInfo->log.push_back("");
            system.combatInfo->log.push_back("Press a key to continue.");
            gfx_combatWaitKey(system);
            return CombatResult::Retreat;
        }
        CombatResult result = combatStatus(system);
        if (result != CombatResult::None) {
            for (int i = 0; i < GameState::PARTY_SIZE; ++i) {
                Beast *beast = system.party.at(i);
                if (beast && beast->xp > beast->getStat(Stat::XP)) {
                    int oldStats[statCount];
                    for (int j = 0; j < statCount; ++j) {
                        oldStats[j] = beast->getStat(static_cast<Stat>(j));
                    }

                    beast->xp -= beast->getStat(Stat::XP);
                    beast->autolevel(beast->level + 1, system.coreRNG);

                    system.combatInfo->log.push_back(beastName(beast) + " is now level " + std::to_string(beast->level) + ".");
                    for (int j = 0; j < statCount - 1; ++j) {
                        if (oldStats[j] != beast->getStat(static_cast<Stat>(j))) {
                            int diff = beast->getStat(static_cast<Stat>(j)) - oldStats[j];
                            std::stringstream line;
                            line << static_cast<Stat>(j);
                            if (diff > 0)   line << " increases by ";
                            else            line << " decreases by ";
                            line << diff << '.';
                            system.combatInfo->log.push_back(line.str());
                        }
                    }
                }
            }

            system.combatInfo->log.push_back("");
            if (result == CombatResult::Victory) system.combatInfo->log.push_back("\x1B\x02\x7F\xFF\x7FYou are victorious.");
            if (result == CombatResult::Defeat)  system.combatInfo->log.push_back("\x1B\x02\xFF\x7F\x7FYou have been defeated.");
            system.combatInfo->log.push_back("Press a key to continue.");
            gfx_combatWaitKey(system);
            return result;
        }
    }
}

// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////
// COMBAT ROUND HANDLING
// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////

int getRandomMove(Beast *beast, Random &rng) {
    if (!beast) return -1;

    int count = 0;
    int moves[Beast::MAX_MOVES];
    for (int i = 0; i < Beast::MAX_MOVES; ++i) {
        if (beast->move[i] > 0) moves[count++] = beast->move[i];
    }

    if (count == 0) return -1;
    return moves[rng.next32() % count];
}

bool beastOrder(Beast *left, Beast *right) {
    return left->getStat(Stat::Speed) < right->getStat(Stat::Speed);
}

void processCombatRound(System &system) {
    switch(system.combatInfo->playerAction) {
        case 0: // FIGHT
            /* no special processing required */
            break;
        case 1: // RUN
            system.combatInfo->log.push_back("Player attempts to flee.");
            if (system.coreRNG.next32() % 2) {
                system.combatInfo->log.push_back("The attempt failed!");
            } else {
                system.combatInfo->log.push_back("They escape.");
                system.combatInfo->retreatFlag = true;
                return;
            }
            break;
    }
    system.combatInfo->playerAction = -1;

    std::vector<Beast*> beasts;
    // build the combat turn list
    for (int i = 0; i < BeastParty::MAX_SIZE; ++i) {
        Beast *beast = system.combatInfo->foes.at(i);
        if (beast) {
            int move = getRandomMove(beast, system.coreRNG);
            if (move > 0) beast->nextAction = move;
            else          beast->nextAction = -1;
            beasts.push_back(beast);
        }
    }
    for (int i = 0; i < GameState::PARTY_SIZE; ++i) {
        Beast *beast = system.party.at(i);
        if (beast) beasts.push_back(beast);
    }
    std::sort(beasts.begin(), beasts.end(), beastOrder);

    for (Beast *beast : beasts) {
        if (beast->isKOed()) continue;
        if (beast->nextAction < 0) continue;

        const MoveType &info = MoveType::get(beast->nextAction);
        beast->nextAction = -1;
        system.combatInfo->log.push_back(beastName(beast) + " uses \x1B\x02\x7F\xFF\xFF" + info.name + "\x1B\x01.");
        Beast *target = nullptr;
        if (beast->inParty) target = system.combatInfo->foes.getRandom(system.coreRNG);
        else                target = system.party.getRandom(system.coreRNG);
        if (!target) {
            system.combatInfo->log.push_back(beastName(beast) + " does nothing.");
        } else {
            double randMod = system.coreRNG.between(217, 255) / 255.0;
            int damage = ((((2.0 * beast->level) / 5.0 + 2.0) * info.power * beast->getStat(Stat::Attack) / target->getStat(Stat::Defense)) / 50.0 + 2.0) * randMod;
            double modifier = 1.0;
            for (int i = 0; i < 3; ++i) {
                int t = target->typeInfo->type[i];
                if (t >= 0) {
                    double effect = TypeInfo::getTypeEffectiveness(info.type, t);
                    modifier *= effect;
                }
            }
            damage *= modifier;
            if (modifier <= 0.5) system.combatInfo->log.push_back("It's super weak!");
            else if (modifier <= 0.7) system.combatInfo->log.push_back("It's especially weak!");
            else if (modifier <= 0.9) system.combatInfo->log.push_back("It's unusually weak!");
            else if (modifier >= 1.1) system.combatInfo->log.push_back("It's unusually effective!");
            else if (modifier >= 1.3) system.combatInfo->log.push_back("It's especially effective!");
            else if (modifier >= 1.5) system.combatInfo->log.push_back("It's super effective!");
            system.combatInfo->log.push_back(beastName(target) + " takes \x1B\x02\xFF\x7F\x7F" + std::to_string(damage) + "\x1B\x01 damage.");
            target->takeDamage(damage);
            if (target->isKOed()) {
                system.combatInfo->log.push_back(beastName(target) + " is knocked out!");
                if (!target->inParty) {
                    // gain XP
                    for (int i = 0; i < GameState::PARTY_SIZE; ++i) {
                        Beast *b = system.party.at(i);
                        if (!b) continue;
                        int xp = 0;
                        if (b == beast) {
                            xp = target->getStat(Stat::XP) / 10;
                        } else {
                            xp = target->getStat(Stat::XP) / 10 * 0.666;
                        }
                        b->xp += xp;
                        system.combatInfo->log.push_back(beastName(b) + " gains " + std::to_string(xp) + " experience.");
                    }
                }
            }
            gfx_combatFrameDelay(system);
        }
    }

    system.combatInfo->curBeast = -1;
    advanceBeast(system);
    ++system.combatInfo->roundNumber;
    CombatResult result = combatStatus(system);
    if (result == CombatResult::None) {
        system.combatInfo->log.push_back("");
        system.combatInfo->log.push_back("\x1B\x02\xFF\xFF\x01* Round " + std::to_string(system.combatInfo->roundNumber) + ". *");
    }

}