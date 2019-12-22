#include <string>
#include <vector>
#include "textutil.h"

const char *whitespaceChars = " \t\n\r";

bool c_isspace(int c) {
    return c == ' ';
}

void wordwrap(std::string &text, int width, std::vector<std::string> &lines) {
    std::string::size_type start = 0;
    std::string::size_type current = width;

    bool dobreak = false;
    while (current < text.size()) {
        dobreak = false;
        for (std::string::size_type pos = start; pos < current; ++pos) {
            if (text[pos] == '\n') {
                lines.push_back(trim(text.substr(start, pos - start)));
                start = pos + 1;
                current = start + width;
                dobreak = true;
            }
        }
        if (dobreak) continue;

        while (current > start && !c_isspace(text[current])) --current;
        if (current == start) {

        } else {
            text[current] = '\n';
            lines.push_back(trim(text.substr(start, current - start)));
            start = current + 1;
            current = start + width;
        }
    }

    if (start < text.size()) {
        lines.push_back(trim(text.substr(start)));
    }

}


int strToInt(const std::string &str) {
    char *endPtr;
    int result = strtol(str.c_str(), &endPtr, 10);
    if (*endPtr == 0) return result;
    return -1;
}

bool strToBool(std::string str) {
    for (char &c : str) {
        if (c > 32 && c < 127) c = std::tolower(c);
    }
    if (str == "yes" || str == "true" || str == "1") return true;
    return false;
}

std::string& convertSpaces(std::string &str) {
    std::string::size_type p = 0;
    while ( (p = str.find_first_of('_')) != std::string::npos ) {
        str[p] = ' ';
    }
    return str;
}

std::vector<std::string> explode(const std::string &str, std::string splitOn) {
    if (splitOn.empty()) splitOn = whitespaceChars;
    std::vector<std::string> parts;
    std::string::size_type p = 0, e = 0;

    while (e < str.size()) {
        p = str.find_first_not_of(splitOn, e);
        if (p == std::string::npos) return parts;
        e = str.find_first_of(splitOn, p + 1);
        parts.push_back(trim(str.substr(p, e - p)));
        if (e < std::string::npos) ++e;
    }

    return parts;
}

std::string trim(const std::string &inStr) {
    std::string str(inStr);

    std::string::size_type pos;

    pos = str.find_first_not_of(whitespaceChars);
    if (pos > 0) str.erase(0, pos);
    pos = str.find_last_not_of(whitespaceChars);
    str.erase(pos + 1);

    return str;
}

