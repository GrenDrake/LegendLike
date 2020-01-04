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
        case Dir::Northwest:
            return Point(mX - amount, mY - amount);
        case Dir::Northeast:
            return Point(mX + amount, mY - amount);
        case Dir::Southwest:
            return Point(mX - amount, mY + amount);
        case Dir::Southeast:
            return Point(mX + amount, mY + amount);
        default:
            return Point(*this);
    }
}

double Point::distanceTo(const Point &rhs) const {
    return sqrt((mX-rhs.mX ) * (mX-rhs.mX) + (mY-rhs.mY) * (mY-rhs.mY));
}

Dir Point::directionTo(const Point &other) const {
    if (other == *this) return Dir::Here;

    double rX = other.mX - mX;
    double rY = other.mY - mY;
    double angle = atan2(rX, rY) * (180 / M_PI);
    if (angle < 0) angle += 360.0;
    if (angle < 22.5)  return Dir::South;
    if (angle < 67.5)  return Dir::Southeast;
    if (angle < 112.5) return Dir::East;
    if (angle < 157.5) return Dir::Northeast;
    if (angle < 202.5) return Dir::North;
    if (angle < 247.5) return Dir::Northwest;
    if (angle < 292.5) return Dir::West;
    if (angle < 337.5) return Dir::Southwest;
    return Dir::North;
}

Dir flipDirection(Dir original) {
    switch(original) {
        case Dir::North:        return Dir::South;
        case Dir::Northeast:    return Dir::Southwest;
        case Dir::East:         return Dir::West;
        case Dir::Southeast:    return Dir::Northwest;
        case Dir::South:        return Dir::North;
        case Dir::Southwest:    return Dir::Northeast;
        case Dir::West:         return Dir::East;
        case Dir::Northwest:    return Dir::Southeast;
        default:                return Dir::North;
    }
}

Dir rotateDirection(Dir original) {
    switch(original) {
        case Dir::North:        return Dir::East;
        case Dir::East:         return Dir::South;
        case Dir::South:        return Dir::West;
        case Dir::West:         return Dir::North;
        case Dir::Northeast:    return Dir::Southeast;
        case Dir::Southeast:    return Dir::Southwest;
        case Dir::Southwest:    return Dir::Northwest;
        case Dir::Northwest:    return Dir::Northeast;
        default:                return Dir::North;
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

std::string dirName(const Dir &d) {
    switch (d) {
        case Dir::North:        return "north";
        case Dir::Northeast:    return "northeast";
        case Dir::East:         return "east";
        case Dir::Southeast:    return "southeast";
        case Dir::South:        return "south";
        case Dir::Southwest:    return "southwest";
        case Dir::West:         return "west";
        case Dir::Northwest:    return "northwest";
        case Dir::Here:         return "here";
        case Dir::None:         return "none";
    }
    return "Dir#" + std::to_string(static_cast<int>(d));
}

std::ostream& operator<<(std::ostream &out, const Point &p) {
    out << '(' << p.x() << ',' << p.y() << ')';
    return out;
}

std::ostream& operator<<(std::ostream &out, const Dir &d) {
    out << dirName(d);
    return out;
}

bool operator<(const Point &left, const Point &right) {
    if (left.x() < right.x()) return true;
    if (left.x() > right.x()) return false;
    if (left.y() < right.y()) return true;
    return false;
}
