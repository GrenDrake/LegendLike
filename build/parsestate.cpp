#include <sstream>
#include <vector>

#include "assemble.h"
#include "parsestate.h"

const Token BAD_TOKEN{Origin(), TokenType::Bad};

ParseState::ParseState(const std::vector<Token> &tokens, ErrorLog &errorLog, Program &code)
: pos(0), tokens(tokens), errorLog(errorLog), code(code)
{ }

const Token& ParseState::here() const {
    if (pos >= tokens.size()) return BAD_TOKEN;
    return tokens[pos];
}
const Token& ParseState::peek() const {
    if (pos + 1 >= tokens.size()) return BAD_TOKEN;
    return tokens[pos + 1];
}
void ParseState::advance() {
    if (pos < tokens.size()) {
        ++pos;
    }
}

void ParseState::checkForEOL() {
    if (here().type != TokenType::EOL) {
        errorLog.add(here().origin, "excess tokens on line.");
    }
}
bool ParseState::matches(TokenType type) const {
    if (here().type != type) {
        return false;
    }
    return true;
}
bool ParseState::require(TokenType type) {
    if (here().type != type) {
        std::stringstream line;
        line << "expected " << type << " but found " << here().type << '.';
        errorLog.add(here().origin, line.str());
        return false;
    }
    return true;
}
