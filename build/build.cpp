#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "assemble.h"

bool buildHeader(Program &code);

int main(int argc, char *argv[]) {
    bool doDumpInternals = false;
    std::vector<std::string> files;
    std::string outputFile = "output.bin";
    Program code;
    code.exports.push_back("__locations");
    code.exports.push_back("__npctypes");
    code.exports.push_back("__tiledefs");
    code.exports.push_back("__mapdata");
    code.exports.push_back("__loottables");
    code.exports.push_back("__itemdefs");
    code.exports.push_back("__worlds");

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg[0] == '-') {
            if (arg == "-dump") {
                doDumpInternals = true;
                code.doTokenDump = true;
            } else if (arg == "-o") {
                ++i;
                if (i >= argc) {
                    std::cerr << "Expected name of output file after -o\n";
                    return 1;
                }
                outputFile = argv[i];
            } else {
                std::cerr << "Unknown option " + arg + "\n";
                return 1;
            }
        } else {
            files.push_back(arg);
        }
    }

    if (files.empty()) {
        std::cerr << "No source files specified.\n";
        return 1;
    }

    if (code.doTokenDump) clearTokenDump();
    for (const std::string &filename : files) {
        parseFile(filename, code.errorLog, code);
        if (!code.errorLog.errors.empty()) {
            dumpErrors(code);
            return 1;
        }
    }

    buildHeader(code);
    if (!code.errorLog.errors.empty()) {
        dumpErrors(code);
        return 1;
    }

    generate(code, outputFile);
    if (!code.errorLog.errors.empty()) {
        dumpErrors(code);
        return 1;
    }

    if (doDumpInternals) {
        dumpStrings(code);
        dumpCode(code.code);
        dumpSymbols(code);
        dumpPatches(code);
    }

    return 0;
}

