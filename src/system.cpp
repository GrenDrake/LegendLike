#include <SDL2/SDL.h>

#include "actor.h"
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
  mapEditMode(false), mapEditTile(0), framecount(0), framerate(0), baseticks(0), lastticks(0), fps(0)
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

    mPlayer = new Actor(playerTypeId);
    mPlayer->name = "player";
    mPlayer->isPlayer = true;
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

void System::addInfo(const std::string &text) {
    messages.push_back(Message{
        turnNumber,
        "\x1B\x02\x7F\x7F\xFF" + text
    });
}

void System::addError(const std::string &text) {
    messages.push_back(Message{
        turnNumber,
        "\x1B\x02\xFF\x7F\x7F" + text
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
        Board *newBoard = new Board(info);
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

bool System::grantItem(int itemId) {
    switch(itemId) {
        case ITM_SWORD_UPGRADE:
            ++swordLevel;
            break;
        case ITM_ARMOUR_UPGRADE:
            ++armourLevel;
            break;
        case ITM_ENERGY_UPGRADE:
            addError("Energy upgrade not implemented.");
            break;
        case ITM_HEALTH_UPGRADE:
            addError("Health upgrade not implemented.");
            break;
        case ITM_BOW:
            ++subweaponLevel[SW_BOW];
            if (currentSubweapon < 0) currentSubweapon = SW_BOW;
            break;
        case ITM_HOOKSHOT:
            ++subweaponLevel[SW_HOOKSHOT];
            if (currentSubweapon < 0) currentSubweapon = SW_HOOKSHOT;
            break;
        case ITM_ICEROD:
            ++subweaponLevel[SW_ICEROD];
            if (currentSubweapon < 0) currentSubweapon = SW_ICEROD;
            break;
        case ITM_FIREROD:
            ++subweaponLevel[SW_FIREROD];
            if (currentSubweapon < 0) currentSubweapon = SW_FIREROD;
            break;
        case ITM_PICKAXE:
            ++subweaponLevel[SW_PICKAXE];
            if (currentSubweapon < 0) currentSubweapon = SW_PICKAXE;
            break;
        case ITM_AMMO_ARROW:
            arrowCount += 5;
            if (arrowCount > arrowCapacity) arrowCount = arrowCapacity;
            break;
        case ITM_AMMO_BOMB:
            bombCount += 1;
            subweaponLevel[SW_BOMB] = 1;
            if (bombCount > bombCapacity) bombCount = bombCapacity;
            if (currentSubweapon < 0) currentSubweapon = SW_BOMB;
            break;
        case ITM_COIN:
            coinCount += 10;
            break;
        case ITM_CAP_ARROW:
            arrowCapacity += 5;
            break;
        case ITM_CAP_BOMB:
            bombCapacity += 1;
            break;
        default:
            addError("Tried to give unknown item #" + std::to_string(itemId));
            return false;
    }
    return true;
}

bool System::hasItem(int itemId) {
    switch(itemId) {
        case ITM_SWORD_UPGRADE:     return swordLevel > 0;
        case ITM_ARMOUR_UPGRADE:    return armourLevel > 0;
        case ITM_ENERGY_UPGRADE:
            addError("Energy upgrade not implemented.");
            return false;
        case ITM_HEALTH_UPGRADE:
            addError("Health upgrade not implemented.");
            return false;
        case ITM_BOW:               return subweaponLevel[SW_BOW] > 0;
        case ITM_HOOKSHOT:          return subweaponLevel[SW_HOOKSHOT] > 0;
        case ITM_ICEROD:            return subweaponLevel[SW_ICEROD] > 0;
        case ITM_FIREROD:           return subweaponLevel[SW_FIREROD] > 0;
        case ITM_PICKAXE:           return subweaponLevel[SW_PICKAXE] > 0;
        case ITM_AMMO_ARROW:        return arrowCount > 0;
        case ITM_AMMO_BOMB:         return bombCount > 0;
        case ITM_COIN:              return coinCount > 0;
        case ITM_CAP_ARROW:         return true;
        case ITM_CAP_BOMB:          return true;
        default:
            addError("Tried to check for unknown item #" + std::to_string(itemId));
            return false;
    }
    return false;
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
