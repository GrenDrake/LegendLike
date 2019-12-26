#ifndef TEXTUTIL_H
#define TEXTUTIL_H

#include <string>
#include <vector>

bool c_isspace(int c);
void wordwrap(std::string &text, int width, std::vector<std::string> &lines);
int strToInt(const std::string &str);
bool strToBool(std::string str);
std::string& convertSpaces(std::string &str);
std::vector<std::string> explode(const std::string &str, std::string splitOn = "");
std::string trim(const std::string &inStr);
std::string& replaceAll(std::string &text, const std::string &from, const std::string &to);
std::string& unescapeString(std::string &text);
std::string upperFirst(std::string text);

#endif
