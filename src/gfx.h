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
struct SDL_Texture;
class Font;
class VM;
class ResourceManager;
struct Mix_Chunk;
class Config;


struct Color {
    int r, g, b;
};

struct TrackInfo {
    int number;
    std::string file;
    std::string name;
    std::string artist;
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

    SDL_Texture* getTile(unsigned index);

    void playMusic(int trackNumber);
    void setMusicVolume(int volume);
    int getTrackNumber() const;
    const TrackInfo& getTrackInfo();

    void playEffect(int effectNumber);
    void setAudioVolume(int volume);


    void requestTick();
    bool hasTick() const;
    void tick();

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
    std::vector<std::string> messages;

    // Map data
    int turnNumber;
    int depth;
    Board *mCurrentBoard;
    Creature *mPlayer;
    std::map<int, Board*> mBoards;

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
    Random &coreRNG;
    VM *vm;
    Config *config;

    // game configuration flags
    bool wantsToQuit;
    bool showInfo;
    bool showFPS;
    bool wantsTick;

    // fps management
    // we declared this as a void* to avoid needing to include the often
    // unneccesary header file or dealing with the typedef FPSManager
    void *fpsManager;
    int fps;

private:
    bool loadMoveData();
    bool loadStringData();
    bool loadTypeData();

};

class Font {
public:
    Font(SDL_Renderer *renderer, SDL_Texture *texture);
    ~Font();
    void setMetrics(int width, int height, int linespace);
    int getCharWidth() const;
    int getCharHeight() const;
    int getLineHeight() const;
    void out(int x, int y, const std::string &text, const Color &defaultColor = Color{255,255,255});
private:
    SDL_Renderer *mRenderer;
    SDL_Texture *mTexture;
    int mCharWidth, mCharHeight, mLineSpace;
};

const int portraitWidth = 128;
const int portraitHeight = 128;
const int portraitOffset = 48;

const int portraitNone = 0;
const int portraitLeft = 1;
const int portraitCentre = 2;
const int portraitRight = 3;

const int symbolNone = 0;
const int symbolExclaim = 1;
const int symbolQuestion = 2;

void repaint(System &state, bool callPresent = true);
void gfx_frameDelay(System &state);
void showFullMap(System &renderState);
void doPartyScreen(System &system);

void doGameMenu(System &state);
void doCredits(System &state);

void gfx_MessageBox(System &state, std::string text, const std::string &portrait, int portraitSide);
void gfx_DrawFrame(System &system, int x, int y, int w, int h);
void gfx_DrawBar(System &system, int x, int y, int length, int height, double percent, const Color &baseColor);
bool gfx_EditText(System &system, const std::string &prompt, std::string &text, int maxLength);

#endif
