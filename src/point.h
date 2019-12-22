#ifndef POINT_H
#define POINT_H

class Random;

enum class Dir {
    North,
    Northeast,
    East,
    Southeast,
    South,
    Southwest,
    West,
    Northwest,
    Here,
    None
};


class Point {
public:
    Point()
    : mX(0), mY(0)
    { }
    Point(int x, int y)
    : mX(x), mY(y)
    { }

    int x() const {
        return mX;
    }
    int y() const {
        return mY;
    }

    Point shift(Dir dir, int amount = 1) const;
    double distanceTo(const Point &rhs) const;
    Dir directionTo(const Point &other) const;

    bool operator==(const Point &rhs) const {
        return mX == rhs.mX && mY == rhs.mY;
    }

private:
    int mX, mY;
};

Dir flipDirection(Dir original);
Dir rotateDirection(Dir original);
Dir randomDirection(Random &rng);
std::ostream& operator<<(std::ostream &out, const Dir &d);

#endif
