#ifndef PARSESTATE_H
#define PARSESTATE_H

#include "token.h"
#include <vector>
struct ErrorLog;
struct Program;

struct ParseState {
    ParseState(const std::vector<Token> &tokens, ErrorLog &errorLog, Program &code);
    const Token& here() const;
    const Token& peek() const;
    void advance();
    void checkForEOL();
    bool matches(TokenType type) const;
    bool require(TokenType type);

    unsigned pos;
    const std::vector<Token> &tokens;
    ErrorLog &errorLog;
    Program &code;
};

#endif
