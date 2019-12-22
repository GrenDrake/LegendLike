#ifndef OBJECT_H
#define OBJECT_H

#include <string>
#include <vector>
#include "point.h"

const int aiPlayer          = 99;
const int aiStill           = 0;
const int aiRandom          = 1;
const int aiPaceHorz        = 2;
const int aiPaceVert        = 3;
const int aiPaceBox         = 4;
const int aiAvoidPlayer     = 5;
const int aiFollowPlayer    = 6;

class Board;

class MapActor {
public:
    MapActor(const std::string &name, int artIndex, int aiType, int aiArg, int talkFunc, int talkArg)
    : name(name), artIndex(artIndex), aiType(aiType), aiArg(aiArg),
      talkFunc(talkFunc), talkArg(talkArg), ai_lastDir(Dir::None)
    { }

    void ai(Board *board);
    bool tryMove(Board *board, Dir direction);

    std::string name;
    Point position;
    int artIndex;
    int aiType;
    int aiArg;
    int talkFunc;
    int talkArg;

    Dir ai_lastDir;
};

#endif
