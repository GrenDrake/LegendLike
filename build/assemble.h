#ifndef ASSEMBLE_H
#define ASSEMBLE_H

#include <map>
#include <string>
#include <vector>
#include "opcode.h"
#include "token.h"

#define HEADER_SIZE 12

struct Mnemonic {
    Opcode opcode;
    std::string name;
    int operandSize;
};


struct Value {
    Value()
    : value(0)
    { }
    Value(int value)
    : value(value)
    { }
    Value(const std::string &identifier)
    : value(0x7FFFFFFF), identifier(identifier)
    { }

    int value;
    std::string identifier;
};

struct AsmLine {
    AsmLine(const Origin &origin)
    : origin(origin)
    { }
    virtual ~AsmLine() {}

    Origin origin;
};

struct AsmLabel : public AsmLine {
    AsmLabel(const Origin &origin, const std::string &name)
    : AsmLine(origin), name(name)
    { }
    virtual ~AsmLabel() {}

    std::string name;
};

struct AsmData : public AsmLine {
    AsmData(const Origin &origin, int width)
    : AsmLine(origin), width(width), fromString(false)
    { }
    AsmData(const Origin &origin, int width, bool fromString)
    : AsmLine(origin), width(width), fromString(fromString)
    { }
    virtual ~AsmData() {}

    int width;
    std::vector<Value> data;
    bool fromString;
};

struct AsmCode : public AsmLine {
    AsmCode(const Origin &origin, const Mnemonic &mnemonic)
    : AsmLine(origin), mnemonic(mnemonic), operandValue{0}
    { }
    AsmCode(const Origin &origin, const Mnemonic &mnemonic, int value)
    : AsmLine(origin), mnemonic(mnemonic), operandValue{value}
    { }
    virtual ~AsmCode() {}

    const Mnemonic &mnemonic;
    Value operandValue;
};

struct ErrorMsg {
    Origin origin;
    std::string text;
};
struct ErrorLog {
    std::vector<ErrorMsg> errors;

    void add(const Origin &origin, const std::string &text) {
        errors.push_back(ErrorMsg{origin, text});
    }
    void add(const std::string &text) {
        errors.push_back(ErrorMsg{Origin{"internal"}, text});
    }
};

struct StringData {
    Origin origin;
    std::string text;
};

struct SymbolDef {
    SymbolDef()
    : valid(false)
    { }
    SymbolDef(const Origin &origin, const Value &value)
    : origin(origin), value(value), valid(true)
    { }

    Origin origin;
    Value value;
    bool valid;
};

struct Backpatch {
    Origin origin;
    unsigned long pos;
    std::string name;
    int width;
};

struct TileDef {
    Origin origin;
    std::string identifier;
    Value name;
    Value group;
    Value artIndex;
    Value red, green, blue;
    Value interactTo;
    Value animLength;
    unsigned flags;
};

struct NpcType {
    Origin origin;
    std::string identifier;
    Value name;
    Value artIndex;
    Value aiType;
    Value health;
    Value energy;
    Value damage;
    Value accuracy;
    Value evasion;
    Value moveRate;
    Value lootType;
    Value loot;
};

struct MapData {
    Origin origin;
    std::string identifier;
    Value name;
    Value mapId;
    Value width, height;
    Value onBuild, onEnter, onReset;
    Value musicTrack;
    unsigned flags;
};

struct ItemLocation {
    Origin origin;
    std::string name;
    Value itemId;
};

struct LootRow {
    Value chance;
    Value itemId;
};

struct LootTable {
    Origin origin;
    std::string identifier;
    std::vector<LootRow> rows;
};

struct ItemDef {
    Origin origin;
    std::string identifier;
    Value name;
    Value artFile;
    Value itemId;
};

struct World {
    Origin origin;
    std::string identifier;
    Value name;
    Value width;
    Value height;
    Value firstmap;
};

struct Program {
    Program();
    ErrorLog errorLog;
    std::vector<AsmLine*> code;
    std::vector<std::string> exports;
    std::vector<ItemLocation> locations;
    std::vector<NpcType> npcTypes;
    std::vector<TileDef> tileDefs;
    std::vector<MapData> mapData;
    std::vector<LootTable> lootTables;
    std::vector<ItemDef> itemDefs;
    std::vector<World> worlds;

    std::map<std::string, SymbolDef> symbolTable;
    std::map<std::string, StringData> strings;
    std::vector<Backpatch> patches;

    bool doTokenDump;
    Value gameName, gameId, gameMajorVersion, gameMinorVersion;

    void addSymbol(const std::string &name, const SymbolDef &symbol);
    const SymbolDef& getSymbol(const std::string &name);
    void add(AsmLine *line);
};

std::ostream& operator<<(std::ostream &out, const Origin &origin);
std::ostream& operator<<(std::ostream &out, TokenType type);
void clearTokenDump();
void dumpCode(const std::vector<AsmLine*> &code);
void dumpErrors(const Program &program);
void dumpPatches(const Program &program);
void dumpStrings(const Program &program);
void dumpSymbols(const Program &program);
void dumpTokens(const std::vector<Token> &tokens);

bool g_is_whitespace(int c);
bool g_is_alpha(int c);
bool g_is_digit(int c);
bool g_is_identifier(int c);
std::string& trim(std::string &text);
std::vector<std::string> explode(const std::string &text);

std::vector<Token> parseLine(const std::string &line, const std::string &filename, int lineNo);
bool parseFile(const std::string &filename, ErrorLog &errorLog, Program &code);

const Mnemonic& getMnemonic(const std::string &name);

void generate(Program &code, const std::string &outputFile);

#endif
