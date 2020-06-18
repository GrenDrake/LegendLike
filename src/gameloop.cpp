#include <cstdint>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <vector>

#include <SDL2/SDL.h>

#include "command.h"
#include "config.h"
#include "creature.h"
#include "board.h"
#include "game.h"
#include "point.h"
#include "vm.h"
#include "random.h"
#include "gfx.h"
#include "textutil.h"

void gfx_handleInput(System &state);

struct ProjectileInfo {
    std::string name;
    int damageDice, damageSides;
};

bool basicProjectileAttack(System &state, const ProjectileInfo &projectile, Dir d) {
    Board *board = state.getBoard();
    Creature *actor = nullptr;

    Point work(state.getPlayer()->position);
    while (1) {
        work = work.shift(d);
        if (!board->valid(work)) break;
        const TileInfo &tileInfo = TileInfo::get(board->getTile(work));
        if (tileInfo.flags & TF_SOLID) {
            state.addMessage("Your " + projectile.name + " hits the " + tileInfo.name + ".");
            return false;
        }

        actor = board->actorAt(work);
        if (actor) {
            bool isHit = false;
            if (actor->typeInfo->aiType == aiEnemy) {
                isHit = doAccuracyCheck(state, state.getPlayer(), actor, 0);
            }
            if (!isHit) {
                state.addMessage("Your " + projectile.name + " misses " + actor->getName() + ".");
                actor = nullptr;
            }
            else break;
        }
    }

    if (actor) {
        int d = projectile.damageDice;
        if (d <= 0) d = state.subweaponLevel[SW_BOW];
        int damage = state.coreRNG.roll(d, projectile.damageSides);
        if (state.config->getBool("showrolls", false)) {
            std::stringstream msg;
            msg << "[damage: " << d << 'd' << projectile.damageSides << '=' << damage << ']';
            state.addInfo(msg.str());
        }
        board->doDamage(state, actor, damage, 0, "your " + projectile.name);
        return true;
    }
    return false;
}

Dir gfx_GetDirection(System &system, const std::string &prompt, bool allowHere = false) {
    system.addMessage(prompt + "; which way?");
    while (1) {
        repaint(system);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            const CommandDef &cmd = getCommand(system, event, gameCommands);
            switch(cmd.command) {
                case Command::Move:
                    system.appendMessage(" " + dirName(cmd.direction));
                    return cmd.direction;
                case Command::Cancel:
                    system.appendMessage(" Cancelled.");
                    return Dir::None;
                case Command::Wait:
                    if (allowHere) {
                        system.appendMessage(" here");
                        return Dir::Here;
                    }
                    break;
                default:
                    /* we don't need to handle most of the commands */
                    break;
            }
        }
    }
}

Point gfx_SelectTile(System &system, const std::string &prompt) {
    system.cursor = system.getPlayer()->position;
    system.addMessage("Where?");
    while (1) {
        std::stringstream line;
        line << prompt << "; where? ";
        line << system.cursor << ' ';
        Creature *actor = system.getBoard()->actorAt(system.cursor);
        if (actor) line << actor->getName() << " standing at ";
        line << TileInfo::get(system.getBoard()->getTile(system.cursor)).name;
        system.replaceMessage(line.str());
        repaint(system);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            const CommandDef &cmd = getCommand(system, event, gameCommands);
            switch(cmd.command) {
                case Command::Move: {
                    Point newpos = system.cursor.shift(cmd.direction);
                    if (system.getBoard()->valid(newpos)) {
                        system.cursor = newpos;
                    }
                    break; }
                case Command::SelectItem:
                case Command::Interact: {
                    std::stringstream line;
                    line << "Selected " << system.cursor << ".";
                    system.replaceMessage(line.str());
                    Point result = system.cursor;
                    system.cursor = Point(-1,-1);
                    return result; }
                case Command::Cancel:
                    system.removeMessage();
                    system.cursor = Point(-1,-1);
                    return Point(-1, -1);
                default:
                    /* we don't need to handle most of the commands */
                    break;
            }
        }
    }
}

bool tryInteract(System &state, Dir d, const Point &target) {
    Creature *actor = state.getBoard()->actorAt(target);
    const Board::Event *event = state.getBoard()->eventAt(target);
    if (actor && actor != state.getPlayer()) {
        if (actor->typeInfo->aiType == aiPushable) {
            actor->tryMove(state.getBoard(), d);
        } else if (actor->typeInfo->aiType != aiEnemy) {
            if (actor->talkFunc) state.vm->run(actor->talkFunc);
            else                    state.addMessage(upperFirst(actor->getName()) + " has nothing to say.");
        } else if (state.swordLevel <= 0) {
            state.addMessage("You don't have a sword!");
        } else {
            bool isHit = doAccuracyCheck(state, state.getPlayer(), actor, 2);
            if (!isHit) {
                state.addMessage("You miss " + actor->getName() + ".");
            } else {
                int roll = state.coreRNG.roll(state.swordLevel, 4);
                if (state.config->getBool("showrolls", false)) {
                    std::stringstream msg2;
                    msg2 << "[damage: " << state.swordLevel << 'd' << 4 << '=' << roll << ']';
                    state.addInfo(msg2.str());
                }
                state.getBoard()->doDamage(state, actor, roll, 0, "your attack");
            }
        }
        state.requestTick();
        return true;
    } else if (event && event->type == eventTypeManual) {
        state.vm->run(event->funcAddr);
        state.requestTick();
        return true;
    } else {
        const TileInfo &info = TileInfo::get(state.getBoard()->getTile(target));
        int to = info.interactTo;
        if (to >= 0) {
            state.getBoard()->setTile(target, to);
            state.requestTick();
            return true;
        } else if (to == interactGoDown) {
            state.down();
            state.requestTick();
            return true;
        } else if (to == interactGoUp) {
            state.up();
            state.requestTick();
            return true;
        }
    }
    return false;
}

