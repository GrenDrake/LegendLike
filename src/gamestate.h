#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <SDL2/SDL_mixer.h>

#include <deque>
#include <map>
#include <string>
#include <vector>
#include "point.h"

class Board;
class Actor;
class Random;
struct SDL_Renderer;
struct SDL_Window;
struct SDL_Texture;
class Font;
class VM;
class Config;
struct SDL_Rect;

const int SW_BOW = 0;
const int SW_HOOKSHOT = 1;
const int SW_BOMB = 2;
const int SW_PICKAXE = 3;
const int SW_FIREROD = 4;
const int SW_ICEROD = 5;
const int SW_COUNT = 6;

const int ITM_SWORD_UPGRADE  = 0;
const int ITM_ARMOUR_UPGRADE = 1;
const int ITM_HEALTH_UPGRADE = 2;
const int ITM_ENERGY_UPGRADE = 3;
const int ITM_BOW            = 4;
const int ITM_HOOKSHOT       = 5;
const int ITM_ICEROD         = 7;
const int ITM_FIREROD        = 8;
const int ITM_PICKAXE        = 9;
const int ITM_AMMO_ARROW     = 10;
const int ITM_AMMO_BOMB      = 11;
const int ITM_COIN           = 12;
const int ITM_CAP_ARROW      = 13;
const int ITM_CAP_BOMB       = 14;

struct Color {
    int r, g, b;
};

const int animFrame = 0;
const int animDamage = 1;
const int animText = 2;
const int animRollText = 3;
struct AnimFrame {
    AnimFrame(int type);
    AnimFrame(const Point &point, SDL_Texture *texture);
    AnimFrame(int type, const std::string &text);
    AnimFrame(Actor *actor, int amount, int damageType, const std::string &source);

    int special;
    // for animation frames
    std::map<Point, SDL_Texture*> data;
    // for damage frames
    Actor *actor;
    int damageAmount, damageType;
    // for damage, message, and roll frames
    std::string text;
};

struct TrackInfo {
    int number;
    std::string file;
    std::string name;
    std::string artist;
};

struct Message {
    int newTurns;
    std::string text;
};

struct ItemLocation {
    int itemId;
    bool used;
};

struct Subweapon {
    std::string name;
    std::string artfile;
    bool directional;
};

struct LootRow {
    int chance;
    int itemId;
};

struct LootTable {
    std::vector<LootRow> rows;
};

struct ItemDef {
    std::string name;
    std::string artFile;
    int itemId;

    SDL_Texture *art;
};

struct Item {
    const ItemDef *typeInfo;
    Point position;
    int fromLocation;
};

struct World {
    std::string name;
    int index;
    int firstMap, lastMap;
    int width, height;

    bool valid(const Point &where) const {
        if (where.x() < 0 || where.y() < 0) return false;
        if (where.x() >= width || where.y() >= height) return false;
        return true;
    }
    int mapForPosition(const Point &where) const {
        if (!valid(where)) return -1;
        int offset = where.x() + where.y() * width;
        return firstMap + offset;
    }
};

class System {
public:
    System(SDL_Renderer *renderer, Random &rng);
    ~System();

    bool load();
    void unloadAll();
    void reset();
    void endGame();

    SDL_Texture* getImageCore(const std::string &name);
    SDL_Texture* getImage(const std::string &name);
    Mix_Music* getMusic(const std::string &name);
    Mix_Chunk* getAudio(const std::string &name);
    Font* getFont(const std::string &name);

    void setFontScale(int scale);

    void queueFrame(const AnimFrame &frame);
    void queueFrames(const std::vector<AnimFrame> &frame);

    void playMusic(int trackNumber);
    void setMusicVolume(int volume);
    int getTrackNumber() const;
    const TrackInfo& getTrackInfo();

    void playEffect(int effectNumber);
    void setAudioVolume(int volume);

    void addMessage(const std::string &text);
    void addInfo(const std::string &text);
    void addError(const std::string &text);
    void appendMessage(const std::string &newText);
    void replaceMessage(const std::string &newText);
    void removeMessage();

    void requestTick();
    bool hasTick() const;
    void tick();

