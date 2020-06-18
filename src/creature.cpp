#include <ostream>
#include <sstream>
#include <vector>

#include "board.h"
#include "config.h"
#include "creature.h"
#include "game.h"
#include "random.h"
#include "gfx.h"
#include "point.h"
#include "textutil.h"

std::vector<CreatureType> CreatureType::types;


void CreatureType::add(const CreatureType &type) {
    types.push_back(type);
}

const CreatureType& CreatureType::get(int ident) {
    for (const CreatureType &type : types) {
        if (type.ident == ident) {
            return type;
        }
    }
    throw GameError("no such CreatureType " + std::to_string(ident));
}

int CreatureType::typeCount() {
    return types.size();
}


Creature::Creature(int type)
: level(1), xp(0), curHealth(0), curEnergy(0), isPlayer(false),
  aiType(aiStill), aiArg(0), talkFunc(0), talkArg(0),
  ai_lastDir(Dir::None), ai_lastTarget(-1, -1), ai_moveCount(0)
{
    typeIdent = type;
    typeInfo = &CreatureType::get(type);
}

std::string Creature::getName() const {
    if (name.empty()) return typeInfo->name;
    return name;
}

void Creature::reset() {
    curHealth = typeInfo->maxHealth;
    curEnergy = typeInfo->maxEnergy;
}

int Creature::takeDamage(int amount) {
    if (amount > 0) {
        if (amount > curHealth) amount = curHealth;
    } else {
        int maxGain = typeInfo->maxHealth - curHealth;
        if (-amount > maxGain) amount = -maxGain;
    }
    curHealth -= amount;
    if (curHealth <= 0) position = Point(-1, -1);
    return amount;
}

bool Creature::isKOed() const {
    return curHealth <= 0;
}

void Creature::ai(System &system) {
    // creatures have a 1 in moveRate chance of taking a turn
    if (typeInfo->moveRate > 1) {
        ++ai_moveCount;
        if (ai_moveCount < typeInfo->moveRate) return;
        ai_moveCount = 0;
    }

    // dead things don't do AI
    if (curHealth <= 0) return;
    Board *board = system.getBoard();

    const Dir dirs[4] = { Dir::West, Dir::North, Dir::East, Dir::South };

    switch(aiType) {
        case aiPlayer:
        case aiStill:
            return;
        case aiPaceHorz:
            if (ai_lastDir == Dir::None) ai_lastDir = Dir::East;
        case aiPaceVert:
            if (ai_lastDir == Dir::None) ai_lastDir = Dir::North;
            if (!tryMove(board, ai_lastDir)) {
                ai_lastDir = flipDirection(ai_lastDir);
                tryMove(board, ai_lastDir);
            }
            break;
        case aiRandom:
            ai_lastDir = dirs[rand() % 4];
            tryMove(board, ai_lastDir);
            break;
        case aiPaceBox:
            if (ai_lastDir == Dir::None) ai_lastDir = Dir::West;
            if (!tryMove(board, ai_lastDir)) {
                ai_lastDir = rotateDirection(ai_lastDir);
                tryMove(board, ai_lastDir);
            }
            break;
        case aiAvoidPlayer:
            ai_lastDir = position.directionTo(board->getPlayer()->position);
            tryMove(board, flipDirection(ai_lastDir));
            break;
        case aiFollowPlayer:
            ai_lastDir = position.directionTo(board->getPlayer()->position);
            tryMove(board, ai_lastDir);
            break;
        case aiEnemy: {
            Point playerPos = board->getPlayer()->position;
            bool canSeePlayer = board->canSee(position, playerPos);
            if (canSeePlayer) {
                ai_lastTarget = playerPos;
                if (position.distanceTo(playerPos) < 2) {
                    Creature *player = board->getPlayer();
                    // do attack
                    bool isHit = doAccuracyCheck(system, this, player, 2);
                    if (!isHit) {
                        system.addMessage(upperFirst(getName()) + " misses you.");
                    } else {
                        int damage = typeInfo->damage;
                        player->takeDamage(damage);
                        std::stringstream msg;
                        msg << upperFirst(getName()) << " attacks you for " << damage << " damage.";
                        system.addMessage(msg.str());
                        if (player->curHealth <= 0) system.appendMessage(" You die!");
                    }
                } else {
                    // move towards player
                    // std::cerr << this << " can see player\n";
                    ai_lastPath = board->findPath(position, playerPos);
                    if (ai_lastPath.size() < 2) {
                        // std::cerr << this << " no valid path to player\n";
                        break;
                    }
                    ai_pathNext = 2;
                    // std::cerr << this << " trying to move to " << ai_lastPath[1] << "\n";
                    ai_lastDir = position.directionTo(ai_lastPath[1]);
                    tryMove(board, ai_lastDir);
                }
            } else {
                if (ai_lastTarget.x() >= 0) {
                    // std::cerr << this << " lost sight of player\n";
                    if (position == ai_lastTarget || ai_pathNext >= static_cast<int>(ai_lastPath.size())) {
                        // std::cerr << this << " reach last known location; wandering\n";
                        ai_lastTarget = Point(-1,-1);
                        ai_lastDir = dirs[rand() % 4];
                    } else {
                        // std::cerr << this << " moving to last known location\n";
                        ai_lastDir = position.directionTo(ai_lastPath[ai_pathNext]);
                        ++ai_pathNext;
                    }
                } else {
                    // std::cerr << this << " player not visible; wandering\n";
                    ai_lastDir = dirs[rand() % 4];
                }
                if (!tryMove(board, ai_lastDir)) {
                    Point newPos = position.shift(ai_lastDir);
                    const TileInfo &info = TileInfo::get(board->getTile(newPos));
                    if (info.is(TF_ISDOOR)) {
                        board->setTile(newPos, info.interactTo);
                    }
                }
            }
            break; }
    }
}

bool Creature::tryMove(Board *board, Dir direction) {
    Point newPosition = position.shift(direction);

    if (!board->valid(newPosition)) return false;

    int tile = board->getTile(newPosition);
    if (TileInfo::get(tile).is(TF_SOLID)) {
        return false;
    }

    Creature *inWay = board->actorAt(newPosition);
    if (inWay != nullptr) {
        return false;
    }

    position = newPosition;
    return true;
}

bool doAccuracyCheck(System &system, Creature *attacker, Creature *target, int modifier) {
    int roll = -10000;

    roll = system.coreRNG.roll(1,10);
    if (system.config->getBool("showrolls", false)) {
        std::stringstream msg;
        msg << "[to hit: 1d10+" << modifier << "=" << (roll+modifier) << " > 5]";
        system.addMessage(msg.str());
    }

    if (roll + modifier > 5) return true;
    return false;
}
