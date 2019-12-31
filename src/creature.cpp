#include <ostream>
#include <sstream>
#include <vector>

#include "board.h"
#include "creature.h"
#include "game.h"
#include "random.h"
#include "gfx.h"
#include "point.h"
#include "textutil.h"

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
: level(1), xp(0), curHealth(0), curEnergy(0), isPlayer(false),
  ai_lastDir(Dir::None)
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

int Creature::takeDamage(int amount, DamageType type) {
    amount *= getResist(type);
    if (amount > curHealth) amount = curHealth;
    curHealth -= amount;
    return amount;
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

void Creature::useAbility(System &system, int abilityNumber, const Dir &d) {
    Board *board = system.getBoard();
    const MoveType &move = MoveType::get(abilityNumber);

    if (this->isPlayer) system.addMessage("You use " + move.name + ". ");
    else                system.addMessage(getName() + " uses " + move.name + ". ");

    // get the point of impact for the move
    Point target = position;
    switch(move.form) {
        case formSelf:
            // already have correct target; don't need to do anything
            break;
        case formBullet:
        case formMelee:
            for (int i = 0; i < move.minRange; ++i) target = target.shift(d);
            for (int i = 0; i < move.maxRange - move.minRange; ++i) {
                if (board->actorAt(target) != nullptr) {
                    break;
                }
                target = target.shift(d);
            }
            break;
        case formLobbed:
            break;
        case formFourway:
            break;
    }

    // get the set of effected tiles
    std::vector<Point> effected;
    effected.push_back(target);
    switch(move.shape) {
        case shapeSquare:
            break;
        case shapeCircle:
            break;
        case shapeLong: {
            Point work = target;
            for (int i = 0; i < move.damageSize * 2; ++i) {
                work = work.shift(d);
                if (TileInfo::get(board->getTile(work)).block_travel) {
                    break;
                }
                effected.push_back(work);
            }
            break; }
        case shapeWide: {
            Point work = target;
            Dir a = rotateDirection(d);
            for (int i = 0; i < move.damageSize; ++i) {
                work = work.shift(a);
                if (TileInfo::get(board->getTile(work)).block_travel) {
                    break;
                }
                effected.push_back(work);
            }
            work = target;
            a = flipDirection(a);
            for (int i = 0; i < move.damageSize; ++i) {
                work = work.shift(a);
                if (TileInfo::get(board->getTile(work)).block_travel) {
                    break;
                }
                effected.push_back(work);
            }
            break; }
        case shapeCone:
            break;
    }

    // apply to actors in effected range
    for (const Point &p : effected) {
        Creature *who = board->actorAt(p);
        if (who) {
            if (who->talkFunc) {
                std::stringstream line;
                line << upperFirst(who->getName()) << " resists! ";
                system.appendMessage(line.str());
            } else {
                DamageType damageType = static_cast<DamageType>(move.type);
                int realDamage = who->takeDamage(move.damage, damageType);
                std::stringstream line;
                line << upperFirst(who->getName()) << " takes " << realDamage << ' ' << damageType << " damage. ";
                if (who->curHealth <= 0) {
                    line << upperFirst(who->getName()) << " is defeated! ";
                    board->removeActor(who);
                    delete who;
                }
                system.appendMessage(line.str());
            }
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

std::string statName(const Stat &stat) {
    switch(stat) {
        case Stat::Speed:       return "speed";
        case Stat::Health:      return "health";
        case Stat::Energy:      return "energy";
        case Stat::Accuracy:    return "accuracy";
        case Stat::Evasion:     return "evasion";
        case Stat::DR:          return "damage reduction";
        case Stat::DB:          return "damage bonus";
    }
    return "Stat" + std::to_string(static_cast<int>(stat));
}

std::ostream& operator<<(std::ostream &out, const Stat &stat) {
    return out;
}

std::string damageTypeName(const DamageType &type) {
    switch(type) {
        case DamageType::Physical:  return "physical";
        case DamageType::Fire:      return "fire";
        case DamageType::Cold:      return "cold";
        case DamageType::Electric:  return "electric";
        case DamageType::Divine:    return "divine";
        case DamageType::Infernal:  return "infernal";
        case DamageType::Void:      return "void";
        case DamageType::Toxic:     return "toxic";
    }
    return "DamageType" + std::to_string(static_cast<int>(type));
}

std::ostream& operator<<(std::ostream &out, const DamageType &stat) {
    out << damageTypeName(stat);
    return out;
}

std::string damageShapeName(const int &shape) {
    switch(shape) {
        case shapeSquare:           return "square";
        case shapeCircle:           return "circle";
        case shapeLong:             return "long";
        case shapeWide:             return "wide";
        case shapeCone:             return "cone";
    }
    return "DamageShape" + std::to_string(shape);
}

std::string damageFormName(const int &form) {
    switch(form) {
        case formSelf:              return "self";
        case formBullet:            return "bullet";
        case formMelee:             return "melee";
        case formLobbed:            return "lobbed";
        case formFourway:           return "fourway";
    }
    return "DamageForm" + std::to_string(form);
}

void Creature::ai(Board *board) {
    // dead things don't do AI
    if (curHealth <= 0) return;

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
