#include <cstdint>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <vector>

#include <SDL2/SDL.h>

#include "command.h"
#include "config.h"
#include "actor.h"
#include "board.h"
#include "game.h"
#include "point.h"
#include "vm.h"
#include "random.h"
#include "gamestate.h"
#include "textutil.h"

struct ProjectileInfo {
    std::string name;
    std::string filename;
    int damageDice, damageSides;
    bool directional;
};

std::string dirToAbbrev(Dir d);
Dir gfx_GetDirection(GameState &system, const std::string &prompt, bool allowHere = false);
Point gfx_SelectTile(GameState &system, const std::string &prompt);
void gfx_handleInput(GameState &state);
void mapedloop(GameState &state);

bool basicProjectileAttack(GameState &state, const ProjectileInfo &projectile, Dir d);
void doPlayerMove(GameState &state, Dir dir, bool forRun);
void doMeleeAttack(GameState &state, Actor *actor);
bool tryInteract(GameState &state, Dir d, const Point &target);


const int projArrow = 0;
const int projFireBolt = 1;
const int projIceBolt = 2;
std::vector<ProjectileInfo> projectiles{
    {   "arrow",        "arrow_",   0, 6, true  },
    {   "fire bolt",    "firebolt", 2, 6, false },
    {   "ice bolt",     "icebolt",  2, 6, false },
};

std::string dirToAbbrev(Dir d) {
    switch(d) {
        case Dir::North: return "n";
        case Dir::East: return "e";
        case Dir::South: return "s";
        case Dir::West: return "w";
        case Dir::Northeast: return "ne";
        case Dir::Northwest: return "nw";
        case Dir::Southeast: return "se";
        case Dir::Southwest: return "sw";
        default: return "";
    }
}

bool basicProjectileAttack(GameState &state, const ProjectileInfo &projectile, Dir d) {
    Board *board = state.getBoard();
    Actor *actor = nullptr;

    SDL_Texture *texProj = nullptr;
    if (projectile.directional) {
        texProj = state.getImage("effects/" + projectile.filename + dirToAbbrev(d) + ".png");
    } else {
        texProj = state.getImage("effects/" + projectile.filename + ".png");
    }

    std::vector<AnimFrame> frames;
    Point initial = state.getPlayer()->position;
    Point work(initial);
    while (1) {
        work = work.shift(d);
        if (!board->valid(work)) break;
        const TileInfo &tileInfo = TileInfo::get(board->getTile(work));
        if (tileInfo.flags & TF_SOLID) {
            frames.push_back(AnimFrame(animText, "Your " + projectile.name + " hits the " + tileInfo.name + "."));
            state.queueFrames(frames);
            return false;
        }

        actor = board->actorAt(work);
        if (actor) {
            bool isHit = false;
            if (actor->typeInfo->aiType == aiEnemy || actor->typeInfo->aiType == aiBreakable) {
                isHit = doAccuracyCheck(state, state.getPlayer(), actor, 0);
            }
            if (!isHit) {
                frames.push_back(AnimFrame(animText, "Your " + projectile.name + " misses " + actor->getName() + "."));
                actor = nullptr;
            }
            else break;
        }

        frames.push_back(AnimFrame(work, texProj));
    }

    if (actor) {
        int d = projectile.damageDice;
        if (d <= 0) d = state.subweaponLevel[SW_BOW];
        int damage = state.coreRNG.roll(d, projectile.damageSides);
        if (state.config->getBool("showrolls", false)) {
            std::stringstream msg;
            msg << "[damage: " << d << 'd' << projectile.damageSides << '=' << damage << ']';
            frames.push_back(AnimFrame(animRollText, msg.str()));
        }
        SDL_Texture *texSplat = state.getImage("effects/splat.png");
        frames.push_back(AnimFrame(work, texSplat));
        frames.push_back(AnimFrame(actor, damage, 0, "your " + projectile.name));

        state.queueFrames(frames);
        return true;
    }
    return false;
}

void doPlayerMove(GameState &state, Dir dir, bool forRun) {
    Point dest = state.getPlayer()->position.shift(dir);
    if (!state.getBoard()->valid(dest)) {
        if (!forRun) state.screenTransition(dir);
        else            state.runDirection = Dir::None;
    } else if (state.getPlayer()->tryMove(state.getBoard(), dir)) {
        Item *item = state.getBoard()->itemAt(dest);
        if (item) {
            state.grantItem(item->typeInfo->itemId);
            state.addMessage("Claimed: " + item->typeInfo->name + ".");
            state.getBoard()->removeAndDeleteItem(item);
        }
        const Board::Event *e = state.getBoard()->eventAt(dest);
        if (e && e->type == eventTypeAuto) {
            state.vm->run(e->funcAddr);
        }
        state.requestTick();
    } else if (forRun) {
        state.runDirection = Dir::None;
    } else {
        tryInteract(state, dir, dest);
    }
}

