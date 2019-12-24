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



bool tryInteract(System &state, const Point &target) {
    Creature *actor = state.getBoard()->actorAt(target);
    const Board::Event *event = state.getBoard()->eventAt(target);
    if (actor && actor != state.getPlayer()) {
        if (actor->talkFunc) {
            state.vm->run(actor->talkFunc);
        } else {
            state.messages.push_back("They have nothing to say.");
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
                case Command::ReturnToMenu:
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
                    showFullMap(state);
                    break;

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