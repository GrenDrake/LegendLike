#include <ostream>
#include "beast.h"
#include "beastparty.h"
#include "game.h"
#include "random.h"

std::vector<BeastType> BeastType::types;
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

void BeastType::add(const BeastType &type) {
    types.push_back(type);
}

const BeastType& BeastType::get(int ident) {
    for (const BeastType &type : types) {
        if (type.ident == ident) {
            return type;
        }
    }
    throw GameError("no such BeastType");
}

int BeastType::typeCount() {
    return types.size();
}


Beast::Beast(int type)
: level(1), xp(0), curHealth(0), curEnergy(0), move{0, 0, 0, 0}, inParty(false)
{
    typeIdent = type;
    typeInfo = &BeastType::get(type);
    level = 0;
}

std::string Beast::getName() const {
    if (name.empty()) return typeInfo->name;
    return name;
}

int Beast::getStat(Stat stat) const {
    int statNumber = static_cast<int>(stat);
    int l0 = typeInfo->stats[statNumber][0];
    int l100 = typeInfo->stats[statNumber][1];
    return interp(0, l0, 100, l100, level);
}

void Beast::reset() {
    curHealth = getStat(Stat::Health);
    curEnergy = getStat(Stat::Energy);
}

void Beast::takeDamage(int amount) {
    if (amount >= curHealth)    curHealth = 0;
    else                        curHealth -= amount;
}

bool Beast::isKOed() const {
    return curHealth <= 0;
}

void Beast::autolevel(int toLevel, Random &rng) {
    std::vector<MovesetRow>::size_type pos = 0;
    for (int lvl = level + 1; lvl <= toLevel; ++lvl) {
        while (pos < typeInfo->moveset.size() && typeInfo->moveset[pos].level < lvl) {
            ++pos;
        }
        while (pos < typeInfo->moveset.size() && typeInfo->moveset[pos].level == lvl) {
            int index = getEmptyMove();
            if (index < 0) {
                index = rng.next32() % MAX_MOVES;
            }
            move[index] = typeInfo->moveset[pos].moveId;
            ++pos;
        }

    }
    level = toLevel;
}

int Beast::getEmptyMove() {
    for (int i = 0; i < MAX_MOVES; ++i) {
        if (move[i] == 0) return i;
    }
    return -1;
}

BeastParty::BeastParty() {
    for (int i = 0; i < MAX_SIZE; ++i) mParty[i] = nullptr;
}

bool BeastParty::add(Beast *beast) {
    if (beast) {
        for (int i = 0; i < MAX_SIZE; ++i) {
            if (mParty[i] == nullptr) {
                mParty[i] = beast;
                return true;
            }
        }
    }
    return false;
}

int BeastParty::size() const {
    int size = 0;
    for (Beast *b : mParty) {
        if (b) ++size;
    }
    return size;
}

Beast* BeastParty::at(unsigned slot) {
    if (slot >= mParty.size()) return nullptr;
    return mParty[slot];
}

bool BeastParty::remove(unsigned slot) {
    if (slot >= mParty.size()) return false;
    if (mParty[slot] == nullptr) return false;
    mParty[slot] = nullptr;
    return true;
}

Beast* BeastParty::getRandom(Random &rng) {
    if (!alive()) return nullptr;
    Beast *who = nullptr;
    do {
        int r = rng.between(0, 3);
        who = at(r);
    } while (!who || who->curHealth <= 0);
    return who;
}

bool BeastParty::alive() const {
    for (const Beast *b : mParty) {
        if (b && b->curHealth > 0) return true;
    }
    return false;
}

void BeastParty::clear() {
    for (unsigned i = 0; i < mParty.size(); ++i) {
        mParty[i] = nullptr;
    }
}

void BeastParty::deleteAll() {
    for (Beast *b : mParty) {
        if (b) delete b;
    }
    clear();
}

void BeastParty::reset() {
    for (Beast *b : mParty) {
        if (b) b->reset();
    }
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
