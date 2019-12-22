#include <cmath>
#include <ostream>

#include "point.h"
#include "random.h"

Point Point::shift(Dir dir, int amount) const {
    switch(dir) {
        case Dir::North:
            return Point(mX, mY - amount);
        case Dir::East:
            return Point(mX + amount, mY);
        case Dir::South:
            return Point(mX, mY + amount);
        case Dir::West:
            return Point(mX - amount, mY);
        default:
            return Point(*this);
    }
}

double Point::distanceTo(const Point &rhs) const {
    return sqrt((mX-rhs.mX ) * (mX-rhs.mX) + (mY-rhs.mY) * (mY-rhs.mY));
}

Dir Point::directionTo(const Point &other) const {
    if (other == *this) return Dir::Here;

    int sx = mX - other.mX;
    int sy = mY - other.mY;
    int dx = std::abs(static_cast<double>(sx));
    int dy = std::abs(static_cast<double>(sy));

    if (dx > dy) {
        if (sx < 0) return Dir::East;
        else        return Dir::West;
    } else {
        if (sy < 0) return Dir::South;
        else        return Dir::North;
    }
}

Dir flipDirection(Dir original) {
    switch(original) {
        case Dir::North:  return Dir::South;
        case Dir::South:  return Dir::North;
        case Dir::East:   return Dir::West;
        case Dir::West:   return Dir::East;
        default:        return Dir::North;
    }
}
Dir rotateDirection(Dir original) {
    switch(original) {
        case Dir::North:  return Dir::East;
        case Dir::East:   return Dir::South;
        case Dir::South:  return Dir::West;
        case Dir::West:   return Dir::North;
        default:        return Dir::North;
    }
}

Dir randomDirection(Random &rng) {
    switch(rng.next32() % 4) {
        case 0: return Dir::North;
        case 1: return Dir::East;
        case 2: return Dir::South;
        case 3: return Dir::West;
    }
    return Dir::Here;
}

std::ostream& operator<<(std::ostream &out, const Dir &d) {
    switch (d) {
        case Dir::North:        out << "north"; break;
        case Dir::Northeast:    out << "northeast"; break;
        case Dir::East:         out << "east"; break;
        case Dir::Southeast:    out << "southeast"; break;
        case Dir::South:        out << "south"; break;
        case Dir::Southwest:    out << "southwest"; break;
        case Dir::West:         out << "west"; break;
        case Dir::Northwest:    out << "northwest"; break;
        case Dir::Here:         out << "here"; break;
        case Dir::None:         out << "none"; break;
    }
    return out;
}