#include <cstdint>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <vector>

#include <SDL2/SDL.h>

#include "command.h"
#include "creature.h"
#include "board.h"
#include "game.h"
#include "point.h"
#include "vm.h"
#include "random.h"
#include "gfx.h"
#include "textutil.h"

void gfx_handleInput(System &state);

int getAccuracyModifier(Creature *attacker, Creature *target, bool isMelee) {
    return isMelee ? 2 : 0;
}

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
            int roll = -10000;
            int modifier = getAccuracyModifier(state.getPlayer(), actor, false);
            if (actor->aiType == aiEnemy) {
                roll = state.coreRNG.roll(1,10);
                if (state.showDieRolls) {
                    std::stringstream msg;
                    msg << "[to hit: 1d10+" << modifier << "=" << (roll+modifier) << " > 5: ";
                    if (roll + modifier > 5)    msg << "hit]";
                    else                        msg << "miss]";
                    state.addMessage(msg.str());
                }
            }
            if (roll + modifier <= 5) {
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
        if (state.showDieRolls) {
            std::stringstream msg;
            msg << "[damage: " << d << 'd' << projectile.damageSides << '=' << damage << ']';
            state.addMessage(msg.str());
        }
        actor->takeDamage(damage);
        state.addMessage(upperFirst(actor->getName()) + " takes " + std::to_string(damage) + " damage from your " + projectile.name + ". ");
        if (actor->curHealth <= 0) {
            state.appendMessage(upperFirst(actor->getName()) + " is defeated! ");
        }
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

bool tryInteract(System &state, const Point &target) {
    Creature *actor = state.getBoard()->actorAt(target);
    const Board::Event *event = state.getBoard()->eventAt(target);
    if (actor && actor != state.getPlayer()) {
        if (actor->talkFunc) {
            // has talk function, is friendly
            state.vm->run(actor->talkFunc);
        } else if (state.swordLevel <= 0) {
            state.addMessage("You don't have a sword!");
        } else {
            int roll = -10000;
            int modifier = getAccuracyModifier(state.getPlayer(), actor, true);
            roll = state.coreRNG.roll(1,10);
            if (state.showDieRolls) {
                std::stringstream msg;
                msg << "[to hit: 1d10+" << modifier << "=" << (roll+modifier) << " > 5]";
                state.addMessage(msg.str());
            }
            if (roll + modifier <= 5) {
                state.addMessage("You miss " + actor->getName() + ".");
            } else {
                roll = state.coreRNG.roll(state.swordLevel, 4);
                if (state.showDieRolls) {
                    std::stringstream msg2;
                    msg2 << "[damage: " << state.swordLevel << 'd' << 4 << '=' << roll << ']';
                    state.addMessage(msg2.str());
                }
                actor->takeDamage(roll);
                state.addMessage("You do " + std::to_string(roll) + " damage to " + actor->getName() + ". ");
                if (actor->curHealth <= 0) {
                    state.appendMessage(upperFirst(actor->getName()) + " is defeated! ");
                }
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
            if (!gfx_confirm(state, "Confirm", "Are you sure you want to quit?")) {
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
            case Command::Run: {
                Dir d = cmd.direction;
                if (d == Dir::None) {
                    d = gfx_GetDirection(state, "Run");
                    if (d == Dir::None) break;
                }
                state.runDirection = d;
                break; }

            case Command::Interact: {
                Dir d = cmd.direction;
                if (d == Dir::None) {
                    d = gfx_GetDirection(state, "Activate", true);
                    if (d == Dir::None) break;
                }
                Point target = state.getPlayer()->position.shift(d, 1);
                if (!tryInteract(state, target)) {
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

            case Command::CharacterInfo:
                doCharInfo(state, charStats);
                break;
            case Command::Inventory:
                doCharInfo(state, charInventory);
                break;
            case Command::AbilityList:
                doCharInfo(state, charAbilities);
                break;

            case Command::Examine:
                if (gfx_SelectTile(state, "Looking at").x() >= 0) {
                    state.removeMessage();
                }
                break;


            case Command::NextSubweapon: {
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
                    default:
                        state.addMessage("The " + state.subweapons[state.currentSubweapon].name + " is not implemented.");
                }
                break; }

            case Command::Debug_Restore:
                state.getPlayer()->curHealth = state.getPlayer()->typeInfo->maxHealth;
                state.getPlayer()->curEnergy = state.getPlayer()->typeInfo->maxEnergy;
                state.bombCount = state.bombCapacity;
                state.arrowCount = state.arrowCapacity;
                break;
            case Command::Debug_ShowChecks:
                state.showDieRolls = !state.showDieRolls;
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
                    state.addMessage("Wrote map to disk as binary.map.");
                } else {
                    state.addMessage("Failed to write map to disk.");
                }
                break; }
            case Command::Debug_TestPathfinder: {
                Board *board = state.getBoard();
                board->resetMark();
                Point src = state.getPlayer()->position;
                Point dest = board->findRandomTile(state.coreRNG, tileFloor);
                std::stringstream line;
                line << "Finding path from " << src << " to " << dest << '.';
                state.addMessage(line.str());
                auto points = board->findPath(src, dest);
                if (points.empty()) {
                    state.addMessage("No path found.");
                } else {
                    state.addMessage("Path has " + std::to_string(points.size()) + " tiles.");
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
