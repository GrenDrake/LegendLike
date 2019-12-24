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
const int TYPE_ID       = 0x45505954;

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
        if (!vm->loadFromFile("game.dat")) {
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
                    std::vector<std::string> result = explode(*trackIter, "|");
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

        // LOAD BEAST TYPES
        VM beasts;
        if (!beasts.loadFromFile("beasts.dat")) {
            log.error("Failed to load beast info.");
            return false;
        }
        const int beastSize = 66;
        int baseAddress = beasts.getExport("beast_info");
        if (baseAddress == -1) {
            log.error("Could not find beast_info in beasts.dat");
        } else {
            int ident = beasts.readWord(baseAddress);
            while (ident > 0) {
                CreatureType type;
                type.ident = ident;
                type.name = beasts.readString(baseAddress + 4);
                type.artIndex = beasts.readWord(baseAddress + 36);
                type.defaultMove = beasts.readWord(baseAddress + 40);
                int offset = baseAddress + 44;
                for (int j = 0; j < statCount; ++j) {
                    type.stats[j] = beasts.readShort(offset);
                    offset += 2;
                }
                for (int j = 0; j < damageTypeCount; ++j) {
                    int value = beasts.readByte(offset);
                    type.resistances[j] = value / 100.0;
                    offset += 1;
                }

                CreatureType::add(type);

                baseAddress += beastSize;
                ident = beasts.readWord(baseAddress);
            }
        }
        log.info(std::string("Loaded ") + std::to_string(CreatureType::typeCount()) + " beasts.");

    } catch (VMError &e) {
        log.error(std::string("Failed to initialize VM: ") + e.what());
        return false;
    }

    // LOAD TILE TYPE DATA
    PHYSFS_file *fp = PHYSFS_openRead("tiles.dat");
    if (!fp) {
        log.error("Failed to open tile data file.");
        return 0;
    }
    PHYSFS_uint32 tileId = 0;
    PHYSFS_readULE32(fp, &tileId);
    if (tileId != TILE_ID) {
        log.error("tiles.dat has wrong datafile ID.");
    } else {
        PHYSFS_sint32 ident = 0;
        PHYSFS_readSLE32(fp, &ident);
        while (ident >= 0) {
            TileInfo info;
            char nameBuffer[32] = { 0 };
            PHYSFS_uint8 u8;
            PHYSFS_uint32 u32;

            info.index = ident;
            PHYSFS_readULE32(fp, &u32);     info.artIndex = u32;
            PHYSFS_readULE32(fp, &u32);     info.interactTo = u32;
            PHYSFS_readBytes(fp, &u8, 1);   info.red = u8;
            PHYSFS_readBytes(fp, &u8, 1);   info.green = u8;
            PHYSFS_readBytes(fp, &u8, 1);   info.blue = u8;
            PHYSFS_readBytes(fp, &u8, 1);   info.block_travel = u8;
            PHYSFS_readBytes(fp, &u8, 1);   info.block_los = u8;
            PHYSFS_readBytes(fp, nameBuffer, 32);
            info.name = nameBuffer;
            TileInfo::add(info);

            PHYSFS_readSLE32(fp, &ident);
        }
    }

    if (!loadMoveData())   return false;
    if (!loadTypeData())   return false;
    if (!loadStringData()) return false;
    return true;
}

bool System::loadMoveData() {
    Logger &log = Logger::getInstance();
    VM moves;
    if (!moves.loadFromFile("moves.dat")) {
        log.error("Failed to load move info.");
        return false;
    }

    const int moveSize = 22;
    int baseAddress = moves.getExport("move_info");
    if (baseAddress == -1) {
        log.error("Could not find move_info in moves.dat");
    } else {
        int ident = moves.readWord(baseAddress);
        while (ident > 0) {
            MoveType type;
            type.ident          = ident;

            int nameId          = moves.readWord(baseAddress + 4);
            auto nameStr = strings.find(nameId);
            if (nameStr == strings.end()) {
                type.name = "attack #" + std::to_string(ident);
            } else {
                type.name = nameStr->second;
            }

            type.accuracy       = moves.readByte(baseAddress + 8);
            type.speed          = moves.readByte(baseAddress + 9);
            type.cost           = moves.readByte(baseAddress + 10);

            type.type           = moves.readByte(baseAddress + 11);
            type.minRange       = moves.readByte(baseAddress + 12);
            type.maxRange       = moves.readByte(baseAddress + 13);

            type.damage         = moves.readByte(baseAddress + 14);
            type.damageSize     = moves.readByte(baseAddress + 15);
            type.damageShape    = moves.readByte(baseAddress + 16);
            type.damageType     = moves.readByte(baseAddress + 17);

            type.flags          = moves.readWord(baseAddress + 18);

            MoveType::add(type);
            baseAddress += moveSize;
            ident = moves.readWord(baseAddress);
        }
    }
    log.info(std::string("Loaded ") + std::to_string(MoveType::typeCount()) + " moves.");
    return true;
}


bool System::loadStringData() {
    Logger &log = Logger::getInstance();
    char *text = slurpFile("strings.dat");
    if (text == nullptr) return false;
    std::stringstream fulltext(text);
    std::string line;
    int lineNo = 0;
    while (std::getline(fulltext, line)) {
        ++lineNo;
        if (line.empty()) continue;
        std::string::size_type pos = line.find_first_of("=");
        if (pos == std::string::npos) {
            log.warn("strings.dat: malformed string def on line " + std::to_string(lineNo));
            continue;
        }
        std::string number = trim(line.substr(0, pos));
        std::string text = trim(line.substr(pos + 1));
        unescapeString(text);
        int textNo = strToInt(number);
        auto old = strings.find(textNo);
        if (old != strings.end()) {
            log.warn("String ID " + number + " used on line " + std::to_string(lineNo) + " was previously defined.");
        }
        strings[textNo] = text;
    }
    return true;
}

bool System::loadTypeData() {
    Logger &log = Logger::getInstance();
    PHYSFS_file *fp = PHYSFS_openRead("types.dat");
    if (!fp) {
        log.error("Failed to open type data file.");
        return false;
    }

    PHYSFS_uint32 typeId = 0;
    PHYSFS_readULE32(fp, &typeId);
    if (typeId != TYPE_ID) {
        log.error("types.dat has wrong datafile ID.");
        return false;
    }

    PHYSFS_sint8 count = 0;
    PHYSFS_readBytes(fp, &count, 1);
    if (count <= 0) {
        log.error("Number of types cannot be zero or less.");
        return false;
    }
    TypeInfo::setTypeCount(count);

    for (int i = 0; i < count; ++i) {
        char name[32] = { 0 };
        PHYSFS_readBytes(fp, name, 16);
        TypeInfo::setName(i, name);
    }

    for (int x = 0; x < count; ++x) {
        for (int y = 0; y < count; ++y) {
            PHYSFS_uint8 b = 0;
            PHYSFS_readBytes(fp, &b, 1);
            TypeInfo::setEffectiveness(y, x, b);
        }
    }

    PHYSFS_close(fp);
    log.info("Loaded " + std::to_string(count) + " types.");
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
