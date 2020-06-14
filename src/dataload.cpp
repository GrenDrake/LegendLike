#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#include "board.h"
#include "creature.h"
#include "vm.h"
#include "physfs.h"
#include "logger.h"
#include "textutil.h"
#include "gfx.h"
#include "physfsrwops.h"

const int END_MARKER    = -1;
const int MAP_INFO_SIZE = 42;
const int TILE_ID       = 0x454C4954;
const int MOVE_ID       = 0x45564F4D;

static bool fileHasExtension(const std::string &filename, const std::vector<std::string> &list) {
    std::string::size_type pos = filename.find_last_of('.');
    if (pos == std::string::npos) return false;
    std::string ext = filename.substr(pos);
    for (const std::string &iter : list) {
        if (iter == ext) return true;
    }
    return false;
}

static int getFileIndexNumber(const std::string &filename) {
    std::string::size_type pos = filename.find_first_of(" .");
    if (pos == std::string::npos) return -1;
    std::string num = filename.substr(0, pos);
    return strToInt(num);
}

char* slurpFile(const std::string &filename) {
    Logger &log = Logger::getInstance();
    PHYSFS_file *tilesDat = PHYSFS_openRead(filename.c_str());
    if (!tilesDat) {
        const char *errMsg = PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
        log.error(std::string("Failed to open file ") + filename + ": " + errMsg);
        return nullptr;
    }
    PHYSFS_sint64 length = PHYSFS_fileLength(tilesDat);
    char *buffer = new char[length + 1];
    buffer[length] = 0;
    PHYSFS_readBytes(tilesDat, buffer, length);
    PHYSFS_close(tilesDat);
    return buffer;
}

bool System::load() {
    Logger &log = Logger::getInstance();

    try {
        if (!vm->loadFromFile("game.dat", true)) {
            log.error("Failed to load VM image.");
            return false;
        }

        // LOAD TILE IMAGES
        char **tileArt = PHYSFS_enumerateFiles("/gfx/tiles");
        char **tileIter = tileArt;
        for (; *tileIter != nullptr; ++tileIter) {
            std::string filename = "/gfx/tiles/";
            filename += *tileIter;
            if (!fileHasExtension(filename, {".png",".jpg",".bmp",".tga",".tif"})) {
                log.info("File " + filename + " does not appear to be an image file.");
                continue;
            }
            int index = getFileIndexNumber(*tileIter);
            if (mTiles.find(index) != mTiles.end()) {
                log.error("Attempted to load as tile " + std::to_string(index) + " \"" + filename + "\", but that tile number was already in use.");
            } else {
                SDL_Texture *t = getImageCore(filename);
                if (!t) {
                    std::stringstream line;
                    line << "Failed creating tile " << index << ": ";
                    line << SDL_GetError();
                    log.error(line.str());
                } else {
                    mTiles.insert(std::make_pair(index, t));
                }
            }
        }
        PHYSFS_freeList(tileArt);
        log.info("Loaded " + std::to_string(mTiles.size()) + " tiles.");

        // LOAD MUSIC TRACKS
        char **musicTracks = PHYSFS_enumerateFiles("/music");
        char **trackIter = musicTracks;
        for (; *trackIter != nullptr; ++trackIter) {
            std::string filename = "/music/";
            filename += *trackIter;
            if (!fileHasExtension(filename, {".ogg",".wav",".mp3"})) {
                log.info("File " + filename + " does not appear to be an music file.");
                continue;
            }
            int index = getFileIndexNumber(*trackIter);
            if (index < 0) {
                log.error("Invalid index " + std::to_string(index) + " for track " + filename + ".");
            } else {
                if (mTracks.find(index) != mTracks.end()) {
                    log.error("Attempted to load as track " + std::to_string(index) + " \"" + filename + "\", but that track number was already in use.");
                } else {
                    std::vector<std::string> result = explode(*trackIter, "~");
                    TrackInfo info = { index, filename };
                    if (result.size() != 3) {
                        log.warn(std::string("Track has malformed name: ") + *trackIter);
                        info.name = filename;
                        info.artist = "unknown";
                        mTracks.insert(std::make_pair(index, info));
                    } else {
                        info.name = result[1];
                        info.artist = result[2];
                        mTracks.insert(std::make_pair(index, info));
                    }
                }
            }
        }
        PHYSFS_freeList(musicTracks);
        log.info("Loaded " + std::to_string(mTracks.size()) + " tracks.");

        // LOAD AUDIO EFFECTS
        char **audioEffects = PHYSFS_enumerateFiles("/audio");
        char **audioIter = audioEffects;
        for (; *audioIter != nullptr; ++audioIter) {
            std::string filename = *audioIter;
            if (!fileHasExtension(filename, {".ogg",".wav",".aiff"})) {
                log.info("File " + filename + " does not appear to be an audio file.");
                continue;
            }
            if (filename.substr(filename.length() - 4) != ".ogg") continue;
            int index = getFileIndexNumber(*audioIter);
            if (mAudio.find(index) != mAudio.end()) {
                log.error("Attempted to load as effect " + std::to_string(index) + " \"" + filename + "\", but that effect number was already in use.");
            } else {
                Mix_Chunk *a = getAudio(filename);
                if (!a) {
                    std::stringstream line;
                    line << "Failed loading track : ";
                    line << Mix_GetError();
                    log.error(line.str());
                } else {
                    mAudio.insert(std::make_pair(index, a));
                }
            }
        }
        PHYSFS_freeList(audioEffects);
        log.info("Loaded " + std::to_string(mAudio.size()) + " audio effects.");

        // LOAD MAP INFO
        const int mapBase = vm->getExport("map_info");
        if (mapBase < 0) {
            log.error("VM image lacks map info.");
            return false;
        }
        int counter = 0;
        while (1) {
            int base = mapBase + counter * MAP_INFO_SIZE;
            int ident = vm->readShort(base);
            if (ident == END_MARKER) break;
            int width = vm->readShort(base + 2);
            int height = vm->readShort(base + 4);
            int func = vm->readWord(base + 6);
            int onEnter = vm->readWord(base + 10);
            unsigned flags = vm->readWord(base + 14);
            int track = vm->readWord(base + 18);
            std::string name = vm->readString(base + 22);
            MapInfo::add(MapInfo{ident, width, height, func, onEnter, flags, track, name});
            ++counter;
        }
        log.info("Loaded " + std::to_string(counter) + " maps.");


    } catch (VMError &e) {
        log.error(std::string("Failed to initialize VM: ") + e.what());
        return false;
    }

    try {
        if (!loadStringData())      return false;
        if (!loadCreatureData())    return false;
        if (!loadLocationsData())        return false;
        if (!loadMoveData())        return false;
        if (!loadTileData())        return false;
    } catch (VMError &e) {
        log.error(e.what());
        return false;
    }

    subweapons.push_back(Subweapon{ "bow",      "ui/bow.png",      true });
    subweapons.push_back(Subweapon{ "hookshot", "ui/hookshot.png", true });
    subweapons.push_back(Subweapon{ "bomb",     "ui/bomb.png",     true });
    subweapons.push_back(Subweapon{ "mattock",  "ui/mattock.png",  true });
    subweapons.push_back(Subweapon{ "firerod",  "ui/firerod.png",  true });
    subweapons.push_back(Subweapon{ "icerod",   "ui/icerod.png",   true });
    return true;
}