bool buildHeader(Program &code) {
    std::vector<AsmLine*> stringTable;

    {
        // add header and initial data (like the string table) to asm code
        AsmLabel *headerTableLabel = new AsmLabel(Origin(), "__header");
        stringTable.push_back(headerTableLabel);
        AsmData *headerData = new AsmData(Origin(), 4);
        stringTable.push_back(headerData);
        headerData->data.push_back(Value{0x004D5654});
        headerData->data.push_back(code.gameName);
        headerData->data.push_back(code.gameId);
        headerData->data.push_back(code.gameMajorVersion);
        headerData->data.push_back(code.gameMinorVersion);
        while (headerData->data.size() < 12) {
            headerData->data.push_back(Value{0});
        }
        unsigned headerSize = headerData->data.size() * headerData->width;

        // build the exports table
        AsmLabel *exportsLabel = new AsmLabel(Origin(), "__exports");
        stringTable.push_back(exportsLabel);
        AsmData *exportsData = new AsmData(Origin(), 4);
        stringTable.push_back(exportsData);
        Value exportCount;
        exportCount.value = code.exports.size();
        exportsData->data.push_back(exportCount);
        for (const std::string &name : code.exports) {
            unsigned counter = 0;
            while (counter < 16) {
                unsigned word = 0;
                for (int i = 0; i < 4; i++) {
                    char c = 0;
                    if (counter + i < name.size()) {
                        c = name[counter + i];
                    }
                    word >>= 8;
                    word |= c << 24;
                }
                Value wordValue;
                wordValue.value = word;
                exportsData->data.push_back(wordValue);
                counter += 4;
            }
            exportsData->data.push_back(Value(name));
        }
        std::cerr << "Header size: " << headerSize + exportsData->data.size() * exportsData->width << "\n";
    }

    {
        // build the string table
        AsmLabel *stringTableLabel = new AsmLabel(Origin(), "__string_table");
        stringTable.push_back(stringTableLabel);
        unsigned stringTableSize = 0;
        for (auto iter : code.strings) {
            AsmLabel *label = new AsmLabel(iter.second.origin, iter.first);
            AsmData *data = new AsmData(iter.second.origin, 1, true);
            for (char c : iter.second.text) {
                data->data.push_back(Value{c});
            }
            data->data.push_back(Value{0});
            stringTableSize += data->data.size() * data->width;
            stringTable.push_back(label);
            stringTable.push_back(data);
        }
        std::cerr << "String table size: " << stringTableSize << "\n";
    }

    {
        // build the item defs table
        AsmLabel *itemDefsLabel = new AsmLabel(Origin(), "__itemdefs");
        stringTable.push_back(itemDefsLabel);
        AsmData *itemDefsCountData = new AsmData(Origin(), 4);
        stringTable.push_back(itemDefsCountData);
        Value itemDefCount(code.itemDefs.size());
        itemDefsCountData->data.push_back(itemDefCount);
        unsigned itemDefsSize = 4;
        int counter = 0;
        for (const ItemDef &itemDef : code.itemDefs) {
            code.addSymbol(itemDef.identifier, SymbolDef{itemDef.origin, Value{counter}});
            AsmData *data = new AsmData(itemDef.origin, 4);
            data->data.push_back(itemDef.name);
            data->data.push_back(itemDef.artFile);
            data->data.push_back(itemDef.itemId);
            stringTable.push_back(data);
            itemDefsSize += data->data.size() * data->width;
            ++counter;
        }
        std::cerr << "ItemDefs size: " << itemDefsSize << " (" << counter << " items)\n";
    }

    {
        // build the item locations table
        AsmLabel *locationsLabel = new AsmLabel(Origin(), "__locations");
        stringTable.push_back(locationsLabel);
        AsmData *locationsCountData = new AsmData(Origin(), 4);
        stringTable.push_back(locationsCountData);
        Value locationCount(code.locations.size());
        locationsCountData->data.push_back(locationCount);
        unsigned locationsSize = 4;
        int counter = 0;
        for (const ItemLocation &location : code.locations) {
            code.addSymbol(location.name, SymbolDef{location.origin, Value{counter}});
            AsmData *data = new AsmData(location.origin, 4);
            data->data.push_back(location.itemId);
            stringTable.push_back(data);
            locationsSize += data->data.size() * data->width;
            ++counter;
        }
        std::cerr << "Locations size: " << locationsSize << " (" << counter << " items)\n";
    }

    {
        // build the loot tables table
        AsmLabel *lootTablesLabel = new AsmLabel(Origin(), "__loottables");
        stringTable.push_back(lootTablesLabel);
        AsmData *lootTablesCountData = new AsmData(Origin(), 4);
        stringTable.push_back(lootTablesCountData);
        Value lootTablesCount(code.lootTables.size());
        lootTablesCountData->data.push_back(lootTablesCount);
        unsigned lootTablesSize = 4;
        int counter = 0;
        for (const LootTable &lootTable : code.lootTables) {
            code.addSymbol(lootTable.identifier, SymbolDef{lootTable.origin, Value{counter}});
            AsmData *data = new AsmData(lootTable.origin, 4);
            data->data.push_back(Value(lootTable.rows.size()));
            lootTablesSize += 4;
            for (const LootRow &row : lootTable.rows) {
                data->data.push_back(row.chance);
                data->data.push_back(row.itemId);
                lootTablesSize += 8;
            }
            stringTable.push_back(data);
            ++counter;
        }
        std::cerr << "Loot tables size: " << lootTablesSize << " (" << counter << " items)\n";
    }

    {
        // build the tile defs table
        AsmLabel *tiledefsLabel = new AsmLabel(Origin(), "__tiledefs");
        stringTable.push_back(tiledefsLabel);
        AsmData *tiledefsCountData = new AsmData(Origin(), 4);
        stringTable.push_back(tiledefsCountData);
        Value tiledefCount;
        tiledefCount.value = code.tileDefs.size();
        tiledefsCountData->data.push_back(tiledefCount);
        unsigned tiledefsSize = 4;
        int counter = 0;
        for (const TileDef &def : code.tileDefs) {
            code.addSymbol(def.identifier, SymbolDef{def.origin, Value{counter}});
            AsmData *data = new AsmData(def.origin, 4);
            data->data.push_back(def.name);
            data->data.push_back(def.group);
            data->data.push_back(def.artIndex);
            data->data.push_back(def.red);
            data->data.push_back(def.green);
            data->data.push_back(def.blue);
            data->data.push_back(def.interactTo);
            data->data.push_back(def.animLength);
            Value flags;
            flags.value = def.flags;
            data->data.push_back(Value{flags});
            stringTable.push_back(data);
            tiledefsSize += data->data.size() * data->width;
            ++counter;
        }
        std::cerr << "TileDefs size: " << tiledefsSize << " (" << counter << " items)\n";
    }

    for (const MapData &outer : code.mapData) {
        for (const MapData &inner : code.mapData) {
            if (outer.identifier == inner.identifier) continue;
            if (outer.mapId.value == inner.mapId.value) {
                std::stringstream msg;
                msg << "Map " << outer.identifier << " shares map id " << outer.mapId.value << " with ";
                msg << inner.identifier << '.';
                code.errorLog.add(outer.origin, msg.str());
            }
        }
    }

    {
        // build the map data table
        AsmLabel *mapDataLabel = new AsmLabel(Origin(), "__mapdata");
        stringTable.push_back(mapDataLabel);
        AsmData *mapDataCountData = new AsmData(Origin(), 4);
        stringTable.push_back(mapDataCountData);
        Value mapDataCount;
        mapDataCount.value = code.mapData.size();
        mapDataCountData->data.push_back(mapDataCount);
        unsigned mapDataSizeSize = 4;
        int counter = 0;
        for (const MapData &mapData : code.mapData) {
            code.addSymbol(mapData.identifier, SymbolDef{mapData.origin, mapData.mapId});
            AsmData *data = new AsmData(mapData.origin, 4);
            data->data.push_back(mapData.name);
            data->data.push_back(mapData.mapId);
            data->data.push_back(mapData.width);
            data->data.push_back(mapData.height);
            data->data.push_back(mapData.onBuild);
            data->data.push_back(mapData.onEnter);
            data->data.push_back(mapData.onReset);
            data->data.push_back(mapData.musicTrack);
            Value flags;
            flags.value = mapData.flags;
            data->data.push_back(flags);
            stringTable.push_back(data);
            mapDataSizeSize += data->data.size() * data->width;
            ++counter;
        }
        std::cerr << "Map data size: " << mapDataSizeSize << " (" << counter << " items)\n";
    }

    {
        // build the npc types table
        AsmLabel *npcTypesLabel = new AsmLabel(Origin(), "__npctypes");
        stringTable.push_back(npcTypesLabel);
        AsmData *npcTypesCountData = new AsmData(Origin(), 4);
        stringTable.push_back(npcTypesCountData);
        Value npcTypesCount;
        npcTypesCount.value = code.npcTypes.size();
        npcTypesCountData->data.push_back(npcTypesCount);
        unsigned npcTypesSize = 4;
        int counter = 0;
        for (const NpcType &npcType : code.npcTypes) {
            code.addSymbol(npcType.identifier, SymbolDef{npcType.origin, Value{counter}});
            AsmData *data = new AsmData(npcType.origin, 4);
            data->data.push_back(npcType.name);
            data->data.push_back(npcType.artIndex);
            data->data.push_back(npcType.aiType);
            data->data.push_back(npcType.health);
            data->data.push_back(npcType.energy);
            data->data.push_back(npcType.damage);
            data->data.push_back(npcType.accuracy);
            data->data.push_back(npcType.evasion);
            data->data.push_back(npcType.moveRate);
            data->data.push_back(npcType.lootType);
            data->data.push_back(npcType.loot);
            stringTable.push_back(data);
            npcTypesSize += data->data.size() * data->width;
            ++counter;
        }
        std::cerr << "NPC Types size: " << npcTypesSize << " (" << counter << " items)\n";
    }

    {
        // build the worlds table
        AsmLabel *worldsLabel = new AsmLabel(Origin(), "__worlds");
        stringTable.push_back(worldsLabel);
        AsmData *worldsCountData = new AsmData(Origin(), 4);
        stringTable.push_back(worldsCountData);
        Value itemDefCount(code.worlds.size());
        worldsCountData->data.push_back(itemDefCount);
        unsigned worldsSize = 4;
        int counter = 0;
        for (const World &world : code.worlds) {
            code.addSymbol(world.identifier, SymbolDef{world.origin, Value{counter}});
            AsmData *data = new AsmData(world.origin, 4);
            data->data.push_back(world.name);
            data->data.push_back(world.width);
            data->data.push_back(world.height);
            data->data.push_back(world.firstmap);
            stringTable.push_back(data);
            worldsSize += data->data.size() * data->width;
            ++counter;
        }
        std::cerr << "Worlds size: " << worldsSize << " (" << counter << " items)\n";
    }
    // insert into beginning of program code
    code.code.insert(code.code.begin(), stringTable.begin(), stringTable.end());
    return true;
}