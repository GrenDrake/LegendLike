#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include "assemble.h"
#include "parsestate.h"

std::string anonymousString(ParseState &state, const std::string &text);
bool buildPush(ParseState &state, const Origin &origin, const Value &value);
Value tokenToValue(ParseState &state);

void unescapeString(std::string &text) {
    for (unsigned i = 0; i < text.size(); ++i) {
        if (text[i] != '\\') continue;
        ++i;
        if (i >= text.size()) break;
        switch(text[i]) {
            case 'n':
                text[i - 1] = '\n';
                break;
            default:
                text[i - 1] = text[i];
                break;
        }
        text.erase(i, 1);
    }
}

std::vector<Token> parseLine(const std::string &line, const std::string &filename, int lineNo) {
    std::vector<Token> tokens;

    std::string::size_type pos = 0;
    while (pos < line.size()) {
        if (g_is_whitespace(line[pos])) {
            while (g_is_whitespace(line[pos])) {
                ++pos;
            }
            continue;

        } else if (line[pos] == ';') {
            // line comment - skip rest of line
            break;

        } else if (line[pos] == ':') {
            int column = pos + 1;
            Origin origin{ filename, lineNo, column};
            ++pos;
            tokens.push_back(Token{origin, TokenType::Colon});
        } else if (line[pos] == '=') {
            int column = pos + 1;
            Origin origin{ filename, lineNo, column};
            ++pos;
            tokens.push_back(Token{origin, TokenType::Equals});

        } else if (line[pos] == '@') {
            int column = pos + 1;
            Origin origin{ filename, lineNo, column};
            ++pos;
            tokens.push_back(Token{origin, TokenType::At});

        } else if (line[pos] == '-' || g_is_digit(line[pos])) {
            int column = pos + 1;
            Origin origin{ filename, lineNo, column};
            bool negative = line[pos] == '-';
            if (negative) ++pos;
            auto start = pos;
            while (g_is_digit(line[pos])) {
                ++pos;
            }
            std::string number = line.substr(start, pos - start);
            int value = 0;
            for (char c : number) {
                value *= 10;
                value += c - '0';
            }
            if (negative) value = -value;
            tokens.push_back(Token{origin, TokenType::Integer, "", value});

        } else if (line[pos] == '.' || g_is_identifier(line[pos])) {
            int column = pos + 1;
            Origin origin{ filename, lineNo, column};
            auto start = pos;
            do {
                ++pos;
            } while (g_is_identifier(line[pos]));
            std::string name = line.substr(start, pos - start);
            if (name[0] == '.') {
                tokens.push_back(Token{origin, TokenType::Directive, name});
            } else {
                tokens.push_back(Token{origin, TokenType::Identifier, name});
            }

        } else if (line[pos] == '"') {
            int column = pos + 1;
            Origin origin{ filename, lineNo, column};
            ++pos;
            const auto start = pos;
            while (pos < line.size()) {
                if (line[pos] == '"' && line[pos - 1] != '\\') break;
                ++pos;
            }
            std::string text = line.substr(start, pos - start);
            unescapeString(text);
            tokens.push_back(Token{origin, TokenType::String, text});
            ++pos;


        } else {
            // unexpected character in source
            ++pos;
        }
    }

    int finalColumn = line.size();
    tokens.push_back(Token{Origin{filename, lineNo, finalColumn}, TokenType::EOL});
    return tokens;
}

bool parseData(ParseState &state, int width) {
    const Origin &origin = state.here().origin;
    state.advance();

    AsmData *data = new AsmData(origin, width);
    while (state.here().type != TokenType::EOL) {
        if (state.here().type == TokenType::Identifier) {
            data->data.push_back(Value(state.here().text));
        } else if (state.require(TokenType::Integer)) {
            data->data.push_back(Value{state.here().i});
        }
        state.advance();
    }

    state.code.add(data);
    return true;
}

bool parseDefine(ParseState &state) {
    const Origin &origin = state.here().origin;
    state.advance();

    if (!state.require(TokenType::Identifier)) return false;
    const std::string &name = state.here().text;
    state.advance();

    if (!state.require(TokenType::Integer)) return false;
    SymbolDef newSymbol(origin, Value{state.here().i});
    state.code.addSymbol(name, newSymbol);

    state.advance();
    state.checkForEOL();
    return true;
}