    Board* getBoard() {
        return mCurrentBoard;
    }
    Actor* getPlayer() {
        return mPlayer;
    }
    bool warpTo(int boardIndex, int x, int y);
    bool down();
    bool up();
    void screenTransition(Dir dir);
    bool switchBoard(int forIndex);
    const World& getWorld() const;
    Point getWorldPosition() const;
    bool grantItem(int itemId);
    bool hasItem(int itemId);

    void advanceFrame();
    unsigned getFPS() const;

    // player state
    Dir runDirection;
    int swordLevel, armourLevel;
    int subweaponLevel[SW_COUNT];
    int arrowCount, bombCount, coinCount;
    int arrowCapacity, bombCapacity;
    int currentSubweapon;

    // message log
    std::vector<Message> messages;

    // Map data
    int turnNumber;
    int depth;
    Board *mCurrentBoard;
    Actor *mPlayer;
    std::map<int, Board*> mBoards;
    Point cursor;

    // game resources
    Font *smallFont;
    Font *tinyFont;
    int mCurrentTrack;
    Mix_Music *mCurrentMusic;
    std::map<int, SDL_Texture*> mTiles;
    std::map<int, TrackInfo> mTracks;
    std::map<int, Mix_Chunk*> mAudio;
    std::map<std::string, SDL_Texture*> mImages;
    std::map<std::string, Font*> mFonts;
    std::vector<ItemLocation> itemLocations;
    std::vector<Subweapon> subweapons;
    std::vector<LootTable> lootTables;
    std::vector<ItemDef> itemDefs;
    std::vector<World> worlds;
    std::deque<AnimFrame> animationQueue;

    // system modules
    SDL_Renderer *renderer;
    SDL_Window *window;
    Random &coreRNG;
    VM *vm;
    Config *config;

    // game configuration flags
    bool wantsToQuit;
    bool gameInProgress;
    bool returnToMenu;
    bool showTooltip;
    bool showInfo;
    bool showFPS;
    bool wantsTick;
    bool mapEditMode;
    int  mapEditTile;

    bool loadAudioTracks();
    bool loadActorData();
    bool loadItemDefs();
    bool loadLocationsData();
    bool loadMapInfoData();
    bool loadMusicTracks();
    bool loadStringData();
    bool loadTileData();
    bool loadLootTables();
    bool loadWorldData();

    int framecount, framerate, baseticks, lastticks, fps;
    int timerFrames, timerTime, actualFPS;
    double tickrate;

    std::string gameName;
    int gameId, majorVersion, minorVersion;
};

class Font {
public:
    Font(SDL_Renderer *renderer, SDL_Texture *texture);
    ~Font();
    void setMetrics(int width, int height, int linespace);
    void setScale(int scale);
    int getCharWidth() const;
    int getCharHeight() const;
    int getLineHeight() const;
    void out(int x, int y, const std::string &text, const Color &defaultColor = Color{255,255,255});
private:
    SDL_Renderer *mRenderer;
    SDL_Texture *mTexture;
    int mScale, mCharWidth, mCharHeight, mLineSpace;
};

const int charStats = 0;
const int charAbilities = 1;
const int charInventory = 2;
const int charModeCount = 3;

char* slurpFile(const std::string &filename);

bool repaint(System &state, const AnimFrame *frame = nullptr, bool callPresent = true);

void doCharInfo(System &system);
void doCredits(System &state);
void doGameMenu(System &state);
void doShowMap(System &system);

bool gfx_Confirm(System &state, const std::string &line1, const std::string &line2, bool defaultResult = true);
void gfx_Alert(System &state, const std::string &line1, const std::string &line2);
bool gfx_EditText(System &system, const std::string &prompt, std::string &text, int maxLength);
void gfx_DrawTooltip(System &system, int x, int y, const std::string &text);

bool pointInBox(int x, int y, const SDL_Rect &box);
int gfx_DrawFrame(System &system, int x, int y, int w, int h, const std::string &title);
void gfx_DrawBar(System &system, int x, int y, int length, int height, double percent, const Color &baseColor);
void gfx_DrawButton(System &system, const SDL_Rect &box, bool selected, const std::string &text);
void gfx_HLine(System &system, int x, int y, int y2, const Color &color);
void gfx_VLine(System &system, int x, int x2, int y, const Color &color);

#endif
