#include <cstdint>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL2_framerate.h>

#include "command.h"
#include "creature.h"
#include "board.h"
#include "game.h"
#include "point.h"
#include "vm.h"
#include "random.h"
#include "gfx.h"

Dir gfx_GetDirection(System &system) {
    system.messages.push_back("Which way?");
    while (1) {
        repaint(system);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            const CommandDef &cmd = getCommand(event, gameCommands);
            switch(cmd.command) {
                case Command::Move:
                    system.messages.back() += " " + dirName(cmd.direction);
                    return cmd.direction;
                case Command::Cancel:
                    system.messages.back() += " Cancelled";
                    return Dir::None;
                default:
                    /* we don't need to handle most of the commands */
                    break;
            }
        }
    }
}

bool tryInteract(System &state, const Point &target) {
    Creature *actor = state.getBoard()->actorAt(target);
    const Board::Event *event = state.getBoard()->eventAt(target);
    if (actor && actor != state.getPlayer()) {
        if (actor->talkFunc) {
            // has talk function, is friendly
            state.vm->run(actor->talkFunc);
        } else {
            // no talk function, presume hostile
            if (state.quickSlots[0].type == quickSlotAbility) {
                state.getPlayer()->useAbility(state, state.quickSlots[0].action, state.getPlayer()->position.directionTo(target));
            } else {
                state.messages.push_back("They have nothing to say.");
            }
        }
        state.requestTick();
        return true;
    } else if (event && event->type == eventTypeManual) {
        state.vm->run(event->funcAddr);
    } else {
        const TileInfo &info = TileInfo::get(state.getBoard()->getTile(target));
        int to = info.interactTo;
        if (to >= 0) {
            state.getBoard()->setTile(target, to);
            state.requestTick();
        } else if (to == interactGoDown) {
            state.down();
            state.requestTick();
        } else if (to == interactGoUp) {
            state.up();
            state.requestTick();
        }
    }
    return false;
}
void gameloop(System &state) {
    state.getBoard()->calcFOV(state.getPlayer()->position);

    Dir runDirection = Dir::None;

    while (!state.wantsToQuit) {
        if (runDirection != Dir::None) {
            bool hitWall = false;
            do {
                Point t = state.getPlayer()->position.shift(runDirection, 1);
                if (state.getPlayer()->tryMove(state.getBoard(), runDirection)) {
                    const Board::Event *e = state.getBoard()->eventAt(t);
                    if (e && e->type == eventTypeAuto) state.vm->run(e->funcAddr);
                    state.requestTick();
                } else {
                    hitWall = true;
                }
                if (state.hasTick()) {
                    state.tick();
                    gfx_frameDelay(state);
                }
            } while (!hitWall);
            runDirection = Dir::None;
            continue;
        }

        if (state.hasTick()) state.tick();
        repaint(state);

        // ////////////////////////////////////////////////////////////////////
        // event loop
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            const CommandDef &cmd = getCommand(event, gameCommands);
            switch(cmd.command) {
                case Command::None:
                case Command::Cancel:
                    // do nothing
                    break;
                case Command::SystemMenu:
                    return;
                case Command::Quit:
                    state.wantsToQuit = true;
                    break;

                case Command::Move: {
                    Point dest = state.getPlayer()->position.shift(cmd.direction);
                    if (state.getPlayer()->tryMove(state.getBoard(), cmd.direction)) {
                        const Board::Event *e = state.getBoard()->eventAt(dest);
                        if (e && e->type == eventTypeAuto) {
                            state.vm->run(e->funcAddr);
                        }
                        state.requestTick();
                    } else {
                        tryInteract(state, dest);
                    }
                    break; }
                case Command::Run:
                    runDirection = cmd.direction;
                    break;

                case Command::Interact: {
                    Dir d = cmd.direction;
                    Point target = state.getPlayer()->position.shift(d, 1);
                    tryInteract(state, target);
                    break; }
                case Command::Wait:
                    state.requestTick();
                    break;
                case Command::ShowMap:
                    doShowMap(state);
                    break;
                case Command::ShowTooltip:
                    state.showTooltip = !state.showTooltip;
                    break;

                case Command::CharacterInfo:
                    doCharInfo(state, charStats);
                    break;
                case Command::Inventory:
                    doCharInfo(state, charInventory);
                    break;
                case Command::AbilityList:
                    doCharInfo(state, charAbilities);
                    break;

                case Command::QuickKey_1:
                case Command::QuickKey_2:
                case Command::QuickKey_3:
                case Command::QuickKey_4: {
                    int slot = static_cast<int>(cmd.command) - static_cast<int>(Command::QuickKey_1);
                    switch(state.quickSlots[slot].type) {
                        case quickSlotUnused:
                            /* do nothing */
                            break;
                        case quickSlotItem:
                            /* not implemented */
                            break;
                        case quickSlotAbility: {
                            int abilityNumber = state.quickSlots[slot].action;
                            const MoveType &move = MoveType::get(abilityNumber);
                            Dir d = Dir::Here;
                            if (move.damageType != formSelf) {
                                d = gfx_GetDirection(state);
                            }
                            state.getPlayer()->useAbility(state, abilityNumber, d);
                            state.requestTick();
                            break; }
                    }
                    break; }

                case Command::Debug_Reveal:
                    state.getBoard()->dbgRevealAll();
                    break;
                case Command::Debug_NoFOV:
                    state.getBoard()->dbgToggleFOV();
                    break;
                case Command::Debug_ShowInfo:
                    state.showInfo = !state.showInfo;
                    break;
                case Command::Debug_ShowFPS:
                    state.showFPS = !state.showFPS;
                    break;

                default:
                    /* we don't need to worry about the other kinds of command */
                    break;
            }
        }

        SDL_framerateDelay(static_cast<FPSmanager*>(state.fpsManager));
    }
}