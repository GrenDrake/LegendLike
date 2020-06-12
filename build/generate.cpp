#include <cstdint>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include "assemble.h"


void Program::addSymbol(ErrorLog &errorLog, const std::string &name, const SymbolDef &symbol) {
    auto existing = symbolTable.find(name);
    if (existing != symbolTable.end()) {
        std::stringstream msg;
        msg << "Symbol " << name << " already defined at " << existing->second.origin << '.';
        errorLog.add(symbol.origin, msg.str());
        return;
    }

    symbolTable.insert(std::make_pair(name, symbol));
}

const SymbolDef BAD_SYMBOL;
const SymbolDef& Program::getSymbol(const std::string &name) {
    auto existing = symbolTable.find(name);
    if (existing == symbolTable.end()) {
        return BAD_SYMBOL;
    }
    return existing->second;
}

void write32(std::ostream &out, std::uint32_t value) {
    out.write(reinterpret_cast<const char*>(&value), 4);
}
void write16(std::ostream &out, std::uint16_t value) {
    out.write(reinterpret_cast<const char*>(&value), 2);
}
void write8(std::ostream &out, std::uint8_t value) {
    out.write(reinterpret_cast<const char*>(&value), 1);
}

void writeValue(Program &code, std::ostream &out, unsigned filePos, int width, const Value &value) {
    if (value.identifier.empty()) {
        switch(width) {
            case 1:
                write8(out, value.value);
                break;
            case 2:
                write16(out, value.value);
                break;
            case 4:
                write32(out, value.value);
                break;
            default:
                // code.errorLog.add(Origin(), "(internal) Unhandled value width " + std::to_string(width));
                break;
        }
    } else {
        if (filePos == 0) return; // signals updating backpatches; we should never create more at this point

        const SymbolDef &symbol = code.getSymbol(value.identifier);
        if (symbol.valid) {
            writeValue(code, out, filePos, width, symbol.value);
        } else {
            unsigned long pos = out.tellp();
            Backpatch patch{ pos, value.identifier, width };
            code.patches.push_back(patch);
            writeValue(code, out, filePos, width, Value{0x7FFFFFFF});
        }
    }
}

void generate(Program &code, const std::string &outputFile) {
    std::ofstream outfile(outputFile, std::ios_base::binary);
    unsigned filePos = 0;

    for (const AsmLine *line : code.code) {
        const AsmCode *asmcode = dynamic_cast<const AsmCode*>(line);
        const AsmData *data = dynamic_cast<const AsmData*>(line);
        const AsmLabel *label = dynamic_cast<const AsmLabel*>(line);

        if (asmcode) {
            write8(outfile, static_cast<int>(asmcode->mnemonic.opcode));
            ++filePos;
            if (asmcode->mnemonic.operandSize > 0) {
                writeValue(code, outfile, filePos, asmcode->mnemonic.operandSize, asmcode->operandValue);
                filePos += asmcode->mnemonic.operandSize;
            }
        } else if (data) {
            for (const Value &value : data->data) {
                writeValue(code, outfile, filePos, data->width, value);
                filePos += data->width;
            }
        } else if (label) {
            Value value;
            value.value = filePos;
            code.addSymbol(code.errorLog, label->name, SymbolDef(label->origin, value));
        }
    }

    for (Backpatch patch : code.patches) {
        const SymbolDef &symbol = code.getSymbol(patch.name);
        if (!symbol.valid) {
            code.errorLog.add(Origin(), "Undefined symbol " + patch.name);
        } else {
            outfile.seekp(patch.pos);
            writeValue(code, outfile, 0, patch.width, symbol.value);
        }
    }
}

