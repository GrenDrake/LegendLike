#include "beast.h"
#include "gamestate.h"
#include "board.h"
#include "mapactor.h"
#include "vm.h"
#include "gfx.h"


GameState::GameState(Random &rng, VM &vm)
: wantsTick(false), rng(rng), vm(vm), mTurnNumber(1), mDepth(0), mCurrentBoard(nullptr)
{
    mPlayer = new MapActor("player", 41, aiPlayer, 0, 0, 0);
}
GameState::~GameState() {
    for (auto boardIter : mBoards) {
        delete boardIter.second;
    }
}

void GameState::requestTick() {
    wantsTick = true;
}

bool GameState::hasTick() const {
    return wantsTick;
}

void GameState::tick() {
    wantsTick = false;
    if (mCurrentBoard) {
        ++mTurnNumber;
        mCurrentBoard->tick();
    }
}

bool GameState::warpTo(int boardIndex, int x, int y) {
    if (boardIndex < 0) {
        if (!mCurrentBoard) return false;
        mCurrentBoard->removeActor(mPlayer);
        mCurrentBoard->addActor(mPlayer, Point(x, y));
    } else {
        int newDepth = boardIndex;
        if (build(newDepth)) {
            mCurrentBoard->addActor(mPlayer, Point(x, y));
            mCurrentBoard->calcFOV(getPlayer()->position);
        } else {
            return false;
        }
    }
    return true;
}

bool GameState::down() {
    int newDepth = mDepth + 1;
    if (build(newDepth)) {
        Point startPos = mCurrentBoard->findTile(tileUp);
        mCurrentBoard->addActor(mPlayer, startPos);
    } else {
        return false;
    }
    return true;
}

bool GameState::up() {
    int newDepth = mDepth - 1;
    if (build(newDepth)) {
        Point startPos = mCurrentBoard->findTile(tileDown);
        mCurrentBoard->addActor(mPlayer, startPos);
    } else {
        return false;
    }
    return true;
}

bool GameState::build(int forIndex) {
    const MapInfo &info = MapInfo::get(forIndex);
    if (info.index < 0) return false;

    if (mCurrentBoard) {
        mCurrentBoard->removeActor(mPlayer);
    }

    auto oldBoard = mBoards.find(forIndex);
    if (oldBoard != mBoards.end()) {
        mCurrentBoard = oldBoard->second;
    } else {
        Board *newBoard = new Board(info.width, info.height, info.name);
        mCurrentBoard = newBoard;
        if (info.buildFunction) {
            vm.run(info.buildFunction);
        }
        mBoards.insert(std::make_pair(forIndex, mCurrentBoard));
    }
    if (info.enterFunction) vm.run(info.enterFunction);
    mDepth = forIndex;
    if (info.musicTrack >= 0) {

        system->playMusic(info.musicTrack);
    }
    return true;
}
