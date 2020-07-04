#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <map>
#include <sstream>
#include <vector>
#include <physfs.h>

#include "board.h"
#include "actor.h"
#include "point.h"
#include "random.h"
#include "gamestate.h"
#include "logger.h"
#include "vm.h"
#include "textutil.h"


std::vector<TileInfo> TileInfo::types;

const TileInfo TileInfo::BAD_TILE{-1, "bad tile"};
Board::Tile outOfBounds{ tileOutOfBounds };

bool TileInfo::is(unsigned flag) const {
    return flags & flag;
}

void TileInfo::add(const TileInfo &type) {
    types.push_back(type);
}

const TileInfo& TileInfo::get(int ident) {
    for (const TileInfo &info : types) {
        if (info.index == ident) return info;
    }
    return BAD_TILE;
}


std::vector<MapInfo> MapInfo::types;

const MapInfo MapInfo::BAD_MAP{-1};

void MapInfo::add(const MapInfo &type) {
    types.push_back(type);
}

const MapInfo& MapInfo::get(int ident) {
    for (const MapInfo &info : types) {
        if (info.index == ident) return info;
    }
    return BAD_MAP;
}

Board::Board(const MapInfo &mapInfo)
: mapInfo(mapInfo), mWidth(mapInfo.width), mHeight(mapInfo.height), dbgDisableFOV(false)
{
    tiles = new Tile[mWidth * mHeight];
    memset(tiles, 0, mWidth * mHeight * sizeof(Tile));
}
Board::~Board() {
    delete[] tiles;
    for (Actor *actor : actors) {
        delete actor;
    }
}

void Board::reset(GameState &state) {
    for (Actor *actor : actors) {
        delete actor;
    }
    actors.clear();
    for (Item *item : items) {
        delete item;
    }
    items.clear();
    if (mapInfo.onReset) {
        state.vm->run(mapInfo.onReset);
    }
}

Actor* Board::actorAt(const Point &where) {
    for (Actor *actor : actors) {
        if (actor->position == where) {
            return actor;
        }
    }
    return nullptr;
}
void Board::addActor(Actor *actor, const Point &where) {
    actor->position = where;
    actors.push_back(actor);
}

void Board::removeActor(Actor *actor) {
    for (auto iter = actors.begin(); iter != actors.end();) {
        if (*iter == actor) {
            iter = actors.erase(iter);
        } else {
            ++iter;
        }
    }
}

void Board::removeActor(const Point &p) {
    for (auto iter = actors.begin(); iter != actors.end();) {
        if ((*iter)->position == p) {
            iter = actors.erase(iter);
        } else {
            ++iter;
        }
    }
}

void Board::doDamage(GameState &state, Actor *to, int amount, int type, const std::string &source) {
    if (!to) return;
    Point pos = to->position;
    to->takeDamage(amount);
    state.addMessage(upperFirst(to->getName()) + " takes " + std::to_string(amount) + " damage from " + source + ". ");
    if (to->curHealth <= 0) {
        if (to->typeInfo->aiType == aiPlayer) {
            state.appendMessage("You die!");
        } else if (to->typeInfo->aiType == aiBreakable) {
            state.appendMessage(upperFirst(to->getName()) + " is destroyed! ");
            makeLoot(state, to, pos);
        } else {
            state.appendMessage(upperFirst(to->getName()) + " is defeated! ");
            makeLoot(state, to, pos);
        }
    }
}

void Board::makeLoot(GameState &state, const Actor *from, const Point &where) {
    if (!from) return;
    switch(from->typeInfo->lootType) {
        case lootNone:
            // no loot to generate
            break;
        case lootTable: {
            int tableNum = from->typeInfo->loot;
            int roll = state.coreRNG.roll(1,100);
            const LootTable &table = state.lootTables[tableNum];
            unsigned i = 0;
            for (; i < table.rows.size(); ++i) {
                if (table.rows[i].chance >= roll) break;
            }
            if (i == 0 || i > table.rows.size()) break; // rolled off table
            Item *item = new Item(&state.itemDefs[table.rows[i - 1].itemId]);
            addItem(item, where);
            break; }
        case lootLocation: {
            int locationNum = from->typeInfo->loot;
            ItemLocation &itemLoc = state.itemLocations[locationNum];
            if (itemLoc.used) break; // already dropped from this location!
            itemLoc.used = true;
            state.grantItem(itemLoc.itemId);
            break; }
        case lootFunction: {
            state.vm->run(from->typeInfo->loot);
            break;
        }
    }
}

