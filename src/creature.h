#ifndef BEAST_H
#define BEAST_H

#include <array>
#include <iosfwd>
#include <string>
#include <vector>
#include "point.h"


class Board;
struct System;
class Random;
struct SDL_Texture;

const int aiPlayer          = 99;
const int aiStill           = 0;
const int aiRandom          = 1;
const int aiPaceHorz        = 2;
const int aiPaceVert        = 3;
const int aiPaceBox         = 4;
const int aiAvoidPlayer     = 5;
const int aiFollowPlayer    = 6;

const int statCount = 7;
enum class Stat {
    Health, Energy, Accuracy, Evasion,
    DR,     DB,     Speed,
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

class CreatureType {
public:
    static void add(const CreatureType &type);
    static const CreatureType& get(int ident);
    static int typeCount();

    struct Morph {
        int type, arg, target;
    };

    int ident;
    std::string name;
    int artIndex;
    int type[3];
    int stats[statCount];
    Morph morphs[3];
    int movesetAddr;
    std::vector<MovesetRow> moveset;
private:
    static std::vector<CreatureType> types;
};

struct Creature {
    Creature(int type);

    std::string name;
    Point position;
    int typeIdent;
    int level, xp;
    int curHealth, curEnergy;

    int nextAction;
    bool isPlayer;

    const CreatureType *typeInfo;
    SDL_Texture *art;

    int aiType;
    int aiArg;
    int talkFunc;
    int talkArg;

    Dir ai_lastDir;


    void ai(Board *board);
    bool tryMove(Board *board, Dir direction);
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
