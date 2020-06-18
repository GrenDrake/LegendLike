#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#include <physfs.h>
#include "physfsrwops.h"
#include "gamestate.h"
#include "logger.h"


const TrackInfo noTrack{-1, "", "no track playing", ""};


SDL_Texture* System::getImageCore(const std::string &name) {
    Logger &log = Logger::getInstance();
    auto previous = mImages.find(name);
    if (previous != mImages.end()) return previous->second;

    auto fp = PHYSFS_openRead(name.c_str());
    if (!fp) {
        auto err = PHYSFS_getLastErrorCode();
        log.error(std::string("Failed to load file ") + name + ": " + PHYSFS_getErrorByCode(err));
        return nullptr;
    }
    auto rwops = PHYSFSRWOPS_makeRWops(fp);

    SDL_Surface *surf = IMG_Load_RW(rwops, 0);
    rwops->close(rwops);
    if (!surf) {
        log.error(std::string("Failed to load image ") + name + ": " + IMG_GetError() + "\n");
        return nullptr;
    }

    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);

    if (!tex) {
        log.error(std::string("Error Creating image ") + name + ": " + SDL_GetError());
    }
    return tex;
}

SDL_Texture* System::getImage(const std::string &name) {
    std::string fullName("/gfx/");
    fullName += name;
    SDL_Texture *tex = getImageCore(fullName);
    return tex;
}

Mix_Music* System::getMusic(const std::string &name) {
    Logger &log = Logger::getInstance();

    std::string fullName("/music/");
    fullName += name;
    PHYSFS_file *file = PHYSFS_openRead(name.c_str());
    if (!file) {
        log.error(std::string("Error loading music ") + name + ": " + Mix_GetError());
        return nullptr;
    }
    SDL_RWops *ops = PHYSFSRWOPS_makeRWops(file);
    Mix_Music *music = Mix_LoadMUS_RW(ops, 1);
    if (!music) {
        log.error("Error loading music " + name + ": " + Mix_GetError());
    }
    return music;
}

Mix_Chunk* System::getAudio(const std::string &name) {
    Logger &log = Logger::getInstance();

    std::string fullName("/audio/");
    fullName += name;
    auto fp = PHYSFS_openRead(fullName.c_str());
    if (!fp) {
        auto err = PHYSFS_getLastErrorCode();
        log.error(std::string("Error to load audio file ") + name + ": " + PHYSFS_getErrorByCode(err));
        return nullptr;
    }
    auto rwops = PHYSFSRWOPS_makeRWops(fp);

    Mix_Chunk *surf = Mix_LoadWAV_RW(rwops, 0);
    rwops->close(rwops);
    if (!surf) {
        log.error(std::string("Error loadig audio ") + name + ": " + Mix_GetError());
    }
    return surf;
}

Font* System::getFont(const std::string &name) {
    auto previous = mFonts.find(name);
    if (previous != mFonts.end()) return previous->second;

    Font *font = nullptr;
    SDL_Texture *tex = getImage(name);
    if (tex)    font = new Font(renderer, tex);
    mFonts.insert(std::make_pair(name, font));
    return font;
}

void System::playMusic(int trackNumber) {
    if (mCurrentTrack == trackNumber) return;

    if (mCurrentMusic) {
        Mix_HaltMusic();
        mCurrentTrack = -1;
        Mix_FreeMusic(mCurrentMusic);
        mCurrentMusic = nullptr;
    }
    if (trackNumber < 0) return;

    auto iter = mTracks.find(trackNumber);
    if (iter != mTracks.end()) {
        mCurrentMusic = getMusic(iter->second.file);
        if (mCurrentMusic) {
            Mix_PlayMusic(mCurrentMusic, -1);
            mCurrentTrack = trackNumber;
        } else {
            Logger &log = Logger::getInstance();
            log.error("Failed to load track " + iter->second.file);
        }
    } else {
        Logger &log = Logger::getInstance();
        log.error("Tried to play non-existant track " + std::to_string(trackNumber));
    }
}

void System::setMusicVolume(int volume) {
    Mix_VolumeMusic(volume);
}

int System::getTrackNumber() const {
    return mCurrentTrack;
}

const TrackInfo& System::getTrackInfo() {
    auto iter = mTracks.find(mCurrentTrack);
    if (iter != mTracks.end()) return iter->second;
    return noTrack;
}

void System::playEffect(int effectNumber) {
    auto iter = mAudio.find(effectNumber);
    if (iter != mAudio.end()) {
        Mix_PlayChannel(-1, iter->second, 0);
    }
}

void System::setAudioVolume(int volume) {
    Mix_Volume(-1, volume);
}