bool parseItemDef(ParseState &state) {
    const Origin &origin = state.here().origin;
    state.advance(); // skip .itemdef

    if (!state.require(TokenType::Identifier)) return false;
    const std::string &identifier = state.here().text;
    state.advance();

    Value name{0};
    Value artFile{0};
    Value itemId{0};
    while (!state.matches(TokenType::EOL)) {
        if (!state.require(TokenType::Identifier)) return false;
        const std::string &label = state.here().text;
        state.advance();

        if (!state.require(TokenType::Equals)) return false;
        state.advance();

        Value value = tokenToValue(state);
        state.advance();

        if (label == "name")          name = value;
        else if (label == "art")      artFile = value;
        else if (label == "itemId")   itemId = value;
        else {
            state.code.errorLog.add(origin, "Unknown item def attribute " + label + ".");
            return false;
        }
    }

    ItemDef newItemDef{ origin, identifier, name, artFile, itemId };
    state.code.itemDefs.push_back(newItemDef);
    return true;
}

bool parseLocation(ParseState &state) {
    const Origin &origin = state.here().origin;
    state.advance(); // skip .location

    if (!state.require(TokenType::Identifier)) return false;
    const std::string &identifier = state.here().text;
    state.advance();

    Value itemId{-1};
    while (!state.matches(TokenType::EOL)) {
        if (!state.require(TokenType::Identifier)) return false;
        const std::string &label = state.here().text;
        state.advance();

        if (!state.require(TokenType::Equals)) return false;
        state.advance();

        Value value = tokenToValue(state);
        state.advance();

        if (label == "item")          itemId = value;
        else {
            state.code.errorLog.add(origin, "Unknown location attribute " + label + ".");
            return false;
        }
    }

    ItemLocation newLoc{ origin, identifier, itemId };
    state.code.locations.push_back(newLoc);
    return true;
}

bool parseWorld(ParseState &state) {
    const Origin &origin = state.here().origin;
    state.advance(); // skip .world

    if (!state.require(TokenType::Identifier)) return false;
    const std::string &identifier = state.here().text;
    state.advance();

    Value name{-1};
    Value width{-1};
    Value height{-1};
    Value firstmap{-1};
    while (!state.matches(TokenType::EOL)) {
        if (!state.require(TokenType::Identifier)) return false;
        const std::string &label = state.here().text;
        state.advance();

        if (!state.require(TokenType::Equals)) return false;
        state.advance();

        Value value = tokenToValue(state);
        state.advance();

        if (label == "name")            name = value;
        else if (label == "width")      width = value;
        else if (label == "height")     height = value;
        else if (label == "firstmap")   firstmap = value;
        else {
            state.code.errorLog.add(origin, "Unknown world attribute " + label + ".");
            return false;
        }
    }

    World newWorld{ origin, identifier, name, width, height, firstmap };
    state.code.worlds.push_back(newWorld);
    return true;
}

bool parseLootTable(ParseState &state) {
    const Origin &origin = state.here().origin;
    state.advance(); // skip .loottable

    if (!state.require(TokenType::Identifier)) return false;
    const std::string &identifier = state.here().text;
    state.advance();

    LootTable lootTable{ origin, identifier };
    while (!state.matches(TokenType::EOL)) {
        if (!state.require(TokenType::Integer)) return false;
        int chance = state.here().i;
        state.advance();

        Value ItemId = tokenToValue(state);
        state.advance();
        lootTable.rows.push_back(LootRow{Value{chance}, ItemId});
    }

    state.code.lootTables.push_back(lootTable);
    return true;
}

const unsigned AF_PROPER = 0x01;
bool parseNPC(ParseState &state) {
    const Origin &origin = state.here().origin;
    state.advance(); // skip .npc

    if (!state.require(TokenType::Identifier)) return false;
    const std::string &identifier = state.here().text;
    state.advance();

    Value name{0}, aiType{0}, aiArg{0}, talkFunc{0}, specialValue{0};
    Value typeId{0}, xPos{0}, yPos{0};
    bool isProper = false;
    while (!state.matches(TokenType::EOL)) {
        if (!state.require(TokenType::Identifier)) return false;
        const std::string &label = state.here().text;
        state.advance();

        if (label == "proper") isProper = true;
        else {
            if (!state.require(TokenType::Equals)) return false;
            state.advance();

            Value value = tokenToValue(state);
            state.advance();

            if (label == "name")          name = value;
            else if (label == "talkFunc") talkFunc = value;
            else if (label == "special")  specialValue = value;
            else if (label == "typeId")   typeId = value;
            else if (label == "x")        xPos = value;
            else if (label == "y")        yPos = value;
            else {
                state.code.errorLog.add(origin, "Unknown NPC attribute " + label + ".");
                return false;
            }
        }
    }

    AsmLabel *npcLabel = new AsmLabel(origin, identifier);
    AsmData *npcDataW = new AsmData(origin, 4);
    AsmData *npcDataS = new AsmData(origin, 2);
    AsmData *npcDataB = new AsmData(origin, 1);

    npcDataW->data.push_back(name);
    npcDataW->data.push_back(talkFunc);
    npcDataW->data.push_back(specialValue);
    npcDataW->data.push_back(Value(isProper ? AF_PROPER : 0));
    npcDataS->data.push_back(xPos);
    npcDataS->data.push_back(yPos);
    npcDataS->data.push_back(typeId);

    state.code.add(npcLabel);
    state.code.add(npcDataW);
    state.code.add(npcDataS);
    state.code.add(npcDataB);
    return true;
}

