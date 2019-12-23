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
#include "gamestate.h"
#include "gfx.h"



bool tryInteract(System &renderState, const Point &target) {
    Creature *actor = renderState.game->getBoard()->actorAt(target);
    const Board::Event *event = renderState.game->getBoard()->eventAt(target);
    if (actor && actor != renderState.game->getPlayer()) {
        if (actor->talkFunc) {
            renderState.game->getVM().run(actor->talkFunc);
        } else {
            renderState.messages.push_back("They have nothing to say.");
        }
        renderState.game->requestTick();
        return true;
    } else if (event && event->type == eventTypeManual) {
        renderState.game->getVM().run(event->funcAddr);
    } else {
        const TileInfo &info = TileInfo::get(renderState.game->getBoard()->getTile(target));
        int to = info.interactTo;
        if (to >= 0) {
            renderState.game->getBoard()->setTile(target, to);
            renderState.game->requestTick();
        } else if (to == interactGoDown) {
            renderState.game->down();
            renderState.game->requestTick();
        } else if (to == interactGoUp) {
            renderState.game->up();
            renderState.game->requestTick();
        }
    }
    return false;
}
void gameloop(System &renderState) {
    GameState &state = *renderState.game;
    state.getBoard()->calcFOV(state.getPlayer()->position);

    Dir runDirection = Dir::None;

    while (!renderState.wantsToQuit) {
        if (runDirection != Dir::None) {
            bool hitWall = false;
            do {
                Point t = state.getPlayer()->position.shift(runDirection, 1);
                if (state.getPlayer()->tryMove(state.getBoard(), runDirection)) {
                    const Board::Event *e = state.getBoard()->eventAt(t);
                    if (e && e->type == eventTypeAuto) state.getVM().run(e->funcAddr);
                    state.requestTick();
                } else {
                    hitWall = true;
                }
                if (state.hasTick()) {
                    state.tick();
                    gfx_frameDelay(renderState);
                }
            } while (!hitWall);
            runDirection = Dir::None;
            continue;
        }

        if (state.hasTick()) state.tick();
        repaint(renderState);

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
                    renderState.wantsToQuit = true;
                    break;

                case Command::Move: {
                    Point dest = state.getPlayer()->position.shift(cmd.direction);
                    if (state.getPlayer()->tryMove(state.getBoard(), cmd.direction)) {
                        const Board::Event *e = state.getBoard()->eventAt(dest);
                        if (e && e->type == eventTypeAuto) {
                            state.getVM().run(e->funcAddr);
                        }
                        state.requestTick();
                    } else {
                        tryInteract(renderState, dest);
                    }
                    break; }
                case Command::Run:
                    runDirection = cmd.direction;
                    break;

                case Command::Interact: {
                    Dir d = cmd.direction;
                    Point target = state.getPlayer()->position.shift(d, 1);
                    tryInteract(renderState, target);
                    break; }
                case Command::Wait:
                    state.requestTick();
                    break;
                case Command::ShowMap:
                    showFullMap(renderState);
                    break;

                case Command::Debug_Reveal:
                    state.getBoard()->dbgRevealAll();
                    break;
                case Command::Debug_NoFOV:
                    state.getBoard()->dbgToggleFOV();
                    break;
                case Command::Debug_ShowInfo:
                    renderState.showInfo = !renderState.showInfo;
                    break;
                case Command::Debug_ShowFPS:
                    renderState.showFPS = !renderState.showFPS;
                    break;

                default:
                    /* we don't need to worry about the other kinds of command */
                    break;
            }
        }

        SDL_framerateDelay(static_cast<FPSmanager*>(renderState.fpsManager));
    }
}