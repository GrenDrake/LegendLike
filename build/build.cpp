#include <iostream>
#include <string>
#include <vector>

#include "assemble.h"

bool buildHeader(Program &code);

int main(int argc, char *argv[]) {
    bool doDumpInternals = false;
    std::vector<std::string> files;
    std::string outputFile = "output.bin";
    Program code;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg[0] == '-') {
            if (arg == "-dump") doDumpInternals = true;
            else if (arg == "-o") {
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

    clearTokenDump();
    for (const std::string &filename : files) {
        parseFile(filename, code.errorLog, code);
        if (!code.errorLog.errors.empty()) {
            dumpErrors(code);
            return 1;
        }
    }

    buildHeader(code);

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
    // add header and initial data (like the string table) to asm code
    std::vector<AsmLine*> stringTable;
    AsmLabel *headerTableLabel = new AsmLabel(Origin(), "__header");
    stringTable.push_back(headerTableLabel);
    AsmData *headerData = new AsmData(Origin(), 4);
    stringTable.push_back(headerData);
    headerData->data.push_back(Value{0x004D5654});
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
        exportsData->data.push_back(Value{0, name});
    }
    std::cerr << "Header size: " << headerSize + exportsData->data.size() * exportsData->width << "\n";


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

    // insert into beginning of program code
    code.code.insert(code.code.begin(), stringTable.begin(), stringTable.end());
    return true;
}