const unsigned MF_UPSTAIRS = 0x01;
const unsigned MF_DOWNSTAIRS = 0x02;
bool parseMapData(ParseState &state) {
    const Origin &origin = state.here().origin;
    state.advance(); // skip .mapdata

    if (!state.require(TokenType::Identifier)) return false;
    const std::string &identifier = state.here().text;
    state.advance();

    Value name{0};
    Value mapId{-1};
    Value width{16};
    Value height{16};
    Value onBuild{0};
    Value onEnter{0};
    Value onReset{0};
    Value musicTrack{1};
    bool genUpStairs, genDownStairs;
    while (!state.matches(TokenType::EOL)) {
        if (!state.require(TokenType::Identifier)) return false;
        const std::string &label = state.here().text;
        state.advance();

        if (label == "addUpStairs") genUpStairs = true;
        else if (label == "addDownStairs") genDownStairs = true;
        else {
            if (!state.require(TokenType::Equals)) return false;
            state.advance();

            Value value = tokenToValue(state);
            state.advance();

            if (label == "name")            name = value;
            else if (label == "mapId")      mapId = value;
            else if (label == "width")      width = value;
            else if (label == "height")     height = value;
            else if (label == "onBuild")    onBuild = value;
            else if (label == "onEnter")    onEnter = value;
            else if (label == "onReset")    onReset = value;
            else if (label == "musicTrack") musicTrack = value;
            else {
                state.code.errorLog.add(origin, "Unknown map data attribute " + label + ".");
                return false;
            }
        }
    }

    MapData mapData{ origin, identifier, name, mapId, width, height, onBuild, onEnter, onReset, musicTrack };
    if (genUpStairs)   mapData.flags |= MF_UPSTAIRS;
    if (genDownStairs) mapData.flags |= MF_DOWNSTAIRS;
    state.code.mapData.push_back(mapData);
    return true;
}

bool parseNpcType(ParseState &state) {
    const Origin &origin = state.here().origin;
    state.advance(); // skip .npctype

    if (!state.require(TokenType::Identifier)) return false;
    const std::string &identifier = state.here().text;
    state.advance();

    Value name{0};
    Value artIndex{0};
    Value health{10000};
    Value energy{10000};
    Value accuracy{0};
    Value evasion{0};
    Value damage{1};
    Value moveRate{1};
    Value aiType{0};
    Value lootType(0);
    Value loot(0);
    while (!state.matches(TokenType::EOL)) {
        if (!state.require(TokenType::Identifier)) return false;
        const std::string &label = state.here().text;
        state.advance();

        if (!state.require(TokenType::Equals)) return false;
        state.advance();

        Value value = tokenToValue(state);
        state.advance();

        if (label == "name")          name = value;
        else if (label == "art")      artIndex = value;
        else if (label == "health")   health = value;
        else if (label == "energy")   energy = value;
        else if (label == "damage")   damage = value;
        else if (label == "accuracy") accuracy = value;
        else if (label == "evasion")  evasion = value;
        else if (label == "moveRate") moveRate = value;
        else if (label == "aiType")   aiType = value;
        else if (label == "lootType") lootType = value;
        else if (label == "loot")     loot = value;
        else {
            state.code.errorLog.add(origin, "Unknown npc type attribute " + label + ".");
            return false;
        }
    }

    NpcType npcType{ origin, identifier, name, artIndex, aiType, health, energy, damage, accuracy, evasion, moveRate, lootType, loot };
    state.code.npcTypes.push_back(npcType);
    return true;
}