Actor* Board::getPlayer() {
    for (Actor *who : actors) {
        if (who->typeInfo->aiType == aiPlayer) {
            return who;
        }
    }

    return nullptr;
}

Item* Board::itemAt(const Point &where) {
    for (Item *item : items) {
        if (item->position == where) {
            return item;
        }
    }
    return nullptr;
}

void Board::addItem(Item *item, const Point &where) {
    item->position = where;
    items.push_back(item);
}

void Board::removeAndDeleteItem(Item *item) {
    for (auto iter = items.begin(); iter != items.end();) {
        if (*iter == item) {
            iter = items.erase(iter);
        } else {
            ++iter;
        }
    }
    delete item;
}

void Board::removeAndDeleteItem(const Point &p) {
    for (auto iter = items.begin(); iter != items.end();) {
        if ((*iter)->position == p) {
            delete *iter;
            iter = items.erase(iter);
        } else {
            ++iter;
        }
    }
}


void Board::clearTo(int tile) {
    for (int y = 0; y < height(); ++y) {
        for (int x = 0; x < width(); ++x) {
            tiles[x+y*mWidth].tile = tile;
            tiles[x+y*mWidth].fov = 0;
            tiles[x+y*mWidth].mark = false;
        }
    }
}

void Board::setTile(const Point &where, int tile) {
    int t = coord(where);
    if (t < 0) return;
    tiles[t].tile = tile;
}
int Board::getTile(const Point &where) const {
    int t = coord(where);
    if (t < 0) return tileOutOfBounds;
    return tiles[t].tile;
}

bool Board::isSolid(const Point &p) {
    const TileInfo &info = TileInfo::get(getTile(p));
    return info.is(TF_SOLID);
}

bool Board::isOpaque(const Point &p) {
    const TileInfo &info = TileInfo::get(getTile(p));
    return info.is(TF_OPAQUE);
}

const Board::Tile& Board::at(const Point &where) const {
    int t = coord(where);
    if (t < 0) return outOfBounds;
    return tiles[t];
}

Board::Tile& Board::at(const Point &where) {
    int t = coord(where);
    if (t < 0) {
        outOfBounds.tile = tileOutOfBounds;
        return outOfBounds;
    }
    return tiles[t];
}

Point Board::findTile(int tile) const {
    for (int y = 0; y < height(); ++y) {
        for (int x = 0; x < width(); ++x) {
            Point here(x, y);
            if (getTile(here) == tile) return here;
        }
    }
    return Point(-1,-1);
}

Point Board::findRandomTile(Random &rng, int tile) const {
    const int MAX_ITERATIONS = 1000;
    for (int i = 0; i < MAX_ITERATIONS; ++i) {
        int x = rng.next32() % mWidth;
        int y = rng.next32() % mHeight;
        Point here(x, y);
        if (getTile(here) == tile) return here;
    }
    return Point(-1,-1);
}

void Board::resetFOV() {
    for (int y = 0; y < height(); ++y) {
        for (int x = 0; x < width(); ++x) {
            tiles[x+y*width()].fov &= FOV_EVER_SEEN;
        }
    }
}

void Board::setSeen(const Point &where) {
    int t = coord(where);
    if (t >= 0) {
        tiles[t].fov |= FOV_EVER_SEEN | FOV_IN_VIEW;
    }
}

bool Board::isKnown(const Point &where) const {
    if (dbgDisableFOV) return true;
    int t = coord(where);
    if (t >= 0) return tiles[t].fov & FOV_EVER_SEEN;
    return false;
}

bool Board::isVisible(const Point &where) const {
    if (dbgDisableFOV) return true;
    int t = coord(where);
    if (t >= 0) return tiles[t].fov & FOV_IN_VIEW;
    return false;
}

