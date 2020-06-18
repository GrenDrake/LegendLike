#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#include "board.h"
#include "actor.h"
#include "vm.h"
#include "physfs.h"
#include "logger.h"
#include "textutil.h"
#include "gfx.h"
#include "physfsrwops.h"


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
        if (!vm->loadFromFile("game.dat", true))    return false;
        if (!loadAudioTracks())                     return false;
        if (!loadActorData())                    return false;
        if (!loadLocationsData())                   return false;
        if (!loadLootTables())                      return false;
        if (!loadMapInfoData())                     return false;
        if (!loadMusicTracks())                     return false;
        if (!loadTileData())                        return false;

        gameName = vm->readString(vm->readWord(4));
        gameId = vm->readWord(8);
        majorVersion = vm->readWord(12);
        minorVersion = vm->readWord(16);
    } catch (VMError &e) {
        log.error(e.what());
        return false;
    }

    subweapons.push_back(Subweapon{ "bow",      "ui/bow.png",      true });
    subweapons.push_back(Subweapon{ "hookshot", "ui/hookshot.png", true });
    subweapons.push_back(Subweapon{ "bomb",     "ui/bomb.png",     true });
    subweapons.push_back(Subweapon{ "pickaxe",  "ui/pickaxe.png",  true });
    subweapons.push_back(Subweapon{ "firerod",  "ui/firerod.png",  true });
    subweapons.push_back(Subweapon{ "icerod",   "ui/icerod.png",   true });
    return true;
}

bool System::loadMusicTracks() {
    Logger &log = Logger::getInstance();
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
    return true;
}

bool System::loadAudioTracks() {
    Logger &log = Logger::getInstance();
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
    return true;
}

bool System::loadMapInfoData() {
    Logger &log = Logger::getInstance();
    unsigned mapBase = vm->getExport("__mapdata");
    if (mapBase < 0) {
        log.error("VM image lacks map info.");
        return false;
    }
    const unsigned mapDataSize = 36;
    int mapCount = vm->readWord(mapBase);
    unsigned counter = 0;
    mapBase += 4;
    for (int i = 0; i < mapCount; ++i) {
        MapInfo mapInfo;
        int nameAddr = vm->readWord(mapBase);
        if (nameAddr) mapInfo.name = vm->readString(nameAddr);
        mapInfo.index = vm->readWord(mapBase + 4);
        mapInfo.width = vm->readWord(mapBase + 8);
        mapInfo.height = vm->readWord(mapBase + 12);
        mapInfo.onBuild = vm->readWord(mapBase + 16);
        mapInfo.onEnter = vm->readWord(mapBase + 20);
        mapInfo.onReset = vm->readWord(mapBase + 24);
        mapInfo.musicTrack = vm->readWord(mapBase + 28);
        mapInfo.flags = vm->readWord(mapBase + 32);

        MapInfo::add(mapInfo);
        ++counter;
        mapBase += mapDataSize;
    }
    log.info("Loaded " + std::to_string(counter) + " maps.");
    return true;
}

bool System::loadActorData() {
    Logger &log = Logger::getInstance();
    const unsigned npcTypeSize = 44;
    unsigned npcTypesAddr = vm->getExport("__npctypes");
    if (npcTypesAddr == static_cast<unsigned>(-1)) {
        log.error("NPC types data not found.");
        return false;
    }
    const unsigned npcTypesCount = vm->readWord(npcTypesAddr);
    npcTypesAddr += 4;
    for (unsigned counter = 0; counter < npcTypesCount; ++counter) {
        ActorType type;
        int nameAddr = vm->readWord(npcTypesAddr + counter * npcTypeSize);
        type.ident = counter;
        if (nameAddr) type.name    = vm->readString(nameAddr);
        int artAddr    = vm->readWord(npcTypesAddr + counter * npcTypeSize + 4);
        if (artAddr)  type.artFile = vm->readString(artAddr);
        type.aiType    = vm->readWord(npcTypesAddr + counter * npcTypeSize + 8);
        type.maxHealth = vm->readWord(npcTypesAddr + counter * npcTypeSize + 12);
        type.maxEnergy = vm->readWord(npcTypesAddr + counter * npcTypeSize + 16);
        type.damage    = vm->readWord(npcTypesAddr + counter * npcTypeSize + 20);
        type.accuracy  = vm->readWord(npcTypesAddr + counter * npcTypeSize + 24);
        type.evasion   = vm->readWord(npcTypesAddr + counter * npcTypeSize + 28);
        type.moveRate  = vm->readWord(npcTypesAddr + counter * npcTypeSize + 32);
        type.lootType  = vm->readWord(npcTypesAddr + counter * npcTypeSize + 36);
        type.loot      = vm->readWord(npcTypesAddr + counter * npcTypeSize + 40);
        if (!type.artFile.empty()) type.art = getImage("actors/" + type.artFile + ".png");
        ActorType::add(type);
    }
    log.info(std::string("Loaded ") + std::to_string(ActorType::typeCount()) + " npc types.");
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

bool System::loadLootTables() {
    Logger &log = Logger::getInstance();
    unsigned lootTablesAddr = vm->getExport("__loottables");
    if (lootTablesAddr == static_cast<unsigned>(-1)) {
        log.error("Loot table data not found.");
        return false;
    }
    const unsigned lootTableCount = vm->readWord(lootTablesAddr);
    lootTablesAddr += 4;
    for (unsigned counter = 0; counter < lootTableCount; ++counter) {
        int rowCount = vm->readWord(lootTablesAddr);
        lootTablesAddr += 4;
        LootTable table;
        for (int i = 0; i < rowCount; ++i) {
            int chance = vm->readWord(lootTablesAddr);
            int itemId = vm->readWord(lootTablesAddr + 4);
            lootTablesAddr += 8;
            table.rows.push_back(LootRow{chance, itemId});
        }
        lootTables.push_back(table);
    }
    log.info(std::string("Loaded ") + std::to_string(lootTables.size()) + " loot tables.");
    return true;
}

bool System::loadTileData() {
    Logger &log = Logger::getInstance();
    const unsigned tileDefSize = 36;
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
        if (nameAddr) tile.name    = vm->readString(nameAddr);
        tile.group      = vm->readWord(tileDefsAddr + counter * tileDefSize + 4);
        int artAddr     = vm->readWord(tileDefsAddr + counter * tileDefSize + 8);
        if (artAddr)  tile.artFile = vm->readString(artAddr);
        tile.red        = vm->readWord(tileDefsAddr + counter * tileDefSize + 12);
        tile.green      = vm->readWord(tileDefsAddr + counter * tileDefSize + 16);
        tile.blue       = vm->readWord(tileDefsAddr + counter * tileDefSize + 20);
        tile.interactTo = vm->readWord(tileDefsAddr + counter * tileDefSize + 24);
        tile.animLength = vm->readWord(tileDefsAddr + counter * tileDefSize + 28);
        tile.flags      = vm->readWord(tileDefsAddr + counter * tileDefSize + 32);
        if (!tile.artFile.empty()) tile.art = getImage("tiles/" + tile.artFile + ".png");
        TileInfo::add(tile);
    }
    log.info(std::string("Loaded ") + std::to_string(TileInfo::types.size()) + " tiles.");
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