bool System::loadCreatureData() {
    Logger &log = Logger::getInstance();
    const unsigned npcTypeSize = 28;
    unsigned npcTypesAddr = vm->getExport("__npctypes");
    if (npcTypesAddr == static_cast<unsigned>(-1)) {
        log.error("NPC types data not found.");
        return false;
    }
    const unsigned npcTypesCount = vm->readWord(npcTypesAddr);
    npcTypesAddr += 4;
    for (unsigned counter = 0; counter < npcTypesCount; ++counter) {
        CreatureType type;
        int nameAddr = vm->readWord(npcTypesAddr + counter * npcTypeSize);
        type.ident = counter;
        if (nameAddr) type.name = vm->readString(nameAddr);
        type.artIndex = vm->readWord(npcTypesAddr + counter * npcTypeSize + 4);
        type.maxHealth = vm->readWord(npcTypesAddr + counter * npcTypeSize + 8);
        type.maxEnergy = vm->readWord(npcTypesAddr + counter * npcTypeSize + 12);
        type.accuracy = vm->readWord(npcTypesAddr + counter * npcTypeSize + 16);
        type.evasion = vm->readWord(npcTypesAddr + counter * npcTypeSize + 20);
        type.moveRate = vm->readWord(npcTypesAddr + counter * npcTypeSize + 24);
        CreatureType::add(type);
    }
    log.info(std::string("Loaded ") + std::to_string(CreatureType::typeCount()) + " npc types.");
    return true;
}

bool System::loadLocationsData() {
    Logger &log = Logger::getInstance();
    const unsigned locationSize = 4;
    unsigned locationsAddr = vm->getExport("__locations");
    if (locationsAddr == static_cast<unsigned>(-1)) {
        log.error("Locations data not found.");
        return false;
    }
    const unsigned locationCount = vm->readWord(locationsAddr);
    locationsAddr += 4;
    for (unsigned counter = 0; counter < locationCount; ++counter) {
        int itemId = vm->readWord(locationsAddr + counter * locationSize);
        itemLocations.push_back(ItemLocation{itemId});
    }
    log.info(std::string("Loaded ") + std::to_string(itemLocations.size()) + " item locations.");
    return true;
}

