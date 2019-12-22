#include <cstdlib>
#include <deque>
#include <vector>

#include "mapactor.h"
#include "board.h"
#include "point.h"

Dir openDirection(Board *board, const Point &point);
void addNewDoor(Board *board, const Point &here, Random &rng);
void doTrimDeadEnd(Board *board, Point where);
void trimDeadEnds(Board *board, int trimPercent, Random &rng);


bool isClear(Board *board, int x1, int y1, int x2, int y2, int clearTile = tileWall) {
    for (int x = x1; x <= x2; ++x) {
        for (int y = y1; y <= y2; ++y) {
            if (board->getTile(Point(x, y)) != clearTile) {
                return false;
            }
        }
    }
    return true;
}

void setTiles(Board *board, const Point &topLeft, int width, int height, int newTile) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            board->setTile(Point(topLeft.x() + x,
                                topLeft.y() + y), newTile);
        }
    }
}

bool validDoorLocation(Board *board, const Point &where) {
    if (where.x() == 0 || where.y() == 0) {
        return false;
    }

    if (board->getTile(where.shift(Dir::East)) != board->getTile(where.shift(Dir::West))) {
        return false;
    }
    if (board->getTile(where.shift(Dir::North)) != board->getTile(where.shift(Dir::South))) {
        return false;
    }

    if (board->getTile(where.shift(Dir::North)) == board->getTile(where.shift(Dir::West))) {
        return false;
    }

    int northTile = board->getTile(where.shift(Dir::North));
    int westTile = board->getTile(where.shift(Dir::West));
    if (northTile != tileFloor && northTile != tileWall) {
        return false;
    }
    if (westTile != tileFloor && westTile != tileWall) {
        return false;
    }

    return true;
}


/* ************************************************************************** *
 * CORE MAP GENERATION                                                        *
 * ************************************************************************** */
struct Room {
    Room(int x, int y, int w, int h, bool forceHollow = false)
    : x(x), y(y), w(w), h(h), forceHollow(forceHollow)
    { }
    int x, y, w, h;
    bool forceHollow;
};

