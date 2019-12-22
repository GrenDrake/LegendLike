#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include "logger.h"
#include "physfs.h"

Logger *Logger::mLoggerInstance = nullptr;


Logger& Logger::getInstance() {
    static Logger logger;
    return logger;
}

void Logger::log(const std::string &text) {
    if (mLogFile) {
        PHYSFS_writeBytes(mLogFile, (text+'\n').c_str(), text.size() + 1);
    } else {
        std::cerr << text << "\n";
    }
}

void Logger::error(const std::string &text) {
    log("ERROR:  " + text);
}

void Logger::warn(const std::string &text) {
    log("WARN:   " + text);
}

void Logger::info(const std::string &text) {
    log("INFO:   " + text);
}

void Logger::debug(const std::string &text) {
    log("DEBUG:  " + text);
}

Logger::Logger() {
    mLogFile = PHYSFS_openWrite("game.log");
    if (!mLogFile) {
        std::cerr << "Could not open game log file for writing.\n";
    } else {
        std::stringstream line;
        line << "START: ";
        auto start = std::chrono::system_clock::now();
        std::time_t end_time = std::chrono::system_clock::to_time_t(start);
        line << std::ctime(&end_time);
        log(line.str());
    }
}

void Logger::endLog() {
    std::stringstream line;
    line << "\nEND:    ";
    auto start = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t(start);
    line << std::ctime(&end_time) << '\n';
    log(line.str());

   if (mLogFile) {
        PHYSFS_close(mLogFile);
    }
}