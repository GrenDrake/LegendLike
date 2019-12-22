#ifndef BEAST_H
#define BEAST_H

#include <array>
#include <iosfwd>
#include <string>
#include <vector>

struct System;
class Random;
struct SDL_Texture;

const int statCount = 8;
enum class Stat {
    Attack, Defense, Mind, Will,
    Speed, Health, Energy, XP
};

namespace TypeInfo {
    void setTypeCount(int count);
    void setName(int type, const std::string &name);
    const std::string& getName(int type);
    void setEffectiveness(int atk, int def, int effect);
    double getTypeEffectiveness(int atk, int def);
}

class MoveType {
public:
    static void add(const MoveType &type);
    static const MoveType& get(int ident);
    static int typeCount();

    std::string name;
    int ident;
    int power;
    int accuracy;
    int cost;
    int type;
    int priority;
    int attackStat;
    int defendStat;
    int statusEffect;
    int statusChance;
    int statToShift;
    int shiftAmount;
    int special;
    unsigned flags;
private:
    static std::vector<MoveType> types;
};

struct MovesetRow {
    int level;
    int moveId;
};

class BeastType {
public:
    static void add(const BeastType &type);
    static const BeastType& get(int ident);
    static int typeCount();

    struct Morph {
        int type, arg, target;
    };

    int ident;
    std::string name;
    int frontArt, backArt;
    int type[3];
    int stats[statCount][2];
    Morph morphs[3];
    int movesetAddr;
    std::vector<MovesetRow> moveset;
private:
    static std::vector<BeastType> types;
};

struct Beast {
    static const int MAX_MOVES = 4;

    Beast(int type);
    std::string name;
    int typeIdent;
    int level, xp;
    int curHealth, curEnergy;
    int move[MAX_MOVES];
    int nextAction;

    bool inParty;

    const BeastType *typeInfo;
    SDL_Texture *art;

    std::string getName() const;
    int getStat(Stat stat) const;
    void reset();
    void takeDamage(int amount);
    bool isKOed() const;
    void autolevel(int toLevel, Random &rng);
    int getEmptyMove();
};

std::ostream& operator<<(std::ostream &out, const Stat &stat);

#endif