bool parseOpcode(ParseState &state) {
    bool doShortcutOpcode = false;
    if (state.matches(TokenType::At)) {
        doShortcutOpcode = true;
        state.advance();
    }
    if (!state.require(TokenType::Identifier)) return false;

    const Mnemonic &mnemonic = getMnemonic(state.here().text);
    if (mnemonic.opcode == Opcode::bad) {
        state.errorLog.add(state.here().origin, "unknown mnemonic " + state.here().text + ".");
        return false;
    }

    if (mnemonic.operandSize != 0 && doShortcutOpcode) {
        state.errorLog.add(state.here().origin, "cannot use shortcut codes with opcodes requiring operands.");
        return false;
    }

    AsmCode *acode = new AsmCode(state.here().origin, mnemonic);
    state.advance();
    if (!doShortcutOpcode && mnemonic.operandSize == 0) {
        state.checkForEOL();
        state.code.add(acode);
        return true;
    }
    if (mnemonic.operandSize != 0) {
        state.code.add(acode);
        acode->operandValue = tokenToValue(state);
        state.advance();
        state.checkForEOL();
        return true;
    }
    if (doShortcutOpcode) {
        while (state.here().type != TokenType::EOL) {
            buildPush(state, state.here().origin, tokenToValue(state));
            state.advance();
        }
        state.code.add(acode);
    }
    return true;
}

bool parsePushCount(ParseState &state) {
    const Origin &origin = state.here().origin;
    state.advance(); // skip .push_count
    int pushCount = 0;
    while (!state.matches(TokenType::EOL)) {
        buildPush(state, state.here().origin, tokenToValue(state));
        state.advance();
        ++pushCount;
    }
    buildPush(state, origin, Value{pushCount});
    return true;
}

bool parseString(ParseState &state) {
    const Origin &origin = state.here().origin;
    state.advance(); // skip .string

    if (!state.require(TokenType::Identifier)) {
        return false;
    }
    const std::string &name = state.here().text;
    state.advance();

    if (!state.require(TokenType::String)) {
        return false;
    }
    StringData data{ origin, state.here().text };
    state.code.strings.insert(std::make_pair(name, data));
    state.advance();
    state.checkForEOL();
    return true;
}

const unsigned TILE_ISSOLID  = 0x01;
const unsigned TILE_ISOPAQUE = 0x02;
const unsigned TILE_ISDOOR   = 0x04;
bool parseTileDef(ParseState &state) {
    const Origin &origin = state.here().origin;
    state.advance(); // skip .tiledef

    if (!state.require(TokenType::Identifier)) return false;
    const std::string &identifier = state.here().text;
    state.advance();

    Value name{0};
    Value artIndex{0};
    Value red{0};
    Value green{0};
    Value blue{0};
    Value interactTo{-1};
    Value animLength{1};
    Value group{0};
    bool isSolid = false, isOpaque = false, isDoor = false;
    while (!state.matches(TokenType::EOL)) {
        if (!state.require(TokenType::Identifier)) return false;
        const std::string &label = state.here().text;
        state.advance();

        if (label == "solid")       isSolid = true;
        else if (label == "opaque") isOpaque = true;
        else if (label == "door")   isDoor = true;
        else {
            if (!state.require(TokenType::Equals)) return false;
            state.advance();

            Value value = tokenToValue(state);
            state.advance();

            if (label == "name")            name = value;
            else if (label == "group")      group = value;
            else if (label == "art")        artIndex = value;
            else if (label == "red")        red = value;
            else if (label == "green")      green = value;
            else if (label == "blue")       blue = value;
            else if (label == "interactTo") interactTo = value;
            else if (label == "animLength") animLength = value;
            else {
                state.code.errorLog.add(origin, "Unknown tiledef attribute " + label + ".");
                return false;
            }
        }
    }

    TileDef newTile{ origin, identifier, name, group, artIndex, red, green, blue, interactTo, animLength };
    if (isSolid)  newTile.flags |= TILE_ISSOLID;
    if (isOpaque) newTile.flags |= TILE_ISOPAQUE;
    if (isDoor)   newTile.flags |= TILE_ISDOOR;

    state.code.tileDefs.push_back(newTile);
    return true;
}

bool parseVersion(ParseState &state) {
    state.advance(); // skip .version

    state.code.gameName = tokenToValue(state);
    state.advance();

    state.code.gameId = tokenToValue(state);
    state.advance();

    state.code.gameMajorVersion = tokenToValue(state);
    state.advance();

    state.code.gameMinorVersion = tokenToValue(state);
    state.advance();

    state.checkForEOL();
    return true;
}

