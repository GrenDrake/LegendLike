#ifndef GFX_H
#define GFX_H

#include <SDL2/SDL_mixer.h>
#include <map>
#include <string>
#include <vector>

class Board;
class Creature;
class Random;
class GameState;
struct SDL_Renderer;
struct SDL_Window;
struct SDL_Texture;
class Font;
class VM;
class ResourceManager;
struct Mix_Chunk;
class Config;
struct SDL_Keysym;

const int quickSlotUnused = 0;
const int quickSlotAbility = 1;
const int quickSlotItem = 2;
const int quickSlotCount = 4;

struct Color {
    int r, g, b;
};

struct TrackInfo {
    int number;
    std::string file;
    std::string name;
    std::string artist;
};

struct QuickSlot {
    int type;
    int action;
};

struct Message {
    int newTurns;
    std::string text;
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
    SDL_Texture* getTile(unsigned index);

    void playMusic(int trackNumber);
    void setMusicVolume(int volume);
    int getTrackNumber() const;
    const TrackInfo& getTrackInfo();

    void playEffect(int effectNumber);
    void setAudioVolume(int volume);

    void addMessage(const std::string &text);
    void appendMessage(const std::string &newText);

    void requestTick();
    bool hasTick() const;
    void tick();

    void setTarget(int targetFps);
    void waitFrame();
    int getActualFps();

    Board* getBoard() {
        return mCurrentBoard;
    }
    Creature* getPlayer() {
        return mPlayer;
    }
    bool warpTo(int boardIndex, int x, int y);
    bool down();
    bool up();
    bool build(int forIndex);

    // message log
    std::vector<Message> messages;

    // Map data
    int turnNumber;
    int depth;
    Board *mCurrentBoard;
    Creature *mPlayer;
    std::map<int, Board*> mBoards;
    QuickSlot quickSlots[quickSlotCount];

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
    std::map<int, std::string> strings;

    // system modules
    SDL_Renderer *renderer;
    SDL_Window *window;
    Random &coreRNG;
    VM *vm;
    Config *config;

    // game configuration flags
    bool wantsToQuit;
    bool showTooltip;
    bool showInfo;
    bool showFPS;
    bool wantsTick;

private:
    bool loadCreatureData();
    bool loadMoveData();
    bool loadStringData();
    bool loadTileData();

    int framecount, framerate, baseticks, lastticks, fps;
    int timerFrames, timerTime, actualFPS;
    double tickrate;
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

void repaint(System &state, bool callPresent = true);
void gfx_frameDelay(System &state);

void doCharInfo(System &system, int initialMode);
void doCredits(System &state);
void doGameMenu(System &state);
void doShowMap(System &system);

void gfx_MessageBox(System &state, std::string text);
void gfx_DrawTooltip(System &system, int x, int y, const std::string &text);
void gfx_DrawFrame(System &system, int x, int y, int w, int h);
void gfx_DrawBar(System &system, int x, int y, int length, int height, double percent, const Color &baseColor);
bool gfx_EditText(System &system, const std::string &prompt, std::string &text, int maxLength);
void gfx_DrawButton(System &system, int x, int y, int w, int h, bool selected, const std::string &text);
int keyToIndex(const SDL_Keysym &key);
void gfx_Clear(System &system);
void gfx_DrawRect(System &system, int x, int y, int x2, int y2, const Color &color);
void gfx_FillRect(System &system, int x, int y, int x2, int y2, const Color &color);
void gfx_HLine(System &system, int x, int y, int y2, const Color &color);
void gfx_VLine(System &system, int x, int x2, int y, const Color &color);

#endif
