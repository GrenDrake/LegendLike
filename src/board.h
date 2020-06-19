#ifndef BOARD_H
#define BOARD_H

#include <stdexcept>
#include <string>
#include <vector>
#include "point.h"

class System;
class Actor;
struct Item;
class Random;
struct SDL_Texture;

struct TileInfo {
    int index;
    std::string name;
    std::string artFile;
    int group;
    int red;
    int green;
    int blue;
    int interactTo;
    int animLength;
    unsigned flags;

    SDL_Texture *art;
    std::vector<SDL_Texture*> frames;

    bool is(unsigned flag) const;
    static void add(const TileInfo &type);
    static const TileInfo& get(int ident);
    const static TileInfo BAD_TILE;
    static std::vector<TileInfo> types;
};

const int TF_SOLID          = 0x01;
const int TF_OPAQUE         = 0x02;
const int TF_ISDOOR         = 0x04;

const int tileFloor         = 0;
const int tileWall          = 1;
const int tileDown          = 2;
const int tileUp            = 3;
const int tileDoorClosed    = 4;
const int tileDoorOpen      = 5;
const int tileWindow        = 6;
const int tileInterior      = 7;
const int tileClosedGate    = 15;
const int tileOpenGate      = 16;
const int tileOutOfBounds   = 99;

const int interactGoDown    = -10;
const int interactGoUp      = -11;

const int eventTypeAuto     = 0;
const int eventTypeManual   = 1;

const int MF_ADD_UP         = 0x01;
const int MF_ADD_DOWN       = 0x02;

const int blockSolid        = 0x01;
const int blockOpaque       = 0x02;
const int blockTarget       = 0x04;
const int blockActor        = 0x08;

struct MapInfo {
    int index;
    int width;
    int height;
    int onBuild, onEnter, onReset;
    unsigned flags;
    int musicTrack;
    std::string name;

    static void add(const MapInfo &type);
    static const MapInfo& get(int ident);
    const static MapInfo BAD_MAP;
    static std::vector<MapInfo> types;
};

const int FOV_EVER_SEEN     = 0x01;
const int FOV_IN_VIEW       = 0x02;

class BuildError : std::runtime_error {
public:
    BuildError(const std::string &msg)
    : std::runtime_error(msg)
    { }
};

class Board {
public:
    struct Tile {
        int tile;
        unsigned fov;
        bool mark;
    };
    struct Event {
        Point pos;
        int funcAddr;
        int type;
    };

    Board(const MapInfo &mapInfo);
    ~Board();

    int width() const {
        return mWidth;
    }
    int height() const {
        return mHeight;
    }
    bool valid(const Point &where) const {
        return coord(where) >= 0;
    }
    const MapInfo& getInfo() const {
        return mapInfo;
    }

    Actor* actorAt(const Point &where);
    void addActor(Actor *actor, const Point &where);
    void removeActor(Actor *actor);
    void removeActor(const Point &p);
    void doDamage(System &state, Actor *to, int amount, int type, const std::string &source);
    void makeLoot(System &state, const Actor *from, const Point &where);
    Actor* getPlayer();

    Item* itemAt(const Point &where);
    void addItem(Item *item, const Point &where);
    void removeAndDeleteItem(Item *item);
    void removeAndDeleteItem(const Point &p);

    void clearTo(int tile);
    void setTile(const Point &where, int tile);
    int getTile(const Point &where) const;
    bool isSolid(const Point &p);
    bool isOpaque(const Point &p);
    const Tile& at(const Point &where) const;
    Tile& at(const Point &where);
    Point findTile(int tile) const;
    Point findRandomTile(Random &rng, int tile) const;

    void resetFOV();
    void calcFOV(const Point &origin);
    void setSeen(const Point &where);
    bool isKnown(const Point &where) const;
    bool isVisible(const Point &where) const;
    std::vector<Point> findPoints(const Point &from, const Point &to, int blockOn);
    std::vector<Point> findPath(const Point &from, const Point &to);
    bool canSee(const Point &from, const Point &to);

    void addEvent(const Point &where, int funcAddr, int type);
    const Event* eventAt(const Point &where) const;

    void tick(System &system);

    void dbgShiftMap(Dir d);
    void dbgRevealAll();
    void dbgToggleFOV();
    void resetMark();

    bool readFromFile(const std::string &filename);
    bool writeToFile(const std::string &filename) const;
private:
    int coord(const Point &p) const;

    const MapInfo &mapInfo;
    int mWidth, mHeight;
    Tile *tiles;
    std::vector<Actor*> actors;
    std::vector<Item*> items;
    std::vector<Event> events;
    bool dbgDisableFOV;
};

struct RandomFoeInfo {
    int count;
    std::vector<int> typeList;
};

void makeMapMaze(Board *board, Random &player, unsigned flags);
void mapRandomEnemies(Board *board, Random &rng, const RandomFoeInfo &info);

#endif
