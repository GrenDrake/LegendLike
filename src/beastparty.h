#ifndef BEASTPARTY_H
#define BEASTPARTY_H

#include <array>
#include <vector>

struct Beast;
class Random;
class System;

enum class CombatType {
    Wild     = 0,
    Informal = 1,
    Formal   = 2,
    Deadly   = 3
};

enum class CombatResult {
    Victory = 1,
    Defeat  = 0,
    Retreat = -1,
    QuitGame = -2,
    None     = -3
};


class BeastParty {
public:
    static const int MAX_SIZE = 4;

    BeastParty();
    bool add(Beast*);
    int size() const;
    Beast* at(unsigned slot);
    bool remove(unsigned slot);
    Beast* getRandom(Random &rng);
    bool alive() const;
    void clear();
    void deleteAll();
    void reset();
private:
    std::array<Beast*, MAX_SIZE> mParty;
};

struct CombatInfo {
    BeastParty foes;
    CombatType type;
    int background;

    std::vector<std::string> log;
    int playerAction, curBeast, curSelection, roundNumber;
    bool retreatFlag;
};


CombatResult doCombat(System &system, CombatInfo &info);

#endif
