#include "board.h"
#include "creature.h"
#include "logger.h"
#include "random.h"

const int MAX_ITERATIONS = 10000;

void mapRandomEnemies(Board *board, Random &rng, const RandomFoeInfo &info) {
    if (!board) return;


    for (int i = 0; i < info.count; ++i) {
        Point here;
        int iterations = 0;
        do {
            ++iterations;
            here = Point(1 + (rand() % (board->width() - 2)),
                         1 + (rand() % (board->height() - 2)));
        } while (iterations < MAX_ITERATIONS && board->getTile(here) != tileFloor);

        if (board->getTile(here) != tileFloor) {
            Logger &log = Logger::getInstance();
            log.warn("Failed to place MapActor in " + std::to_string(MAX_ITERATIONS) + " attempts.");
            continue;
        }

        int rowId = rng.next32() % info.typeList.size();
        int type = info.typeList[rowId];
        Creature *actor = new Creature(type);
        actor->reset();
        board->addActor(actor, here);
    }
}
