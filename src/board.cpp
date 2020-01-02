#include <cstdlib>
#include <cstring>

#include "board.h"
#include "creature.h"


std::vector<TileInfo> TileInfo::types;

const TileInfo TileInfo::BAD_TILE{-1, "bad tile"};

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

Board::Board(int width, int height, const std::string &name)
: mWidth(width), mHeight(height), name(name), dbgDisableFOV(false)
{
    tiles = new Tile[width * height];
    memset(tiles, 0, width * height * sizeof(Tile));
}
Board::~Board() {
    delete[] tiles;
    for (Creature *actor : creatures) {
        delete actor;
    }
}

Creature* Board::actorAt(const Point &where) {
    for (Creature *creature : creatures) {
        if (creature->position == where) {
            return creature;
        }
    }
    return nullptr;
}
void Board::addActor(Creature *creature, const Point &where) {
    creature->position = where;
    creatures.push_back(creature);
}

void Board::removeActor(Creature *creature) {
    for (auto iter = creatures.begin(); iter != creatures.end();) {
        if (*iter == creature) {
            iter = creatures.erase(iter);
        } else {
            ++iter;
        }
    }
}

void Board::removeActor(const Point &p) {
    for (auto iter = creatures.begin(); iter != creatures.end();) {
        if ((*iter)->position == p) {
            iter = creatures.erase(iter);
        } else {
            ++iter;
        }
    }
}

Creature* Board::getPlayer() {
    for (Creature *who : creatures) {
        if (who->aiType == aiPlayer) {
            return who;
        }
    }

    return nullptr;
}

void Board::clearTo(int tile) {
    for (int y = 0; y < height(); ++y) {
        for (int x = 0; x < width(); ++x) {
            tiles[x+y*width()].tile = tile;
            tiles[x+y*width()].fov = 0;
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

Point Board::findTile(int tile) const {
    for (int y = 0; y < height(); ++y) {
        for (int x = 0; x < width(); ++x) {
            Point here(x, y);
            if (getTile(here) == tile) return here;
        }
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
            if (TileInfo::get(getTile(here)).block_travel && (blockOn & blockSolid))  break;
            if (TileInfo::get(getTile(here)).block_los    && (blockOn & blockOpaque)) break;

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
            if (TileInfo::get(getTile(here)).block_travel && (blockOn & blockSolid))  break;
            if (TileInfo::get(getTile(here)).block_los    && (blockOn & blockOpaque)) break;

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

void Board::tick() {
    for (Creature *who : creatures) {
        who->ai(this);
    }

    calcFOV(getPlayer()->position);
}

void Board::dbgRevealAll() {
    for (int i = 0; i < mWidth * mHeight; ++i) {
        tiles[i].fov |= FOV_EVER_SEEN;
    }
}

void Board::dbgToggleFOV() {
    dbgDisableFOV = !dbgDisableFOV;
}
