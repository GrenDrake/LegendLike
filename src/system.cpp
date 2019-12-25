#include "creature.h"
#include "board.h"
#include "vm.h"
#include "gfx.h"


System::System(SDL_Renderer *renderer, Random &rng)
: turnNumber(1), depth(0), mCurrentBoard(nullptr),  mPlayer(nullptr),
  smallFont(nullptr), tinyFont(nullptr), mCurrentTrack(-1),
  mCurrentMusic(nullptr), renderer(renderer), coreRNG(rng), vm(nullptr),
  config(nullptr), wantsToQuit(false), showTooltip(false), showInfo(false),
  showFPS(false), wantsTick(false), fpsManager(nullptr), fps(0)
{
}

System::~System() {
    endGame();
}

void System::reset() {
    endGame();

    turnNumber = 1;
    depth = 0;
    mCurrentBoard = nullptr;
    wantsTick = false;

    mPlayer = new Creature(1);
    mPlayer->name = "player";
    mPlayer->isPlayer = true;
    mPlayer->aiType = aiPlayer;
    mPlayer->talkFunc = 0;
    mPlayer->reset();
}

void System::endGame() {
    messages.clear();
    if (mCurrentBoard) mCurrentBoard = nullptr;
    if (mPlayer) mPlayer = nullptr;
    for (auto boardIter : mBoards) {
        delete boardIter.second;
    }
    mBoards.clear();
}

void System::requestTick() {
    wantsTick = true;
}

bool System::hasTick() const {
    return wantsTick;
}

void System::tick() {
    wantsTick = false;
    if (mCurrentBoard) {
        ++turnNumber;
        mCurrentBoard->tick();
    }
}

bool System::warpTo(int boardIndex, int x, int y) {
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

bool System::down() {
    int newDepth = depth + 1;
    if (build(newDepth)) {
        Point startPos = mCurrentBoard->findTile(tileUp);
        mCurrentBoard->addActor(mPlayer, startPos);
    } else {
        return false;
    }
    return true;
}

bool System::up() {
    int newDepth = depth - 1;
    if (build(newDepth)) {
        Point startPos = mCurrentBoard->findTile(tileDown);
        mCurrentBoard->addActor(mPlayer, startPos);
    } else {
        return false;
    }
    return true;
}

bool System::build(int forIndex) {
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
            vm->run(info.buildFunction);
        }
        mBoards.insert(std::make_pair(forIndex, mCurrentBoard));
    }
    if (info.enterFunction) vm->run(info.enterFunction);
    depth = forIndex;
    if (info.musicTrack >= 0) {

        playMusic(info.musicTrack);
    }
    return true;
}
