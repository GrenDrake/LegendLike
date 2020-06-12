#include <string>
#include <vector>

static const char *whitespaceChars = " \t\n\r";

bool g_is_whitespace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool g_is_alpha(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool g_is_digit(int c) {
    return c >= '0' && c <= '9';
}

bool g_is_identifier(int c) {
    return g_is_digit(c) || g_is_alpha(c) || c == '_';
}

std::string& trim(std::string &text) {
    std::string::size_type pos;

    pos = text.find_first_not_of(whitespaceChars);
    if (pos > 0) text.erase(0, pos);
    pos = text.find_last_not_of(whitespaceChars);
    text.erase(pos + 1);

    return text;
}

std::vector<std::string> explode(const std::string &text) {
    std::string::size_type lastpos = 0;
    std::string::size_type pos = 0;

    lastpos = text.find_first_not_of(whitespaceChars);

    std::vector<std::string> parts;
    while (lastpos != std::string::npos) {
        pos = text.find_first_of(whitespaceChars, lastpos);
        parts.push_back(text.substr(lastpos, pos - lastpos));
        lastpos = text.find_first_not_of(whitespaceChars, pos);
    }
    return parts;
}