void makeMapMaze(Board *board, Random &rng, unsigned flags) {
    if (!board) return;
    Point start(1 + 2 * (rand() % (board->width() / 2 - 2)),
                1 + 2 * (rand() % (board->height() / 2 - 2)));
    std::vector<Room> rooms;
    std::deque<Point> list;
    list.push_back(start);
    board->clearTo(tileWall);
    board->setTile(start, tileFloor);

    // add central chamber
    const int centralWidth = 5;
    const int centralHeight = 5;
    Point centralRoomtopLeft(board->width() / 2 - centralWidth / 2,
                             board->height() / 2 - centralHeight / 2);
    rooms.push_back(Room(centralRoomtopLeft.x(), centralRoomtopLeft.y(),
                         centralWidth, centralHeight, true));
    setTiles(board, centralRoomtopLeft, centralWidth, centralHeight, tileInterior);

    // add some room templates
    for (int i = 0; i < 60; ++i) {
        Point topleft(1 + 2 * (rand() % (board->width() / 2 - 2)),
                      1 + 2 * (rand() % (board->height() / 2 - 2)));
        int width = 1 + rand() % 6;
        int height = 1 + rand() % 6;
        if (width % 2 == 0) ++width;
        if (height % 2 == 0) ++height;
        if (topleft.x() + width  > board->width()  - 1 ||
            topleft.y() + height > board->height() - 1) {
            continue;
        }

        if (!isClear(board, topleft.x(), topleft.y(), topleft.x() + width, topleft.y() + height)) {
            continue;
        }

        rooms.push_back(Room(topleft.x(), topleft.y(), width, height));

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                board->setTile(Point(topleft.x() + x, topleft.y() + y), tileInterior);
            }
        }
    }

    // build the maze
    while (!list.empty()) {
        int index = rand() % list.size();
        Point here = list[index];

        Point dest;
        Dir initialDir = randomDirection(rng);
        Dir dir = initialDir;
        bool success = false;
        do {
            dest = Point(here);
            dest = dest.shift(dir, 2);
            if (board->getTile(dest) == tileWall) {
                list.push_front(dest);
                board->setTile(dest, tileFloor);
                board->setTile(here.shift(dir, 1), tileFloor);
                success = true;
                break;
            }

            dir = rotateDirection(dir);
        } while (dir != initialDir);

        if (!success) {
            list.erase(list.begin()+index);
        }
    }

    // build the room interiors
    for (const Room &room : rooms) {
        bool makeSolid = true;
        if (room.w >= 3 && room.h >= 3 && rand() % 6 != 2) makeSolid = false;
        if (room.forceHollow) makeSolid = false;

        setTiles(board, Point(room.x, room.y), room.w, room.h, makeSolid ? tileWall : tileFloor);
        if (makeSolid) continue;

        const int maxIterations = 100;
        int iterations = 0;
        Point doorLoc;
        bool vert = rand() % 2 == 0;
        bool side = rand() % 2 == 0;
        do {
            if (vert) {
                doorLoc = Point(side ?
                                    room.x - 1 :
                                    room.x + room.w,
                                room.y + rand() % room.h);
            } else {
                doorLoc = Point(room.x + rand() % room.w,
                                side ?
                                    room.y - 1 :
                                    room.y + room.h);
            }
            ++iterations;
        } while (!validDoorLocation(board, doorLoc) && iterations <= maxIterations);

        if (validDoorLocation(board, doorLoc)) {
            board->setTile(doorLoc, tileDoorClosed);
        } else {
            if (!room.forceHollow)
                makeSolid = true;
        }

        if (makeSolid) {
            setTiles(board, Point(room.x, room.y), room.w, room.h, tileWall);
        }

    }

    trimDeadEnds(board, 100, rng);

    // add some random doors, windows, and secrets to the map
    for (int i = 0; i < 100; ++i) {
        Point here = Point(1 + (rand() % (board->width() - 2)),
                           1 + (rand() % (board->height() - 2)));

        if (!validDoorLocation(board, here)) {
            continue;
        }

        int tile = board->getTile(here);

        int roll;
        int decor[4] = { tileDoorClosed, tileDoorClosed, tileWindow };
        switch(tile) {
            case tileWall:
                roll = rand() % 3;
                board->setTile(here, decor[roll]);
                break;
            case tileFloor:
                roll = rand() % 2;
                board->setTile(here, decor[roll]);
                break;
        }
    }

    // add the down stairs
    if (flags & MF_ADD_DOWN) {
        Point downStair;
        do {
            downStair = Point(1 + (rand() % (board->width() - 2)),
                            1 + (rand() % (board->height() - 2)));
        } while (board->getTile(downStair) != tileFloor);
        board->setTile(downStair, tileDown);
    }

    // add the up stairs
    if (flags & MF_ADD_UP) {
        Point upStair;
        do {
            upStair = Point(1 + (rand() % (board->width() - 2)),
                            1 + (rand() % (board->height() - 2)));
        } while (board->getTile(upStair) != tileFloor);
        board->setTile(upStair, tileUp);
    }
}

/* ************************************************************************** *
 * DEAD END CLEANUP                                                           *
 * ************************************************************************** */
Dir openDirection(Board *board, const Point &point) {
    Dir initialDir = Dir::North;
    Dir theDir = Dir::None;
    Dir d = initialDir;
    do {
        Point dest = point.shift(d, 1);
        int tile = board->getTile(dest);
        if (tile == tileWindow) return Dir::None;
        if (tile == tileFloor || tile == tileDoorClosed || tile == tileOutOfBounds) {
            if (theDir != Dir::None)    return Dir::None;
            else                        theDir = d;
        }
        d = rotateDirection(d);
    } while (d != initialDir);
    return theDir;
}

void addNewDoor(Board *board, const Point &here, Random &rng) {
    Dir startDir = randomDirection(rng);
    Dir theDir = startDir;
    do {
        Point dest = here.shift(theDir, 1);
        int tile = board->getTile(dest);
        if (tile == tileWall && validDoorLocation(board, dest)) {
            board->setTile(dest, rand() % 3 == 1 ? tileWindow : tileDoorClosed);
            return;
        }
        theDir = rotateDirection(theDir);
    } while (theDir != startDir);
}

void doTrimDeadEnd(Board *board, Point where) {
    while (1) {
        Dir openDir = openDirection(board, where);
        if (openDir == Dir::None) break;
        board->setTile(where, tileWall);
        where = where.shift(openDir, 1);
    }
}

void trimDeadEnds(Board *board, int trimPercent, Random &rng) {
    for (int y = 0; y < board->height(); ++y) {
        for (int x = 0; x < board->width(); ++x) {
            Point here(x, y);
            int tile = board->getTile(here);
            if (tile != tileFloor && tile != tileDoorClosed) continue;
            if ((rand() % 100) > trimPercent) break;
            if (openDirection(board, here) != Dir::None) {
                if (rand() % 10000 > 3000)  addNewDoor(board, here, rng);
                else                        doTrimDeadEnd(board, here);
            }
        }
    }
}