bool parsePaddedString(ParseState &state) {
    const Origin &origin = state.here().origin;
    state.advance(); // skip .string_pad

    if (!state.require(TokenType::Integer)) return false;;
    unsigned length = state.here().i;
    state.advance();

    if (state.require(TokenType::String)) {
        if (state.here().text.size() > length) {
            state.errorLog.add(state.here().origin, "padded string length is longer than padding.");
        }
        AsmData *data = new AsmData(origin, 1, true);
        for (char c : state.here().text) {
            data->data.push_back(Value{c});
        }
        for (unsigned i = state.here().text.size(); i < length; ++i) {
            data->data.push_back(Value{0});
        }
        state.code.add(data);
        state.advance();
        state.checkForEOL();
    }
    return true;
}

bool parseFile(const std::string &filename, ErrorLog &errorLog, Program &code) {

    std::ifstream inf(filename);
    if (!inf) {
        errorLog.add("Failed to open file " + filename + ".");
        return false;
    }

    std::string line;
    int lineNo = 0;
    while (std::getline(inf, line)) {
        ++lineNo;

        std::vector<Token> tokens = parseLine(line, filename, lineNo);
        if (tokens.empty() || tokens.front().type == TokenType::EOL) continue;
        ParseState state(tokens, errorLog, code);
        if (code.doTokenDump) dumpTokens(tokens);

        if (state.here().type == TokenType::Directive) {
            // Directive
            if (state.here().text == ".push_count") {
                if (!parsePushCount(state)) continue;
            } else if (state.here().text == ".define") {
                if (!parseDefine(state)) continue;
            } else if (state.here().text == ".string") {
                if (!parseString(state)) continue;
            } else if (state.here().text == ".string_pad") {
                if (!parsePaddedString(state)) continue;
            } else if (state.here().text == ".word") {
                if (!parseData(state, 4)) continue;
            } else if (state.here().text == ".short") {
                if (!parseData(state, 2)) continue;
            } else if (state.here().text == ".byte") {
                if (!parseData(state, 1)) continue;
            } else if (state.here().text == ".location") {
                if (!parseLocation(state)) continue;
            } else if (state.here().text == ".npc") {
                if (!parseNPC(state)) continue;
            } else if (state.here().text == ".npctype") {
                if (!parseNpcType(state)) continue;
            } else if (state.here().text == ".mapdata") {
                if (!parseMapData(state)) continue;
            } else if (state.here().text == ".tiledef") {
                if (!parseTileDef(state)) continue;
            } else if (state.here().text == ".version") {
                if (!parseVersion(state)) continue;
            } else if (state.here().text == ".loottable") {
                if (!parseLootTable(state)) continue;
            } else if (state.here().text == ".itemdef") {
                if (!parseItemDef(state)) continue;
            } else if (state.here().text == ".world") {
                if (!parseWorld(state)) continue;
            } else if (state.here().text == ".export") {
                state.advance();
                while (state.here().type != TokenType::EOL) {
                    if (state.require(TokenType::Identifier)) {
                        code.exports.push_back(state.here().text);
                    }
                    state.advance();
                }

            } else {
                errorLog.add(state.here().origin, "unknown directive " + state.here().text + ".");
            }
            continue;
        }

        if (state.matches(TokenType::Identifier) && state.peek().type == TokenType::Colon) {
            // Label
            Origin origin{filename, lineNo};
            AsmLabel *label = new AsmLabel(origin, state.here().text);
            code.add(label);
            state.advance();    state.advance();
            state.checkForEOL();
            continue;
        }

        // Otherwise must be a mnemonic
        parseOpcode(state);
    }

    return true;
}

std::string anonymousString(ParseState &state, const Origin &origin, const std::string &text) {
    static unsigned nextIdent = 1;
    std::string labelName = "__string_" + std::to_string(nextIdent);
    state.code.strings.insert(std::make_pair(labelName, StringData{origin, text}));
    ++nextIdent;
    return labelName;
}

bool buildPush(ParseState &state, const Origin &origin, const Value &value) {
    const Mnemonic &pushOpcode = getMnemonic("push");
    AsmCode *pushCode = new AsmCode(origin, pushOpcode);
    pushCode->operandValue = value;
    state.code.add(pushCode);
    return true;
}

Value tokenToValue(ParseState &state) {
    if (state.here().type == TokenType::Identifier) {
        return Value(state.here().text);
    } else if (state.matches(TokenType::String)) {
        std::string label = anonymousString(state, state.here().origin, state.here().text);
        return Value(label);
    } else if (state.require(TokenType::Integer)) {
        return Value{state.here().i};
    } else {
        state.errorLog.add(state.here().origin, "Invalid data type for value.");
        return Value{0x7FFFFFFF};
    }
}
