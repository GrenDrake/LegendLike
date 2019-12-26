#ifndef BEAST_H
#define BEAST_H

#include <array>
#include <iosfwd>
#include <string>
#include <vector>
#include "point.h"


class Board;
class System;
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

const int shapeSquare       = 0;
const int shapeCircle       = 1;
const int shapeLong         = 2;
const int shapeWide         = 3;
const int shapeCone         = 4;

const int formSelf          = 0;
const int formBullet        = 1;
const int formMelee         = 2;
const int formLobbed        = 3;
const int formFourway       = 4;


const int damageTypeCount = 8;
enum class DamageType {
    Physical, Cold,   Fire,     Electric,
    Toxic,    Divine, Infernal, Void,
};

const int statCount = 7;
enum class Stat {
    Health, Energy, Accuracy, Evasion,
    DR,     DB,     Speed,
};

class MoveType {
public:
    static void add(const MoveType &type);
    static const MoveType& get(int ident);
    static bool valid(int ident);
    static int typeCount();

    std::string name;
    int ident;
    int accuracy;
    int speed;
    int cost;
    int type;
    int minRange;
    int maxRange;
    int damage;
    int damageSize;
    int damageShape;
    int damageType;
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

    int ident;
    std::string name;
    int artIndex;
    int stats[statCount];
    double resistances[damageTypeCount];
    int defaultMove;
private:
    static std::vector<CreatureType> types;
};

class Creature {
public:
    Creature(int type);

    std::string name;
    Point position;
    int typeIdent;
    int level, xp;
    int curHealth, curEnergy;
    std::vector<int> moves;

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
    double getResist(DamageType stat) const;
    void reset();
    void takeDamage(int amount, DamageType type);
    bool isKOed() const;
    void autolevel(int toLevel, Random &rng);
    void learnMove(int moveId);
    bool knowsMove(int moveId);
    void forgetMove(int moveId);
};

const char* getAbbrev(const Stat &stat);
const char* getAbbrev(const DamageType &stat);

std::string statName(const Stat &stat);
std::ostream& operator<<(std::ostream &out, const Stat &stat);
std::string damageTypeName(const DamageType &type);
std::ostream& operator<<(std::ostream &out, const DamageType &stat);
std::string damageFormName(const int &form);
std::string damageShapeName(const int &shape);

#endif
