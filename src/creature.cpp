#include <ostream>
#include "board.h"
#include "creature.h"
#include "game.h"
#include "random.h"

std::vector<CreatureType> CreatureType::types;
std::vector<MoveType> MoveType::types;

void MoveType::add(const MoveType &type) {
    types.push_back(type);
}

const MoveType& MoveType::get(int ident) {
    for (const MoveType &mt : types) {
        if (mt.ident == ident) return mt;
    }
    throw GameError("Tried to get nonexistant move type " + std::to_string(ident));
}

bool MoveType::valid(int ident) {
    for (const MoveType &mt : types) {
        if (mt.ident == ident) return true;
    }
    return false;
}

int MoveType::typeCount() {
    return types.size();
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
    moves.push_back(typeInfo->defaultMove);
}

std::string Creature::getName() const {
    if (name.empty()) return typeInfo->name;
    return name;
}

int Creature::getStat(Stat stat) const {
    int statNumber = static_cast<int>(stat);
    return typeInfo->stats[statNumber];
}

double Creature::getResist(DamageType stat) const {
    int statNumber = static_cast<int>(stat);
    return typeInfo->resistances[statNumber];
}

void Creature::reset() {
    curHealth = getStat(Stat::Health);
    curEnergy = getStat(Stat::Energy);
}

void Creature::takeDamage(int amount, DamageType type) {
    amount *= getResist(type);
    if (amount >= curHealth)    curHealth = 0;
    else                        curHealth -= amount;
}

bool Creature::isKOed() const {
    return curHealth <= 0;
}

void Creature::autolevel(int toLevel, Random &rng) {
    level = toLevel;
}

void Creature::learnMove(int moveId) {
    if (!MoveType::valid(moveId)) return;
    for (int oldId : moves) {
        if (oldId == moveId) return;
    }
    moves.push_back(moveId);
}

bool Creature::knowsMove(int moveId) {
    for (int oldId : moves) {
        if (oldId == moveId) return true;
    }
    return false;
}

void Creature::forgetMove(int moveId) {
    for (auto iter = moves.begin(); iter != moves.end(); ) {
        if (*iter == moveId) {
            iter = moves.erase(iter);
        } else {
            ++iter;
        }
    }
}

const char* getAbbrev(const Stat &stat) {
    switch (stat) {
        case Stat::Health:  return "HP";
        case Stat::Energy:  return "EN";
        case Stat::DR:      return "DR";
        case Stat::DB:      return "DB";
        case Stat::Accuracy:return "AC";
        case Stat::Evasion: return "EV";
        case Stat::Speed:   return "SP";
    }
    return "??";
}

const char* getAbbrev(const DamageType &stat) {
    switch(stat) {
        case DamageType::Physical:  return "P";
        case DamageType::Fire:      return "F";
        case DamageType::Cold:      return "C";
        case DamageType::Electric:  return "E";
        case DamageType::Divine:    return "D";
        case DamageType::Infernal:  return "I";
        case DamageType::Void:      return "V";
        case DamageType::Toxic:     return "T";
    }
    return "?";
}

std::ostream& operator<<(std::ostream &out, const Stat &stat) {
    switch(stat) {
        case Stat::Speed:       out << "speed"; break;
        case Stat::Health:      out << "health"; break;
        case Stat::Energy:      out << "energy"; break;
        case Stat::Accuracy:    out << "accuracy"; break;
        case Stat::Evasion:     out << "evasion"; break;
        case Stat::DR:          out << "damage reduction"; break;
        case Stat::DB:          out << "damage bonus"; break;
    }
    return out;
}

std::ostream& operator<<(std::ostream &out, const DamageType &stat) {
    switch(stat) {
        case DamageType::Physical:  out << "physical"; break;
        case DamageType::Fire:      out << "fire"; break;
        case DamageType::Cold:      out << "cold"; break;
        case DamageType::Electric:  out << "electric"; break;
        case DamageType::Divine:    out << "divine"; break;
        case DamageType::Infernal:  out << "infernal"; break;
        case DamageType::Void:      out << "void"; break;
        case DamageType::Toxic:     out << "toxic"; break;
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
