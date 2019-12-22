#include <ctime>
#include <fstream>
#include <sstream>
#include <iostream>

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL2_framerate.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#include "physfs.h"

#include "game.h"
#include "gfx.h"
#include "vm.h"
#include "random.h"
#include "config.h"
#include "logger.h"

bool printVersions();
int innerMain(System &renderState);

const int defaultScreenWidth = 1366;
const int defaultScreenHeight = 768;

std::string versionString() {
    std::string text = GAME_NAME;
    text += ' ';
    text += std::to_string(GAME_MAJOR);
    text += '.';
    text += std::to_string(GAME_MINOR);
    text += '.';
    text += std::to_string(GAME_PATCH);
    return text;
}

int main(int argc, char *argv[]) {
    if (!PHYSFS_init(argv[0])) {
        auto err = PHYSFS_getLastErrorCode();
        std::cerr << "Failed to initialize PHYSFS:" << PHYSFS_getErrorByCode(err) << "\n";
        return 1;
    }

    const char *prefDir = PHYSFS_getPrefDir("grenslair", GAME_NAME);
    std::string baseDir = PHYSFS_getBaseDir();
    PHYSFS_setWriteDir(prefDir);
    PHYSFS_mount((baseDir+"data").c_str(), "/", 0);
    PHYSFS_mount(baseDir.c_str(), "/root", 1);
    PHYSFS_mount(prefDir, "/save", 1);
    std::cerr << GAME_NAME " write directory: " << prefDir << "\n";

    Logger &log = Logger::getInstance();
    printVersions();

    Config config;
    config.loadFromFile("/root/game.cfg");
    config.loadFromFile("/save/game.cfg");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0){
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    int initialXRes = config.getInt("xres", defaultScreenWidth);
    int initialYRes = config.getInt("yres", defaultScreenHeight);
    int displayNum = config.getInt("display", 0);
    unsigned flags = SDL_WINDOW_SHOWN;
    if (config.getSBool("fullscreen", false)) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    SDL_Window *win = SDL_CreateWindow(GAME_NAME,
                                       SDL_WINDOWPOS_CENTERED_DISPLAY(displayNum),
                                       SDL_WINDOWPOS_CENTERED_DISPLAY(displayNum),
                                       initialXRes, initialYRes,
                                       flags);
    if (win == nullptr){
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr){
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }
    // SDL_RenderSetLogicalSize(renderer, 960, 540);
    double scaleX = initialXRes / static_cast<double>(screenWidth);
    double scaleY = initialYRes / static_cast<double>(screenHeight);
    log.info("Setting render scale: " + std::to_string(scaleX) + "x" + std::to_string(scaleY));
    SDL_RenderSetScale(renderer, scaleX, scaleY);
    // "nearest" "linear" "best" (direct3d only)
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.

    int result = IMG_Init(IMG_INIT_PNG);
    if (!result) {
        std::cerr << "Failed to initialize SDL_image: " << IMG_GetError() << "\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Failed to initialize SDL_mixer: " << Mix_GetError() << "\n";
        IMG_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(win);
        SDL_Quit();
    }

// apply settings from configuration files
    Mix_VolumeMusic(config.getInt("music", MIX_MAX_VOLUME));
    Mix_Volume(-1, config.getInt("audio", MIX_MAX_VOLUME));

    Random coreRandom;
    coreRandom.seed(time(0));
    System renderState(renderer, coreRandom);
    renderState.config = &config;
    innerMain(renderState);
    config.writeToFile();
    log.endLog();

    Mix_CloseAudio();
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    PHYSFS_deinit();
    return 0;
}

bool printVersions() {
    Logger &log = Logger::getInstance();
    PHYSFS_Version physCompiled, physLinked;
    SDL_version compiled, linked;

    log.info(versionString());

    std::stringstream versionText;
    PHYSFS_VERSION(&physCompiled);
    PHYSFS_getLinkedVersion(&physLinked);
    versionText << "PHYSFS: (compiled)/";
    versionText << static_cast<int>(physCompiled.major) << '.' << static_cast<int>(physCompiled.minor) << '.' << static_cast<int>(physCompiled.patch);
    versionText << "  (linked)/";
    versionText << static_cast<int>(physLinked.major) << '.' << static_cast<int>(physLinked.minor) << '.' << static_cast<int>(physLinked.patch);
    log.info(versionText.str());
    versionText.str(""); versionText.clear();

    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);
    versionText << "SDL version: (compiled)/";
    versionText << static_cast<int>(compiled.major) << '.' << static_cast<int>(compiled.minor) << '.' << static_cast<int>(compiled.patch);
    versionText << "  (linked)/";
    versionText << static_cast<int>(linked.major) << '.' << static_cast<int>(linked.minor) << '.' << static_cast<int>(linked.patch);
    log.info(versionText.str());
    versionText.str(""); versionText.clear();

    SDL_IMAGE_VERSION(&compiled);
    linked = *IMG_Linked_Version();
    versionText << "SDL_image version: (compiled)/";
    versionText << static_cast<int>(compiled.major) << '.' << static_cast<int>(compiled.minor) << '.' << static_cast<int>(compiled.patch);
    versionText << "  (linked)/";
    versionText << static_cast<int>(linked.major) << '.' << static_cast<int>(linked.minor) << '.' << static_cast<int>(linked.patch);
    log.info(versionText.str());
    versionText.str(""); versionText.clear();

    SDL_MIXER_VERSION(&compiled);
    linked = *Mix_Linked_Version();
    versionText << "SDL_mixer version: (compiled)/";
    versionText << static_cast<int>(compiled.major) << '.' << static_cast<int>(compiled.minor) << '.' << static_cast<int>(compiled.patch);
    versionText << "  (linked)/";
    versionText << static_cast<int>(linked.major) << '.' << static_cast<int>(linked.minor) << '.' << static_cast<int>(linked.patch);
    log.info(versionText.str());
    versionText.str(""); versionText.clear();

    return true;
}

int innerMain(System &renderState) {

    VM vm;
    renderState.vm = &vm;
    vm.setSystem(&renderState);

    renderState.smallFont = renderState.getFont("medfont.png");
    if (!renderState.smallFont) return 1;
    renderState.smallFont->setMetrics(12, 24, 1);

    renderState.tinyFont = renderState.getFont("smallfont.png");
    if (!renderState.tinyFont) return 1;
    renderState.tinyFont->setMetrics(9, 18, 1);

    if (!renderState.load()) {
        return 1;
    }

    FPSmanager fpsManager;
    SDL_initFramerate(&fpsManager);
    SDL_setFramerate(&fpsManager, renderState.config->getInt("fps", 60));
    renderState.fpsManager = &fpsManager;

    try {
        doGameMenu(renderState);
    } catch (GameError &e) {
        std::cerr << "Fatal Error: " << e.what() << "\n";
    }

    renderState.unloadAll();
    return 0;
}
