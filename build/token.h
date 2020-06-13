#ifndef TOKEN_H
#define TOKEN_H

enum class TokenType {
    Bad,
    Directive,
    Identifier,
    Integer,
    String,
    Colon,
    At,
    Equals,
    EOL
};

struct Origin {
    std::string sourceFile;
    int line, column;
};

struct Token {
    Origin origin;

    TokenType type;
    std::string text;
    int i;
};

#endif
