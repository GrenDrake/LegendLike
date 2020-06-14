#include <sstream>

#include "assemble.h"

Program::Program() 
: doTokenDump(false)
{ }

void Program::addSymbol(const std::string &name, const SymbolDef &symbol) {
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

void Program::add(AsmLine *line) {
    code.push_back(line);
}
