#ifndef BOARD_H
#define BOARD_H

#include <stdexcept>
#include <string>
#include <vector>
#include "point.h"

class Creature;
class Random;

struct TileInfo {
    int index;
    std::string name;
    int artIndex;
    int red;
    int green;
    int blue;
    bool block_travel;
    bool block_los;
    int interactTo;

    static void add(const TileInfo &type);
    static const TileInfo& get(int ident);
    const static TileInfo BAD_TILE;
    static std::vector<TileInfo> types;
};

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

struct MapInfo {
    int index;
    int width;
    int height;
    int buildFunction, enterFunction;
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
    };
    struct Event {
        Point pos;
        int funcAddr;
        int type;
    };

    Board(int width, int height, const std::string &name);
    ~Board();

    int width() const {
        return mWidth;
    }
    int height() const {
        return mHeight;
    }
    const std::string &getName() const {
        return name;
    }
    bool valid(const Point &where) const {
        return coord(where) >= 0;
    }

    Creature* actorAt(const Point &where);
    void addActor(Creature *creature, const Point &where);
    void removeActor(Creature *creature);
    void removeActor(const Point &p);
    Creature* getPlayer();

    void clearTo(int tile);
    void setTile(const Point &where, int tile);
    int getTile(const Point &where) const;
    Point findTile(int tile) const;

    void resetFOV();
    void calcFOV(const Point &origin);
    void setSeen(const Point &where);
    bool isKnown(const Point &where) const;
    bool isVisible(const Point &where) const;

    void addEvent(const Point &where, int funcAddr, int type);
    const Event* eventAt(const Point &where) const;

    void tick();

    void dbgRevealAll();
    void dbgToggleFOV();
private:
    int coord(const Point &p) const;

    int mWidth, mHeight;
    Tile *tiles;
    std::vector<Creature*> creatures;
    std::vector<Event> events;
    std::string name;
    bool dbgDisableFOV;
};

struct RandomFoeRow {
    int art;
    int ai;
    std::string name;
    int fightInfo;
};

struct RandomFoeInfo {
    int count;
    std::vector<RandomFoeRow> rows;
};

void makeMapMaze(Board *board, Random &player, unsigned flags);
void mapRandomEnemies(Board *board, Random &rng, const RandomFoeInfo &info);

#endif
