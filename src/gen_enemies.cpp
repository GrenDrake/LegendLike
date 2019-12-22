#include "board.h"
#include "logger.h"
#include "random.h"
#include "mapactor.h"

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

        int rowId = rng.next32() % info.rows.size();
        const RandomFoeRow &row = info.rows[rowId];
        MapActor *actor = new MapActor(row.name, row.art, row.ai, 0, -1, row.fightInfo);
        board->addActor(actor, here);
    }
}
