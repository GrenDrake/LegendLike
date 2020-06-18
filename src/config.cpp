#include <cstdlib>
#include <fstream>
#include <sstream>
#include "gamestate.h"
#include "config.h"
#include "physfs.h"
#include "logger.h"
#include "textutil.h"

bool Config::loadFromFile(const std::string &file) {
    Logger &log = Logger::getInstance();
    char *buffer = slurpFile(file);
    if (!buffer) {
        log.warn(std::string("Could not open configuration file ") + file);
        return false;
    }
    std::stringstream configFile(buffer);
    delete[] buffer;

    unsigned lineNo = 0;
    std::string line;
    while (std::getline(configFile, line)) {
        ++lineNo;
        std::string::size_type e = line.find_first_of('=');
        if (e == std::string::npos) {
            continue;
        }
        std::string left = trim(line.substr(0, e));
        std::string right = trim(line.substr(e + 1));
        mKeys.insert(std::make_pair(left, right));
    }

    log.info(std::string("Loaded configuration from ") + file);
    return true;
}

bool Config::writeToFile() const {
    PHYSFS_file *out = PHYSFS_openWrite("game.cfg");
    if (!out) {
        Logger &log = Logger::getInstance();
        log.error("Failed to open config file for writing.");
        return false;
    }
    for (const auto &i : mKeys) {
        std::string line = i.first + " = " + i.second + "\n";
        PHYSFS_writeBytes(out, line.c_str(), line.size());
    }
    PHYSFS_close(out);
    return true;
}

const std::string& Config::getString(const std::string &key, const std::string &defValue) const {
    auto iter = mKeys.find(key);
    if (iter != mKeys.end()) return iter->second;
    return defValue;
}

int Config::getInt(const std::string &key, int defValue) const {
    auto iter = mKeys.find(key);
    if (iter == mKeys.end()) return defValue;

    char *endPtr;
    int v = strtol(iter->second.c_str(), &endPtr, 10);
    if (*endPtr == 0) return v;
    else {
        Logger &log = Logger::getInstance();
        log.warn("Failed to read config key " + key + " (value: " + iter->second + ") as int.");
        return defValue;
    }
}

double Config::getDouble(const std::string &key, double defValue) const {
    auto iter = mKeys.find(key);
    if (iter == mKeys.end()) return defValue;

    char *endPtr;
    double v = strtod(iter->second.c_str(), &endPtr);
    if (*endPtr == 0) return v;
    else {
        Logger &log = Logger::getInstance();
        log.warn("Failed to read config key " + key + " (value: " + iter->second + ") as double.");
        return defValue;
    }
}

bool Config::getBool(const std::string &key, bool defValue) const {
    auto iter = mKeys.find(key);
    if (iter == mKeys.end()) return defValue;

    if (iter->second == "yes" || iter->second == "true" || iter->second == "1" || iter->second == "on") {
        return true;
    } else if (iter->second == "no" || iter->second == "false" || iter->second == "0" || iter->second == "off") {
        return false;
    } else {
        Logger &log = Logger::getInstance();
        log.warn("Failed to read config key " + key + " (value: " + iter->second + ") as int.");
        return defValue;
    }
}

void Config::set(const std::string &key, const std::string &value) {
    mKeys[key] = value;
}

void Config::set(const std::string &key, int value) {
    set(key, std::to_string(value));
}
