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
  ai_lastDir(Dir::None), ai_lastTarget(-1, -1)
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
    if (amount > 0) {
        if (amount > curHealth) amount = curHealth;
    } else {
        int maxGain = getStat(Stat::Health) - curHealth;
        if (-amount > maxGain) amount = -maxGain;
    }
    curHealth -= amount;
    if (curHealth <= 0) position = Point(-1, -1);
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

void Creature::useAbility(System &system, int abilityNumber, const Point &aimedAt) {
    Board *board = system.getBoard();
    const MoveType &move = MoveType::get(abilityNumber);

    if (curEnergy < move.cost) {
        if (this->isPlayer) system.addMessage("You use try to use " + move.name + ", but lack the energy.");
        else                system.addMessage(getName() + " tries to use " + move.name + ", but lacks the energy.");
        return;
    }
    curEnergy -= move.cost;

    if (this->isPlayer) system.addMessage("You use " + move.name + ". ");
    else                system.addMessage(getName() + " uses " + move.name + ". ");

    // get the point of impact for the move
    Point target = position;
    Dir attackDir = Dir::None;
    switch(move.form) {
        case formSelf:
            // already have correct target; don't need to do anything
            break;
        case formBullet: {
            std::vector<Point> points = board->findPoints(position, aimedAt, blockSolid|blockActor);
            if (points.size() >= 2) {
                attackDir = points.front().directionTo(points.back());
            } else {
                attackDir = randomDirection(system.coreRNG);
            }
            target = points.back();
            if (board->isSolid(target)) {
                points.pop_back();
                target = points.back();
            }
            if (!points.empty()) points.erase(points.begin());
            if (!points.empty()) {
                Animation anim{AnimType::Travel, move.damageIcon};
                for (const Point &p : points) anim.points.push_back(p);
                system.queueAnimation(anim);
            }
            break; }
        case formMelee:
        case formLobbed:
            attackDir = position.directionTo(aimedAt);
            target = aimedAt;
            break;
        case formFourway:
            break;
    }

    // get the set of effected tiles
    std::vector<Point> effected;
    effected.push_back(target);
    Animation anim{AnimType::All, move.damageIcon};
    anim.points.push_back(target);
    switch(move.shape) {
        case shapeSquare:
            break;
        case shapeCircle:
            break;
        case shapeLong: {
            Point work = target;
            anim.points.push_back(work);
            effected.push_back(work);

            for (int i = 0; i < move.damageSize * 2; ++i) {
                if (board->isSolid(work)) {
                    break;
                }
                work = work.shift(attackDir);
                anim.points.push_back(work);
                effected.push_back(work);
            }
            break; }
        case shapeWide: {
            Point work = target;
            Dir a = rotateDirection(attackDir);
            for (int i = 0; i < move.damageSize; ++i) {
                work = work.shift(a);
                if (board->isSolid(work)) {
                    break;
                }
                anim.points.push_back(work);
                effected.push_back(work);
            }
            work = target;
            a = flipDirection(a);
            for (int i = 0; i < move.damageSize; ++i) {
                work = work.shift(a);
                if (board->isSolid(work)) {
                    break;
                }
                anim.points.push_back(work);
                effected.push_back(work);
            }
            break; }
        case shapeCone:
            break;
    }
    system.queueAnimation(anim);

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
                line << upperFirst(who->getName());
                if (move.damage > 0) line << " takes " << realDamage << ' ' << damageType << " damage. ";
                else                line << " recovers " << -realDamage << " via " << damageType << ". ";
                system.appendMessage(line.str());
                if (who->curHealth <= 0) {
                    who->position = Point(-1, -1);
                    if (!who->isPlayer) {
                        system.appendMessage(upperFirst(who->getName()) + " is defeated! ");
                    }
                }
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

void Creature::ai(System &system) {
    if (system.coreRNG.next32() & 1) return;
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
                    // do attack
                    useAbility(system, typeInfo->defaultMove, playerPos);
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
