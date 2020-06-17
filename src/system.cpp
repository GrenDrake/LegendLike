#include <SDL2/SDL.h>

#include "creature.h"
#include "logger.h"
#include "board.h"
#include "vm.h"
#include "gfx.h"


System::System(SDL_Renderer *renderer, Random &rng)
: runDirection(Dir::None),
  swordLevel(0), armourLevel(0),
  subweaponLevel{0},
  arrowCount(0), bombCount(0), coinCount(0),
  arrowCapacity(30), bombCapacity(10), currentSubweapon(-1),
  turnNumber(1), depth(0), mCurrentBoard(nullptr),  mPlayer(nullptr),
  quickSlots{ {0} }, cursor(-1,-1), smallFont(nullptr), tinyFont(nullptr), mCurrentTrack(-1),
  mCurrentMusic(nullptr),renderer(renderer), coreRNG(rng), vm(nullptr),
  config(nullptr), wantsToQuit(false), returnToMenu(false), showTooltip(false),
  showInfo(false), showFPS(false), wantsTick(false),
  framecount(0), framerate(0), baseticks(0), lastticks(0), fps(0), showDieRolls(false)
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

    mPlayer = new Creature(playerTypeId);
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

void System::setFontScale(int scale) {
    for (auto iter : mFonts) {
        iter.second->setScale(scale);
    }
}

void System::queueAnimation(const Animation &anim) {
    switch(anim.type) {
        case AnimType::None:
            return;
        case AnimType::Travel:
        case AnimType::All:
            if (anim.points.empty()) return;
            break;
    }
    animationQueue.push_back(anim);
}

void System::addMessage(const std::string &text) {
    messages.push_back(Message{
        turnNumber,
        text
    });
}

void System::appendMessage(const std::string &newText) {
    if (messages.empty()) {
        addMessage(newText);
    } else {
        messages.back().text += newText;
    }
}

void System::replaceMessage(const std::string &newText) {
    if (messages.empty()) {
        addMessage(newText);
    } else {
        messages.back().text = newText;
    }
}

void System::removeMessage() {
    if (!messages.empty()) {
        messages.pop_back();
    }
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
        mCurrentBoard->tick(*this);
        ++turnNumber;
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
            Logger &log = Logger::getInstance();
            log.error("Failed to access map ID " + std::to_string(boardIndex));
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
        if (info.onBuild) {
            vm->run(info.onBuild);
        }
        mBoards.insert(std::make_pair(forIndex, mCurrentBoard));
    }
    if (info.onEnter) vm->run(info.onEnter);
    depth = forIndex;
    if (info.musicTrack >= 0) {

        playMusic(info.musicTrack);
    }
    return true;
}

void System::advanceFrame() {
    ++framecount;
    ++timerFrames;
    unsigned newTime = SDL_GetTicks();
    unsigned frameTime = newTime - lastticks;
    const unsigned MS_PER_FRAME = 16;
    if (frameTime < MS_PER_FRAME) SDL_Delay(MS_PER_FRAME - frameTime);
    lastticks = SDL_GetTicks();
}

unsigned System::getFPS() const {
    return fps;
}
