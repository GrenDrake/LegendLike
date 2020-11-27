#include <SDL2/SDL.h>

#include "actor.h"
#include "logger.h"
#include "board.h"
#include "vm.h"
#include "gamestate.h"


AnimFrame::AnimFrame(int type)
: special(type)
{ }

AnimFrame::AnimFrame(const Point &point, SDL_Texture *texture)
: special(animFrame)
{
    data.insert(std::make_pair(point, texture));
}

AnimFrame::AnimFrame(int type, const std::string &text)
: special(type), text(text)
{ }

AnimFrame::AnimFrame(Actor *actor, int amount, int damageType, const std::string &source)
: special(animDamage), actor(actor), damageAmount(amount), damageType(damageType), text(source)
{ }

GameState::GameState(SDL_Renderer *renderer, Random &rng)
: runDirection(Dir::None),
  swordLevel(0), armourLevel(0),
  subweaponLevel{0},
  arrowCount(0), bombCount(0), coinCount(0),
  arrowCapacity(30), bombCapacity(10), currentSubweapon(-1),
  turnNumber(1), depth(0), mCurrentBoard(nullptr),  mPlayer(nullptr),
  cursor(-1,-1), smallFont(nullptr), tinyFont(nullptr), mCurrentTrack(-1),
  mCurrentMusic(nullptr),renderer(renderer), coreRNG(rng), vm(nullptr),
  config(nullptr), wantsToQuit(false), gameInProgress(false), returnToMenu(false), showTooltip(false),
  showInfo(false), showFPS(false), wantsTick(false),
  mapEditTile(-1), framecount(0), framerate(0), baseticks(0), lastticks(0), fps(0)
{
}

GameState::~GameState() {
    endGame();
}