void gameloop(System &state) {
    state.returnToMenu = false;
    state.getBoard()->calcFOV(state.getPlayer()->position);

    state.runDirection = Dir::None;
    state.lastticks = SDL_GetTicks();

    while (!state.wantsToQuit && !state.returnToMenu) {
        if (state.runDirection != Dir::None) {
            bool hitWall = false;
            const Point initalTilePos = state.getPlayer()->position.shift(state.runDirection, 1);
            const TileInfo &initialTile = TileInfo::get(state.getBoard()->getTile(initalTilePos));
            do {
                Point t = state.getPlayer()->position.shift(state.runDirection, 1);
                if (state.getPlayer()->tryMove(state.getBoard(), state.runDirection)) {
                    const TileInfo &thisTile = TileInfo::get(state.getBoard()->getTile(state.getPlayer()->position));
                    if (initialTile.group != thisTile.group) {
                        hitWall = true;
                    }

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
            state.runDirection = Dir::None;
            continue;
        }

        while (!state.animationQueue.empty()) {
            if (repaint(state)) {
                SDL_Delay(200);
                SDL_PumpEvents();
            }
        }
        if (state.hasTick()) state.tick();
        repaint(state);

        gfx_handleInput(state);
        if (state.wantsToQuit) {
            if (!gfx_Confirm(state, "Are you sure you want to quit?", "Your progress will NOT be saved.")) {
                state.wantsToQuit = false;
            }
        }
    }

}

void gfx_handleInput(System &state) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        const CommandDef &cmd = getCommand(state, event, gameCommands);
        switch(cmd.command) {
            case Command::None:
            case Command::Cancel:
                // do nothing
                break;
            case Command::SystemMenu:
                state.returnToMenu = true;
                return;
            case Command::Quit:
                state.wantsToQuit = true;
                break;

            case Command::Move: {
                if (state.mapEditMode) {
                    Creature *player = state.getPlayer();
                    player->position = player->position.shift(cmd.direction);
                    break;
                }
                Point dest = state.getPlayer()->position.shift(cmd.direction);
                if (state.getPlayer()->tryMove(state.getBoard(), cmd.direction)) {
                    const Board::Event *e = state.getBoard()->eventAt(dest);
                    if (e && e->type == eventTypeAuto) {
                        state.vm->run(e->funcAddr);
                    }
                    state.requestTick();
                } else {
                    tryInteract(state, cmd.direction, dest);
                }
                break; }
            case Command::Run: {
                if (state.mapEditMode) {
                    Point dest = state.getPlayer()->position.shift(cmd.direction);
                    state.getBoard()->setTile(dest, state.mapEditTile);
                    break;
                }
                Dir d = cmd.direction;
                if (d == Dir::None) {
                    d = gfx_GetDirection(state, "Run");
                    if (d == Dir::None) break;
                }
                state.runDirection = d;
                break; }

            case Command::Interact: {
                if (state.mapEditMode) {
                    Point dest = state.getPlayer()->position.shift(cmd.direction);
                    state.mapEditTile = state.getBoard()->getTile(dest);
                    break;
                }
                Dir d = cmd.direction;
                if (d == Dir::None) {
                    d = gfx_GetDirection(state, "Activate", true);
                    if (d == Dir::None) break;
                }
                Point target = state.getPlayer()->position.shift(d, 1);
                if (!tryInteract(state, d, target)) {
                    state.addMessage("Nothing to do!");
                }
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

            case Command::Inventory:
                doCharInfo(state);
                break;

            case Command::Examine:
                if (gfx_SelectTile(state, "Looking at").x() >= 0) {
                    state.removeMessage();
                }
                break;


            case Command::NextSubweapon: {
                if (state.mapEditMode) {
                    ++state.mapEditTile;
                    break;
                }
                bool hasSubweapon = false;
                for (int i = 0; i < SW_COUNT; ++i) {
                    if (state.subweaponLevel[i] > 0) {
                        hasSubweapon = true;
                        break;
                    }
                }
                if (!hasSubweapon) {
                    state.currentSubweapon = -1;
                } else {
                    do {
                        ++state.currentSubweapon;
                        if (state.currentSubweapon >= SW_COUNT) state.currentSubweapon = 0;
                    } while (state.subweaponLevel[state.currentSubweapon] == 0);
                }
                break; }
            case Command::PrevSubweapon: {
                if (state.mapEditMode) {
                    if (state.mapEditTile) --state.mapEditTile;
                    break;
                }
                bool hasSubweapon = false;
                for (int i = 0; i < SW_COUNT; ++i) {
                    if (state.subweaponLevel[i] > 0) {
                        hasSubweapon = true;
                        break;
                    }
                }
                if (!hasSubweapon) {
                    state.currentSubweapon = -1;
                } else {
                    do {
                        --state.currentSubweapon;
                        if (state.currentSubweapon < 0) state.currentSubweapon = SW_COUNT - 1;
                    } while (state.subweaponLevel[state.currentSubweapon] == 0);
                }
                break; }
            case Command::Subweapon: {
                if (state.mapEditMode) {
                    Dir d = gfx_GetDirection(state, "Shift map");
                    if (d == Dir::None) break;
                    state.getBoard()->dbgShiftMap(d);
                    break;
                }
                if (state.currentSubweapon == -1) {
                    state.addMessage("You don't have any subweapons.");
                    break;
                }
                Dir d = Dir::None;
                if (state.subweapons[state.currentSubweapon].directional) {
                    d = gfx_GetDirection(state, state.subweapons[state.currentSubweapon].name);
                    if (d == Dir::None) break;
                }
                switch(state.currentSubweapon) {
                    case SW_BOW:
                        if (state.arrowCount <= 0) {
                            state.addMessage("Out of ammo!");
                            break;
                        }
                        --state.arrowCount;
                        state.requestTick();
                        basicProjectileAttack(state, ProjectileInfo{"arrow", 0, 6}, d);
                        break;
                    case SW_FIREROD:
                        if (state.getPlayer()->curEnergy < 3) {
                            state.addMessage("Out of energy!");
                            break;
                        }
                        state.getPlayer()->curEnergy -= 3;
                        state.requestTick();
                        basicProjectileAttack(state, ProjectileInfo{"fire bolt", 2, 6}, d);
                        break;
                    case SW_ICEROD:
                        if (state.getPlayer()->curEnergy < 3) {
                            state.addMessage("Out of energy!");
                            break;
                        }
                        state.getPlayer()->curEnergy -= 3;
                        state.requestTick();
                        basicProjectileAttack(state, ProjectileInfo{"ice bolt", 2, 6}, d);
                        break;
                    case SW_PICKAXE: {
                        Creature *actor = state.getBoard()->actorAt(state.getPlayer()->position.shift(d));
                        if (actor) {
                            state.requestTick();
                            bool isHit = doAccuracyCheck(state, state.getPlayer(), actor, -2);
                            if (!isHit) {
                                state.addMessage("You miss " + actor->getName() + ".");
                            } else {
                                int roll = state.coreRNG.roll(3, 4);
                                if (state.config->getBool("showrolls", false)) {
                                    std::stringstream msg2;
                                    msg2 << "[damage: 3d4" << '=' << roll << ']';
                                    state.addInfo(msg2.str());
                                }
                                state.getBoard()->doDamage(state, actor, roll, 0, "your pickaxe");
                            }
                        } else {
                            state.addMessage("Your pickaxe has no impact.");
                        }
                        break; }
                    default:
                        state.addError("The " + state.subweapons[state.currentSubweapon].name + " is not implemented.");
                }
                break; }

            case Command::Debug_MapEditMode:
                state.mapEditMode = !state.mapEditMode;
                if (state.mapEditMode)  state.addError("Entering map editing mode");
                else                    state.addError("Exiting map editing mode");
                break;
            case Command::Debug_Restore:
                state.getPlayer()->curHealth = state.getPlayer()->typeInfo->maxHealth;
                state.getPlayer()->curEnergy = state.getPlayer()->typeInfo->maxEnergy;
                state.bombCount = state.bombCapacity;
                state.arrowCount = state.arrowCapacity;
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
            case Command::Debug_WriteMapBinary: {
                if (state.getBoard()->writeToFile("binary.map")) {
                    state.addError("Wrote map to disk as binary.map.");
                } else {
                    state.addError("Failed to write map to disk.");
                }
                break; }
            case Command::Debug_TestPathfinder: {
                Board *board = state.getBoard();
                board->resetMark();
                Point src = state.getPlayer()->position;
                Point dest = board->findRandomTile(state.coreRNG, tileFloor);
                std::stringstream line;
                line << "Finding path from " << src << " to " << dest << '.';
                state.addError(line.str());
                auto points = board->findPath(src, dest);
                if (points.empty()) {
                    state.addError("No path found.");
                } else {
                    state.addError("Path has " + std::to_string(points.size()) + " tiles.");
                    for (const Point &p : points) {
                        board->at(p).mark = true;
                    }
                }
                break; }

            default:
                /* we don't need to worry about the other kinds of command */
                break;
        }
    }
}
