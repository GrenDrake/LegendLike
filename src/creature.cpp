#include <ostream>
#include "board.h"
#include "creature.h"
#include "game.h"
#include "random.h"

std::vector<CreatureType> CreatureType::types;
std::vector<MoveType> MoveType::types;

namespace TypeInfo {
    static std::vector<double> typeEffects;
    static std::vector<std::string> typeNames;
    static int typeCount = 0;

    void setTypeCount(int count) {
        typeCount = count;
        typeEffects.resize(count * count, 100.0);
        typeNames.resize(count);
    }

    void setName(int type, const std::string &name) {
        typeNames[type] = name;
    }

    const std::string& getName(int type) {
        return typeNames[type];
    }

    void setEffectiveness(int atk, int def, int effect) {
        typeEffects[atk + def * typeCount] = effect / 100.0;
    }

    double getTypeEffectiveness(int atk, int def) {
        return typeEffects[atk + def * typeCount];
    }

}

void MoveType::add(const MoveType &type) {
    types.push_back(type);
}

const MoveType& MoveType::get(int ident) {
    for (const MoveType &mt : types) {
        if (mt.ident == ident) return mt;
    }
    throw GameError("Tried to get nonexistant move type " + std::to_string(ident));
}

int MoveType::typeCount() {
    return types.size();
}


/*
    X    Y
1   0    L0
2   100  L100
3   lev  ??
*/
// static int interp(int L0, int L100, int level) {
//     return (level * (L100 - L0)) / 100;

//     int l0 = typeInfo->stats[statNumber][0];;
// }

static double interp(double x1, double y1, double x2, double y2, double target) {
    return ((target - x1) * (y2 - y1)) / (x2 - x1) + y1;
}

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
: level(1), xp(0), curHealth(0), curEnergy(0), isPlayer(false)
{
    typeIdent = type;
    typeInfo = &CreatureType::get(type);
}

std::string Creature::getName() const {
    if (name.empty()) return typeInfo->name;
    return name;
}

int Creature::getStat(Stat stat) const {
    int statNumber = static_cast<int>(stat);
    int l0 = typeInfo->stats[statNumber][0];
    int l100 = typeInfo->stats[statNumber][1];
    return interp(0, l0, 100, l100, level);
}

void Creature::reset() {
    curHealth = getStat(Stat::Health);
    curEnergy = getStat(Stat::Energy);
}

void Creature::takeDamage(int amount) {
    if (amount >= curHealth)    curHealth = 0;
    else                        curHealth -= amount;
}

bool Creature::isKOed() const {
    return curHealth <= 0;
}

void Creature::autolevel(int toLevel, Random &rng) {
    level = toLevel;
}


std::ostream& operator<<(std::ostream &out, const Stat &stat) {
    switch(stat) {
        case Stat::Attack:  out << "attack"; break;
        case Stat::Defense: out << "defense"; break;
        case Stat::Mind:    out << "mind"; break;
        case Stat::Will:    out << "will"; break;
        case Stat::Speed:   out << "speed"; break;
        case Stat::Health:  out << "health"; break;
        case Stat::Energy:  out << "energy"; break;
        case Stat::XP:      out << "experience requirement"; break;
    }
    return out;
}

void Creature::ai(Board *board) {
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
    }


}

bool Creature::tryMove(Board *board, Dir direction) {
    Point newPosition = position.shift(direction);

    if (!board->valid(newPosition)) return false;

    int tile = board->getTile(newPosition);
    if (TileInfo::get(tile).block_travel) {
        return false;
    }

    Creature *inWay = board->actorAt(newPosition);
    if (inWay != nullptr) {
        return false;
    }

    position = newPosition;
    return true;
}
