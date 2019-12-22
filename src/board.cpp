#include <cstdlib>
#include <cstring>

#include "fov.h"
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

void apply(void *mapVoid, int x, int y, int dx, int dy, void *src) {
    Board *map = static_cast<Board*>(mapVoid);
    map->setSeen(Point(x, y));
	// if (((MAP *)map)->onMap(x, y))
	// 	((MAP *)map)->setSeen(x, y);
}
bool opaque(void *mapVoid, int x, int y) {
    Board *map = static_cast<Board*>(mapVoid);
    int t = map->getTile(Point(x, y));
    const TileInfo &info = TileInfo::get(t);
    return info.block_los;
	// return ((MAP *)map)->blockLOS(x, y);
}

void Board::calcFOV(const Point &origin) {
    resetFOV();
    fov_settings_type fov_settings;
    fov_settings_init(&fov_settings);
    fov_settings_set_opacity_test_function(&fov_settings, opaque);
    fov_settings_set_apply_lighting_function(&fov_settings, apply);
    fov_circle(&fov_settings, this, NULL, origin.x(), origin.y(), 100);
    setSeen(origin);
    fov_settings_free(&fov_settings);
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
