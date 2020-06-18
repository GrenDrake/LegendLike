#include <ctime>
#include <fstream>
#include <sstream>
#include <iostream>

#include <SDL2/SDL.h>
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

unsigned perSecondTimer(unsigned interval, void *param) {
    SDL_Event event;
    event.type = SDL_USEREVENT;
    event.user.type = SDL_USEREVENT;
    SDL_PushEvent(&event);
    return(interval);
}


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

    Logger &log = Logger::getInstance();
    printVersions();

    Config config;
    config.loadFromFile("/save/game.cfg");
    config.set("writeDir", prefDir);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0){
        log.error(std::string("SDL_Init Error: ") + SDL_GetError());
        return 1;
    }

    const int displayNum = config.getInt("display", 0);
    int defaultScreenWidth = 1920;
    int defaultScreenHeight = 1080;
    SDL_Rect bounds;
    SDL_GetDisplayUsableBounds(0, &bounds);
    if (bounds.w < defaultScreenWidth)  defaultScreenWidth = bounds.w * 0.9;
    if (bounds.h < defaultScreenHeight) defaultScreenHeight = bounds.h * 0.9;
    int initialXRes = config.getInt("xres", defaultScreenWidth);
    int initialYRes = config.getInt("yres", defaultScreenHeight);
    unsigned windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    if (config.getBool("fullscreen", false)) windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    if (config.getBool("maximized", false))  windowFlags |= SDL_WINDOW_MAXIMIZED;
    SDL_Window *win = SDL_CreateWindow(GAME_NAME,
                                       SDL_WINDOWPOS_CENTERED_DISPLAY(displayNum),
                                       SDL_WINDOWPOS_CENTERED_DISPLAY(displayNum),
                                       initialXRes, initialYRes,
                                       windowFlags);
    if (win == nullptr) {
        log.error(std::string("SDL_CreateWindow Error: ") + SDL_GetError());
        SDL_Quit();
        return 1;
    }

    unsigned rendererFlags = SDL_RENDERER_ACCELERATED;
    if (config.getBool("vsync", true)) {
        rendererFlags |= SDL_RENDERER_PRESENTVSYNC;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, rendererFlags);
    if (renderer == nullptr){
        log.error(std::string("SDL_CreateRenderer Error: ") + SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }
    SDL_RendererInfo renderInfo;
    if (SDL_GetRendererInfo(renderer, &renderInfo) != 0) {
        log.error(std::string("SDL_GetRenderInfo Error: ") + SDL_GetError());
    } else {
        log.info(std::string("Renderer is ") + renderInfo.name + ".");
    }

    int result = IMG_Init(IMG_INIT_PNG);
    if (!result) {
        log.error(std::string("Failed to initialize SDL_image: ") + IMG_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        log.error(std::string("Failed to initialize SDL_mixer: ") + Mix_GetError());
        IMG_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(win);
        SDL_Quit();
    }

// apply settings from configuration files
    Mix_VolumeMusic(config.getInt("music", MIX_MAX_VOLUME));
    Mix_Volume(-1, config.getInt("audio", MIX_MAX_VOLUME));

    SDL_AddTimer(1000, perSecondTimer, nullptr);

    Random coreRandom;
    coreRandom.seed(time(0));
    System renderState(renderer, coreRandom);
    renderState.window = win;
    renderState.config = &config;
    int returnCode = innerMain(renderState);
    config.writeToFile();
    log.endLog();

    Mix_CloseAudio();
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    PHYSFS_deinit();
    return returnCode;
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
        Logger &log = Logger::getInstance();
        log.error("Fatal error: failed to load game data.");
        return 1;
    }

    renderState.setFontScale(renderState.config->getInt("font_scale", 1));

    try {
        doGameMenu(renderState);
    } catch (GameError &e) {
        Logger &log = Logger::getInstance();
        log.error(std::string("Fatal Error: ") + e.what());
        return 1;
    }

    renderState.unloadAll();
    return 0;
}