// Based on http://www.roguebasin.com/index.php?title=Bresenham%27s_Line_Algorithm
std::vector<Point> Board::findPoints(const Point &from, const Point &to, int blockOn) {
    std::vector<Point> points;

    if (from == to) {
        points.push_back(from);
        return points;
    }

    int x1 = from.x();
    int y1 = from.y();
    const int x2 = to.x();
    const int y2 = to.y();

    int delta_x = x2 - x1;
    const int ix = (delta_x > 0) - (delta_x < 0);
    delta_x = std::abs(delta_x) << 1;

    int delta_y = y2 - y1;
    const int iy = (delta_y > 0) - (delta_y < 0);
    delta_y = std::abs(delta_y) << 1;

    points.push_back(Point(x1, y1));

    if (delta_x >= delta_y) {
        // error may go below zero
        int error = delta_y - (delta_x >> 1);

        while (1) {
            if (x1 == x2 && (blockOn & blockTarget)) break;
            Point here(x1, y1);
            if (isSolid(here)  && (blockOn & blockSolid))  break;
            if (isOpaque(here) && (blockOn & blockOpaque)) break;
            if ((blockOn & blockActor) && here != from && actorAt(here)) break;

            if ((error > 0) || (!error && (ix > 0))) {
                error -= delta_x;
                y1 += iy;
            }

            error += delta_y;
            x1 += ix;
            points.push_back(Point(x1, y1));
        }
    } else {
        int error = delta_x - (delta_y >> 1);

        while (1) {
            if (y1 == y2 && (blockOn & blockTarget)) break;
            Point here(x1, y1);
            if (!valid(here)) break;
            if (isSolid(here)  && (blockOn & blockSolid))  break;
            if (isOpaque(here) && (blockOn & blockOpaque)) break;
            if ((blockOn & blockActor) && here != from && actorAt(here)) break;

            if ((error > 0) || (!error && (iy > 0))) {
                error -= delta_y;
                x1 += ix;
            }

            error += delta_x;
            y1 += iy;
            points.push_back(Point(x1, y1));
        }
    }

    return points;
}

struct PathPoint : public Point {
    PathPoint(int x, int y, int priority = 0)
    : Point(x, y), priority(priority)
    { }
    PathPoint(const Point &p, int priority = 0)
    : Point(p.x(), p.y()), priority(priority)
    { }

    int priority;
};
bool operator<(const PathPoint &a, const PathPoint &b) {
    return a.priority > b.priority;
}

int pathfinderHeuristic(const Point &a, const Point &b) {
    return std::abs(a.x() - b.x()) + std::abs(a.y() - b.y());
}

bool containsPoint(std::map<Point, int> container, Point value) {
    auto iter = container.find(value);
    if (iter == container.end())    return false;
    else                            return true;
}

std::vector<Point> Board::findPath(const Point &from, const Point &to) {
    std::priority_queue<PathPoint, std::vector<PathPoint>> frontier;
    frontier.push(from);
    std::map<Point, Point> cameFrom;
    cameFrom[from] = Point(-1,-1);
    std::map<Point, int> costSoFar;
    costSoFar[from] = 0;

    while (!frontier.empty()) {
        // pop the top value and put it on the done list
        PathPoint here = frontier.top();
        frontier.pop();

        // we're done, build the path and return
        if (here == to) {
            std::vector<Point> points;
            points.push_back(to);
            Point p = cameFrom[to];
            while (p != from) {
                points.push_back(p);
                p = cameFrom[p];
            }
            points.push_back(from);
            std::reverse(points.begin(), points.end());
            return points;

        // otherwise move on to the next point
        } else {
            const Dir initialDir = Dir::North;
            Dir d = initialDir;
            do {
                PathPoint shifted = here.shift(d);
                int travelCost = 1;
                if (TileInfo::get(getTile(shifted)).index == tileDoorClosed) ++travelCost;
                int newCost = costSoFar[here] + 1;
                if (!isSolid(shifted) || TileInfo::get(getTile(shifted)).index == tileDoorClosed) {
                    if (!containsPoint(costSoFar, shifted)
                            || newCost < costSoFar[shifted]) {
                        costSoFar[shifted] = newCost;
                        shifted.priority = newCost + pathfinderHeuristic(shifted, to);
                        frontier.push(shifted);
                        cameFrom[shifted] = here;
                    }
                }
                d = rotateDirection(d);
            } while (d != initialDir);
        }
    }

    // no path found, return empty list
    return std::vector<Point>();
}