Dir gfx_GetDirection(GameState &system, const std::string &prompt, bool allowHere) {
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

Point gfx_SelectTile(GameState &system, const std::string &prompt) {
    system.cursor = system.getPlayer()->position;
    system.addMessage("Where?");
    while (1) {
        std::stringstream line;
        line << prompt << "; where? ";
        line << system.cursor << ' ';
        Actor *actor = system.getBoard()->actorAt(system.cursor);
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

void doMeleeAttack(GameState &state, Actor *actor) {
    bool isHit = doAccuracyCheck(state, state.getPlayer(), actor, 2);
    if (!isHit) {
        state.addMessage("You miss " + actor->getName() + ".");
    } else {
        int roll = 1;
        if (state.swordLevel > 0) {
            roll = state.coreRNG.roll(state.swordLevel, 4);
            if (state.config->getBool("showrolls", false)) {
                std::stringstream msg2;
                msg2 << "[damage: " << state.swordLevel << 'd' << 4 << '=' << roll << ']';
                state.addInfo(msg2.str());
            }
        }
        state.getBoard()->doDamage(state, actor, roll, 0, "your attack");
        SDL_Texture *texSplat = state.getImage("effects/splat.png");
        state.queueFrame(AnimFrame(actor->position, texSplat));
    }
}

bool tryInteract(GameState &state, Dir d, const Point &target) {
    Actor *actor = state.getBoard()->actorAt(target);
    const Board::Event *event = state.getBoard()->eventAt(target);
    if (actor && actor != state.getPlayer()) {
        if (actor->typeInfo->aiType == aiPushable || actor->typeInfo->aiType == aiBomb) {
            actor->tryMove(state.getBoard(), d);
        } else if (actor->typeInfo->aiType == aiEnemy) {
            if (state.swordLevel > 0) {
                doMeleeAttack(state, actor);
            } else {
                state.addMessage("You don't have a sword!");
            }
        } else if (actor->typeInfo->aiType == aiBreakable) {
            doMeleeAttack(state, actor);
        } else {
            if (actor->talkFunc) state.vm->run(actor->talkFunc);
            else                 state.addMessage(upperFirst(actor->getName()) + " has nothing to say.");
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

void gameloop(GameState &state) {
    state.returnToMenu = false;
    state.getBoard()->calcFOV(state.getPlayer()->position);

    state.runDirection = Dir::None;
    state.lastticks = SDL_GetTicks();

    while (!state.wantsToQuit && !state.returnToMenu) {
        if (state.runDirection != Dir::None) {
            const Point initalTilePos = state.getPlayer()->position.shift(state.runDirection, 1);
            const TileInfo &initialTile = TileInfo::get(state.getBoard()->getTile(initalTilePos));
            do {
                doPlayerMove(state, state.runDirection, true);
                if (state.runDirection != Dir::None) {
                    const TileInfo &thisTile = TileInfo::get(state.getBoard()->getTile(state.getPlayer()->position));
                    if (initialTile.group != thisTile.group) {
                        state.runDirection = Dir::None;
                    }
                }
                if (state.hasTick()) {
                    state.tick();
                    repaint(state);
                    if (passCommand(state)) state.runDirection = Dir::None;
                }
            } while (state.runDirection != Dir::None && !state.wantsToQuit);
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
            } else {
                state.gameInProgress = false;
            }
        }
    }

}


void gfx_handleInput(GameState &state) {
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
                doPlayerMove(state, cmd.direction, false);
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
                        basicProjectileAttack(state, projectiles[projArrow], d);
                        break;
                    case SW_FIREROD:
                        if (state.getPlayer()->curEnergy < 3) {
                            state.addMessage("Out of energy!");
                            break;
                        }
                        state.getPlayer()->curEnergy -= 3;
                        state.requestTick();
                        basicProjectileAttack(state, projectiles[projFireBolt], d);
                        break;
                    case SW_ICEROD:
                        if (state.getPlayer()->curEnergy < 3) {
                            state.addMessage("Out of energy!");
                            break;
                        }
                        state.getPlayer()->curEnergy -= 3;
                        state.requestTick();
                        basicProjectileAttack(state, projectiles[projIceBolt], d);
                        break;
                    case SW_PICKAXE: {
                        Actor *actor = state.getBoard()->actorAt(state.getPlayer()->position.shift(d));
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
                    case SW_BOMB: {
                        if (state.bombCount <= 0) {
                            state.addMessage("You don't have any bombs.");
                            break;
                        }
                        Point dest = state.getPlayer()->position.shift(d);
                        const TileInfo &tileInfo = TileInfo::get(state.getBoard()->getTile(dest));
                        Actor *existing = state.getBoard()->actorAt(dest);
                        if (tileInfo.flags & TF_SOLID || existing) {
                            state.addMessage("There's no space.");
                        } else {
                            --state.bombCount;
                            if (state.bombCount <= 0) state.subweaponLevel[SW_BOMB] = 0;
                            Actor *bomb = new Actor(1);
                            bomb->reset();
                            bomb->ai_pathNext = 5;
                            state.getBoard()->addActor(bomb, dest);
                        }
                        break; }
                    default:
                        state.addError("The " + state.subweapons[state.currentSubweapon].name + " is not implemented.");
                }
                break; }

            case Command::Debug_MapEditMode:
                state.getBoard()->dbgRevealAll();
                state.getBoard()->dbgSetFOV(true);
                state.addError("Entering map editing mode");
                state.mapEditTile = 0;
                mapedloop(state);
                state.mapEditTile = -1;
                state.addError("Exiting map editing mode");
                break;
            case Command::Debug_WarpMap: {
                std::string mapIdStr;
                if (gfx_EditText(state, "Map Number", mapIdStr, 10)) {
                    int mapId = strToInt(mapIdStr);
                    if (mapId >= 0) {
                        Point position = state.getPlayer()->position;
                        state.warpTo(mapId, position.x(), position.y());
                    }
                }
                break; }
            case Command::Debug_Teleport: {
                std::string xPos = std::to_string(state.getPlayer()->position.x());
                std::string yPos = std::to_string(state.getPlayer()->position.y());
                if (!gfx_EditText(state, "X Coord", xPos, 10)) break;
                if (!gfx_EditText(state, "Y Coord", yPos, 10)) break;
                int x = strToInt(xPos);
                int y = strToInt(yPos);
                if (x < 0) x = 0;
                if (y < 0) y = 0;
                if (x > state.getBoard()->width())  x = state.getBoard()->width() - 1;
                if (y > state.getBoard()->height()) y = state.getBoard()->height() - 1;
                state.warpTo(-1, x, y);
                break; }
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
