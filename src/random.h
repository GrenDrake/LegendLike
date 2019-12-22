/*
    Random number generator.

    Based on XorShift+ as described at:
    https://codingha.us/2018/12/17/xorshift-fast-csharp-random-number-generator/
*/

#ifndef RANDOM_H
#define RANDOM_H

#include <cstdint>

class Random {
public:
    Random()
    : v1(203594), v2(708952)
    { }

    void seed(std::uint64_t seed) {
        v1 = seed << 1;
        v2 = seed >> 2;
    }

    uint64_t next64() {
        uint64_t t1, t2, result;
        t1 = v2;
        v1 ^= v1 << 23;
        t2 = v1 ^ v2 ^ (v1 >> 17) ^ (v2 >> 26);
        result = t2 + v2;
        v1 = t1;
        v2 = t2;
        return result;
    }

    uint32_t next32() {
        int result;
        uint64_t t1, t2;

        t1 = v2;
        v1 ^= v1 << 23;
        t2 = v1 ^ v2 ^ (v1 >> 17) ^ (v2 >> 26);
        result = static_cast<int>(t2 + v2);
        v1 = t1;
        v2 = t2;
        return result;
    }

    unsigned roll(unsigned dice, unsigned sides) {
        unsigned result = 0;
        for (unsigned i = 0; i < dice; ++i) {
            result += between(1, sides);
        }
        return result;
    }

    unsigned between(unsigned low, unsigned high) {
        unsigned range = high - low + 1;
        unsigned value = next32();
        return low + (value % range);
    }

private:
    std::uint64_t v1, v2;
};

#endif