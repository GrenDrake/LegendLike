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

const int playerTypeId      = 0;

const int aiPlayer          = 99;
const int aiStill           = 0;
const int aiRandom          = 1;
const int aiPaceHorz        = 2;
const int aiPaceVert        = 3;
const int aiPaceBox         = 4;
const int aiAvoidPlayer     = 5;
const int aiFollowPlayer    = 6;
const int aiEnemy           = 7;
const int aiPushable        = 8;


class CreatureType {
public:
    static void add(const CreatureType &type);
    static const CreatureType& get(int ident);
    static int typeCount();

    int ident;
    std::string name;
    std::string artFile;
    int maxHealth, maxEnergy;
    int damage, accuracy, evasion, moveRate;
    int aiType;

    SDL_Texture *art;
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

    int nextAction;
    bool isPlayer;

    const CreatureType *typeInfo;
    SDL_Texture *art;

    int talkFunc;
    int talkArg;

    Dir ai_lastDir;
    Point ai_lastTarget;
    std::vector<Point> ai_lastPath;
    int ai_pathNext;
    int ai_moveCount;


    void ai(System &system);
    bool tryMove(Board *board, Dir direction);
    std::string getName() const;
    void reset();
    int takeDamage(int amount);
    bool isKOed() const;
};


bool doAccuracyCheck(System &state, Creature *attacker, Creature *target, int modifier);

#endif
