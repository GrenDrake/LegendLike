#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include "assemble.h"

void dumpString(std::ostream &out, const std::string &text);



std::ostream& operator<<(std::ostream &out, const Origin &origin) {
    out << origin.sourceFile << ':' << origin.line << ':' << origin.column;
    return out;
}


std::ostream& operator<<(std::ostream &out, TokenType type) {
    switch(type) {
        case TokenType::Bad:
            out << "BAD_TOKEN";
            break;
        case TokenType::Directive:
            out << "DIRECTIVE";
            break;
        case TokenType::Identifier:
            out << "IDENTIFIER";
            break;
        case TokenType::Integer:
            out << "INTEGER";
            break;
        case TokenType::String:
            out << "STRING";
            break;
        case TokenType::Colon:
            out << "COLON";
            break;
        case TokenType::At:
            out << "AT";
            break;
        case TokenType::Equals:
            out << "EQUALS";
            break;
        case TokenType::EOL:
            out << "EOL";
            break;
    }
    return out;
}

void clearTokenDump() {
    std::ofstream out("_tokens.txt");
}

void dumpCode(const std::vector<AsmLine*> &code) {
    static unsigned sizeMasks[5] = { 0, 0xFF, 0xFFFF, 0, 0xFFFFFFFF };

    std::ofstream codeDump("_code.txt");
    for (AsmLine *line : code) {
        AsmData  *data = dynamic_cast<AsmData*>(line);
        AsmLabel *label = dynamic_cast<AsmLabel*>(line);
        AsmCode *asmcode = dynamic_cast<AsmCode*>(line);

        if (data) {
            const int valueWidth = data->width * 2;
            codeDump << "DATA   " << data->origin << " / ";
            codeDump << data->data.size() * data->width << " bytes (";
            codeDump << data->data.size() << 'x' << data->width;
            codeDump << "):" << std::hex;
            if (data->fromString) {
                std::string text;
                for (const Value &v : data->data) {
                    text += v.value;
                }
                codeDump << "\n    ~";
                dumpString(codeDump, text);
                codeDump << "~\n";
            } else if (data->data.size() == 1) {
                codeDump << " 0x" << std::setw(valueWidth) << (data->data[0].value & sizeMasks[data->width]) << '\n';
            } else if (data->data.size() > 1) {
                codeDump << '\n' << std::setfill('0');
                const int stepSize = 16 / data->width;
                const int breakSpace = 8 / data->width - 1;
                for (unsigned i = 0; i < data->data.size(); i += stepSize) {
                    codeDump << "   ";
                    for (unsigned j = 0; j < static_cast<unsigned>(stepSize); ++j) {
                        unsigned k = i + j;
                        codeDump << ' ';
                        if (k < data->data.size()) {
                            int v = data->data[k].value;
                            codeDump << std::setw(valueWidth) << (v & sizeMasks[data->width]);
                        } else {
                            codeDump << "  ";
                        }
                        if (j == static_cast<unsigned>(breakSpace)) codeDump << "  ";
                    }
                    codeDump << '\n';
                }
            }
            codeDump << std::dec;
        } else if (label) {
            codeDump << "LABEL  " << label->origin << " / ~" << label->name << "~\n";
        } else if (asmcode) {
            codeDump << "OPCODE " << asmcode->origin << " " << asmcode->mnemonic.name;
            codeDump << '(' << static_cast<int>(asmcode->mnemonic.opcode) << ')';
            if (asmcode->mnemonic.operandSize != 0) {
                codeDump << "  <";
                if (asmcode->operandValue.identifier.empty()) {
                    codeDump << asmcode->operandValue.value;
                } else {
                    codeDump << asmcode->operandValue.identifier;
                }
                codeDump << '>';
            }
            codeDump << '\n';
        }
    }
}

void dumpErrors(const Program &program) {
    for (const ErrorMsg &err : program.errorLog.errors) {
        if (err.origin.sourceFile.empty()) {
            std::cerr << "(generate)";
        } else {
            std::cerr << err.origin;
        }
        std::cerr << "  " << err.text << '\n';
    }
    std::cerr << program.errorLog.errors.size() << " errors occured.\n";
}

void dumpPatches(const Program &program) {
    std::ofstream backpatchDump("_patches.txt");
    backpatchDump << std::left;
    for (Backpatch iter : program.patches) {
        backpatchDump << std::setw(30) << iter.name << "  ";
        backpatchDump << iter.width << "  ";
        backpatchDump << iter.pos << "\n";
    }
}

void dumpString(std::ostream &out, const std::string &text) {
    for (char c : text) {
        if (c >= 32 && c <= 127) {
            out << c;
        } else if (c == '\n') {
            out << "\\n";
        } else {
            out << "\\x" << std::hex << static_cast<int>(c);
        }
    }
}

void dumpStrings(const Program &program) {
    unsigned longestLabel = 0;
    for (auto iter : program.strings) {
        if (iter.first.size() > longestLabel) {
            longestLabel = iter.first.size();
        }
    }
    longestLabel += 2;

    std::ofstream dumpFile("_strings.txt");
    dumpFile << std::left;
    for (auto iter : program.strings) {
        dumpFile << std::setw(longestLabel) << iter.first << '~';
        dumpString(dumpFile, iter.second.text);
        dumpFile << "~\n";
    }
}

void dumpSymbols(const Program &program) {
    unsigned identifierWidth = 0;
    for (auto iter : program.symbolTable) {
        if (iter.first.size() > identifierWidth) identifierWidth = iter.first.size();
    }
    identifierWidth += 2;

    std::ofstream labelValues("_symbols.txt");
    labelValues << std::left;
    for (auto iter : program.symbolTable) {
        labelValues << std::setw(identifierWidth) << iter.first << "  ";
        if (!iter.second.value.identifier.empty()) {
            labelValues << iter.second.value.identifier;
        } else {
            labelValues << iter.second.value.value << " (0x";
            labelValues << std::hex << iter.second.value.value << std::dec << ')';
        }
        labelValues << " (" << iter.second.origin << ")\n";
    }
}

void dumpTokens(const std::vector<Token> &tokens) {
    std::ofstream out("_tokens.txt", std::ios_base::out | std::ios_base::app);
    for (const Token &token : tokens) {
        out << token.origin << "  " << token.type << " ";
        out << token.i << " ~" << token.text << "~\n";
    }
}
