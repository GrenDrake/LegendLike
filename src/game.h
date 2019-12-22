#ifndef GAME_H
#define GAME_H

#define GAME_NAME   "BeastDuel"
#define GAME_MAJOR  0
#define GAME_MINOR  2
#define GAME_PATCH  0

#include <stdexcept>
#include <string>
#include <vector>

const int screenWidth = 960;
const int screenHeight = 540;

struct System;
class VM;

class GameError : public std::runtime_error {
public:
    GameError(const std::string &msg)
    : std::runtime_error(msg)
    { }
};

const int tileWidth = 32;
const int tileHeight = 32;

std::string versionString();
void gameloop(System &renderState);
char* slurpFile(const std::string &filename);
bool loaddata(System &system);

#endif