bool System::loadMoveData() {
    Logger &log = Logger::getInstance();
    VM moves;
    if (!moves.loadFromFile("moves.dat", false)) {
        log.error("Failed to load move info.");
        return false;
    }

    int magicWord = moves.readWord();
    if (magicWord != MOVE_ID) {
        log.error("File moves.dat is corrupt or invalid.");
        return false;
    }

    int ident = moves.readWord();
    while (ident > 0) {
        MoveType type;
        type.ident          = ident;
        int nameId          = moves.readWord();
        auto nameStr = strings.find(nameId);
        if (nameStr == strings.end()) {
            type.name = "attack #" + std::to_string(ident);
        } else {
            type.name = nameStr->second;
        }

        type.accuracy       = moves.readWord();
        type.speed          = moves.readWord();
        type.cost           = moves.readWord();

        type.type           = moves.readWord();
        type.minRange       = moves.readWord();
        type.maxRange       = moves.readWord();

        type.damage         = moves.readWord();
        type.damageSize     = moves.readWord();
        type.shape          = moves.readWord();
        type.form           = moves.readWord();

        type.player_use     = moves.readWord();
        type.other_use      = moves.readWord();
        type.damageIcon     = moves.readWord();
        type.flags          = moves.readWord();

        MoveType::add(type);
        ident = moves.readWord();
    }

    log.info("Loaded " + std::to_string(MoveType::typeCount()) + " moves.");
    return true;
}


bool System::loadStringData() {
    Logger &log = Logger::getInstance();
    char *text = slurpFile("strings.dat");
    if (text == nullptr) return false;
    std::stringstream fulltext(text);
    delete[] text;
    std::string line;
    int lineNo = 0;
    while (std::getline(fulltext, line)) {
        ++lineNo;
        // skip blank lines and comments
        if (line.empty()) continue;
        std::string::size_type pos = line.find_first_not_of(" \t");
        if (pos == std::string::npos || line[pos] == ';') continue;
        // break lines into parts
        pos = line.find_first_of("=");
        if (pos == std::string::npos) {
            log.warn("strings.dat: malformed string def on line " + std::to_string(lineNo));
            continue;
        }
        std::string number = trim(line.substr(0, pos));
        std::string text = trim(line.substr(pos + 1));
        // save new string
        unescapeString(text);
        int textNo = strToInt(number);
        auto old = strings.find(textNo);
        if (old != strings.end()) {
            log.warn("String ID " + number + " used on line " + std::to_string(lineNo) + " was previously defined.");
        }
        strings[textNo] = text;
    }
    log.info("Loaded " + std::to_string(strings.size()) + " strings.");
    return true;
}

bool System::loadTileData() {
    Logger &log = Logger::getInstance();
    const unsigned tileDefSize = 32;
    unsigned tileDefsAddr = vm->getExport("__tiledefs");
    if (tileDefsAddr == static_cast<unsigned>(-1)) {
        log.error("TileDef data not found.");
        return false;
    }
    const unsigned tileDefsCount = vm->readWord(tileDefsAddr);
    tileDefsAddr += 4;
    for (unsigned counter = 0; counter < tileDefsCount; ++counter) {
        TileInfo tile;
        tile.index = counter;
        int nameAddr = vm->readWord(tileDefsAddr + counter * tileDefSize);
        if (nameAddr) tile.name = vm->readWord(nameAddr);
        tile.artIndex   = vm->readWord(tileDefsAddr + counter * tileDefSize + 4);
        tile.red        = vm->readWord(tileDefsAddr + counter * tileDefSize + 8);
        tile.green      = vm->readWord(tileDefsAddr + counter * tileDefSize + 12);
        tile.blue       = vm->readWord(tileDefsAddr + counter * tileDefSize + 16);
        tile.interactTo = vm->readWord(tileDefsAddr + counter * tileDefSize + 20);
        tile.animLength = vm->readWord(tileDefsAddr + counter * tileDefSize + 24);
        tile.flags      = vm->readWord(tileDefsAddr + counter * tileDefSize + 28);
        TileInfo::add(tile);
    }
    log.info(std::string("Loaded ") + std::to_string(TileInfo::types.size()) + " npc types.");
    return true;
}

void System::unloadAll() {
    for (auto iter : mTiles)          SDL_DestroyTexture(iter.second);
    for (auto iter : mImages)         SDL_DestroyTexture(iter.second);
    for (auto iter : mAudio)          Mix_FreeChunk(iter.second);
    for (auto iter : mFonts)          delete iter.second;
    if (mCurrentMusic)                Mix_FreeMusic(mCurrentMusic);

    mTiles.clear();
    mImages.clear();
    mAudio.clear();
    mFonts.clear();
    mCurrentMusic = nullptr;
    mCurrentTrack = -1;
}