void GameState::reset() {
    Logger &logger = Logger::getInstance();
    logger.info("Resetting game state");
    endGame();

    unsigned count = 0;
    for (const MapInfo &mapInfo : MapInfo::types) {
        Board *newBoard = new Board(mapInfo);
        mCurrentBoard = newBoard;
        if (mapInfo.onBuild) vm->run(mapInfo.onBuild);
        mBoards.insert(std::make_pair(mapInfo.index, newBoard));
        ++count;
    }
    logger.info("Built " + std::to_string(count) + " maps.");

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

void GameState::endGame() {
    messages.clear();
    if (mCurrentBoard) mCurrentBoard = nullptr;
    if (mPlayer) mPlayer = nullptr;
    for (auto boardIter : mBoards) {
        delete boardIter.second;
    }
    mBoards.clear();
}

void GameState::setFontScale(int scale) {
    for (auto iter : mFonts) {
        iter.second->setScale(scale);
    }
}

void GameState::queueFrame(const AnimFrame &frame) {
    animationQueue.push_back(frame);
}

void GameState::queueFrames(const std::vector<AnimFrame> &frame) {
    animationQueue.insert(animationQueue.begin(), frame.begin(), frame.end());
}

void GameState::addMessage(const std::string &text) {
    messages.push_back(Message{
        turnNumber,
        text
    });
}

void GameState::addInfo(const std::string &text) {
    messages.push_back(Message{
        turnNumber,
        "\x1B\x02\x7F\x7F\xFF" + text
    });
}

void GameState::addError(const std::string &text) {
    messages.push_back(Message{
        turnNumber,
        "\x1B\x02\xFF\x7F\x7F" + text
    });
}

void GameState::appendMessage(const std::string &newText) {
    if (messages.empty()) {
        addMessage(newText);
    } else {
        messages.back().text += newText;
    }
}

void GameState::replaceMessage(const std::string &newText) {
    if (messages.empty()) {
        addMessage(newText);
    } else {
        messages.back().text = newText;
    }
}

void GameState::removeMessage() {
    if (!messages.empty()) {
        messages.pop_back();
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
        mCurrentBoard->tick(*this);
        ++turnNumber;
    }
}


bool GameState::warpTo(int boardIndex, int x, int y) {
    if (boardIndex < 0) {
        // warp within current board
        if (!mCurrentBoard) return false;
        mCurrentBoard->removeActor(mPlayer);
        mCurrentBoard->addActor(mPlayer, Point(x, y));
    } else {
        // warp to new board
        if (switchBoard(boardIndex)) {
            mCurrentBoard->addActor(mPlayer, Point(x, y));
            mCurrentBoard->calcFOV(getPlayer()->position);
        } else {
            return false;
        }
    }
    return true;
}

bool GameState::down() {
    int newDepth = depth + 1;
    if (switchBoard(newDepth)) {
        Point startPos = mCurrentBoard->findTile(tileUp);
        mCurrentBoard->addActor(mPlayer, startPos);
    } else {
        return false;
    }
    return true;
}

bool GameState::up() {
    int newDepth = depth - 1;
    if (switchBoard(newDepth)) {
        Point startPos = mCurrentBoard->findTile(tileDown);
        mCurrentBoard->addActor(mPlayer, startPos);
    } else {
        return false;
    }
    return true;
}

void GameState::screenTransition(Dir dir) {
    const World &world = getWorld();
    if (world.index < 0) return;
    Point work = getWorldPosition();
    work = work.shift(dir);
    int mapId = world.mapForPosition(work);
    if (mapId < 0) return;
    Point playerPos = getPlayer()->position;
    switch(dir) {
        case Dir::North:
            playerPos = Point(playerPos.x(), mCurrentBoard->height() - 1);
            break;
        case Dir::South:
            playerPos = Point(playerPos.x(), 0);
            break;
        case Dir::West:
            playerPos = Point(mCurrentBoard->width() - 1, playerPos.y());
            break;
        case Dir::East:
            playerPos = Point(0, playerPos.y());
            break;
        default:
            // we don't care about the other directions
            break;
    }
    warpTo(mapId, playerPos.x(), playerPos.y());
}

bool GameState::switchBoard(int forIndex) {
    const MapInfo &info = MapInfo::get(forIndex);
    if (info.index < 0) return false;

    if (mCurrentBoard) {
        mCurrentBoard->removeActor(mPlayer);
    }

    auto oldBoard = mBoards.find(forIndex);
    if (oldBoard == mBoards.end()) {
        Logger &log = Logger::getInstance();
        log.error("Failed to access map ID " + std::to_string(forIndex));
        return false;
    }
    mCurrentBoard = oldBoard->second;
    mCurrentBoard->reset(*this);
    depth = forIndex;
    if (info.musicTrack >= 0) {

        playMusic(info.musicTrack);
    }
    return true;
}

const World noWorld{ "", -1 };
const World& GameState::getWorld() const {
    const int boardId = mCurrentBoard->getInfo().index;
    for (const World &world : worlds) {
        if (boardId >= world.firstMap && boardId <= world.lastMap) {
            return world;
        }
    }
    return noWorld;
}

Point GameState::getWorldPosition() const {
    const int boardId = mCurrentBoard->getInfo().index;
    for (const World &world : worlds) {
        if (boardId >= world.firstMap && boardId <= world.lastMap) {
            int index = boardId - world.firstMap;
            int y = index / world.width;
            int x = index - y * world.width;
            return Point(x, y);
        }
    }
    return Point(-1, -1);
}

bool GameState::grantItem(int itemId) {
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
        case ITM_RESTORE_EN: {
            Actor *player = getPlayer();
            player->curEnergy += 3;
            if (player->curEnergy > player->typeInfo->maxEnergy) {
                player->curEnergy = player->typeInfo->maxEnergy;
            }
            break; }
        case ITM_RESTORE_HP: {
            Actor *player = getPlayer();
            player->curHealth += 3;
            if (player->curHealth > player->typeInfo->maxHealth) {
                player->curHealth = player->typeInfo->maxHealth;
            }
            break; }
        default:
            addError("Tried to give unknown item #" + std::to_string(itemId));
            return false;
    }
    return true;
}

bool GameState::hasItem(int itemId) {
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

void GameState::advanceFrame() {
    ++framecount;
    ++timerFrames;
    unsigned newTime = SDL_GetTicks();
    unsigned frameTime = newTime - lastticks;
    const unsigned MS_PER_FRAME = 16;
    if (frameTime < MS_PER_FRAME) SDL_Delay(MS_PER_FRAME - frameTime);
    lastticks = SDL_GetTicks();
}

unsigned GameState::getFPS() const {
    return fps;
}