bool Board::canSee(const Point &from, const Point &to) {
    std::vector<Point> points = findPoints(from, to, blockOpaque|blockTarget);
    if (points.back() == to)    return true;
    else                        return false;
}

int Board::coord(const Point &p) const {
    if (p.x() < 0 || p.y() < 0 || p.x() >= mWidth || p.y() >= mHeight) return -1;
    return p.x() + p.y() * mWidth;
}

void Board::addEvent(const Point &where, int funcAddr, int type) {
    events.push_back(Event{where, funcAddr, type});
}

const Board::Event* Board::eventAt(const Point &where) const {
    for (const Event &e : events) {
        if (e.pos == where) return &e;
    }
    return nullptr;
}

void Board::tick(GameState &system) {
    for (Actor *who : actors) {
        who->ai(system);
    }

    for (auto iter = actors.begin(); iter != actors.end(); ) {
        Actor *who = *iter;
        if (who->curHealth <= 0) {
            iter = actors.erase(iter);
            if (who->isPlayer) {
                who->reset();
                gfx_Alert(system, "You have died!", "");
                system.vm->runFunction("onDeath");
                system.runDirection = Dir::None;
            } else {
                delete who;
            }
        } else {
            ++iter;
        }
    }

    Actor *player = getPlayer();
    if (player) calcFOV(player->position);
}

void Board::dbgShiftMap(Dir d) {
    Tile *newTiles = new Tile[mWidth * mHeight];
    memset(newTiles, 0, mWidth * mHeight * sizeof(Tile));

    for (int y = 0; y < mHeight; ++y) {
        for (int x = 0; x < mWidth; ++x) {
            Point from(x, y);
            Point to = from.shift(d);
            int fromPos = coord(from);
            if (to.x() < 0 || to.y() < 0 || to.x() >= mWidth || to.y() >= mHeight) {
                ;
            } else {
                int toPos   = coord(to);
                newTiles[toPos] = tiles[fromPos];
            }
        }
    }
    delete[] tiles;
    tiles = newTiles;
}

void Board::dbgRevealAll() {
    for (int i = 0; i < mWidth * mHeight; ++i) {
        tiles[i].fov |= FOV_EVER_SEEN;
    }
}

void Board::dbgToggleFOV() {
    dbgDisableFOV = !dbgDisableFOV;
}

void Board::resetMark() {
    for (int y = 0; y < height(); ++y) {
        for (int x = 0; x < width(); ++x) {
            at(Point(x,y)).mark = false;
        }
    }
}

bool Board::readFromFile(const std::string &filename) {
    PHYSFS_file *inf = PHYSFS_openRead(filename.c_str());
    if (!inf) return false;

    PHYSFS_uint16 fileWidth, fileHeight;
    PHYSFS_readULE16(inf, &fileWidth);
    PHYSFS_readULE16(inf, &fileHeight);
    if (fileWidth != mWidth || fileHeight != mHeight) {
        Logger &log = Logger::getInstance();
        std::stringstream msg;
        msg << "Loaded map " << filename << " is unexpected size; file is ";
        msg << fileWidth << 'x' << fileHeight << ", but map expected ";
        msg << mWidth << 'x' << mHeight << '.';
        log.warn(msg.str());
    }

    PHYSFS_uint32 tile;
    for (int y = 0; y < fileHeight; ++y) {
        for (int x = 0; x < fileWidth; ++x) {
            PHYSFS_readULE32(inf, &tile);
            at(Point(x, y)).tile = tile;
        }
    }

    PHYSFS_close(inf);
    return true;
}

bool Board::writeToFile(const std::string &filename) const {
    PHYSFS_file *inf = PHYSFS_openWrite(filename.c_str());
    if (!inf) return false;

    PHYSFS_writeULE16(inf, mWidth);
    PHYSFS_writeULE16(inf, mHeight);

    for (int y = 0; y < mHeight; ++y) {
        for (int x = 0; x < mWidth; ++x) {
            PHYSFS_writeULE32(inf, at(Point(x, y)).tile);
        }
    }

    PHYSFS_close(inf);
    return true;
}
